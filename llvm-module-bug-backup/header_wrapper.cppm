// Module that wraps the header library
module;

#include "simple_header_lib.h"

export module header_wrapper;

export namespace simple_lib {
using simple_lib::get_message;
}  // namespace simple_lib
