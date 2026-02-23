#ifndef ARGUMENT_EXCEPTION_HPP
#define ARGUMENT_EXCEPTION_HPP

#include "base_exception.hpp"

namespace exceptions {

/**
 * @brief Exception thrown when invalid command-line arguments are provided.
 */
class argument_exception : public base_exception {
 public:
  using base_exception::base_exception;  // inherit constructor from
                                         // base_exception

  ~argument_exception() override;
  argument_exception(const argument_exception&) = default;
  argument_exception(argument_exception&&) = default;
  auto operator=(const argument_exception&) -> argument_exception& = default;
  auto operator=(argument_exception&&) -> argument_exception& = default;
};

}  // namespace exceptions

#endif  // ARGUMENT_EXCEPTION_HPP
