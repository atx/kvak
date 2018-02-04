
#pragma once

#include <array>
#include <cmath>
#include <complex>

#include "utils.hpp"

namespace kvak {

template <int N, typename Float = float>
class expj_precalc {
public:

	expj_precalc()
	{
		for (unsigned int i = 0; i < N; i++) {
			Float angle = static_cast<Float>(i) / (N - 1) * 2*M_PI;
			this->values[i] = utils::expj(angle);
		}
	}

	std::complex<Float> operator()(Float angle) const
	{
		// Observation: symmetry between the quarter-circles could be exploited
		// here. Not sure if the added code overhead would outweight having
		// one fourth of the coefficients.
		// Observation 2: Linear interpolation could be used to improve
		// result precision.
		unsigned int i = std::rint(angle / (2*M_PI) * (N - 1));
		return this->values[i];
	}

private:
	std::array<std::complex<Float>, N> values;
};

}
