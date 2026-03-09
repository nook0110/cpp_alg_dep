#!/bin/bash

set -e

echo "Building Polynomial Dependency Checker (C++)"
echo "=============================================="

if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

cd build

echo "Running CMake..."
cmake ..

echo "Building project..."
make -j$(nproc)

echo ""
echo "Build complete!"
echo "Executable: build/poly_dep_check"
echo ""
echo "To install system-wide, run:"
echo "  cd build && sudo make install"