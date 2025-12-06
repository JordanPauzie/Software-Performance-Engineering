#!/bin/bash
# Test script to show depth progression across multiple test runs
# Runs multiple tests and shows statistics on how deep the search can go
#
# Usage: ./test_depth_progression.sh [time_ms] [engine] [num_tests]
#   time_ms: Time limit for each search (default: 2000ms)
#   engine: "baseline" or "optimized" (default: "optimized")
#   num_tests: Number of test runs (default: 25)

set -e
cd "$(dirname "$0")"

BASELINE="./player/leiserchess_original"
OPTIMIZED="./player/leiserchess"
TIME_MS=${1:-2000}
ENGINE_CHOICE=${2:-optimized}
NUM_TESTS=${3:-5}

if [ "$ENGINE_CHOICE" = "baseline" ]; then
    ENGINE="$BASELINE"
    ENGINE_NAME="Baseline"
elif [ "$ENGINE_CHOICE" = "optimized" ]; then
    ENGINE="$OPTIMIZED"
    ENGINE_NAME="Optimized"
else
    echo "Error: engine must be 'baseline' or 'optimized'"
    exit 1
fi

echo "=========================================="
echo "Depth Progression Test - $ENGINE_NAME"
echo "=========================================="
echo "Engine: $ENGINE"
echo "Time limit per test: ${TIME_MS}ms"
echo "Number of tests: $NUM_TESTS"
echo "Estimated total time: ~$((TIME_MS * NUM_TESTS / 1000)) seconds"
echo ""

# Extract depth progression from search output
get_depth_progression() {
    local output="$1"
    # Get all depths where move_no = 1 (new depth reached)
    echo "$output" | grep -oP 'info depth \K[0-9]+(?= move_no 1)' | sort -nu
}

# Get max depth
get_max_depth() {
    local progression=$(get_depth_progression "$1")
    if [ -z "$progression" ]; then
        echo "0"
    else
        echo "$progression" | tail -1
    fi
}

# Get nodes at max depth
get_nodes_at_depth() {
    local output="$1"
    local depth="$2"
    echo "$output" | grep "info depth $depth move_no 1" | grep -oP 'nodes \K[0-9]+' | head -1
}

# Run search
run_search() {
    local engine=$1
    local time_ms=$2
    { 
        echo "uci"
        echo "position startpos"
        echo "go time $((time_ms * 50))"
        echo "quit"
    } | "$engine" 2>&1
}

# Storage for statistics
max_depths=""
all_depths=""
total_nodes=0
start_time=$(date +%s)

echo "Running $NUM_TESTS tests..."
echo ""

for i in $(seq 1 $NUM_TESTS); do
    echo -n "Test $i/$NUM_TESTS: "
    
    # Run search
    output=$(run_search "$ENGINE" "$TIME_MS")
    
    # Extract data
    max_depth=$(get_max_depth "$output")
    depths=$(get_depth_progression "$output")
    nodes=$(get_nodes_at_depth "$output" "$max_depth")
    
    # Store for statistics
    max_depths="$max_depths $max_depth"
    if [ -n "$depths" ]; then
        all_depths="$all_depths $(echo "$depths" | tr '\n' ' ')"
    fi
    if [ -n "$nodes" ] && [ "$nodes" != "0" ]; then
        total_nodes=$((total_nodes + nodes))
    fi
    
    # Show progress
    echo "Max depth: $max_depth"
    
    # Show estimated time remaining
    if [ $i -eq 1 ]; then
        elapsed=$(($(date +%s) - start_time))
        if [ $elapsed -gt 0 ]; then
            estimated_total=$((elapsed * NUM_TESTS))
            estimated_remaining=$((estimated_total - elapsed))
            echo "  Estimated time remaining: ~${estimated_remaining}s"
        fi
    fi
done

end_time=$(date +%s)
total_elapsed=$((end_time - start_time))

echo ""
echo "=========================================="
echo "RESULTS"
echo "=========================================="
echo ""

# Calculate statistics
avg_max_depth=$(echo $max_depths | tr ' ' '\n' | awk '{sum+=$1; count++} END {if(count>0) printf "%.1f", sum/count; else print "0"}')
min_max_depth=$(echo $max_depths | tr ' ' '\n' | sort -n | head -1)
max_max_depth=$(echo $max_depths | tr ' ' '\n' | sort -n | tail -1)
avg_nodes=$((total_nodes / NUM_TESTS))

# Count how many times each depth was reached
echo "Max depth statistics:"
echo "  Average max depth: $avg_max_depth"
echo "  Minimum max depth: $min_max_depth"
echo "  Maximum max depth: $max_max_depth"
echo "  Average nodes at max depth: $avg_nodes"
echo ""

# Show depth distribution
echo "Max depth distribution:"
echo $max_depths | tr ' ' '\n' | sort -n | uniq -c | sort -rn | head -10 | awk '{printf "  Depth %3d: %2d times\n", $2, $1}'
echo ""

# Show all unique depths reached across all tests
echo "All depths reached (across all tests):"
unique_depths=$(echo $all_depths | tr ' ' '\n' | sort -nu)
depth_count=$(echo "$unique_depths" | wc -l)
if [ "$depth_count" -gt 50 ]; then
    echo "  (showing first 50 of $depth_count depths)"
    echo "$unique_depths" | head -50 | awk '{printf "  %3d", $1; if(NR % 10 == 0) printf "\n"} END {if(NR % 10 != 0) printf "\n"}'
    echo "  ... and $(($depth_count - 50)) more"
else
    echo "$unique_depths" | awk '{printf "  %3d", $1; if(NR % 10 == 0) printf "\n"} END {if(NR % 10 != 0) printf "\n"}'
fi
echo ""

# Show timing
echo "Timing:"
echo "  Total time: ${total_elapsed}s"
echo "  Average time per test: $((total_elapsed * 1000 / NUM_TESTS))ms"
echo "  Expected time per test: ${TIME_MS}ms"
echo ""

echo "=========================================="
echo ""
echo "Summary:"
echo "  - Ran $NUM_TESTS tests with ${TIME_MS}ms time limit each"
echo "  - Average max depth reached: $avg_max_depth"
echo "  - Best max depth reached: $max_max_depth"
echo "  - Total unique depths reached: $depth_count"
echo ""
echo "To compare with other version, run:"
echo "  ./test_depth_progression.sh $TIME_MS baseline $NUM_TESTS"
echo "  ./test_depth_progression.sh $TIME_MS optimized $NUM_TESTS"
echo "=========================================="
