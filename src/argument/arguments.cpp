#include <filesystem>
#include <span>

#include "../exceptions/argument_exception.hpp"
#include "argument.hpp"

namespace argument {

auto validate_arguments(const argv_view& args_view) -> std::span<char* const> {
  std::span<char*> args(args_view);

  // Must match exact number of arguments
  if (args.size() < 2 || args.size() > 3) {
    throw exceptions::argument_exception(
        "Usage: " + std::string(args[0]) +
        " <text_file_path> <optional_output_file_path>");
  }

  // Input file must exist
  if (!std::filesystem::exists(args[1])) {
    throw exceptions::argument_exception("Input file does not exist: " +
                                         std::string(args[1]));
  }

  // output file must not exist
  if (args.size() == 3 && std::filesystem::exists(args[2])) {
    throw exceptions::argument_exception("Output file already exists: " +
                                         std::string(args[2]));
  }

  return args;
}

auto get_output_file_path(const std::span<char* const>& args) -> std::string {
  if (args.size() == 3) {
    return {args[2]};
  }
  return "output.compressed";
}

}  // namespace argument
