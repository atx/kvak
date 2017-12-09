
#pragma once

#include <capnp/ez-rpc.h>
#include <mutex>

#include "demodulator.hpp"

namespace kvak::server {

class server {
public:
					server(const std::string &bind,
						   std::vector<demodulator> &demods,
						   std::mutex &mutex);

private:
	capnp::EzRpcServer ezrpc;

};

}
