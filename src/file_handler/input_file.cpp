#include "input_file.hpp"

#include <utf8.h>

#include <boost/iostreams/device/mapped_file.hpp>
#include <span>
#include <vector>

#include "../exceptions/file_operation_exception.hpp"

namespace file_handler {

constexpr auto MIN_FILE_SIZE_FOR_BUFFER = 100ULL * 1024ULL * 1024ULL;

namespace detail {

// -- in_memory_input --
class in_memory_input : public input_file {
 public:
  explicit in_memory_input(const std::string& path) : file_name(path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
      throw exceptions::file_operation_exception("Cannot open file: " + path);
    }
    stream.seekg(0, std::ios::end);
    const auto stream_size = static_cast<std::size_t>(stream.tellg());
    if (stream_size > MIN_FILE_SIZE_FOR_BUFFER) {
      throw exceptions::file_operation_exception(
          "File too large for full load (max " +
          std::to_string(MIN_FILE_SIZE_FOR_BUFFER) + "): " + path);
    }
    stream.seekg(0);

    // Resize buffer to hold the file content.
    // std::vector guarantees contiguous memory storage.
    buffer.resize(stream_size);

    if (stream_size > 0U &&
        !stream.read(buffer.data(),
                     static_cast<std::streamsize>(stream_size))) {
      throw exceptions::file_operation_exception("Failed to read file: " +
                                                 path);
    }
  }

  ~in_memory_input() override;
  in_memory_input(const in_memory_input&) = delete;
  auto operator=(const in_memory_input&) -> in_memory_input& = delete;
  in_memory_input(in_memory_input&&) = delete;
  auto operator=(in_memory_input&&) -> in_memory_input& = delete;

  auto next_codepoint() -> std::optional<codepoint_type> override {
    if (pos >= buffer.size()) {
      return std::nullopt;
    }
    auto iter = buffer.begin() + static_cast<std::ptrdiff_t>(pos);
    const codepoint_type codepoint = utf8::next(iter, buffer.end());
    pos = static_cast<std::size_t>(std::distance(buffer.begin(), iter));
    return codepoint;
  }

  void reset() override { pos = 0; }

  [[nodiscard]] auto good() const -> bool override { return true; }

  [[nodiscard]] auto name() const -> std::string override { return file_name; }

  [[nodiscard]] auto size() const noexcept -> std::size_t override {
    return buffer.size();
  }

  [[nodiscard]] auto at_end() const noexcept -> bool override {
    return pos >= buffer.size();
  }

  [[nodiscard]] auto data() const noexcept -> const char* override {
    return buffer.data();
  }

  [[nodiscard]] auto current_offset() const noexcept -> std::size_t override {
    return pos;
  }

 private:
  std::string file_name;
  std::vector<char> buffer;
  std::size_t pos = 0;
};

in_memory_input::~in_memory_input() = default;

// -- mmap_input --

class mmap_input : public input_file {
 public:
  explicit mmap_input(const std::string& path) : file_name(path) {
    mmap_file_src.open(path);
    if (!mmap_file_src.is_open()) {
      throw exceptions::file_operation_exception("Failed to memory-map file: " +
                                                 path);
    }
    // pointer wrapper so never loaded into memory fullt
    content = std::span<const char>(
        std::string_view(mmap_file_src.data(), mmap_file_src.size()));
  }

  ~mmap_input() override;
  mmap_input(const mmap_input&) = delete;
  auto operator=(const mmap_input&) -> mmap_input& = delete;
  mmap_input(mmap_input&&) = delete;
  auto operator=(mmap_input&&) -> mmap_input& = delete;

  auto next_codepoint() -> std::optional<codepoint_type> override {
    if (pos >= content.size()) {
      return std::nullopt;
    }
    auto iter = content.begin() + static_cast<std::ptrdiff_t>(pos);
    const codepoint_type codepoint = utf8::next(iter, content.end());
    pos = static_cast<std::size_t>(std::distance(content.begin(), iter));
    return codepoint;
  }

  void reset() override { pos = 0; }

  [[nodiscard]] auto good() const -> bool override {
    return mmap_file_src.is_open();
  }

  [[nodiscard]] auto name() const -> std::string override { return file_name; }

  [[nodiscard]] auto size() const noexcept -> std::size_t override {
    return content.size();
  }

  [[nodiscard]] auto at_end() const noexcept -> bool override {
    return pos >= content.size();
  }

  [[nodiscard]] auto data() const noexcept -> const char* override {
    return content.data();
  }

  [[nodiscard]] auto current_offset() const noexcept -> std::size_t override {
    return pos;
  }

 private:
  std::string file_name;
  boost::iostreams::mapped_file_source mmap_file_src;
  std::span<const char> content;
  std::size_t pos{0};
};

mmap_input::~mmap_input() = default;

}  // namespace detail

namespace {

/**
 * @brief Get the file load type based on the file size.
 *
 * @param filename The name of the file.
 * @return The file load type.
 */
auto get_file_load_type(const std::string& filename)
    -> detail::file_loading_strategy {
  namespace fs = std::filesystem;
  std::error_code err_code;
  const fs::path file_path(filename);
  if (!fs::exists(file_path, err_code) || err_code) {
    throw exceptions::file_operation_exception("File does not exist: " +
                                               filename);
  }
  if (!fs::is_regular_file(file_path, err_code) || err_code) {
    throw exceptions::file_operation_exception("Not a regular file: " +
                                               filename);
  }
  const auto file_size = fs::file_size(file_path, err_code);
  if (err_code) {
    throw exceptions::file_operation_exception("Cannot get file size: " +
                                               filename);
  }
  return file_size < MIN_FILE_SIZE_FOR_BUFFER
             ? detail::file_loading_strategy::load_into_memory
             : detail::file_loading_strategy::load_into_buffer;
}

}  // namespace

auto load_file(const std::string& path) -> std::unique_ptr<input_file> {
  const auto load_type = get_file_load_type(path);
  if (load_type == detail::file_loading_strategy::load_into_memory) {
    return std::make_unique<detail::in_memory_input>(path);
  }
  return std::make_unique<detail::mmap_input>(path);
}

auto codepoint_to_utf8(input_file::codepoint_type codepoint) -> std::string {
  std::string out;
  utf8::append(static_cast<utf8::utfchar32_t>(codepoint),
               std::back_inserter(out));
  return out;
}

}  // namespace file_handler
