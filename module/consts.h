#ifndef DDFS_CONSTS_H
#define DDFS_CONSTS_H

#define DDFS_SUPER_MAGIC 0xddf5
#define DDFS_CLUSTER_UNUSED 0
#define DDFS_CLUSTER_NOT_ASSIGNED 0xfffffffe
#define DDFS_CLUSTER_EOF 0xffffffff

#define DDFS_PART_NAME 1
#define DDFS_PART_ATTRIBUTES 2
#define DDFS_PART_SIZE 4
#define DDFS_PART_FIRST_CLUSTER 8
#define DDFS_PART_ALL                                                          \
	(DDFS_PART_NAME | DDFS_PART_ATTRIBUTES | DDFS_PART_SIZE |              \
	 DDFS_PART_FIRST_CLUSTER)

#define DDFS_ROOT_INO 0

#define DDFS_FILE_ATTR 1
#define DDFS_DIR_ATTR 2

#define DDFS_DEFAULT_MODE ((umode_t)(S_IRUGO | S_IWUGO | S_IXUGO))

#define DDFS_DIR_ENTRY_NAME_TYPE __u8
#define DDFS_DIR_ENTRY_ATTRIBUTES_TYPE __u8
#define DDFS_DIR_ENTRY_SIZE_TYPE __u64
#define DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE __u32
#define DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE 4

#define DDFS_TABLE_ENTRY_TYPE __u32

struct buffer_head;
struct ddfs_block {
	struct buffer_head *bh;
	char *data;
};

typedef struct ddfs_block (*block_provider)(void *, unsigned);

#ifndef DDFS_BUILDING_TESTS
#define dd_print(...)                                                          \
	do {                                                                   \
		printk(KERN_INFO "-------------------[DDFS]: " __VA_ARGS__);   \
	} while (0);

#define dd_error(...)                                                          \
	do {                                                                   \
		printk(KERN_ERR                                                \
		       "-------------------[DDFS ERR]: " __VA_ARGS__);         \
	} while (0);
#else
#define dd_print(...)
#define dd_error(...)
#endif

#endif