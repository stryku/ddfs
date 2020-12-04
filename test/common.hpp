#pragma once

#include <string>

inline std::string make_path(std::string name)
{
	if (name.front() != '/') {
		name = '/' + name;
	}

	return DDFS_MOUNTED_DIR + name;
}
