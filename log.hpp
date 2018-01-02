
#pragma once

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
		stream(stream),
		muted(false)
	{
	}

	class log_ender {
	public:
		log_ender(std::ostream &stream, bool muted)
			:
			stream(stream),
			muted(muted)
		{
		}

		~log_ender()
		{
			if (!this->muted) {
				this->stream << "\x1b[0m" << std::endl;
			}
		}

		template<typename T>
		log_ender &operator<<(T &&val)
		{
			if (!this->muted) {
				this->stream << val;
			}
			return *this;
		}

	private:
		std::ostream &stream;
		bool muted;
	};

	void mute(bool b)
	{
		this->muted = b;
	}

	void redirect(std::ostream &stream)
	{
		this->stream = stream;
	}

	template<typename T>
	log_ender operator<<(T &&val)
	{
		// TODO: This requires some major unhackify-ing
		if (this->muted) {
			return log_ender(this->stream.get(), true);
		}
		return log_ender(this->stream.get() << this->prefix << val, false);
	}


private:
	std::string prefix;
	std::reference_wrapper<std::ostream> stream;
	bool muted;
};

extern log_wrapper debug;
extern log_wrapper info;
extern log_wrapper error;

}
