#include <cstdio>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "3rdparty/doctest.h"

#include "common.hpp"

TEST_CASE("DDFS.Write")
{
	FILE *fp = std::fopen(make_path("/aaa").c_str(), "w");
	REQUIRE(fp != nullptr);

	const auto to_write = std::string{ "foo, bar, baz" };

	const auto write_size =
		std::fwrite(to_write.data(), 1u, to_write.size(), fp);

	REQUIRE(write_size == to_write.size());
	REQUIRE(std::fclose(fp) == 0);
}

TEST_CASE("DDFS.WriteRead")
{
	const auto path = make_path("aaa");
	FILE *fp = std::fopen(path.c_str(), "w");
	REQUIRE(fp != nullptr);

	const auto to_write = std::string{ "foo, bar, baz" };

	const auto write_size =
		std::fwrite(to_write.data(), 1u, to_write.size(), fp);

	REQUIRE(write_size == to_write.size());
	REQUIRE(std::fclose(fp) == 0);

	fp = std::fopen(path.c_str(), "r");
	REQUIRE(fp != nullptr);

	std::string result{};
	result.resize(to_write.size());

	const auto read_size = std::fread(&result[0], 1, to_write.size(), fp);
	REQUIRE(read_size == to_write.size());
	REQUIRE(result == to_write);
	REQUIRE(std::fclose(fp) == 0);
}
