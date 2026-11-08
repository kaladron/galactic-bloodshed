// SPDX-License-Identifier: Apache-2.0

/// \file log_test.cc
/// \brief Unit tests for the Tier 1 gb.log structured stderr logger.

import gb.log;
import test;
import std;

namespace {

void test_level_prefixes_and_labels() {
  static_assert(GB::log::journald_prefix(GB::log::Level::Info) == "<6>");
  static_assert(GB::log::journald_prefix(GB::log::Level::Warn) == "<4>");
  static_assert(GB::log::journald_prefix(GB::log::Level::Error) == "<3>");
  static_assert(GB::log::journald_prefix(GB::log::Level::Fatal) == "<2>");

  static_assert(GB::log::level_label(GB::log::Level::Info) == "[INFO]");
  static_assert(GB::log::level_label(GB::log::Level::Warn) == "[WARN]");
  static_assert(GB::log::level_label(GB::log::Level::Error) == "[ERROR]");
  static_assert(GB::log::level_label(GB::log::Level::Fatal) == "[FATAL]");

  test::expect_eq(GB::log::journald_prefix(GB::log::Level::Info), "<6>");
  test::expect_eq(GB::log::journald_prefix(GB::log::Level::Warn), "<4>");
  test::expect_eq(GB::log::journald_prefix(GB::log::Level::Error), "<3>");
  test::expect_eq(GB::log::journald_prefix(GB::log::Level::Fatal), "<2>");

  test::expect_eq(GB::log::level_label(GB::log::Level::Info), "[INFO]");
  test::expect_eq(GB::log::level_label(GB::log::Level::Warn), "[WARN]");
  test::expect_eq(GB::log::level_label(GB::log::Level::Error), "[ERROR]");
  test::expect_eq(GB::log::level_label(GB::log::Level::Fatal), "[FATAL]");
}

void test_format_record_iso8601_utc_and_newline_normalization() {
  // 2023-11-14T22:13:20Z == 1700000000s since Unix epoch
  const std::chrono::sys_seconds ts{std::chrono::seconds{1700000000}};

  test::expect_eq(
      GB::log::format_record(GB::log::Level::Info, ts, "Server started"),
      "<6>2023-11-14T22:13:20Z [INFO] Server started\n");
  test::expect_eq(
      GB::log::format_record(GB::log::Level::Warn, ts, "Trailing newline\n"),
      "<4>2023-11-14T22:13:20Z [WARN] Trailing newline\n");
  test::expect_eq(
      GB::log::format_record(GB::log::Level::Error, ts, "Accept failed"),
      "<3>2023-11-14T22:13:20Z [ERROR] Accept failed\n");
  test::expect_eq(
      GB::log::format_record(GB::log::Level::Fatal, ts, "Database corrupted"),
      "<2>2023-11-14T22:13:20Z [FATAL] Database corrupted\n");
}

void test_scoped_sink_and_convenience_helpers() {
  std::ostringstream outer_buf;
  {
    GB::log::ScopedSink outer_sink{outer_buf};
    GB::log::info("Listening on port {}", 2010);
    GB::log::warn("Slow client queue={}", 64);

    std::ostringstream inner_buf;
    {
      GB::log::ScopedSink inner_sink{inner_buf};
      GB::log::error("Invalid option \"{}\"", "--bad");
      GB::log::fatal("Crash in command \"{}\"", "cs /Sol");
    }

    test::expect_contains(inner_buf.str(),
                          "Z [ERROR] Invalid option \"--bad\"\n");
    test::expect_true(inner_buf.str().starts_with("<3>"));
    test::expect_contains(inner_buf.str(), "\n<2>");
    test::expect_contains(inner_buf.str(),
                          "Z [FATAL] Crash in command \"cs /Sol\"\n");

    // Outer sink resumes after inner ScopedSink exits
    GB::log::info("Resumed outer sink");
  }

  const std::string outer = outer_buf.str();
  test::expect_true(outer.starts_with("<6>"));
  test::expect_contains(outer, "Z [INFO] Listening on port 2010\n");
  test::expect_contains(outer, "\n<4>");
  test::expect_contains(outer, "Z [WARN] Slow client queue=64\n");
  test::expect_contains(outer, "Z [INFO] Resumed outer sink\n");
  test::expect_not_contains(outer, "Invalid option");
}

void test_default_sink_writes_to_std_cerr() {
  std::ostringstream cerr_capture;
  auto* original_rdbuf = std::cerr.rdbuf(cerr_capture.rdbuf());
  GB::log::write_line(GB::log::Level::Info, "Default cerr sink check",
                      std::chrono::sys_seconds{std::chrono::seconds{0}});
  std::cerr.rdbuf(original_rdbuf);

  test::expect_eq(cerr_capture.str(),
                  "<6>1970-01-01T00:00:00Z [INFO] Default cerr sink check\n");
}

}  // namespace

int main() {
  test_level_prefixes_and_labels();
  test_format_record_iso8601_utc_and_newline_normalization();
  test_scoped_sink_and_convenience_helpers();
  test_default_sink_writes_to_std_cerr();

  std::println(std::cout, "✓ log_test passed!");
  return 0;
}
