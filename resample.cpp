
#include <cstring>
#include <experimental/filesystem>
#include <iostream>
#include <sstream>

#include "interpolator.hpp"


using namespace std::complex_literals;


namespace interp = kvak::interpolator;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <interp-by>" << std::endl;
		return EXIT_FAILURE;
	}

	// Note that we are calculating between 0.0-1.0 (not 0.0-2pi)
	float phase_inc = 1 / std::stof(argv[1]);
	float phase = 0.0;

	std::array<std::complex<float>, 1024 * 8> data;
	std::fill(data.begin(), data.end(), 0.0);

	// Prefill the FIR buffer
	std::size_t len = std::fread(&data[0], sizeof(data[0]), interp::num_taps / 2, stdin);
	if (len != interp::num_taps / 2) {
		std::cerr << "Read from stdin failed!" << std::endl;
		return EXIT_FAILURE;
	}

	while (true) {
		std::size_t len = std::fread(
			&data[interp::num_taps],
			sizeof(data[0]),
			data.size() - interp::num_taps,
			stdin
		);
		if (len == 0) {
			if (std::ferror(stdin) && !std::feof(stdin)) {
				std::cerr << "Read from stdin failed!" << std::endl;
			}
			break;
		}

		for (unsigned int i = 0; i < len; i++) {
			while (phase <= 1.0) {
				std::complex<float> out = interp::interpolate(&data[i], phase);
				phase += phase_inc;
				std::fwrite(&out, sizeof(out), 1, stdout);
			}
			phase -= 1.0;
		}
		std::memcpy(&data[0], &data[data.size() - interp::num_taps],
					interp::num_taps * sizeof(data[0]));
	}

	return 0;
}
