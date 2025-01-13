////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>
#include <bpl/arch/ArchDummy.hpp>
#include <bpl/utils/split.hpp>

using namespace bpl;

#include <tasks/SketchJaccard.hpp>
#include <tasks/SketchJaccardOptim.hpp>
#include <tasks/SketchJaccardTopK.hpp>

//////////////////////////////////////////////////////////////////////////////
template<typename PROC_UNIT, typename LAUNCHER>
void SketchJaccard_aux (LAUNCHER& launcher, const std::vector<size_t>& sizes, bool require=true)
{
    using sketch_t = SketchJaccard<ArchUpmem>::sketch_t;
    static constexpr int SSIZE = std::tuple_size<sketch_t> {};

    //using arch_t  = typename LAUNCHER::arch_t;

    for (size_t nbSketches : sizes)
    {
        //printf ("nbSketches: %ld\n", nbSketches);

        std::vector<sketch_t> db;
        for (size_t i=1; i<=nbSketches; i++)
        {
            sketch_t s;
            for (size_t j=0; j<s.size(); j++)  {  s[j] = (i+j);   }
            db.push_back (s);
        }

        auto result = launcher.template run<SketchJaccard> (db, db);

        size_t tuid = 0;
        for (const auto& res : result)
        {
            if (require)  { REQUIRE (res.size() == nbSketches*nbSketches); }

            size_t idx = 0;
            for (auto x : res)
            {
                int row    = idx / db.size();
                int col    = idx % db.size();
                auto check = std::max (0, SSIZE - std::max(0,std::abs(row-col)));
                if (require)  { REQUIRE (x == check);  }
                if (require and x != check)  { printf ("ERROR: tuid=%ld nbSketches=%ld  x=%d check=%d  (%3d,%3d) %5ld \n", tuid, nbSketches, x, check, row, col, idx); }
                idx ++;
            }

            tuid++;
        }

        if (false)
        {
            for (const auto& res : result)
            {
                size_t i=0;
                for (auto x : res)  { printf ("%d ", x);  if ( (++i)%db.size()==0)  { printf("\n"); } }
                printf ("\n");
            }
        }

    } // end of for (size_t nbSketches : sizes)
}

template<typename PROC_UNIT, typename ...ARGS>
void Test_SketchJaccard (PROC_UNIT pu, ARGS... args)
{
    using arch_t = typename PROC_UNIT::arch_t;

    Launcher<arch_t> launcher (pu, args...);

    SketchJaccard_aux<PROC_UNIT> (launcher, {1<<3, 1<<4, 1<<5, 1<<6, 1<<7});

    std::vector<size_t> sizes; for (size_t i=1; i<=50; i++)  { sizes.push_back(i); }
    SketchJaccard_aux<PROC_UNIT> (launcher, sizes);
}

TEST_CASE ("SketchJaccard", "[Sketch]" )
{
    Test_SketchJaccard (ArchUpmem::DPU {1});
    Test_SketchJaccard (ArchMulticore::Thread {1});
}

//////////////////////////////////////////////////////////////////////////////
template<typename PROC_UNIT, typename ...ARGS>
void Test_SketchJaccard0 (PROC_UNIT pu, ARGS... args)
{
    using arch_t = typename PROC_UNIT::arch_t;

    Launcher<arch_t> launcher (pu, args...);

    SketchJaccard_aux<PROC_UNIT> (launcher, {256}, false);

    //launcher.getStatistics().dump(true);
}

TEST_CASE ("SketchJaccard0", "[Sketch]" )
{
    Test_SketchJaccard0 (ArchUpmem::Rank (1));
    Test_SketchJaccard0 (ArchMulticore::Thread {16});
}

//////////////////////////////////////////////////////////////////////////////
template<typename PROC_UNIT, typename ...ARGS>
void Test_SketchJaccard1 (PROC_UNIT pu, ARGS... args)
{
    using arch_t = typename PROC_UNIT::arch_t;

    Launcher<arch_t> launcher (pu, args...);

    using sketch_t = typename SketchJaccard <arch_t>::sketch_t;

    static constexpr size_t SKETCH_SIZE = std::tuple_size<sketch_t> {};

    auto init = [&] (sketch_t& sk, size_t init)
    {
        for (size_t i=0; i<SKETCH_SIZE; i++)  { sk[i] = init + i; }
    };

    std::vector<sketch_t> ref(2);  init(ref[0], 2);  init(ref[1], 3);
    std::vector<sketch_t> qry(2);  init(qry[0], 1);  init(qry[1], 2);

    auto result = launcher.template run<SketchJaccard> (ref, qry);

    for (const auto& res : result)
    {
        REQUIRE (res.size() == ref.size()*qry.size());

        REQUIRE (res[0] == SKETCH_SIZE - 1);
        REQUIRE (res[1] == SKETCH_SIZE - 2);
        REQUIRE (res[2] == SKETCH_SIZE - 0);
        REQUIRE (res[3] == SKETCH_SIZE - 1);
    }
}

TEST_CASE ("SketchJaccard1", "[Sketch]" )
{
    Test_SketchJaccard1 (ArchUpmem    ::DPU    { 1}, false);
    Test_SketchJaccard1 (ArchMulticore::Thread {16});
}

//////////////////////////////////////////////////////////////////////////////
template<typename hash_t>
std::vector<hash_t> getSketches (const char* filename, size_t ssize, size_t nbSketches, size_t firstSketch = 0)
{
    std::vector<hash_t> result (ssize * nbSketches);

    FILE* fp = fopen (filename,"rb");
    if (fp!=nullptr)
    {
        fseek (fp, firstSketch*ssize, SEEK_SET);
        fread (result.data(), sizeof(hash_t), ssize*nbSketches, fp);
        fclose(fp);
    }

    return result;
}

//////////////////////////////////////////////////////////////////////////////
template<typename CONFIG>
auto SketchJaccardTopK_aux (double& runtime, size_t nbSketchRef = 10, size_t nbSketchQry = 100, size_t SSIZE = 10000)
{
    auto timestamp = []
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
    };

    size_t nbunits = CONFIG::nbunits;

    Launcher<typename CONFIG::arch_t> launcher (typename CONFIG::level_t {nbunits}, false);

    using hash_t = typename SketchJaccardTopK<typename CONFIG::arch_t>::hash_t;

    size_t refSize = nbSketchRef * launcher.getProcUnitNumber();
    size_t qrySize = nbSketchQry;

    std::vector<hash_t> ref;
    std::vector<hash_t> qry;

    [[maybe_unused]] size_t refCoeff = 1;
    [[maybe_unused]] size_t qryCoeff = 1;

#if 1

    REQUIRE (SSIZE % qryCoeff == 0);

    for (size_t i=1; i<=refSize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  ref.push_back (refCoeff*j);   }  }
    for (size_t i=1; i<=qrySize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  qry.push_back (qryCoeff*j);   }  }

//    srand(0);  for (size_t i=1; i<=refSize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  ref.push_back (rand());   }  }
//    srand(0);  for (size_t i=1; i<=qrySize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  qry.push_back (rand());   }  }

#else
    const char* sketchesFilename = "/data/edrezen/data/sketches_all_2.bin";
    ref = getSketches<hash_t> (sketchesFilename, SSIZE, refSize);
    qry = getSketches<hash_t> (sketchesFilename, SSIZE, qrySize, refSize);
#endif

//    printf ("nbSketchRef: %ld   nbSketchQry: %ld  #ref: %ld   #qry: %ld    #pu: %ld   sizeof(hash_t): %ld \n",
//        nbSketchRef, nbSketchQry, ref.size(), qry.size(), launcher.getProcUnitNumber(), sizeof(hash_t)
//    );

    double t0 = timestamp();

    auto result = launcher.template run<SketchJaccardTopK> (split(ref), qry, SSIZE);

    double t1 = timestamp();

    runtime = t1-t0;

    //printf ("#result: %ld\n", result.size());

    [[maybe_unused]] size_t k=0;
    for ([[maybe_unused]] auto&& res : result)
    {
        REQUIRE (res.size() == nbSketchRef*nbSketchQry);
        for (auto x : res)  { REQUIRE(x==SSIZE/qryCoeff); }
        //printf ("\n[%3ld]   %ld  => ", k++, res.size());  for (auto x : res)  { printf ("%6d ", x); }   printf ("\n");
    }

    //launcher.getStatistics().dump(true);

    return result;
}

struct config1
{
    struct upmem
    {
        using arch_t  = ArchUpmem;
        using level_t = ArchUpmem::DPU;
        using unit_t  = ArchUpmem::Tasklet;
        static const int nbunits = 1;
    };

    struct multicore
    {
        using arch_t  = ArchMulticore;
        using level_t = ArchMulticore::Thread;
        using unit_t  = ArchMulticore::Thread;
        static const int nbunits = 16*upmem::nbunits;
    };
};

TEST_CASE ("SketchJaccardTopK", "[Sketch]" )
{
    size_t nbSketchRef = 16;
    size_t nbSketchQry = 100;
    size_t SSIZE       = 10000;

//    nbSketchRef = 16;
//    nbSketchQry = 1200;

//    nbSketchRef = 10;
//    nbSketchQry = 100;

//    nbSketchRef = 2;
//    nbSketchQry = 4;
//    SSIZE       = 100;

//    nbSketchRef = 10;
//    nbSketchQry = 13;
//    SSIZE = 10000;

    [[maybe_unused]] double t1;
    auto res1 = SketchJaccardTopK_aux<config1::upmem>     (t1, nbSketchRef, nbSketchQry, SSIZE);

    [[maybe_unused]] double t2;
    auto res2 = SketchJaccardTopK_aux<config1::multicore> (t2, nbSketchRef, nbSketchQry, SSIZE);

    //printf ("UPMEM: %.2f  MULTICORE: %.2f\n", t1,t2);

    REQUIRE (res1.size() == res2.size());

    auto it1 = res1.begin();   auto end1 = res1.end();
    auto it2 = res2.begin();   auto end2 = res2.end();

    for (size_t k=0; it1!=end1 and it2!=end2; ++it1, ++it2, ++k)
    {
        REQUIRE ((*it1).size() == (*it2).size());
        for (size_t j=0; j<(*it1).size(); j++)   {   REQUIRE ((*it1)[j] == (*it2)[j]);   }
    }
}

//////////////////////////////////////////////////////////////////////////////

TEST_CASE ("SketchJaccardTopKSeries", "[Sketch]" )
{
#if 1
    struct config
    {
        using arch_t  = ArchUpmem;
        using level_t = ArchUpmem::Rank;
        //using unit_t  = ArchUpmem::Tasklet;
    };
#else
    struct config
    {
        using arch_t  = ArchMulticore;
        using level_t = ArchMulticore::Thread;
        using unit_t  = ArchMulticore::Thread;
    };
#endif


for (size_t qryCoeff : { 1, 2, 5, 10, 50, 100, 500, 1000, 2000, 5000, 10000})
{
    size_t nbunits = 1;

    Launcher<config::arch_t> launcher (config::level_t(nbunits), false);

    using hash_t = SketchJaccard<config::arch_t>::hash_t;
    size_t SSIZE = 10000;

    size_t nbSketchRef = 10;
    size_t nbSketchQry = 70;

    size_t refSize = nbSketchRef * launcher.getProcUnitNumber();
    size_t qrySize = nbSketchQry;

    std::vector<hash_t> ref;
    std::vector<hash_t> qry;

    size_t refCoeff = 1;
    //size_t qryCoeff = 10000;


    REQUIRE (SSIZE % qryCoeff == 0);

    for (size_t i=1; i<=refSize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  ref.push_back (refCoeff*j);   }  }
    for (size_t i=1; i<=qrySize; i++)  {  for (size_t j=0; j<SSIZE; j++)  {  qry.push_back (qryCoeff*j);   }  }

//    printf ("nbSketchRef: %ld   nbSketchQry: %ld  #ref: %ld   #qry: %ld    #pu: %ld \n",
//        nbSketchRef, nbSketchQry, ref.size(), qry.size(), launcher.getProcUnitNumber()
//    );

    auto result = launcher.template run<SketchJaccardTopK> (split(ref), qry, SSIZE);

    [[maybe_unused]] size_t k=0;
    for (auto&& res : result)
    {
        REQUIRE (res.size() == nbSketchRef*nbSketchQry);

        for (auto x : res)  { REQUIRE(x==SSIZE/qryCoeff); }

        //printf ("\n[%3ld]   %ld  => ", k++, res.size());  for (auto x : res)  { printf ("%6d ", x); }   printf ("\n");
    }

    //launcher.getStatistics().dump(true);

    //printf ("%4ld %.3f \n ", qryCoeff, launcher.getStatistics().getTiming("run/launch"));
}
}

//////////////////////////////////////////////////////////////////////////////

TEST_CASE ("SketchJaccard2", "[Sketch]" )
{
    Launcher<ArchUpmem> launcher (1_rank, false);

    using sketch_t = SketchJaccard<ArchUpmem>::sketch_t;

    size_t refSize = 500000;

    std::vector<sketch_t> ref;
    for (size_t i=1; i<=refSize; i++)
    {
        sketch_t s;
        for (size_t j=0; j<s.size(); j++)  {  s[j] = (i+j);   }
        ref.push_back (s);
    }

    std::vector<sketch_t> qry;
    for (size_t i=1; i<=128; i++)
    {
        sketch_t s;
        for (size_t j=0; j<s.size(); j++)  {  s[j] = (i+j);   }
        qry.push_back (s);
    }

    //printf ("#ref: %ld   #qry: %ld \n", ref.size(), qry.size());

    auto result = launcher.template run<SketchJaccard> (split(ref), qry);

    //launcher.getStatistics().dump(true);
}
