#include <iostream>

#include "argument/argument.hpp"
#include "compress.hpp"
#include "decompress.hpp"
#include "exceptions/argument_exception.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "exit_codes.hpp"

auto main(int argc, char** argv) -> int {
  argument::argv_view args(argv, static_cast<std::size_t>(argc));

  try {
    std::cout << "Validating arguments...\n";
    auto validated = argument::validate_arguments(args);

    if (validated.operation == argument::mode::compress) {
      compress::run(validated);
    } else {
      decompress::run(validated);
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
