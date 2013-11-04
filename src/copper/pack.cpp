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
        default:         assert(false);
        }
    }

    JsonValue jsonFromPacked(const void * data, PackedType type)
    {
        switch (type)
        {
        case PackFloat:  return *reinterpret_cast<const float    *>(data);
        case PackDouble: return *reinterpret_cast<const double   *>(data);
        case PackInt:    return *reinterpret_cast<const int32_t  *>(data);
        case PackUInt:   return *reinterpret_cast<const uint32_t *>(data);
        case PackBool:   return *reinterpret_cast<const int32_t  *>(data) != 0;
        default:         assert(false); return nullptr;
        }
    }

    static JsonValue flatten(JsonArray arr)
    {
        if (arr.size() == 1) return std::move(arr[0]);
        return arr;
    }
 
    JsonValue PackedField::readJson(const void * structBuffer) const
    {
        JsonArray arr;
        for (uint3 index; index.z<dimensions.z; ++index.z)
        {
            JsonArray mat;
            for (index.y = 0; index.y<dimensions.y; ++index.y)
            {
                JsonArray col;
                for (index.x = 0; index.x<dimensions.x; ++index.x)
                {
                    col.push_back(jsonFromPacked(reinterpret_cast<const int8_t *>(structBuffer) + offset + dot(index, stride), baseType));
                }
                mat.push_back(flatten(col));
            }
            arr.push_back(flatten(mat));
        }
        return flatten(arr);
    }

    void PackedField::writeOne(void * structBuffer, uint3 index, double value) const
    {
        if (index.x < dimensions.x && index.y < dimensions.y && index.z < dimensions.z)
        {
            write(reinterpret_cast<int8_t *>(structBuffer) + offset + dot(index, stride), baseType, value);
        }
    }

    JsonValue PackedStruct::readJson(const void * buffer) const
    {
        JsonObject obj;
        for (auto & f : fields) obj.emplace_back(f.name, f.readJson(buffer));
        return obj;
    }
}