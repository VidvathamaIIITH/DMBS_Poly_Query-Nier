#!/bin/bash
# PolyQuery Graph Test Runner
# Runs all generated test cases and reports results

set -e

cd "$(dirname "$0")"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test definitions (name:expected_pattern)
declare -A TESTS
TESTS["T01_LoadDirected"]="Loaded Graph"
TESTS["T02_LoadUndirected"]="Loaded Graph"
TESTS["T03_LoadSingleNode"]="Node Count:1"
TESTS["T04_LoadEmptyEdges"]="Edge Count:0"
TESTS["T05_LoadDuplicateName"]="SEMANTIC ERROR"
TESTS["T06_PrintDirected"]="NodeID"
TESTS["T07_PrintUndirected"]="U"
TESTS["T08_PrintNonExistent"]="SEMANTIC ERROR"
TESTS["T09_DegreeDirectedCenter"]="4"
TESTS["T10_DegreeDirectedLeaf"]="1"
TESTS["T11_DegreeUndirected"]="2"
TESTS["T12_DegreeIsolatedNode"]="0"
TESTS["T13_DegreeNonExistentNode"]="Node does not exist"
TESTS["T14_PathExists"]="TRUE"
TESTS["T15_PathNoPath"]="FALSE"
TESTS["T16_PathSameNode"]="TRUE"
TESTS["T17_PathWithCondition"]="TRUE"
TESTS["T18_PathNonExistentSrc"]="FALSE"
TESTS["T19_PathNonExistentDst"]="FALSE"
TESTS["T20_ExportGraph"]=""
TESTS["T21_ExportNonExistent"]="SEMANTIC ERROR"
TESTS["T22_CompleteGraph"]="8"
TESTS["T23_LargeLinear"]="TRUE"
TESTS["T24_UndirectedComplete"]="3"
TESTS["T25_MultiConditionPath"]="TRUE"

# Results tracking
PASSED=0
FAILED=0
CRASHED=0
RESULTS_FILE="../data/test_results.txt"

echo "PolyQuery Graph Test Suite" > "$RESULTS_FILE"
echo "=========================" >> "$RESULTS_FILE"
echo "Test run: $(date)" >> "$RESULTS_FILE"
echo -e "${YELLOW}PolyQuery Graph Test Suite${NC}"
echo "========================="
echo ""

# Run tests
echo "Compiling server..."
    END_TIME=$(date +%s%N)
    
    ELAPSED_MS=$(( (END_TIME - START_TIME) / 1000000 ))
    
    # Check for crash
    if [ "${EXIT_CODE:-0}" -eq 124 ]; then
        echo -e "${RED}TIMEOUT${NC} $TEST_NAME (>${ELAPSED_MS}ms)"
        echo "TIMEOUT $TEST_NAME (>30s)" >> "$RESULTS_FILE"
        ((CRASHED++))
        continue
    fi
    
    if [ "${EXIT_CODE:-0}" -ne 0 ] && [ "${EXIT_CODE:-0}" -ne 124 ]; then
        if [[ "$OUTPUT" == *"Segmentation fault"* ]] || [[ "$OUTPUT" == *"Aborted"* ]]; then
            echo -e "${RED}CRASH${NC} $TEST_NAME - ${OUTPUT##*$'\n'}"
            echo "CRASH $TEST_NAME - Segfault/Abort" >> "$RESULTS_FILE"
            echo "  Output: $OUTPUT" >> "$RESULTS_FILE"
            ((CRASHED++))
            continue
        fi
    fi
    
    # Check expected pattern
    if [ -z "$EXPECTED" ]; then
        # No specific pattern expected, just check no crash
        echo -e "${GREEN}PASS${NC} $TEST_NAME (${ELAPSED_MS}ms) - No crash"
        echo "PASS $TEST_NAME (${ELAPSED_MS}ms)" >> "$RESULTS_FILE"
        ((PASSED++))
    elif echo "$OUTPUT" | grep -q "$EXPECTED"; then
        echo -e "${GREEN}PASS${NC} $TEST_NAME (${ELAPSED_MS}ms)"
        echo "PASS $TEST_NAME (${ELAPSED_MS}ms)" >> "$RESULTS_FILE"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC} $TEST_NAME - Expected '$EXPECTED'"
        echo "FAIL $TEST_NAME - Expected '$EXPECTED'" >> "$RESULTS_FILE"
        echo "  Output: $OUTPUT" >> "$RESULTS_FILE"
        ((FAILED++))
    fi
done

echo ""
echo "========================="
echo -e "Results: ${GREEN}${PASSED} passed${NC}, ${RED}${FAILED} failed${NC}, ${RED}${CRASHED} crashed${NC}"
echo ""
echo "Results: $PASSED passed, $FAILED failed, $CRASHED crashed" >> "$RESULTS_FILE"

# Summary
echo "" >> "$RESULTS_FILE"
echo "Total: $((PASSED + FAILED + CRASHED)) tests" >> "$RESULTS_FILE"

if [ $FAILED -eq 0 ] && [ $CRASHED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. Check $RESULTS_FILE for details.${NC}"
    exit 1
fi
