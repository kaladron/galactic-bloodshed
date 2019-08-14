
#include "storage/storage.h"
#include <iostream>

using namespace Storage;

int main(int argc, char *argv[]) {
    cout << "Num args: " << argc << endl;
    const char *filename = argc == 0 ? "test.db" : argv[0];
    SQLStore store(filename);
}

