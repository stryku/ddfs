#ifndef DDFS_DIR_ENTRY_H
#define DDFS_DIR_ENTRY_H

#include "consts.h"

struct ddfs_dir_entry {
	unsigned entry_index;
#define DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE 4
	DDFS_DIR_ENTRY_NAME_TYPE name[DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE];
	DDFS_DIR_ENTRY_ATTRIBUTES_TYPE attributes;
	DDFS_DIR_ENTRY_SIZE_TYPE size;
	DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE first_cluster;
};

struct dir_entry_part_offsets {
	unsigned block_on_device;
	unsigned offset_on_block;
};

struct dir_entry_offsets {
	unsigned entry_index;

	struct dir_entry_part_offsets name;
	struct dir_entry_part_offsets attributes;
	struct dir_entry_part_offsets size;
	struct dir_entry_part_offsets first_cluster;
};

struct ddfs_dir_entry_calc_params {
	unsigned entries_per_cluster;
	unsigned blocks_per_cluster;
	unsigned data_cluster_no;
	unsigned block_size;
	unsigned dir_logical_start;
};

inline struct dir_entry_part_offsets ddfs_calc_dir_entry_part_offsets(
	const struct ddfs_dir_entry_calc_params *calc_params,
	unsigned entry_index, unsigned entries_offset_on_cluster,
	unsigned entry_part_size)
{
	const unsigned entry_index_on_cluster =
		entry_index % calc_params->entries_per_cluster;

	// Logical cluster no on which the entry lays
	const unsigned entry_logic_cluster_no =
		calc_params->dir_logical_start +
		(entry_index / calc_params->entries_per_cluster);

	// The entry part offset on cluster. In bytes.
	const unsigned offset_on_cluster =
		entries_offset_on_cluster +
		entry_index_on_cluster * entry_part_size;

	// The entry part block on cluster
	const unsigned entry_part_block_no_on_logic_cluster =
		offset_on_cluster / calc_params->block_size;

	// The entry part block no on device.
	const unsigned entry_part_block_no_on_device =
		(calc_params->data_cluster_no + entry_logic_cluster_no) *
			calc_params->blocks_per_cluster +
		entry_part_block_no_on_logic_cluster;

	// The entry part offset on block. In bytes.
	const unsigned entry_part_offset_on_block =
		offset_on_cluster % calc_params->block_size;

	const struct dir_entry_part_offsets result = {
		.block_on_device = entry_part_block_no_on_device,
		.offset_on_block = entry_part_offset_on_block
	};

	return result;
}

struct dir_entry_offsets ddfs_calc_dir_entry_offsets(
	const struct ddfs_dir_entry_calc_params *calc_params,
	unsigned entry_index)
{
	const unsigned name_entries_offset = 0;
	const unsigned attributes_entries_offset =
		calc_params->entries_per_cluster *
		DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE;
	const unsigned size_entries_offset =
		attributes_entries_offset +
		calc_params->entries_per_cluster *
			sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE);
	const unsigned first_cluster_entries_offset =
		size_entries_offset + calc_params->entries_per_cluster *
					      sizeof(DDFS_DIR_ENTRY_SIZE_TYPE);

	const struct dir_entry_offsets result = {
		.entry_index = entry_index,

		.name = ddfs_calc_dir_entry_part_offsets(
			calc_params, entry_index, name_entries_offset,
			sizeof(DDFS_DIR_ENTRY_NAME_TYPE) *
				DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE),

		.attributes = ddfs_calc_dir_entry_part_offsets(
			calc_params, entry_index, attributes_entries_offset,
			sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE)),

		.size = ddfs_calc_dir_entry_part_offsets(
			calc_params, entry_index, size_entries_offset,
			sizeof(DDFS_DIR_ENTRY_SIZE_TYPE)),

		.first_cluster = ddfs_calc_dir_entry_part_offsets(
			calc_params, entry_index, first_cluster_entries_offset,
			sizeof(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE))
	};

	return result;
}

#endif
