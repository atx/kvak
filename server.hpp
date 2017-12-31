
#pragma once

#include <capnp/ez-rpc.h>
#include <mutex>

#include "main.hpp"

namespace kvak::server {

class server {
public:
					server(const std::string &bind,
						   std::vector<channel> &channels,
						   std::mutex &mutex);

private:
	capnp::EzRpcServer ezrpc;

};

}
