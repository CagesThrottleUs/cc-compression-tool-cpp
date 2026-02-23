#ifndef INPUT_FILE_HPP
#define INPUT_FILE_HPP

#include <fstream>
#include <optional>
#include <string>

namespace file_handler {

/**
 * @brief A class representing an input file.
 *
 * This class is responsible for reading the input file and providing a view
 * of the file's contents.
 */
class input_file {
 public:
  using codepoint_type = char32_t;

  virtual ~input_file() = default;
  input_file(const input_file&) = delete;
  auto operator=(const input_file&) -> input_file& = delete;
  input_file(input_file&&) = delete;
  auto operator=(input_file&&) -> input_file& = delete;

 protected:
  input_file() = default;

 public:
  /**
   * Reads the next codepoint from the input file.
   *
   * @return The next codepoint, or std::nullopt if the end of the file is
   * reached.
   */
  virtual auto next_codepoint() -> std::optional<codepoint_type> = 0;

  /**
   * Resets the input file to the beginning of the file.
   */
  virtual void reset() = 0;

  /**
   * Returns true if the file is in a good state.
   */
  [[nodiscard]] virtual auto good() const -> bool = 0;

  /**
   * Returns the name of the file.
   */
  [[nodiscard]] virtual auto name() const -> std::string = 0;

  /**
   * Returns the size of the file.
   */
  [[nodiscard]] virtual auto size() const noexcept -> std::size_t = 0;

  /**
   * Returns true if the file pointer is at the end of the file.
   */
  [[nodiscard]] virtual auto at_end() const noexcept -> bool = 0;

  /**
   * Returns a pointer to the start of the underlying data buffer.
   * This is necessary for zero-copy tokenization (creating string_views).
   */
  [[nodiscard]] virtual auto data() const noexcept -> const char* = 0;

  /**
   * Returns the current byte offset in the file.
   */
  [[nodiscard]] virtual auto current_offset() const noexcept -> std::size_t = 0;

 private:
  std::string path_;
  mutable std::ifstream file_;
};

/**
 * Loads a file into an input_file object.
 *
 * @param path The path to the file to load.
 * @return A unique pointer to the input_file object.
 * @throws exceptions::file_operation_exception If the file cannot be loaded.
 */
auto load_file(const std::string& path) -> std::unique_ptr<input_file>;

namespace detail {

enum class file_loading_strategy : int {  // NOLINT(performance-enum-size)
  load_into_memory,
  load_into_buffer,
};

}  // namespace detail

/**
 * Converts a codepoint to a UTF-8 string.
 *
 * @param codepoint The codepoint to convert.
 * @return The UTF-8 string.
 */
auto codepoint_to_utf8(input_file::codepoint_type codepoint) -> std::string;

}  // namespace file_handler

#endif  // INPUT_FILE_HPP
