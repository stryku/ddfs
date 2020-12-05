#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

#include <dir_entry.h>

TEST_CASE("DDFS.DirEntry.calc_dir_entry_offsets")
{
	{
		const auto calc_params =
			ddfs_dir_entry_calc_params{ .entries_per_cluster = 10,
						    .blocks_per_cluster = 1,
						    .data_cluster_no = 0,
						    .block_size = 512,
						    .dir_logical_start = 0 };

		const auto result =
			ddfs_calc_dir_entry_offsets(&calc_params, 0);

		REQUIRE_EQ(result.name.block_on_device, 0);
		REQUIRE_EQ(result.name.offset_on_block, 0);

		REQUIRE_EQ(result.attributes.block_on_device, 0);
		REQUIRE_EQ(result.attributes.offset_on_block, 10 * 4);

		REQUIRE_EQ(result.size.block_on_device, 0);
		REQUIRE_EQ(result.size.offset_on_block,
			   10 * (4 + sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE)));

		REQUIRE_EQ(result.first_cluster.block_on_device, 0);
		REQUIRE_EQ(result.first_cluster.offset_on_block,
			   10 * (4 + sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE) +
				 sizeof(DDFS_DIR_ENTRY_SIZE_TYPE)));
	}
}
