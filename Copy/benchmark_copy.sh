#!/bin/bash

#arugments tests
if [ $# -ne 3 ]; then
    echo "not enough arguments"
    exit 1
fi
#arguments assignment
INPUT_FILE=$1
OUTPUT_BASE=$2
BUFFER_LIST=$3

#checking if copy exists
if [ ! -f ./copy ]; then 
    echo "copy not found"
    exit 1
fi

#checking if input file exists and less than 100MB
if [ ! -f $INPUT_FILE ]; then
    echo "input file does not exist"
    exit 1
fi

INPUT_SIZE=$(stat -c%s "$INPUT_FILE")
if [ $INPUT_SIZE -lt 104857600 ]; then
    echo "input file's size is smaller than 100MB"
    exit 1
fi

echo "buffer size,time (seconds)" > results.csv

#value assignment for tests
IFS=',' read -ra SIZES <<< "$BUFFER_LIST"
TOTAL_TESTS=${#SIZES[@]}
PASSED_COUNT=0
TEST_NUMBER=1

#loop
for size in ${SIZES[@]}; do
    echo "Clearing Cache..."
    sync; echo 3 | sudo tee /proc/sys/vm/drop_caches
    OUTPUT_FILE="${OUTPUT_BASE}_${size}.tmp"
    echo "[ $TEST_NUMBER/$TOTAL_TESTS ] Running copy for buffer size $size..."
    time_sec=$( { /usr/bin/time -f "%e" ./copy "$INPUT_FILE" "$OUTPUT_FILE" "$size" ; } 2>&1 )
    echo $?
    if [ $? -ne 0 ]; then
        echo "Error on test $size"
        continue
    fi
    PASSED_COUNT=$((PASSED_COUNT + 1))
    echo "Time: $time_sec seconds"
    echo "$size,$time_sec" >> results.csv
    rm -f $OUTPUT_FILE
    TEST_NUMBER=$((TEST_NUMBER + 1))
done

echo "Done"
echo "Summary:"
echo ""
printf "%-15s %-15s\n" "Buffer Size" "Time (seconds)"
awk -F"," 'NR > 1 {printf "%-15s %-15s\n", $1, $2}' results.csv
echo "Tests completed: $PASSED_COUNT/$TOTAL_TESTS"
