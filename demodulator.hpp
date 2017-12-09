
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
#include "gardner.hpp"

namespace kvak {


class demodulator {
public:
					demodulator();

	std::optional<std::uint8_t> push_sample(std::complex<float> sample);

	struct info {
		float timing_offset;
		float frequency_offset;
		float power_level;
		bool is_muted;
	};

	info get_info();

private:

	void shift_delay_line(std::complex<float> sample);
	void resample();
	std::uint8_t calculate_output();

	// How many samples are we forward by at the moment
	kvak::costas costas;
	kvak::gardner gardner;
	// Stores the recent input samples, twice to allow easy pointer-ing
	// (trick from gnuradio)
	std::vector<std::complex<float>> delay_line;
	unsigned int delay_line_i;

	// Storage for (resampled) sample history
	std::complex<float> sample_prev_prev;
	std::complex<float> sample_prev;
	std::complex<float> sample_now;

	bool at_midpoint;
};

};
