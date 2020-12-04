#include <cstdio>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "3rdparty/doctest.h"

#include "common.hpp"

TEST_CASE("DDFS.WriteFile")
{
	FILE *fp = std::fopen(make_path("/aaa").c_str(), "w");
	REQUIRE(fp != nullptr);

	const auto to_write = std::string{ "foo, bar, baz" };

	const auto write_size =
		std::fwrite(to_write.data(), 1u, to_write.size(), fp);

	REQUIRE(write_size == to_write.size());
	REQUIRE(std::fclose(fp) == 0);
}
