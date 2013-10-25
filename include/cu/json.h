#ifndef COPPER_JSON_H
#define COPPER_JSON_H

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>

namespace cu 
{
    namespace json
    {
        struct value;
        typedef std::vector<value> array;
        typedef std::vector<std::pair<std::string, value>> object;

        value       parse(std::string & json);
        void        packed_print(std::ostream & out, const value & val);
        void        packed_print(std::ostream & out, const array & arr);
        void        packed_print(std::ostream & out, const object & obj);
        void        pretty_print(std::ostream & out, const value & val, int tabs);
        void        pretty_print(std::ostream & out, const array & arr, int tabs);
        void        pretty_print(std::ostream & out, const object & obj, int tabs);
        bool        is_number(const std::string & num);

        struct      not_json : std::runtime_error { not_json(const std::string & what) : runtime_error("not json - " + what) {} };
        value       parse(const std::string & text); // throws not_json

        struct value
        {
            template<class T> static std::string to_str(const T & val) { std::ostringstream ss; ss << val; return ss.str(); }

            enum                Kind { Null, False, True, String, Number, Array, Object };
            Kind                kind; // What kind of value is this?
            std::string         str;  // Contents of String or Number value
            object              obj;  // Fields of Object value
            array               arr;  // Elements of Array value

                                value(Kind kind, std::string str)           : kind(kind), str(move(str)) {}
        public:
                                value()                                     : kind(Null) {}                 // Default construct null
                                value(nullptr_t)                            : kind(Null) {}                 // Construct null from nullptr
                                value(bool b)                               : kind(b ? True : False) {}     // Construct true or false from boolean
                                value(const char * s)                       : value(String, s) {}           // Construct String from C-string
                                value(std::string s)                        : value(String, move(s)) {}     // Construct String from std::string
                                value(int32_t n)                            : value(Number, to_str(n)) {}   // Construct Number from integer
                                value(uint32_t n)                           : value(Number, to_str(n)) {}   // Construct Number from integer
                                value(int64_t n)                            : value(Number, to_str(n)) {}   // Construct Number from integer
                                value(uint64_t n)                           : value(Number, to_str(n)) {}   // Construct Number from integer
                                value(float n)                              : value(Number, to_str(n)) {}   // Construct Number from float
                                value(double n)                             : value(Number, to_str(n)) {}   // Construct Number from double
                                value(object o)                             : kind(Object), obj(move(o)) {} // Construct Object from vector<pair<string,value>> (TODO: Assert no duplicate keys)
                                value(array a)                              : kind(Array), arr(move(a)) {}  // Construct Array from vector<value>

            bool                is_string() const                           { return kind == String; }
            bool                is_number() const                           { return kind == Number; }
            bool                is_object() const                           { return kind == Object; }
            bool                is_array() const                            { return kind == Array; }
            bool                is_true() const                             { return kind == True; }
            bool                is_false() const                            { return kind == False; }
            bool                is_null() const                             { return kind == Null; }

            const value &       operator[](size_t index) const              { const static value null; return index < arr.size() ? arr[index] : null; }
            const value &       operator[](int index) const                 { const static value null; return index < 0 ? null : (*this)[static_cast<size_t>(index)]; }
            const value &       operator[](const char * key) const          { for (auto & kvp : obj) if (kvp.first == key) return kvp.second; const static value null; return null; }
            const value &       operator[](const std::string & key) const   { return (*this)[key.c_str()]; }

            bool                bool_or_default(bool def) const             { return is_true() ? true : is_false() ? false : def; }
            std::string         string_or_default(const char * def) const   { return kind == String ? str : def; }
            template<class T> T number_or_default(T def) const              { if (kind != Number) return def; T val = def; std::istringstream(str) >> val; return val; }

            std::string         string() const                              { return string_or_default(""); } // Value, if a String, empty otherwise
            template<class T> T number() const                              { return number_or_default(T()); } // Value, if a Number, empty otherwise
            const object &      object() const                              { return obj; }    // Name/value pairs, if an Object, empty otherwise
            const array &       array() const                               { return arr; }    // Values, if an Array, empty otherwise

            const std::string & contents() const                            { return str; }    // Contents, if a String, JSON format number, if a Number, empty otherwise

            static value        from_number(std::string num)                { assert(json::is_number(num)); return value(Number, move(num)); }
        };

        inline std::ostream & operator << (std::ostream & out, const value & val) { pretty_print(out, val, 0); return out; }
        inline std::ostream & operator << (std::ostream & out, const array & val) { pretty_print(out, val, 0); return out; }
        inline std::ostream & operator << (std::ostream & out, const object & val) { pretty_print(out, val, 0); return out; }

        inline bool operator == (const value & a, const value & b) { std::ostringstream sa, sb; packed_print(sa, a); packed_print(sb, b); return sa.str() == sb.str(); }
        inline bool operator != (const value & a, const value & b) { return !(a == b); }
    } 
}

#endif