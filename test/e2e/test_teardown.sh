#!/bin/bash

source test_common.sh

test_log "Test teardown start"

MODULE_NAME=$1
DDFS_DIR=$2

test_log "Test teardown module path: $MODULE_NAME, DDFS_DIR: ${DDFS_DIR}"

test_log "Umounting ddfs..."
umount $DDFS_DIR
ec=$?
if [ $ec -ne 0 ]; then
    save_journal
    test_log "FAILED Umounting ddfs with ec: ${ec}."
    exit 1
fi
test_log "Umounting ddfs... OK"

test_log "Removing module..."
rmmod $MODULE_NAME
ec=$?
if [ $ec -ne 0 ]; then
    save_journal
    test_log "FAILED Removing module with ec: ${ec}."
    exit 1
fi
test_log "Removing module... OK"

save_journal

test_log "Test teardown end"
