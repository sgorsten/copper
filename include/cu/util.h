#ifndef COPPER_UTIL_H
#define COPPER_UTIL_H

#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <cmath>

#include <functional>
#include <sstream>
#include <vector>

namespace cu
{
    template<class T> struct tabbed_ref { const T & value; int tabWidth, indent; };
    template<class T> tabbed_ref<T> tabbed(const T & value, int tabWidth, int indent = 0) { return {value, tabWidth, indent}; }
}

#endif