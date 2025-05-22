// tests/logging_tests.cc

#include "logging.h"

#include <gtest/gtest.h>

#include <boost/filesystem.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = boost::filesystem;
using severity_level = boost::log::trivial::severity_level;

// Helper to tear down and re-init logging with a fresh file sink
static void reinit_file_sink(const std::string &pattern) {
  // Remove existing sinks
  boost::log::core::get()->remove_all_sinks();
  // Re-install only the file sink
  logging::init_logging(nullptr, pattern);
  boost::log::core::get()->flush();
}

// --------------------
// Console-sink tests
// --------------------
class LoggingConsoleTest : public ::testing::Test {
 protected:
  std::ostringstream capture_stream;

  void SetUp() override {
    // Only console sink, filtering error+ by default
    logging::init_logging(&capture_stream, /*file_pattern=*/"");
  }
};

TEST_F(LoggingConsoleTest, ErrorSeverityIsCaptured) {
  BOOST_LOG_TRIVIAL(error) << "Test message";
  std::string output = capture_stream.str();
  ASSERT_NE(output.find("Test message"), std::string::npos);
  ASSERT_NE(output.find("<error>"), std::string::npos);
}

TEST_F(LoggingConsoleTest, FatalSeverityIsCaptured) {
  BOOST_LOG_TRIVIAL(fatal) << "Fatal error occurred";
  std::string output = capture_stream.str();
  EXPECT_NE(output.find("Fatal error occurred"), std::string::npos);
  EXPECT_NE(output.find("<fatal>"), std::string::npos);
}

TEST_F(LoggingConsoleTest, InfoAndWarningAreCaptured) {
  BOOST_LOG_TRIVIAL(info) << "InfoCaptured";
  BOOST_LOG_TRIVIAL(warning) << "WarningCaptured";
  std::string output = capture_stream.str();
  EXPECT_NE(output.find("InfoCaptured"), std::string::npos);
  EXPECT_NE(output.find("WarningCaptured"), std::string::npos);
}

// --------------------
// File-sink tests
// --------------------
class LoggingFileTest : public ::testing::Test {
 protected:
  fs::path tmp_dir;
  std::string pattern;

  static void remove_all(const fs::path &p) {
    if (fs::exists(p)) fs::remove_all(p);
  }

  void SetUp() override {
    // Setup unique temp directory
    tmp_dir = fs::temp_directory_path() / fs::unique_path("logtest_%%%%-%%%%");
    remove_all(tmp_dir);
    fs::create_directories(tmp_dir);
    pattern = (tmp_dir / "test_%N.log").string();
    // Only file sink
    logging::init_logging(nullptr, pattern);
  }

  void TearDown() override { remove_all(tmp_dir); }

  std::string read_file() {
    std::vector<fs::path> files;
    for (auto &ent : fs::directory_iterator(tmp_dir)) {
      if (fs::is_regular_file(ent.status())) {
        files.push_back(ent.path());
      }
    }
    // Expect exactly one log file, but always continue to read
    EXPECT_EQ(files.size(), 1u) << "Expected exactly one log file";

    std::ifstream ifs;
    if (!files.empty()) {
      ifs.open(files[0].string());
    }
    EXPECT_TRUE(ifs.is_open()) << "Log file should open successfully";

    std::ostringstream oss;
    if (ifs.is_open()) {
      oss << ifs.rdbuf();
    }
    return oss.str();
  }
};

// Info and above should be logged to file, debug should not
TEST_F(LoggingFileTest, InfoAndAboveAreLoggedToFile) {
  BOOST_LOG_TRIVIAL(info) << "Info message";
  BOOST_LOG_TRIVIAL(error) << "Error message";
  BOOST_LOG_TRIVIAL(debug) << "Debug message";

  // Flush all sinks
  boost::log::core::get()->flush();

  std::string out = read_file();
  EXPECT_NE(out.find("Info message"), std::string::npos);
  EXPECT_NE(out.find("Error message"), std::string::npos);
  EXPECT_EQ(out.find("Debug message"), std::string::npos);
}

// Fatal should also be logged
TEST_F(LoggingFileTest, FatalIsLoggedToFile) {
  BOOST_LOG_TRIVIAL(fatal) << "Fatal message";
  boost::log::core::get()->flush();

  std::string out = read_file();
  EXPECT_NE(out.find("Fatal message"), std::string::npos);
}

// Testing debug level
TEST_F(LoggingFileTest, DebugEnabledLogsDebugButNotTrace) {
  // Enable DEBUG (but not TRACE)
  setenv("CREEPER_LOG_DEBUG", "debug", /*overwrite=*/1);
  reinit_file_sink(pattern);

  BOOST_LOG_TRIVIAL(debug) << "Debug message";
  BOOST_LOG_TRIVIAL(trace) << "Trace message";
  boost::log::core::get()->flush();

  std::string out = read_file();
  EXPECT_NE(out.find("Debug message"), std::string::npos)
      << "DEBUG should be logged when CREEPER_LOG_DEBUG=debug";
  EXPECT_EQ(out.find("Trace message"), std::string::npos)
      << "TRACE should NOT be logged when CREEPER_LOG_DEBUG=debug";
}

// Testing trace level
TEST_F(LoggingFileTest, TraceEnabledLogsTraceAndDebug) {
  // Enable TRACE (this should also capture DEBUG)
  setenv("CREEPER_LOG_DEBUG", "trace", /*overwrite=*/1);
  reinit_file_sink(pattern);

  BOOST_LOG_TRIVIAL(debug) << "Debug message";
  BOOST_LOG_TRIVIAL(trace) << "Trace message";
  boost::log::core::get()->flush();

  std::string out = read_file();
  EXPECT_NE(out.find("Debug message"), std::string::npos)
      << "DEBUG should be logged when CREEPER_LOG_DEBUG=trace";
  EXPECT_NE(out.find("Trace message"), std::string::npos)
      << "TRACE should be logged when CREEPER_LOG_DEBUG=trace";
}