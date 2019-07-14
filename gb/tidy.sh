#!/bin/bash

set -e
set -u

show_info() {
  clang_tidy -checks=* -list-checks
  exit 0
}

if [ $# -eq 0 ]; then
  show_info
fi

check=$1
shift
clang_tidy $@ \
  -checks=-*,$check -- \
  '-DPREFIX="/usr"' '-DPKGSTATEDIR="/lib/galactic-bloodshed/"' '-DPKGDATADIR="/share/galactic-bloodshed/"' '-DDOCDIR="/share/galactic-bloodshed/"' -std=c++2a

# clang-analyzer-core.NonnilStringConstants,modernize-deprecated-headers,modernize-loop-convert,modernize-replace-auto-ptr,modernize-use-noexcept -- \
#-header-filter=.*
