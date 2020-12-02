#!/bin/bash

source test_common.sh

test_log "Test setup start"

MODULE_NAME=$1
DDFS_DIR=$2
DDFS_IMG=$3
MODULE_PATH=$(readlink -f ../module/$MODULE_NAME.ko)

test_log "Test setup module path: $MODULE_PATH, DDFS_DIR: ${DDFS_DIR}, DDFS_IMG: ${DDFS_IMG}"


test_log "Formatting DDFS_IMG... with block_size=512, sectors_per_cluster=1 and number_of_clusters=4"
printf "\x00\x02\x00\x00\x01\x00\x00\x00\x04\x00\x00\x00" > $DDFS_IMG
if [ $? -ne 0 ]; then
    test_log "FAILED Formatting DDFS_IMG"
    exit 1
fi
test_log "Formatting DDFS_IMG... OK"

test_log "Appending DDFS_IMG with zeroes..."
dd if=/dev/zero count=10240 bs=1024 >> $DDFS_IMG
if [ $? -ne 0 ]; then
    test_log "FAILED Appending DDFS_IMG with zeroes"
    exit 1
fi
test_log "Appending DDFS_IMG with zeroes... OK"

test_log "Insering module..."
insmod $MODULE_PATH
ec=$?
if [ $ec -ne 0 ]; then
    save_journal
    test_log "FAILED Insering module with ec: ${ec}."
    exit 1
fi
test_log "Insering module... OK"

test_log "Mounting ddfs..."
mount -t $MODULE_NAME -o loop $DDFS_IMG $DDFS_DIR
ec=$?
if [ $ec -ne 0 ]; then
    save_journal
    test_log "FAILED Mounting ddfs with ec: ${ec}."
    exit 1
fi
test_log "Mounting ddfs... OK"

test_log "Test setup end"
