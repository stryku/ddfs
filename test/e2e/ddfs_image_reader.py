import struct
from typing import *


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
        self.first_data_cluster = 2
        # For now assume dir entry type sizes: name, attributes, size, first cluster
        self.dir_entries_per_cluster = int(self.cluster_size / (4 + 1 + 8 + 4))

    def table(self) -> List[int]:
        raw_table = self._raw_table()
        unpack_format = '{}I'.format(self.table_size)
        result = struct.unpack(unpack_format, raw_table)
        return list(result)

    def read_cluster(self, logical_cluster_no: int) -> bytes:
        offset = (logical_cluster_no + self.first_data_cluster) * \
            self.cluster_size
        return self._raw_data[offset:offset + self.cluster_size]

    def root_dir(self) -> bytes:
        # For now assume it occupies one cluster
        return self.read_cluster(0)

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
