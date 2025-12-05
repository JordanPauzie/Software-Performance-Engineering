#!/bin/bash
# Script to compare depth progression between baseline and optimized engines
# Tests both engines with the same time limit and compares max depth reached
#
# Usage: ./test_depth_comparison.sh [time_ms] [num_tests]
#   time_ms: Time limit for each search (default: 2000ms)
#   num_tests: Number of test runs per engine (default: 5)

set -e
cd "$(dirname "$0")"

BASELINE="./player/leiserchess_original"
OPTIMIZED="./player/leiserchess"
TIME_MS=${1:-2000}
NUM_TESTS=${2:-5}

# Check if binaries exist
if [ ! -f "$BASELINE" ]; then
    echo "Error: Baseline bot not found at $BASELINE"
    exit 1
fi

if [ ! -f "$OPTIMIZED" ]; then
    echo "Error: Optimized bot not found at $OPTIMIZED"
    exit 1
fi

echo "=========================================="
echo "Depth Comparison Test"
echo "=========================================="
echo "Baseline:  $BASELINE"
echo "Optimized: $OPTIMIZED"
echo "Time limit: ${TIME_MS}ms"
echo "Tests per engine: $NUM_TESTS"
echo ""

# Extract max depth from search output
get_max_depth() {
    local output="$1"
    # Get all depths where move_no = 1 (new depth reached)
    local depths=$(echo "$output" | grep -oP 'info depth \K[0-9]+(?= move_no 1)' | sort -nu)
    if [ -z "$depths" ]; then
        echo "0"
    else
        echo "$depths" | tail -1
    fi
}

# Run search and get max depth
run_search_get_depth() {
    local engine=$1
    local time_ms=$2
    local output=$(timeout $((time_ms / 1000 + 2)) bash -c '{
        echo "uci"
        echo "position startpos"
        echo "go time '"$((time_ms * 50))"'"
        echo "quit"
    }' | "$engine" 2>&1)
    get_max_depth "$output"
}

# Test baseline engine
echo "Testing baseline engine..."
baseline_depths=""
baseline_total=0
for i in $(seq 1 $NUM_TESTS); do
    echo -n "  Test $i/$NUM_TESTS: "
    depth=$(run_search_get_depth "$BASELINE" "$TIME_MS")
    baseline_depths="$baseline_depths $depth"
    baseline_total=$((baseline_total + depth))
    echo "Depth $depth"
done

# Test optimized engine
echo ""
echo "Testing optimized engine..."
optimized_depths=""
optimized_total=0
for i in $(seq 1 $NUM_TESTS); do
    echo -n "  Test $i/$NUM_TESTS: "
    depth=$(run_search_get_depth "$OPTIMIZED" "$TIME_MS")
    optimized_depths="$optimized_depths $depth"
    optimized_total=$((optimized_total + depth))
    echo "Depth $depth"
done

# Calculate averages
baseline_avg=$(echo "scale=2; $baseline_total / $NUM_TESTS" | bc)
optimized_avg=$(echo "scale=2; $optimized_total / $NUM_TESTS" | bc)

# Get min and max depths
baseline_min=$(echo $baseline_depths | tr ' ' '\n' | sort -n | head -1)
baseline_max=$(echo $baseline_depths | tr ' ' '\n' | sort -n | tail -1)
optimized_min=$(echo $optimized_depths | tr ' ' '\n' | sort -n | head -1)
optimized_max=$(echo $optimized_depths | tr ' ' '\n' | sort -n | tail -1)

echo ""
echo "=========================================="
echo "RESULTS"
echo "=========================================="
echo ""
echo "Baseline Engine:"
echo "  Average max depth: $baseline_avg"
echo "  Minimum depth: $baseline_min"
echo "  Maximum depth: $baseline_max"
echo "  Individual depths: $baseline_depths"
echo ""
echo "Optimized Engine:"
echo "  Average max depth: $optimized_avg"
echo "  Minimum depth: $optimized_min"
echo "  Maximum depth: $optimized_max"
echo "  Individual depths: $optimized_depths"
echo ""

# Compare depths
baseline_avg_int=$(echo "$baseline_avg" | cut -d. -f1)
optimized_avg_int=$(echo "$optimized_avg" | cut -d. -f1)

# Check if optimized reaches at least twice the depth
# Use awk for floating point comparison
twice_baseline=$(echo "scale=2; $baseline_avg * 2" | bc)
is_twice=$(awk "BEGIN {if ($optimized_avg >= $twice_baseline) print 1; else print 0}")

# Also check max depths
baseline_max_int=$baseline_max
optimized_max_int=$optimized_max
if [ "$baseline_max_int" -gt 0 ] && [ "$optimized_max_int" -ge $((baseline_max_int * 2)) ]; then
    max_is_twice=1
else
    max_is_twice=0
fi

# Check if optimized reaches twice the depth
if [ "$is_twice" = "1" ] && [ "$baseline_avg_int" -gt 0 ]; then
    echo ""
    echo "=========================================="
    echo "✓ SUCCESS: Optimized engine reaches MORE THAN TWICE the depth!"
    echo "  Baseline average: $baseline_avg"
    echo "  Optimized average: $optimized_avg"
    echo "  Required for 2x: $twice_baseline"
    echo "  Speedup: $(echo "scale=2; $optimized_avg / $baseline_avg" | bc)x"
    echo "=========================================="
    echo ""
elif [ "$max_is_twice" -eq 1 ]; then
    echo ""
    echo "=========================================="
    echo "✓ SUCCESS: Optimized engine reaches MORE THAN TWICE the MAXIMUM depth!"
    echo "  Baseline max depth: $baseline_max"
    echo "  Optimized max depth: $optimized_max"
    echo "  Baseline average: $baseline_avg"
    echo "  Optimized average: $optimized_avg"
    echo "=========================================="
    echo ""
elif [ "$optimized_avg_int" -gt "$baseline_avg_int" ]; then
    speedup=$(echo "scale=2; $optimized_avg / $baseline_avg" | bc)
    echo "Optimized engine is faster:"
    echo "  Speedup: ${speedup}x"
    echo "  Baseline average: $baseline_avg"
    echo "  Optimized average: $optimized_avg"
elif [ "$baseline_avg_int" -gt "$optimized_avg_int" ]; then
    slowdown=$(echo "scale=2; $baseline_avg / $optimized_avg" | bc)
    echo "Baseline engine is faster:"
    echo "  Optimized is ${slowdown}x slower"
    echo "  Baseline average: $baseline_avg"
    echo "  Optimized average: $optimized_avg"
else
    echo "Both engines reach similar depth:"
    echo "  Baseline average: $baseline_avg"
    echo "  Optimized average: $optimized_avg"
fi

echo ""
echo "=========================================="

