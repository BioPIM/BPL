////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <common.hpp>

#include <bpl/utils/MemoryTree.hpp>

using namespace bpl;

//////////////////////////////////////////////////////////////////////////////
class MyAllocator
{
public:
    using address_t = uint64_t;

    template<int N>  using address_array_t = address_t [N];

    static constexpr bool is_freeable = true;

    static void free (address_t a)  {  delete[] (char*)a;  }

    static address_t write (void* src, size_t n)
    {
        return (address_t) memcpy (new char [n], src, n);
    };

    static address_t* read (void*  src, void* tgt, size_t n)
    {
        return (address_t*) memcpy (tgt, (void*)src, n);
    };

    static address_t writeAt (void*  tgt, void* src, size_t n)
    {
        return (address_t) memcpy ((void*)tgt, (void*)src, n);
    };

    static auto writeAtomic (address_t tgt, address_t src)
    {
        writeAt ((void*)tgt, (void*)&src, sizeof(address_t));
    }
};

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("MemoryTree1", "[MemoryTree]" )
{
    static constexpr int NBITEMS_PER_BLOCK_LOG2 = 2;

    MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,8> memtree;

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

    MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,8> memtree;

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
    static constexpr int NBITEMS_PER_BLOCK_LOG2 = 3;

    auto fct  = []  (size_t imax)
    {
        MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,9> memtree;

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

//////////////////////////////////////////////////////////////////////////////
template<int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
auto MemoryTree4_aux ()
{
    MemoryTree<MyAllocator,NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2> memtree;

    using tt = decltype(memtree);

    size_t imax = std::min (size_t(tt::NBLEAVES_MAX), size_t(1000*1000));

    for (size_t i=1; i<=imax; i++)  {  memtree.insert (i);  }

    uint64_t truth    = imax*(imax+1)/2;
    uint64_t checksum = 0;
    for (auto a : memtree)
    {
        checksum += a;
    }
    REQUIRE (checksum == truth);
}

TEST_CASE ("MemoryTree4", "[MemoryTree]" )
{
    //MemoryTree4_aux<1,5> ();

    //MemoryTree4_aux<1,6> ();
    MemoryTree4_aux<2,6> ();

    //MemoryTree4_aux<1,7> ();
    MemoryTree4_aux<2,7> ();
    MemoryTree4_aux<3,7> ();

    //MemoryTree4_aux<1,8> ();
    MemoryTree4_aux<2,8> ();
    MemoryTree4_aux<3,8> ();
    MemoryTree4_aux<4,8> ();

    //MemoryTree4_aux<1,9> ();
    MemoryTree4_aux<2,9> ();
    MemoryTree4_aux<3,9> ();
    MemoryTree4_aux<4,9> ();
    MemoryTree4_aux<5,9> ();
    MemoryTree4_aux<6,9> ();
}

//////////////////////////////////////////////////////////////////////////////
template<int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
auto MemoryTree5_aux_aux (size_t nbitems)
{
    MemoryTree <MyAllocator, NBITEMS_PER_BLOCK_LOG2, MAX_MEMORY_LOG2> memtree;

    if (nbitems <= memtree.max_size() )
    {
        for (size_t i=1; i<=nbitems; i++)  {  memtree.insert (i);  }

        {  size_t n=1;        for (auto x : memtree)                                      {  REQUIRE (x == n++);  }  }
        {  size_t n=      1;  memtree.leaves ([&] (auto x)        {  REQUIRE (x == n++);  });  }
        {  size_t n=nbitems;  memtree.template iterate_leaves<false> ([&] (auto x)        {  REQUIRE (x == n--);  });  }
        {  size_t n=1;        for (auto it=memtree. begin(); it != memtree. end(); ++it)  {  REQUIRE (*it == n++);  }  }
        {  size_t n=nbitems;  for (auto it=memtree.rbegin(); it != memtree.rend(); ++it)  {  REQUIRE (*it == n--);  }  }
    }
}

template<int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
auto MemoryTree5_aux ()
{
    for (int n=1; n<=16; n++)
    {
        MemoryTree5_aux_aux<NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2> (1<<n);
    }

    for (int n=1; n<=2000; n++)
    {
        MemoryTree5_aux_aux<NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2> (n);
    }
}

TEST_CASE ("MemoryTree5", "[MemoryTree]" )
{
    MemoryTree5_aux<2,8> ();
    MemoryTree5_aux<3,8> ();
    MemoryTree5_aux<4,8> ();

    MemoryTree5_aux<2,9> ();
    MemoryTree5_aux<3,9> ();
    MemoryTree5_aux<4,9> ();

    MemoryTree5_aux<2,10> ();
    MemoryTree5_aux<3,10> ();
    MemoryTree5_aux<4,10> ();
}

//////////////////////////////////////////////////////////////////////////////
template<int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
void MemoryTree6_aux (size_t nbitems=10000, uint32_t mask=0xff)
{
    MemoryTree <MyAllocator,NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2> memtree;

    for (size_t i=1; i<=std::min(nbitems,memtree.max_size()) ; i++)
    {
        memtree.insert (i);
        REQUIRE(memtree.size() == i);

        if (mask & 0x1) { uint64_t s=0;  for (auto x : memtree)  { s+=x; }  REQUIRE (s == i*(i+1)/2); }
        if (mask & 0x2) { uint64_t s=0;  for (auto it=memtree. begin(); it != memtree. end(); ++it)  { s+=*it; }  REQUIRE (s == i*(i+1)/2); }
        if (mask & 0x4) { uint64_t s=0;  for (auto it=memtree.rbegin(); it != memtree.rend(); ++it)  { s+=*it; }  REQUIRE (s == i*(i+1)/2); }
    }
}

TEST_CASE ("MemoryTree6", "[MemoryTree]" )
{
    MemoryTree6_aux<2,8> ();
    MemoryTree6_aux<3,8> ();
    MemoryTree6_aux<4,8> ();

    MemoryTree6_aux<2,9> ();
    MemoryTree6_aux<3,9> ();
    MemoryTree6_aux<4,9> ();

    MemoryTree6_aux<2,10> ();
    MemoryTree6_aux<3,10> ();
    MemoryTree6_aux<4,10> ();
}

//////////////////////////////////////////////////////////////////////////////
template<int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
void MemoryTree7_aux (size_t nbitems=100000)
{
    MemoryTree <MyAllocator,NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2> memtree;

    size_t actualNbItems = std::min(nbitems,memtree.max_size());

    for (size_t i=0; i<actualNbItems; i++)  {  memtree.insert (i);  }

    REQUIRE (memtree.size() == actualNbItems);

    for (size_t i=0; i<memtree.size(); i++)
    {
        REQUIRE (memtree[i] == i);
    }
}

TEST_CASE ("MemoryTree7", "[MemoryTree]" )
{
    MemoryTree7_aux<2,8> ();
    MemoryTree7_aux<3,8> ();
    MemoryTree7_aux<4,8> ();

    MemoryTree7_aux<2,9> ();
    MemoryTree7_aux<3,9> ();
    MemoryTree7_aux<4,9> ();

    MemoryTree7_aux<2,10> ();
    MemoryTree7_aux<3,10> ();
    MemoryTree6_aux<4,10> ();
}

//////////////////////////////////////////////////////////////////////////////
template<int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
void MemoryTree8_aux (size_t nbitems=5000)
{
    MemoryTree <MyAllocator,NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2> memtree;

    size_t actualNbItems = std::min(nbitems,memtree.max_size());

    for (size_t i=1; i<=actualNbItems; i++)
    {
        memtree.insert (i);
        REQUIRE (memtree.size() == i);

        for (size_t j=0; j<memtree.size(); j++)
        {
            REQUIRE (memtree[j] == j+1);
        }
    }
}

TEST_CASE ("MemoryTree8", "[MemoryTree]" )
{
    MemoryTree8_aux<2,8> (30);
    MemoryTree8_aux<3,8> ();
    MemoryTree8_aux<4,8> ();

    MemoryTree8_aux<2,9> ();
    MemoryTree8_aux<3,9> ();
    MemoryTree8_aux<4,9> ();

    MemoryTree8_aux<2,10> ();
    MemoryTree8_aux<3,10> ();
    MemoryTree8_aux<4,10> ();
}
