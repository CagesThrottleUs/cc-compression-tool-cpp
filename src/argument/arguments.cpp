#include <filesystem>
#include <string>
#include <vector>

#include "../exceptions/argument_exception.hpp"
#include "argument.hpp"

namespace argument {

namespace {

constexpr const char* COMPRESS_FLAG = "--compress";
constexpr const char* DECOMPRESS_FLAG = "--decompress";
constexpr const char* DEFAULT_COMPRESSION_OUT_FILENAME = "output.compressed";
constexpr const char* DEFAULT_DECOMPRESSION_OUT_FILENAME =
    "output.decompressed";

}  // namespace

auto validate_arguments(const argv_view& args_view) -> validated_args {
  const std::vector<std::string> args(args_view.begin(), args_view.end());

  if (args.size() < 2) {
    throw exceptions::argument_exception(
        "Usage: " + args.at(0) +
        " [--compress|--decompress] <input_file> [output_file]\n"
        "  Default is compress. Use --decompress to decompress.");
  }

  validated_args result{};
  std::size_t idx = 1;  // skip program name

  // Optional --compress or --decompress
  if (idx < args.size()) {
    const std::string& arg = args.at(idx);
    if (arg == COMPRESS_FLAG) {
      result.operation = mode::compress;
      ++idx;
    } else if (arg == DECOMPRESS_FLAG) {
      result.operation = mode::decompress;
      ++idx;
    }
    // else: no flag, keep default compress
  }

  // Require input file
  if (idx >= args.size()) {
    throw exceptions::argument_exception(
        "Usage: " + args.at(0) +
        " [--compress|--decompress] <input_file> [output_file]\n"
        "  Missing input file.");
  }
  result.input_path = args.at(idx);
  ++idx;

  // Optional output file
  if (idx < args.size()) {
    result.output_path = args.at(idx);
    ++idx;
  } else {
    result.output_path = (result.operation == mode::compress)
                             ? DEFAULT_COMPRESSION_OUT_FILENAME
                             : DEFAULT_DECOMPRESSION_OUT_FILENAME;
  }

  if (idx != args.size()) {
    throw exceptions::argument_exception(
        "Usage: " + args.at(0) +
        " [--compress|--decompress] <input_file> [output_file]\n"
        "  Too many arguments.");
  }

  // Input file must exist
  if (!std::filesystem::exists(result.input_path)) {
    throw exceptions::argument_exception("Input file does not exist: " +
                                         result.input_path);
  }

  // Output file must not exist
  if (std::filesystem::exists(result.output_path)) {
    throw exceptions::argument_exception("Output file already exists: " +
                                         result.output_path);
  }

  return result;
}

}  // namespace argument
