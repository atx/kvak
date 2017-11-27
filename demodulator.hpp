
#pragma once

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <deque>
#include <memory>
#include <vector>

namespace kvak {


class demodulator {
public:
					demodulator();

	std::optional<std::uint8_t> push_sample(std::complex<float> sample);

private:

	void shift_delay_line(std::complex<float> sample);
	void resample();
	void timing_recovery();
	void phase_recovery();
	std::uint8_t calculate_output();

	// How many samples are we forward by at the moment
	float timing_delta;
	// Phase rotator state
	float phase_rotator;
	// Stores the recent input samples, twice to allow easy pointer-ing
	// (trick from gnuradio)
	std::vector<std::complex<float>> delay_line;
	unsigned int delay_line_i;

	// Storage for (resampled) sample history
	std::complex<float> sample_prev_prev;
	std::complex<float> sample_prev;
	std::complex<float> sample_now;

	// Frequency and phase offsets
	float phase_offset;
	float frequency_offset;

	float samples_per_symbol;

	bool at_midpoint;
};

};
