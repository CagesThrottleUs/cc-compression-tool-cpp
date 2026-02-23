#ifndef OUTPUT_FILE_HPP
#define OUTPUT_FILE_HPP

#include <fstream>
#include <map>
#include <string>

#include "../exceptions/file_operation_exception.hpp"
#include "input_file.hpp"

namespace file_handler {

class output_file {
 private:
  std::string path_;
  std::ofstream file_;

 public:
  /**
   * @brief Constructs an output_file object.
   * @param path The path to the output file.
   * @throws exceptions::file_operation_exception If the file cannot be opened.
   */
  explicit output_file(const std::string& path) : path_(path) {
    file_.open(path, std::ios::binary);
    if (!file_.is_open()) {
      throw exceptions::file_operation_exception("Failed to open file: " +
                                                 path);
    }
  }

  output_file(const output_file&) = delete;
  auto operator=(const output_file&) -> output_file& = delete;
  output_file(output_file&&) = delete;
  auto operator=(output_file&&) -> output_file& = delete;

  /**
   * @brief Writes data to the output file.
   * @param data The data to write.
   */
  auto write(const std::string& data) -> void {
    file_.write(data.data(), static_cast<std::streamsize>(data.size()));
  }

  /**
   * @brief Closes the output file.
   */
  ~output_file() {
    if (file_.is_open()) {
      file_.close();
    }
  }
};

/**
 * @brief Writes the header to the output file.
 * @param prefixes The prefixes.
 * @param file The output file.
 * @throws exceptions::file_operation_exception If the header cannot be written.
 */
auto write_header(const std::map<char32_t, std::string>& prefixes,
                  output_file& file) noexcept -> void;

auto write_file_contents(std::unique_ptr<file_handler::input_file> input,
                         const std::map<char32_t, std::string>& prefixes,
                         output_file& file) noexcept -> void;

}  // namespace file_handler

#endif  // OUTPUT_FILE_HPP
