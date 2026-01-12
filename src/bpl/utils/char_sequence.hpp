////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

// SEE https://gist.github.com/erwandrezen/b79301ae43514a1f2473e77ed194a4d0

// NOT USED IN REAL USE CASES IN THE BPL

#include <array>
#include <optional>
#include <limits>

namespace bpl {

#define FOR_EACH_8(doit,start,args...) \
    doit((start+0), args)  doit((start+1), args)  \
    doit((start+2), args)  doit((start+3), args)  \
    doit((start+4), args)  doit((start+5), args)  \
    doit((start+6), args)  doit((start+7), args)  \

#define FOR_EACH_32(doit,start,args...) \
    FOR_EACH_8(doit,start+ 0,args)  FOR_EACH_8(doit,start+ 8,args) \
    FOR_EACH_8(doit,start+16,args)  FOR_EACH_8(doit,start+24,args)

#define FOR_EACH_128(doit,start,args...) \
    FOR_EACH_32(doit,start+ 0,args)  FOR_EACH_32(doit,start+32,args) \
    FOR_EACH_32(doit,start+64,args)  FOR_EACH_32(doit,start+96,args)

#define FOR_EACH(doit,args...)  FOR_EACH_128(doit,0,args)  /* will call doit(idx,args...) for idx in [0:128[ */

template <typename T, T t=std::numeric_limits<T>::max(), T...Cs>
struct int_sequence {

    static constexpr T ends = t;

    template <T c>
    using push_back = std::conditional_t<
        c==ends,
        int_sequence<T,ends,Cs...>,
        int_sequence<T,ends,Cs..., c>
    >;
};

#define PUSH_BACK_SEQ(idx,N)   ::push_back < (idx<N ? idx: int_sequence<int>::ends ) >
#define MakeSequence(N) int_sequence<int> FOR_EACH(PUSH_BACK_SEQ,N)

#define PUSH_BACK_LIT(idx,str,filter)   ::push_back < (idx<std::size(str) ? filter(str[idx]) :char{})>
constexpr char myfilter(char c)  { return c!='-' ? c : char{}; }
#define MakeLiteral(str) int_sequence<char,char{}> FOR_EACH(PUSH_BACK_LIT,str,myfilter)

template<char...Cs> constexpr auto make_array (int_sequence<char,char{},Cs...> ) {
    return std::array<char,sizeof...(Cs)> { Cs...};
}

};

//using foo = MakeLiteral("af1c-dc-09");
//constexpr auto bar = make_array(foo{});
//static_assert (bar.size()==8);
//static_assert ('a' == bar[0]);
//static_assert ('f' == bar[1]);
//static_assert ('1' == bar[2]);
//static_assert ('c' == bar[3]);
//using dum = MakeSequence(10);
//int main() {}
