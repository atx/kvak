
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

	std::vector<std::complex<float>> dline(interp::num_taps * 2, 0.0);
	std::size_t dline_i = 0;

	std::array<std::complex<float>, 1024 * 8> data;
	std::fill(data.begin(), data.end(), 0.0);

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
			dline[dline_i] = data[i];
			dline[dline_i + interp::num_taps] = data[i];
			dline_i = (dline_i + 1) % interp::num_taps;

			while (phase <= 1.0) {
				std::complex<float> out = interp::interpolate(&dline[dline_i], phase);
				phase += phase_inc;
				std::fwrite(&out, sizeof(out), 1, stdout);
			}

			phase -= 1.0;
		}
		std::memmove(&data[0], &data[data.size() - interp::num_taps],
					 interp::num_taps * sizeof(data[0]));
	}

	return 0;
}
