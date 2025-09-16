// SPDX-License-Identifier: Apache-2.0
// 
// Minimal test case to reproduce glaze char/unsigned char serialization bug
// This is a standalone C++23 program that demonstrates the issue.
// 
// Bug: glaze does not handle char and unsigned char fields correctly during
// JSON serialization/deserialization. The issue was discovered while adding
// JSON serialization support to the Race class in galactic-bloodshed.
//
// Expected behavior: char and unsigned char fields should serialize to JSON
// numbers and deserialize back to the same values.
//
// Actual behavior: char/unsigned char serialization fails or produces 
// incorrect results.
//
// To reproduce: compile with C++23 and glaze library, then run this program.

#include <iostream>
#include <cassert>
#include <string>

// Glaze JSON library
#include <glaze/glaze.hpp>

// Test structure with various types including problematic char types
struct TestStruct {
    int id;
    char char_field;
    unsigned char uchar_field;
    bool bool_field;
    int int_field;
};

// Glaze meta template for reflection-based serialization
template <>
struct glz::meta<TestStruct> {
    using T = TestStruct;
    static constexpr auto value = object(
        "id", &T::id,
        "char_field", &T::char_field,
        "uchar_field", &T::uchar_field, 
        "bool_field", &T::bool_field,
        "int_field", &T::int_field
    );
};

int main() {
    std::cout << "Testing glaze serialization with char and unsigned char fields\n";
    std::cout << "=============================================================\n\n";

    // Create test object with known values
    TestStruct original;
    original.id = 42;
    original.char_field = 65;  // Should serialize as 65 (ASCII 'A')
    original.uchar_field = 200; // Should serialize as 200
    original.bool_field = true;
    original.int_field = 12345;

    std::cout << "Original values:\n";
    std::cout << "  id: " << original.id << "\n";
    std::cout << "  char_field: " << static_cast<int>(original.char_field) << " (should be 65)\n";
    std::cout << "  uchar_field: " << static_cast<int>(original.uchar_field) << " (should be 200)\n";
    std::cout << "  bool_field: " << original.bool_field << "\n";
    std::cout << "  int_field: " << original.int_field << "\n\n";

    // Test serialization to JSON
    std::cout << "Attempting to serialize to JSON...\n";
    auto json_result = glz::write_json(original);
    
    if (!json_result) {
        std::cout << "ERROR: Failed to serialize to JSON\n";
        std::cout << "Error: " << glz::format_error(json_result.error(), json_result.error()) << "\n";
        return 1;
    }

    std::string json_str = json_result.value();
    std::cout << "JSON output: " << json_str << "\n\n";

    // Test deserialization from JSON
    std::cout << "Attempting to deserialize from JSON...\n";
    TestStruct deserialized{};
    auto read_result = glz::read_json(deserialized, json_str);
    
    if (read_result) {
        std::cout << "ERROR: Failed to deserialize from JSON\n";
        std::cout << "Error: " << glz::format_error(read_result, json_str) << "\n";
        return 1;
    }

    std::cout << "Deserialized values:\n";
    std::cout << "  id: " << deserialized.id << "\n";
    std::cout << "  char_field: " << static_cast<int>(deserialized.char_field) << " (expected 65)\n";
    std::cout << "  uchar_field: " << static_cast<int>(deserialized.uchar_field) << " (expected 200)\n";
    std::cout << "  bool_field: " << deserialized.bool_field << "\n";
    std::cout << "  int_field: " << deserialized.int_field << "\n\n";

    // Verify round-trip accuracy
    std::cout << "Checking round-trip accuracy...\n";
    bool success = true;

    if (deserialized.id != original.id) {
        std::cout << "FAIL: id mismatch\n";
        success = false;
    }

    if (deserialized.char_field != original.char_field) {
        std::cout << "FAIL: char_field mismatch - expected " 
                  << static_cast<int>(original.char_field) 
                  << ", got " << static_cast<int>(deserialized.char_field) << "\n";
        success = false;
    }

    if (deserialized.uchar_field != original.uchar_field) {
        std::cout << "FAIL: uchar_field mismatch - expected " 
                  << static_cast<int>(original.uchar_field)
                  << ", got " << static_cast<int>(deserialized.uchar_field) << "\n";
        success = false;
    }

    if (deserialized.bool_field != original.bool_field) {
        std::cout << "FAIL: bool_field mismatch\n";
        success = false;
    }

    if (deserialized.int_field != original.int_field) {
        std::cout << "FAIL: int_field mismatch\n";
        success = false;
    }

    if (success) {
        std::cout << "SUCCESS: All fields serialized and deserialized correctly!\n";
        std::cout << "\nNote: If you see this success message, the bug may have been fixed\n";
        std::cout << "in your version of glaze, or the bug may manifest differently.\n";
    } else {
        std::cout << "BUG REPRODUCED: char and/or unsigned char fields failed round-trip serialization\n";
    }

    std::cout << "\n=============================================================\n";
    std::cout << "Test complete. Use this code to report the bug to glaze maintainers.\n";

    return success ? 0 : 1;
}