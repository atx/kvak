
#pragma once

#include <complex>

namespace kvak {

class gardner {
public:

				gardner();

	// Step backward (we got one input sample)
	void step_backward();
	// Step forward (we produced one output sample)
	void step_forward();
	void recover(std::complex<float> prev, std::complex<float> mid, std::complex<float> next);

	bool ready() const;

	// Amount of samples we are currently forward/behind
	float timing_delta;

private:
	// Amount of samples per half symbol
	float resampling_fraction;
	float last_error;

};

}
