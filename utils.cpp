
#include "utils.hpp"

#include <iostream>

namespace kvak::utils {

std::string replace_first(const std::string &haystack,
						  const std::string &needle,
						  const std::string &with)
{
	std::size_t i = haystack.find(needle);
	if (i == std::string::npos) {
		return haystack;
	}

	return std::string(haystack).replace(i, needle.length(), with);
}

}
