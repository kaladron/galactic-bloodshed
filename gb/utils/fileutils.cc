
#include "gb/utils/fileutils.h"

// TODO - Actually move implementation to c++ instead of fopens
void InitFile(const std::string &path, void *buffer, size_t len) {
  const char *filename = path.c_str();
  FILE *f = fopen(filename, "w+");
  if (buffer != nullptr && len > 0) {
      if (f == nullptr) {
        printf("Unable to open \"%s\".\n", filename);
        exit(-1);
      }
      fwrite(buffer, len, 1, f);
  }
  chmod(filename, 00660);
  fclose(f);
}

void EmptyFile(const std::string &path) {
    InitFile(path);
}
