#ifndef EXIT_CODES_HPP
#define EXIT_CODES_HPP

namespace exit_codes {

enum class exit_code : int {  // NOLINT(performance-enum-size)
  success = 0,
  argument_error = 1,
  unknown_error = 2,
  file_operation_error = 3,
};

}  // namespace exit_codes

#endif  // EXIT_CODES_HPP
