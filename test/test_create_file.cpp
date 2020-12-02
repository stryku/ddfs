#include <cstdio>

#define CATCH_CONFIG_MAIN
#include "catch2.hpp"

TEST_CASE("DDFS", "FileCreation")
{
	const std::string file_path =
		DDFS_MOUNTED_DIR + std::string{ "/test_create_file_aaa" };
	FILE *fp = std::fopen(file_path.c_str(), "w");
	REQUIRE(fp != nullptr);

	const auto result = std::fclose(fp);
	REQUIRE(result == 0);
}
