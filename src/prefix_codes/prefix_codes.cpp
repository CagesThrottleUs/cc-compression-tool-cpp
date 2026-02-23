#include "prefix_codes.hpp"

#include <cstddef>
#include <memory>
#include <queue>
#include <stack>
#include <vector>

namespace prefix_codes {

namespace detail {

namespace multiprecision = boost::multiprecision;

/**
 * @brief A node in the Huffman tree.
 */
struct huffman_node {
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  multiprecision::mpz_int count;
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::shared_ptr<huffman_node> left{nullptr};
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  std::shared_ptr<huffman_node> right{nullptr};
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  char32_t codepoint{0};

  // Explicit padding to align the struct size to an 8-byte boundary.
  // This resolves the -Wpadded warning by making the tail padding intentional.
  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  uint32_t padding_{0};

  [[nodiscard]] auto is_leaf() const -> bool { return !left && !right; }
};

namespace {

/**
 * @brief Helper to pick the smallest node from two queues.
 * Refactored to avoid 'bugprone-branch-clone'.
 */
auto pop_smallest(std::queue<std::shared_ptr<huffman_node>>& first,
                  std::queue<std::shared_ptr<huffman_node>>& second)
    -> std::shared_ptr<huffman_node> {
  std::shared_ptr<huffman_node> result;

  // If second is empty, OR first has a smaller/equal value, take from first.
  // This merged condition removes the duplicate branch body.
  if (!first.empty() &&
      (second.empty() || first.front()->count < second.front()->count)) {
    result = first.front();
    first.pop();
  } else {
    result = second.front();
    second.pop();
  }
  return result;
}

/**
 * @brief Builds the Huffman tree using the O(n) two-queue method.
 */
auto build_huffman_tree(const frequency_table::table& sorted_nodes)
    -> std::shared_ptr<huffman_node> {
  std::queue<std::shared_ptr<huffman_node>> first;
  std::queue<std::shared_ptr<huffman_node>> second;

  for (const auto& entry : sorted_nodes) {
    auto leaf = std::make_shared<huffman_node>();
    leaf->codepoint = entry.codepoint;
    leaf->count = entry.count;
    first.push(leaf);
  }

  while ((first.size() + second.size()) > 1) {
    auto left_child = pop_smallest(first, second);
    auto right_child = pop_smallest(first, second);

    auto parent = std::make_shared<huffman_node>();
    parent->count = left_child->count + right_child->count;
    parent->left = left_child;
    parent->right = right_child;
    second.push(parent);
  }

  return second.empty() ? first.front() : second.front();
}

/**
 * @brief Generates codes using an iterative stack to avoid misc-no-recursion.
 */
auto generate_codes_iterative(const std::shared_ptr<huffman_node>& root)
    -> std::map<char32_t, std::string> {
  if (!root) {
    return {};
  }

  std::map<char32_t, std::string> bit_table;

  struct State {
    std::shared_ptr<huffman_node> node;
    std::string path;
  };

  std::stack<State> traversal_stack;
  traversal_stack.push({root, ""});

  while (!traversal_stack.empty()) {
    State current = traversal_stack.top();
    traversal_stack.pop();

    if (current.node->is_leaf()) {
      bit_table[current.node->codepoint] = current.path;
    } else {
      if (current.node->right) {
        traversal_stack.push({current.node->right, current.path + "1"});
      }
      if (current.node->left) {
        traversal_stack.push({current.node->left, current.path + "0"});
      }
    }
  }

  return bit_table;
}

}  // namespace

}  // namespace detail

auto generate_prefix_codes(const frequency_table::table& table)
    -> std::map<char32_t, std::string> {
  const std::size_t node_count = table.size();
  if (node_count == 0) {
    return {};
  }
  if (node_count == 1) {
    return {{table.at(0).codepoint, "0"}};
  }

  // 1. Build the tree
  auto root = detail::build_huffman_tree(table);

  // 2. Traverse the tree iteratively to get the codes
  return detail::generate_codes_iterative(root);
}

}  // namespace prefix_codes
