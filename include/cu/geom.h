#ifndef COPPER_GEOM_H
#define COPPER_GEOM_H

#include "math.h"

namespace cu
{
    float3 transformVector(const float4x4 & transform, const float3 & vec) { return mul(transform, float4(vec, 0)).xyz(); }
    float3 transformCoord(const float4x4 & transform, const float3 & coord) { auto r = mul(transform, float4(coord, 1)); return r.xyz() / r.w; }

    // Maps x:right, y:up, z:back into x:[-1,+1], y:[-1,+1], z:[-1,+1] with a perspective transform
    template<class T> mat<T,4,4> perspectiveMatrixRhGl(T vertFieldOfViewRadians, T aspectWidthOverHeight, T nearClipDist, T farClipDist)
    {
        auto yf = 1/tan(vertFieldOfViewRadians/2), xf = yf/aspectWidthOverHeight, dz = nearClipDist-farClipDist;
        return {{xf,0,0,0}, {0,yf,0,0}, {0,0,(nearClipDist+farClipDist)/dz,-1}, {0,0,2*nearClipDist*farClipDist/dz,0}};
    }
    
    template<class T> mat<T,4,4> rigidTransformMatrix(const vec<T,4> & rotationQuat, const vec<T,3> & translationVec)
    {
        return {{qxdir(rotationQuat),0}, {qydir(rotationQuat),0}, {qzdir(rotationQuat),0}, {translationVec,1}};
    }

    // Pose is useful for representing a rigid transformation (a translation + rotation)
    struct Pose
    {
        float3      position;
        float4      orientation;

                    Pose(const float3 & pos, const float4 & ori)   : position(pos), orientation(ori) {}
                    Pose()                                         : Pose({0,0,0}, {0,0,0,1}) {}
                    Pose(const float3 & position)                  : Pose(position, {0,0,0,1}) {}
                    Pose(const float4 & orientation)               : Pose({0,0,0}, orientation) {}

        float3      transformVector(const float3 & vec) const      { return qtransform(orientation, vec); }
        float3      transformCoord(const float3 & coord) const     { return position + transformVector(coord); }

        Pose        operator * (const Pose & pose) const           { return {transformCoord(pose.position), qmul(orientation,pose.orientation)}; }
        float3      operator * (const float3 & coord) const        { return transformCoord(coord); }

        Pose        inverse() const                                { auto invOri = qinv(orientation); return {qtransform(invOri, -position), invOri}; }
        float4x4    matrix() const                                 { return rigidTransformMatrix(orientation, position); }
        float3      xdir() const                                   { return qxdir(orientation); } // Equivalent to transformVector({1,0,0})
        float3      ydir() const                                   { return qydir(orientation); } // Equivalent to transformVector({0,1,0})
        float3      zdir() const                                   { return qzdir(orientation); } // Equivalent to transformVector({0,0,1})
    };
    template<class F> void visit_fields(Pose & o, F f) { f("position", o.position); f("orientation", o.orientation); }
}

#endif