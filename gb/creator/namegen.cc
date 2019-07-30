
/**
 * Different name generator implementations.
 */

#include "gb/creator/namegen.h"
#include <sstream>

bool SequentialNameGenerator::next() {
    std::ostringstream out;
    out << prefix << currval << suffix;
    currval++;
    current_value = out.str();
    return true;
}

bool IterativeNameGenerator::next() {
    current_value = "";
    if (head == tail) {
        return false;
    }
    current_value = *head;
    ++head;
    return true;
}

