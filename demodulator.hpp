
#pragma once

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <deque>
#include <memory>
#include <vector>

#include "costas.hpp"

namespace kvak {


class demodulator {
public:
					demodulator();

	std::optional<std::uint8_t> push_sample(std::complex<float> sample);

private:

	void shift_delay_line(std::complex<float> sample);
	void resample();
	void timing_recovery();
	std::uint8_t calculate_output();

	// How many samples are we forward by at the moment
	kvak::costas costas;
	float timing_delta;
	// Stores the recent input samples, twice to allow easy pointer-ing
	// (trick from gnuradio)
	std::vector<std::complex<float>> delay_line;
	unsigned int delay_line_i;

	// Storage for (resampled) sample history
	std::complex<float> sample_prev_prev;
	std::complex<float> sample_prev;
	std::complex<float> sample_now;

	float samples_per_symbol;

	bool at_midpoint;
};

};
