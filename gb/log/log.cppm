// SPDX-License-Identifier: Apache-2.0

/// \file log.cppm
/// \brief Zero-dependency Tier 1 structured logger writing systemd-journald
/// priority-prefixed UTC records to unbuffered stderr.

export module gb.log;

import std;

export namespace GB::log {

/// Severity levels mapped to systemd-journald / syslog priority prefixes.
enum class Level : std::uint8_t {
  Info,
  Warn,
  Error,
  Fatal,
};

/// Return the systemd-journald priority prefix (`<6>`, `<4>`, `<3>`, `<2>`).
[[nodiscard]] constexpr std::string_view journald_prefix(Level level) noexcept {
  switch (level) {
    case Level::Info:
      return "<6>";
    case Level::Warn:
      return "<4>";
    case Level::Error:
      return "<3>";
    case Level::Fatal:
      break;
  }
  return "<2>";
}

/// Return the human-readable bracketed severity label.
[[nodiscard]] constexpr std::string_view level_label(Level level) noexcept {
  switch (level) {
    case Level::Info:
      return "[INFO]";
    case Level::Warn:
      return "[WARN]";
    case Level::Error:
      return "[ERROR]";
    case Level::Fatal:
      break;
  }
  return "[FATAL]";
}

/// Format a complete newline-terminated log record for an explicit UTC time.
[[nodiscard]] inline std::string
format_record(Level level, std::chrono::sys_seconds timestamp,
              std::string_view message) {
  if (message.ends_with('\n')) {
    message.remove_suffix(1);
  }
  return std::format("{}{:%FT%TZ} {} {}\n", journald_prefix(level), timestamp,
                     level_label(level), message);
}

namespace detail {

inline std::ostream*& active_sink() noexcept {
  static thread_local std::ostream* sink = &std::cerr;
  return sink;
}

}  // namespace detail

/// RAII guard temporarily redirecting current-thread log output to a stream.
class ScopedSink {
public:
  explicit ScopedSink(std::ostream& stream) noexcept
      : previous_(std::exchange(detail::active_sink(), &stream)) {}

  ~ScopedSink() {
    detail::active_sink() = previous_;
  }

  ScopedSink(const ScopedSink&) = delete;
  ScopedSink& operator=(const ScopedSink&) = delete;

private:
  std::ostream* previous_;
};

/// Emit a single pre-formatted record atomically and flush the active sink.
inline void write_line(Level level, std::string_view message,
                       std::chrono::sys_seconds timestamp =
                           std::chrono::floor<std::chrono::seconds>(
                               std::chrono::system_clock::now())) {
  const std::string line = format_record(level, timestamp, message);
  std::ostream& out = *detail::active_sink();
  out.write(line.data(), static_cast<std::streamsize>(line.size()));
  out.flush();
}

/// Log an informational daemon event (`<6>` / `[INFO]`).
template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args) {
  write_line(Level::Info, std::format(fmt, std::forward<Args>(args)...));
}

/// Log a warning or recoverable invariant anomaly (`<4>` / `[WARN]`).
template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args) {
  write_line(Level::Warn, std::format(fmt, std::forward<Args>(args)...));
}

/// Log an operational error (`<3>` / `[ERROR]`).
template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args) {
  write_line(Level::Error, std::format(fmt, std::forward<Args>(args)...));
}

/// Log an unrecoverable fatal crash condition (`<2>` / `[FATAL]`).
template <typename... Args>
void fatal(std::format_string<Args...> fmt, Args&&... args) {
  write_line(Level::Fatal, std::format(fmt, std::forward<Args>(args)...));
}

}  // namespace GB::log
