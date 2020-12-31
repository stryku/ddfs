#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "types.hpp"

#include <table.h>

#include <vector>

constexpr auto k_entries_per_cluster = 128u;

TEST_CASE("DDFS.Table.find_free_cluster empty table")
{
	std::vector<DDFS_TABLE_ENTRY_TYPE> clusters(k_entries_per_cluster,
						    DDFS_CLUSTER_UNUSED);
	ddfs_table table{ .clusters = clusters.data(), .index_offset = 0u };
	ddfs_sbi_values sbi_v{ .number_of_table_entries_per_cluster =
				       k_entries_per_cluster };

	const auto result = ddfs_table_find_free_cluster(&table, &sbi_v);

	REQUIRE_EQ(result, 0);
}

TEST_CASE("DDFS.Table.find_free_cluster almost fully taken")
{
	std::vector<DDFS_TABLE_ENTRY_TYPE> clusters(k_entries_per_cluster,
						    DDFS_CLUSTER_EOF);
	ddfs_table table{ .clusters = clusters.data(), .index_offset = 0u };
	ddfs_sbi_values sbi_v{ .number_of_table_entries_per_cluster =
				       k_entries_per_cluster };

	clusters[16] = DDFS_CLUSTER_UNUSED;
	clusters[32] = DDFS_CLUSTER_UNUSED;

	{
		const auto result =
			ddfs_table_find_free_cluster(&table, &sbi_v);
		REQUIRE_EQ(result, 16);
	}

	clusters[16] = DDFS_CLUSTER_EOF;

	{
		const auto result =
			ddfs_table_find_free_cluster(&table, &sbi_v);
		REQUIRE_EQ(result, 32);
	}
}

TEST_CASE("DDFS.Table.find_free_cluster fully taken")
{
	std::vector<DDFS_TABLE_ENTRY_TYPE> clusters(k_entries_per_cluster,
						    DDFS_CLUSTER_EOF);
	ddfs_table table{ .clusters = clusters.data(), .index_offset = 0u };
	ddfs_sbi_values sbi_v{ .number_of_table_entries_per_cluster =
				       k_entries_per_cluster };

	const auto result = ddfs_table_find_free_cluster(&table, &sbi_v);
	REQUIRE_EQ(result, -1);
}