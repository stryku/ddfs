#!/bin/bash

source ../test_common.sh

FAILED_TESTS_COUNT=0

for f in *.out
do
    test_log "Testing ${f}..."

    ./${f}
    ec=$?
    if [[ ec -eq 0 ]]
    then
        test_log "Testing ${f}... PASSED"
    else
        test_log "Testing ${f}... FAILED"
        FAILED_TESTS_COUNT=$((${FAILED_TESTS_COUNT}+1))
    fi
done

if [[ ${FAILED_TESTS_COUNT} -eq 0 ]]
then
    test_log "ALL UNIT TESTS PASSED"
else
    test_log "SOME UNIT TESTS FAILED"
    exit 1
fi
