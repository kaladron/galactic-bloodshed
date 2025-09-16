// Minimal test case for glaze char/unsigned char serialization bug
// 
// This is the simplest possible reproduction case. Copy this entire file
// to report the bug to glaze maintainers.
//
// Issue: char and unsigned char fields don't serialize/deserialize correctly
// 
// To test: compile with C++23 and link with glaze, then run

#include <iostream>
#include <cassert>
#include <glaze/glaze.hpp>

struct TestData {
    char char_val;
    unsigned char uchar_val;
    int int_val;  // for comparison - this should work
};

template <>
struct glz::meta<TestData> {
    using T = TestData;
    static constexpr auto value = object(
        "char_val", &T::char_val,
        "uchar_val", &T::uchar_val,
        "int_val", &T::int_val
    );
};

int main() {
    // Test case 1: Normal values
    std::cout << "=== Test 1: Normal values ===" << std::endl;
    TestData test1{65, 200, 42};  // char=65('A'), uchar=200, int=42
    
    std::cout << "Original: char=" << (int)test1.char_val 
              << " uchar=" << (int)test1.uchar_val 
              << " int=" << test1.int_val << std::endl;
    
    auto json1 = glz::write_json(test1);
    if (!json1) {
        std::cout << "Serialization failed!" << std::endl;
        return 1;
    }
    std::cout << "JSON: " << json1.value() << std::endl;
    
    TestData deserialized1{};
    auto error1 = glz::read_json(deserialized1, json1.value());
    if (error1) {
        std::cout << "Deserialization failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Deserialized: char=" << (int)deserialized1.char_val 
              << " uchar=" << (int)deserialized1.uchar_val 
              << " int=" << deserialized1.int_val << std::endl;
    
    bool test1_ok = (deserialized1.char_val == test1.char_val && 
                     deserialized1.uchar_val == test1.uchar_val &&
                     deserialized1.int_val == test1.int_val);
    std::cout << "Test 1 result: " << (test1_ok ? "PASS" : "FAIL") << std::endl << std::endl;
    
    // Test case 2: Zero values (potential special case)
    std::cout << "=== Test 2: Zero values ===" << std::endl;
    TestData test2{0, 0, 0};  // char=0, uchar=0, int=0
    
    std::cout << "Original: char=" << (int)test2.char_val 
              << " uchar=" << (int)test2.uchar_val 
              << " int=" << test2.int_val << std::endl;
    
    auto json2 = glz::write_json(test2);
    if (!json2) {
        std::cout << "Serialization failed!" << std::endl;
        return 1;
    }
    std::cout << "JSON: " << json2.value() << std::endl;
    
    TestData deserialized2{};
    auto error2 = glz::read_json(deserialized2, json2.value());
    if (error2) {
        std::cout << "Deserialization failed!" << std::endl;
        return 1;
    }
    
    std::cout << "Deserialized: char=" << (int)deserialized2.char_val 
              << " uchar=" << (int)deserialized2.uchar_val 
              << " int=" << deserialized2.int_val << std::endl;
    
    bool test2_ok = (deserialized2.char_val == test2.char_val && 
                     deserialized2.uchar_val == test2.uchar_val &&
                     deserialized2.int_val == test2.int_val);
    std::cout << "Test 2 result: " << (test2_ok ? "PASS" : "FAIL") << std::endl << std::endl;
    
    // Overall results
    std::cout << "=== Overall Results ===" << std::endl;
    if (test1_ok && test2_ok) {
        std::cout << "All tests passed - bug may be fixed!" << std::endl;
        return 0;
    } else {
        std::cout << "BUG CONFIRMED: char/unsigned char serialization issues detected!" << std::endl;
        return 1;
    }
}