#include "base_exception.hpp"

namespace exceptions {

class file_operation_exception : public base_exception {
 public:
  using base_exception::base_exception;

  ~file_operation_exception() override;
  file_operation_exception(const file_operation_exception&) = default;
  file_operation_exception(file_operation_exception&&) = default;
  auto operator=(const file_operation_exception&)
      -> file_operation_exception& = default;
  auto operator=(file_operation_exception&&)
      -> file_operation_exception& = default;
};

}  // namespace exceptions
