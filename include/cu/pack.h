// pack module - Provides support for conveniently accessing packed binary representations of data
#ifndef COPPER_PACK_H
#define COPPER_PACK_H

#include "math.h"
#include "json.h"

namespace cu
{
    template<class C, class T> ptrdiff_t fieldOffset(const T C::*field) { return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<const C *>(0)->*field)); }

    enum PackedType { PackFloat, PackDouble, PackInt, PackUInt, PackBool };
    void write(void * dest, PackedType type, double value);
  
    struct PackedField 
    {
        std::string     name;       // Name of field, for conversion purposes
        ptrdiff_t       offset;     // Offset in bytes from start of structure
        PackedType      baseType;   // Basic type of scalar, vector, or matrix type
        uint3           dimensions; // Number of rows (x), columns (y), and array elements (z)
        uint3           stride;     // Offset in bytes between rows (x), columns (y), and array elements (z)

        void            writeOne(void * structBuffer, uint3 index, double value) const;

        template<class T>               void writeValue(void * structBuffer, size_t index, const T          & value) const {                                                 writeOne(structBuffer, uint3(0, 0, index), static_cast<double>(value     )); }
        template<class T, int M>        void writeValue(void * structBuffer, size_t index, const vec<T,M>   & value) const {                         for (int j=0; j<M; ++j) writeOne(structBuffer, uint3(j, 0, index), static_cast<double>(value[j]  )); }
        template<class T, int M, int N> void writeValue(void * structBuffer, size_t index, const mat<T,M,N> & value) const { for (int i=0; i<N; ++i) for (int j=0; j<M; ++j) writeOne(structBuffer, uint3(j, i, index), static_cast<double>(value(i,j))); }
    };

    struct PackedStruct
    {
        std::vector<PackedField>    fields;     // Fields of structure
        size_t                      size;       // Size of structure in bytes

        template<class T> void      write(void * buffer, const char * field, size_t index, const T & value) const { for(auto & f : fields) if(f.name == field) f.writeValue(buffer, index, value); }
    };
}

#endif