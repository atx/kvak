
#pragma once

#include "agc.hpp"
#include "demodulator.hpp"

namespace kvak {

class channel {
public:
	channel(unsigned int id, agc &agc, std::FILE *file, unsigned int preallocate)
		:
		id(id),
		agc_all(agc),
		file(file),
		out_data(preallocate, 0),
		out_counter(0),
		muted(false)
	{
	};

	float get_power();

	void mute(bool m);
	bool is_muted();

	void push_sample(std::complex<float> sample);
	void flush();

	const unsigned int id;
	demodulator demod;
private:
	// Note that for vectorization reasons we have single multi-channel AGC object
	// for all channels. Maybe this can be patched in a way which makes the
	// code more elegant but would get optimized to the same result?
	agc &agc_all;
	std::FILE *file;
	std::vector<std::uint8_t> out_data;
	unsigned int out_counter;
	bool muted;
};

}