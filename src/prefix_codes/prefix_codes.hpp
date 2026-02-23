#ifndef PREFIX_CODES_HPP
#define PREFIX_CODES_HPP

#include <map>

#include "../frequency_table/frequency_table.hpp"

namespace prefix_codes {

auto generate_prefix_codes(const frequency_table::table& table)
    -> std::map<char32_t, std::string>;

}  // namespace prefix_codes

#endif  // PREFIX_CODES_HPP
