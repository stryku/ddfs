import sys
import os
import subprocess


def test_log(msg):
    print('[DDFS test] ' + msg)


def save_journal():
    output = subprocess.check_output('journalctl -k', shell=True)
    with open('test.journal', 'wb') as f:
        f.write(output)
        test_log('Saved journal to: {}'.format(f.name))


def setup(module_name: str, ddfs_dir: str, ddfs_img: str):
    module_path = os.path.abspath('../../module/{}.ko'.format(module_name))

    test_log('Test setup module path: {}, DDFS_DIR: {}, DDFS_IMG: {}'.format(
        module_path, ddfs_dir, ddfs_img))

    test_log(
        'Creating DDFS_IMG... with block_size=512, sectors_per_cluster=1 and number_of_clusters=4')
    with open(ddfs_img, 'w') as f:
        f.write('\x00\x02\x00\x00\x01\x00\x00\x00\x04\x00\x00\x00')
        f.write('\x00' * 1024 * 1024 * 10)
    test_log('Creating DDFS_IMG... OK')

    test_log('Inserting module...')
    try:
        subprocess.check_call('insmod {}'.format(module_path), shell=True)
    except:
        save_journal()
        test_log('FAILED Inserting module')
        return 1
    test_log('Inserting module... OK')

    test_log('Mounting ddfs...')
    try:
        subprocess.check_call(
            'mount -t {} -o loop {} {}'.format(module_name, ddfs_img, ddfs_dir), shell=True)
    except:
        save_journal()
        test_log('FAILED Mounting ddfs')
        return 1
    test_log('Mounting ddfs... OK')

    test_log('Test setup end')
    return 0


if __name__ == '__main__':
    module_name = sys.argv[1]
    ddfs_dir = sys.argv[2]
    ddfs_img = sys.argv[3]
    exit(setup(module_name, ddfs_dir, ddfs_img))
