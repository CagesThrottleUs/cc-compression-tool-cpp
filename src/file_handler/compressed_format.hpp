#ifndef COMPRESSED_FORMAT_HPP
#define COMPRESSED_FORMAT_HPP

#include <functional>
#include <istream>
#include <map>
#include <string>

#include "output_file.hpp"

namespace file_handler {

/**
 * @brief Progress callback for decode: (current_bits, total_bits).
 */
using read_progress_callback =
    std::function<void(std::size_t current, std::size_t total)>;

/**
 * @brief Reads the prefix-code header from a compressed stream.
 * @param stream Binary input stream positioned at the start of the header.
 * @return Map from codepoint to code string (e.g. "0", "10", "11").
 * @throws exceptions::file_operation_exception On read or format error.
 */
[[nodiscard]] auto read_header(std::istream& stream)
    -> std::map<char32_t, std::string>;

/**
 * @brief Reads encoded bits from stream, decodes using the prefix map, and
 *        writes UTF-8 to the output file.
 * @param stream Binary input stream positioned after the header (at
 * total_bits).
 * @param prefixes Map from codepoint to code string (from read_header).
 * @param file Output file to write decoded UTF-8.
 * @param progress Optional progress callback (current_bits, total_bits).
 * @throws exceptions::file_operation_exception On read or decode error.
 */
void decode_and_write(std::istream& stream,
                      const std::map<char32_t, std::string>& prefixes,
                      output_file& file,
                      read_progress_callback* progress = nullptr);

}  // namespace file_handler

#endif  // COMPRESSED_FORMAT_HPP
