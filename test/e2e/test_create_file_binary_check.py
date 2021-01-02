from typing import *
import struct


# Keep in sync with module/consts.h
class DdfsConsts:
    CLUSTER_UNUSED = 0
    CLUSTER_NOT_ASSIGNED = 0xfffffffe
    CLUSTER_EOF = 0xffffffff


class DdfsImageReader:
    def __init__(self, ddfs_img_path: str):
        self._ddfs_img_path = ddfs_img_path
        with open(self._ddfs_img_path, 'rb') as f:
            self._raw_data = f.read()

        # For now assume table is one cluster
        # For now assume cluster is one block
        # For now assume block size is 512
        # For now assume table entry size as u32 -> 4 bytes
        self.block_size = 512
        self.table_entry_size = 4

    def table(self) -> List[int]:
        table_size = int(self.block_size / self.table_entry_size)

        raw_table = self._raw_table()
        unpack_format = '{}I'.format(table_size)
        result = struct.unpack(unpack_format, raw_table)
        return list(result)

    def _raw_table(self) -> bytes:
        return self._raw_data[self.block_size:self.block_size + self.block_size]


def run_tests(ddfs_image_path: str):
    image_reader = DdfsImageReader(ddfs_image_path)

    table = image_reader.table()

    # Root cluster
    assert table[0] == DdfsConsts.CLUSTER_EOF

    # All other cluster should be unused because files have no content. They don't occupy any of the clusters
    print(table[1:])
    assert table[1:] == [DdfsConsts.CLUSTER_UNUSED] * \
        (image_reader.block_size - 1)

    return 0


if __name__ == '__main__':
    run_tests()
