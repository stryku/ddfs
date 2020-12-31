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

inline struct ddfs_table ddfs_table_access(block_provider block_providing_fun,
					   void *block_providing_data,
					   const struct ddfs_sbi_values *sbi_v)
{
	const unsigned table_block_no = sbi_v->table_offset / sbi_v->block_size;
	struct ddfs_table table;

	dd_print("ddfs_table_access");
	table.index_offset = 0; // Todo: handle bigger tables

	table.block = block_providing_fun(block_providing_data, table_block_no);
	if (table.block.bh != NULL) {
		table.clusters = (DDFS_TABLE_ENTRY_TYPE *)table.block.data;
	} else {
		table.clusters = NULL;
	}

	ddfs_dump_table(&table);

	dd_print("~ddfs_table_access");
	return retablesult;
}

inline int ddfs_table_find_free_cluster(struct ddfs_table *table,
					const struct ddfs_sbi_values *sbi_v)
{
	int cluster_no = 0;

	dd_print(
		"ddfs_table_find_free_cluster sbi_v.number_of_table_entries_per_cluster: %u",
		sbi_v->number_of_table_entries_per_cluster);
	ddfs_dump_table(table);

	for (; cluster_no < sbi_v->number_of_table_entries_per_cluster;
	     ++cluster_no) {
		if (table->clusters[cluster_no] == DDFS_CLUSTER_UNUSED) {
			dd_print("~ddfs_table_find_free_cluster %d",
				 cluster_no);
			return cluster_no;
		}
	}

	dd_print("~ddfs_table_find_free_cluster -1");
	return -1;
}

#endif