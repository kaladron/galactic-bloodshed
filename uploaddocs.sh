#!/bin/bash

set -e
set -x
set -u

doxygen
gsutil -m rsync -r -d src/html gs://doxygen.galacticbloodshed.com/
