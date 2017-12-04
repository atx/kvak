
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <iostream>
#include <thread>


#include "log.hpp"
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
	kvak::log::info << "Server starting up";
	kj::NEVER_DONE.wait(this->ezrpc.getWaitScope());
	kvak::log::info << "Server finished";
}

}
