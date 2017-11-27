
#include <algorithm>
#include <iostream>

#include "interpolator.hpp"
#include "utils.hpp"

#include "demodulator.hpp"

using namespace std::complex_literals;

namespace interp = kvak::interpolator;

namespace kvak {

demodulator::demodulator()
	:
	timing_delta(3.0),
	phase_rotator(0.0),
	delay_line(interp::num_taps * 2, 0.0),
	delay_line_i(0),
	sample_prev_prev(0.0),
	sample_prev(0.0),
	sample_now(0.0),
	phase_offset(0.0),
	frequency_offset(-M_PI / 8),
	samples_per_symbol(2.0),
	at_midpoint(false)
{
}


void demodulator::shift_delay_line(std::complex<float> sample)
{

	this->delay_line[this->delay_line_i] = sample;
	this->delay_line[this->delay_line_i + interp::num_taps] = sample;
	this->delay_line_i = (this->delay_line_i + 1) % interp::num_taps;
	this->timing_delta -= 1.0;
}


void demodulator::resample()
{
	std::swap(this->sample_prev_prev, this->sample_prev);
	std::swap(this->sample_prev, this->sample_now);

	std::complex<float> *dline = &this->delay_line.data()[this->delay_line_i];
	std::complex<float> raw = interp::interpolate(dline, this->timing_delta);

	this->phase_offset += this->frequency_offset;
	this->phase_offset = kvak::utils::modular_clamp<float>(
		this->phase_offset, -M_PI*2, M_PI*2, M_PI*2
	);

	std::complex<float> phase(0.0, this->phase_offset);
	this->sample_now = raw * std::exp(phase);
}


void demodulator::timing_recovery()
{
	std::complex<float> rot = std::exp(std::complex<float>(0.0, M_PI / 4));
	std::complex<float> error_c =
		(this->sample_prev_prev*rot - this->sample_now*rot);
	float err_i = error_c.imag() * (this->sample_prev*rot).imag();
	float err_q = error_c.real() * (this->sample_prev*rot).real();
	float err = err_i + err_q;
	this->samples_per_symbol -= err * 0.001;
}


static float calculate_phase_error(std::complex<float> sample)
{
	// https://github.com/gnuradio/gnuradio/blob/master/gr-digital/lib/costas_loop_cc_impl.cc#L122
	float err =
		((sample.real() > 0.0 ? 1.0 : -1.0) * sample.imag() -
		 (sample.imag() > 0.0 ? 1.0 : -1.0) * sample.real());
	return err;
}


void demodulator::phase_recovery()
{
	float err = calculate_phase_error(this->sample_now);

	// Magical constants taken from
	// https://github.com/gnuradio/gnuradio/blob/master/gr-blocks/lib/control_loop.cc
	float freq_weight = 0.00377634;
	this->frequency_offset += err * freq_weight;

	float phase_weight = 0.0849974;
	this->phase_offset += err * phase_weight;
}


std::uint8_t demodulator::calculate_output()
{

	float phase_difference =
		std::arg(this->sample_now * std::conj(this->sample_prev_prev));
	phase_difference /= M_PI / 2;
	int symbol_idx = round(phase_difference);

	switch(symbol_idx) {
	case  0:  // No rotation
		return 0;
	case 1:
		return 1;
	case -1:
		return 2;
	case -2:  // Rotation by (+-) PI
	case  2:
		return 3;
	}

	// We should never get here
	return 0;
}


std::optional<std::uint8_t> demodulator::push_sample(std::complex<float> sample)
{
	// Shift the delay line
	this->shift_delay_line(sample);
	if (this->timing_delta > 1.0) {
		return {};
	}

	// Resample
	this->resample();

	this->at_midpoint = !this->at_midpoint;
	if (this->at_midpoint) {
		return {};
	}

	this->timing_recovery();
	this->timing_delta += this->samples_per_symbol;
	if (this->timing_delta < 0) {
		this->timing_delta = 0;
	}
	this->phase_recovery();

	std::uint8_t ret = this->calculate_output();
	return ret;
}

}
