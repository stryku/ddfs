#include "ddfs.h"
#include "table.h"

static inline struct ddfs_block
ddfs_default_block_reading_provider(void *data, unsigned block_no)
{
	struct buffer_head *bh;
	struct super_block *sb = (struct super_block *)data;
	struct ddfs_block result;

	bh = sb_bread(sb, block_no);
	if (!bh) {
		result.bh = NULL;
	} else {
		result.bh = bh;
		result.data = bh->b_data;
	}

	return result;
}

static inline struct ddfs_dir_entry_calc_params
ddfs_make_dir_entry_calc_params(struct inode *dir)
{
	const struct ddfs_inode_info *dd_dir = DDFS_I(dir);
	struct super_block *sb = dir->i_sb;
	const struct ddfs_sb_info *sbi = DDFS_SB(sb);

	const struct ddfs_dir_entry_calc_params result = {
		.entries_per_cluster = sbi->v.entries_per_cluster,
		.blocks_per_cluster = sbi->v.blocks_per_cluster,
		.data_cluster_no = sbi->v.data_cluster_no,
		.block_size = sb->s_blocksize,
		.dir_logical_start = dd_dir->i_logstart
	};

	return result;
}

static inline void lock_table(struct ddfs_sb_info *sbi)
{
	dd_print("lock_table");
	mutex_lock(&sbi->table_lock);
	dd_print("~lock_table");
}

static inline void unlock_table(struct ddfs_sb_info *sbi)
{
	dd_print("unlock_table");
	mutex_unlock(&sbi->table_lock);
	dd_print("~unlock_table");
}

static inline void lock_data(struct ddfs_sb_info *sbi)
{
	dd_print("lock_data");
	mutex_lock(&sbi->s_lock);
	dd_print("~lock_data");
}

static inline void unlock_data(struct ddfs_sb_info *sbi)
{
	dd_print("unlock_data");
	mutex_unlock(&sbi->s_lock);
	dd_print("~unlock_data");
}

static inline void lock_inode_build(struct ddfs_sb_info *sbi)
{
	dd_print("lock_inode_build");
	mutex_lock(&sbi->build_inode_lock);
	dd_print("~lock_inode_build");
}

static inline void unlock_inode_build(struct ddfs_sb_info *sbi)
{
	dd_print("unlock_inode_build");
	mutex_unlock(&sbi->build_inode_lock);
	dd_print("~unlock_inode_build");
}

static struct kmem_cache *ddfs_inode_cachep;

static struct inode *ddfs_alloc_inode(struct super_block *sb)
{
	struct ddfs_inode_info *ei;
	dd_print("ddfs_alloc_inode");
	dd_print("calling kmem_cache_alloc");
	ei = kmem_cache_alloc(ddfs_inode_cachep, GFP_NOFS);
	if (!ei) {
		dd_print("kmem_cache_alloc failed");
		dd_print("~ddfs_alloc_inode NULL");
		return NULL;
	}

	dd_print("kmem_cache_alloc succeed");

	dd_print("calling init_rwsem");
	init_rwsem(&ei->truncate_lock);

	dd_print("~ddfs_alloc_inode %p", &ei->ddfs_inode);
	return &ei->ddfs_inode;
}

static void ddfs_free_inode(struct inode *inode)
{
	kmem_cache_free(ddfs_inode_cachep, DDFS_I(inode));
}

static inline void ddfs_get_blknr_offset(struct ddfs_sb_info *sbi, loff_t i_pos,
					 sector_t *blknr, int *offset)
{
	*blknr = i_pos / sbi->v.block_size;
	*offset = i_pos % sbi->v.block_size;
}

static int __ddfs_write_inode(struct inode *inode, int wait)
{
	struct super_block *sb = inode->i_sb;
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct ddfs_inode_info *dd_inode = DDFS_I(inode);

	dd_print("__ddfs_write_inode: inode: %p, wait: %d", inode, wait);
	dump_ddfs_inode_info(dd_inode);

	dd_print("locking data");
	lock_data(sbi);

	// Todo check whether entry index is inside cluster

	{
		const unsigned part_flags = DDFS_PART_ALL;

		const struct ddfs_dir_entry_calc_params calc_params =
			ddfs_make_dir_entry_calc_params(inode);

		struct dir_entry_ptrs entry_ptrs = ddfs_access_dir_entries(
			ddfs_default_block_reading_provider, sb, &calc_params,
			dd_inode->dentry_index, part_flags);

		// *entry_ptrs.first_cluster.ptr = dd_inode->i_logstart;
		// *entry_ptrs.size.ptr = inode->i_size;
		// *entry_ptrs.attributes.ptr = DDFS_FILE_ATTR;

		// mark_buffer_dirty(entry_ptrs.first_cluster.bh);
		// mark_buffer_dirty(entry_ptrs.size.bh);
		// mark_buffer_dirty(entry_ptrs.attributes.bh);
		// if (wait) {
		// 	// Todo: handle failures
		// 	sync_dirty_buffer(entry_ptrs.first_cluster.bh);
		// 	sync_dirty_buffer(entry_ptrs.size.bh);
		// 	sync_dirty_buffer(entry_ptrs.attributes.bh);
		// }

		ddfs_release_dir_entries(&entry_ptrs, part_flags);
	}

	dd_print("calling unlock_data");
	unlock_data(sbi);

	dd_print("~__ddfs_write_inode 0");
	return 0;
}

static int ddfs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
	return __ddfs_write_inode(inode, wbc->sync_mode == WB_SYNC_ALL);
}

int ddfs_sync_inode(struct inode *inode)
{
	return __ddfs_write_inode(inode, 1);
}

/* Free all clusters after the skip'th cluster. */
static int ddfs_free(struct inode *inode, int skip)
{
	struct super_block *sb = inode->i_sb;
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	int err, wait, free_start, i_start, i_logstart;
	struct ddfs_inode_info *dd_inode = DDFS_I(inode);
	struct buffer_head *bh;

	if (dd_inode->i_start == 0)
		return 0;

	// fat_cache_inval_inode(inode);

	wait = IS_DIRSYNC(inode);
	i_start = free_start = dd_inode->i_start;
	i_logstart = dd_inode->i_logstart;

	/* First, we write the new file size. */
	if (!skip) {
		dd_inode->i_start = 0;
		dd_inode->i_logstart = 0;
	}
	// dd_inode->i_attrs |= ATTR_ARCH;
	// fat_truncate_time(inode, NULL, S_CTIME | S_MTIME);
	if (wait) {
		err = ddfs_sync_inode(inode);
		if (err) {
			dd_inode->i_start = i_start;
			dd_inode->i_logstart = i_logstart;
			return err;
		}
	} else {
		mark_inode_dirty(inode);
	}

	inode->i_blocks = 0;

	{
		// Index of cluster entry in table.
		const unsigned cluster_no = dd_inode->i_logstart;
		// How many cluster indices fit in one cluster, in table.
		const unsigned cluster_idx_per_cluster =
			sbi->v.cluster_size / 4u;
		// Cluster of table on which `cluster_no` lays.
		const unsigned table_cluster_no_containing_cluster_no =
			cluster_no / cluster_idx_per_cluster;
		// Index of block on the cluster, on which `cluster_no` lays.
		const unsigned block_no_containing_cluster_no =
			(cluster_no % cluster_idx_per_cluster) /
			sbi->v.blocks_per_cluster;
		// Index of block on device, on which `cluster_no` lays.
		// Calculated:
		//    1 cluster for boot sector * blocks_per_cluster
		//    + table_cluster_no_containing_cluster_no * blocks_per_cluster
		//    +  block_no_containing_cluster_no
		const unsigned device_block_no_containing_cluster_no =
			sbi->v.blocks_per_cluster +
			table_cluster_no_containing_cluster_no *
				sbi->v.blocks_per_cluster +
			block_no_containing_cluster_no;
		// Cluster index on the block
		const unsigned cluster_idx_on_block =
			cluster_no % (sb->s_blocksize / 4u);

		DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE *cluster_index_ptr;

		lock_table(sbi);

		// Read the block
		bh = sb_bread(sb, device_block_no_containing_cluster_no);
		if (!bh) {
			dd_error("unable to read inode block to free ");
			return -EIO;
		}

		cluster_index_ptr =
			(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE *)(bh->b_data) +
			cluster_idx_on_block;

		// Finally, write cluster as unused:
		*cluster_index_ptr = DDFS_CLUSTER_UNUSED;

		brelse(bh);

		unlock_table(sbi);
	}

	return 0;
}

static int writeback_inode(struct inode *inode)
{
	int ret;

	/* if we used wait=1, sync_inode_metadata waits for the io for the
	* inode to finish.  So wait=0 is sent down to sync_inode_metadata
	* and filemap_fdatawrite is used for the data blocks
	*/
	ret = sync_inode_metadata(inode, 0);
	if (!ret)
		ret = filemap_fdatawrite(inode->i_mapping);
	return ret;
}

int ddfs_flush_inodes(struct super_block *sb, struct inode *i1,
		      struct inode *i2)
{
	int ret = 0;
	if (i1)
		ret = writeback_inode(i1);
	if (!ret && i2)
		ret = writeback_inode(i2);
	if (!ret) {
		struct address_space *mapping = sb->s_bdev->bd_inode->i_mapping;
		ret = filemap_flush(mapping);
	}
	return ret;
}

void ddfs_truncate_blocks(struct inode *inode, loff_t offset)
{
	ddfs_free(inode, 0);
	ddfs_flush_inodes(inode->i_sb, inode, NULL);
}

static void ddfs_evict_inode(struct inode *inode)
{
	truncate_inode_pages_final(&inode->i_data);

	ddfs_truncate_blocks(inode, 0);

	invalidate_inode_buffers(inode);
	clear_inode(inode);
	// fat_cache_inval_inode(inode);
	// fat_detach(inode);
}

static void ddfs_put_super(struct super_block *sb)
{
	// Todo: put table inode
}

static int ddfs_remount(struct super_block *sb, int *flags, char *data)
{
	sync_filesystem(sb);
	return 0;
}

// static int ddfs_show_options(struct seq_file *m, struct dentry *root);
static const struct super_operations ddfs_sops = {
	.alloc_inode = ddfs_alloc_inode,
	.free_inode = ddfs_free_inode,
	.write_inode = ddfs_write_inode,
	.evict_inode = ddfs_evict_inode,
	.put_super = ddfs_put_super,
	// .statfs = ddfs_statfs,
	.remount_fs = ddfs_remount,

	// .show_options = ddfs_show_options,
};

const struct export_operations ddfs_export_ops = {
	// .fh_to_dentry = ddfs_fh_to_dentry,
	// .fh_to_parent = ddfs_fh_to_parent,
	// .get_parent = ddfs_get_parent,
};

static struct ddfs_dir_entry
ddfs_make_dir_entry(const struct dir_entry_ptrs *parts_ptrs)
{
	struct ddfs_dir_entry result;
	if (parts_ptrs->name.bh) {
		memcpy(result.name, parts_ptrs->name.ptr, 4);
	}
	if (parts_ptrs->size.bh) {
		result.size = *parts_ptrs->size.ptr;
	}
	if (parts_ptrs->attributes.bh) {
		result.attributes = *parts_ptrs->attributes.ptr;
	}
	if (parts_ptrs->first_cluster.bh) {
		result.first_cluster = *parts_ptrs->first_cluster.ptr;
	}

	return result;
}

static long ddfs_add_dir_entry(struct inode *dir, const struct qstr *qname,
			       struct ddfs_dir_entry *de)
{
	int i;
	struct ddfs_inode_info *dd_idir = DDFS_I(dir);
	struct super_block *sb = dir->i_sb;
	// Todo: handle no space on cluster

	const unsigned new_entry_index = dd_idir->number_of_entries;

	const struct ddfs_dir_entry_calc_params calc_params =
		ddfs_make_dir_entry_calc_params(dir);

	const struct dir_entry_ptrs parts_ptrs =
		ddfs_access_dir_entries(ddfs_default_block_reading_provider, sb,
					&calc_params, new_entry_index,
					DDFS_PART_ALL);

	dd_print("ddfs_add_dir_entry, dir: %p, name: %s, de: %p", dir,
		 (const char *)qname->name, de);
	dump_dir_entry_ptrs(&parts_ptrs);

	++dd_idir->number_of_entries;

	// Set name
	if (!parts_ptrs.name.bh) {
		dd_error("unable to read inode block for name");
		goto fail_io;
	}

	dd_print("assigning name");
	for (i = 0; i < DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE; ++i) {
		parts_ptrs.name.ptr[i] = qname->name[i];
		if (!qname->name[i]) {
			break;
		}
	}

	dd_print("calling mark_buffer_dirty_inode");
	mark_buffer_dirty(parts_ptrs.name.bh);
	sync_dirty_buffer(parts_ptrs.name.bh);
	dd_print("calling mark_inode_dirty");
	inode_inc_iversion(dir);

	// Set first cluster
	if (!parts_ptrs.first_cluster.bh) {
		dd_error("unable to read inode block for first_cluster");
		goto fail_io;
	}

	*parts_ptrs.first_cluster.ptr = DDFS_CLUSTER_NOT_ASSIGNED;
	mark_buffer_dirty(parts_ptrs.first_cluster.bh);
	sync_dirty_buffer(parts_ptrs.first_cluster.bh);

	*parts_ptrs.size.ptr = 0;
	mark_buffer_dirty(parts_ptrs.size.bh);
	sync_dirty_buffer(parts_ptrs.size.bh);

	*parts_ptrs.attributes.ptr = DDFS_FILE_ATTR;
	mark_buffer_dirty(parts_ptrs.attributes.bh);
	sync_dirty_buffer(parts_ptrs.attributes.bh);

	inode_inc_iversion(dir);
	mark_inode_dirty(dir);

	*de = ddfs_make_dir_entry(&parts_ptrs);
	// de->attributes = DDFS_FILE_ATTR;
	// de->size = 0;
	de->entry_index = new_entry_index;

	ddfs_release_dir_entries(&parts_ptrs, DDFS_PART_ALL);

	dd_print("~ddfs_add_dir_entry 0");
	return 0;

fail_io:
	--dd_idir->number_of_entries;
	ddfs_release_dir_entries(&parts_ptrs, DDFS_PART_ALL);

	dd_print("~ddfs_add_dir_entry error: %d", -EIO);
	return -EIO;
}

int ddfs_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = d_inode(dentry);
	dd_print("ddfs_setattr, dentry: %p, iattr: %p", dentry, attr);
	dump_ddfs_inode_info(DDFS_I(inode));

	dd_print("~ddfs_setattr 0");
	return 0;
}

int ddfs_getattr(const struct path *path, struct kstat *stat, u32 request_mask,
		 unsigned int flags)
{
	struct inode *inode = d_inode(path->dentry);

	dd_print("ddfs_getattr, inode: %p, request_mask: %x, flags: %u", inode,
		 request_mask, flags);
	dump_ddfs_inode_info(DDFS_I(inode));

	generic_fillattr(inode, stat);
	stat->blksize = DDFS_SB(inode->i_sb)->v.cluster_size;

	dd_print("stat->mode: %x", stat->mode);
	dd_print("S_ISDIR(stat->mode): %d", S_ISDIR(stat->mode));

	dd_print("~ddfs_getattr 0");
	return 0;
}

int ddfs_update_time(struct inode *inode, struct timespec64 *now, int flags)
{
	dd_print("ddfs_update_time inode: %p, timespec: %p, flags: %d", inode,
		 now, flags);
	dump_ddfs_inode_info(DDFS_I(inode));

	dd_print("~ddfs_update_time 0");
	return 0;
}

const struct inode_operations ddfs_file_inode_operations = {
	.setattr = ddfs_setattr,
	.getattr = ddfs_getattr,
	.update_time = ddfs_update_time,
};

ssize_t ddfs_read(struct file *file, char __user *buf, size_t size,
		  loff_t *ppos)
{
	struct inode *inode = d_inode(file->f_path.dentry);
	struct ddfs_inode_info *dd_inode = DDFS_I(inode);
	struct super_block *sb = inode->i_sb;
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct buffer_head *bh;
	unsigned cluster_no = dd_inode->i_logstart + sbi->v.data_cluster_no;
	unsigned block_on_device = cluster_no * sbi->v.blocks_per_cluster;
	char *data_ptr;

	dd_print("ddfs_read, file: %p, size: %lu, ppos: %llu", file, size,
		 *ppos);

	dump_ddfs_inode_info(dd_inode);

	lock_data(sbi);

	bh = sb_bread(sb, block_on_device);
	if (!bh) {
		dd_print("sb_read failed");
		//todo: handle
	}
	dd_print("sb_read succeed");

	data_ptr = (char *)bh->b_data;
	data_ptr += *ppos;

	memcpy(buf, data_ptr, size);

	brelse(bh);
	unlock_data(sbi);

	return size;
}

static int ddfs_take_first_root_cluster_if_not_taken(struct ddfs_sb_info *sbi)
{
	struct super_block *sb = sbi->sb;
	struct ddfs_table table;

	dd_print("ddfs_take_first_root_cluster_if_not_taken");

	lock_table(sbi);

	table = ddfs_table_access(ddfs_default_block_reading_provider, sb,
				  &sbi->v);
	if (!table.clusters) {
		dd_print("failed accessing table");
		// Todo: handle
	}
	dd_print("accessing table succeed");

	if (table.clusters[0] == DDFS_CLUSTER_UNUSED) {
		dd_print("Setting first root cluster to %d", DDFS_CLUSTER_EOF);
		table.clusters[0] = DDFS_CLUSTER_EOF;
		mark_buffer_dirty(table.block.bh);
	}

	brelse(table.block.bh);
	unlock_table(sbi);

	dd_print("~ddfs_take_first_root_cluster_if_not_taken 0");
	return 0;
}

int ddfs_find_free_cluster(struct super_block *sb)
{
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct ddfs_table table;
	int cluster_no = 0;

	dd_print("ddfs_find_free_cluster");

	lock_table(sbi);

	table = ddfs_table_access(ddfs_default_block_reading_provider, sb,
				  &sbi->v);
	if (!table.clusters) {
		dd_print("failed accessing table");
		// Todo: handle
	}
	dd_print("accessing table succeed");

	cluster_no = ddfs_table_find_free_cluster(&table, &sbi->v);
	if (cluster_no == -1) {
		// Todo handle
		dd_print("No free cluster");
	}

	dd_print("found cluster %d", cluster_no);
	table.clusters[cluster_no] = DDFS_CLUSTER_EOF;

	mark_buffer_dirty(table.block.bh);
	brelse(table.block.bh);
	unlock_table(sbi);

	dd_print("~ddfs_find_free_cluster %d", cluster_no);
	return cluster_no;
}

static ssize_t ddfs_write(struct file *file, const char __user *u, size_t count,
			  loff_t *ppos)
{
	struct inode *inode = d_inode(file->f_path.dentry);
	struct ddfs_inode_info *dd_inode = DDFS_I(inode);
	struct super_block *sb = inode->i_sb;
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct buffer_head *bh;
	char *dest;
	unsigned cluster_on_device;
	unsigned block_on_device;
	int cluster_no = dd_inode->i_logstart;

	dd_print("ddfs_write, file: %p, size: %lu, ppos: %llu", file, count,
		 *ppos);

	dd_print("inode: %p", inode);
	dump_ddfs_inode_info(dd_inode);

	if (cluster_no == DDFS_CLUSTER_NOT_ASSIGNED) {
		dd_print("no cluster, need to search for a free one");
		cluster_no = ddfs_find_free_cluster(inode->i_sb);
		// Todo: handle cluster_no == -1 which means no free cluster available
		dd_inode->i_logstart = cluster_no;
		dd_inode->i_start = dd_inode->i_logstart + 3;

		inode_inc_iversion(inode);
		mark_inode_dirty(inode);
	}

	dd_print("cluster_no to use: %d", cluster_no);

	cluster_on_device = cluster_no + sbi->v.data_cluster_no;
	block_on_device = cluster_on_device * sbi->v.blocks_per_cluster;

	dd_print("cluster_on_device: %u", cluster_on_device);
	dd_print("block_on_device: %u", block_on_device);

	dd_print("calling lock_data");
	lock_data(sbi);

	bh = sb_bread(sb, block_on_device);
	if (!bh) {
		dd_print("sb_bread failed");
		//Todo: handle
	}
	dd_print("sb_bread succeed");

	dest = (char *)bh->b_data;
	dest += *ppos;

	dd_print("writing data to buffer");
	memcpy(dest, u, count);
	dd_print("writing done");

	dd_print("calling mark_buffer_dirty_inode");
	inode->i_size = *ppos + count;
	inode_inc_iversion(inode);
	mark_inode_dirty(inode);
	mark_buffer_dirty(bh);
	sync_dirty_buffer(bh);

	dd_print("updating dir entry to parent's directory entry");
	{
		struct inode *parent_dir_inode =
			d_inode(file->f_path.dentry->d_parent);

		const struct ddfs_dir_entry_calc_params calc_params =
			ddfs_make_dir_entry_calc_params(parent_dir_inode);

		const unsigned part_flags =
			DDFS_PART_FIRST_CLUSTER | DDFS_PART_SIZE;
		const struct dir_entry_ptrs entry_ptrs =
			ddfs_access_dir_entries(
				ddfs_default_block_reading_provider, sb,
				&calc_params, dd_inode->dentry_index,
				part_flags);

		// Todo: handle nulls
		*entry_ptrs.first_cluster.ptr = cluster_no;
		mark_buffer_dirty(entry_ptrs.first_cluster.bh);
		*entry_ptrs.size.ptr = inode->i_size;
		mark_buffer_dirty(entry_ptrs.size.bh);

		ddfs_release_dir_entries(&entry_ptrs, part_flags);

		inode_inc_iversion(parent_dir_inode);
		mark_inode_dirty(parent_dir_inode);
	}

	dd_print("calling brelse");
	brelse(bh);
	dd_print("calling unlock_data");
	unlock_data(sbi);
	ddfs_sync_inode(inode);

	dd_print("~ddfs_write %lu", count);
	return count;
}

int ddfs_file_fsync(struct file *filp, loff_t start, loff_t end, int datasync)
{
	struct inode *inode = filp->f_mapping->host;
	int err;

	err = __generic_file_fsync(filp, start, end, datasync);
	if (err)
		return err;

	return blkdev_issue_flush(inode->i_sb->s_bdev, GFP_KERNEL, NULL);
}

const struct file_operations ddfs_file_operations = {
	// Todo: fill
	.read = ddfs_read,
	.write = ddfs_write,
	// 	.llseek = generic_file_llseek,
	// 	.read_iter = generic_file_read_iter,
	// 	.write_iter = generic_file_write_iter,
	// 	.mmap = generic_file_mmap,
	// 	.release = fat_file_release,
	// 	.unlocked_ioctl = fat_generic_ioctl,
	// #ifdef CONFIG_COMPAT
	// 	.compat_ioctl = fat_generic_compat_ioctl,
	// #endif
	.fsync = ddfs_file_fsync,
	// 	.splice_read = generic_file_splice_read,
	// 	.splice_write = iter_file_splice_write,
	// 	.fallocate = fat_fallocate,
};

static const struct address_space_operations ddfs_aops = {
	// Todo: fill
	// .readpage = ddfs_readpage,
	// .readpages = fat_readpages,
	// .writepage = ddfs_writepage,
	// .writepages = fat_writepages,
	// .write_begin = fat_write_begin,
	// .write_end = fat_write_end,
	// .direct_IO = fat_direct_IO,
	// .bmap = _fat_bmap
};

/* doesn't deal with root inode */
int ddfs_fill_inode(struct inode *inode, struct ddfs_dir_entry *de)
{
	struct ddfs_inode_info *dd_inode = DDFS_I(inode);

	dd_print("ddfs_fill_inode: inode: %p, de: %p", inode, de);
	dump_ddfs_inode_info(dd_inode);
	dump_ddfs_dir_entry(de);

	dd_inode->i_pos = 0;
	inode_inc_iversion(inode);
	inode->i_generation = get_seconds();

	// Todo: Handle directory filling

	dd_inode->i_start = de->first_cluster;

	dd_inode->i_logstart = dd_inode->i_start;
	inode->i_size = le32_to_cpu(de->size);
	inode->i_op = &ddfs_file_inode_operations;
	inode->i_fop = &ddfs_file_operations;
	inode->i_mapping->a_ops = &ddfs_aops;
	dd_inode->mmu_private = inode->i_size;

	dd_inode->i_attrs = de->attributes;
	inode->i_blocks = inode->i_size / inode->i_sb->s_blocksize;

	dd_print("filled inode");
	dump_ddfs_inode_info(dd_inode);

	dd_print("~ddfs_fill_inode 0");
	return 0;
}

struct inode *ddfs_build_inode(struct super_block *sb,
			       struct ddfs_dir_entry *de)
{
	struct inode *inode;
	int err;

	lock_inode_build(DDFS_SB(sb));

	dd_print("ddfs_build_inode, de: ");
	dump_ddfs_dir_entry(de);

	dd_print("calling new_inode");
	inode = new_inode(sb);
	if (!inode) {
		dd_print("new_inode call failed");
		inode = ERR_PTR(-ENOMEM);
		goto out;
	}
	dd_print("new_inode call succeed");

	inode->i_ino = iunique(sb, 128); // todo: 128 is probably not needed
	inode_set_iversion(inode, 1);

	dd_print("calling ddfs_fill_inode");
	err = ddfs_fill_inode(inode, de);
	if (err) {
		dd_print("ddfs_fill_inode call failed");
		iput(inode);
		inode = ERR_PTR(err);
		goto out;
	}

	dd_print("ddfs_fill_inode call succeed");

	// fat_attach(inode, i_pos);
	insert_inode_hash(inode);

out:
	unlock_inode_build(DDFS_SB(sb));
	dd_print("~ddfs_build_inode, inode: %p", inode);
	return inode;
}

static int ddfs_create(struct inode *dir, struct dentry *dentry, umode_t mode,
		       bool excl)
{
	struct super_block *sb = dir->i_sb;
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct inode *inode;
	int err;
	struct ddfs_dir_entry de;

	dd_print("ddfs_create, inode: %p, dentry: %p, mode: %u, excl: %d", dir,
		 dentry, mode, (int)excl);
	dump_ddfs_inode_info(DDFS_I(dir));

	lock_data(sbi);

	dd_print("calling ddfs_add_dir_entry");
	err = ddfs_add_dir_entry(dir, &dentry->d_name, &de);
	if (err) {
		dd_print("ddfs_add_dir_entry failed with err: %d", err);
		goto out;
	}
	dd_print("ddfs_add_dir_entry succeed");

	inode_inc_iversion(dir);
	mark_inode_dirty(dir);
	inode_inc_iversion(dir);

	dd_print("calling ddfs_build_inode");
	inode = ddfs_build_inode(sb, &de);
	if (IS_ERR(inode)) {
		dd_print("ddfs_build_inode call failed");
		err = PTR_ERR(inode);
		goto out;
	}
	dd_print("ddfs_build_inode call succeed");

	inode_inc_iversion(inode);
	mark_inode_dirty(inode);

	dd_print("calling d_instantiate");
	d_instantiate(dentry, inode);

out:
	unlock_data(sbi);
	dd_print("~ddfs_create %d", err);
	return err;
}

static int ddfs_find(struct inode *dir, const char *name,
		     struct ddfs_dir_entry *dest_de)
{
	int entry_index;
	struct ddfs_inode_info *dd_dir = DDFS_I(dir);
	struct super_block *sb = dir->i_sb;

	dd_print("ddfs_find, dir: %p, name: %s, dest_de: %p", dir, name,
		 dest_de);

	dump_ddfs_inode_info(dd_dir);

	for (entry_index = 0; entry_index < dd_dir->number_of_entries;
	     ++entry_index) {
		int i;

		const struct ddfs_dir_entry_calc_params calc_params =
			ddfs_make_dir_entry_calc_params(dir);

		const struct dir_entry_ptrs entry_ptrs =
			ddfs_access_dir_entries(
				ddfs_default_block_reading_provider, sb,
				&calc_params, entry_index, DDFS_PART_ALL);

		dd_print("entry_index: %d", entry_index);
		dump_dir_entry_ptrs(&entry_ptrs);

		for (i = 0; i < 4; ++i) {
			if (entry_ptrs.name.ptr[i] != name[i]) {
				break;
			}
			if (entry_ptrs.name.ptr[i] == '\0') {
				dd_print("found entry at: %d", entry_index);

				memcpy(dest_de->name, entry_ptrs.name.ptr,
				       i + 1);
				dest_de->entry_index = entry_index;
				dest_de->size = *entry_ptrs.size.ptr;
				dest_de->first_cluster =
					*entry_ptrs.first_cluster.ptr;
				dest_de->attributes =
					*entry_ptrs.attributes.ptr;

				ddfs_release_dir_entries(&entry_ptrs,
							 DDFS_PART_ALL);

				dd_print("~ddfs_find 0");
				return 0;
			}

			if (!entry_ptrs.name.ptr[i] || !name[i]) {
				break;
			}
		}

		ddfs_release_dir_entries(&entry_ptrs, DDFS_PART_NAME);
	}

	dd_print("~ddfs_find %d", -ENOENT);
	return -ENOENT;
}

static struct dentry *ddfs_lookup(struct inode *dir, struct dentry *dentry,
				  unsigned int flags)
{
	struct super_block *sb = dir->i_sb;
	struct ddfs_sb_info *sbi = DDFS_SB(sb);
	struct ddfs_dir_entry de;
	struct inode *inode;
	struct dentry *alias;
	int err;

	dd_print("ddfs_lookup: dir: %p, dentry: %p, flags: %u", dir, dentry,
		 flags);

	lock_data(sbi);
	dd_print("ddfs_lookup locked");

	dump_ddfs_inode_info(DDFS_I(dir));

	err = ddfs_find(dir, (const char *)(dentry->d_name.name), &de);
	if (err) {
		if (err == -ENOENT) {
			inode = NULL;
			goto out;
		}
		goto error;
	}

	inode = ddfs_build_inode(sb, &de);
	if (IS_ERR(inode)) {
		err = PTR_ERR(inode);
		goto error;
	}

	alias = d_find_alias(inode);

	if (alias && alias->d_parent == dentry->d_parent) {
		if (!S_ISDIR(inode->i_mode)) {
			d_move(alias, dentry);
		}
		iput(inode);
		unlock_data(sbi);
		return alias;
	} else {
		dput(alias);
	}

out:
	unlock_data(sbi);
	dd_print("~ddfs_lookup, inode: %p", inode);
	return d_splice_alias(inode, dentry);
error:
	unlock_data(sbi);
	dd_print("~ddfs_lookup error: %p", ERR_PTR(err));
	return ERR_PTR(err);
}

static int ddfs_unlink(struct inode *dir, struct dentry *dentry)
{
	dd_print("ddfs_unlink: dir: %p, dentry: %p", dir, dentry);
	dump_ddfs_inode_info(DDFS_I(dir));

	dd_print("~ddfs_unlink %d", -EINVAL);
	return -EINVAL;
}

static int ddfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	dd_print("ddfs_mkdir: dir: %p, dentry: %p, mode: %u", dir, dentry,
		 mode);
	dump_ddfs_inode_info(DDFS_I(dir));

	dd_print("~ddfs_mkdir %d", -EINVAL);
	return -EINVAL;
}

static int ddfs_rmdir(struct inode *dir, struct dentry *dentry)
{
	dd_print("ddfs_rmdir: dir: %p, dentry: %p", dir, dentry);
	dump_ddfs_inode_info(DDFS_I(dir));
	dd_print("~ddfs_rmdir %d", -EINVAL);
	return -EINVAL;
}

static int ddfs_rename(struct inode *old_dir, struct dentry *old_dentry,
		       struct inode *new_dir, struct dentry *new_dentry,
		       unsigned int flags)
{
	dd_print(
		"ddfs_rename: old_dir: %p, old_dentry: %p, new_dir: %p, new_dentry: %p, flags: %u",
		old_dir, old_dentry, new_dir, new_dentry, flags);
	dd_print("old_dir");
	dump_ddfs_inode_info(DDFS_I(old_dir));
	dd_print("new_dir");
	dump_ddfs_inode_info(DDFS_I(new_dir));

	dd_print("~ddfs_rename %d", -EINVAL);
	return -EINVAL;
}

static const struct inode_operations ddfs_dir_inode_operations = {
	.create = ddfs_create,
	.lookup = ddfs_lookup,
	.unlink = ddfs_unlink,
	.mkdir = ddfs_mkdir,
	.rmdir = ddfs_rmdir,
	.rename = ddfs_rename,
	.setattr = ddfs_setattr,
	.getattr = ddfs_getattr,
	.update_time = ddfs_update_time,
};

const struct file_operations ddfs_dir_operations = {
	// Todo: fill
	// .llseek = generic_file_llseek,
	// .read = generic_read_dir,
	// .iterate_shared = fat_readdir,
	// #ifdef CONFIG_COMPAT
	// .compat_ioctl	= fat_compat_dir_ioctl,
	// #endif
	.fsync = ddfs_file_fsync
};

static int ddfs_revalidate(struct dentry *dentry, unsigned int flags)
{
	dd_print("ddfs_revalidate: dentry: %p, flags: %u", dentry, flags);
	dd_print("~ddfs_revalidate 0");
	return 0;
}

static int ddfs_hash(const struct dentry *dentry, struct qstr *qstr)
{
	qstr->hash = full_name_hash(dentry, qstr->name,
				    DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE);
	return 0;
}

static int ddfs_cmp(const struct dentry *dentry, unsigned int len,
		    const char *str, const struct qstr *name)
{
	return !(strncmp(name->name, str, DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE) ==
		 0);
}

static const struct dentry_operations ddfs_dentry_ops = {
	.d_revalidate = ddfs_revalidate,
	.d_hash = ddfs_hash,
	.d_compare = ddfs_cmp,
};

long ddfs_read_boot_sector(struct super_block *sb, void *data,
			   struct ddfs_boot_sector *boot_sector)
{
	dd_print("ddfs_read_boot_sector");
	memcpy(boot_sector, data, sizeof(struct ddfs_boot_sector));
	dd_print("~ddfs_read_boot_sector 0");
	return 0;
}

void log_boot_sector(struct ddfs_boot_sector *boot_sector)
{
	dd_print("sector_size: %u, s/c: %u, number_of_clusters: %u",
		 (unsigned)boot_sector->sector_size,
		 (unsigned)boot_sector->sectors_per_cluster,
		 (unsigned)boot_sector->number_of_clusters);
}

/* Convert ddfs attribute bits and a mask to the UNIX mode. */
static inline umode_t ddfs_make_mode(struct ddfs_sb_info *sbi, u8 attrs,
				     umode_t mode)
{
	if (attrs & DDFS_DIR_ATTR) {
		return (mode & ~S_IFMT) | S_IFDIR;
	}

	return 0;
}

static int ddfs_read_root(struct inode *inode)
{
	struct ddfs_sb_info *sbi = DDFS_SB(inode->i_sb);
	struct ddfs_inode_info *dd_inode = DDFS_I(inode);

	dd_print("ddfs_read_root %p", inode);

	dd_inode->i_pos = DDFS_ROOT_INO;
	inode_inc_iversion(inode);

	inode->i_generation = 0;
	inode->i_mode = ddfs_make_mode(sbi, DDFS_DIR_ATTR, S_IRWXUGO);
	dd_print("root inode->i_mode: %x", inode->i_mode);
	inode->i_op = sbi->dir_ops;
	inode->i_fop = &ddfs_dir_operations;

	dd_inode->i_start = sbi->v.root_cluster;

	// Todo: handle root bigget than one cluster
	inode->i_size = sbi->v.cluster_size;
	inode->i_blocks = sbi->v.blocks_per_cluster;

	dd_inode->i_logstart = 0;
	dd_inode->mmu_private = inode->i_size;

	dd_inode->i_attrs |= DDFS_DIR_ATTR;

	ddfs_take_first_root_cluster_if_not_taken(sbi);

	dd_print("set up root:");
	dump_ddfs_inode_info(dd_inode);

	dd_print("~ddfs_read_root 0");
	return 0;
}

static int ddfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct ddfs_sb_info *sbi;
	long error;
	struct buffer_head *bh;
	struct ddfs_boot_sector boot_sector;
	struct inode *root_inode;

	dd_print("ddfs_fill_super");

	sbi = kzalloc(sizeof(struct ddfs_sb_info), GFP_KERNEL);
	if (!sbi) {
		dd_error("kzalloc of sbi failed");
		dd_print("~ddfs_fill_super %d", -ENOMEM);
		return -ENOMEM;
	}
	sb->s_fs_info = sbi;

	sb->s_flags |= SB_NODIRATIME;
	sb->s_magic = DDFS_SUPER_MAGIC;
	sb->s_op = &ddfs_sops;
	sb->s_export_op = &ddfs_export_ops;
	sb->s_time_gran = 1;
	sb->s_d_op = &ddfs_dentry_ops;

	error = -EIO;
	sb_min_blocksize(sb, 512);
	bh = sb_bread(sb, 0);
	if (bh == NULL) {
		dd_error("unable to read SB");
		goto out_fail;
	}

	error = ddfs_read_boot_sector(sb, bh->b_data, &boot_sector);
	if (error == -EINVAL) {
		dd_error("unable to read boot sector");
		goto out_fail;
	}
	brelse(bh);
	log_boot_sector(&boot_sector);

	sbi->v.blocks_per_cluster = boot_sector.sectors_per_cluster;

	mutex_init(&sbi->s_lock);
	sbi->sb = sb;
	sbi->dir_ops = &ddfs_dir_inode_operations;

	sbi->v = ddfs_calc_sbi_values(&boot_sector);

	dd_print("Making root inode");
	root_inode = new_inode(sb);
	if (!root_inode) {
		dd_print("new_inode for root node failed");
		goto out_fail;
	}
	dd_print("root_inode ptr: %p", root_inode);

	root_inode->i_ino = DDFS_ROOT_INO;
	dd_print("calling inode_set_iversion(root_inode, 1)");
	inode_set_iversion(root_inode, 1);

	dd_print("calling ddfs_read_root");
	error = ddfs_read_root(root_inode);
	if (error) {
		iput(root_inode);
		dd_print("ddfs_read_root failed with: %ld", error);
		goto out_fail;
	}
	dd_print("ddfs_read_root succeed");

	dd_print("calling insert_inode_hash");
	insert_inode_hash(root_inode);
	dd_print("insert_inode_hash call succeed");

	sb->s_root = d_make_root(root_inode);
	if (!sb->s_root) {
		dd_print("d_make_root root inode failed");
		goto out_fail;
	}

	dd_print("making root_inode success. root_inode: %p, sb->s_root: %p",
		 root_inode, sb->s_root);

	dd_print("~ddfs_fill_super 0");

	return 0;

out_fail:
	sb->s_fs_info = NULL;
	kfree(sbi);
	dd_print("~ddfs_fill_super %ld", error);
	return error;
}

static struct dentry *ddfs_mount(struct file_system_type *fs_type, int flags,
				 const char *dev_name, void *data)
{
	struct dentry *result;
	dd_print("ddfs_mount");
	result = mount_bdev(fs_type, flags, dev_name, data, ddfs_fill_super);
	dd_print("~ddfs_mount: %p", result);
	return result;
}

static struct file_system_type ddfs_fs_type = {
	.owner = THIS_MODULE,
	.name = KBUILD_MODNAME,
	.mount = ddfs_mount,
	.kill_sb = kill_block_super,
	.fs_flags = FS_REQUIRES_DEV,
};

static void init_once(void *foo)
{
	struct ddfs_inode_info *ei = (struct ddfs_inode_info *)foo;

	spin_lock_init(&ei->cache_lru_lock);
	ei->nr_caches = 0;
	ei->cache_valid_id = 1;
	INIT_LIST_HEAD(&ei->cache_lru);
	INIT_HLIST_NODE(&ei->i_fat_hash);
	INIT_HLIST_NODE(&ei->i_dir_hash);
	inode_init_once(&ei->ddfs_inode);
}

static int __init ddfs_init_inodecache(void)
{
	dd_print("ddfs_init_inodecache");

	ddfs_inode_cachep = kmem_cache_create(
		"ddfs_inode_cachep", sizeof(struct ddfs_inode_info), 0,
		(SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD | SLAB_ACCOUNT),
		init_once);

	if (ddfs_inode_cachep == NULL) {
		dd_print("kmem_cache_create failed");
		dd_print("~ddfs_init_inodecache %d", -ENOMEM);
		return -ENOMEM;
	}
	dd_print("kmem_cache_create succeed");

	dd_print("~ddfs_init_inodecache 0");
	return 0;
}

static void __exit ddfs_destroy_inodecache(void)
{
	dd_print("ddfs_destroy_inodecache");
	rcu_barrier();
	kmem_cache_destroy(ddfs_inode_cachep);
	dd_print("~ddfs_destroy_inodecache");
}

static int __init init_ddfs_fs(void)
{
	int err;
	dd_print("init_ddfs_fs");

	err = ddfs_init_inodecache();
	if (err) {
		dd_print("ddfs_init_inodecache failed with %d", err);
		dd_print("~init_ddfs_fs %d", err);
		return err;
	}

	err = register_filesystem(&ddfs_fs_type);
	dd_print("~init_ddfs_fs");
	return err;
}

static void __exit exit_ddfs_fs(void)
{
	dd_print("exit_ddfs_fs");

	dd_print("calling ddfs_destroy_inodecache");
	ddfs_destroy_inodecache();

	unregister_filesystem(&ddfs_fs_type);
	dd_print("~exit_ddfs_fs");
}

MODULE_ALIAS_FS(KBUILD_MODNAME);
MODULE_LICENSE("Dual BSD/GPL");

module_init(init_ddfs_fs);
module_exit(exit_ddfs_fs);
