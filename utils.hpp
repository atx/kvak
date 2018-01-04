
#pragma once

#include <complex>
#include <functional>
#include <cstdlib>

namespace kvak::utils {

template <typename T>
T modular_clamp(T v, const T lo, const T hi, const T step)
{
	// TODO: We might want to specialize this for floats and use fmod instead
	// However, for our current application, we are never going to walk too far
	// away, so it does not matter.
	while (v > hi) {
		v -= step;
	}
	while (v < lo) {
		v += step;
	}

	return v;
}

template<typename T>
T modular_inc(T a, T b, T c=1)
{
	return (a + c) % b;
}

template<typename T>
T bit(T a, unsigned int n)
{
	return (a >> n) & 0x1;
}

template<typename T>
std::complex<T> expj(T phase)
{
	return std::exp(std::complex<T>(0.0, phase));
}

std::string replace_first(const std::string &haystack,
						  const std::string &needle,
						  const std::string &with);


// This is a hacky method for injecting constants through environment
// variables.
template <typename T>
class env_initializer {
public:

	env_initializer(const std::string &name, T &value)
	{
		const char *envval = std::getenv(name.c_str());
		if (envval != nullptr) {
			std::istringstream(envval) >> value;
		}
	}
};


}
