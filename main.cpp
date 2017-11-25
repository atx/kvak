
#include <argp.h>
#include <complex>
#include <cstdio>
#include <experimental/filesystem>
#include <iostream>

#include "demodulator.hpp"

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
		chunk_size(1024)
	{
	}

	unsigned int nchannels;
	filesystem::path input_path;
	filesystem::path output_path;
	bool fifo_mode;
	std::size_t chunk_size;
};


static struct argp_option argp_options[] = {
	{ "nchannels",	'n',	"NCHANNELS",	0,		"Number of channels",	0 },
	{ "input",		'i',	"INPUT",		0,		"Input file path",		0 },
	{ "output",		'o',	"OUTPUT",		0,
		"Output file paths (use %d for channel number substitution)",		0 },
	{ "fifo",		'f',	nullptr,		0,
		"Explicitly create output FIFOs instead of files",					0 },
	{ "chunk-size", 'c',	"CHUNK",		0,		"Chunk size",			0 },
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
		std::cerr << "Failed to open the input file " << args.input_path << std::endl;
		return EXIT_FAILURE;
	}

	std::cout << std::fixed;
	std::cout.precision(4);

	std::vector<kvak::demodulator> demods(args.nchannels, kvak::demodulator());
	std::vector<std::FILE *> output_files;
	for (unsigned int n = 0; n < args.nchannels; n++) {
		std::string name = args.output_path;  // TODO: Implement wildcard replace
		std::FILE *file = std::fopen(name.c_str(), "w");
		if (file == nullptr) {
			std::cerr << "Failed to open the output file " << args.output_path << std::endl;
			return EXIT_FAILURE;
		}
		output_files.push_back(file);
	}

	while (true) {
		std::size_t len = std::fread(
			input_buffer.data(),
			sizeof(input_buffer[0]),
			input_buffer.size(),
			input_file
		);
		if (len != input_buffer.size()) {
			std::cerr << "Short read (" << len << "), quitting..." << std::endl;
			break;
		}

		auto iter = input_buffer.cbegin();
		for (unsigned int n = 0; n < args.nchannels; n++) {
			std::optional<std::uint8_t> ret = demods[n].push_sample(*iter++);
			if (ret) {
				std::uint8_t val = ret.value();
				std::fwrite(&val, sizeof(val), 1, output_files[n]);
			}
		}
	}
}
