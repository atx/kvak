
#pragma once

#include <experimental/filesystem>

#include "agc.hpp"
#include "demodulator.hpp"

namespace kvak {

class channel {
public:
	channel(unsigned int id, agc &agc, const std::experimental::filesystem::path &path,
			unsigned int preallocate)
		:
		id(id),
		agc_all(agc),
		output_path(path),
		file(nullptr),
		out_data(preallocate, 0),
		out_counter(0),
		muted(false)
	{
	};

	~channel();

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
	const std::experimental::filesystem::path output_path;
	std::FILE *file;
	std::vector<std::uint8_t> out_data;
	unsigned int out_counter;
	bool muted;
};

}
