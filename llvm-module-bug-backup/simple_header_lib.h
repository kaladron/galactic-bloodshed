// Minimal header library demonstrating the bug
#ifndef SIMPLE_HEADER_LIB_H
#define SIMPLE_HEADER_LIB_H

#include <format>
#include <iostream>
#include <string>

namespace simple_lib {

inline std::string get_message() {
  return "Hello";
}

}  // namespace simple_lib

#endif  // SIMPLE_HEADER_LIB_H
