// SPDX-License-Identifier: Apache-2.0

/// \file scnlib.cppm
/// \brief Module wrapper for the scnlib library
///
/// This module provides a clean C++20 module interface to the scnlib
/// library, isolating the textual header includes to avoid conflicts with
/// C++ module imports.
///
/// scnlib provides scanf-like functionality with type safety and better
/// error handling. Main functions:
///   - scn::scan<T...>(source, format) - Parse from a source
///   - scn::prompt<T...>(prompt_msg, format) - Interactive input with prompt
///
/// Example usage:
///   import scnlib;
///   
///   auto result = scn::scan<int>("42", "{}");
///   if (result) {
///       auto [value] = result->values();
///       // use value
///   }

module;

#include <scn/scan.h>

export module scnlib;

export namespace scn {
// Core scanning functions
using scn::scan;
using scn::prompt;
using scn::input;
using scn::scan_value;

// Result types
using scn::scan_result;
using scn::scan_error;

// Common type aliases for convenience
using scn::expected;
}  // namespace scn
