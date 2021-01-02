#ifndef DDFS_BOOT_SECTOR_H
#define DDFS_BOOT_SECTOR_H

struct ddfs_boot_sector {
	__u32 sector_size;
	__u32 sectors_per_cluster;
	__u32 number_of_clusters;
};

#endif