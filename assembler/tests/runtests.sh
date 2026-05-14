#!/bin/bash

# Simple test runner for UF2S8 assembler tests

VALGRIND=$1
TEST_DIR=$(dirname "$0")
FAILED=0
TOTAL=0

echo "Running tests..."

for test_exe in "$TEST_DIR"/test_*; do
    # Skip source files and other scripts
    if [[ "$test_exe" == *.c ]] || [[ "$test_exe" == *.sh ]] || [[ "$test_exe" == *.log ]]; then
        continue
    fi

    if [[ -x "$test_exe" ]]; then
        TOTAL=$((TOTAL + 1))
        echo -n "Running $(basename "$test_exe")... "
        
        if [ -n "$VALGRIND" ]; then
            $VALGRIND --leak-check=full --error-exitcode=1 "$test_exe" > /dev/null 2>&1
        else
            "$test_exe" > /dev/null 2>&1
        fi

        if [ $? -eq 0 ]; then
            echo "PASSED"
        else
            echo "FAILED"
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo "------------------------------"
echo "Total: $TOTAL, Passed: $((TOTAL - FAILED)), Failed: $FAILED"

if [ $FAILED -ne 0 ]; then
    exit 1
fi

exit 0
