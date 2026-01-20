// SPDX-License-Identifier: Apache-2.0

/// \file glaze_core.cppm
/// \brief C++ Module wrapper for Glaze core customization templates
///
/// This module provides access to Glaze's low-level customization point
/// templates that allow specializing serialization/deserialization behavior
/// for custom types. These are separate from the high-level JSON API in
/// glaze.json module.
///
/// Use this when you need to specialize:
///   - glz::from<Format, T> - Custom deserialization
///   - glz::to<Format, T> - Custom serialization
///   - glz::parse<Format> - Low-level parsing machinery
///   - glz::serialize<Format> - Low-level serialization machinery
///
/// Example usage for strong ID types:
///   import glaze.core;
///   import glaze.json;
///
///   namespace glz {
///   template <FixedString Tag, typename T>
///   struct from<JSON, ID<Tag, T>> {
///     template <auto Opts>
///     static void op(ID<Tag, T>& id, is_context auto&& ctx, auto&& it, auto&&
///     end) {
///       T val{};
///       parse<JSON>::op<Opts>(val, ctx, it, end);
///       id = ID<Tag, T>{val};
///     }
///   };
///
///   template <FixedString Tag, typename T>
///   struct to<JSON, ID<Tag, T>> {
///     template <auto Opts>
///     static void op(const ID<Tag, T>& id, is_context auto&& ctx, auto&& b,
///     auto&& ix) noexcept {
///       serialize<JSON>::op<Opts>(id.value, ctx, b, ix);
///     }
///   };
///   }

module;

#include <glaze/core/read.hpp>
#include <glaze/core/write.hpp>

export module glaze.core;

/// Export the glz namespace with core customization templates
export namespace glz {

// ============================================================================
// Format Constants
// ============================================================================

/// JSON format constant
using glz::JSON;

// ============================================================================
// Customization Point Templates
// ============================================================================

/// Deserialization customization point
/// Specialize this for custom types to control how they are read from format
using glz::from;

/// Serialization customization point
/// Specialize this for custom types to control how they are written to format
using glz::to;

/// Low-level parsing machinery
/// Used within custom from<> specializations
using glz::parse;

/// Low-level serialization machinery
/// Used within custom to<> specializations
using glz::serialize;

// ============================================================================
// Context Types
// ============================================================================

/// Context concept used in customization templates
/// Parameters marked with 'is_context auto&&' accept this
using glz::is_context;

/// Context type for tracking parse/serialize state
using glz::context;

// ============================================================================
// Concept Helpers
// ============================================================================

/// Concept to check if type supports write operations for given format
using glz::write_supported;

/// Concept to check if type supports read operations for given format
using glz::read_supported;

}  // namespace glz
