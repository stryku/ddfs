#!/bin/bash

echo "[test_dry_run] Test start"

DIR=$(mktemp -d)
IMG=$(mktemp)

echo "[test_dry_run] DIR: ${DIR} IMG: ${IMG}"

echo "[test_dry_run] Creating IMG"
dd if=/dev/zero of=$IMG count=10240 bs=1024
if [ $? -ne 0 ]; then
    echo "[test_dry_run] FAILED Creating IMG"
    exit 1
fi

echo "[test_dry_run] Formatting IMG with block_size=512, sectors_per_cluster=1 and number_of_clusters=4"
printf "\x000002000000000100000004" > $IMG
if [ $? -ne 0 ]; then
    echo "[test_dry_run] FAILED Formatting IMG"
    exit 1
fi

echo "[test_dry_run] Insering module"
insmod ../module/ddfs.ko
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Insering module with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi

echo "[test_dry_run] Mounting ddfs"
mount -t ddfs -o loop $IMG $DIR
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Mounting ddfs with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi

echo "[test_dry_run] Umounting ddfs"
umount $DIR
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Umounting ddfs with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi

echo "[test_dry_run] Removing module"
rmmod ddfs
ec=$?
if [ $ec -ne 0 ]; then
    journalctl -k > test_dry_run.journal
    echo "[test_dry_run] FAILED Removing module with ec: ${ec}. Saved journal to $(readlink -f test_dry_run.journal)"
    exit 1
fi

echo "[test_dry_run] PASSED"
