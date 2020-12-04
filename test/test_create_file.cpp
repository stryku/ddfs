#include <cstdio>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "3rdparty/doctest.h"

TEST_CASE("DDFS.FileCreation")
{
	const std::string file_path = DDFS_MOUNTED_DIR + std::string{ "/aaa" };
	FILE *fp = std::fopen(file_path.c_str(), "w");
	REQUIRE(fp != nullptr);

	const auto result = std::fclose(fp);
	REQUIRE(result == 0);
}
