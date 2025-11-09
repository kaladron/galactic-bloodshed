// SPDX-License-Identifier: Apache-2.0

/// \file tabulate.cppm
/// \brief Module wrapper for the tabulate library
///
/// This module provides a clean C++20 module interface to the tabulate
/// library, isolating the textual header includes to avoid conflicts with
/// C++ module imports.

module;

#include <tabulate/table.hpp>

export module tabulate;

export namespace tabulate {
using tabulate::Table;
using tabulate::FontAlign;
using tabulate::FontStyle;
}  // namespace tabulate
