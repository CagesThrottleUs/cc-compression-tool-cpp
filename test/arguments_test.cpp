#include <filesystem>
#include <fstream>
#include <string>

#include <gtest/gtest.h>

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
  }

  void TearDown() override { std::filesystem::remove_all(temp_dir_); }

  std::filesystem::path temp_dir_;
  std::filesystem::path existing_input_;
  std::filesystem::path existing_output_;
};

TEST_F(ArgumentsTest, TooFewArguments_ThrowsUsage) {
  char* argv[] = {const_cast<char*>("prog")};
  argument::argv_view view(argv, 1);
  EXPECT_THROW(
      {
        argument::validate_arguments(view);
      },
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
      {
        argument::validate_arguments(view);
      },
      exceptions::argument_exception);
}

TEST_F(ArgumentsTest, TwoArgs_InputFileMissing_Throws) {
  std::string prog = "prog";
  std::string missing = (temp_dir_ / "nonexistent.txt").string();
  char* argv[] = {prog.data(), missing.data()};
  argument::argv_view view(argv, 2);
  EXPECT_THROW(argument::validate_arguments(view),
               exceptions::argument_exception);
}

TEST_F(ArgumentsTest, TwoArgs_InputFileMissing_MessageContainsInputFile) {
  std::string prog = "prog";
  std::string missing = (temp_dir_ / "nonexistent.txt").string();
  char* argv[] = {prog.data(), missing.data()};
  argument::argv_view view(argv, 2);
  try {
    argument::validate_arguments(view);
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
  EXPECT_NO_THROW(argument::validate_arguments(view));
}

TEST_F(ArgumentsTest, ThreeArgs_OutputFileExists_Throws) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = existing_output_.string();
  char* argv[] = {prog.data(), in.data(), out.data()};
  argument::argv_view view(argv, 3);
  EXPECT_THROW(argument::validate_arguments(view),
               exceptions::argument_exception);
}

TEST_F(ArgumentsTest, ThreeArgs_OutputFileExists_MessageContainsOutputFile) {
  std::string prog = "prog";
  std::string in = existing_input_.string();
  std::string out = existing_output_.string();
  char* argv[] = {prog.data(), in.data(), out.data()};
  argument::argv_view view(argv, 3);
  try {
    argument::validate_arguments(view);
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
  EXPECT_NO_THROW(argument::validate_arguments(view));
}

TEST_F(ArgumentsTest, UsageMessage_ContainsProgramName) {
  char* argv[] = {const_cast<char*>("my_tool")};
  argument::argv_view view(argv, 1);
  try {
    argument::validate_arguments(view);
    FAIL() << "Expected argument_exception";
  } catch (const exceptions::argument_exception& e) {
    std::string msg(e.what());
    EXPECT_NE(msg.find("my_tool"), std::string::npos);
    EXPECT_NE(msg.find("text_file_path"), std::string::npos);
  }
}

}  // namespace
