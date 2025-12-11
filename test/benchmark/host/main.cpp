////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <ranges>
#include <benchmark.hpp>

#include <tasks/SyracuseReduce.hpp>
#include <tasks/SyracuseVector.hpp>
#include <tasks/VectorCreation.hpp>
#include <tasks/VectorSerialize.hpp>
#include <tasks/VectorReverseInPlace.hpp>
#include <tasks/VectorReverseIterator.hpp>
#include <tasks/VectorChecksum.hpp>
#include <tasks/VectorAdd.hpp>
#include <tasks/SketchJaccardDistance.hpp>
#include <tasks/SketchJaccardDistanceOnce.hpp>
#include <tasks/SortSelectionVector.hpp>

#include <bpl/utils/RandomUtils.hpp>

//////////////////////////////////////////////////////////////////////////////
auto getDefaultUpmemView() {
    return std::views::iota(1,25) | std::views::transform([](auto nbrank) {
        return Launcher<ArchUpmem> { ArchUpmem::DPU(64*nbrank) };
    });
}

auto getDefaultMulticoreView() {
    return std::views::iota(1,25) | std::views::transform([](auto n) {
        return Launcher<ArchMulticore> { ArchMulticore::Thread(n), 32_thread};
    });
}

auto getDefaultMulticoreView_pow2() {
    return std::views::iota(0,6) | std::views::transform([](auto nbthreadLog2) {
        return Launcher<ArchMulticore> { ArchMulticore::Thread(1<<nbthreadLog2), 32_thread};
    });
}

//////////////////////////////////////////////////////////////////////////////
auto getDefaultLaunchers () {
	return std::make_tuple(
		getDefaultUpmemView(),
		getDefaultMulticoreView()
	);
}

//////////////////////////////////////////////////////////////////////////////
// returns launcher that have the same split strategy
auto getLaunchersSameSplitStrategy() {

    // Special case here: we want the number of process units to be the same for upmem and multicore
    auto upmem = std::views::iota(1,25) | std::views::transform([](auto nbrank) {
        return Launcher<ArchUpmem> { ArchUpmem::DPU(64*nbrank) };
    });

    auto multicore = std::views::iota(1,25) | std::views::transform([](auto nbrank) {
        return Launcher<ArchMulticore> { ArchMulticore::Thread(16*64*nbrank), ArchMulticore::Thread(nbrank)};
    });

    return std::make_tuple (upmem, multicore);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorChecksum", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        std::vector<uint32_t> v (1UL<<input);
        std::iota (std::begin(v), std::end(v), 1);
        auto t = Benchmark::run<VectorChecksum> (launcher, nbruns, false, split(v));
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run ("VectorChecksum",
		getDefaultLaunchers(),
		std::views::iota(10,20),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAdd", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        std::vector<uint32_t> v1 (1UL<<input);
        std::vector<uint32_t> v2 (1UL<<input);
        std::iota (std::begin(v1), std::end(v1),  100);
        std::iota (std::begin(v2), std::end(v2), 1000);
        auto t = Benchmark::run<VectorAdd> (launcher, nbruns, false, split(v1), split(v2));
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run ("VectorAdd",
		getDefaultLaunchers(),
		std::views::iota(10,20),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorCreation", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        auto t = Benchmark::run<VectorCreation> (launcher, nbruns, false, 1ULL<<input);
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run (
		"VectorCreation",
		getDefaultLaunchers(),
		std::views::iota(10,16),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorSerialize", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        using type = typename VectorSerialize<ArchDummy>::type;
        std::vector<type> truth (1UL<<input);
        std::iota (std::begin(truth), std::end(truth), 1);
        auto t = Benchmark::run<VectorSerialize> (launcher, nbruns, false, truth);
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run (
		"VectorSerialize",
		getDefaultLaunchers(),
		std::views::iota(10,19),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorReverseInPlace", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        using value_type = VectorReverseInPlace<ArchMulticore>::value_type;
        std::vector<value_type> v (1UL<<input);
        std::iota (std::begin(v), std::end(v), 1);
        auto t = Benchmark::run<VectorReverseInPlace> (launcher, nbruns, false, split(v));
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run (
		"VectorReverseInPlace",
		getDefaultLaunchers(),
		std::views::iota(10,18),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorReverseIterator", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        std::vector<uint32_t> v (1UL<<input);
        std::iota (std::begin(v), std::end(v), 1);
        auto t = Benchmark::run<VectorReverseIterator> (launcher, nbruns, false, v,0);
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run (
		"VectorReverseIterator",
		getDefaultLaunchers(),
		std::views::iota(10,18),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SyracuseReduce", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        std::pair<uint64_t,uint64_t> range (1, 1ULL<<input);
        auto t = Benchmark::run<SyracuseReduce> (launcher, nbruns, false, split(range));
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run ("SyracuseReduce",
		getDefaultLaunchers(),
		std::views::iota(20,31),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SyracuseVector", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {
        std::pair<uint64_t,uint64_t> range (1, 1ULL<<input);
        auto t = Benchmark::run<SyracuseVector> (launcher, nbruns, false, split(range));
        callback (taskname, launcher, t, input, nbruns);
    };
    Benchmark::run (
		"SyracuseVector",
		getDefaultLaunchers(),
		std::views::iota(20,29),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SketchJaccardDistance", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {

        using hash_t = SketchJaccardDistance<ArchDummy>::hash_t;
        size_t SSIZE = 1000;

        size_t refSize = 1UL<<input;
        size_t qrySize = 100;

        std::vector<hash_t> ref;
        std::vector<hash_t> qry;

        size_t refCoeff = 1;
        size_t qryCoeff = 2;

        for (size_t i=1; i<=refSize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  ref.push_back (refCoeff*j);   }  }
        for (size_t i=1; i<=qrySize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  qry.push_back (qryCoeff*j);   }  }

    	auto t = Benchmark::run<SketchJaccardDistance> (launcher, nbruns, false, split(ref), qry, SSIZE);
        callback (taskname, launcher, t, input, nbruns);
    };

    Benchmark::run (
		"SketchJaccardDistance",
		getDefaultLaunchers(),
		std::views::iota(17,20),
		fct
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SortSelection", "[benchmark]" )
{
    auto fct = [] (const char* taskname, auto&& launcher, uint64_t input, auto callback, size_t nbruns) {

        using vector_type = typename SortSelection<ArchDummy>::vector_t;
        using value_type  = typename vector_type::value_type;

        auto v = get_random_permutation<value_type> (1UL << input);

        auto t = Benchmark::run<SortSelection> (launcher, nbruns, false, split(v));
        callback (taskname, launcher, t, input, nbruns);
    };

    Benchmark::run (
        "SortSelection",
        getLaunchersSameSplitStrategy(),
        std::views::iota(20,24), // log2 nb items
        fct
    );
}
