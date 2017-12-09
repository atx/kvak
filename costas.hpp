
#pragma once

#include <complex>

namespace kvak {

class costas {
public:
					costas();

	void push_sample(std::complex<float> sample);
	std::complex<float> advance(std::complex<float> sample);

	float get_phase_increment() {
		return this->phase_increment;
	}

private:
	static float calculate_phase_error(std::complex<float> sample);

	float phase_increment;
	float phase_offset;
	float last_error;

};

}
