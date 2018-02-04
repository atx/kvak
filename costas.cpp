
#include <iostream>

#include "expj.hpp"
#include "utils.hpp"

#include "costas.hpp"

namespace kvak {

static float freq_weight = 7.416531163999999e-05;
static float phase_weight = 0.0005857249552;

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
		-(std::copysign(1.0, sample.real()) * sample.imag() -
		  std::copysign(1.0, sample.imag()) * sample.real());
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


static const expj_precalc<256> expj;


std::complex<float> costas::advance(std::complex<float> sample)
{
	this->phase_offset += this->last_error * phase_weight;
	this->phase_offset += this->phase_increment;
	this->phase_offset = utils::modular_clamp<float>(
		this->phase_offset, 0.0, M_PI*2, M_PI*2
	);

	return sample * expj(this->phase_offset);
}

};
