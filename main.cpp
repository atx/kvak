
#include <argp.h>
#include <complex>
#include <cstdio>
#include <experimental/filesystem>
#include <iostream>
#include <mutex>
#include <thread>
#include <sys/types.h>
#include <sys/stat.h>

#include "agc.hpp"
#include "demodulator.hpp"
#include "log.hpp"
#include "server.hpp"
#include "utils.hpp"

namespace filesystem = std::experimental::filesystem;

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
		unpack(true),
		bind("127.0.0.1:6677"),
		loop(false)
	{
	}

	enum arg_ids {
		DONT_UNPACK = 1000,
		LOOP = 1001,
	};

	unsigned int nchannels;
	filesystem::path input_path;
	filesystem::path output_path;
	bool fifo_mode;
	std::size_t chunk_size;
	bool unpack;
	std::string bind;
	bool loop;
};


static struct argp_option argp_options[] = {
	{ "nchannels",	'n',	"NCHANNELS",	0,		"Number of channels",	0 },
	{ "input",		'i',	"INPUT",		0,		"Input file path",		0 },
	{ "output",		'o',	"OUTPUT",		0,
		"Output file paths (use %d for channel number substitution)",		0 },
	{ "fifo",		'f',	nullptr,		0,
		"Explicitly create output FIFOs instead of files",					0 },
	{ "chunk-size", 'c',	"CHUNK",		0,		"Chunk size",			0 },
	{ "dont-unpack", arguments::arg_ids::DONT_UNPACK,
		nullptr,		0,		"Don't unpack the output symbols", 0 },
	{ "bind",		'b',	"ADDR",			0,		"Bind to ADDR:PORT",	0 },
	{ "loop",		 arguments::arg_ids::LOOP,
		nullptr,		0,		"Loop the input file", 0 },
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
	case arguments::arg_ids::DONT_UNPACK:
		args->unpack = false;
		break;
	case arguments::arg_ids::LOOP:
		args->loop = true;
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

	// TODO
	args.chunk_size = 1;
	std::vector<std::complex<float>> input_buffer;
	input_buffer.resize(args.chunk_size * args.nchannels);
	//std::vector<std::uint8_t> output_buffer;
	//output_buffer.resize(args.chunk_size * args.nchannels *
	//					 kvak::demodulator::bits_per_symbol);

	std::FILE *input_file = std::fopen(args.input_path.c_str(), "r");
	if (input_file == nullptr) {
		kvak::log::error << "Failed to open the input file " << args.input_path
			<< kvak::log::perror;
		return EXIT_FAILURE;
	}

	std::vector<kvak::demodulator> demods(args.nchannels, kvak::demodulator());
	std::vector<std::FILE *> output_files;
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

		std::FILE *file = std::fopen(name.c_str(), "w");
		if (file == nullptr) {
			kvak::log::error << "Failed to open the output file "
				<< args.output_path << kvak::log::perror;
			return EXIT_FAILURE;
		}
		kvak::log::info << "Opened file " << name << " for output";
		output_files.push_back(file);
	}

	// Start up the server
	std::mutex server_mtx;  // We have one global mutex for all channels
	std::thread server_thread([&] () {
		// This is annoyingly hacky
		if (args.bind != "false") {
			kvak::server::server(args.bind, demods, server_mtx);
		}
	});

	kvak::agc agc(args.nchannels);

	while (true) {
		std::size_t len = std::fread(
			input_buffer.data(),
			sizeof(input_buffer[0]),
			input_buffer.size(),
			input_file
		);
		if (len != input_buffer.size()) {
			if (args.loop) {
				std::fseek(input_file, 0, SEEK_SET);
				kvak::log::debug << "Looping";
				continue;
			}
			kvak::log::error << "Short read (" << len << "), quitting...";
			break;
		}

		agc.push_samples(input_buffer.data(), 1);

		std::lock_guard<std::mutex> lock(server_mtx);
		auto iter = input_buffer.cbegin();
		for (unsigned int n = 0; n < args.nchannels; n++) {
			std::optional<std::uint8_t> ret = demods[n].push_sample(*iter++);
			if (ret) {
				std::uint8_t val = ret.value();
				if (args.unpack) {
					uint8_t data[2] = {
						kvak::utils::bit(val, 1), kvak::utils::bit(val, 0)
					};
					std::fwrite(data, sizeof(data), 1, output_files[n]);
				} else {
					std::fwrite(&val, sizeof(val), 1, output_files[n]);
				}
			}
		}
	}

	if (args.bind == "false") {
		// Uuuh, we have to gracefully kill the thread somehow in the other case
		server_thread.join();
	}
}
