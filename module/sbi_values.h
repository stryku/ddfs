#ifndef DDFS_SBI_VALUES_H
#define DDFS_SBI_VALUES_H

#include "boot_sector.h"
#include "consts.h"

// All ddfs super block information values independent of the kernel types
struct ddfs_sbi_values {
	unsigned int combined_dir_entry_parts_size;

	unsigned int table_offset; // From begin of partition
	unsigned int table_size; // In bytes
	unsigned int number_of_table_entries;
	unsigned int number_of_table_entries_per_cluster;

	unsigned int cluster_size; // In bytes

	unsigned int data_offset; // From begin of partition
	unsigned int data_cluster_no; // Data first cluster no

	unsigned int root_cluster; //First cluster of root dir.

	unsigned long block_size; // Size of block (sector) in bytes
	unsigned int blocks_per_cluster; // Number of blocks (sectors) per cluster

	unsigned int entries_per_cluster; // Dir entries per cluster

	unsigned int name_entries_offset;
	unsigned int attributes_entries_offset;
	unsigned int size_entries_offset;
	unsigned int first_cluster_entries_offset;
};

static inline struct ddfs_sbi_values
ddfs_calc_sbi_values(const struct ddfs_boot_sector *bs)
{
	struct ddfs_sbi_values v;

	v.combined_dir_entry_parts_size =
		sizeof(DDFS_DIR_ENTRY_NAME_TYPE) *
			DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
		sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE) +
		sizeof(DDFS_DIR_ENTRY_SIZE_TYPE) +
		sizeof(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE);

	v.blocks_per_cluster = bs->sectors_per_cluster;

	v.cluster_size = bs->sector_size * v.blocks_per_cluster;
	v.number_of_table_entries = bs->number_of_clusters;
	v.number_of_table_entries_per_cluster =
		v.cluster_size / sizeof(DDFS_TABLE_ENTRY_TYPE);
	v.table_offset = v.cluster_size;
	v.table_size = bs->number_of_clusters * sizeof(DDFS_TABLE_ENTRY_TYPE);
	{
		unsigned table_end = v.table_offset + v.table_size;
		unsigned last_table_cluster = table_end / v.cluster_size;
		v.data_cluster_no = last_table_cluster + 1u;
		v.data_offset = v.data_cluster_no * v.cluster_size;
	}
	v.root_cluster = v.data_cluster_no;
	v.block_size = bs->sector_size;

	v.entries_per_cluster =
		v.cluster_size / v.combined_dir_entry_parts_size;

	v.name_entries_offset = 0;
	v.attributes_entries_offset =
		v.entries_per_cluster * DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE;
	v.size_entries_offset =
		v.attributes_entries_offset +
		v.entries_per_cluster * sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE);
	v.first_cluster_entries_offset =
		v.size_entries_offset +
		v.entries_per_cluster * sizeof(DDFS_DIR_ENTRY_SIZE_TYPE);

	return v;
}

#endif