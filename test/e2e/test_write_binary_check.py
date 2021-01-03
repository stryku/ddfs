from typing import *
from ddfs_consts import DdfsConsts
from ddfs_image_reader import *


def run_tests(ddfs_image_path: str):
    image_reader = DdfsImageReader(ddfs_image_path)

    table = image_reader.table()

    # Root cluster
    assert table[0] == DdfsConsts.CLUSTER_EOF

    # aaa, bbb and ccc files
    assert table[1] == DdfsConsts.CLUSTER_EOF
    assert table[2] == DdfsConsts.CLUSTER_EOF
    assert table[3] == DdfsConsts.CLUSTER_EOF

    # All other cluster should be unused because files have no content.
    # They don't occupy any of the clusters
    rest_of_table = table[4:]
    assert rest_of_table == [DdfsConsts.CLUSTER_UNUSED] * len(rest_of_table)

    # Test root dir entries
    root_dir = image_reader.root_dir()
    root_entries = image_reader.decode_dir_entries(root_dir)

    print(root_entries[0])
    aaa_content = 'aaa content'.encode()
    assert root_entries[0].name == 'aaa\x00'.encode()
    assert root_entries[0].size == len(aaa_content)
    assert root_entries[0].first_cluster == 1
    assert root_entries[0].attributes == DdfsConsts.FILE_ATTR

    aaa_cluster = image_reader.read_cluster(1)
    assert aaa_cluster[:len(aaa_content)] == aaa_content

    print(root_entries[1])
    bbb_content = 'bbb content'.encode()
    assert root_entries[1].name == 'bbb\x00'.encode()
    assert root_entries[1].size == len(aaa_content)
    assert root_entries[1].first_cluster == 2
    assert root_entries[1].attributes == DdfsConsts.FILE_ATTR

    bbb_cluster = image_reader.read_cluster(2)
    assert bbb_cluster[:len(bbb_content)] == bbb_content

    print(root_entries[2])
    ccc_content = 'ccc content'.encode()
    assert root_entries[2].name == 'ccc\x00'.encode()
    assert root_entries[2].size == len(ccc_content)
    assert root_entries[2].first_cluster == 3
    assert root_entries[2].attributes == DdfsConsts.FILE_ATTR

    ccc_cluster = image_reader.read_cluster(3)
    assert ccc_cluster[:len(ccc_content)] == ccc_content

    return 0


if __name__ == '__main__':
    run_tests()
