
#include <string>
#include <ostream>

#pragma once

namespace kvak::log {

// TODO: This is somewhat fragile

class log_wrapper {
public:
	log_wrapper(const std::string &prefix, std::ostream &stream)
		:
		prefix(prefix),
		stream(stream)
	{
	}

	class log_ender {
	public:
		log_ender(std::ostream &stream)
			:
			stream(stream)
		{
		}

		~log_ender()
		{
			this->stream << "\x1b[0m" << std::endl;
		}

		template<typename T>
		log_ender &operator<<(T &&val)
		{
			this->stream << val;
			return *this;
		}

	private:
		std::ostream &stream;
	};

	template<typename T>
	log_ender operator<<(T &val)
	{
		return log_ender(this->stream << this->prefix << val);
	}


private:
	std::string prefix;
	std::ostream &stream;
};

extern log_wrapper debug;
extern log_wrapper info;
extern log_wrapper error;

}
