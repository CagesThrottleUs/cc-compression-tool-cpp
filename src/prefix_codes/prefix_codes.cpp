#include "prefix_codes.hpp"

#include <algorithm>
#include <cstddef>
#include <queue>
#include <vector>

namespace prefix_codes {

namespace detail {

namespace multiprecision = boost::multiprecision;

/**
 * @brief A meta node in the prefix codes.
 *
 * This struct represents a meta node in the prefix codes. It contains the
 * parent of the node and the count of the node.
 */
struct meta_node {
  multiprecision::mpz_int count;
  // Use ptrdiff_t (8 bytes) instead of int (4 bytes) to satisfy alignment
  // without needing tail padding.
  std::ptrdiff_t parent = -1;
};

namespace {

/**
 * @brief Builds the parent array for the prefix codes.
 *
 * This function builds the parent array for the prefix codes. It builds the
 * parent array for the prefix codes.
 */
auto build_parent_array(const frequency_table::table& leaves)
    -> std::vector<meta_node> {
  std::size_t leaves_len = leaves.size();
  std::vector<meta_node> nodes((2 * leaves_len) - 1);
  for (size_t i = 0; i < leaves_len; ++i) {
    nodes[i].count = leaves[i].count;
  }

  std::queue<std::size_t> first_queue;
  std::queue<std::size_t> second_queue;
  for (size_t i = 0; i < leaves_len; ++i) {
    first_queue.push(i);
  }
  auto pop_min = [&](std::queue<std::size_t>& first,
                     std::queue<std::size_t>& second) {
    std::size_t idx{};
    if (second.empty() || (!first.empty() && nodes[first.front()].count <
                                                 nodes[second.front()].count)) {
      idx = first.front();
      first.pop();
    } else {
      idx = second.front();
      second.pop();
    }
    return idx;
  };

  for (std::size_t i = 0; i < leaves_len - 1; ++i) {
    std::size_t left = pop_min(first_queue, second_queue);
    std::size_t right = pop_min(first_queue, second_queue);
    std::size_t parent_idx = leaves_len + i;

    nodes[parent_idx].count = nodes[left].count + nodes[right].count;
    nodes[left].parent = static_cast<std::ptrdiff_t>(parent_idx);
    nodes[right].parent = static_cast<std::ptrdiff_t>(parent_idx);
    second_queue.push(parent_idx);
  }
  return nodes;
}

auto calculate_bit_len(const std::vector<meta_node>& nodes,
                       std::size_t leaves_size) -> std::vector<int> {
  std::vector<int> lengths(leaves_size);
  for (std::size_t i = 0; i < leaves_size; ++i) {
    int depth = 0;
    std::size_t current = i;
    while (nodes[current].parent != -1) {
      // Cast the ptrdiff_t parent to size_t for the index
      current = static_cast<std::size_t>(nodes[current].parent);
      depth++;
    }
    lengths[i] = depth;
  }
  return lengths;
}

auto generate_canonical_codes(const std::vector<int>& lengths)
    -> std::vector<unsigned int> {
  if (lengths.empty()) {
    return {};
  }

  int max_len = *std::ranges::max_element(lengths);
  std::vector<int> length_counts(static_cast<std::size_t>(max_len + 1), 0);
  for (int length : lengths) {
    length_counts[static_cast<std::size_t>(length)]++;
  }

  std::vector<unsigned int> next_code(static_cast<std::size_t>(max_len + 1), 0);
  unsigned int code = 0;
  for (int len = 1; len <= max_len; ++len) {
    // Fix: Use 1u to ensure bitwise operation is on unsigned types
    code = (code + static_cast<unsigned int>(
                       length_counts[static_cast<std::size_t>(len - 1)]))
           << 1U;
    next_code[static_cast<std::size_t>(len)] = code;
  }

  return next_code;
}

auto assign_final_prefix(const std::vector<int>& lengths,
                         const frequency_table::table& leaves,
                         std::vector<unsigned int>& next_code)
    -> std::map<char32_t, std::string> {
  std::map<char32_t, std::string> prefixes;
  for (size_t i = 0; i < leaves.size(); ++i) {
    int len = lengths[i];
    unsigned int code = next_code[static_cast<std::size_t>(len)]++;

    std::string bit_str;
    bit_str.reserve(static_cast<std::size_t>(len));  // Optimization

    for (int bit = len - 1; bit >= 0; --bit) {
      // Fix:
      // 1. Cast 'bit' to unsigned for HICPP compliance
      // 2. Use 1u literal
      // 3. Explicit comparison '!= 0u' for bool conversion
      const bool is_bit_set =
          ((code >> static_cast<unsigned int>(bit)) & 1U) != 0U;
      bit_str += is_bit_set ? '1' : '0';
    }
    prefixes[leaves[i].codepoint] = bit_str;
  }

  return prefixes;
}

}  // namespace

}  // namespace detail

auto generate_prefix_codes(const frequency_table::table& table)
    -> std::map<char32_t, std::string> {
  std::size_t node_count = table.size();
  if (node_count == 0) {
    return {};
  }
  if (node_count == 1) {
    return {{table.at(0).codepoint, "0"}};
  }
  auto nodes = detail::build_parent_array(table);
  auto lengths = detail::calculate_bit_len(nodes, node_count);
  auto next_code = detail::generate_canonical_codes(lengths);
  return detail::assign_final_prefix(lengths, table, next_code);
}

}  // namespace prefix_codes
