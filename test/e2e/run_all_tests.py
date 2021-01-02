import sys
import tempfile
import os
import subprocess
import glob
import test_setup
import test_teardown
import shutil
import importlib


def test_log(msg):
    print('[DDFS test] ' + msg)


def save_journal():
    global MODULE_NAME
    output = subprocess.check_output('journalctl -k', shell=True)
    with open('test_{}.journal'.format(MODULE_NAME), 'wb') as f:
        f.write(output)
        test_log('Saved journal to: {}'.format(f.name))


MODULE_NAME = sys.argv[1]
FAILED_TESTS_COUNT = 0

for test_binary_file in glob.glob('*.out'):
    def log(msg):
        test_log('Testing {}... {}'.format(test_binary_file, msg))

    log('')

    ddfs_dir = tempfile.mkdtemp()
    ddfs_img = tempfile.mktemp()

    ret = test_setup.setup(module_name=MODULE_NAME,
                           ddfs_dir=ddfs_dir, ddfs_img=ddfs_img)
    if ret == 0:
        log('TEST SETUP SUCCESS')
    else:
        log('TEST SETUP FAILED')
        FAILED_TESTS_COUNT += 1
        os.remove(ddfs_dir)
        os.remove(ddfs_img)
        continue

    passed = True

    try:
        log('Running test...')
        subprocess.run('./{} {}'.format(
            test_binary_file, ddfs_dir), shell=True, check=True)
        log('Running test... OK')

        log('Syncing filesystem...')
        subprocess.check_call('sync', shell=True)
        log('Syncing filesystem... OK')

        log('PASSED')
    except:
        log('FAILED')
        FAILED_TESTS_COUNT += 1
        passed = False

    ret = test_teardown.teardown(module_name=MODULE_NAME, ddfs_dir=ddfs_dir)
    if ret == 0:
        log('TEST TEARDOWN SUCCESS')
    else:
        log('TEST TEARDOWN FAILED')
        FAILED_TESTS_COUNT += 1

    if not passed:
        log('Test failed - not looking for binary check tests')
    else:
        binary_check_module_name = '{}_binary_check'.format(
            os.path.splitext(test_binary_file)[0])
        binary_check_module_path = '{}.py'.format(binary_check_module_name)

        log('Test passed - looking for binary check tests: {}'.format(binary_check_module_path))

        if not os.path.exists(binary_check_module_path):
            log('Binary check module not found')
        else:
            log('Running binary check...')
            module = importlib.import_module(binary_check_module_name)
            try:
                run_tests = getattr(module, 'run_tests')
                ret = run_tests(ddfs_img)
            except:
                ret = 1
                passed = False

            if ret == 0:
                log('Running binary check... PASSED')
            else:
                log('Running binary check... FAILED')
                FAILED_TESTS_COUNT += 1

    # Clean up only if passed
    if passed:
        shutil.rmtree(ddfs_dir, ignore_errors=True)
        os.remove(ddfs_img)


if FAILED_TESTS_COUNT == 0:
    test_log('ALL E2E TESTS PASSED')
else:
    test_log('SOME E2E TESTS FAILED')
    exit(1)
