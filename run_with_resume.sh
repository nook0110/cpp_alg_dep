#!/bin/bash

# Script to run brute force with automatic resume after timeout kills
# Runs until the program completes successfully (exit code 0)

TIMEOUT_SECONDS=1
EXECUTABLE="./build/poly_dep_check"
ARGS="brute --max-degree-f 2 --max-degree-g 2 --coeff-min -1 --coeff-max 1 --batch-size 1000"

# Clean up old data
echo "Cleaning up old data..."
rm -rf data/ brute_force_state.json
mkdir -p data

echo "Starting brute force with automatic resume..."
echo "Will timeout every ${TIMEOUT_SECONDS} seconds and resume"
echo ""

iteration=1
while true; do
    echo "=========================================="
    echo "Iteration $iteration"
    echo "=========================================="
    
    timeout ${TIMEOUT_SECONDS}s ${EXECUTABLE} ${ARGS}
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo ""
        echo "=========================================="
        echo "COMPLETED SUCCESSFULLY!"
        echo "=========================================="
        break
    elif [ $exit_code -eq 124 ]; then
        echo ""
        echo "Timeout reached, resuming..."
        echo ""
    else
        echo ""
        echo "Error: Program exited with code $exit_code"
        exit $exit_code
    fi
    
    iteration=$((iteration + 1))
    sleep 0.5
done

echo ""
echo "Checking final statistics..."
python3 check_stats.py data/*
