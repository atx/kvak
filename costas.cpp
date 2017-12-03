
#include <iostream>

#include "utils.hpp"

#include "costas.hpp"

namespace kvak {

costas::costas()
	:
	phase_increment(-M_PI / 8),
	phase_offset(0.0),
	last_error(0.0)
{
}


float costas::calculate_phase_error(std::complex<float> sample)
{
	// https://github.com/gnuradio/gnuradio/blob/master/gr-digital/lib/costas_loop_cc_impl.cc#L122

	return
		-((sample.real() > 0.0 ? 1.0 : -1.0) * sample.imag() -
		  (sample.imag() > 0.0 ? 1.0 : -1.0) * sample.real());
}


void costas::push_sample(std::complex<float> sample)
{
	float error = costas::calculate_phase_error(sample);

	// Magical constants taken from
	// https://github.com/gnuradio/gnuradio/blob/master/gr-blocks/lib/control_loop.cc
	float freq_weight = 0.00377634 * 0.5;
	this->phase_increment += error * freq_weight;

	this->last_error = error;
}


std::complex<float> costas::advance(std::complex<float> sample)
{
	float phase_weight = 0.0849974 * 0.5;
	this->phase_offset += this->last_error * phase_weight;
	this->phase_offset += this->phase_increment;
	this->phase_offset = utils::modular_clamp<float>(
		this->phase_offset, -M_PI*2, M_PI*2, M_PI*2
	);

	return sample * utils::expj(this->phase_offset);
}

};
