
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
		// Do we want separate IQ gain?
		float ampl = (std::fabs(data[i].real()) + std::fabs(data[i].imag())) / 2.0;
		this->gains[i] = this->gains[i] * (1.0 - weight) + weight * ampl;
	}

	for (unsigned int i = 0; i < this->nchannels; i++) {
		data[i] = data[i] / this->gains[i];
		// TODO: We want to have 1.0 here, but rest of the code isn't ready
		// for that at the moment
		data[i] *= 0.05;
	}
}


void agc::push_samples(std::complex<float> *data, size_t nchunks)
{
	for (size_t chunk = 0; chunk < nchunks; chunk++) {
		this->process_chunk(&data[this->nchannels * chunk]);
	}
}



}
