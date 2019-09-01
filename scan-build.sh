#!/bin/bash
scan-build-9 ./configure
scan-build-9 --keep-cc make
