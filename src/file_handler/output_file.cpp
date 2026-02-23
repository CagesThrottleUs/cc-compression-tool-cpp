#include "output_file.hpp"

#include <utf8.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <vector>

namespace file_handler {

namespace {

constexpr unsigned int BITS_PER_BYTE = 8U;
constexpr unsigned int LOW_BYTE_MASK = 0xFFU;

/**
 * Packs a string of '0' and '1' into bytes (MSB first).
 * Returns a string of raw bytes. Padding bits in the last byte are zero.
 */
auto pack_bits(const std::string& code_str) -> std::string {
  if (code_str.empty()) {
    return {};
  }
  const std::size_t num_bits = code_str.size();
  const std::size_t num_bytes = (num_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
  std::string out(num_bytes, '\0');
  for (std::size_t i = 0; i < num_bits; ++i) {
    if (code_str.at(i) == '1') {
      const std::size_t byte_idx = i / BITS_PER_BYTE;
      const unsigned int bit_idx =
          BITS_PER_BYTE - 1 - static_cast<unsigned int>(i % BITS_PER_BYTE);
      out.at(byte_idx) = static_cast<char>(
          static_cast<unsigned char>(out.at(byte_idx)) | (1U << bit_idx));
    }
  }
  return out;
}

/**
 * Writes a fixed-width integer in big-endian order.
 */
template <typename T>
auto write_be(output_file& file, T value) noexcept -> void {
  constexpr std::size_t byte_count = sizeof(T);
  std::string buf(byte_count, '\0');
  auto remaining = static_cast<uint64_t>(value);
  for (std::size_t idx = 0; idx < byte_count; ++idx) {
    buf.at(byte_count - 1 - idx) = static_cast<char>(remaining & LOW_BYTE_MASK);
    remaining >>= BITS_PER_BYTE;
  }
  file.write(buf);
}

}  // namespace

auto write_header(const std::map<char32_t, std::string>& prefixes,
                  output_file& file) noexcept -> void {
  const auto count = prefixes.size();
  if (count > static_cast<std::size_t>(std::numeric_limits<uint32_t>::max())) {
    return;
  }
  write_be(file, static_cast<uint32_t>(count));

  for (const auto& [codepoint, code_str] : prefixes) {
    const auto code_len = code_str.size();
    if (code_len >
        static_cast<std::size_t>(std::numeric_limits<uint8_t>::max())) {
      continue;
    }
    write_be(file, static_cast<uint32_t>(codepoint));
    write_be(file, static_cast<uint8_t>(code_len));
    const std::string packed = pack_bits(code_str);
    if (!packed.empty()) {
      file.write(packed);
    }
  }
}

auto write_file_contents(std::unique_ptr<file_handler::input_file> input,
                         const std::map<char32_t, std::string>& prefixes,
                         output_file& file,
                         write_progress_callback* progress) noexcept -> void {
  if (!input || !input->good()) {
    return;
  }

  constexpr std::size_t INIT_BUFFER_SIZE = 4096;
  std::vector<char> bit_buffer;
  bit_buffer.reserve(INIT_BUFFER_SIZE);

  const auto* const data_start = input->data();
  const std::size_t total_bytes = input->size();
  const auto* end =
      std::next(data_start, static_cast<std::ptrdiff_t>(total_bytes));
  const auto* curr = data_start;
  std::size_t last_reported = 0;
  constexpr std::size_t PROGRESS_INTERVAL_BYTES = 65536;

  while (curr < end) {
    const char32_t codepoint = utf8::next(curr, end);
    const auto entry = prefixes.find(codepoint);
    if (entry == prefixes.end()) {
      continue;
    }
    for (const char bit_char : entry->second) {
      bit_buffer.push_back(bit_char);
    }
    if (progress != nullptr && total_bytes > 0) {
      const auto current_bytes = static_cast<std::size_t>(curr - data_start);
      if (current_bytes - last_reported >= PROGRESS_INTERVAL_BYTES ||
          curr >= end) {
        (*progress)(current_bytes, total_bytes);
        last_reported = current_bytes;
      }
    }
  }
  if (progress != nullptr && total_bytes > 0) {
    (*progress)(total_bytes, total_bytes);
  }

  const uint64_t total_bits = bit_buffer.size();
  write_be(file, total_bits);

  if (total_bits == 0) {
    return;
  }

  const std::size_t num_bytes =
      (static_cast<std::size_t>(total_bits) + BITS_PER_BYTE - 1) /
      BITS_PER_BYTE;
  std::string out(num_bytes, '\0');
  for (std::size_t i = 0; i < static_cast<std::size_t>(total_bits); ++i) {
    if (bit_buffer.at(i) == '1') {
      const std::size_t byte_idx = i / BITS_PER_BYTE;
      const unsigned int bit_idx =
          BITS_PER_BYTE - 1 - static_cast<unsigned int>(i % BITS_PER_BYTE);
      out.at(byte_idx) = static_cast<char>(
          static_cast<unsigned char>(out.at(byte_idx)) | (1U << bit_idx));
    }
  }
  file.write(out);
}

}  // namespace file_handler
