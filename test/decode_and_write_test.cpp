#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "file_handler/compressed_format.hpp"
#include "file_handler/output_file.hpp"

namespace {

/**
 * Writes uint64_t big-endian to a file_handler::output_file.
 */
void write_be64(file_handler::output_file& out, uint64_t value) {
  std::string buf(8, '\0');
  for (int i = 7; i >= 0; --i) {
    buf.at(7 - i) = static_cast<char>((value >> (i * 8)) & 0xFF);
  }
  out.write(buf);
}

class DecodeAndWriteTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() /
                ("cc_decode_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(temp_dir_);
  }
  void TearDown() override { std::filesystem::remove_all(temp_dir_); }

  std::filesystem::path temp_dir_;
};

TEST_F(DecodeAndWriteTest, SingleCodepoint_DecodesToUtf8) {
  std::map<char32_t, std::string> prefixes = {{97, "0"}};
  const auto compressed_path = (temp_dir_ / "compressed.bin").string();
  const auto output_path = (temp_dir_ / "out.txt").string();

  {
    file_handler::output_file out(compressed_path);
    file_handler::write_header(prefixes, out);
    write_be64(out, 1);               // total_bits = 1
    out.write(std::string(1, '\0'));  // one bit 0
  }

  std::ifstream stream(compressed_path, std::ios::binary);
  ASSERT_TRUE(stream.is_open());
  auto read_prefixes = file_handler::read_header(stream);
  {
    file_handler::output_file decoded_out(output_path);
    file_handler::decode_and_write(stream, read_prefixes, decoded_out, nullptr);
  }

  std::ifstream result(output_path, std::ios::binary);
  std::string content((std::istreambuf_iterator<char>(result)),
                      std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "a");
}

TEST_F(DecodeAndWriteTest, TwoCodepoints_DecodesCorrectly) {
  std::map<char32_t, std::string> prefixes = {{97, "0"}, {98, "1"}};
  const auto compressed_path = (temp_dir_ / "comp2.bin").string();
  const auto output_path = (temp_dir_ / "out2.txt").string();

  {
    file_handler::output_file out(compressed_path);
    file_handler::write_header(prefixes, out);
    write_be64(out, 2);                 // total_bits = 2
    out.write(std::string(1, '\x80'));  // bits "10" -> 'b'
  }

  std::ifstream stream(compressed_path, std::ios::binary);
  ASSERT_TRUE(stream.is_open());
  auto read_prefixes = file_handler::read_header(stream);
  {
    file_handler::output_file decoded_out(output_path);
    file_handler::decode_and_write(stream, read_prefixes, decoded_out, nullptr);
  }

  std::ifstream result(output_path, std::ios::binary);
  std::string content((std::istreambuf_iterator<char>(result)),
                      std::istreambuf_iterator<char>());
  EXPECT_EQ(content, "ba");  // bits "10" -> code "1"='b', then "0"='a'
}

}  // namespace
