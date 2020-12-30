#ifndef DDFS_TABLE_H
#define DDFS_TABLE_H

#include "consts.h"
#include "sbi_values.h"

struct ddfs_table {
	struct ddfs_block block;
	unsigned index_offset;
};

inline struct ddfs_table ddfs_access_table(block_provider block_providing_fun,
					   void *block_providing_data,
					   const struct ddfs_sbi_values *sbi_v)
{
	const unsigned table_block_no = sbi_v->table_offset / sbi_v->block_size;
	struct ddfs_table result = {
		.index_offset = 0 // Todo: handle bigger tables
	};
	result.block =
		block_providing_fun(block_providing_data, table_block_no);

	return result;
}

#endif