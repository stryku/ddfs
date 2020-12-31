#ifndef DDFS_SBI_VALUES_H
#define DDFS_SBI_VALUES_H

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

#endif