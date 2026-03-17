////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <firstinclude.hpp>
#include <queue>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/** Get N sorted iterables as input (not necessarely std::vector objects) and
 * sort-merge them and call a functor on each item. This kind of function can be called
 * to finish a big sort that has been split into N parts.
 * \param sortedVectors: iterable objects to be merged/sorted
 * \param fct: functor called on each sorted item.
 */
template<typename FCT>
auto merge (auto const& sortedVectors, FCT fct)
{
    // We create a priority queue with begin/end iterators for all input vectors

    using value_type = decltype(sortedVectors[0].begin());
    using queue_type = std::pair<value_type,value_type>;

    auto cmp = [](queue_type left, queue_type right) {  return *(left.first) > *(right.first);  };

    // We declare our priority queue.
    std::priority_queue<queue_type, std::vector<queue_type>, decltype(cmp)> pq (cmp);

    // and fill it
    for (auto const& v : sortedVectors)  {  pq.push (std::make_pair(std::begin(v), std::end(v)));  }

    // We iterate the queue and call our functor on each value.
    for (; !pq.empty(); )  {
        auto iters = pq.top();
        pq.pop();
        fct (*iters.first);
        iters.first++;
        if (iters.first != iters.second)  {  pq.push(std::make_pair(iters.first,iters.second)); }
    }
}

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
