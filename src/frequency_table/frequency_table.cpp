#include "frequency_table.hpp"

#include <utf8.h>

#include <algorithm>
#include <array>
#include <boost/unordered/unordered_flat_map.hpp>
#include <functional>
#include <iterator>
#include <ranges>

#include "../exceptions/file_operation_exception.hpp"

namespace frequency_table {

constexpr auto ASCII_COUNT = 128;

auto build_frequency_table(
    std::unique_ptr<file_handler::input_file> input,
    build_progress_callback* progress) -> table {
  std::array<multiprecision::mpz_int, ASCII_COUNT> ascii_counts{{0}};
  boost::unordered_flat_map<char32_t, multiprecision::mpz_int> unicode_counts;

  const auto* const data_start = input->data();
  const std::size_t total_bytes = input->size();
  const auto* end =
      std::next(data_start, static_cast<std::ptrdiff_t>(total_bytes));
  const auto* curr = data_start;
  std::size_t last_reported = 0;
  constexpr std::size_t PROGRESS_INTERVAL_BYTES = 65536;

  try {
    while (curr < end) {
      char32_t codepoint = utf8::next(curr, end);
      if (codepoint < ASCII_COUNT) {
        [[likely]]  // Hint to compiler for common
        ascii_counts.at(static_cast<std::size_t>(codepoint))++;
      } else {
        unicode_counts[codepoint]++;
      }
      if (progress != nullptr && total_bytes > 0) {
        const auto current_bytes =
            static_cast<std::size_t>(curr - data_start);
        if (current_bytes - last_reported >= PROGRESS_INTERVAL_BYTES ||
            curr >= end) {
          (*progress)(current_bytes, total_bytes);
          last_reported = current_bytes;
        }
      }
    }
  } catch (const std::exception& e) {
    throw exceptions::file_operation_exception(
        "Error building frequency table: " + std::string(e.what()) +
        " at offset " + std::to_string(std::distance(input->data(), curr)));
  }
  if (progress != nullptr && total_bytes > 0) {
    (*progress)(total_bytes, total_bytes);
  }

  table result;
  result.reserve(ascii_counts.size() + unicode_counts.size());

  for (std::size_t i = 0; i < ASCII_COUNT; i++) {
    if (ascii_counts.at(i) > 0) {
      result.push_back({static_cast<char32_t>(i), ascii_counts.at(i)});
    }
  }
  for (const auto& [codepoint, count] : unicode_counts) {
    result.push_back({codepoint, count});
  }

  std::ranges::sort(result, std::less<>());

  return result;
}

void print_frequency_table(const table& table) {
  // print reverse as sorted in ascending order
  for (const auto& [codepoint, count] : std::ranges::reverse_view(table)) {
    std::cout << file_handler::codepoint_to_utf8(codepoint) << " " << count
              << '\n';
  }
}

}  // namespace frequency_table
