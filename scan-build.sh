#!/bin/bash
export CCC_CXX=clang++-14
export CCC_CC=clang-14
scan-build-14 ./configure
scan-build-14 --keep-cc make
