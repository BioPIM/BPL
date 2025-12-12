////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <ranges>
#include <benchmark.hpp>
#include <bpl/utils/RandomUtils.hpp>

#include <tasks/VectorChecksum.hpp>
#include <tasks/VectorChecksumOnce.hpp>
#include <tasks/VectorAdd.hpp>
#include <tasks/VectorReverseInPlace.hpp>
#include <tasks/SyracuseReduce.hpp>
#include <tasks/SyracuseVector.hpp>
#include <tasks/SketchJaccardDistance.hpp>
#include <tasks/SortSelectionVector.hpp>

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
    Benchmark::run (
		getDefaultLaunchers(),
		std::vector {20,22,24,26,28},
		[] (auto&& launcher, uint64_t input, size_t nbruns) {
            std::vector<uint32_t> v (1UL<<input);
            std::iota (std::begin(v), std::end(v), 1);
            return Benchmark::run<VectorChecksum> (launcher, nbruns, false, split(v));
        }
	);
}
//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorChecksumOnce", "[benchmark]" )
{
    Benchmark::run (
        getDefaultLaunchers(),
        std::vector {20,22,24,26,28},
        [] (auto&& launcher, uint64_t input, size_t nbruns) {
            std::vector<uint32_t> v (1UL<<input);
            std::iota (std::begin(v), std::end(v), 1);
            return Benchmark::run<VectorChecksumOnce> (launcher, nbruns, true, split(v));
        }
    );
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAdd", "[benchmark]" )
{
    Benchmark::run (
		getDefaultLaunchers(),
		std::views::iota(10,20),
		[] (auto&& launcher, uint64_t input, size_t nbruns) {
            std::vector<uint32_t> v1 (1UL<<input);
            std::vector<uint32_t> v2 (1UL<<input);
            std::iota (std::begin(v1), std::end(v1),  100);
            std::iota (std::begin(v2), std::end(v2), 1000);
            return Benchmark::run<VectorAdd> (launcher, nbruns, false, split(v1), split(v2));
        }
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorReverseInPlace", "[benchmark]" )
{
    Benchmark::run (
		getDefaultLaunchers(),
		std::views::iota(10,18),
		[] (auto&& launcher, uint64_t input, size_t nbruns) {
            using value_type = VectorReverseInPlace<ArchMulticore>::value_type;
            std::vector<value_type> v (1UL<<input);
            std::iota (std::begin(v), std::end(v), 1);
            return Benchmark::run<VectorReverseInPlace> (launcher, nbruns, false, split(v));
        }
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SyracuseReduce", "[benchmark]" )
{
    Benchmark::run (
		getDefaultLaunchers(),
		std::views::iota(20,31),
		[] (auto&& launcher, uint64_t input, size_t nbruns) {
            std::pair<uint64_t,uint64_t> range (1, 1ULL<<input);
            return Benchmark::run<SyracuseReduce> (launcher, nbruns, false, split(range));
        }
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SyracuseVector", "[benchmark]" )
{
    Benchmark::run (
		getDefaultLaunchers(),
		std::views::iota(20,29),
		[] (auto&& launcher, uint64_t input, size_t nbruns) {
            std::pair<uint64_t,uint64_t> range (1, 1ULL<<input);
            return Benchmark::run<SyracuseVector> (launcher, nbruns, false, split(range));
        }
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SketchJaccardDistance", "[benchmark]" )
{
    Benchmark::run (
		getDefaultLaunchers(),
		std::views::iota(17,20),
		[] (auto&& launcher, uint64_t input, size_t nbruns) {

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

            return Benchmark::run<SketchJaccardDistance> (launcher, nbruns, false, split(ref), qry, SSIZE);
        }
	);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SortSelection", "[benchmark]" )
{
    Benchmark::run (
        getLaunchersSameSplitStrategy(),
        std::views::iota(20,24), // log2 nb items
        [] (auto&& launcher, uint64_t input, size_t nbruns) {

            using vector_type = typename SortSelection<ArchDummy>::vector_t;
            using value_type  = typename vector_type::value_type;

            auto v = get_random_permutation<value_type> (1UL << input);

            return Benchmark::run<SortSelection> (launcher, nbruns, false, split(v));
        }
    );
}
