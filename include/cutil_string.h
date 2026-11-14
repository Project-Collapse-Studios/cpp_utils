#pragma once
#include <algorithm>
#include <string_view>



/* Holds some QoL string functions
 */


namespace cutil {

    /// Caseless comparison of two strings. Credit goes to CraftableScience/sourcepp
    inline bool iequals(std::string_view s1, std::string_view s2) {
        return std::ranges::equal(s1, s2, [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    }
}
