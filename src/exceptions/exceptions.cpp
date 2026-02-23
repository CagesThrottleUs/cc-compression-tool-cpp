#include "argument_exception.hpp"
#include "base_exception.hpp"
#include "file_operation_exception.hpp"

namespace exceptions {

base_exception::~base_exception() = default;
argument_exception::~argument_exception() = default;
file_operation_exception::~file_operation_exception() = default;
}  // namespace exceptions
