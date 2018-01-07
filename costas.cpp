
#include <iostream>

#include "utils.hpp"

#include "costas.hpp"

namespace kvak {

static float freq_weight = 2.64876113e-05;
static float phase_weight = 2.09187484e-04;

static utils::env_initializer<float> freq_weight_init("KVAK_COSTAS_FREQ", freq_weight);
static utils::env_initializer<float> phase_weight_init("KVAK_COSTAS_PHASE", phase_weight);

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
	// https://www.dsprelated.com/showthread/comp.dsp/112658-1.php
	this->phase_increment += error * freq_weight;
	this->phase_increment = utils::modular_clamp<float>(
		this->phase_increment, -M_PI*2, M_PI*2, M_PI*2
	);

	this->last_error = error;
}


std::complex<float> costas::advance(std::complex<float> sample)
{
	this->phase_offset += this->last_error * phase_weight;
	this->phase_offset += this->phase_increment;
	this->phase_offset = utils::modular_clamp<float>(
		this->phase_offset, -M_PI*2, M_PI*2, M_PI*2
	);

	return sample * utils::expj(this->phase_offset);
}

};
