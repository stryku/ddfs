#pragma once

#include <string>

std::string DDFS_MOUNTED_DIR;

inline std::string make_path(std::string name)
{
	if (name.front() != '/') {
		name = '/' + name;
	}

	return DDFS_MOUNTED_DIR + name;
}

int main(int argc, char **argv)
{
	DDFS_MOUNTED_DIR = argv[1];
	return doctest::Context(argc - 1, argv).run();
}
