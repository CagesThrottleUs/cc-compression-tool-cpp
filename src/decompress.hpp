#ifndef DECOMPRESS_HPP
#define DECOMPRESS_HPP

#include <fstream>
#include <iostream>

#include "argument/argument.hpp"
#include "exceptions/file_operation_exception.hpp"
#include "file_handler/compressed_format.hpp"
#include "progress.hpp"

namespace decompress {

/**
 * @brief Runs decompression: reads header from compressed file, decodes
 *        contents, and writes UTF-8 to the output path.
 * @param validated Validated arguments (operation must be decompress).
 * @throws exceptions::file_operation_exception On read or write error.
 */
inline void run(const argument::validated_args& validated) {
  std::cout << "Opening compressed file...\n";
  std::ifstream stream(validated.input_path, std::ios::binary);
  if (!stream.is_open()) {
    throw exceptions::file_operation_exception("Failed to open file: " +
                                                validated.input_path);
  }

  std::cout << "Reading header...\n";
  auto prefixes = file_handler::read_header(stream);

  std::cout << "Creating output file...\n";
  file_handler::output_file out(validated.output_path);

  std::cout << "Decoding and writing...\n";
  file_handler::read_progress_callback on_progress =
      [](std::size_t cur, std::size_t tot) -> void {
    progress::print_progress(cur, tot, "Decoding ");
  };
  file_handler::decode_and_write(stream, prefixes, out, &on_progress);

  std::cout << "\nDecompression complete.\n";
}

}  // namespace decompress

#endif  // DECOMPRESS_HPP
