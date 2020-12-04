#include <cstdio>

#define DOCTEST_CONFIG_IMPLEMENT
#include "3rdparty/doctest.h"

#include "common.hpp"

TEST_CASE("DDFS.FileCreation")
{
	FILE *fp = std::fopen(make_path("/aaaaaaaaaaaaaaaaaaaaaaaaaaa").c_str(),
			      "w");
	REQUIRE(fp != nullptr);

	REQUIRE(std::fclose(fp) == 0);
}

TEST_CASE("DDFS.MultipleFileCreation")
{
	FILE *fpa = std::fopen(make_path("/aaa").c_str(), "w");
	REQUIRE(fpa != nullptr);
	FILE *fpb = std::fopen(make_path("/bbb").c_str(), "w");
	REQUIRE(fpb != nullptr);
	FILE *fpc = std::fopen(make_path("/ccc").c_str(), "w");
	REQUIRE(fpc != nullptr);

	REQUIRE(std::fclose(fpa) == 0);
	REQUIRE(std::fclose(fpb) == 0);
	REQUIRE(std::fclose(fpc) == 0);
}
