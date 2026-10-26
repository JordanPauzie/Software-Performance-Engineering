#!/usr/bin/env bash

if [ "$#" -le 0 ]; then
	echo "Usage: bash test-real.sh malloc_wrapper.so"
	exit -1
fi

MALLOC_SO=$(realpath $1)
TEST_CASE=$2
TEST_PATH=/var/6106/project3-tests/test-suite-build

TOTAL_RATIO=1.000
TOTAL_APP=0

test_bench() {
	
	stdout_PATH=$(mktemp /tmp/std_XXXXXX)
	stderr_PATH=$(mktemp /tmp/std_XXXXXX)


	export TIMEFORMAT='%3R'
	{ time $@ ; } >$stdout_PATH 2>$stderr_PATH
	if [[ $? != "0" ]]; then
		echo "FAIL!"
		echo "Failed command:" $@
		rm -f $stderr_PATH $stdout_PATH
		return
	fi
	BASE_OUTPUT=$(cat $stdout_PATH)
	BASE_TIME=$(head -n 2 $stderr_PATH | tail -n 1)

	{ time LD_PRELOAD=$MALLOC_SO $@ ; } >$stdout_PATH 2>$stderr_PATH
	if [[ $? != "0" ]]; then
		echo "FAIL!"
		cat $stderr_PATH
		echo "Failed command:" LD_PRELOAD=$MALLOC_SO $@
		rm -f $stderr_PATH $stdout_PATH
		return
	fi
	NEW_OUTPUT=$(cat $stdout_PATH)
	NEW_TIME=$(head -n 2 $stderr_PATH | tail -n 1)

	if [[ "$BASE_OUTPUT" != "$NEW_OUTPUT" ]]; then
		echo "FAIL!"
		echo "Outputs don't match for:" $@ " and " LD_PRELOAD=$MALLOC_SO $@
	else
		echo "PASS!"
		RATIO=$(echo $BASE_TIME / $NEW_TIME | bc -l)
		printf 'System Malloc: %s s, My Malloc: %s s, Speedup (higher better): %.4f\n' "$BASE_TIME" "$NEW_TIME" "$RATIO"

		TOTAL_RATIO=$(echo $TOTAL_RATIO "*" $RATIO | bc -l)	
		((TOTAL_APP++))
	fi


	rm -f $stderr_PATH $stdout_PATH
        
}



# ESPRESSO
if [[ "$TEST_CASE" == "espresso" || "$TEST_CASE" == "" ]]; then
	echo "Running espresso"
	test_bench $TEST_PATH/MallocBench/espresso/espresso -t $TEST_PATH/MallocBench/espresso/INPUT/largest.espresso
fi

# CFRAC
if [[ "$TEST_CASE" == "cfrac" || "$TEST_CASE" == "" ]]; then
	echo "Running cfrac"
	test_bench $TEST_PATH/MallocBench/cfrac/cfrac 41757646344123832613190542166099121
fi

# tsp
if [[ "$TEST_CASE" == "tsp" || "$TEST_CASE" == "" ]]; then
	echo "Running tsp"
	test_bench $TEST_PATH/Olden/tsp/tsp 2048000 4 1
fi

# health
if [[ "$TEST_CASE" == "health" || "$TEST_CASE" == "" ]]; then
	echo "Running health"
	test_bench $TEST_PATH/Olden/health/health 10 40 1
fi

# perimeter
if [[ "$TEST_CASE" == "perimeter" || "$TEST_CASE" == "" ]]; then
	echo "Running perimeter"
	test_bench $TEST_PATH/Olden/perimeter/perimeter 11
fi

# bisort
if [[ "$TEST_CASE" == "bisort" || "$TEST_CASE" == "" ]]; then
	echo "Running bisort"
	test_bench $TEST_PATH/Olden/bisort/bisort 3000000
fi

# voronoi
if [[ "$TEST_CASE" == "voronoi" || "$TEST_CASE" == "" ]]; then
	echo "Running voronoi"
	test_bench $TEST_PATH/Olden/voronoi/voronoi 1000000 20 32 7
fi

# treeadd
if [[ "$TEST_CASE" == "treeadd" || "$TEST_CASE" == "" ]]; then
	echo "Running treeadd"
	test_bench $TEST_PATH/Olden/treeadd/treeadd 22
fi

echo "----------------------------------------"
echo "Total Apps:" $TOTAL_APP
echo "Geomean:" $(awk 'BEGIN { print (ARGV[1] ** (1.0 / ARGV[2])) }' $TOTAL_RATIO $TOTAL_APP)
echo "Final Score:" $(awk 'BEGIN { print (100.0 * (ARGV[1] ** (1.0 / ARGV[2]))) }' $TOTAL_RATIO $TOTAL_APP)


