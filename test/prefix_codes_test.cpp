#include "prefix_codes/prefix_codes.hpp"

#include <gtest/gtest.h>

#include <string>

#include "frequency_table/frequency_table.hpp"

namespace {

using namespace prefix_codes;
using namespace frequency_table;

// Returns true if no code is a prefix of another (prefix-free).
bool is_prefix_free(const std::map<char32_t, std::string>& codes) {
  for (auto it = codes.begin(); it != codes.end(); ++it) {
    const std::string& a = it->second;
    for (auto jt = codes.begin(); jt != codes.end(); ++jt) {
      if (it == jt) continue;
      const std::string& b = jt->second;
      if (a.size() <= b.size() && b.compare(0, a.size(), a) == 0) return false;
      if (b.size() <= a.size() && a.compare(0, b.size(), b) == 0) return false;
    }
  }
  return true;
}

class PrefixCodesTest : public ::testing::Test {};

// --- Empty and single symbol ---

TEST_F(PrefixCodesTest, GeneratePrefixCodes_EmptyTable_ReturnsEmptyMap) {
  table t;
  auto codes = generate_prefix_codes(t);
  EXPECT_TRUE(codes.empty());
}

TEST_F(PrefixCodesTest, GeneratePrefixCodes_SingleSymbol_ReturnsSingleBitZero) {
  table t = {{U'a', 1}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 1U);
  EXPECT_EQ(codes.at(U'a'), "0");
}

// --- Two symbols ---

TEST_F(PrefixCodesTest, GeneratePrefixCodes_TwoSymbols_ReturnsOneBitEach) {
  table t = {{U'a', 1}, {U'b', 1}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 2U);
  EXPECT_TRUE(codes.count(U'a'));
  EXPECT_TRUE(codes.count(U'b'));
  EXPECT_EQ(codes.at(U'a').size(), 1U);
  EXPECT_EQ(codes.at(U'b').size(), 1U);
  EXPECT_NE(codes.at(U'a'), codes.at(U'b'));
  EXPECT_TRUE(is_prefix_free(codes));
}

TEST_F(PrefixCodesTest, GeneratePrefixCodes_TwoSymbolsDifferentFreq_ShorterCodeForHigherFreq) {
  table t = {{U'a', 1}, {U'b', 3}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 2U);
  EXPECT_EQ(codes.at(U'b').size(), 1U);
  EXPECT_EQ(codes.at(U'a').size(), 1U);
  EXPECT_TRUE(is_prefix_free(codes));
}

// --- Three or more symbols ---

TEST_F(PrefixCodesTest, GeneratePrefixCodes_ThreeSymbols_AllPresentAndPrefixFree) {
  table t = {{U'a', 1}, {U'b', 1}, {U'c', 2}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 3U);
  EXPECT_TRUE(codes.count(U'a'));
  EXPECT_TRUE(codes.count(U'b'));
  EXPECT_TRUE(codes.count(U'c'));
  for (const auto& [cp, code] : codes) {
    for (char ch : code) {
      EXPECT_TRUE(ch == '0' || ch == '1') << "code for U+" << std::hex << static_cast<unsigned>(cp);
    }
  }
  EXPECT_TRUE(is_prefix_free(codes));
}

TEST_F(PrefixCodesTest, GeneratePrefixCodes_ThreeSymbols_HigherFreqGetsShorterCode) {
  table t = {{U'x', 1}, {U'y', 2}, {U'z', 4}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 3U);
  std::size_t len_x = codes.at(U'x').size();
  std::size_t len_y = codes.at(U'y').size();
  std::size_t len_z = codes.at(U'z').size();
  EXPECT_LE(len_z, len_y);
  EXPECT_LE(len_y, len_x);
  EXPECT_TRUE(is_prefix_free(codes));
}

TEST_F(PrefixCodesTest, GeneratePrefixCodes_EqualFrequencies_AllCodesSameLengthOrOneShorter) {
  table t = {{U'a', 1}, {U'b', 1}, {U'c', 1}, {U'd', 1}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 4U);
  std::size_t min_len = codes.begin()->second.size();
  std::size_t max_len = codes.begin()->second.size();
  for (const auto& [cp, code] : codes) {
    (void)cp;
    min_len = std::min(min_len, code.size());
    max_len = std::max(max_len, code.size());
  }
  EXPECT_LE(max_len - min_len, 1U) << "canonical codes for equal freqs differ by at most 1 bit";
  EXPECT_TRUE(is_prefix_free(codes));
}

// --- Codepoints beyond ASCII ---

TEST_F(PrefixCodesTest, GeneratePrefixCodes_UTF8Codepoints_KeysPreserved) {
  table t = {{U'a', 1}, {U'\u00E9', 2}, {U'\u4E2D', 1}};
  auto codes = generate_prefix_codes(t);
  ASSERT_EQ(codes.size(), 3U);
  EXPECT_TRUE(codes.count(U'a'));
  EXPECT_TRUE(codes.count(U'\u00E9'));
  EXPECT_TRUE(codes.count(U'\u4E2D'));
  EXPECT_TRUE(is_prefix_free(codes));
}

// --- Larger table ---

TEST_F(PrefixCodesTest, GeneratePrefixCodes_ManySymbols_AllPresentAndPrefixFree) {
  table t;
  for (int i = 0; i < 20; ++i) {
    t.push_back({static_cast<char32_t>(U'a' + i), i + 1});
  }
  auto codes = generate_prefix_codes(t);
  EXPECT_EQ(codes.size(), 20U);
  for (int i = 0; i < 20; ++i) {
    char32_t cp = static_cast<char32_t>(U'a' + i);
    EXPECT_TRUE(codes.count(cp)) << "missing codepoint U+" << std::hex << static_cast<unsigned>(cp);
  }
  EXPECT_TRUE(is_prefix_free(codes));
}

}  // namespace
