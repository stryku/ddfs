import sys
import subprocess


def test_log(msg):
    print('[DDFS test] ' + msg)


def save_journal(module_name):
    output = subprocess.check_output('journalctl -k', shell=True)
    with open('test_{}.journal'.format(module_name), 'wb') as f:
        f.write(output)
        test_log('Saved journal to: {}'.format(f.name))


def teardown(module_name: str, ddfs_dir: str):

    test_log('Test teardown module path: {}, DDFS_DIR: {}'.format(
        module_name, ddfs_dir))

    test_log('Umounting ddfs...')
    try:
        subprocess.check_call('umount {}'.format(ddfs_dir), shell=True)
    except:
        save_journal(module_name)
        test_log('FAILED Umounting ddfs')
        return 1
    test_log('Umounting ddfs... OK')

    test_log('Removing module...')
    try:
        subprocess.check_call('rmmod {}'.format(module_name), shell=True)
    except:
        save_journal(module_name)
        test_log('FAILED Removing module')
        return 1
    test_log('Removing module... OK')

    save_journal(module_name)

    test_log('Test teardown end')
    return 0


if __name__ == "__main__":
    module_name = sys.argv[1]
    ddfs_dir = sys.argv[2]
    exit(teardown(module_name, ddfs_dir))
