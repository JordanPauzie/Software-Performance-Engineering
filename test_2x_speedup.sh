#!/bin/bash
# Test to verify optimized version can search same depth in half the time
# Compares leiserchess (optimized) vs leiserchess_original (baseline)

set -e

cd "$(dirname "$0")"

# Configuration
BASELINE_BIN="./player/leiserchess_original"
OPTIMIZED_BIN="./player/leiserchess"
TEST_TIME_MS=${1:-2000}  # Time for baseline (default 2 seconds)
NUM_TESTS=${2:-5}         # Number of test positions

echo "=========================================="
echo "2x Speedup Verification Test"
echo "=========================================="
echo "Baseline binary: $BASELINE_BIN"
echo "Optimized binary: $OPTIMIZED_BIN"
echo "Baseline time: ${TEST_TIME_MS}ms"
echo "Target time (optimized): $((TEST_TIME_MS / 2))ms"
echo "Number of tests: $NUM_TESTS"
echo ""

# Check binaries exist
if [ ! -f "$BASELINE_BIN" ]; then
    echo "❌ ERROR: Baseline binary not found: $BASELINE_BIN"
    exit 1
fi

if [ ! -f "$OPTIMIZED_BIN" ]; then
    echo "❌ ERROR: Optimized binary not found: $OPTIMIZED_BIN"
    echo "Building optimized version..."
    cd player
    make clean
    make > /dev/null 2>&1
    cd ..
    if [ ! -f "$OPTIMIZED_BIN" ]; then
        echo "❌ ERROR: Failed to build optimized binary"
        exit 1
    fi
fi

# Make sure binaries are executable
chmod +x "$BASELINE_BIN" "$OPTIMIZED_BIN"

# Function to run timed search and extract max depth
run_timed_search() {
    local binary="$1"
    local time_ms="$2"
    local adjusted_time=$((time_ms * 50))  # Engine uses ~1/50 of specified time
    
    {
        echo "uci"
        echo "position startpos"
        echo "go time $adjusted_time"
        echo "quit"
    } | "$binary" 2>&1
}

# Function to extract max depth from output
# Looks for "info depth X" lines and finds the maximum valid depth
# Format: "info depth X move_no Y time ..."
# Only considers reasonable depths (1-20) - ignores internal counters and timeout markers
extract_max_depth() {
    local output="$1"
    local max_depth=0
    
    # Process lines in order
    while IFS= read -r line; do
        # Match "info depth X move_no" pattern - this indicates a completed depth
        if [[ $line =~ info[[:space:]]+depth[[:space:]]+([0-9]+)[[:space:]]+move_no ]]; then
            local depth="${BASH_REMATCH[1]}"
            # Only consider reasonable search depths (1-20)
            # Ignore internal counters (100+), timeout markers (980-999), and INF_DEPTH (999)
            if [ "$depth" -ge 1 ] && [ "$depth" -le 20 ] && [ "$depth" -gt "$max_depth" ]; then
                max_depth=$depth
            fi
            # Stop processing if we hit timeout markers (980-999)
            if [ "$depth" -ge 980 ]; then
                break
            fi
        fi
    done <<< "$output"
    
    # Return the maximum valid depth found
    echo "$max_depth"
}

# Function to extract nodes searched
extract_nodes() {
    local output="$1"
    local last_nodes=0
    
    while IFS= read -r line; do
        if [[ $line =~ nodes[[:space:]]+([0-9]+) ]]; then
            last_nodes="${BASH_REMATCH[1]}"
        fi
    done <<< "$output"
    
    echo "$last_nodes"
}

echo "Running tests..."
echo ""

# Arrays to store results
declare -a baseline_depths
declare -a optimized_depths
declare -a baseline_nodes
declare -a optimized_nodes

# Run tests
for i in $(seq 1 $NUM_TESTS); do
    echo "Test $i/$NUM_TESTS:"
    
    # Test baseline
    echo -n "  Baseline (${TEST_TIME_MS}ms): "
    baseline_output=$(run_timed_search "$BASELINE_BIN" "$TEST_TIME_MS")
    baseline_depth=$(extract_max_depth "$baseline_output")
    baseline_node=$(extract_nodes "$baseline_output")
    baseline_depths+=($baseline_depth)
    baseline_nodes+=($baseline_node)
    echo "Depth $baseline_depth, Nodes $baseline_node"
    
    # Test optimized with HALF the time
    optimized_time=$((TEST_TIME_MS / 2))
    echo -n "  Optimized (${optimized_time}ms): "
    optimized_output=$(run_timed_search "$OPTIMIZED_BIN" "$optimized_time")
    optimized_depth=$(extract_max_depth "$optimized_output")
    optimized_node=$(extract_nodes "$optimized_output")
    optimized_depths+=($optimized_depth)
    optimized_nodes+=($optimized_node)
    echo "Depth $optimized_depth, Nodes $optimized_node"
    
    # Compare
    if [ "$optimized_depth" -ge "$baseline_depth" ]; then
        echo "  ✅ PASS: Optimized reached same or greater depth in half the time"
    else
        depth_diff=$((baseline_depth - optimized_depth))
        echo "  ⚠️  WARNING: Optimized reached depth $depth_diff less (target: same or greater)"
    fi
    echo ""
done

# Calculate averages
baseline_avg_depth=0
optimized_avg_depth=0
baseline_avg_nodes=0
optimized_avg_nodes=0

for i in $(seq 0 $((NUM_TESTS - 1))); do
    baseline_avg_depth=$((baseline_avg_depth + baseline_depths[$i]))
    optimized_avg_depth=$((optimized_avg_depth + optimized_depths[$i]))
    baseline_avg_nodes=$((baseline_avg_nodes + baseline_nodes[$i]))
    optimized_avg_nodes=$((optimized_avg_nodes + optimized_nodes[$i]))
done

baseline_avg_depth=$((baseline_avg_depth / NUM_TESTS))
optimized_avg_depth=$((optimized_avg_depth / NUM_TESTS))
baseline_avg_nodes=$((baseline_avg_nodes / NUM_TESTS))
optimized_avg_nodes=$((optimized_avg_nodes / NUM_TESTS))

# Calculate speedup
if [ "$baseline_avg_nodes" -gt 0 ]; then
    baseline_nps=$((baseline_avg_nodes * 1000 / TEST_TIME_MS))
    optimized_nps=$((optimized_avg_nodes * 1000 / (TEST_TIME_MS / 2)))
    speedup_pct=$(( (optimized_nps - baseline_nps) * 100 / baseline_nps ))
else
    baseline_nps=0
    optimized_nps=0
    speedup_pct=0
fi

echo "=========================================="
echo "Results Summary"
echo "=========================================="
echo ""
echo "Baseline (${TEST_TIME_MS}ms):"
echo "  Average depth: $baseline_avg_depth"
echo "  Average nodes: $baseline_avg_nodes"
echo "  Nodes/sec: $baseline_nps"
echo ""
echo "Optimized ($((TEST_TIME_MS / 2))ms = half the time):"
echo "  Average depth: $optimized_avg_depth"
echo "  Average nodes: $optimized_avg_nodes"
echo "  Nodes/sec: $optimized_nps"
echo ""
echo "Speedup: ${speedup_pct}% faster (nodes/sec)"
echo ""

# Final verdict
echo "=========================================="
echo "2x Speedup Test Result"
echo "=========================================="

if [ "$optimized_avg_depth" -ge "$baseline_avg_depth" ]; then
    echo "✅ SUCCESS: Optimized version reaches same or greater depth"
    echo "   in HALF the time!"
    echo ""
    echo "   Baseline:  Depth $baseline_avg_depth in ${TEST_TIME_MS}ms"
    echo "   Optimized: Depth $optimized_avg_depth in $((TEST_TIME_MS / 2))ms"
    echo ""
    if [ "$optimized_avg_depth" -gt "$baseline_avg_depth" ]; then
        echo "🎉 EXCELLENT: Optimized version searches DEEPER in half the time!"
    fi
    exit 0
else
    depth_diff=$((baseline_avg_depth - optimized_avg_depth))
    echo "❌ FAIL: Optimized version does not reach same depth"
    echo "   Baseline:  Depth $baseline_avg_depth"
    echo "   Optimized: Depth $optimized_avg_depth (missing $depth_diff)"
    echo ""
    echo "   Need more optimizations to achieve 2x speedup goal."
    exit 1
fi

