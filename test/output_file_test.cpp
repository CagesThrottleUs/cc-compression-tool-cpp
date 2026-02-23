#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "exceptions/file_operation_exception.hpp"
#include "file_handler/input_file.hpp"
#include "file_handler/output_file.hpp"

namespace {

class OutputFileTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ =
        std::filesystem::temp_directory_path() /
        ("cc_compression_output_file_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(temp_dir_); }

  void write_file(const std::filesystem::path& p, const std::string& content) {
    std::ofstream f(p, std::ios::binary);
    ASSERT_TRUE(f) << "Failed to create " << p;
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    ASSERT_TRUE(f);
  }

  std::string read_file_binary(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) return {};
    return std::string(std::istreambuf_iterator<char>(f),
                      std::istreambuf_iterator<char>());
  }

  std::filesystem::path temp_dir_;
};

// --- output_file constructor ---

TEST_F(OutputFileTest, Constructor_ValidPath_OpensFile) {
  const auto path = (temp_dir_ / "out.bin").string();
  EXPECT_NO_THROW({
    file_handler::output_file out(path);
    out.write("x");
  });
  EXPECT_EQ(read_file_binary(path), "x");
}

TEST_F(OutputFileTest, Constructor_DirectoryPath_ThrowsFileOperationException) {
  const auto path = temp_dir_.string();
  EXPECT_THROW({ file_handler::output_file out(path); },
               exceptions::file_operation_exception);
}

TEST_F(OutputFileTest,
       Constructor_InvalidPath_MessageContainsFailedToOpenFile) {
  const auto path = temp_dir_.string();
  try {
    file_handler::output_file out(path);
    FAIL() << "Expected file_operation_exception";
  } catch (const exceptions::file_operation_exception& e) {
    EXPECT_NE(std::string(e.what()).find("Failed to open file"),
              std::string::npos);
  }
}

// --- output_file::write ---

TEST_F(OutputFileTest, Write_EmptyString_WritesNothing) {
  const auto path = (temp_dir_ / "empty.bin").string();
  {
    file_handler::output_file out(path);
    out.write("");
  }
  EXPECT_TRUE(read_file_binary(path).empty());
}

TEST_F(OutputFileTest, Write_NonEmptyString_ContentMatches) {
  const auto path = (temp_dir_ / "content.bin").string();
  const std::string content = "hello binary";
  {
    file_handler::output_file out(path);
    out.write(content);
  }
  EXPECT_EQ(read_file_binary(path), content);
}

TEST_F(OutputFileTest, Write_MultipleCalls_AppendsInOrder) {
  const auto path = (temp_dir_ / "multi.bin").string();
  {
    file_handler::output_file out(path);
    out.write("ab");
    out.write("cd");
  }
  EXPECT_EQ(read_file_binary(path), "abcd");
}

// --- write_header ---

TEST_F(OutputFileTest, WriteHeader_EmptyMap_WritesCountZero) {
  const auto path = (temp_dir_ / "header_empty.bin").string();
  const std::map<char32_t, std::string> prefixes;
  {
    file_handler::output_file out(path);
    file_handler::write_header(prefixes, out);
  }
  const std::string data = read_file_binary(path);
  ASSERT_EQ(data.size(), 4U);
  // Big-endian uint32_t 0
  EXPECT_EQ(static_cast<unsigned char>(data[0]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[1]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[2]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[3]), 0x00);
}

TEST_F(OutputFileTest, WriteHeader_OneEntry_WritesCountCodepointLenAndPackedBits) {
  const auto path = (temp_dir_ / "header_one.bin").string();
  std::map<char32_t, std::string> prefixes;
  prefixes[U'A'] = "1";  // 1 bit set -> one byte 0x80 (MSB)
  {
    file_handler::output_file out(path);
    file_handler::write_header(prefixes, out);
  }
  const std::string data = read_file_binary(path);
  // count=1 (4 bytes BE), codepoint=65 (4 bytes BE), code_len=1 (1 byte), 1 packed byte
  ASSERT_GE(data.size(), 10U);
  EXPECT_EQ(static_cast<unsigned char>(data[0]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[1]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[2]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[3]), 0x01);
  EXPECT_EQ(static_cast<unsigned char>(data[4]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[5]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[6]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[7]), 0x41);  // 'A' = 65
  EXPECT_EQ(static_cast<unsigned char>(data[8]), 0x01);  // code_len = 1
  EXPECT_EQ(static_cast<unsigned char>(data[9]), 0x80);   // bit "1" MSB
}

TEST_F(OutputFileTest, WriteHeader_TwoEntries_WritesBoth) {
  const auto path = (temp_dir_ / "header_two.bin").string();
  std::map<char32_t, std::string> prefixes;
  prefixes[U'A'] = "0";   // 1 bit unset -> byte 0x00
  prefixes[U'B'] = "11";  // 2 bits set -> one byte 0xC0 (11000000)
  {
    file_handler::output_file out(path);
    file_handler::write_header(prefixes, out);
  }
  const std::string data = read_file_binary(path);
  // count=2 (4 bytes), then A: codepoint 65, len 1, 1 byte; B: codepoint 66, len 2, 1 byte
  ASSERT_GE(data.size(), 16U);
  EXPECT_EQ(static_cast<unsigned char>(data[3]), 0x02);  // count
  EXPECT_EQ(static_cast<unsigned char>(data[7]), 0x41);   // 'A'
  EXPECT_EQ(static_cast<unsigned char>(data[8]), 0x01);
  EXPECT_EQ(static_cast<unsigned char>(data[9]), 0x00);   // "0" -> 0
  EXPECT_EQ(static_cast<unsigned char>(data[13]), 0x42);   // 'B'
  EXPECT_EQ(static_cast<unsigned char>(data[14]), 0x02);  // len 2
  EXPECT_EQ(static_cast<unsigned char>(data[15]), 0xC0);  // "11" -> 0xC0
}

// --- write_file_contents ---

TEST_F(OutputFileTest, WriteFileContents_NullInput_DoesNotWriteTotalBits) {
  const auto path = (temp_dir_ / "contents_null.bin").string();
  std::map<char32_t, std::string> prefixes;
  prefixes[U'A'] = "1";
  {
    file_handler::output_file out(path);
    file_handler::write_header(prefixes, out);
    file_handler::write_file_contents(nullptr, prefixes, out);
  }
  // Only header: count(4) + 1 entry (4+1+1) = 10 bytes
  EXPECT_EQ(read_file_binary(path).size(), 10U);
}

TEST_F(OutputFileTest, WriteFileContents_EmptyInput_WritesZeroTotalBits) {
  const auto in_path = temp_dir_ / "empty_in.txt";
  write_file(in_path, "");
  auto input = file_handler::load_file(in_path.string());
  ASSERT_NE(input, nullptr);

  const auto out_path = (temp_dir_ / "contents_empty.bin").string();
  std::map<char32_t, std::string> prefixes;
  prefixes[U'A'] = "1";
  {
    file_handler::output_file out(out_path);
    file_handler::write_header(prefixes, out);
    file_handler::write_file_contents(std::move(input), prefixes, out);
  }
  const std::string data = read_file_binary(out_path);
  // Header (10 bytes) + total_bits uint64_t (8 bytes) = 18, total_bits = 0
  ASSERT_GE(data.size(), 18U);
  EXPECT_EQ(static_cast<unsigned char>(data[10]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[11]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[12]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[13]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[14]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[15]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[16]), 0x00);
  EXPECT_EQ(static_cast<unsigned char>(data[17]), 0x00);
}

TEST_F(OutputFileTest, WriteFileContents_OneCodepointInPrefixes_WritesPackedBits) {
  const auto in_path = temp_dir_ / "one_char.txt";
  write_file(in_path, "A");
  auto input = file_handler::load_file(in_path.string());
  ASSERT_NE(input, nullptr);

  const auto out_path = (temp_dir_ / "contents_one.bin").string();
  std::map<char32_t, std::string> prefixes;
  prefixes[U'A'] = "1";  // A -> 1 bit
  {
    file_handler::output_file out(out_path);
    file_handler::write_header(prefixes, out);
    file_handler::write_file_contents(std::move(input), prefixes, out);
  }
  const std::string data = read_file_binary(out_path);
  // Header 10 bytes, then total_bits=1 (8 bytes BE), then 1 packed byte 0x80
  ASSERT_GE(data.size(), 19U);
  EXPECT_EQ(static_cast<unsigned char>(data[17]), 0x01);  // total_bits low byte
  EXPECT_EQ(static_cast<unsigned char>(data[18]), 0x80);  // one bit set
}

TEST_F(OutputFileTest, WriteFileContents_CodepointNotInPrefixes_SkipsCodepoint) {
  const auto in_path = temp_dir_ / "mixed.txt";
  write_file(in_path, "AB");  // B not in prefixes
  auto input = file_handler::load_file(in_path.string());
  ASSERT_NE(input, nullptr);

  std::map<char32_t, std::string> prefixes;
  prefixes[U'A'] = "1";
  const auto out_path = (temp_dir_ / "contents_skip.bin").string();
  {
    file_handler::output_file out(out_path);
    file_handler::write_header(prefixes, out);
    file_handler::write_file_contents(std::move(input), prefixes, out);
  }
  const std::string data = read_file_binary(out_path);
  // Only A is encoded: total_bits=1, one byte 0x80
  ASSERT_GE(data.size(), 19U);
  EXPECT_EQ(static_cast<unsigned char>(data[17]), 0x01);
  EXPECT_EQ(static_cast<unsigned char>(data[18]), 0x80);
}

TEST_F(OutputFileTest, WriteFileContents_UTF8MultiByte_EncodesCodepoints) {
  // é = U+00E9, 中 = U+4E2D
  const std::string content = "\xc3\xa9\xe4\xb8\xad";
  const auto in_path = temp_dir_ / "utf8_in.txt";
  write_file(in_path, content);
  auto input = file_handler::load_file(in_path.string());
  ASSERT_NE(input, nullptr);

  std::map<char32_t, std::string> prefixes;
  prefixes[U'\u00E9'] = "0";   // é -> 1 bit 0
  prefixes[U'\u4E2D'] = "1";   // 中 -> 1 bit 1
  const auto out_path = (temp_dir_ / "contents_utf8.bin").string();
  {
    file_handler::output_file out(out_path);
    file_handler::write_header(prefixes, out);
    file_handler::write_file_contents(std::move(input), prefixes, out);
  }
  const std::string data = read_file_binary(out_path);
  // Header: count(4) + 2 entries × (codepoint(4)+len(1)+packed(1)) = 4+12 = 16 bytes
  // total_bits uint64_t = 8 bytes (BE), then 1 packed byte. total_bits = 2, packed = 0x40
  ASSERT_GE(data.size(), 25U);
  EXPECT_EQ(static_cast<unsigned char>(data[23]), 0x02);  // total_bits low byte = 2
  EXPECT_EQ(static_cast<unsigned char>(data[24]), 0x40);  // bits "01" -> 01000000
}

}  // namespace
