#include <iostream>
#include <utility>

#include "argument/argument.hpp"
#include "exceptions/argument_exception.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "exit_codes.hpp"
#include "file_handler/input_file.hpp"
#include "frequency_table/frequency_table.hpp"
#include "prefix_codes/prefix_codes.hpp"

auto main(int argc, char** argv) -> int {
  argument::argv_view args(argv, static_cast<std::size_t>(argc));

  try {
    auto validated_args = argument::validate_arguments(args);
    auto table = frequency_table::build_frequency_table(
        file_handler::load_file(validated_args[1]));
    frequency_table::print_frequency_table(table);
    auto prefixes = prefix_codes::generate_prefix_codes(table);
    for (const auto& [codepoint, prefix] : prefixes) {
      std::cout << file_handler::codepoint_to_utf8(codepoint) << " " << prefix
                << '\n';
    }
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
