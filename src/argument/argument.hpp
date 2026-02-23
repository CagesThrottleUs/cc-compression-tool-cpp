#ifndef ARGUMENT_HPP
#define ARGUMENT_HPP

#include <cstddef>
#include <iterator>
#include <span>

namespace argument {

/**
 * @brief A lightweight, non-owning view over the command-line arguments.
 *
 * This class wraps the raw C-style `argv` array and `argc` count into a
 * C++ range-compatible interface. It allows utilizing C++20 ranges and
 * spans with command-line arguments without deep copying.
 */
class argv_view {
 public:
  /**
   * @brief Constructs a view from the raw argument vector and count.
   * @param data Pointer to the array of C-strings (argv).
   * @param n Number of arguments (argc).
   */
  explicit argv_view(char** data, std::size_t n) : data_(data), size_(n) {}

  /** @brief Returns an iterator to the first argument. */
  [[nodiscard]] auto begin() const -> char** { return data_; }

  /** @brief Returns an iterator one past the last argument. */
  [[nodiscard]] auto end() const -> char** {
    return std::next(data_, static_cast<std::ptrdiff_t>(size_));
  }

  /** @brief Returns the number of arguments. */
  [[nodiscard]] auto size() const -> std::size_t { return size_; }

  /** @brief Returns the underlying raw pointer. */
  [[nodiscard]] auto data() const -> char** { return data_; }

 private:
  char** data_;         ///< Pointer to the beginning of the argv array.
  std::size_t size_{};  ///< Number of elements in the array.
};

}  // namespace argument

// Standard permits specializing enable_borrowed_range for program-defined types
// ([namespace.std]). NOLINTNEXTLINE(cert-dcl58-cpp)
namespace std::ranges {

/**
 * @brief Opt-in to the borrowed_range concept for argv_view.
 *
 * This specialization informs the ranges library that iterators obtained from
 * an rvalue `argv_view` remain valid after the view is destroyed. This is
 * safe because `argv_view` does not own the underlying memory (the OS does).
 */
template <>
inline constexpr bool enable_borrowed_range<argument::argv_view> = true;
}  // namespace std::ranges

namespace argument {

/**
 * @brief Validates the command-line arguments.
 * @param args The command-line arguments.
 */
[[nodiscard]] auto validate_arguments(const argv_view& args)
    -> std::span<char* const>;

}  // namespace argument

#endif  // ARGUMENT_HPP
