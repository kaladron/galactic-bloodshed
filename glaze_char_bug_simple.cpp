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
    // Test zero values - this exposes the glaze char/unsigned char bug
    std::cout << "Testing glaze char/unsigned char serialization with zero values" << std::endl;
    std::cout << "================================================================" << std::endl;
    
    TestData original{0, 0, 0};  // char=0, uchar=0, int=0
    
    std::cout << "Original: char=" << (int)original.char_val 
              << " uchar=" << (int)original.uchar_val 
              << " int=" << original.int_val << std::endl;
    
    // Serialize to JSON
    auto json = glz::write_json(original);
    if (!json) {
        std::cout << "Serialization failed!" << std::endl;
        return 1;
    }
    
    std::cout << "JSON: " << json.value() << std::endl;
    
    // Deserialize from JSON
    TestData deserialized{};
    auto error = glz::read_json(deserialized, json.value());
    if (error) {
        std::cout << "Deserialization failed! This demonstrates the bug." << std::endl;
        std::cout << "Note: char(0) serializes as empty string \"\" while unsigned char(0) serializes as number 0" << std::endl;
        std::cout << "This inconsistency causes deserialization to fail." << std::endl;
        return 1;
    }
    
    std::cout << "Deserialized: char=" << (int)deserialized.char_val 
              << " uchar=" << (int)deserialized.uchar_val 
              << " int=" << deserialized.int_val << std::endl;
    
    // Check if values match
    bool char_ok = (deserialized.char_val == original.char_val);
    bool uchar_ok = (deserialized.uchar_val == original.uchar_val);
    bool int_ok = (deserialized.int_val == original.int_val);
    
    std::cout << "Results: char=" << (char_ok ? "OK" : "FAIL")
              << " uchar=" << (uchar_ok ? "OK" : "FAIL") 
              << " int=" << (int_ok ? "OK" : "FAIL") << std::endl;
    
    if (!char_ok || !uchar_ok) {
        std::cout << "BUG CONFIRMED: char/unsigned char fields failed round-trip!" << std::endl;
        return 1;
    }
    
    std::cout << "Unexpected: All tests passed - the bug may have been fixed!" << std::endl;
    return 0;
}