// SPDX-License-Identifier: Apache-2.0

/// \file glaze_json.cppm
/// \brief C++ Module wrapper for the Glaze JSON library
///
/// This module provides a clean C++20 module interface to Glaze's JSON
/// serialization/deserialization functionality, isolating the textual header
/// includes to avoid conflicts with C++ standard library module imports.
///
/// Glaze is an extremely fast JSON library that uses compile-time reflection
/// for automatic serialization of C++ structs without requiring any macros
/// or metadata definitions for aggregate initializable types.
///
/// Main functions:
///   - glz::read_json<T>(buffer) - Parse JSON into a new object
///   - glz::read_json(obj, buffer) - Parse JSON into existing object
///   - glz::write_json(obj) - Serialize object to JSON string
///   - glz::write_json(obj, buffer) - Serialize object to existing buffer
///   - glz::read_file_json(obj, path, buffer) - Read JSON from file
///   - glz::write_file_json(obj, path, buffer) - Write JSON to file
///
/// Example usage:
///   import glaze_json;
///
///   struct MyStruct {
///       int value = 42;
///       std::string name = "test";
///   };
///
///   MyStruct obj{};
///   auto json = glz::write_json(obj).value_or("error");
///   // json == R"({"value":42,"name":"test"})"
///
///   auto result = glz::read_json<MyStruct>(json);
///   if (result) {
///       auto& parsed = result.value();
///   }
///
/// For struct reflection metadata customization:
///   template<>
///   struct glz::meta<MyStruct> {
///       using T = MyStruct;
///       static constexpr auto value = object(&T::value, &T::name);
///   };

module;

#include <glaze/json.hpp>

export module glaze.json;

/// Export the glz namespace with JSON-specific functionality
export namespace glz {

// ============================================================================
// Core Types and Concepts
// ============================================================================

/// Format enumeration for specifying serialization format
using glz::JSON;

/// Options structure for customizing read/write behavior
using glz::opts;

/// Error context returned by read/write operations
using glz::error_ctx;

/// Error codes for detailed error handling
using glz::error_code;

/// Expected type for result handling (similar to std::expected)
using glz::expected;

// ============================================================================
// Reflection and Metadata
// ============================================================================

/// Meta template for customizing struct reflection
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
// JSON Reading Functions
// ============================================================================

/// Read JSON into a new object of type T
/// Returns std::expected<T, error_ctx>
using glz::read_json;

/// Read JSON from a file into an object
/// Signature: error_ctx read_file_json(T& obj, const char* path, Buffer& buffer)
using glz::read_file_json;

// ============================================================================
// JSON Writing Functions
// ============================================================================

/// Write object to JSON string
/// Returns std::expected<std::string, error_ctx> or error_ctx if buffer provided
using glz::write_json;

/// Write object to JSON file
/// Signature: error_ctx write_file_json(const T& obj, const char* path, Buffer& buffer)
using glz::write_file_json;

// ============================================================================
// Generic Read/Write with Options
// ============================================================================

/// Generic read function that respects compile-time options
/// Usage: glz::read<glz::opts{.format = glz::JSON}>(obj, buffer)
using glz::read;

/// Generic write function that respects compile-time options
/// Usage: glz::write<glz::opts{.format = glz::JSON}>(obj, buffer)
using glz::write;

// ============================================================================
// Error Handling
// ============================================================================

/// Format an error into a human-readable string with context
/// Shows line/column and surrounding text for parse errors
using glz::format_error;

// ============================================================================
// JSON Utilities
// ============================================================================

/// Prettify a JSON string with indentation
using glz::prettify_json;

/// Minify a JSON string (remove whitespace)
using glz::minify_json;

// ============================================================================
// JSON Pointer Syntax (RFC 6901)
// ============================================================================

/// Get a value at a JSON pointer path
using glz::get;

/// Set a value at a JSON pointer path
using glz::set;

// ============================================================================
// Wrappers for Customizing Behavior
// ============================================================================

/// Skip a field during serialization (acknowledge existence but don't read/write)
using glz::skip;

/// Hide a field from JSON output while allowing API access
using glz::hide;

/// Treat numbers as quoted strings in JSON
using glz::quoted_num;

/// Custom read/write handler wrapper
using glz::custom;

/// Read constraint wrapper for validation
using glz::read_constraint;

// ============================================================================
// Container Helpers
// ============================================================================

/// Compile-time array type for mixed-type JSON arrays
using glz::arr;

/// Compile-time object type for building JSON objects on the fly
using glz::obj;

/// Merge multiple objects into one JSON object
using glz::merge;

// ============================================================================
// NDJSON (Newline Delimited JSON) Support
// ============================================================================

/// Read newline-delimited JSON into array-like container
using glz::read_ndjson;

/// Write array-like container as newline-delimited JSON
using glz::write_ndjson;

// ============================================================================
// JSONC (JSON with Comments) Support
// ============================================================================

/// Read JSONC (JSON with // and /* */ comments)
using glz::read_jsonc;

// ============================================================================
// JSON Schema Support
// ============================================================================

/// Generate JSON schema for a type
using glz::json_schema;

// ============================================================================
// Generic JSON (Dynamic JSON Handling)
// ============================================================================

/// Generic JSON value type for dynamic JSON handling
/// Can hold any JSON value: null, bool, number, string, array, object
using glz::generic;

}  // namespace glz
