import sys
import tempfile
import os
import subprocess
import glob
import test_setup
import test_teardown
import shutil


def test_log(msg):
    print('[DDFS test] ' + msg)


def save_journal():
    output = subprocess.check_output('journalctl -k', shell=True)
    with open('test.journal', 'wb') as f:
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

    try:
        subprocess.run('./{} {}'.format(
            test_binary_file, ddfs_dir), shell=True, check=True)
        log('PASSED')
    except:
        log('FAILED')
        FAILED_TESTS_COUNT += 1

    ret = test_teardown.teardown(module_name=MODULE_NAME, ddfs_dir=ddfs_dir)
    if ret == 0:
        log('TEST TEARDOWN SUCCESS')
    else:
        log('TEST TEARDOWN FAILED')
        FAILED_TESTS_COUNT += 1

    shutil.rmtree(ddfs_dir, ignore_errors=True)
    os.remove(ddfs_img)


if FAILED_TESTS_COUNT == 0:
    test_log('ALL E2E TESTS PASSED')
else:
    test_log('SOME E2E TESTS FAILED')
    exit(1)
