#include <iostream>
#include <utility>

#include "argument/argument.hpp"
#include "exceptions/argument_exception.hpp"
#include "exit_codes.hpp"

auto main(int argc, char** argv) -> int {
  argument::argv_view args(argv, static_cast<std::size_t>(argc));

  try {
    argument::validate_arguments(args);
  } catch (const exceptions::argument_exception& e) {
    std::cerr << e.what() << '\n';
    return std::to_underlying(exit_codes::exit_code::argument_error);
  } catch (...) {
    std::cerr << "Unknown error\n";
    return std::to_underlying(exit_codes::exit_code::unknown_error);
  }

  return std::to_underlying(exit_codes::exit_code::success);
}
