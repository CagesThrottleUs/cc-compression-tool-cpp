#ifndef PROGRESS_HPP
#define PROGRESS_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>

namespace progress {

constexpr int kBarWidth = 40;

/**
 * @brief Prints a progress bar to stdout.
 * @param current Current progress count.
 * @param total Total count (0 is treated as complete).
 * @param label Label shown before the bar.
 */
inline void print_progress(std::size_t current, std::size_t total,
                           const char* label = "Writing") {
  if (total == 0) {
    std::cout << '\r' << label << " [========================================] "
              << "100%\n"
              << std::flush;
    return;
  }
  const int pct = static_cast<int>(
      std::min(100.0, std::round(100.0 * static_cast<double>(current) /
                                 static_cast<double>(total))));
  const int filled =
      static_cast<int>(std::round(static_cast<double>(pct) / 100.0 *
                                  static_cast<double>(kBarWidth)));
  std::cout << '\r' << label << " [";
  for (int i = 0; i < kBarWidth; ++i) {
    char segment = ' ';
    if (i < filled) {
      segment = '=';
    } else if (i == filled) {
      segment = '>';
    }
    std::cout << segment;
  }
  std::cout << "] " << pct << "%" << std::flush;
}

}  // namespace progress

#endif  // PROGRESS_HPP
