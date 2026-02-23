#ifndef DECOMPRESS_HPP
#define DECOMPRESS_HPP

#include "argument/argument.hpp"
#include "exceptions/argument_exception.hpp"

namespace decompress {

/**
 * @brief Runs decompression (placeholder until implemented).
 * @param validated Validated arguments (operation must be decompress).
 * @throws exceptions::argument_exception Currently always; decompress not
 * implemented.
 */
[[noreturn]] inline void run(const argument::validated_args& validated) {
  (void)validated;
  throw exceptions::argument_exception("Decompress not yet implemented.");
}

}  // namespace decompress

#endif  // DECOMPRESS_HPP
