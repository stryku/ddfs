from typing import *
import struct


# Keep in sync with module/consts.h
class DdfsConsts:
    CLUSTER_UNUSED = 0
    CLUSTER_NOT_ASSIGNED = 0xfffffffe
    CLUSTER_EOF = 0xffffffff

    FILE_ATTR = 1
    DIR_ATTR = 2


class DdfsDirEntry:
    def __init__(self, name: bytes, attributes: int, size: int, first_cluster: int):
        self.name = name
        self.attributes = attributes
        self.size = size
        self.first_cluster = first_cluster

    def __str__(self):
        return 'name: {}, attr: {}, size: {}, first_cluster: {}'.format(self.name, self.attributes, self.size, self.first_cluster)

    def __eq__(self, other):
        return str(self) == str(other)


class DdfsImageReader:
    def __init__(self, ddfs_img_path: str):
        print('Reading ddfs image: {}'.format(ddfs_img_path))
        self._ddfs_img_path = ddfs_img_path
        with open(self._ddfs_img_path, 'rb') as f:
            self._raw_data = f.read()

        # For now assume table is one cluster
        # For now assume cluster is one block
        # For now assume block size is 512
        # For now assume table entry size as u32 -> 4 bytes
        self.block_size = 512
        self.cluster_size = self.block_size
        self.table_entry_size = 4
        self.table_size = int(self.block_size / self.table_entry_size)
        # For now assume dir entry type sizes: name, attributes, size, first cluster
        self.dir_entries_per_cluster = int(self.cluster_size / (4 + 1 + 8 + 4))

    def table(self) -> List[int]:
        raw_table = self._raw_table()
        unpack_format = '{}I'.format(self.table_size)
        result = struct.unpack(unpack_format, raw_table)
        return list(result)

    def root_dir(self) -> bytes:
        # For now assume it occupies one cluster
        offset = self.block_size * 2
        return self._raw_data[offset:offset + self.block_size]

    def decode_dir_entries(self, cluster: bytes) -> List[DdfsDirEntry]:
        result = list()

        name_offset = 0
        attributes_offset = self.dir_entries_per_cluster * 4
        size_offset = self.dir_entries_per_cluster * (4+1)
        first_cluster_offset = self.dir_entries_per_cluster * (4 + 1 + 8)

        print('{}, {}, {}, {}'.format(name_offset,
                                      attributes_offset, size_offset, first_cluster_offset))

        for i in range(self.dir_entries_per_cluster):
            name = struct.unpack('4c', cluster[name_offset:name_offset + 4])
            name = b''.join(name)
            name_offset += 4

            attributes = struct.unpack(
                'B', cluster[attributes_offset:attributes_offset + 1])[0]
            attributes_offset += 1

            size = struct.unpack('Q', cluster[size_offset:size_offset + 8])[0]
            size_offset += 8

            first_cluster = struct.unpack(
                'I', cluster[first_cluster_offset:first_cluster_offset + 4])[0]
            first_cluster_offset += 4

            entry = DdfsDirEntry(name, attributes, size, first_cluster)

            result.append(entry)

        return result

    def _raw_table(self) -> bytes:
        return self._raw_data[self.block_size:self.block_size + self.block_size]


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

    assert root_entries[0].name == 'aaa\x00'.encode()
    assert root_entries[0].size == 0
    assert root_entries[0].first_cluster == DdfsConsts.CLUSTER_NOT_ASSIGNED
    assert root_entries[0].attributes == DdfsConsts.FILE_ATTR

    return 0


if __name__ == '__main__':
    run_tests()
