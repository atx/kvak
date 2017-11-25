
#pragma once

#include <array>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <deque>
#include <memory>
#include <vector>

#include "ring.hpp"

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


//class demodulator_ {
//public:
//						demodulator(unsigned int nchannels);
//
//	void push_data(std::complex<float> *data, std::size_t length,
//				   std::uint8_t *bits);
//
//	static const unsigned int samples_per_symbol = 2;
//	static const unsigned int bits_per_symbol = 2;
//
//private:
//
//	void push_buffers(std::complex<float> *data);
//	bool shift_delay_lines();
//	void resample_lines();
//	void increment_deltas();
//	void timing_recovery();
//	void calculate_output();
//
//	void process_round(std::complex<float> *data,
//					   std::function<void(std::uint8_t *)> callback);
//
//	const unsigned int nchannels;
//
//	// Buffer of samples
//	std::vector<ring<std::complex<float>, 128>> channel_buffers;
//	// Says how many samples are we forward by
//	std::vector<float> timing_deltas;
//	// Phase rotator states
//	std::vector<float> phase_rotators;
//	// Stores the recent input samples, twice to allow easy pointer-ing
//	// (trick from gnuradio)
//	std::vector<std::vector<std::complex<float>>> delay_lines;
//	// Counts our current delay line position
//	std::vector<unsigned int> delay_line_is;
//	// Storage for current samples
//	std::unique_ptr<std::complex<float>[]> samples_prev_prev;
//	std::unique_ptr<std::complex<float>[]> samples_prev;
//	std::unique_ptr<std::complex<float>[]> samples_now;
//	// Frequency and phase offsets
//	std::vector<float> phase_offsets;
//	std::vector<float> frequency_offsets;
//	// Output symbol
//	std::unique_ptr<std::uint8_t[]> output_symbols;
//	// State switching
//	bool at_midpoint;
//
//	std::FILE *fdbg_err;
//	std::FILE *fdbg_res;
//};

};
