#ifndef DDFS_H
#define DDFS_H

#include <linux/buffer_head.h>
#include <linux/exportfs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/namei.h>
#include <linux/stat.h>
#include <linux/kernel.h>
#include <linux/iversion.h>
#include <linux/writeback.h>

#include "dir_entry.h"

/*
 * DDFS inode data in memory
 */
struct ddfs_inode_info {
	unsigned dentry_index; //
	unsigned number_of_entries;

	// fat stuff:
	spinlock_t cache_lru_lock;
	struct list_head cache_lru;
	int nr_caches;
	/* for avoiding the race between fat_free() and fat_get_cluster() */
	unsigned int cache_valid_id;

	/* NOTE: mmu_private is 64bits, so must hold ->i_mutex to access */
	loff_t mmu_private; /* physically allocated size */

	int i_start; /* first cluster or 0 */
	int i_logstart; /* logical first cluster */
	int i_attrs; /* unused attribute bits */
	loff_t i_pos; /* on-disk position of directory entry or 0 */
	struct hlist_node i_fat_hash; /* hash by i_location */
	struct hlist_node i_dir_hash; /* hash by i_logstart */
	struct rw_semaphore truncate_lock; /* protect bmap against truncate */
	struct inode ddfs_inode; // Todo: Should be named vfs_inode
};

inline void dump_ddfs_inode_info(struct ddfs_inode_info *info)
{
	dd_print("dump_ddfs_inode_info, info: %p", info);
	dd_print("\t\tinfo->dentry_index: %u", info->dentry_index);
	dd_print("\t\tinfo->number_of_entries: %u", info->number_of_entries);
	dd_print("\t\tinfo->i_start: %d", info->i_start);
	dd_print("\t\tinfo->i_logstart: %d", info->i_logstart);
	dd_print("\t\tinfo->i_attrs: %d", info->i_attrs);
	dd_print("\t\tinfo->i_pos: %llu", info->i_pos);
}

static inline void dump_ddfs_dir_entry(const struct ddfs_dir_entry *entry)
{
	dd_print("dump_ddfs_dir_entry: %p", entry);

	dd_print("\t\tentry->entry_index %u", entry->entry_index);
	dd_print("\t\tentry->name %s", entry->name);
	dd_print("\t\tentry->attributes %u", (unsigned)entry->attributes);
	dd_print("\t\tentry->size %llu", entry->size);
	dd_print("\t\tentry->first_cluster %u", (unsigned)entry->first_cluster);
}

static inline struct ddfs_inode_info *DDFS_I(struct inode *inode)
{
	return container_of(inode, struct ddfs_inode_info, ddfs_inode);
}

struct ddfs_sb_info {
	unsigned int table_offset; // From begin of partition
	unsigned int table_size; // In bytes
	unsigned int number_of_table_entries;

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

	struct mutex table_lock;
	struct mutex s_lock;
	struct mutex build_inode_lock;

	const void *dir_ops; /* Opaque; default directory operations */
};

static inline struct ddfs_sb_info *DDFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

inline struct ddfs_dir_entry_calc_params
ddfs_make_dir_entry_calc_params(struct super_block *sb, struct inode *dir)
{
	const struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct ddfs_inode_info *dd_dir = DDFS_I(dir);
	const struct ddfs_dir_entry_calc_params calc_params = {
		.entries_per_cluster = sbi->entries_per_cluster,
		.blocks_per_cluster = sbi->blocks_per_cluster,
		.data_cluster_no = sbi->data_cluster_no,
		.block_size = sb->s_blocksize,
		.dir_logical_start = dd_dir->i_logstart
	};

	return calc_params;
}

void dump_dir_entry_offsets(struct dir_entry_offsets *offsets)
{
	dd_print("dump_dir_entry_offsets: %p", offsets);

	dd_print("\t\toffsets->name.block_on_device: %u",
		 offsets->name.block_on_device);
	dd_print("\t\toffsets->name.offset_on_block: %u",
		 offsets->name.offset_on_block);

	dd_print("\t\toffsets->attributes.block_on_device: %u",
		 offsets->attributes.block_on_device);
	dd_print("\t\toffsets->attributes.offset_on_block: %u",
		 offsets->attributes.offset_on_block);

	dd_print("\t\toffsets->size.block_on_device: %u",
		 offsets->size.block_on_device);
	dd_print("\t\toffsets->size.offset_on_block: %u",
		 offsets->size.offset_on_block);

	dd_print("\t\toffsets->first_cluster.block_on_device: %u",
		 offsets->first_cluster.block_on_device);
	dd_print("\t\toffsets->first_cluster.offset_on_block: %u",
		 offsets->first_cluster.offset_on_block);
}

void dump_dir_entry_ptrs(const struct dir_entry_ptrs *ptrs)
{
	dd_print("dump_dir_entry_ptrs: %px", ptrs);
	dd_print("\t\tptrs->error: %ld", ptrs->error);
	dd_print("\t\tptrs->name.ptr: %px", ptrs->name.ptr);
	dd_print("\t\tptrs->name.bh: %px", ptrs->name.bh);

	dd_print("\t\tptrs->attributes.ptr: %px", ptrs->attributes.ptr);
	dd_print("\t\tptrs->attributes.bh: %px", ptrs->attributes.bh);

	dd_print("\t\tptrs->size.ptr: %px", ptrs->size.ptr);
	dd_print("\t\tptrs->size.bh: %px", ptrs->size.bh);

	dd_print("\t\tptrs->first_cluster.ptr: %px", ptrs->first_cluster.ptr);
	dd_print("\t\tptrs->first_cluster.bh: %px", ptrs->first_cluster.bh);
}

// static inline struct dir_entry_ptrs
// access_dir_entries(struct inode *dir, unsigned entry_index, unsigned part_flags)
// {
// 	struct super_block *sb = dir->i_sb;
// 	const struct ddfs_dir_entry_calc_params calc_params =
// 		ddfs_make_dir_entry_calc_params(sb, dir);
// 	const struct dir_entry_offsets offsets =
// 		ddfs_calc_dir_entry_offsets(&calc_params, entry_index);
// 	struct dir_entry_ptrs result;
// 	unsigned char *ptr;

// 	struct buffer_head *hydra[4] = { NULL };
// 	unsigned block_no[4] = { 0 };
// 	unsigned counter = 0;

// 	struct part_data {
// 		unsigned flag;
// 		const struct dir_entry_part_offsets *offsets;
// 		struct buffer_head *dest_bh;
// 	};

// 	struct part_data parts_data[] = { { .flag = DDFS_PART_NAME,
// 					    .offsets = &offsets.name,
// 					    .dest_bh = ((void *)0) },
// 					  { .flag = DDFS_PART_ATTRIBUTES,
// 					    .offsets = &offsets.attributes,
// 					    .dest_bh = ((void *)0) },
// 					  { .flag = DDFS_PART_SIZE,
// 					    .offsets = &offsets.size,
// 					    .dest_bh = ((void *)0) },
// 					  { .flag = DDFS_PART_FIRST_CLUSTER,
// 					    .offsets = &offsets.first_cluster,
// 					    .dest_bh = ((void *)0) } };

// 	unsigned number_of_parts = sizeof(parts_data) / sizeof(parts_data[0]);
// 	int i;

// 	dd_print("access_dir_entries");

// 	for (i = 0; i < number_of_parts; ++i) {
// 		int used_cached = 0;
// 		int j;
// 		struct buffer_head *bh;

// 		if (!(part_flags & parts_data[i].flag)) {
// 			dd_print("omit flag[%d]: %u", i, parts_data[i].flag);
// 			parts_data[i].dest_bh = ((void *)0);
// 			dd_print("parts_data[%d].dest_bh = %p", i,
// 				 parts_data[i].dest_bh);
// 			continue;
// 		}

// 		dd_print("calculate flag[%d]: %u", i, parts_data[i].flag);
// 		for (j = 0; j < counter; ++j) {
// 			// char *ptr;

// 			if (block_no[j] !=
// 			    parts_data[i].offsets->block_on_device) {
// 				continue;
// 			}

// 			// The same block no as cached one. Reuse it.

// 			used_cached = 1;
// 			parts_data[i].dest_bh = hydra[j];
// 			// ptr = (char *)(hydra[j]->b_data) +
// 			//       data->offsets->offset_on_block;
// 			// *data->dest_ptr = ptr;
// 			break;
// 		}

// 		if (used_cached) {
// 			continue;
// 		}

// 		// No cached bh. Need to read
// 		bh = sb_bread(sb, parts_data[i].offsets->block_on_device);
// 		if (!bh) {
// 			parts_data[i].dest_bh = NULL;
// 			continue;
// 		}

// 		// char *ptr =
// 		// 	(char *)(bh->b_data) + data->offsets->offset_on_block;
// 		// *data->dest_ptr = ptr;
// 		parts_data[i].dest_bh = bh;

// 		hydra[counter] = bh;
// 		block_no[counter] = parts_data[i].offsets->block_on_device;
// 		++counter;
// 	}

// 	result.error = 0;

// 	result.name.bh = parts_data[0].dest_bh;
// 	if (result.name.bh) {
// 		result.name.bh = parts_data[0].dest_bh;
// 		ptr = result.name.bh->b_data + offsets.name.offset_on_block;
// 		result.name.ptr = (DDFS_DIR_ENTRY_NAME_TYPE *)ptr;
// 	}

// 	result.attributes.bh = parts_data[1].dest_bh;
// 	if (result.attributes.bh) {
// 		result.attributes.bh = parts_data[1].dest_bh;
// 		ptr = result.attributes.bh->b_data +
// 		      offsets.attributes.offset_on_block;
// 		result.attributes.ptr = (DDFS_DIR_ENTRY_ATTRIBUTES_TYPE *)ptr;
// 	}

// 	result.size.bh = parts_data[2].dest_bh;
// 	if (result.size.bh) {
// 		ptr = result.size.bh->b_data + offsets.size.offset_on_block;
// 		result.size.ptr = (DDFS_DIR_ENTRY_SIZE_TYPE *)ptr;
// 	}

// 	result.first_cluster.bh = parts_data[3].dest_bh;
// 	if (result.first_cluster.bh) {
// 		ptr = result.first_cluster.bh->b_data +
// 		      offsets.first_cluster.offset_on_block;
// 		result.first_cluster.ptr =
// 			(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE *)ptr;
// 	}

// 	dd_print("access_dir_entries: dir: %p, entry_index: %d, part_flags: %u",
// 		 dir, entry_index, part_flags);

// 	dd_print("access_dir_entries dump 2");
// 	dump_dir_entry_ptrs(&result);

// 	return result;
// }

static inline void release_dir_entries(const struct dir_entry_ptrs *ptrs,
				       unsigned part_flags)
{
	struct buffer_head *hydra[4];
	unsigned counter = 0;
	int already_freed = 0;
	int i;

	dd_print("release_dir_entries: ptrs: %p, part_flags: %u", ptrs,
		 part_flags);
	dump_dir_entry_ptrs(ptrs);

	if (part_flags & DDFS_PART_NAME && ptrs->name.bh) {
		brelse(ptrs->name.bh);
		hydra[counter++] = ptrs->name.bh;
	}

	if (part_flags & DDFS_PART_ATTRIBUTES && ptrs->attributes.bh) {
		already_freed = 0;
		for (i = 0; i < counter; ++i) {
			if (hydra[i] == ptrs->attributes.bh) {
				already_freed = 1;
				break;
			}
		}
		if (!already_freed) {
			brelse(ptrs->attributes.bh);
			hydra[counter++] = ptrs->attributes.bh;
		}
	}

	if (part_flags & DDFS_PART_SIZE && ptrs->size.bh) {
		already_freed = 0;
		for (i = 0; i < counter; ++i) {
			if (hydra[i] == ptrs->size.bh) {
				already_freed = 1;
				break;
			}
		}
		if (!already_freed) {
			brelse(ptrs->size.bh);
			hydra[counter++] = ptrs->size.bh;
		}
	}

	if (part_flags & DDFS_PART_FIRST_CLUSTER && ptrs->first_cluster.bh) {
		already_freed = 0;
		for (i = 0; i < counter; ++i) {
			if (hydra[i] == ptrs->first_cluster.bh) {
				already_freed = 1;
				break;
			}
		}
		if (!already_freed) {
			brelse(ptrs->first_cluster.bh);
			hydra[counter++] = ptrs->first_cluster.bh;
		}
	}
}

#endif
