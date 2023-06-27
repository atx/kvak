
#pragma once

#include <optional>
#include <functional>
#include <string>
#include <ostream>
#include <string.h>

namespace kvak::log {

// TODO: This (everything here) is somewhat fragile

class perror_class {};
extern perror_class perror;

static inline std::ostream &operator<<(std::ostream &os, const perror_class pr)
{
	return os << " : " << strerror(errno);
}

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
		log_ender()
		{
		}

		log_ender(std::ostream &stream)
			:
			stream(stream)
		{
		}

		~log_ender()
		{
			if (this->stream) {
				this->stream.value().get() << "\x1b[0m" << std::endl;
			}
		}

		template<typename T>
		log_ender &operator<<(T &&val)
		{
			if (this->stream) {
				this->stream.value().get() << val;
			}
			return *this;
		}

	private:
		std::optional<std::reference_wrapper<std::ostream>> stream;
	};

	void mute()
	{
		this->stream.reset();
	}

	void redirect(std::ostream &stream)
	{
		this->stream = stream;
	}

	template<typename T>
	log_ender operator<<(T &&val)
	{
		// TODO: This requires some major unhackify-ing
		if (!this->stream) {
			return log_ender();
		}
		return log_ender(this->stream.value().get() << this->prefix << val);
	}


private:
	std::string prefix;
	std::optional<std::reference_wrapper<std::ostream>> stream;
};

extern log_wrapper debug;
extern log_wrapper info;
extern log_wrapper error;

}
