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
    if (haystack.size() < needle.size()) {
      return false;
    }
    for (std::size_t i = 0; i <= haystack.size() - needle.size(); ++i) {
      bool match = true;
      for (std::size_t j = 0; j < needle.size(); ++j) {
        char a = haystack[i + j];
        if (a >= 'A' && a <= 'Z') {
          a = static_cast<char>(a - 'A' + 'a');
        }
        if (a != needle[j]) {
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
  constexpr std::array<std::string_view, 12> COLOR_TERMS = {
      "color", "xterm",     "rxvt",  "tmux",    "screen",  "linux",
      "ansi",  "alacritty", "kitty", "wezterm", "ghostty", "foot"};
  constexpr std::array<std::string_view, 3> VT_TERMS = {"vt100", "vt102",
                                                        "vt220"};
  const bool color =
      std::ranges::any_of(COLOR_TERMS, [&](std::string_view pat) {
        return contains_ci(term, pat);
      });
  const bool ansi_sgr =
      color || std::ranges::any_of(VT_TERMS, [&](std::string_view pat) {
        return contains_ci(term, pat);
      });
  return TerminalCapabilities{
      .ansi_sgr = ansi_sgr,
      .color = color,
      .utf8 = color,
  };
}

}  // namespace GB::presentation
