
#include <iostream>

#include "log.hpp"

#include "agc.hpp"

namespace kvak {

agc::agc(unsigned int nchannels)
	:
	nchannels(nchannels),
	gains(nchannels, 1.0)
{
}


void agc::process_chunk(std::complex<float> *data)
{
	float weight = 0.001;

	for (unsigned int i = 0; i < this->nchannels; i++) {
		data[i] = data[i] * this->gains[i];
	}

	for (unsigned int i = 0; i < this->nchannels; i++) {
		// This is magnitude squared, to avoid calculating sqrt()
		float mag =
			data[i].real() * data[i].real() +
			data[i].imag() * data[i].imag();
		float error = 1.0 - mag;
		this->gains[i] += weight * error;
	}
}


float agc::get_channel_power(unsigned int channel)
{
	return this->gains[channel];
}


void agc::push_samples(std::complex<float> *data, size_t nchunks)
{
	for (size_t chunk = 0; chunk < nchunks; chunk++) {
		this->process_chunk(&data[this->nchannels * chunk]);
	}
}



}
