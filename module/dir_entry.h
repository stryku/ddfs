#ifndef DDFS_DIR_ENTRY_H
#define DDFS_DIR_ENTRY_H

#include "consts.h"

struct ddfs_dir_entry {
	unsigned entry_index;
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
	unsigned dir_entries_per_cluster;
	unsigned blocks_per_cluster;
	unsigned data_cluster_no;
	unsigned block_size;
	unsigned dir_logical_start;
};

struct dir_entry_ptrs {
	long error;

	struct {
		DDFS_DIR_ENTRY_NAME_TYPE *ptr;
		struct buffer_head *bh;
	} name;

	struct {
		DDFS_DIR_ENTRY_ATTRIBUTES_TYPE *ptr;
		struct buffer_head *bh;
	} attributes;

	struct {
		DDFS_DIR_ENTRY_SIZE_TYPE *ptr;
		struct buffer_head *bh;
	} size;

	struct {
		DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE *ptr;
		struct buffer_head *bh;
	} first_cluster;
};

inline struct dir_entry_part_offsets ddfs_calc_dir_entry_part_offsets(
	const struct ddfs_dir_entry_calc_params *calc_params,
	unsigned entry_index, unsigned entries_offset_on_cluster,
	unsigned entry_part_size)
{
	const unsigned entry_index_on_cluster =
		entry_index % calc_params->dir_entries_per_cluster;

	// Logical cluster no on which the entry lays
	const unsigned entry_logic_cluster_no =
		calc_params->dir_logical_start +
		(entry_index / calc_params->dir_entries_per_cluster);

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
		calc_params->dir_entries_per_cluster *
		DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE;
	const unsigned size_entries_offset =
		attributes_entries_offset +
		calc_params->dir_entries_per_cluster *
			sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE);
	const unsigned first_cluster_entries_offset =
		size_entries_offset + calc_params->dir_entries_per_cluster *
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

inline struct dir_entry_ptrs
ddfs_access_dir_entries(block_provider block_providing_fun,
			void *block_providing_data,
			const struct ddfs_dir_entry_calc_params *calc_params,
			unsigned entry_index, unsigned part_flags)
{
	const struct dir_entry_offsets offsets =
		ddfs_calc_dir_entry_offsets(calc_params, entry_index);
	struct dir_entry_ptrs result;
	char *ptr;

	struct ddfs_block hydra[4];
	unsigned block_no[4] = { 0 };
	unsigned counter = 0;

	struct part_data {
		unsigned flag;
		const struct dir_entry_part_offsets *offsets;
		struct ddfs_block dest_block;
	};

	struct part_data parts_data[] = { { .flag = DDFS_PART_NAME,
					    .offsets = &offsets.name,
					    .dest_block = { .bh = NULL } },
					  { .flag = DDFS_PART_ATTRIBUTES,
					    .offsets = &offsets.attributes,
					    .dest_block = { .bh = NULL } },
					  { .flag = DDFS_PART_SIZE,
					    .offsets = &offsets.size,
					    .dest_block = { .bh = NULL } },
					  { .flag = DDFS_PART_FIRST_CLUSTER,
					    .offsets = &offsets.first_cluster,
					    .dest_block = { .bh = NULL } } };

	unsigned number_of_parts = sizeof(parts_data) / sizeof(parts_data[0]);
	int i;

	dd_print("access_dir_entries");

	for (i = 0; i < number_of_parts; ++i) {
		int used_cached = 0;
		int j;
		struct ddfs_block block;

		if (!(part_flags & parts_data[i].flag)) {
			dd_print("omit flag[%d]: %u", i, parts_data[i].flag);
			continue;
		}

		dd_print("calculate flag[%d]: %u", i, parts_data[i].flag);
		for (j = 0; j < counter; ++j) {
			// char *ptr;

			if (block_no[j] !=
			    parts_data[i].offsets->block_on_device) {
				continue;
			}

			// The same block no as cached one. Reuse it.

			used_cached = 1;
			parts_data[i].dest_block = hydra[j];
			// ptr = (char *)(hydra[j]->b_data) +
			//       data->offsets->offset_on_block;
			// *data->dest_ptr = ptr;
			break;
		}

		if (used_cached) {
			continue;
		}

		// No cached bh. Need to read
		block = block_providing_fun(
			block_providing_data,
			parts_data[i].offsets->block_on_device);
		if (!block.bh) {
			// Todo: handle error
			continue;
		}

		// char *ptr =
		// 	(char *)(bh->b_data) + data->offsets->offset_on_block;
		// *data->dest_ptr = ptr;
		parts_data[i].dest_block = block;

		hydra[counter] = block;
		block_no[counter] = parts_data[i].offsets->block_on_device;
		++counter;
	}

	result.error = 0;

	result.name.bh = parts_data[0].dest_block.bh;
	if (result.name.bh) {
		ptr = parts_data[0].dest_block.data +
		      offsets.name.offset_on_block;
		result.name.ptr = (DDFS_DIR_ENTRY_NAME_TYPE *)ptr;
	}

	result.attributes.bh = parts_data[1].dest_block.bh;
	if (result.attributes.bh) {
		ptr = parts_data[1].dest_block.data +
		      offsets.attributes.offset_on_block;
		result.attributes.ptr = (DDFS_DIR_ENTRY_ATTRIBUTES_TYPE *)ptr;
	}

	result.size.bh = parts_data[2].dest_block.bh;
	if (result.size.bh) {
		ptr = parts_data[2].dest_block.data +
		      offsets.size.offset_on_block;
		result.size.ptr = (DDFS_DIR_ENTRY_SIZE_TYPE *)ptr;
	}

	result.first_cluster.bh = parts_data[3].dest_block.bh;
	if (result.first_cluster.bh) {
		ptr = parts_data[3].dest_block.data +
		      offsets.first_cluster.offset_on_block;
		result.first_cluster.ptr =
			(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE *)ptr;
	}

	// dd_print("access_dir_entries dump 2");
	// dump_dir_entry_ptrs(&result);

	return result;
}

#endif
