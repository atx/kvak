
#include <argp.h>
#include <complex>
#include <cstdio>
#include <experimental/filesystem>
#include <fcntl.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>

#include "agc.hpp"
#include "demodulator.hpp"
#include "log.hpp"
#include "main.hpp"
#include "server.hpp"
#include "utils.hpp"

namespace filesystem = std::experimental::filesystem;

// Implementation of channel -- this should probably get moved to another file TODO

namespace kvak {


channel::~channel()
{
	if (this->file != nullptr) {
		std::fclose(this->file);
	}
}


float channel::get_power()
{
	return this->agc_all.get_channel_power(this->id);
}


void channel::mute(bool m)
{
	this->muted= m;
}


bool channel::is_muted()
{
	return this->muted;
}


void channel::push_sample(std::complex<float> sample)
{
	std::optional<std::uint8_t> ret = this->demod.push_sample(sample);
	if (!ret) {
		return;
	}

	std::uint8_t val = ret.value();
	// TODO: Do we want not-unpacking back?
	this->out_data[this->out_counter++] =
			kvak::utils::bit(val, 1);
	this->out_data[this->out_counter++] =
			kvak::utils::bit(val, 0);
}


void channel::flush()
{
	unsigned int to_write = this->out_counter;
	this->out_counter = 0;  // Whatever happens, we have to drop the buffer

	// This is done to prevent blocking on channels which don't have any other
	// side reading from the FIFO - useful for debugging/tetra-rx crashing etc.
	if (this->file == nullptr) {
		// Note that O_CREAT should only apply for non-FIFO mode, as FIFOs
		// are alreadt created
		int fd = open(this->output_path.c_str(), O_WRONLY | O_NONBLOCK | O_CREAT, 0660);
		if (fd < 0) {
			// There is noone at the reading end, try next time
			if (errno != ENXIO) {
				kvak::log::info << "Failed to open " << kvak::log::perror;
			}
			return;
		}
		kvak::log::debug << "Opened file " << this->output_path << " for output";
		this->file = fdopen(fd, "wb");
	}

	// If the other side hung up, we are going to get EPIPE here
	std::fwrite(
		this->out_data.data(),
		sizeof(this->out_data[0]),
		to_write,
		this->file
	);
}

}

// argp stuff

const char *argp_program_version = "kvak 0.1";
const char *argp_program_bug_address = "Josef Gajdusek <atx@atx.name>";

static char argp_doc[] = "pi/4-DQPSK multi-channel demodulator";


struct arguments {
	arguments()
		:
		nchannels(1),
		input_path(""),
		output_path(""),
		fifo_mode(false),
		chunk_size(1024),
		bind("127.0.0.1:6677"),
		loop(false),
		verbose(false)
	{
	}

	enum arg_ids {
		LOOP = 1001,
	};

	unsigned int nchannels;
	filesystem::path input_path;
	filesystem::path output_path;
	bool fifo_mode;
	std::size_t chunk_size;
	std::string bind;
	bool loop;
	bool verbose;
};


static struct argp_option argp_options[] = {
	{ "nchannels",	'n',	"NCHANNELS",	0,		"Number of channels",	0 },
	{ "input",		'i',	"INPUT",		0,		"Input file path",		0 },
	{ "output",		'o',	"OUTPUT",		0,
		"Output file paths (use %d for channel number substitution)",		0 },
	{ "fifo",		'f',	nullptr,		0,
		"Explicitly create output FIFOs instead of files",					0 },
	{ "chunk-size", 'c',	"CHUNK",		0,		"Chunk size",			0 },
	{ "bind",		'b',	"ADDR",			0,		"Bind to ADDR:PORT",	0 },
	{ "loop",		 arguments::arg_ids::LOOP,
		nullptr,		0,		"Loop the input file", 0 },
	{ "verbose",	'v',	nullptr,		0,		"Enable verbose debugging",	0 },
	{ nullptr,		0,		nullptr,		0,		nullptr,				0 },
};


static error_t parse_opt(int key, char *arg_, struct argp_state *state)
{
	arguments *args = static_cast<arguments *>(state->input);
	std::string arg(arg_ != nullptr ? arg_ : "");

#define FAIL(with, ...) argp_failure(state, EXIT_FAILURE, 0, with, ##__VA_ARGS__)

	switch (key) {
	case 'n':
		args->nchannels = std::stoul(arg);
		break;
	case 'i':
		args->input_path = arg;
		break;
	case 'o':
		args->output_path = arg;
		break;
	case 'f':
		args->fifo_mode = true;
		break;
	case 'c':
		args->chunk_size = std::stoul(arg);
		if (args->chunk_size == 0) {
			FAIL("Invalid chunk size specified (0)");
		}
		break;
	case 'b':
		args->bind = arg;
		// TODO: Check the argument maybe?
		break;
	case arguments::arg_ids::LOOP:
		args->loop = true;
		break;
	case 'v':
		args->verbose = true;
		break;
	case ARGP_KEY_END:
		if (!args->input_path.has_filename()) {
			FAIL("No input path specified");
		}
		if (!args->output_path.has_filename()) {
			FAIL("No output path specified");
		}
		if (args->nchannels > 1 &&
				args->output_path.string().find("%d") == std::string::npos) {
			FAIL("No channel number placeholder (%%d) specified in the output path string '%s'",
				 args->output_path.c_str());
		}
	}

#undef FAIL

	return 0;
}


static argp argp_parser = {
	.options = argp_options,
	.parser = parse_opt,
	.args_doc = nullptr,
	.doc = argp_doc,
	.children = nullptr,
	.help_filter = nullptr,
	.argp_domain = nullptr,
};


int main(int argc, char *argv[])
{
	struct arguments args;
	argp_parse(&argp_parser, argc, argv, 0, nullptr, &args);

	if (!args.verbose) {
		kvak::log::debug.mute();
	}

	// Setup buffers
	std::vector<std::complex<float>> input_buffer(args.chunk_size * args.nchannels);

	// Open input file - note that we can block for a considerable amount
	// of time if this is a FIFO
	std::FILE *input_file = std::fopen(args.input_path.c_str(), "r");
	if (input_file == nullptr) {
		kvak::log::error << "Failed to open the input file " << args.input_path
			<< kvak::log::perror;
		return EXIT_FAILURE;
	}

	kvak::agc agc(args.nchannels);
	std::vector<kvak::channel> channels;

	// Open output files (mkfifo if necesarry, again, we can block here in
	// that case until the other side connects)
	for (unsigned int n = 0; n < args.nchannels; n++) {
		std::string name = kvak::utils::replace_first(
				args.output_path, "%d", std::to_string(n));

		if (args.fifo_mode && !filesystem::is_fifo(name)) {
			int ret = mkfifo(name.c_str(), S_IRUSR | S_IWUSR);
			if (ret) {
				kvak::log::error << "Failed to create FIFO " << name
					<< kvak::log::perror;
				return EXIT_FAILURE;
			}
		}

		channels.push_back(kvak::channel(n, agc, name, args.chunk_size * 2));
	}
	kvak::log::info << "Opened files " << args.output_path << " [0-"
		<< args.nchannels << "]";

	// Start up the server
	std::mutex server_mtx;  // We have one global mutex for all channels
	std::thread server_thread([&] () {
		// This is annoyingly hacky
		if (args.bind != "false") {
			kvak::server::server(args.bind, channels, server_mtx);
		}
	});

	while (true) {
		// Note that the input floats are read in _machine byte order_
		// This is intentional
		std::size_t len = std::fread(
			input_buffer.data(),
			sizeof(input_buffer[0]),
			input_buffer.size(),
			input_file
		);
		if (len == 0) {
			if (std::feof(input_file)) {
				if (args.loop) {
					std::fseek(input_file, 0, SEEK_SET);
					kvak::log::debug << "Looping";
					continue;
				}
				kvak::log::info << "EOF reached, quitting";
				break;
			}
			kvak::log::error << "Read error, quitting...";
			break;
		}

		agc.push_samples(input_buffer.data(), len / args.nchannels);

		std::lock_guard<std::mutex> lock(server_mtx);

		#pragma omp parallel for
		for (unsigned int n = 0; n < args.nchannels; n++) {
			auto iter = input_buffer.cbegin() + n;
			kvak::channel &ch = channels[n];
			if (ch.is_muted()) {
				// TODO: As the running set is not updated very often, it's
				// probably for the best to make a separate "running" list
				// instead of skipping like this
				continue;
			}
			for (unsigned int i = 0; i < len / args.nchannels; i++) {
				ch.push_sample(*iter);
				iter += args.nchannels;
			}
		}

		for (auto &ch : channels) {
			ch.flush();
		}

		if (len % args.nchannels != 0) {
			// At this point, we either quit after the next read or loop if --loop
			// got passed
			kvak::log::error << "Dropping short read of " << (len % args.nchannels);
		}
	}

	if (args.bind == "false") {
		// Uuuh, we have to gracefully kill the thread somehow in the other case
		server_thread.join();
	}
}
