
#include <algorithm>
#include <iostream>

#include "utils.hpp"
#include "gardner.hpp"

namespace kvak {

static float proportional_weight = 0.0009115413708799999;
static float integral_weight = 1.3459823876799997e-07;

static utils::env_initializer<float> proportional_weight_init("KVAK_GARDNER_P", proportional_weight);
static utils::env_initializer<float> integral_weight_init("KVAK_GARDNER_I", integral_weight);

gardner::gardner()
	:
	timing_delta(1.0),
	resampling_fraction(1.0),
	last_error(0.0)
{

}

void gardner::step_backward()
{
	this->timing_delta -= 1.0;
}


void gardner::step_forward()
{
	this->timing_delta += this->resampling_fraction;
}


void gardner::recover(std::complex<float> prev,
					  std::complex<float> mid,
					  std::complex<float> next)
{
	std::complex<float> err_c = next - prev;
	float err_i = err_c.real() * mid.real();
	float err_q = err_c.imag() * mid.imag();
	float err = err_i + err_q;

	this->resampling_fraction -= err * integral_weight;
	this->resampling_fraction = std::clamp(this->resampling_fraction, 0.8f, 1.2f);
	this->timing_delta -= err * proportional_weight;
	if (this->timing_delta < 0.0) {
		// We can't very well walk back in time, so we force a one sample
		// advance and hope that it will fix itself then ¯\_(ツ)_/¯
		this->timing_delta = this->resampling_fraction;
	}
}


bool gardner::ready() const
{
	return this->timing_delta <= 1.0;
}

}
