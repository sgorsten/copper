#include "cu/math.h"
#include "cu/json.h"
#include <iostream>

using namespace cu;

int main()
{
	// We have convenient linear algebra types available
	float3x4 a = mul(float3x2(), float2x4()) + float3x4();
	float4x2 b = mul(float4x3(), float3x2()) - float4x2();
	float2x3 c = mul(float2x4(), float4x3()) * 2.0f;
	float4x4 d = mul(float4x4(), float4x4()) / 3.0f;
	float2x4 e = transpose(float4x2());

	// We can parse JSON from a string
	auto parsed_value = json::parse(R"(
	{
		"firstName": "John",
		"lastName": "Smith",
		"age": 25,
		"isEmployed": true,
		"spouse": null,
		"address": {
			"streetAddress": "21 2nd Street",
			"city": "New York",
			"state": "NY",
			"postalCode": 10021
		},
		"phoneNumbers": [
			{
				"type": "home",
				"number": "212 555-1234"
			},
			{
				"type": "fax",
				"number": "646 555-4567"
			}
		]
	})");

	// We can also construct it using initializer lists				
	json::value literal_value = json::object{
		{"firstName", "John"},
		{"lastName", "Smith"},
		{"age", 25},
		{"isEmployed", true},
		{"spouse", nullptr},
		{"address", json::object{
			{"streetAddress", "21 2nd Street"},
			{"city", "New York"},
			{"state", "NY"},
			{"postalCode", 10021}
		}},
		{"phoneNumbers", json::array{
			json::object{
				{"type", "home"},
				{"number", "212 555-1234"}
			},
			json::object{
				{"type", "fax"},
				{"number", "646 555-4567"}
			}
		}}
	};

	// We can easily print our json::values back into JSON
	std::cout << "Parsed value = " << parsed_value << std::endl;
	std::cout << "Literal value = " << literal_value << std::endl;
	std::cout << "Are literals equal? " << (parsed_value == literal_value ? "yes" : "no") << std::endl;
	return 0;
}