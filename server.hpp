
#pragma once

#include <capnp/ez-rpc.h>

#include "demodulator.hpp"

namespace kvak::server {

class server {
public:
					server(const std::string &bind,
						   std::vector<demodulator> &demods);

private:
	capnp::EzRpcServer ezrpc;

};

}
