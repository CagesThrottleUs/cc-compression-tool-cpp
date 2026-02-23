#include "compressed_format.hpp"

#include <array>
#include <cstdint>
#include <vector>

#include "../exceptions/file_operation_exception.hpp"
#include "input_file.hpp"

namespace file_handler {

namespace {

constexpr unsigned int BITS_PER_BYTE = 8U;
constexpr std::size_t DECODE_BUFFER_BYTES = 65536;
constexpr std::size_t OUTPUT_BUFFER_SIZE = 65536;
constexpr std::size_t INVALID_NODE = static_cast<std::size_t>(-1);

template <typename T>
auto read_be(std::istream* stream) -> T {
  constexpr std::size_t byte_count = sizeof(T);
  std::string buf(byte_count, '\0');
  if (!stream->read(buf.data(), static_cast<std::streamsize>(byte_count)) ||
      stream->gcount() != static_cast<std::streamsize>(byte_count)) {
    throw exceptions::file_operation_exception(
        "Failed to read header field from compressed stream");
  }
  uint64_t value = 0;
  for (std::size_t idx = 0; idx < byte_count; ++idx) {
    value = (value << BITS_PER_BYTE) | static_cast<unsigned char>(buf.at(idx));
  }
  return static_cast<T>(value);
}

/**
 * Unpacks num_bits from packed bytes (MSB first) into a string of '0' and '1'.
 * Used only for header (small).
 */
auto unpack_bits(std::istream* stream, std::size_t num_bits) -> std::string {
  if (num_bits == 0) {
    return {};
  }
  const std::size_t num_bytes = (num_bits + BITS_PER_BYTE - 1) / BITS_PER_BYTE;
  std::string packed(num_bytes, '\0');
  if (!stream->read(packed.data(), static_cast<std::streamsize>(num_bytes)) ||
      stream->gcount() != static_cast<std::streamsize>(num_bytes)) {
    throw exceptions::file_operation_exception(
        "Failed to read packed bits from compressed stream");
  }
  std::string result;
  result.reserve(num_bits);
  for (std::size_t i = 0; i < num_bits; ++i) {
    const std::size_t byte_idx = i / BITS_PER_BYTE;
    const unsigned int bit_idx =
        BITS_PER_BYTE - 1 - static_cast<unsigned int>(i % BITS_PER_BYTE);
    const bool bit_set = (static_cast<unsigned char>(packed.at(byte_idx)) &
                          (1U << bit_idx)) != 0;
    result.push_back(bit_set ? '1' : '0');
  }
  return result;
}

/**
 * Reads bits from stream in chunks; yields one bit at a time without
 * materializing the full bit string.
 */
class BitReader {
 public:
  BitReader(std::istream* stream, std::size_t total_bits)
      : stream_(stream), total_bits_(total_bits) {
    refill();
  }

  [[nodiscard]] auto has_next() const -> bool {
    return bits_read_ < total_bits_;
  }

  auto next_bit() -> bool {
    if (buffer_bit_cursor_ >= buffer_.size() * BITS_PER_BYTE) {
      refill();
    }
    if (buffer_.empty() || bits_read_ >= total_bits_) {
      return false;
    }
    const std::size_t byte_idx = buffer_bit_cursor_ / BITS_PER_BYTE;
    const unsigned int bit_idx =
        BITS_PER_BYTE - 1 -
        static_cast<unsigned int>(buffer_bit_cursor_ % BITS_PER_BYTE);
    const bool bit_set = (static_cast<unsigned char>(buffer_.at(byte_idx)) &
                          (1U << bit_idx)) != 0;
    ++buffer_bit_cursor_;
    ++bits_read_;
    return bit_set;
  }

  [[nodiscard]] auto bits_read() const -> std::size_t { return bits_read_; }
  [[nodiscard]] auto total_bits() const -> std::size_t { return total_bits_; }

 private:
  std::istream* stream_;
  std::size_t total_bits_;
  std::size_t bits_read_{};
  std::size_t buffer_bit_cursor_{};
  std::vector<char> buffer_;

  void refill() {
    buffer_.clear();
    buffer_bit_cursor_ = 0;
    const std::size_t remaining = total_bits_ - bits_read_;
    if (remaining == 0) {
      return;
    }
    const std::size_t to_read = std::min(
        DECODE_BUFFER_BYTES, (remaining + BITS_PER_BYTE - 1) / BITS_PER_BYTE);
    buffer_.resize(to_read);
    if (!stream_->read(buffer_.data(), static_cast<std::streamsize>(to_read))) {
      throw exceptions::file_operation_exception(
          "Failed to read bits from compressed stream");
    }
    const auto got = static_cast<std::size_t>(stream_->gcount());
    buffer_.resize(got);
  }
};

/**
 * Decode trie node. Layout avoids -Wpadded: child, codepoint, is_leaf, explicit
 * pad.
 */
struct DecodeNode {
  std::array<std::size_t, 2> child{INVALID_NODE, INVALID_NODE};
  char32_t codepoint = 0;
  uint8_t is_leaf = 0;
  std::array<uint8_t, 3> pad_{};  // explicit padding to alignment boundary
};

auto build_decode_trie(const std::map<char32_t, std::string>& prefixes)
    -> std::vector<DecodeNode> {
  std::vector<DecodeNode> nodes;
  nodes.emplace_back();

  auto ensure_child = [&nodes](std::size_t parent_idx, int bit) -> std::size_t {
    std::size_t& child_ref =
        nodes.at(parent_idx).child.at(static_cast<std::size_t>(bit));
    if (child_ref == INVALID_NODE) {
      const std::size_t new_idx = nodes.size();
      child_ref = new_idx;
      nodes.emplace_back();
      return new_idx;  // return saved index; child_ref may be invalid after
                       // realloc
    }
    return child_ref;
  };

  for (const auto& [codepoint, code] : prefixes) {
    if (code.empty()) {
      continue;
    }
    std::size_t idx = 0;
    for (const char code_char : code) {
      const int bit = (code_char == '1') ? 1 : 0;
      idx = ensure_child(idx, bit);
    }
    nodes.at(idx).is_leaf = 1;
    nodes.at(idx).codepoint = codepoint;
  }
  return nodes;
}

}  // namespace

auto read_header(std::istream& stream) -> std::map<char32_t, std::string> {
  std::istream* stream_ptr = &stream;
  const auto count = read_be<uint32_t>(stream_ptr);
  if (count == 0) {
    return {};
  }
  std::map<char32_t, std::string> prefixes;
  for (uint32_t i = 0; i < count; ++i) {
    const auto codepoint = read_be<uint32_t>(stream_ptr);
    const auto code_len = read_be<uint8_t>(stream_ptr);
    if (code_len == 0) {
      prefixes[static_cast<char32_t>(codepoint)] = "";
      continue;
    }
    std::string code_str =
        unpack_bits(stream_ptr, static_cast<std::size_t>(code_len));
    prefixes[static_cast<char32_t>(codepoint)] = code_str;
  }
  return prefixes;
}

void decode_and_write(std::istream& stream,
                      const std::map<char32_t, std::string>& prefixes,
                      output_file& file, read_progress_callback* progress) {
  std::istream* stream_ptr = &stream;
  const auto total_bits = read_be<uint64_t>(stream_ptr);
  if (total_bits == 0) {
    if (progress != nullptr) {
      (*progress)(0, 0);
    }
    return;
  }

  const std::vector<DecodeNode> trie = build_decode_trie(prefixes);
  const std::size_t root_idx = 0;
  BitReader reader(stream_ptr, static_cast<std::size_t>(total_bits));

  std::string out_buf;
  out_buf.reserve(OUTPUT_BUFFER_SIZE);
  constexpr std::size_t PROGRESS_INTERVAL_BITS = 65536;
  std::size_t last_reported = 0;

  std::size_t node_idx = root_idx;
  while (reader.has_next()) {
    const int bit = reader.next_bit() ? 1 : 0;
    const std::size_t next_idx =
        trie.at(node_idx).child.at(static_cast<std::size_t>(bit));
    if (next_idx == INVALID_NODE) {
      throw exceptions::file_operation_exception(
          "Invalid compressed stream: no code for bit sequence");
    }
    node_idx = next_idx;
    if (trie.at(node_idx).is_leaf != 0) {
      const char32_t codepoint = trie.at(node_idx).codepoint;
      std::string utf8 = codepoint_to_utf8(codepoint);
      out_buf.append(utf8);
      if (out_buf.size() >= OUTPUT_BUFFER_SIZE) {
        file.write(out_buf);
        out_buf.clear();
      }
      node_idx = root_idx;
    }
    if (progress != nullptr) {
      const std::size_t cur = reader.bits_read();
      const std::size_t total = reader.total_bits();
      if (cur - last_reported >= PROGRESS_INTERVAL_BITS || cur == total) {
        (*progress)(cur, total);
        last_reported = cur;
      }
    }
  }

  if (!out_buf.empty()) {
    file.write(out_buf);
  }
  if (progress != nullptr) {
    (*progress)(reader.bits_read(), reader.total_bits());
  }
}

}  // namespace file_handler
