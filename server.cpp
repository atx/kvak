
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <iostream>
#include <thread>


#include "schema.capnp.h"

#include "server.hpp"


namespace kvak::server {

class ServiceImpl : public Service::Server {
public:

	ServiceImpl()
		:
		start_time(std::chrono::steady_clock::now())
	{
	}

	kj::Promise<void> getInfo(Service::Server::GetInfoContext context) override
	{
		auto results = context.getResults();
		results.initInfo();
		auto info = results.getInfo();
		std::chrono::duration<double> diff =
			std::chrono::steady_clock::now() - this->start_time;
		info.setUptime(diff.count());
		return kj::READY_NOW;
	}

private:
	std::chrono::steady_clock::time_point start_time;
};


server::server(const std::string &bind,
			   std::vector<kvak::demodulator> &demods)
	:
	ezrpc(kj::heap<ServiceImpl>(), bind)
{
	std::cerr << "Server starting up" << std::endl;
	kj::NEVER_DONE.wait(this->ezrpc.getWaitScope());
	std::cerr << "Server finished" << std::endl;
}

}
