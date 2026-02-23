#include "decompress.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "argument/argument.hpp"
#include "compress.hpp"
#include "exceptions/file_operation_exception.hpp"

namespace {

class DecompressTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() /
                ("cc_decompress_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(temp_dir_);
    saved_cwd_ = std::filesystem::current_path();
    std::filesystem::current_path(temp_dir_);
  }
  void TearDown() override {
    std::filesystem::current_path(saved_cwd_);
    std::filesystem::remove_all(temp_dir_);
  }

  std::filesystem::path temp_dir_;
  std::filesystem::path saved_cwd_;
};

TEST_F(DecompressTest, RoundTrip_CompressThenDecompress_MatchesOriginal) {
  const std::string original = "hello world\n";
  const auto input_path = (temp_dir_ / "original.txt").string();
  const auto compressed_path = (temp_dir_ / "out.compressed").string();
  const auto decompressed_path = (temp_dir_ / "out.txt").string();

  {
    std::ofstream f(input_path);
    ASSERT_TRUE(f) << input_path;
    f << original;
    ASSERT_TRUE(f);
  }

  argument::validated_args compress_args{argument::mode::compress, input_path,
                                         compressed_path};
  compress::run(compress_args);

  argument::validated_args decompress_args{argument::mode::decompress,
                                           compressed_path, decompressed_path};
  decompress::run(decompress_args);

  std::ifstream result(decompressed_path, std::ios::binary);
  std::string content((std::istreambuf_iterator<char>(result)),
                      std::istreambuf_iterator<char>());
  EXPECT_EQ(content, original);
}

}  // namespace
