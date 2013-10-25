#ifndef COPPER_MATH_H
#define COPPER_MATH_H

#include <cmath>
#include <cstdint>
#include <functional>

namespace cu
{
	template<class T, int M> struct vec;
	template<class T> struct vec<T,2>
	{
		T						x, y;
								vec()								: x(), y() {}
								vec(T x, T y)						: x(x), y(y) {}
		T &						operator [] (int i)					{ return (&x)[i]; } // v[i] retrieves the i'th row
		const T &				operator [] (int i) const			{ return (&x)[i]; } // v[i] retrieves the i'th row 
		template<class F> vec	apply(const vec & r, F f) const		{ return {f(x,r.x), f(y,r.y)}; }
		template<class F> vec	apply(T r, F f) const				{ return {f(x,r), f(y,r)}; }
	};
	template<class T> struct vec<T,3>
	{
		T						x, y, z;
								vec()								: x(), y(), z() {}
								vec(T x, T y, T z)					: x(x), y(y), z(z) {}
								vec(const vec<T,2> & xy, T z)       : vec(xy.x, xy.y, z) {}
		T &						operator [] (int i)					{ return (&x)[i]; } // v[i] retrieves the i'th row
		const T &				operator [] (int i) const			{ return (&x)[i]; } // v[i] retrieves the i'th row 
		template<class F> vec	apply(const vec & r, F f) const		{ return {f(x,r.x), f(y,r.y), f(z,r.z)}; }
		template<class F> vec	apply(T r, F f) const				{ return {f(x,r), f(y,r), f(z,r)}; }
	};
	template<class T> struct vec<T,4>
	{
		T						x, y, z, w;
								vec()								: x(), y(), z(), w() {}
								vec(T x, T y, T z, T w)				: x(x), y(y), z(z), w(w) {}
								vec(const vec<T,2> & xy, T z, T w)  : vec(xy.x, xy.y, z, w) {}
								vec(const vec<T,3> & xyz, T w)      : vec(xyz.x, xyz.y, xyz.z, w) {}
		T &						operator [] (int i)					{ return (&x)[i]; } // v[i] retrieves the i'th row
		const T &				operator [] (int i) const			{ return (&x)[i]; } // v[i] retrieves the i'th row 
		template<class F> vec	apply(const vec & r, F f) const		{ return {f(x,r.x), f(y,r.y), f(z,r.z), f(w,r.w)}; }
		template<class F> vec	apply(T r, F f) const				{ return {f(x,r), f(y,r), f(z,r), f(w,r)}; }
	};

	template<class T, int M> auto operator + (const vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> { return a.apply(b, std::plus      <T>()); }
	template<class T, int M> auto operator - (const vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> { return a.apply(b, std::minus     <T>()); }
	template<class T, int M> auto operator * (const vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> { return a.apply(b, std::multiplies<T>()); }
	template<class T, int M> auto operator / (const vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> { return a.apply(b, std::divides   <T>()); }
	template<class T, int M> auto operator + (const vec<T,M> & a, T b) -> vec<T,M>                { return a.apply(b, std::plus      <T>()); }
	template<class T, int M> auto operator - (const vec<T,M> & a, T b) -> vec<T,M>                { return a.apply(b, std::minus     <T>()); }
	template<class T, int M> auto operator * (const vec<T,M> & a, T b) -> vec<T,M>                { return a.apply(b, std::multiplies<T>()); }
	template<class T, int M> auto operator / (const vec<T,M> & a, T b) -> vec<T,M>                { return a.apply(b, std::divides   <T>()); }
	template<class T, int M> auto operator += (vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> &    { return a=a+b; }
	template<class T, int M> auto operator -= (vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> &    { return a=a-b; }
	template<class T, int M> auto operator *= (vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> &    { return a=a*b; }
	template<class T, int M> auto operator /= (vec<T,M> & a, const vec<T,M> & b) -> vec<T,M> &    { return a=a/b; }
	template<class T, int M> auto operator += (vec<T,M> & a, T b) -> vec<T,M> &                   { return a=a+b; }
	template<class T, int M> auto operator -= (vec<T,M> & a, T b) -> vec<T,M> &                   { return a=a-b; }
	template<class T, int M> auto operator *= (vec<T,M> & a, T b) -> vec<T,M> &                   { return a=a*b; }
	template<class T, int M> auto operator /= (vec<T,M> & a, T b) -> vec<T,M> &                   { return a=a/b; }

	template<class T>        auto dot (const vec<T,2> & a, const vec<T,2> & b) -> T               { return a.x*b.x + a.y*b.y; }
	template<class T>        auto dot (const vec<T,3> & a, const vec<T,3> & b) -> T               { return a.x*b.x + a.y*b.y + a.z*b.z; }
	template<class T>        auto dot (const vec<T,4> & a, const vec<T,4> & b) -> T               { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
	template<class T, int M> auto lerp(const vec<T,4> & a, const vec<T,4> & b, T t) -> vec<T,M>   { return a*(1-t) + b*t; }
	template<class T, int M> auto mag (const vec<T,M> & a) -> T                                   { return sqrt(mag2(a)); }
	template<class T, int M> auto mag2(const vec<T,M> & a) -> T                                   { return dot(a,a); }
	template<class T, int M> auto max (const vec<T,M> & a, const vec<T,M> & b) -> vec<T,M>        { return a.apply(b, std::max<T>); }
	template<class T, int M> auto min (const vec<T,M> & a, const vec<T,M> & b) -> vec<T,M>        { return a.apply(b, std::min<T>); }
	template<class T, int M> auto norm(const vec<T,M> & a) -> vec<T,M>                            { return a/mag(a); }

	template<class T, int M, int N> struct mat;
	template<class T, int M> struct mat<T,M,2>
	{
		typedef vec<T,M>		U;
		U						x, y;
								mat()								: x(), y() {}
								mat(U x, U y)						: x(x), y(y) {}
		U &						operator [] (int j)					{ return (&x)[j]; } // M[j] retrieves the j'th column
		const U &				operator [] (int j) const			{ return (&x)[j]; } // M[j] retrieves the j'th column
		T &						operator () (int i, int j)			{ return (*this)[j][i]; } // M(i,j) retrieves the i'th row of the j'th column
		const T &				operator () (int i, int j) const	{ return (*this)[j][i]; } // M(i,j) retrieves the i'th row of the j'th column
		vec<T,2>				row(int i) const					{ return {x[i],y[i]}; } // row(i) retrieves the i'th row
		template<class F> mat	apply(const mat & r, F f) const		{ return {x.apply(r.x,f), y.apply(r.y,f)}; }
		template<class F> mat	apply(T r, F f) const				{ return {x.apply(r,f), y.apply(r,f)}; }
	};
	template<class T, int M> struct mat<T,M,3>
	{
		typedef vec<T,M>		U;
		U						x, y, z;
								mat()								: x(), y(), z() {}
								mat(U x, U y, U z)					: x(x), y(y), z(z) {}
		U &						operator [] (int j)					{ return (&x)[j]; } // M[j] retrieves the j'th column
		const U &				operator [] (int j) const			{ return (&x)[j]; } // M[j] retrieves the j'th column
		T &						operator () (int i, int j)			{ return (*this)[j][i]; } // M(i,j) retrieves the i'th row of the j'th column
		const T &				operator () (int i, int j) const	{ return (*this)[j][i]; } // M(i,j) retrieves the i'th row of the j'th column
		vec<T,3>				row(int i) const					{ return {x[i],y[i],z[i]}; } // row(i) retrieves the i'th row
		template<class F> mat	apply(const mat & r, F f) const		{ return {x.apply(r.x,f), y.apply(r.y,f), z.apply(r.z,f)}; }
		template<class F> mat	apply(T r, F f) const				{ return {x.apply(r,f), y.apply(r,f), z.apply(r,f)}; }
	};
	template<class T, int M> struct mat<T,M,4>
	{
		typedef vec<T,M>		U;
		U						x, y, z, w;
								mat()								: x(), y(), z(), w() {}
								mat(U x, U y, U z, U w)				: x(x), y(y), z(z), w(w) {}
		U &						operator [] (int j)					{ return (&x)[j]; } // M[j] retrieves the j'th column
		const U &				operator [] (int j) const			{ return (&x)[j]; } // M[j] retrieves the j'th column
		T &						operator () (int i, int j)			{ return (*this)[j][i]; } // M(i,j) retrieves the i'th row of the j'th column
		const T &				operator () (int i, int j) const	{ return (*this)[j][i]; } // M(i,j) retrieves the i'th row of the j'th column
		vec<T,4>				row(int i) const					{ return {x[i],y[i],z[i],w[i]}; } // row(i) retrieves the i'th row
		template<class F> mat	apply(const mat & r, F f) const		{ return {x.apply(r.x,f), y.apply(r.y,f), z.apply(r.z,f), w.apply(r.w,f)}; }
		template<class F> mat	apply(T r, F f) const				{ return {x.apply(r,f), y.apply(r,f), z.apply(r,f), w.apply(r,f)}; }
	};

	template<class T, int M, int N> auto operator + (const mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> { return a.apply(b, std::plus      <T>()); }
	template<class T, int M, int N> auto operator - (const mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> { return a.apply(b, std::minus     <T>()); }
	template<class T, int M, int N> auto operator * (const mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> { return a.apply(b, std::multiplies<T>()); }
	template<class T, int M, int N> auto operator / (const mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> { return a.apply(b, std::divides   <T>()); }
	template<class T, int M, int N> auto operator + (const mat<T,M,N> & a, T b) -> mat<T,M,N>                  { return a.apply(b, std::plus      <T>()); }
	template<class T, int M, int N> auto operator - (const mat<T,M,N> & a, T b) -> mat<T,M,N>                  { return a.apply(b, std::minus     <T>()); }
	template<class T, int M, int N> auto operator * (const mat<T,M,N> & a, T b) -> mat<T,M,N>                  { return a.apply(b, std::multiplies<T>()); }
	template<class T, int M, int N> auto operator / (const mat<T,M,N> & a, T b) -> mat<T,M,N>                  { return a.apply(b, std::divides   <T>()); }
	template<class T, int M, int N> auto operator += (mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> &    { return a=a+b; }
	template<class T, int M, int N> auto operator -= (mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> &    { return a=a-b; }
	template<class T, int M, int N> auto operator *= (mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> &    { return a=a*b; }
	template<class T, int M, int N> auto operator /= (mat<T,M,N> & a, const mat<T,M,N> & b) -> mat<T,M,N> &    { return a=a/b; }
	template<class T, int M, int N> auto operator += (mat<T,M,N> & a, T b) -> mat<T,M,N> &                     { return a=a+b; }
	template<class T, int M, int N> auto operator -= (mat<T,M,N> & a, T b) -> mat<T,M,N> &                     { return a=a-b; }
	template<class T, int M, int N> auto operator *= (mat<T,M,N> & a, T b) -> mat<T,M,N> &                     { return a=a*b; }
	template<class T, int M, int N> auto operator /= (mat<T,M,N> & a, T b) -> mat<T,M,N> &                     { return a=a/b; }

	template<class T> mat<T,2,2> adj(const mat<T,2,2> & a) { return {{a.y.y, -a.x.y}, {-a.y.x, a.x.x}}; }
	template<class T> mat<T,3,3> adj(const mat<T,3,3> & a) { return {{a.y.y*a.z.z - a.z.y*a.y.z, a.z.y*a.x.z - a.x.y*a.z.z, a.x.y*a.y.z - a.y.y*a.x.z},
                                                                     {a.y.z*a.z.x - a.z.z*a.y.x, a.z.z*a.x.x - a.x.z*a.z.x, a.x.z*a.y.x - a.y.z*a.x.x},
                                                                     {a.y.x*a.z.y - a.z.x*a.y.y, a.z.x*a.x.y - a.x.x*a.z.y, a.x.x*a.y.y - a.y.x*a.x.y}}; }
	template<class T> mat<T,4,4> adj(const mat<T,4,4> & a) { return {{a.y.y*a.z.z*a.w.w + a.w.y*a.y.z*a.z.w + a.z.y*a.w.z*a.y.w - a.y.y*a.w.z*a.z.w - a.z.y*a.y.z*a.w.w - a.w.y*a.z.z*a.y.w,
                                                                      a.x.y*a.w.z*a.z.w + a.z.y*a.x.z*a.w.w + a.w.y*a.z.z*a.x.w - a.w.y*a.x.z*a.z.w - a.z.y*a.w.z*a.x.w - a.x.y*a.z.z*a.w.w,
                                                                      a.x.y*a.y.z*a.w.w + a.w.y*a.x.z*a.y.w + a.y.y*a.w.z*a.x.w - a.x.y*a.w.z*a.y.w - a.y.y*a.x.z*a.w.w - a.w.y*a.y.z*a.x.w,
                                                                      a.x.y*a.z.z*a.y.w + a.y.y*a.x.z*a.z.w + a.z.y*a.y.z*a.x.w - a.x.y*a.y.z*a.z.w - a.z.y*a.x.z*a.y.w - a.y.y*a.z.z*a.x.w},
                                                                     {a.y.z*a.w.w*a.z.x + a.z.z*a.y.w*a.w.x + a.w.z*a.z.w*a.y.x - a.y.z*a.z.w*a.w.x - a.w.z*a.y.w*a.z.x - a.z.z*a.w.w*a.y.x,
                                                                      a.x.z*a.z.w*a.w.x + a.w.z*a.x.w*a.z.x + a.z.z*a.w.w*a.x.x - a.x.z*a.w.w*a.z.x - a.z.z*a.x.w*a.w.x - a.w.z*a.z.w*a.x.x,
                                                                      a.x.z*a.w.w*a.y.x + a.y.z*a.x.w*a.w.x + a.w.z*a.y.w*a.x.x - a.x.z*a.y.w*a.w.x - a.w.z*a.x.w*a.y.x - a.y.z*a.w.w*a.x.x,
                                                                      a.x.z*a.y.w*a.z.x + a.z.z*a.x.w*a.y.x + a.y.z*a.z.w*a.x.x - a.x.z*a.z.w*a.y.x - a.y.z*a.x.w*a.z.x - a.z.z*a.y.w*a.x.x},
                                                                     {a.y.w*a.z.x*a.w.y + a.w.w*a.y.x*a.z.y + a.z.w*a.w.x*a.y.y - a.y.w*a.w.x*a.z.y - a.z.w*a.y.x*a.w.y - a.w.w*a.z.x*a.y.y,
                                                                      a.x.w*a.w.x*a.z.y + a.z.w*a.x.x*a.w.y + a.w.w*a.z.x*a.x.y - a.x.w*a.z.x*a.w.y - a.w.w*a.x.x*a.z.y - a.z.w*a.w.x*a.x.y,
                                                                      a.x.w*a.y.x*a.w.y + a.w.w*a.x.x*a.y.y + a.y.w*a.w.x*a.x.y - a.x.w*a.w.x*a.y.y - a.y.w*a.x.x*a.w.y - a.w.w*a.y.x*a.x.y,
                                                                      a.x.w*a.z.x*a.y.y + a.y.w*a.x.x*a.z.y + a.z.w*a.y.x*a.x.y - a.x.w*a.y.x*a.z.y - a.z.w*a.x.x*a.y.y - a.y.w*a.z.x*a.x.y},
                                                                     {a.y.x*a.w.y*a.z.z + a.z.x*a.y.y*a.w.z + a.w.x*a.z.y*a.y.z - a.y.x*a.z.y*a.w.z - a.w.x*a.y.y*a.z.z - a.z.x*a.w.y*a.y.z,
                                                                      a.x.x*a.z.y*a.w.z + a.w.x*a.x.y*a.z.z + a.z.x*a.w.y*a.x.z - a.x.x*a.w.y*a.z.z - a.z.x*a.x.y*a.w.z - a.w.x*a.z.y*a.x.z,
                                                                      a.x.x*a.w.y*a.y.z + a.y.x*a.x.y*a.w.z + a.w.x*a.y.y*a.x.z - a.x.x*a.y.y*a.w.z - a.w.x*a.x.y*a.y.z - a.y.x*a.w.y*a.x.z,
                                                                      a.x.x*a.y.y*a.z.z + a.z.x*a.x.y*a.y.z + a.y.x*a.z.y*a.x.z - a.x.x*a.z.y*a.y.z - a.y.x*a.x.y*a.z.z - a.z.x*a.y.y*a.x.z}}; }
	template<class T> T det(const mat<T,2,2> & a) { return a.x.x*a.y.y - a.x.y*a.y.x; }
	template<class T> T det(const mat<T,3,3> & a) { return a.x.x*(a.y.y*a.z.z - a.z.y*a.y.z) + a.x.y*(a.y.z*a.z.x - a.z.z*a.y.x) + a.x.z*(a.y.x*a.z.y - a.z.x*a.y.y); }
	template<class T> T det(const mat<T,4,4> & a) { return a.x.x*(a.y.y*a.z.z*a.w.w + a.w.y*a.y.z*a.z.w + a.z.y*a.w.z*a.y.w - a.y.y*a.w.z*a.z.w - a.z.y*a.y.z*a.w.w - a.w.y*a.z.z*a.y.w) +
                                                           a.x.y*(a.y.z*a.w.w*a.z.x + a.z.z*a.y.w*a.w.x + a.w.z*a.z.w*a.y.x - a.y.z*a.z.w*a.w.x - a.w.z*a.y.w*a.z.x - a.z.z*a.w.w*a.y.x) +
                                                           a.x.z*(a.y.w*a.z.x*a.w.y + a.w.w*a.y.x*a.z.y + a.z.w*a.w.x*a.y.y - a.y.w*a.w.x*a.z.y - a.z.w*a.y.x*a.w.y - a.w.w*a.z.x*a.y.y) +
                                                           a.x.w*(a.y.x*a.w.y*a.z.z + a.z.x*a.y.y*a.w.z + a.w.x*a.z.y*a.y.z - a.y.x*a.z.y*a.w.z - a.w.x*a.y.y*a.z.z - a.z.x*a.w.y*a.y.z); }
	template<class T, int N> mat<T,N,N> inv(const mat<T,N,N> & a) { return adj(a)/det(a); }
	template<class T, int M> vec<T,M> mul(const mat<T,M,2> & a, const vec<T,2> & b) { return a.x*b.x + a.y*b.y; }
	template<class T, int M> vec<T,M> mul(const mat<T,M,3> & a, const vec<T,3> & b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
	template<class T, int M> vec<T,M> mul(const mat<T,M,4> & a, const vec<T,4> & b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
	template<class T, int M, int N> mat<T,M,2> mul(const mat<T,M,N> & a, const mat<T,N,2> & b) { return {mul(a,b.x), mul(a,b.y)}; }
	template<class T, int M, int N> mat<T,M,3> mul(const mat<T,M,N> & a, const mat<T,N,3> & b) { return {mul(a,b.x), mul(a,b.y), mul(a,b.z)}; }
	template<class T, int M, int N> mat<T,M,4> mul(const mat<T,M,N> & a, const mat<T,N,4> & b) { return {mul(a,b.x), mul(a,b.y), mul(a,b.z), mul(a,b.w)}; }
	template<class T, int M> mat<T,M,2> transpose(const mat<T,2,M> & a) { return {a.row(0), a.row(1)}; }
	template<class T, int M> mat<T,M,3> transpose(const mat<T,3,M> & a) { return {a.row(0), a.row(1), a.row(2)}; }
	template<class T, int M> mat<T,M,4> transpose(const mat<T,4,M> & a) { return {a.row(0), a.row(1), a.row(2), a.row(3)}; }

	typedef vec<int8_t,2> byte2; typedef vec<uint8_t,2> ubyte2; typedef vec<int16_t,2> short2; typedef vec<uint16_t,2> ushort2;
	typedef vec<int8_t,3> byte3; typedef vec<uint8_t,3> ubyte3; typedef vec<int16_t,3> short3; typedef vec<uint16_t,3> ushort3;
	typedef vec<int8_t,4> byte4; typedef vec<uint8_t,4> ubyte4; typedef vec<int16_t,4> short4; typedef vec<uint16_t,4> ushort4;
	typedef vec<int32_t,2> int2; typedef vec<uint32_t,2> uint2; typedef vec<int64_t,2> long2; typedef vec<uint64_t,2> ulong2;
	typedef vec<int32_t,3> int3; typedef vec<uint32_t,3> uint3; typedef vec<int64_t,3> long3; typedef vec<uint64_t,3> ulong3;
	typedef vec<int32_t,4> int4; typedef vec<uint32_t,4> uint4; typedef vec<int64_t,4> long4; typedef vec<uint64_t,4> ulong4;
	typedef vec<float,2> float2; typedef mat<float,2,2> float2x2; typedef mat<float,2,3> float2x3; typedef mat<float,2,4> float2x4; 
	typedef vec<float,3> float3; typedef mat<float,3,2> float3x2; typedef mat<float,3,3> float3x3; typedef mat<float,3,4> float3x4; 
	typedef vec<float,4> float4; typedef mat<float,4,2> float4x2; typedef mat<float,4,3> float4x3; typedef mat<float,4,4> float4x4;
	typedef vec<double,2> double2; typedef mat<double,2,2> double2x2; typedef mat<double,2,3> double2x3; typedef mat<double,2,4> double2x4; 
	typedef vec<double,3> double3; typedef mat<double,3,2> double3x2; typedef mat<double,3,3> double3x3; typedef mat<double,3,4> double3x4; 
	typedef vec<double,4> double4; typedef mat<double,4,2> double4x2; typedef mat<double,4,3> double4x3; typedef mat<double,4,4> double4x4;
}

#endif