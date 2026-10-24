// SPDX-License-Identifier: Apache-2.0

/// \file core.cppm
/// \brief Core presentation dispatch (present_to / render_for_mode) and Glaze
/// JSON envelope serialization.

export module gb.presentation.core;

import strong_id;
import glaze.core;
import glaze.json;
import gb.entities;
import gb.services;
import gb.repositories.glaze;
import std;

export namespace GB::presentation {

/// Wire-protocol JSON envelope wrapping a typed command result, view model, or
/// asynchronous domain event without copying the payload.
template <typename T>
struct JsonEnvelope {
  std::string_view type{};
  const T* data{nullptr};
};

/// Serializes `data` into a newline-delimited JSON envelope:
/// `{"type":"<type>","data":<json>}\n`.
/// Uses a single named return variable (`out`) for guaranteed NRVO.
template <typename T>
[[nodiscard]] std::string render_json_envelope(std::string_view type,
                                               const T& data) {
  std::string out;
  const JsonEnvelope<T> envelope{.type = type, .data = &data};
  if (const auto ec = glz::write_json(envelope, out); !ec) {
    out.push_back('\n');
  }
  return out;
}

/// Concept satisfied by any ViewModel that provides a non-member
/// `render_for_mode(UiMode, const T&)` overload in `namespace
/// GB::presentation`.
template <typename T>
concept HasModeRenderOverload = requires(UiMode mode, const T& vm) {
  { render_for_mode(mode, vm) } -> std::same_as<std::string>;
};

/// Polymorphically formats `vm` according to `mode` and appends it to `out`.
/// Found via ADL from `GameObj::present(const ViewModel&)` because `UiMode`
/// resides in `namespace GB::presentation`.
template <HasModeRenderOverload ViewModel>
void present_to(OutputBuffer& out, UiMode mode, const ViewModel& vm) {
  out.append(render_for_mode(mode, vm));
}

/// Terminal display capabilities derived from a Telnet `TTYPE` (`$TERM`)
/// identifier.
struct TerminalCapabilities {
  bool ansi_sgr{false};
  bool color{false};
  bool utf8{false};

  [[nodiscard]] constexpr bool
  operator==(const TerminalCapabilities&) const noexcept = default;
};

/// Pure compile-time classifier mapping a `$TERM` / Telnet `TTYPE` string to
/// supported ANSI SGR, colour, and UTF-8 capabilities.
[[nodiscard]] constexpr TerminalCapabilities
detect_terminal_capabilities(std::string_view term) noexcept {
  auto contains_ci = [](std::string_view haystack,
                        std::string_view needle) constexpr noexcept {
    if (needle.empty() || haystack.size() < needle.size()) {
      return false;
    }
    for (std::size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
      bool match = true;
      for (std::size_t j = 0; j < needle.size(); ++j) {
        char a = haystack[i + j];
        char b = needle[j];
        if (a >= 'A' && a <= 'Z') a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z') b = static_cast<char>(b - 'A' + 'a');
        if (a != b) {
          match = false;
          break;
        }
      }
      if (match) return true;
    }
    return false;
  };

  if (term.empty() || contains_ci(term, "dumb")) {
    return {};
  }
  const bool color =
      contains_ci(term, "color") || contains_ci(term, "xterm") ||
      contains_ci(term, "rxvt") || contains_ci(term, "tmux") ||
      contains_ci(term, "screen") || contains_ci(term, "linux") ||
      contains_ci(term, "ansi") || contains_ci(term, "alacritty") ||
      contains_ci(term, "kitty") || contains_ci(term, "wezterm") ||
      contains_ci(term, "ghostty") || contains_ci(term, "foot");
  const bool ansi_sgr = color || contains_ci(term, "vt100") ||
                        contains_ci(term, "vt102") ||
                        contains_ci(term, "vt220");
  return TerminalCapabilities{
      .ansi_sgr = ansi_sgr,
      .color = color,
      .utf8 = color,
  };
}

}  // namespace GB::presentation
