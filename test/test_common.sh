#!/bin/bash

function test_log {
    echo "[DDFS test] $1"
}

function save_journal {
    journalctl -k > test_dry_run.journal
    test_log "Saved journal to $(readlink -f test_dry_run.journal)"
}
