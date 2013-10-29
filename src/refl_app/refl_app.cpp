#include "cu/refl.h"

// USER CODE:

using namespace cu;

struct Mesh
{
    std::vector<float3> vertices;
    std::vector<int3> triangles;
};

struct Pose
{
    float3 position;
    float4 orientation;
};

struct Object
{
    std::string name;
    Pose pose;
    Mesh mesh;
};

// Reflect our user defined types
#define FIELD(n) f(#n, o.n)
template<class F> void visit_fields(Mesh & o, F f) { FIELD(vertices); FIELD(triangles); }
template<class F> void visit_fields(Pose & o, F f) { FIELD(position); FIELD(orientation); }
template<class F> void visit_fields(Object & o, F f) { FIELD(name); FIELD(pose); FIELD(mesh); }
#undef FIELD

// APP:

#include <iostream>
#include <map>

int main()
{
    std::map<const std::type_info *, std::string> prettynames = {
        {&typeid(Pose),                "Pose"},
        {&typeid(Object),              "Object"},
        {&typeid(Mesh),                "Mesh"},
        {&typeid(std::string),         "string"},
        {&typeid(float),               "float"},
        {&typeid(float3),              "float3"},
        {&typeid(float4),              "float4"},
        {&typeid(std::vector<float3>), "vector<float3>"},
        {&typeid(std::vector<int3>),   "vector<int3>"}
    };

    std::vector<Class> classes = { reflect<Pose>(), reflect<Mesh>(), reflect<Object>(), reflect<float3>() };

    std::cout << "REFLECTION:" << std::endl;
    for (const auto & cl : classes)
    {
        std::cout << "  class " << prettynames[cl.type] << ":" << std::endl;
        std::cout << "    size: " << cl.size << std::endl;
        for (const auto & field : cl.fields)
        {
            std::cout << "    field " << field.name << ":" << std::endl;
            std::cout << "      type: " << prettynames[field.type] << std::endl;
            std::cout << "      offset: " << field.offset << std::endl;
        }
    }
    std::cout << "\n\n";

    // Create an object
    Object obj = { 
        "Cube", 
        {{1,2,3}, {0,0,0,1}}, 
        {
            {{-0.5f,-0.5f,0}, {0.5f,-0.5f,0}, {0.5f,0.5f,0}, {-0.5f,0.5f,0}}, 
            {{0,1,2}, {0,2,3}}
        }
    };

    // Serialize it to JSON
    std::ostringstream ss; 
    ss << tabbed(toJson(obj),4) << std::endl;
    auto textFile = ss.str();

    // Print the serialized results
    std::cout << "Serialized form:\n" << textFile << "\n";

    auto jVal = jsonFrom(textFile); // Parse the JSON
    Object obj2; fromJson(obj2, jVal); // Read the JSON into the object

    // Print the contents of obj2
    std::cout << "obj2.name = " << obj2.name << std::endl;
    std::cout << "obj2.pose = " << toJson(obj2.pose) << std::endl;
    std::cout << "obj2.mesh = " << toJson(obj2.mesh) << std::endl;
    return 0;
}