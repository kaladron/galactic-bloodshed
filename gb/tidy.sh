#!/bin/bash

set -e
set -u

TIDY=clang-tidy-9

show_info() {
  ${TIDY} -checks=* -list-checks
  exit 0
}

if [ $# -eq 0 ]; then
  show_info
fi

check=$1
shift
${TIDY} $@ \
  -checks=-*,$check -- -I.. \
  '-DPREFIX="/usr"' '-DPKGSTATEDIR="/lib/galactic-bloodshed/"' '-DPKGDATADIR="/share/galactic-bloodshed/"' '-DDOCDIR="/share/galactic-bloodshed/"' -std=c++2a

# clang-analyzer-core.NonnilStringConstants,modernize-deprecated-headers,modernize-loop-convert,modernize-replace-auto-ptr,modernize-use-noexcept -- \
#-header-filter=.*
