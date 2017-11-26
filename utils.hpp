
#pragma once

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

}
