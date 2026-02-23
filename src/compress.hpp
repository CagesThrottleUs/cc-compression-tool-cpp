#ifndef COMPRESS_HPP
#define COMPRESS_HPP

#include "argument/argument.hpp"
#include "file_handler/input_file.hpp"
#include "file_handler/output_file.hpp"
#include "frequency_table/frequency_table.hpp"
#include "prefix_codes/prefix_codes.hpp"
#include "progress.hpp"

namespace compress {

/**
 * @brief Runs compression: build frequency table, generate prefix codes,
 *        write header and encoded contents to the output path.
 * @param validated Validated arguments (operation must be compress).
 */
inline void run(const argument::validated_args& validated) {
  std::cout << "Building frequency table...\n";
  frequency_table::build_progress_callback build_progress =
      [](std::size_t cur, std::size_t tot) -> void {
    progress::print_progress(cur, tot, "Building frequency table ");
  };
  auto table = frequency_table::build_frequency_table(
      file_handler::load_file(validated.input_path), &build_progress);
  std::cout << "\nBuilt frequency table, printing...\n";
  frequency_table::print_frequency_table(table);
  std::cout << "Generated prefix codes, printing...\n";
  auto prefixes = prefix_codes::generate_prefix_codes(table);
  for (const auto& [codepoint, prefix] : prefixes) {
    std::cout << file_handler::codepoint_to_utf8(codepoint) << " " << prefix
              << '\n';
  }

  std::cout << "Creating output file...\n";
  file_handler::output_file out(validated.output_path);
  std::cout << "Writing header to output file...\n";
  file_handler::write_header(prefixes, out);
  std::cout << "Writing file contents to output file...\n";
  file_handler::write_progress_callback on_progress =
      [](std::size_t cur, std::size_t tot) -> void {
    progress::print_progress(cur, tot, "Writing output ");
  };
  file_handler::write_file_contents(
      file_handler::load_file(validated.input_path), prefixes, out,
      &on_progress);
  std::cout << "\nWrote header and file contents to output file.\n";
}

}  // namespace compress

#endif  // COMPRESS_HPP
