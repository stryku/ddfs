#ifndef DDFS_TABLE_H
#define DDFS_TABLE_H

#include "consts.h"
#include "sbi_values.h"

#define DDFS_TABLE_ENTRY_TYPE __u32

struct ddfs_table {
	DDFS_TABLE_ENTRY_TYPE *clusters;
	struct ddfs_block block;
	unsigned index_offset;
};

void ddfs_dump_table(struct ddfs_table *table)
{
	dd_print("ddfs_dump_table %p", table);
	dd_print("\tblock.bh %p", table->block.bh);
	dd_print("\tindex_offset %u", table->index_offset);
}

inline struct ddfs_table ddfs_access_table(block_provider block_providing_fun,
					   void *block_providing_data,
					   const struct ddfs_sbi_values *sbi_v)
{
	const unsigned table_block_no = sbi_v->table_offset / sbi_v->block_size;
	struct ddfs_table result = {
		.index_offset = 0 // Todo: handle bigger tables
	};

	dd_print("ddfs_access_table");

	result.block =
		block_providing_fun(block_providing_data, table_block_no);
	if (result.block.bh != NULL) {
		result.clusters = (DDFS_TABLE_ENTRY_TYPE *)result.block.data;
	} else {
		result.clusters = NULL;
	}

	ddfs_dump_table(&result);

	dd_print("~ddfs_access_table");
	return result;
}

#endif