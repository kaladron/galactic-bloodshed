#!/bin/bash

set -e
set -x
set -u

doxygen
gsutil -m rsync -r -d gb/html gs://doxygen.galacticbloodshed.com/
