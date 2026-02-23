#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "exceptions/file_operation_exception.hpp"
#include "file_handler/input_file.hpp"

namespace {

class FileHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ =
        std::filesystem::temp_directory_path() /
        ("cc_compression_file_handler_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override { std::filesystem::remove_all(temp_dir_); }

  void write_file(const std::filesystem::path& p, const std::string& content) {
    std::ofstream f(p, std::ios::binary);
    ASSERT_TRUE(f) << "Failed to create " << p;
    f.write(content.data(), static_cast<std::streamsize>(content.size()));
    ASSERT_TRUE(f);
  }

  std::filesystem::path temp_dir_;
};

// --- load_file error cases ---

TEST_F(FileHandlerTest, LoadFile_NonexistentPath_ThrowsFileOperationException) {
  const auto path = (temp_dir_ / "nonexistent.txt").string();
  EXPECT_THROW(
      { file_handler::load_file(path); }, exceptions::file_operation_exception);
}

TEST_F(FileHandlerTest,
       LoadFile_NonexistentPath_MessageContainsFileDoesNotExist) {
  const auto path = (temp_dir_ / "nonexistent.txt").string();
  try {
    file_handler::load_file(path);
    FAIL() << "Expected file_operation_exception";
  } catch (const exceptions::file_operation_exception& e) {
    EXPECT_NE(std::string(e.what()).find("File does not exist"),
              std::string::npos);
  }
}

TEST_F(FileHandlerTest, LoadFile_DirectoryPath_ThrowsFileOperationException) {
  const auto path = temp_dir_.string();
  EXPECT_THROW(
      { file_handler::load_file(path); }, exceptions::file_operation_exception);
}

TEST_F(FileHandlerTest, LoadFile_DirectoryPath_MessageContainsNotARegularFile) {
  const auto path = temp_dir_.string();
  try {
    file_handler::load_file(path);
    FAIL() << "Expected file_operation_exception";
  } catch (const exceptions::file_operation_exception& e) {
    EXPECT_NE(std::string(e.what()).find("Not a regular file"),
              std::string::npos);
  }
}

// --- load_file success (small file -> in-memory) ---

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_ReturnsNonNull) {
  const auto path = temp_dir_ / "small.txt";
  write_file(path, "hello");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
}

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_NameReturnsPath) {
  const auto path = temp_dir_ / "small.txt";
  write_file(path, "hello");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->name(), path.string());
}

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_SizeMatchesContent) {
  const std::string content = "hello";
  const auto path = temp_dir_ / "small.txt";
  write_file(path, content);
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->size(), content.size());
}

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_GoodReturnsTrue) {
  const auto path = temp_dir_ / "small.txt";
  write_file(path, "x");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  EXPECT_TRUE(f->good());
}

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_DataReturnsContent) {
  const std::string content = "hello";
  const auto path = temp_dir_ / "small.txt";
  write_file(path, content);
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  ASSERT_NE(f->data(), nullptr);
  EXPECT_EQ(std::string_view(f->data(), f->size()), content);
}

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_AtEndFalseInitially) {
  const auto path = temp_dir_ / "small.txt";
  write_file(path, "ab");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  EXPECT_FALSE(f->at_end());
}

TEST_F(FileHandlerTest, LoadFile_ValidSmallFile_CurrentOffsetStartsAtZero) {
  const auto path = temp_dir_ / "small.txt";
  write_file(path, "ab");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->current_offset(), 0U);
}

// --- next_codepoint and reading ---

TEST_F(FileHandlerTest, NextCodepoint_ASCII_ReturnsCorrectCodepoints) {
  const auto path = temp_dir_ / "ascii.txt";
  write_file(path, "ABC");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);

  auto c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'A');
  c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'B');
  c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'C');
  c = f->next_codepoint();
  EXPECT_FALSE(c.has_value());
  EXPECT_TRUE(f->at_end());
}

TEST_F(FileHandlerTest, NextCodepoint_UTF8MultiByte_ReturnsCorrectCodepoints) {
  // UTF-8: é = U+00E9 (2 bytes), 中 = U+4E2D (3 bytes)
  const std::string content = "\xc3\xa9\xe4\xb8\xad";
  const auto path = temp_dir_ / "utf8.txt";
  write_file(path, content);
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);

  auto c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'\u00E9');
  c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'\u4E2D');
  c = f->next_codepoint();
  EXPECT_FALSE(c.has_value());
  EXPECT_TRUE(f->at_end());
}

TEST_F(FileHandlerTest, NextCodepoint_CurrentOffsetAdvances) {
  const auto path = temp_dir_ / "offset.txt";
  write_file(path, "XY");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);

  EXPECT_EQ(f->current_offset(), 0U);
  f->next_codepoint();
  EXPECT_EQ(f->current_offset(), 1U);
  f->next_codepoint();
  EXPECT_EQ(f->current_offset(), 2U);
  f->next_codepoint();
  EXPECT_EQ(f->current_offset(), 2U);
}

TEST_F(FileHandlerTest, Reset_AllowsRereading) {
  const auto path = temp_dir_ / "reset.txt";
  write_file(path, "ab");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);

  auto c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'a');
  f->reset();
  EXPECT_FALSE(f->at_end());
  EXPECT_EQ(f->current_offset(), 0U);
  c = f->next_codepoint();
  ASSERT_TRUE(c.has_value());
  EXPECT_EQ(*c, U'a');
}

TEST_F(FileHandlerTest, EmptyFile_AtEndImmediately) {
  const auto path = temp_dir_ / "empty.txt";
  write_file(path, "");
  auto f = file_handler::load_file(path.string());
  ASSERT_NE(f, nullptr);
  EXPECT_EQ(f->size(), 0U);
  EXPECT_TRUE(f->at_end());
  auto c = f->next_codepoint();
  EXPECT_FALSE(c.has_value());
}

// --- codepoint_to_utf8 ---

TEST_F(FileHandlerTest, CodepointToUtf8_ASCII) {
  EXPECT_EQ(file_handler::codepoint_to_utf8(U'A'), "A");
  EXPECT_EQ(file_handler::codepoint_to_utf8(U'z'), "z");
  EXPECT_EQ(file_handler::codepoint_to_utf8(U'0'), "0");
}

TEST_F(FileHandlerTest, CodepointToUtf8_Latin1) {
  EXPECT_EQ(file_handler::codepoint_to_utf8(U'\u00E9'), "\xc3\xa9");  // é
}

TEST_F(FileHandlerTest, CodepointToUtf8_CJK) {
  EXPECT_EQ(file_handler::codepoint_to_utf8(U'\u4E2D'),
            "\xe4\xb8\xad");  // 中
}

}  // namespace
