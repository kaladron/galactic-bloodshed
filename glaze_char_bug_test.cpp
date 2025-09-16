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

    bool all_tests_passed = true;

    // Test 1: Normal values
    std::cout << "=== TEST 1: Normal values ===\n";
    TestStruct test1;
    test1.id = 42;
    test1.char_field = 65;  // Should serialize as 65 (ASCII 'A')
    test1.uchar_field = 200; // Should serialize as 200
    test1.bool_field = true;
    test1.int_field = 12345;

    std::cout << "Original values:\n";
    std::cout << "  id: " << test1.id << "\n";
    std::cout << "  char_field: " << static_cast<int>(test1.char_field) << " (should be 65)\n";
    std::cout << "  uchar_field: " << static_cast<int>(test1.uchar_field) << " (should be 200)\n";
    std::cout << "  bool_field: " << test1.bool_field << "\n";
    std::cout << "  int_field: " << test1.int_field << "\n\n";

    // Test serialization to JSON
    std::cout << "Attempting to serialize to JSON...\n";
    auto json_result1 = glz::write_json(test1);
    
    if (!json_result1) {
        std::cout << "ERROR: Failed to serialize to JSON\n";
        return 1;
    }

    std::string json_str1 = json_result1.value();
    std::cout << "JSON output: " << json_str1 << "\n\n";

    // Test deserialization from JSON
    std::cout << "Attempting to deserialize from JSON...\n";
    TestStruct deserialized1{};
    auto read_result1 = glz::read_json(deserialized1, json_str1);
    
    if (read_result1) {
        std::cout << "ERROR: Failed to deserialize from JSON\n";
        return 1;
    }

    std::cout << "Deserialized values:\n";
    std::cout << "  id: " << deserialized1.id << "\n";
    std::cout << "  char_field: " << static_cast<int>(deserialized1.char_field) << " (expected 65)\n";
    std::cout << "  uchar_field: " << static_cast<int>(deserialized1.uchar_field) << " (expected 200)\n";
    std::cout << "  bool_field: " << deserialized1.bool_field << "\n";
    std::cout << "  int_field: " << deserialized1.int_field << "\n\n";

    // Verify round-trip accuracy for test 1
    std::cout << "Checking round-trip accuracy for test 1...\n";
    bool test1_success = true;

    if (deserialized1.id != test1.id) {
        std::cout << "FAIL: id mismatch\n";
        test1_success = false;
    }

    if (deserialized1.char_field != test1.char_field) {
        std::cout << "FAIL: char_field mismatch - expected " 
                  << static_cast<int>(test1.char_field) 
                  << ", got " << static_cast<int>(deserialized1.char_field) << "\n";
        test1_success = false;
    }

    if (deserialized1.uchar_field != test1.uchar_field) {
        std::cout << "FAIL: uchar_field mismatch - expected " 
                  << static_cast<int>(test1.uchar_field)
                  << ", got " << static_cast<int>(deserialized1.uchar_field) << "\n";
        test1_success = false;
    }

    if (deserialized1.bool_field != test1.bool_field) {
        std::cout << "FAIL: bool_field mismatch\n";
        test1_success = false;
    }

    if (deserialized1.int_field != test1.int_field) {
        std::cout << "FAIL: int_field mismatch\n";
        test1_success = false;
    }

    if (test1_success) {
        std::cout << "Test 1 PASSED: All fields serialized and deserialized correctly!\n\n";
    } else {
        std::cout << "Test 1 FAILED: Some fields failed round-trip serialization\n\n";
        all_tests_passed = false;
    }

    // Test 2: Zero values (potential special case)
    std::cout << "=== TEST 2: Zero values ===\n";
    TestStruct test2;
    test2.id = 0;
    test2.char_field = 0;   // Zero char value
    test2.uchar_field = 0;  // Zero unsigned char value
    test2.bool_field = false;
    test2.int_field = 0;

    std::cout << "Original values:\n";
    std::cout << "  id: " << test2.id << "\n";
    std::cout << "  char_field: " << static_cast<int>(test2.char_field) << " (should be 0)\n";
    std::cout << "  uchar_field: " << static_cast<int>(test2.uchar_field) << " (should be 0)\n";
    std::cout << "  bool_field: " << test2.bool_field << "\n";
    std::cout << "  int_field: " << test2.int_field << "\n\n";

    // Test serialization to JSON
    std::cout << "Attempting to serialize to JSON...\n";
    auto json_result2 = glz::write_json(test2);
    
    if (!json_result2) {
        std::cout << "ERROR: Failed to serialize to JSON\n";
        return 1;
    }

    std::string json_str2 = json_result2.value();
    std::cout << "JSON output: " << json_str2 << "\n\n";

    // Test deserialization from JSON
    std::cout << "Attempting to deserialize from JSON...\n";
    TestStruct deserialized2{};
    auto read_result2 = glz::read_json(deserialized2, json_str2);
    
    if (read_result2) {
        std::cout << "ERROR: Failed to deserialize from JSON\n";
        return 1;
    }

    std::cout << "Deserialized values:\n";
    std::cout << "  id: " << deserialized2.id << "\n";
    std::cout << "  char_field: " << static_cast<int>(deserialized2.char_field) << " (expected 0)\n";
    std::cout << "  uchar_field: " << static_cast<int>(deserialized2.uchar_field) << " (expected 0)\n";
    std::cout << "  bool_field: " << deserialized2.bool_field << "\n";
    std::cout << "  int_field: " << deserialized2.int_field << "\n\n";

    // Verify round-trip accuracy for test 2
    std::cout << "Checking round-trip accuracy for test 2...\n";
    bool test2_success = true;

    if (deserialized2.id != test2.id) {
        std::cout << "FAIL: id mismatch\n";
        test2_success = false;
    }

    if (deserialized2.char_field != test2.char_field) {
        std::cout << "FAIL: char_field mismatch - expected " 
                  << static_cast<int>(test2.char_field) 
                  << ", got " << static_cast<int>(deserialized2.char_field) << "\n";
        test2_success = false;
    }

    if (deserialized2.uchar_field != test2.uchar_field) {
        std::cout << "FAIL: uchar_field mismatch - expected " 
                  << static_cast<int>(test2.uchar_field)
                  << ", got " << static_cast<int>(deserialized2.uchar_field) << "\n";
        test2_success = false;
    }

    if (deserialized2.bool_field != test2.bool_field) {
        std::cout << "FAIL: bool_field mismatch\n";
        test2_success = false;
    }

    if (deserialized2.int_field != test2.int_field) {
        std::cout << "FAIL: int_field mismatch\n";
        test2_success = false;
    }

    if (test2_success) {
        std::cout << "Test 2 PASSED: All fields serialized and deserialized correctly!\n\n";
    } else {
        std::cout << "Test 2 FAILED: Some fields failed round-trip serialization\n\n";
        all_tests_passed = false;
    }

    // Final results
    std::cout << "=============================================================\n";
    if (all_tests_passed) {
        std::cout << "SUCCESS: All tests passed! The bug may have been fixed\n";
        std::cout << "in your version of glaze, or the bug may manifest differently.\n";
    } else {
        std::cout << "BUG REPRODUCED: char and/or unsigned char fields failed round-trip serialization\n";
        std::cout << "in at least one test case. Use this code to report the bug to glaze maintainers.\n";
    }

    std::cout << "\nTest complete. Use this code to report the bug to glaze maintainers.\n";

    return all_tests_passed ? 0 : 1;
}