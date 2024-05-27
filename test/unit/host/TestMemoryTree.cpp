////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/utils/MemoryTree.hpp>

using namespace bpl::core;

//////////////////////////////////////////////////////////////////////////////
class MyAllocator
{
public:
    using address_t = uint64_t;

    static constexpr bool is_freeable = true;

    static void free (address_t a)  {  delete[] (char*)a;  }

    static address_t write (void* src, size_t n)
    {
        return (address_t) memcpy (new char [n], src, n);
    };

    static address_t* read (address_t src, void* tgt, size_t n)
    {
        return (address_t*) memcpy (tgt, (void*)src, n);
    };
};

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("MemoryTree1", "[MemoryTree]" )
{
    static constexpr int NBITEMS_PER_BLOCK_LOG2 = 2;

    MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,10> memtree;

    size_t imax = 1 << (NBITEMS_PER_BLOCK_LOG2+2);
    size_t i    = 0;

    for (i=1; i<=imax; i++)  {  memtree.insert (i);  }

    i=0;
    memtree.leaves ([&] (auto a)  {  REQUIRE (a == ++i);  });
    REQUIRE (i == imax);

    i=0;
    memtree.DFS([&] (int depth, auto a)  {  if (depth==0)  {  REQUIRE (a == ++i);  }  });
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("MemoryTree2", "[MemoryTree]" )
{
    static constexpr int NBITEMS_PER_BLOCK_LOG2 = 2;

    MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,10> memtree;

    size_t imax = 1 << (NBITEMS_PER_BLOCK_LOG2+2);

    for (size_t i=1; i<=imax; i++)  {  memtree.insert (i);  }

    size_t check = 1;
    memtree.DFS ([&] (int depth, auto a)
    {
        if (depth==0)
        {
            REQUIRE (a == check++);
        }
    });
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("MemoryTree3", "[MemoryTree]" )
{
    static constexpr int NBITEMS_PER_BLOCK_LOG2 = 2;

    auto fct  = []  (size_t imax)
    {
        MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,10> memtree;

        for (size_t i=1; i<=imax; i++)  {  memtree.insert (i);  }

        uint64_t truth    = imax*(imax+1)/2;
        uint64_t checksum = 0;
        for (auto a : memtree)
        {
            checksum += a;
        }
        REQUIRE (checksum == truth);
    };

    for (size_t i=1; i<=10000; i++)  {  fct (i);  }
}
