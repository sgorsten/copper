// reflection module - Makes use of library and user-defined visit_members(...) functions to support limited reflection and serialization
#ifndef COPPER_REFL_H
#define COPPER_REFL_H

#include "cu/json.h"
#include "cu/math.h"

namespace cu
{
    // reflect<T>() - Produce an object describing the fields of a given class
    struct Field { const char * name; const std::type_info * type; size_t offset; };
    struct Class { const std::type_info * type; size_t size; std::vector<Field> fields; };
    struct reflect_add_fields { std::vector<Field> & fields; template<class T> void operator() (const char * name, const T & field) { fields.push_back({ name, &typeid(T), reinterpret_cast<size_t>(&field) }); } };
    template<class T> Class reflect() { Class cl = { &typeid(T), sizeof(T), {} }; visit_fields(*reinterpret_cast<T *>(nullptr), reflect_add_fields{ cl.fields }); return cl; }

    // toJson(...) - Generic serialization system
    // Generic types use visit_fields() to create a JSON object
    struct json_write_fields { JsonObject & obj; template<class T> void operator() (const char * name, const T & field) { obj.emplace_back(name, toJson(field)); } };
    template<class T> JsonValue toJson(const T & obj) { JsonObject o; visit_fields(const_cast<T &>(obj), json_write_fields{ o }); return o; }

    // For specific types, we can overload toJson with a specific encoding
    template<class T> JsonValue toJson(const std::vector<T> & arr) { JsonArray a; for (const auto & val : arr) a.push_back(toJson(val)); return a; }
    template<class T> JsonValue toJson(const vec<T, 4> & vec) { return JsonArray{ toJson(vec.x), toJson(vec.y), toJson(vec.z), toJson(vec.w) }; }
    template<class T> JsonValue toJson(const vec<T, 3> & vec) { return JsonArray{ toJson(vec.x), toJson(vec.y), toJson(vec.z) }; }
    template<class T> JsonValue toJson(const vec<T, 2> & vec) { return JsonArray{ toJson(vec.x), toJson(vec.y) }; }
    inline JsonValue toJson(const std::string & s) { return s; }
    inline JsonValue toJson(int16_t             n) { return n; }
    inline JsonValue toJson(uint16_t            n) { return n; }
    inline JsonValue toJson(int32_t             n) { return n; }
    inline JsonValue toJson(uint32_t            n) { return n; }
    inline JsonValue toJson(int64_t             n) { return n; }
    inline JsonValue toJson(uint64_t            n) { return n; }
    inline JsonValue toJson(float               n) { return n; }
    inline JsonValue toJson(double              n) { return n; }
    inline JsonValue toJson(bool                b) { return b; }

    // fromJson(...) - Generic deserialization system
    // Generic types use visit_fields() to read from a JSON object
    struct json_read_fields { const JsonValue & val; template<class T> void operator() (const char * name, T & field) { fromJson(field, val[name]); } };
    template<class T> void fromJson(T & obj, const JsonValue & val) { visit_fields(obj, json_read_fields{ val }); }

    // For specific types, we can overload fromJson with a specific encoding
    template<class T> void fromJson(std::vector<T> & arr, const JsonValue & val) { arr.resize(val.array().size()); for (size_t i = 0; i<arr.size(); ++i) fromJson(arr[i], val[i]); }
    template<class T> void fromJson(vec<T, 4> & vec, const JsonValue & val) { fromJson(vec.x, val[0]); fromJson(vec.y, val[1]); fromJson(vec.z, val[2]); fromJson(vec.w, val[3]); }
    template<class T> void fromJson(vec<T, 3> & vec, const JsonValue & val) { fromJson(vec.x, val[0]); fromJson(vec.y, val[1]); fromJson(vec.z, val[2]); }
    template<class T> void fromJson(vec<T, 2> & vec, const JsonValue & val) { fromJson(vec.x, val[0]); fromJson(vec.y, val[1]); }
    inline void fromJson(std::string & s, const JsonValue & val) { s = val.string(); }
    inline void fromJson(int16_t     & n, const JsonValue & val) { n = val.number<int32_t >(); }
    inline void fromJson(uint16_t    & n, const JsonValue & val) { n = val.number<uint32_t>(); }
    inline void fromJson(int32_t     & n, const JsonValue & val) { n = val.number<int32_t >(); }
    inline void fromJson(uint32_t    & n, const JsonValue & val) { n = val.number<uint32_t>(); }
    inline void fromJson(int64_t     & n, const JsonValue & val) { n = val.number<int64_t >(); }
    inline void fromJson(uint64_t    & n, const JsonValue & val) { n = val.number<uint64_t>(); }
    inline void fromJson(float       & n, const JsonValue & val) { n = val.number<float   >(); }
    inline void fromJson(double      & n, const JsonValue & val) { n = val.number<double  >(); }
    inline void fromJson(bool        & b, const JsonValue & val) { b = val.isTrue(); }

    // Utility methods
    template<class T> std::string encodeJson(const T & obj) { std::ostringstream ss; ss << toJson(obj); return ss.str(); }
    template<class T> T decodeJson(const std::string & text) { T obj; fromJson(obj, jsonFrom(text)); return obj; }

    // Support UserTypes as follows: 
    //   template<class F> void visit_fields(UserType & o, F f) { f("alpha", o.alpha); f("beta", o.beta); ... }
    //   See numerous examples throughout library
}

#endif