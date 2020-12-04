#!/bin/bash

source test_common.sh

MODULE_NAME=$1

FAILED_TESTS_COUNT=0

for f in *.out
do
    test_log "Testing ${f}..."

    DDFS_DIR=$(mktemp -d)
    DDFS_IMG=$(mktemp)

    ./test_setup.sh $MODULE_NAME $DDFS_DIR $DDFS_IMG
    ec=$?
    if [[ ec -eq 0 ]]
    then
        test_log "Testing ${f}... TEST SETUP SUCCESS"
    else
        test_log "Testing ${f}... FAILED"
        rm -r ${DDFS_DIR}
        rm ${DDFS_IMG}
        FAILED_TESTS_COUNT=$((${FAILED_TESTS_COUNT}+1))
        continue
    fi

    ./${f}
    ec=$?
    if [[ ec -eq 0 ]]
    then
        test_log "Testing ${f}... PASSED"
    else
        test_log "Testing ${f}... FAILED"
        FAILED_TESTS_COUNT=$((${FAILED_TESTS_COUNT}+1))
    fi

    ./test_teardown.sh $MODULE_NAME $DDFS_DIR
    ec=$?
    if [[ ec -eq 0 ]]
    then
        test_log "Testing ${f}... TEST TEARDOWN SUCCESS"
    else
        test_log "Testing ${f}... FAILED"
        FAILED_TESTS_COUNT=$((${FAILED_TESTS_COUNT}+1))
    fi

    rm -r ${DDFS_DIR}
    rm ${DDFS_IMG}
done

if [[ ${FAILED_TESTS_COUNT} -eq 0 ]]
then
    test_log "ALL TESTS PASSED"
else
    test_log "SOME TESTS FAILED"
fi
