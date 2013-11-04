#include "common.h"
#include "cu/pack.h"

#include <cassert>

namespace cu
{
    void write(void * dest, PackedType type, double value)
    {
        switch (type)
        {
        case PackFloat:  *reinterpret_cast<float    *>(dest) = static_cast<float   >(value); break;
        case PackDouble: *reinterpret_cast<double   *>(dest) = value;                        break;
        case PackInt:    *reinterpret_cast<int32_t  *>(dest) = static_cast<int32_t >(value); break;
        case PackUInt:   *reinterpret_cast<uint32_t *>(dest) = static_cast<uint32_t>(value); break;
        case PackBool:   *reinterpret_cast<int32_t  *>(dest) = value ? 1 : 0;                break;
        default: assert(false);
        }
    }

    void PackedField::writeOne(void * structBuffer, uint3 index, double value) const
    {
        if (index.x < dimensions.x && index.y < dimensions.y && index.z < dimensions.z)
        {
            cu::write(reinterpret_cast<int8_t *>(structBuffer) + offset + dot(index, stride), baseType, value);
        }
    }
}