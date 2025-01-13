////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct GlobalOnce2
{
    USING(ARCH);

    struct Point
    {
        uint32_t x;
        uint32_t y;
        uint32_t z;

        Point operator+ (const Point& o) const  { return { x+o.x, y+o.y, z+o.z}; }
    };

    using array_t = array<Point,4000>;

    auto operator() (global<once<array_t const &>> v, uint32_t k) const
    {
        return bpl::accumulate (*v, Point {1*k, 2*k, 3*k} );
    }
};
