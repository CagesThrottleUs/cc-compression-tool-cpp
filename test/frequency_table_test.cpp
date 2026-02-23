#include "frequency_table/frequency_table.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "exceptions/file_operation_exception.hpp"
#include "file_handler/input_file.hpp"

namespace {

class FrequencyTableTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ =
        std::filesystem::temp_directory_path() /
        ("cc_compression_frequency_table_test_" + std::to_string(::getpid()));
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

// --- node::operator< ---

TEST_F(FrequencyTableTest, Node_LessThan_ComparesByCount) {
  frequency_table::node a{U'a', 1};
  frequency_table::node b{U'b', 2};
  EXPECT_TRUE(a < b);
  EXPECT_FALSE(b < a);
}

TEST_F(FrequencyTableTest, Node_LessThan_EqualCounts_ReturnsFalse) {
  frequency_table::node a{U'a', 5};
  frequency_table::node b{U'b', 5};
  EXPECT_FALSE(a < b);
  EXPECT_FALSE(b < a);
}

// --- build_frequency_table: empty and ASCII ---

TEST_F(FrequencyTableTest, BuildFrequencyTable_EmptyFile_ReturnsEmptyTable) {
  const auto path = temp_dir_ / "empty.txt";
  write_file(path, "");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  EXPECT_TRUE(tbl.empty());
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_SingleASCII_OneEntry) {
  const auto path = temp_dir_ / "single.txt";
  write_file(path, "a");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 1U);
  EXPECT_EQ(tbl.at(0).codepoint, U'a');
  EXPECT_EQ(tbl.at(0).count, 1);
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_RepeatedASCII_CountCorrect) {
  const auto path = temp_dir_ / "repeated.txt";
  write_file(path, "aaa");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 1U);
  EXPECT_EQ(tbl.at(0).codepoint, U'a');
  EXPECT_EQ(tbl.at(0).count, 3);
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_MultipleASCII_EntriesAndCounts) {
  const auto path = temp_dir_ / "multi.txt";
  write_file(path, "aabbbc");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 3U);
  // Sorted by count ascending: a=2, b=3, c=1 -> c, a, b
  EXPECT_EQ(tbl.at(0).codepoint, U'c');
  EXPECT_EQ(tbl.at(0).count, 1);
  EXPECT_EQ(tbl.at(1).codepoint, U'a');
  EXPECT_EQ(tbl.at(1).count, 2);
  EXPECT_EQ(tbl.at(2).codepoint, U'b');
  EXPECT_EQ(tbl.at(2).count, 3);
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_ResultSortedAscendingByCount) {
  const auto path = temp_dir_ / "sort.txt";
  write_file(path, "xxyyyzzzz");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 3U);
  EXPECT_LE(tbl.at(0).count, tbl.at(1).count);
  EXPECT_LE(tbl.at(1).count, tbl.at(2).count);
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_WithProgressCallback_InvokesCallback) {
  const auto path = temp_dir_ / "progress.txt";
  write_file(path, "aabbcc");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);
  const std::size_t total_bytes = input->size();

  std::vector<std::pair<std::size_t, std::size_t>> progress_calls;
  frequency_table::build_progress_callback progress =
      [&progress_calls](std::size_t current, std::size_t total) {
        progress_calls.emplace_back(current, total);
      };

  auto tbl = frequency_table::build_frequency_table(std::move(input), &progress);
  ASSERT_EQ(tbl.size(), 3U);

  EXPECT_FALSE(progress_calls.empty()) << "Progress callback should be invoked";
  EXPECT_EQ(progress_calls.back().first, total_bytes);
  EXPECT_EQ(progress_calls.back().second, total_bytes);
  for (const auto& [current, total] : progress_calls) {
    EXPECT_LE(current, total);
  }
}

// --- build_frequency_table: UTF-8 (codepoints > 127) ---

TEST_F(FrequencyTableTest, BuildFrequencyTable_UTF8Latin1_CountsCorrect) {
  // é = U+00E9 (2 bytes in UTF-8)
  const std::string content = "\xc3\xa9\xc3\xa9\xc3\xa9";
  const auto path = temp_dir_ / "utf8_latin.txt";
  write_file(path, content);
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 1U);
  EXPECT_EQ(tbl.at(0).codepoint, U'\u00E9');
  EXPECT_EQ(tbl.at(0).count, 3);
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_UTF8CJK_CountsCorrect) {
  // 中 = U+4E2D (3 bytes in UTF-8)
  const std::string content = "\xe4" "\xb8" "\xad" "\xe4" "\xb8" "\xad";
  const auto path = temp_dir_ / "utf8_cjk.txt";
  write_file(path, content);
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 1U);
  EXPECT_EQ(tbl.at(0).codepoint, U'\u4E2D');
  EXPECT_EQ(tbl.at(0).count, 2);
}

TEST_F(FrequencyTableTest, BuildFrequencyTable_MixedASCIIAndUTF8_AllEntries) {
  // a, é, a, 中, a (UTF-8: é = C3 A9, 中 = E4 B8 AD; each \x must be in its own literal)
  const char* content = reinterpret_cast<const char*>(u8"aéa中a");
  const std::string content_str(content);
  const auto path = temp_dir_ / "mixed.txt";
  write_file(path, content_str);
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  auto tbl = frequency_table::build_frequency_table(std::move(input));
  ASSERT_EQ(tbl.size(), 3U);
  // Sorted by count: 中=1, é=1, a=3 -> so order of 1-count entries may vary
  int a_count = 0;
  int e9_count = 0;
  int cjk_count = 0;
  for (const auto& n : tbl) {
    if (n.codepoint == U'a')
      a_count = static_cast<int>(n.count.convert_to<long>());
    if (n.codepoint == U'\u00E9')
      e9_count = static_cast<int>(n.count.convert_to<long>());
    if (n.codepoint == U'\u4E2D')
      cjk_count = static_cast<int>(n.count.convert_to<long>());
  }
  EXPECT_EQ(a_count, 3);
  EXPECT_EQ(e9_count, 1);
  EXPECT_EQ(cjk_count, 1);
}

// --- build_frequency_table: invalid UTF-8 throws ---

TEST_F(FrequencyTableTest,
       BuildFrequencyTable_InvalidUTF8_ThrowsFileOperationException) {
  // Incomplete UTF-8: lead byte 0xE4 without continuation
  const std::string content = "a\xe4";
  const auto path = temp_dir_ / "invalid_utf8.txt";
  write_file(path, content);
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  EXPECT_THROW(
      { auto tbl = frequency_table::build_frequency_table(std::move(input)); },
      exceptions::file_operation_exception);
}

TEST_F(FrequencyTableTest,
       BuildFrequencyTable_InvalidUTF8_MessageContainsContext) {
  const std::string content = "ab\xe4";
  const auto path = temp_dir_ / "invalid_utf8_msg.txt";
  write_file(path, content);
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);

  try {
    auto tbl = frequency_table::build_frequency_table(std::move(input));
    FAIL() << "Expected file_operation_exception";
  } catch (const exceptions::file_operation_exception& e) {
    std::string msg(e.what());
    EXPECT_NE(msg.find("frequency table"), std::string::npos);
  }
}

// --- print_frequency_table: no crash ---

TEST_F(FrequencyTableTest, PrintFrequencyTable_EmptyTable_DoesNotCrash) {
  frequency_table::table empty;
  EXPECT_NO_THROW(frequency_table::print_frequency_table(empty));
}

TEST_F(FrequencyTableTest, PrintFrequencyTable_NonEmpty_DoesNotCrash) {
  const auto path = temp_dir_ / "print.txt";
  write_file(path, "hello");
  auto input = file_handler::load_file(path.string());
  ASSERT_NE(input, nullptr);
  auto tbl = frequency_table::build_frequency_table(std::move(input));
  EXPECT_NO_THROW(frequency_table::print_frequency_table(tbl));
}

// --- sample/test.txt (requires working directory = project source dir) ---

TEST_F(FrequencyTableTest,
       BuildFrequencyTable_SampleTestTxt_ExpectedCountsForTandX) {
  const std::filesystem::path sample_path = "sample/test.txt";
  if (!std::filesystem::exists(sample_path)) {
    GTEST_SKIP() << "sample/test.txt not found (run tests with working "
                    "directory = project source)";
  }
  auto input = file_handler::load_file(sample_path.string());
  ASSERT_NE(input, nullptr);
  auto tbl = frequency_table::build_frequency_table(std::move(input));

  auto count_for = [&tbl](char32_t cp) -> long {
    for (const auto& n : tbl) {
      if (n.codepoint == cp) return n.count.convert_to<long>();
    }
    return -1;
  };
  EXPECT_EQ(count_for(U't'), 223000) << "frequency of 't' in sample/test.txt";
  EXPECT_EQ(count_for(U'X'), 333) << "frequency of 'X' in sample/test.txt";
}

}  // namespace
