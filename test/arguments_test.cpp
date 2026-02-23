#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "argument/argument.hpp"
#include "exceptions/argument_exception.hpp"

namespace {

class ArgumentsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() /
                ("cc_compression_args_test_" + std::to_string(::getpid()));
    std::filesystem::create_directories(temp_dir_);
    existing_input_ = temp_dir_ / "existing_input.txt";
    existing_output_ = temp_dir_ / "existing_output.txt";
    std::ofstream(existing_input_).put('x');
    std::ofstream(existing_output_).put('x');
    // Use temp dir as cwd so default output path "output.compressed" does not
    // conflict with an existing file in project/build directory.
    saved_cwd_ = std::filesystem::current_path();
    std::filesystem::current_path(temp_dir_);
  }

  void TearDown() override {
    std::filesystem::current_path(saved_cwd_);
    std::filesystem::remove_all(temp_dir_);
  }

  std::filesystem::path temp_dir_;
  std::filesystem::path existing_input_;
  std::filesystem::path existing_output_;
  std::filesystem::path saved_cwd_;
};

TEST_F(ArgumentsTest, TooFewArguments_ThrowsUsage) {
  char* argv[] = {const_cast<char*>("prog")};
  argument::argv_view view(argv, 1);
  EXPECT_THROW(
      { auto args = argument::validate_arguments(view); },
      exceptions::argument_exception);
}

TEST_F(ArgumentsTest, TooManyArguments_ThrowsUsage) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = (temp_dir_ / "out.txt").string();
  std::string extra = "extra";
  char* argv[] = {prog.data(), in.data(), out.data(), extra.data()};
  argument::argv_view view(argv, 4);
  EXPECT_THROW(
      { auto args = argument::validate_arguments(view); },
      exceptions::argument_exception);
}

TEST_F(ArgumentsTest, TwoArgs_InputFileMissing_Throws) {
  std::string prog = "prog";
  std::string missing = (temp_dir_ / "nonexistent.txt").string();
  char* argv[] = {prog.data(), missing.data()};
  argument::argv_view view(argv, 2);
  EXPECT_THROW(auto args = argument::validate_arguments(view),
               exceptions::argument_exception);
}

TEST_F(ArgumentsTest, TwoArgs_InputFileMissing_MessageContainsInputFile) {
  std::string prog = "prog";
  std::string missing = (temp_dir_ / "nonexistent.txt").string();
  char* argv[] = {prog.data(), missing.data()};
  argument::argv_view view(argv, 2);
  try {
    auto args = argument::validate_arguments(view);
    FAIL() << "Expected argument_exception";
  } catch (const exceptions::argument_exception& e) {
    EXPECT_NE(std::string(e.what()).find("Input file does not exist"),
              std::string::npos);
  }
}

TEST_F(ArgumentsTest, TwoArgs_InputFileExists_DoesNotThrow) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  char* argv[] = {prog.data(), in.data()};
  argument::argv_view view(argv, 2);
  EXPECT_NO_THROW(auto args = argument::validate_arguments(view));
}

TEST_F(ArgumentsTest, ThreeArgs_OutputFileExists_Throws) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = existing_output_.string();
  char* argv[] = {prog.data(), in.data(), out.data()};
  argument::argv_view view(argv, 3);
  EXPECT_THROW(auto args = argument::validate_arguments(view),
               exceptions::argument_exception);
}

TEST_F(ArgumentsTest, ThreeArgs_OutputFileExists_MessageContainsOutputFile) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = existing_output_.string();
  char* argv[] = {prog.data(), in.data(), out.data()};
  argument::argv_view view(argv, 3);
  try {
    auto args = argument::validate_arguments(view);
    FAIL() << "Expected argument_exception";
  } catch (const exceptions::argument_exception& e) {
    EXPECT_NE(std::string(e.what()).find("Output file already exists"),
              std::string::npos);
  }
}

TEST_F(ArgumentsTest, ThreeArgs_OutputFileMissing_DoesNotThrow) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = (temp_dir_ / "nonexistent_output.txt").string();
  char* argv[] = {prog.data(), in.data(), out.data()};
  argument::argv_view view(argv, 3);
  EXPECT_NO_THROW(auto args = argument::validate_arguments(view));
}

TEST_F(ArgumentsTest, UsageMessage_ContainsProgramName) {
  char* argv[] = {const_cast<char*>("my_tool")};
  argument::argv_view view(argv, 1);
  try {
    auto args = argument::validate_arguments(view);
    FAIL() << "Expected argument_exception";
  } catch (const exceptions::argument_exception& e) {
    std::string msg(e.what());
    EXPECT_NE(msg.find("my_tool"), std::string::npos);
    EXPECT_NE(msg.find("input_file"), std::string::npos);
  }
}

TEST_F(ArgumentsTest, TwoArgs_DefaultIsCompress) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  char* argv[] = {prog.data(), in.data()};
  argument::argv_view view(argv, 2);
  auto args = argument::validate_arguments(view);
  EXPECT_EQ(args.operation, argument::mode::compress);
  EXPECT_EQ(args.input_path, in);
  EXPECT_EQ(args.output_path, "output.compressed");
}

TEST_F(ArgumentsTest, ExplicitCompress_TwoArgs) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  char* argv[] = {const_cast<char*>("prog"), const_cast<char*>("--compress"),
                  in.data()};
  argument::argv_view view(argv, 3);
  auto args = argument::validate_arguments(view);
  EXPECT_EQ(args.operation, argument::mode::compress);
  EXPECT_EQ(args.input_path, in);
  EXPECT_EQ(args.output_path, "output.compressed");
}

TEST_F(ArgumentsTest, ExplicitCompress_ThreeArgs) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = (temp_dir_ / "out_compress.txt").string();
  char* argv[] = {prog.data(), const_cast<char*>("--compress"), in.data(),
                  out.data()};
  argument::argv_view view(argv, 4);
  auto args = argument::validate_arguments(view);
  EXPECT_EQ(args.operation, argument::mode::compress);
  EXPECT_EQ(args.input_path, in);
  EXPECT_EQ(args.output_path, out);
}

TEST_F(ArgumentsTest, ExplicitDecompress_TwoArgs) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  char* argv[] = {prog.data(), const_cast<char*>("--decompress"), in.data()};
  argument::argv_view view(argv, 3);
  auto args = argument::validate_arguments(view);
  EXPECT_EQ(args.operation, argument::mode::decompress);
  EXPECT_EQ(args.input_path, in);
  EXPECT_EQ(args.output_path, "output.decompressed");
}

TEST_F(ArgumentsTest, ExplicitDecompress_ThreeArgs) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = (temp_dir_ / "out_decompress.txt").string();
  char* argv[] = {prog.data(), const_cast<char*>("--decompress"), in.data(),
                  out.data()};
  argument::argv_view view(argv, 4);
  auto args = argument::validate_arguments(view);
  EXPECT_EQ(args.operation, argument::mode::decompress);
  EXPECT_EQ(args.input_path, in);
  EXPECT_EQ(args.output_path, out);
}

TEST_F(ArgumentsTest, OnlyCompressFlag_ThrowsMissingInput) {
  char* argv[] = {const_cast<char*>("prog"), const_cast<char*>("--compress")};
  argument::argv_view view(argv, 2);
  EXPECT_THROW(auto args = argument::validate_arguments(view),
               exceptions::argument_exception);
}

TEST_F(ArgumentsTest, OnlyDecompressFlag_ThrowsMissingInput) {
  char* argv[] = {const_cast<char*>("prog"), const_cast<char*>("--decompress")};
  argument::argv_view view(argv, 2);
  EXPECT_THROW(auto args = argument::validate_arguments(view),
               exceptions::argument_exception);
}

}  // namespace
