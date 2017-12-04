
#include <algorithm>
#include <iostream>

#include "interpolator.hpp"
#include "log.hpp"
#include "utils.hpp"

#include "demodulator.hpp"

using namespace std::complex_literals;

namespace interp = kvak::interpolator;

namespace kvak {

demodulator::demodulator()
	:
	delay_line(interp::num_taps * 2, 0.0),
	delay_line_i(0),
	sample_prev_prev(0.0),
	sample_prev(0.0),
	sample_now(0.0),
	at_midpoint(true)
{
}


void demodulator::shift_delay_line(std::complex<float> sample)
{

	this->delay_line[this->delay_line_i] = sample;
	this->delay_line[this->delay_line_i + interp::num_taps] = sample;
	this->delay_line_i = (this->delay_line_i + 1) % interp::num_taps;
	this->gardner.step_backward();
}


void demodulator::resample()
{
	std::swap(this->sample_prev_prev, this->sample_prev);
	std::swap(this->sample_prev, this->sample_now);

	std::complex<float> *dline = &this->delay_line.data()[this->delay_line_i];
	std::complex<float> raw = interp::interpolate(dline, this->gardner.timing_delta);

	this->sample_now = this->costas.advance(raw);
}


std::uint8_t demodulator::calculate_output()
{

	std::complex<float> cdiff = this->sample_now * std::conj(this->sample_prev_prev);
	if (fabs(cdiff.real()) > fabs(cdiff.imag())) {  // Horizontal axis is the major axis
		if (cdiff.real() > 0.0) {
			return 0;  // No rotation
		}
		return 3; // +- pi rotation
	} else if (cdiff.imag() > 0.0 ) {
		return 1;  // +pi rotation
	}
	return 2;  // -pi rotation
}


std::optional<std::uint8_t> demodulator::push_sample(std::complex<float> sample)
{
	// Shift the delay line
	this->shift_delay_line(sample);
	if (this->gardner.timing_delta > 1.0) {
		return {};
	}

	std::optional<std::uint8_t> ret;

	while (this->gardner.ready()) {
		// Resample
		this->resample();

		this->gardner.step_forward();

		this->at_midpoint = !this->at_midpoint;
		if (this->at_midpoint) {
			continue;
		}

		this->gardner.recover(
			this->sample_prev_prev, this->sample_prev, this->sample_now
		);
		this->costas.push_sample(this->sample_now);

		if (ret.has_value()) {
			log::error << "We are running way too fast, dropping symbols!";
		}

		ret = this->calculate_output();
	}
	return ret;
}

}
