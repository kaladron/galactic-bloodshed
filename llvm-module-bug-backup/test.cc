// Test file that imports both header_wrapper and std modules
import std;
import header_wrapper;

int main() {
  // This fails: using std::println after importing a module that wraps
  // headers containing <format> + <iostream> + <string>
  std::println("Test: {}", simple_lib::get_message());
  
  return 0;
}
