#ifndef FREQUENCY_TABLE_HPP
#define FREQUENCY_TABLE_HPP

#include <boost/multiprecision/gmp.hpp>
#include <functional>
#include <memory>
#include <vector>

#include "../file_handler/input_file.hpp"

namespace multiprecision = boost::multiprecision;

namespace frequency_table {

/**
 * @brief A node in the frequency table.
 *
 * This struct represents a node in the frequency table. It contains the
 * codepoint and the count of the codepoint.
 */
struct node {
  char32_t codepoint{};  // NOLINT(misc-non-private-member-variables-in-classes)
  multiprecision::mpz_int
      count;  // NOLINT(misc-non-private-member-variables-in-classes)

  auto operator<(const node& other) const -> bool {
    return count < other.count;
  }
};

/**
 * @brief A frequency table.
 *
 * This vector represents a frequency table. It contains a vector of nodes,
 * each representing a codepoint and its count.
 */
using table = std::vector<node>;

/**
 * @brief Progress callback: (current_bytes, total_bytes) during scan.
 */
using build_progress_callback =
    std::function<void(std::size_t current, std::size_t total)>;

/**
 * @brief Builds a frequency table from an input file.
 *
 * This function builds a frequency table from an input file. It reads the
 * input file and constructs a frequency table from the contents.
 */
[[nodiscard]] auto build_frequency_table(
    std::unique_ptr<file_handler::input_file> input,
    build_progress_callback* progress = nullptr) -> table;

/**
 * @brief Prints a frequency table to the console.
 *
 * This function prints a frequency table to the console. It prints the
 * codepoint and the count of the codepoint.
 */
void print_frequency_table(const table& table);

}  // namespace frequency_table

#endif  // FREQUENCY_TABLE_HPP
