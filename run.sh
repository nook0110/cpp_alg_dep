#!/bin/bash

export LD_LIBRARY_PATH=/opt/gcc-15/lib64:$LD_LIBRARY_PATH

./build/poly_dep_check "$@"