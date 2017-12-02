
#include <cmath>

#include "interpolator.hpp"

using namespace std::complex_literals;

namespace kvak::interpolator {

#include "interpol_taps.hpp"

std::complex<float> interpolate(const std::complex<float> *input, float by)
{
	std::complex<float> ret = 0.0f + 0.0if;
	unsigned int filter_id = lround(by * (num_steps - 1));
	const float *filter = filter_coeffs[filter_id];
	for (unsigned int i = 0; i < num_taps; i++) {
		ret += filter[i] * input[i];
	}
	return ret;
}

}
