
#pragma once

#include <cstddef>
#include <complex>

namespace kvak::interpolator {

std::complex<float> interpolate(const std::complex<float> *input, float by);
extern const unsigned int num_taps;
extern const unsigned int num_steps;

}
