#!/bin/bash

echo "[test_dry_run] Test start"

MODULE_NAME=$1
MODULE_PATH=$(readlink -f ../module/$MODULE_NAME.ko)
echo "[test_dry_run] Test module path: $MODULE_PATH"

DIR=$(mktemp -d)
IMG=$(mktemp)

echo "[test_dry_run] DIR: ${DIR} IMG: ${IMG}"


echo "[test_dry_run] Formatting IMG... with block_size=512, sectors_per_cluster=1 and number_of_clusters=4"
printf "\x000002000000000100000004" > $IMG
if [ $? -ne 0 ]; then
    echo "[test_dry_run] FAILED Formatting IMG"
    exit 1
fi
echo "[test_dry_run] Formatting IMG... OK"

echo "[test_dry_run] Appending IMG with zeroes..."
dd if=/dev/zero count=10240 bs=1024 >> $IMG
if [ $? -ne 0 ]; then
    echo "[test_dry_run] FAILED Appending IMG with zeroes"
    exit 1
fi
echo "[test_dry_run] Appending IMG with zeroes..."

echo "[test_dry_run] Insering module..."
insmod $MODULE_PATH
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Insering module with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi
echo "[test_dry_run] Insering module... OK"

echo "[test_dry_run] Mounting ddfs..."
mount -t ddfs -o loop $IMG $DIR
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Mounting ddfs with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi
echo "[test_dry_run] Mounting ddfs... OK"

echo "[test_dry_run] Umounting ddfs..."
umount $DIR
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Umounting ddfs with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi
echo "[test_dry_run] Umounting ddfs... OK"

echo "[test_dry_run] Removing module..."
rmmod $MODULE_NAME
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Removing module with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi
echo "[test_dry_run] Removing module... OK"

echo "[test_dry_run] PASSED"
