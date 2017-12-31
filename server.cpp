
#include <capnp/ez-rpc.h>
#include <capnp/message.h>
#include <iostream>
#include <thread>


#include "log.hpp"
#include "schema.capnp.h"
#include "server.hpp"


namespace kvak::server {

class ChannelImpl : public Service::Channel::Server {
public:

	ChannelImpl(kvak::channel &channel, std::mutex &mutex)
		:
		channel(channel),
		mutex(mutex)
	{
	}

	kj::Promise<void> getInfo(Service::Channel::Server::GetInfoContext context) override
	{

		auto results = context.getResults();
		results.initInfo();
		auto info = results.getInfo();

		// This is slightly iffy - The client is going to do something like:
		// for ch in channels:
		//     ch.getInfo()
		// Releasing and relocking the mutex again and again, which could
		// possibly leading to very large latency
		std::lock_guard<std::mutex> lock(this->mutex);
		//auto chinfo = this->demod.get_info();
		info.setTimingOffset(this->channel.demod.get_timing_offset());
		info.setFrequencyOffset(this->channel.demod.get_frequency_offset());
		info.setPowerLevel(this->channel.get_power());  // Not yet implemented
		info.setIsMuted(this->channel.is_muted());

		return kj::READY_NOW;
	}

private:
	kvak::channel &channel;
	std::mutex &mutex;
};

class ServiceImpl : public Service::Server {
public:

	ServiceImpl(std::vector<kvak::channel> &channels, std::mutex &mutex)
		:
		start_time(std::chrono::steady_clock::now()),
		channels(channels),
		mutex(mutex)
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

	kj::Promise<void> listChannels(Service::Server::ListChannelsContext context) override
	{
		auto results = context.getResults();

		std::lock_guard<std::mutex> lock(this->mutex);
		results.initList(this->channels.size());
		auto list = results.getList();
		for (std::size_t i = 0; i < this->channels.size(); i++) {
			list.set(i, kj::heap<ChannelImpl>(channels[i], this->mutex));
		}

		return kj::READY_NOW;
	}

private:
	std::chrono::steady_clock::time_point start_time;
	std::vector<kvak::channel> &channels;
	std::mutex &mutex;
};


server::server(const std::string &bind,
			   std::vector<kvak::channel> &channels,
			   std::mutex &mutex)
	:
	ezrpc(kj::heap<ServiceImpl>(channels, mutex), bind)
{
	kvak::log::info << "Server starting up";
	kj::NEVER_DONE.wait(this->ezrpc.getWaitScope());
	kvak::log::info << "Server finished";
}

}
