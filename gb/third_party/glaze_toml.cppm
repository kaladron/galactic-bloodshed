// SPDX-License-Identifier: Apache-2.0

/// \file glaze_toml.cppm
/// \brief C++ Module wrapper for the Glaze TOML library
///
/// This module provides a clean C++20 module interface to Glaze's TOML
/// serialization/deserialization functionality, isolating the textual header
/// includes to avoid conflicts with C++ standard library module imports.
///
/// Glaze supports TOML 1.0 and reuses the same compile-time reflection metadata
/// used for JSON, so you can serialize the same structs to both formats without
/// additional boilerplate.
///
/// Main functions:
///   - glz::read_toml(obj, buffer) - Parse TOML into existing object
///   - glz::write_toml(obj, buffer) - Serialize object to TOML string
///   - glz::read_file_toml(obj, path, buffer) - Read TOML from file
///   - glz::write_file_toml(obj, path, buffer) - Write TOML to file
///
/// Example usage:
///   import glaze_toml;
///
///   struct AppConfig {
///       std::string host = "127.0.0.1";
///       int port = 8080;
///   };
///
///   AppConfig cfg{};
///   std::string toml_str{};
///   auto ec = glz::write_toml(cfg, toml_str);
///   if (!ec) {
///       // toml_str contains:
///       // host = "127.0.0.1"
///       // port = 8080
///   }
///
///   std::string_view config_text = R"(
///       host = "0.0.0.0"
///       port = 9000
///   )";
///   AppConfig loaded{};
///   auto read_ec = glz::read_toml(loaded, config_text);
///
/// TOML-specific features:
///   - Dotted keys: `retry.attempts = 6`
///   - Inline tables: `retry = { attempts = 6, backoff_ms = 500 }`
///   - Standard TOML number formats (binary, octal, hex)
///   - Quoted and multiline strings
///   - Arrays and inline tables
///   - Comments (#)
///
/// For struct reflection metadata customization:
///   template<>
///   struct glz::meta<AppConfig> {
///       using T = AppConfig;
///       static constexpr auto value = object(&T::host, &T::port);
///   };

module;

#include <glaze/toml.hpp>

export module glaze.toml;

/// Export the glz namespace with TOML-specific functionality
export namespace glz {

// ============================================================================
// Core Types and Concepts
// ============================================================================

/// Format enumeration for specifying serialization format
using glz::TOML;

/// Options structure for customizing read/write behavior
/// Can be used with the generic read/write functions:
///   glz::read<glz::opts{.format = glz::TOML}>(obj, buffer)
using glz::opts;

/// Error context returned by read/write operations
/// Becomes truthy when an error occurred
using glz::error_ctx;

/// Error codes for detailed error handling
using glz::error_code;

/// Expected type for result handling (similar to std::expected)
using glz::expected;

// ============================================================================
// Reflection and Metadata
// ============================================================================

/// Meta template for customizing struct reflection
/// Works identically to JSON - same metadata for both formats
using glz::meta;

/// Object helper for defining struct metadata
using glz::object;

/// Flags helper for boolean flag arrays
using glz::flags;

/// Enumerate helper for enum serialization
using glz::enumerate;

/// Reflect template for accessing reflection information
using glz::reflect;

/// For-each field iteration helper
using glz::for_each_field;

// ============================================================================
// TOML Reading Functions
// ============================================================================

/// Read TOML into an existing object
/// Signature: error_ctx read_toml(T& obj, std::string_view buffer)
/// Returns error_ctx (truthy on error)
using glz::read_toml;

/// Read TOML from a file into an object
/// Signature: error_ctx read_file_toml(T& obj, const char* path, Buffer& buffer)
using glz::read_file_toml;

// ============================================================================
// TOML Writing Functions
// ============================================================================

/// Write object to TOML string
/// Signature: error_ctx write_toml(const T& obj, std::string& buffer)
/// Returns error_ctx (truthy on error)
using glz::write_toml;

/// Write object to TOML file
/// Signature: error_ctx write_file_toml(const T& obj, const char* path, Buffer& buffer)
using glz::write_file_toml;

// ============================================================================
// Generic Read/Write with Options
// ============================================================================

/// Generic read function that respects compile-time options
/// Usage: glz::read<glz::opts{.format = glz::TOML}>(obj, buffer)
///
/// Common options for TOML:
///   - .error_on_unknown_keys = false  // Skip unknown keys gracefully
///   - .error_on_missing_keys = true   // Require all keys present
///   - .skip_null_members = true       // Skip null optional members
using glz::read;

/// Generic write function that respects compile-time options
/// Usage: glz::write<glz::opts{.format = glz::TOML}>(obj, buffer)
using glz::write;

// ============================================================================
// Error Handling
// ============================================================================

/// Format an error into a human-readable string with context
/// Shows line/column and surrounding text for parse errors
using glz::format_error;

// ============================================================================
// Wrappers for Customizing Behavior
// ============================================================================

/// Skip a field during serialization (acknowledge existence but don't read/write)
using glz::skip;

/// Hide a field from output while allowing API access
using glz::hide;

/// Treat numbers as quoted strings
using glz::quoted_num;

/// Custom read/write handler wrapper
using glz::custom;

/// Read constraint wrapper for validation
using glz::read_constraint;

}  // namespace glz
