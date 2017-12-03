
#include <algorithm>
#include <iostream>

#include "gardner.hpp"

namespace kvak {

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

	this->resampling_fraction -= err * 0.001;
	this->timing_delta -= err * 0.009;
}


bool gardner::ready() const
{
	return this->timing_delta <= 1.0;
}

}
