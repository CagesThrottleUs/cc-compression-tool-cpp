#include <algorithm>
#include <cmath>
#include <iostream>
#include <utility>

#include "argument/argument.hpp"
#include "exceptions/argument_exception.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "exit_codes.hpp"
#include "file_handler/input_file.hpp"
#include "file_handler/output_file.hpp"
#include "frequency_table/frequency_table.hpp"
#include "prefix_codes/prefix_codes.hpp"

namespace {

constexpr int kProgressBarWidth = 40;

void print_progress(std::size_t current, std::size_t total,
                    const char* label = "Writing") {
  if (total == 0) {
    std::cout << '\r' << label << " [========================================] "
              << "100%\n"
              << std::flush;
    return;
  }
  const int pct = static_cast<int>(
      std::min(100.0, std::round(100.0 * static_cast<double>(current) /
                                 static_cast<double>(total))));
  const int filled =
      static_cast<int>(std::round(static_cast<double>(pct) / 100.0 *
                                  static_cast<double>(kProgressBarWidth)));
  std::cout << '\r' << label << " [";
  for (int i = 0; i < kProgressBarWidth; ++i) {
    char segment = ' ';
    if (i < filled) {
      segment = '=';
    } else if (i == filled) {
      segment = '>';
    }
    std::cout << segment;
  }
  std::cout << "] " << pct << "%" << std::flush;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  argument::argv_view args(argv, static_cast<std::size_t>(argc));

  try {
    std::cout << "Validating arguments...\n";
    auto validated_args = argument::validate_arguments(args);
    std::cout << "Building frequency table...\n";
    frequency_table::build_progress_callback build_progress =
        [&](std::size_t cur, std::size_t tot) -> void {
      print_progress(cur, tot, "Building frequency table ");
    };
    auto table = frequency_table::build_frequency_table(
        file_handler::load_file(validated_args[1]), &build_progress);
    std::cout << "\nBuilt frequency table, printing...\n";
    frequency_table::print_frequency_table(table);
    std::cout << "Generated prefix codes, printing...\n";
    auto prefixes = prefix_codes::generate_prefix_codes(table);
    for (const auto& [codepoint, prefix] : prefixes) {
      std::cout << file_handler::codepoint_to_utf8(codepoint) << " " << prefix
                << '\n';
    }

    std::cout << "Getting output file path...\n";
    auto out_file_path = argument::get_output_file_path(validated_args);
    std::cout << "Creating output file...\n";
    file_handler::output_file out(out_file_path);
    std::cout << "Writing header to output file...\n";
    file_handler::write_header(prefixes, out);
    std::cout << "Writing file contents to output file...\n";
    file_handler::write_progress_callback on_progress =
        [&](std::size_t cur, std::size_t tot) -> void {
      print_progress(cur, tot, "Writing output ");
    };
    file_handler::write_file_contents(
        file_handler::load_file(validated_args[1]), prefixes, out,
        &on_progress);
    std::cout << "\nWrote header and file contents to output file.\n";
  } catch (const exceptions::argument_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(exit_codes::exit_code::argument_error);
  } catch (const exceptions::file_operation_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(exit_codes::exit_code::file_operation_error);
  } catch (...) {
    std::cerr << "Unknown error\n";
    return std::to_underlying(exit_codes::exit_code::unknown_error);
  }

  return std::to_underlying(exit_codes::exit_code::success);
}
