#!/bin/bash
scan-build-14 ./configure
scan-build-14 --keep-cc make
