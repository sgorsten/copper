// mesh module - Contains utilities for building and manipulating various mesh representations
#ifndef COPPER_MESH_H
#define COPPER_MESH_H

#include "math.h"
#include <vector>

namespace cu
{
    // Set a given field of every element in a container to a specific value
    template<class T, class U> void setField(std::vector<T> & vec, U T::*field, const U & val) { for (auto & elem : vec) elem.*field = val; }

    // Compute vertex normals by averaging the geometric normals of all triangles that meet at a given vertex
    template<class T, class V, class I> void computeNormals(std::vector<V> & verts, const std::vector<vec<I,3>> & tris, vec<T,3> V::*normal, const vec<T,3> V::*position)
    {
        setField(verts, normal, vec<T,3>());
        for (auto & t : tris)
        {
            auto & v0 = verts[t.x], & v1 = verts[t.y], & v2 = verts[t.z];
            auto n = norm(cross(v1.*position - v0.*position, v2.*position - v0.*position));
            v0.*normal += n; v1.*normal += n; v2.*normal += n;
        }
        for (auto & v : verts) v.*normal = norm(v.*normal);
    }

    // Simple template class to store a indexed triangle mesh, with no additional data
    template<class V, class I> struct TriMesh { std::vector<V> verts; std::vector<vec<I,3>> tris; };
    template<class V, class I, class F> void visit_fields(TriMesh<V, I> & o, F f) { f("verts",o.verts); f("tris",o.tris); }

    // Define a six-sided box, optionally with flat normals and texture coordinates to use a full image on each side
    template<class V, class T> TriMesh<V,uint8_t> boxMesh(const vec<T,3> & dims, vec<T,3> V::*position, vec<T,3> V::*normal = 0, vec<T,2> V::*texCoord = 0)
    {
        static const vec<T,3> corners[] = {{0,0,0},{0,0,1},{0,1,1},{0,1,0},{1,1,0},{1,1,1},{1,0,1},{1,0,0},{0,0,0},{1,0,0},{1,0,1},{0,0,1},{0,1,1},{1,1,1},{1,1,0},{0,1,0},{0,0,0},{0,1,0},{1,1,0},{1,0,0},{1,0,1},{1,1,1},{0,1,1},{0,0,1}};
        static const vec<T,2> coords[] = {{0,0},{1,0},{1,1},{1,0}};
        TriMesh<V,uint8_t> m = {{24,V()},{{0,1,2},{0,2,3},{4,5,6},{4,6,7},{8,9,10},{8,10,11},{12,13,14},{12,14,15},{16,17,18},{16,18,19},{20,21,22},{20,22,23}}};
        for (int i = 0; i < 24; ++i) m.verts[i].*position = corners[i] * dims - dims / 2.0f;
        if (texCoord) for (int i = 0; i<24; ++i) m.verts[i].*texCoord = coords[i % 4];
        if (normal) computeNormals(m.verts, m.tris, normal, position);
        return m;
    }
}

#endif