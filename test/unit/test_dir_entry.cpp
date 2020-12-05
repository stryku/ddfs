#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

#include <dir_entry.h>

#include <unordered_map>

TEST_CASE(
	"DDFS.DirEntry.calc_dir_entry_offsets.first entry, first cluster, first block")
{
	const auto calc_params =
		ddfs_dir_entry_calc_params{ .entries_per_cluster = 10,
					    .blocks_per_cluster = 1,
					    .data_cluster_no = 0,
					    .block_size = 512,
					    .dir_logical_start = 0 };

	const auto result = ddfs_calc_dir_entry_offsets(&calc_params, 0);

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

TEST_CASE(
	"DDFS.DirEntry.calc_dir_entry_offsets.3 entry, first cluster, first block")
{
	const auto calc_params =
		ddfs_dir_entry_calc_params{ .entries_per_cluster = 10,
					    .blocks_per_cluster = 1,
					    .data_cluster_no = 0,
					    .block_size = 512,
					    .dir_logical_start = 0 };

	const auto result = ddfs_calc_dir_entry_offsets(&calc_params, 3);

	REQUIRE_EQ(result.name.block_on_device, 0);
	REQUIRE_EQ(result.name.offset_on_block,
		   3 * DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE);

	REQUIRE_EQ(result.attributes.block_on_device, 0);
	REQUIRE_EQ(result.attributes.offset_on_block,
		   10 * 4 + 3 * sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE));

	REQUIRE_EQ(result.size.block_on_device, 0);
	REQUIRE_EQ(result.size.offset_on_block,
		   10 * (4 + sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE)) +
			   3 * sizeof(DDFS_DIR_ENTRY_SIZE_TYPE));

	REQUIRE_EQ(result.first_cluster.block_on_device, 0);
	REQUIRE_EQ(result.first_cluster.offset_on_block,
		   10 * (4 + sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE) +
			 sizeof(DDFS_DIR_ENTRY_SIZE_TYPE)) +
			   3 * sizeof(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE));
}

TEST_CASE(
	"DDFS.DirEntry.calc_dir_entry_offsets.7 entry, first cluster, mixed blocks")
{
	const auto calc_params =
		ddfs_dir_entry_calc_params{ .entries_per_cluster = 30,
					    .blocks_per_cluster = 2,
					    .data_cluster_no = 5,
					    .block_size = 256,
					    .dir_logical_start = 10 };

	const auto cluster_first_block_on_device =
		calc_params.blocks_per_cluster *
		(calc_params.data_cluster_no + calc_params.dir_logical_start);

	const auto result = ddfs_calc_dir_entry_offsets(&calc_params, 7);

	// =28 -> first block on cluster
	const auto expected_name_offset = 7 * 4;
	REQUIRE_EQ(result.name.block_on_device, cluster_first_block_on_device);
	REQUIRE_EQ(result.name.offset_on_block, expected_name_offset);

	// =127 -> first block on cluster
	const auto expected_atr_offset =
		calc_params.entries_per_cluster *
			DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
		7 * sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE);
	REQUIRE_EQ(result.attributes.block_on_device,
		   cluster_first_block_on_device);
	REQUIRE_EQ(result.attributes.offset_on_block, expected_atr_offset);

	// =206 -> first block on cluster
	const auto expected_size_offset =
		calc_params.entries_per_cluster *
			(DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
			 sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE)) +
		7 * sizeof(DDFS_DIR_ENTRY_SIZE_TYPE);
	REQUIRE_EQ(result.size.block_on_device, cluster_first_block_on_device);
	REQUIRE_EQ(result.size.offset_on_block, expected_size_offset);

	// =418 -> second block on cluster, thus offset on block = 418 - 256
	const auto expected_first_cluster_offset =
		(calc_params.entries_per_cluster *
			 (DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
			  sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE) +
			  sizeof(DDFS_DIR_ENTRY_SIZE_TYPE)) +
		 7 * sizeof(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE)) -
		256;
	REQUIRE_EQ(result.first_cluster.block_on_device,
		   cluster_first_block_on_device + 1);
	REQUIRE_EQ(result.first_cluster.offset_on_block,
		   expected_first_cluster_offset);
}

TEST_CASE(
	"DDFS.DirEntry.calc_dir_entry_offsets.40 entry, far cluster, mixed blocks")
{
	const auto calc_params =
		ddfs_dir_entry_calc_params{ .entries_per_cluster = 30,
					    .blocks_per_cluster = 2,
					    .data_cluster_no = 5,
					    .block_size = 256,
					    .dir_logical_start = 10 };

	const auto expected_entry_cluster =
		calc_params.data_cluster_no + calc_params.dir_logical_start + 1;
	const auto cluster_first_block_on_device =
		calc_params.blocks_per_cluster * expected_entry_cluster;

	const auto entry = 40;
	const auto entry_on_cluster = entry - calc_params.entries_per_cluster;
	const auto result = ddfs_calc_dir_entry_offsets(&calc_params, entry);

	// =28 -> first block on cluster
	const auto expected_name_offset = entry_on_cluster * 4;
	REQUIRE_EQ(result.name.block_on_device, cluster_first_block_on_device);
	REQUIRE_EQ(result.name.offset_on_block, expected_name_offset);

	// =127 -> first block on cluster
	const auto expected_atr_offset =
		calc_params.entries_per_cluster *
			DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
		entry_on_cluster * sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE);
	REQUIRE_EQ(result.attributes.block_on_device,
		   cluster_first_block_on_device);
	REQUIRE_EQ(result.attributes.offset_on_block, expected_atr_offset);

	// =206 -> first block on cluster
	const auto expected_size_offset =
		calc_params.entries_per_cluster *
			(DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
			 sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE)) +
		entry_on_cluster * sizeof(DDFS_DIR_ENTRY_SIZE_TYPE);
	REQUIRE_EQ(result.size.block_on_device, cluster_first_block_on_device);
	REQUIRE_EQ(result.size.offset_on_block, expected_size_offset);

	// =418 -> second block on cluster, thus offset on block = 418 - 256
	const auto expected_first_cluster_offset =
		(calc_params.entries_per_cluster *
			 (DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
			  sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE) +
			  sizeof(DDFS_DIR_ENTRY_SIZE_TYPE)) +
		 entry_on_cluster * sizeof(DDFS_DIR_ENTRY_FIRST_CLUSTER_TYPE)) -
		256;
	REQUIRE_EQ(result.first_cluster.block_on_device,
		   cluster_first_block_on_device + 1);
	REQUIRE_EQ(result.first_cluster.offset_on_block,
		   expected_first_cluster_offset);
}

static ddfs_block block_provider_fun(void *data, unsigned block_no)
{
	const auto *map =
		reinterpret_cast<std::unordered_map<unsigned, ddfs_block> *>(
			data);

	const auto found = map->find(block_no);
	REQUIRE(found != map->cend());

	return found->second;
}

TEST_CASE("DDFS.ddfs_access_dir_entries.name")
{
	const auto calc_params =
		ddfs_dir_entry_calc_params{ .entries_per_cluster = 30,
					    .blocks_per_cluster = 1,
					    .data_cluster_no = 0,
					    .block_size = 512,
					    .dir_logical_start = 0 };

	std::unordered_map<unsigned, ddfs_block> map;
	map[0] = { .bh = (buffer_head *)100, .data = (char *)1000 };

	// Name
	{
		const auto result = ddfs_access_dir_entries(block_provider_fun,
							    &map, &calc_params,
							    0, DDFS_PART_NAME);

		REQUIRE(result.name.bh == (buffer_head *)100);
		REQUIRE(result.name.ptr == (DDFS_DIR_ENTRY_NAME_TYPE *)1000);

		REQUIRE(result.attributes.bh == nullptr);
		REQUIRE(result.size.bh == nullptr);
		REQUIRE(result.first_cluster.bh == nullptr);
	}

	// Attributes
	{
		const auto result =
			ddfs_access_dir_entries(block_provider_fun, &map,
						&calc_params, 0,
						DDFS_PART_ATTRIBUTES);

		REQUIRE(result.attributes.bh == (buffer_head *)100);

		const auto expected_offset = calc_params.entries_per_cluster *
					     DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE;
		REQUIRE(result.attributes.ptr ==
			(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE *)(1000 +
							   expected_offset));

		REQUIRE(result.name.bh == nullptr);
		REQUIRE(result.size.bh == nullptr);
		REQUIRE(result.first_cluster.bh == nullptr);
	}

	// Size
	{
		const auto result = ddfs_access_dir_entries(block_provider_fun,
							    &map, &calc_params,
							    0, DDFS_PART_SIZE);

		REQUIRE(result.size.bh == (buffer_head *)100);

		const auto expected_offset =
			calc_params.entries_per_cluster *
			(DDFS_DIR_ENTRY_NAME_CHARS_IN_PLACE +
			 sizeof(DDFS_DIR_ENTRY_ATTRIBUTES_TYPE));
		REQUIRE(result.size.ptr ==
			(DDFS_DIR_ENTRY_SIZE_TYPE *)(1000 + expected_offset));

		REQUIRE(result.name.bh == nullptr);
		REQUIRE(result.attributes.bh == nullptr);
		REQUIRE(result.first_cluster.bh == nullptr);
	}
}