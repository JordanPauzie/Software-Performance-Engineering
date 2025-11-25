#!/bin/bash
# Test if optimized version matches baseline depth in half the time

set -e
cd "$(dirname "$0")"

BASELINE="./player/leiserchess_original"
OPTIMIZED="./player/leiserchess"
TIME_MS=${1:-2000}
TESTS=${2:-5}

echo "Testing: Baseline leiserchess [${TIME_MS}ms] versus Optimized leiserchess [$((TIME_MS/2))ms]"

# Extract max depth
get_depth() {
    echo "$1" | grep -oP 'info depth \K[0-9]+(?= move_no)' | awk '$1<=20' | sort -n | tail -1
}

# Extract nodes searched
get_nodes() {
    echo "$1" | grep -oP 'nodes \K[0-9]+' | tail -1
}

# Run search
search() {
    { echo "uci"; echo "position startpos"; echo "go time $(($2*50))"; echo "quit"; } | "$1" 2>&1
}

total_base_d=0 total_opt_d=0 total_base_n=0 total_opt_n=0

for i in $(seq 1 $TESTS); do
    base_out=$(search "$BASELINE" "$TIME_MS")
    opt_out=$(search "$OPTIMIZED" "$((TIME_MS/2))")
    
    base_d=$(get_depth "$base_out")
    opt_d=$(get_depth "$opt_out")
    base_n=$(get_nodes "$base_out")
    opt_n=$(get_nodes "$opt_out")
    
    total_base_d=$((total_base_d + base_d))
    total_opt_d=$((total_opt_d + opt_d))
    total_base_n=$((total_base_n + base_n))
    total_opt_n=$((total_opt_n + opt_n))
    
    echo "Test $i: Base d=$base_d n=$base_n, Opt d=$opt_d n=$opt_n"
done

avg_base_d=$((total_base_d / TESTS))
avg_opt_d=$((total_opt_d / TESTS))
avg_base_n=$((total_base_n / TESTS))
avg_opt_n=$((total_opt_n / TESTS))

# Calculate nodes/sec and speedup
base_nps=$((avg_base_n * 1000 / TIME_MS))
opt_nps=$((avg_opt_n * 1000 / (TIME_MS / 2)))
speedup_pct=$(( (opt_nps - base_nps) * 100 / base_nps ))

echo ""
echo "Average: Base d=$avg_base_d nps=$base_nps, Opt d=$avg_opt_d nps=$opt_nps"
echo "Speedup: ${speedup_pct}% faster (nodes/sec)"

if [ "$avg_opt_d" -ge "$avg_base_d" ]; then
    echo "Achieved 2x speed up at minimum"
    exit 0
else
    echo "Failed to achieve the 2x speed up"
    exit 1
fi