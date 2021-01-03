from typing import *
from ddfs_consts import DdfsConsts
from ddfs_image_reader import *


def run_tests(ddfs_image_path: str):
    image_reader = DdfsImageReader(ddfs_image_path)

    table = image_reader.table()

    # Root cluster
    assert table[0] == DdfsConsts.CLUSTER_EOF

    # All other cluster should be unused because files have no content.
    # They don't occupy any of the clusters
    assert table[1:] == [DdfsConsts.CLUSTER_UNUSED] * \
        (image_reader.table_size - 1)

    root_dir = image_reader.root_dir()
    root_entries = image_reader.decode_dir_entries(root_dir)

    print(root_entries[0])
    print(root_entries[1])
    print(root_entries[2])

    assert root_entries[0].name == 'aaa\x00'.encode()
    assert root_entries[0].size == 0
    assert root_entries[0].first_cluster == DdfsConsts.CLUSTER_NOT_ASSIGNED
    assert root_entries[0].attributes == DdfsConsts.FILE_ATTR

    assert root_entries[1].name == 'bbb\x00'.encode()
    assert root_entries[1].size == 0
    assert root_entries[1].first_cluster == DdfsConsts.CLUSTER_NOT_ASSIGNED
    assert root_entries[1].attributes == DdfsConsts.FILE_ATTR

    assert root_entries[2].name == 'ccc\x00'.encode()
    assert root_entries[2].size == 0
    assert root_entries[2].first_cluster == DdfsConsts.CLUSTER_NOT_ASSIGNED
    assert root_entries[2].attributes == DdfsConsts.FILE_ATTR

    return 0


if __name__ == '__main__':
    run_tests()
