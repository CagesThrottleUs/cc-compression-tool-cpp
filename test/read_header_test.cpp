#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>

#include "exceptions/file_operation_exception.hpp"
#include "file_handler/compressed_format.hpp"
#include "file_handler/output_file.hpp"

namespace {

/**
 * Writes a minimal compressed header to stream: count=1, codepoint 'a' (97),
 * code_len=1, one bit 0. Big-endian.
 */
void write_minimal_header(std::ostream& out) {
  const uint32_t count = 1;
  out.put(static_cast<char>((count >> 24) & 0xFF));
  out.put(static_cast<char>((count >> 16) & 0xFF));
  out.put(static_cast<char>((count >> 8) & 0xFF));
  out.put(static_cast<char>(count & 0xFF));
  const uint32_t codepoint = 97;
  out.put(static_cast<char>((codepoint >> 24) & 0xFF));
  out.put(static_cast<char>((codepoint >> 16) & 0xFF));
  out.put(static_cast<char>((codepoint >> 8) & 0xFF));
  out.put(static_cast<char>(codepoint & 0xFF));
  const uint8_t code_len = 1;
  out.put(static_cast<char>(code_len));
  out.put(static_cast<char>(0));  // 1 bit 0 packed in one byte
}

TEST(ReadHeaderTest, ReadsMinimalHeader) {
  std::stringstream stream;
  write_minimal_header(stream);
  stream.seekg(0);

  auto prefixes = file_handler::read_header(stream);
  ASSERT_EQ(prefixes.size(), 1U);
  ASSERT_EQ(prefixes.count(97), 1U);
  EXPECT_EQ(prefixes.at(97), "0");
}

TEST(ReadHeaderTest, EmptyStream_Throws) {
  std::stringstream stream;
  stream.str("");
  EXPECT_THROW(auto prefixes = file_handler::read_header(stream),
               exceptions::file_operation_exception);
}

TEST(ReadHeaderTest, TwoEntries_RoundTrip) {
  std::map<char32_t, std::string> expected = {{97, "0"}, {98, "10"}};
  const auto path = (std::filesystem::temp_directory_path() /
                     ("read_header_two_" + std::to_string(::getpid()) + ".bin"))
                        .string();
  {
    file_handler::output_file out(path);
    file_handler::write_header(expected, out);
  }
  std::ifstream stream(path, std::ios::binary);
  ASSERT_TRUE(stream.is_open());
  auto prefixes = file_handler::read_header(stream);
  std::filesystem::remove(path);
  EXPECT_EQ(prefixes.size(), expected.size());
  for (const auto& [cp, code] : expected) {
    EXPECT_EQ(prefixes.at(cp), code) << "codepoint " << static_cast<std::uint32_t>(cp);
  }
}

}  // namespace
