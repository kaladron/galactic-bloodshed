#!/bin/bash

set -e
set -x
set -u

doxygen
gsutil -m rsync -r -d doxy gs://doxygen.galacticbloodshed.com/
