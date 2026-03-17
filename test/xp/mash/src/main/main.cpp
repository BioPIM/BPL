////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <print>
#include <ranges>
#include <execution>
#include <source_location>
#include <BS_thread_pool.hpp>

extern "C" {
#include <dpu.h>
}

// We want to use the BPL API.
#include <bpl/bpl.hpp>

// We need to know the task we want to run.
#include <tasks/SketchJaccard.hpp>

#define DPU_BINARY "benchmark_sdk_pim"

using namespace bpl;

constexpr size_t SSIZE = 10'000;

using hash_t = SketchJaccard<ArchDummy>::hash_t;

////////////////////////////////////////////////////////////////////////////////
template<typename hash_t, std::size_t SIZE>
struct SketchesDatabase {
    using sketch_t = std::array<hash_t, SIZE>;

    SketchesDatabase() = default;
    SketchesDatabase(SketchesDatabase const&) = delete;
    SketchesDatabase(SketchesDatabase &&)     = default;

    std::vector<hash_t> db;  // flattened sketches

    std::span<hash_t> get() { return db; }  // flattened sketches

    auto add (sketch_t const& sk) {
        db.insert (db.end(), std::begin(sk), std::end(sk));
    }

    auto size() const { return db.size()/SIZE; }

    auto addRandom () {
    	sketch_t s;
    	for (auto& e : s) {  e = std::rand()%4; }
    	std::sort(std::execution::par, s.begin(),s.end());
    	add (s);
    }

    operator std::vector<hash_t> const& () { return db; }
    operator std::span<hash_t>          () const {
    	return std::span<hash_t> (const_cast<SketchesDatabase&>(*this).db) ; }
};

////////////////////////////////////////////////////////////////////////////////
/** \brief Type trait specialization in case the incoming type is a SplitProxy */
template<typename hash_t, std::size_t SIZE>
struct SplitOperator<SketchesDatabase<hash_t,SIZE>>  {
    static decltype(auto) split (SketchesDatabase<hash_t,SIZE> const& t, std::size_t idx, std::size_t total)  {
        return SplitOperator<std::vector<hash_t>>::split(t.db, idx, total);
    }
    static decltype(auto) split_view (SketchesDatabase<hash_t,SIZE> const& t, std::size_t idx, std::size_t total) {
        return SplitOperator<std::vector<hash_t>>::split_view(t.db, idx, total);
    }
};

////////////////////////////////////////////////////////////////////////////////
auto dump (
	std::string_view label,
	std::size_t ref,
	std::size_t qry,
	std::size_t nbunits,
	double duration,
	std::uint64_t checksum
) {
	std::println ("type: {:20}  #ref: {:6}  #qry: {:4}  nbunits: {:3}   duration: {:7.3f}  checksum: {:12}",
		label, ref, qry, nbunits, duration, checksum);

}

////////////////////////////////////////////////////////////////////////////////
auto benchmark_native_multicore (
	std::string_view label,
	auto const& dbRef,
	auto const& dbQry,
	std::size_t nbthreads
)
{
    auto compute = [] (auto  ref, auto qry) {
        size_t score = 0;

        auto refBegin = ref.begin();   auto refEnd = ref.end();
        auto qryBegin = qry.begin();   auto qryEnd = qry.end();

        // loop that compares two sketches
        while (true)   {
                if (*refBegin < *qryBegin)   {   if (++refBegin == refEnd)  { break; }   }
           else if (*refBegin > *qryBegin)   {   if (++qryBegin == qryEnd)  { break; }   }
           else {
               score++;
               if (++refBegin == refEnd)  { break; }
               if (++qryBegin == qryEnd)  { break; }
           }
        }
        return score;
    };

	double t1 = timestamp();
	BS::thread_pool<> threadpool (nbthreads);

	std::size_t na = dbRef.db.size() / SSIZE;
	std::size_t nb = dbQry.db.size() / SSIZE;

	std::vector<std::vector<std::uint64_t>> results (na, std::vector<std::uint64_t>(nb));

	for (std::size_t i=0; i<na; i++)  {
		auto ref = std::span<const hash_t>(dbRef.db.begin()+(i+0)*SSIZE,dbRef.db.begin()+(i+1)*SSIZE);
		threadpool.detach_task ([&,ref,i] {
			std::vector<std::uint64_t>& res = results[i];
			for (std::size_t j=0; j<nb; j++)  {
				auto qry = std::span<const hash_t>(dbQry.db.begin()+(j+0)*SSIZE,dbQry.db.begin()+(j+1)*SSIZE);
				res[j] = compute (ref,qry);
			}
		});
	}

	threadpool.wait();
	double t2 = timestamp();

	uint64_t totalScore = 0;
	for (auto const& v : results)  {
		totalScore += std::accumulate(v.begin(),v.end(), uint64_t{0});
	}

	dump (label, dbRef.size(), dbQry.size(), nbthreads, (t2-t1)/1000000, totalScore);
}

////////////////////////////////////////////////////////////////////////////////
auto benchmark_sdk_pim(
	std::string_view label,
	auto const& dbRef,
	auto const& dbQry,
	std::size_t nbunits
)
{
	std::size_t nbDPU = 64*nbunits;

	struct dpu_set_t set, dpu;
    uint32_t each_dpu=0;

    // as for the BPL, alloc/dealloc of DPU is not taken into account in exec time.
	DPU_ASSERT(dpu_alloc(nbDPU, NULL, &set));

	double t1 = timestamp();

	DPU_ASSERT(dpu_load(set, DPU_BINARY, NULL));

	uint64_t refSize = dbRef.db.size();
	uint64_t qrySize = dbQry.db.size();
	uint64_t nbRefPerDpu = refSize / nbDPU;

	DPU_FOREACH(set, dpu, each_dpu) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, (void*) dbQry.db.data()));
	}
	DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "qry", 0, qrySize*sizeof(uint32_t), DPU_XFER_DEFAULT));

	DPU_FOREACH(set, dpu, each_dpu) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, (void*) &(dbRef.db[each_dpu*nbRefPerDpu])));
	}
	DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_TO_DPU, "ref", 0, nbRefPerDpu*sizeof(uint32_t), DPU_XFER_DEFAULT));

	DPU_ASSERT(dpu_broadcast_to(set, "refSize", 0, &nbRefPerDpu, sizeof(nbRefPerDpu), DPU_XFER_DEFAULT));
	DPU_ASSERT(dpu_broadcast_to(set, "qrySize", 0, &qrySize,     sizeof(qrySize),     DPU_XFER_DEFAULT));

	DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    std::vector<uint32_t> results (dbRef.size()*dbQry.size());
	std::size_t div = results.size()/nbDPU;

    each_dpu=0;
	DPU_FOREACH(set, dpu, each_dpu) {
		DPU_ASSERT(dpu_prepare_xfer(dpu, &results[each_dpu*div]));
	}
	DPU_ASSERT(dpu_push_xfer(set, DPU_XFER_FROM_DPU, "results", 0, div*sizeof(uint32_t), DPU_XFER_DEFAULT));

	double t2 = timestamp();

	DPU_ASSERT(dpu_free(set));

	auto sum = std::accumulate(results.begin(),results.end(), uint64_t{0});

	dump (label, dbRef.size(), dbQry.size(), nbunits, (t2-t1)/1000000, sum);
}

////////////////////////////////////////////////////////////////////////////////
auto process (std::string_view label, auto& launcher, auto const& dbRef, auto const& dbQry, size_t nbunits)
{
	double t0 = timestamp();
	auto res  = launcher.template run<SketchJaccard> (split(dbRef), dbQry, SSIZE);
	double t1 = timestamp();

	std::uint64_t check=0;  for (auto&& res : res)  { check += std::accumulate(res.begin(),res.end(), std::uint64_t{}); }

	dump (label, dbRef.size(), dbQry.size(), nbunits, (t1-t0)/1000000, check);
}

////////////////////////////////////////////////////////////////////////////////
auto benchmark_bpl_multicore (
	std::string_view label,
	auto const& dbRef,
	auto const& dbQry,
	std::size_t nbunits
)
{
	Launcher<ArchMulticore> launcher {
		ArchMulticore::Thread(dbRef.size()),
		ArchMulticore::Thread(std::min<int>(24,nbunits))
	};

	process (label, launcher, dbRef, dbQry, nbunits);
}

////////////////////////////////////////////////////////////////////////////////
auto benchmark_bpl_pim(
	std::string_view label,
	auto const& dbRef,
	auto const& dbQry,
	std::size_t nbunits
)
{
	Launcher<ArchUpmem> launcher {ArchUpmem::DPU(64*nbunits)};
	process (label, launcher, dbRef, dbQry, nbunits);
}

////////////////////////////////////////////////////////////////////////////////
auto getSketchesDatabases (size_t nbSketchRef, size_t nbSketchQry)
{
    SketchesDatabase<hash_t,SSIZE> dbRef;
    SketchesDatabase<hash_t,SSIZE> dbQry;

	srand(0);
    for (size_t i=1; i<=nbSketchRef; i++)  {  dbRef.addRandom(); }
    for (size_t i=1; i<=nbSketchQry; i++)  {  dbQry.addRandom(); }

    return std::make_pair(std::move(dbRef),std::move(dbQry));
}

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
	//	if (argc<4)  {
	//		std::println ("(1) type of test:");
	//		std::println ("   0: all tests");
	//		std::println ("   1: benchmark_native_multicore");
	//		std::println ("   2: benchmark_bpl_multicore");
	//		std::println ("   3: benchmark_sdk_pim");
	//		std::println ("   4: benchmark_bpl_pim");
	//		std::println ("(2) number of sketches in reference db");
	//		std::println ("(3) number of sketches in query db");
	//		std::println ("(4) number of units (ranks or threads)");
	//		return 1;
	//	}

    int choice = argc>1 ? std::atoi(argv[1]) : 0;

	auto vw = [] (std::string_view str) {  return
		  std::views::split(str,',')
		| std::views::transform([](auto s) -> std::size_t { return std::atol(std::string(s.begin(),s.end()).c_str()); });
	};

	for (auto [nbSketchRef,nbSketchQry,nbunits] : std::views::cartesian_product(
		vw(argc>2 ? argv[2] : "24576,122880,245760"),
		vw(argc>3 ? argv[3] : "16,128"),
		vw(argc>4 ? argv[4] : "3,6,12,24")
	))
	{
	    auto [dbRef,dbQry] = getSketchesDatabases(nbSketchRef,nbSketchQry);

	    if (choice==0 or choice==1)  { benchmark_native_multicore ("native_multicore", dbRef, dbQry, nbunits); }
	    if (choice==0 or choice==2)  { benchmark_bpl_multicore    ("bpl_multicore",    dbRef, dbQry, nbunits); }
	    if (choice==0 or choice==3)  { benchmark_sdk_pim          ("sdk_pim",          dbRef, dbQry, nbunits); }
	    if (choice==0 or choice==4)  { benchmark_bpl_pim          ("bpl_pim",          dbRef, dbQry, nbunits); }
	}
}
