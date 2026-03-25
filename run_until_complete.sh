#!/bin/bash

# Script to run brute force until completion, restarting on crashes
# Continues running until the program completes successfully (exit code 0)

EXECUTABLE="./build/poly_dep_check"
ARGS="brute --max-degree-f 3 --max-degree-g 3 --max-degree-q 10 --coeff-min -1 --coeff-max 1 --num-workers 96"

# Clean up old data
echo "Cleaning up old data..."
rm -rf data/ brute_force_state.json
mkdir -p data

echo "Starting brute force with automatic restart on crash..."
echo ""

iteration=1
while true; do
    echo "=========================================="
    echo "Iteration $iteration"
    echo "=========================================="
    
    ${EXECUTABLE} ${ARGS}
    exit_code=$?
    
    if [ $exit_code -eq 0 ]; then
        echo ""
        echo "=========================================="
        echo "COMPLETED SUCCESSFULLY!"
        echo "=========================================="
        break
    else
        echo ""
        echo "Program crashed with exit code $exit_code, restarting..."
        echo ""
        sleep 0.5
    fi
    
    iteration=$((iteration + 1))
done

echo ""
echo "Checking final statistics..."
python3 check_stats.py data/* | tee final_statistics.txt
echo ""
echo "Statistics saved to final_statistics.txt"
