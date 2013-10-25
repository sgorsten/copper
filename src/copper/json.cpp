#include "cu/json.h"
#include <algorithm>
#include <regex>

namespace cu { namespace json
{
	void print_escaped(std::ostream & out, const std::string & str)
	{
		// Escape sequences for ", \, and control characters, 0 indicates no escaping needed
		static const char * escapes[256] = {
			"\\u0000", "\\u0001", "\\u0002", "\\u0003", "\\u0004", "\\u0005", "\\u0006", "\\u0007",
			"\\b", "\\t", "\\n", "\\u000B", "\\f", "\\r", "\\u000E", "\\u000F",
			"\\u0010", "\\u0011", "\\u0012", "\\u0013", "\\u0014", "\\u0015", "\\u0016", "\\u0017",
			"\\u0018", "\\u0019", "\\u001A", "\\u001B", "\\u001C", "\\u001D", "\\u001E", "\\u001F",
			0, 0, "\\\"", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "\\\\", 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "\\u007F"
		};
		out << '"';
		for (uint8_t ch : str)
		{
			if (escapes[ch]) out << escapes[ch];
			else out << ch;
		}
		out << '"';
	}

	void packed_print(std::ostream & out, const array & arr)
	{
		int i = 0;
		out << '[';
		for (auto & val : arr) packed_print(out << (i++ ? "," : ""), val);
		out << ']';
	}

	void packed_print(std::ostream & out, const object & obj)
	{
		int i = 0;
		out << '{';
		for (auto & kvp : obj)
		{
			print_escaped(out << (i++ ? "," : ""), kvp.first);
			packed_print(out << ':', kvp.second);
		}
		out << '}';
	}

	void packed_print(std::ostream & out, const value & val)
	{
		if (val.is_null()) out << "null";
		else if (val.is_false()) out << "false";
		else if (val.is_true()) out << "true";
		else if (val.is_string()) print_escaped(out, val.contents());
		else if (val.is_number()) out << val.contents();
		else if (val.is_array()) out << val.array();
		else out << val.object();
	}

	static std::ostream & indent(std::ostream & out, int tabs, int n = 0)
	{
		if (n) out << ',';
		out << '\n';
		for (int i = 0; i < tabs * 4; ++i) out << ' ';
		return out;
	}

	void pretty_print(std::ostream & out, const array & arr, int tabs)
	{
		if (std::none_of(begin(arr), end(arr), [](const value & val) { return val.is_array() || val.is_object(); })) packed_print(out, arr);
		else
		{
			int i = 0;
			out << '[';
			for (auto & val : arr) pretty_print(indent(out, tabs + 1, i++), val, tabs + 1);
			indent(out, tabs) << ']';
		}
	}

	void pretty_print(std::ostream & out, const object & obj, int tabs)
	{
		if (obj.empty()) out << "{}";
		else
		{
			int i = 0;
			out << '{';
			for (auto & kvp : obj)
			{
				print_escaped(indent(out, tabs + 1, i++), kvp.first);
				pretty_print(out << ": ", kvp.second, tabs + 1);
			}
			indent(out, tabs) << '}';
		}
	}

	void pretty_print(std::ostream & out, const value & val, int tabs)
	{
		if (val.is_array()) pretty_print(out, val.array(), tabs);
		else if (val.is_object()) pretty_print(out, val.object(), tabs);
		else packed_print(out, val);
	}

	bool is_number(const std::string & num)
	{
		static const std::regex regex(R"(-?(0|([1-9][0-9]*))((\.[0-9]+)?)(((e|E)((\+|-)?)[0-9]+)?))");
		return std::regex_match(begin(num), end(num), regex);
	}

	static uint16_t decode_hex(char ch) {
		if (ch >= '0' && ch <= '9') return ch - '0';
		if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
		if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
		throw not_json(std::string("invalid hex digit: ") + ch);
	}

	static std::string decode_string(std::string::const_iterator first, std::string::const_iterator last)
	{
		if (std::any_of(first, last, iscntrl)) throw not_json("control character found in string literal");
		if (std::find(first, last, '\\') == last) return std::string(first, last); // No escape characters, use the string directly
		std::string s; s.reserve(last - first); // Reserve enough memory to hold the entire string
		for (; first < last; ++first)
		{
			if (*first != '\\') s.push_back(*first);
			else switch (*(++first))
			{
			case '"': s.push_back('"'); break;
			case '\\': s.push_back('\\'); break;
			case '/': s.push_back('/'); break;
			case 'b': s.push_back('\b'); break;
			case 'f': s.push_back('\f'); break;
			case 'n': s.push_back('\n'); break;
			case 'r': s.push_back('\r'); break;
			case 't': s.push_back('\t'); break;
			case 'u':
				if (first + 5 > last) throw not_json("incomplete escape sequence: " + std::string(first - 1, last));
				else
				{
					uint16_t val = (decode_hex(first[1]) << 12) | (decode_hex(first[2]) << 8) | (decode_hex(first[3]) << 4) | decode_hex(first[4]);
					if (val < 0x80) s.push_back(static_cast<char>(val)); // ASCII codepoint, no translation needed
					else if (val < 0x800) // 2-byte UTF-8 encoding
					{
						s.push_back(0xC0 | ((val >> 6) & 0x1F)); // Leading byte: 5 content bits
						s.push_back(0x80 | ((val >> 0) & 0x3F)); // Continuation byte: 6 content bits
					}
					else // 3-byte UTF-8 encoding (16 content bits, sufficient to store all \uXXXX patterns)
					{
						s.push_back(0xE0 | ((val >> 12) & 0x0F)); // Leading byte: 4 content bits
						s.push_back(0x80 | ((val >> 6) & 0x3F)); // Continuation byte: 6 content bits
						s.push_back(0x80 | ((val >> 0) & 0x3F)); // Continuation byte: 6 content bits
					}
					first += 4;
				}
				break;
			default: throw not_json("invalid escape sequence");
			}
		}
		return s;
	}

	struct token { char type; std::string value; token(char type, std::string value = std::string()) : type(type), value(move(value)) {} };

	std::vector<token> lex(const std::string & text)
	{
		std::vector<token> tokens;
		auto it = begin(text);
		while (true)
		{
			it = std::find_if_not(it, end(text), isspace); // Skip whitespace
			if (it == end(text))
			{
				tokens.emplace_back('$');
				return tokens;
			}
			switch (*it)
			{
			case '[': case ']': case ',':
			case '{': case '}': case ':':
				tokens.push_back({ *it++ });
				break;
			case '"':
			{
						auto it2 = ++it;
						for (; it2 < end(text); ++it2)
						{
							if (*it2 == '"') break;
							if (*it2 == '\\') ++it2;
						}
						if (it2 < end(text))
						{
							tokens.emplace_back('"', decode_string(it, it2));
							it = it2 + 1;
						}
						else throw not_json("String missing closing quote");
			}
				break;
			case '-': case '0': case '1': case '2':
			case '3': case '4': case '5': case '6':
			case '7': case '8': case '9':
			{
						  auto it2 = std::find_if_not(it, end(text), [](char ch) { return isalnum(ch) || ch == '+' || ch == '-' || ch == '.'; });
						  auto num = std::string(it, it2);
						  if (!is_number(num)) throw not_json("Invalid number: " + num);
						  tokens.emplace_back('#', move(num));
						  it = it2;
			}
				break;
			default:
				if (isalpha(*it))
				{
					auto it2 = std::find_if_not(it, end(text), isalpha);
					if (std::equal(it, it2, "true")) tokens.emplace_back('t');
					else if (std::equal(it, it2, "false")) tokens.emplace_back('f');
					else if (std::equal(it, it2, "null")) tokens.emplace_back('n');
					else throw not_json("Invalid token: " + std::string(it, it2));
					it = it2;
				}
				else throw not_json("Invalid character: \'" + std::string(1, *it) + '"');
			}
		}
	}

	struct parser
	{
		std::vector<token>::iterator it, last;

		bool match_and_discard(char type) { if (it->type != type) return false; ++it; return true; }
		void discard_expected(char type, const char * what) { if (!match_and_discard(type)) throw not_json(std::string("Syntax error: Expected ") + what); }

		value parse_value()
		{
			auto token = it++;
			switch (token->type)
			{
			case 'n': return nullptr;
			case 'f': return false;
			case 't': return true;
			case '"': return token->value;
			case '#': return value::from_number(token->value);
			case '[':
				if (match_and_discard(']')) return json::array{};
				else
				{
					array arr;
					while (true)
					{
						arr.push_back(parse_value());
						if (match_and_discard(']')) return arr;
						discard_expected(',', ", or ]");
					}
				}
			case '{':
				if (match_and_discard('}')) return json::object{};
				else
				{
					object obj;
					while (true)
					{
						auto name = move(it->value);
						discard_expected('"', "string");
						discard_expected(':', ":");
						obj.emplace_back(move(name), parse_value());
						if (match_and_discard('}')) { return obj; }
						discard_expected(',', ", or }");
					}
				}
			default: throw not_json("Expected value");
			}
		}
	};

	value parse(const std::string & text)
	{
		auto tokens = lex(text);
		parser p = { begin(tokens), end(tokens) };
		auto val = p.parse_value();
		p.discard_expected('$', "end-of-stream");
		return val;
	}
} }