#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

#include <sbi_values.h>

constexpr auto k_entries_per_cluster = 128u;

TEST_CASE("DDFS.SbiValues.calc_sbi_values")
{
	ddfs_boot_sector bs;
	bs.sector_size = 512;
	bs.sectors_per_cluster = 1;
	bs.number_of_clusters = 16;

	const ddfs_sbi_values sbi_v = ddfs_calc_sbi_values(&bs);

	const auto expected_combined_dir_entry_parts_size = 4 + 1 + 8 + 4;
	REQUIRE_EQ(sbi_v.combined_dir_entry_parts_size,
		   expected_combined_dir_entry_parts_size);
	REQUIRE_EQ(sbi_v.table_offset, bs.sector_size);
	REQUIRE_EQ(sbi_v.table_size,
		   bs.number_of_clusters * sizeof(DDFS_TABLE_ENTRY_TYPE));

	REQUIRE_EQ(sbi_v.number_of_table_entries, bs.number_of_clusters);
	REQUIRE_EQ(sbi_v.number_of_table_entries_per_cluster,
		   bs.sector_size * bs.sectors_per_cluster /
			   sizeof(DDFS_TABLE_ENTRY_TYPE));

	REQUIRE_EQ(sbi_v.cluster_size, bs.sector_size * bs.sectors_per_cluster);
	// Data starts on 3rd cluster
	REQUIRE_EQ(sbi_v.data_offset, bs.sector_size + bs.sector_size);
	REQUIRE_EQ(sbi_v.data_cluster_no, 2u);

	// First root dir cluster is the very first cluster
	REQUIRE_EQ(sbi_v.root_cluster, 2u);

	REQUIRE_EQ(sbi_v.block_size, bs.sector_size);
	REQUIRE_EQ(sbi_v.blocks_per_cluster, bs.sectors_per_cluster);

	const auto expected_entries_per_cluster =
		bs.sector_size / expected_combined_dir_entry_parts_size;
	REQUIRE_EQ(sbi_v.entries_per_cluster, expected_entries_per_cluster);

	REQUIRE_EQ(sbi_v.name_entries_offset, 0u);
	REQUIRE_EQ(sbi_v.attributes_entries_offset,
		   expected_entries_per_cluster * 4u);
	REQUIRE_EQ(sbi_v.size_entries_offset,
		   expected_entries_per_cluster * (4u + 1u));
	REQUIRE_EQ(sbi_v.first_cluster_entries_offset,
		   expected_entries_per_cluster * (4u + 1u + 8u));
}
