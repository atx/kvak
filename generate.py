#! /usr/bin/env python3

import argparse
import itertools
import math
import pathlib
import random
import struct

# http://rfmw.em.keysight.com/wireless/helpfiles/89600b/webhelp/subsystems/digdemod/content/dlg_digdemod_fmt_pi4dqpsk.htm
SYMBOLS = [0b00, 0b01, 0b10, 0b11]
SYMBOL_TO_PHASE = {
    0b00:  1 * math.pi / 4,
    0b01:  3 * math.pi / 4,
    0b10: -1 * math.pi / 4,
    0b11: -3 * math.pi / 4,
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-o", "--output",
        type=pathlib.Path,
        required=True,
    )
    parser.add_argument(
        "-d", "--data",
        type=pathlib.Path,
        required=True,
    )
    parser.add_argument(
        "-n", "--nsymbols",
        type=int,
        required=True,
    )
    parser.add_argument(
        "-r", "--repeat-length",
        type=int,
        default=16,
    )
    parser.add_argument(
        "-u", "--unpack",
        action="store_true",
    )
    args = parser.parse_args()

    complex_struct = struct.Struct("ff")
    pattern_symbols = [random.choice(SYMBOLS) for _ in range(args.repeat_length)]
    pattern = itertools.cycle(pattern_symbols)
    print("pattern = ", " ".join("%02d" % s for s in pattern_symbols))

    with args.output.open("wb") as fout, args.data.open("wb") as fdata:
        phase = 0.0
        for _ in range(args.nsymbols):
            # This could be massively sped up with numpy
            symbol = next(pattern)
            phase_inc = SYMBOL_TO_PHASE[symbol]
            if args.unpack:
                fdata.write(bytes([symbol >> 1, symbol & 0b1]))
            else:
                fdata.write(bytes([symbol]))
            for _ in range(2):
                phase = (phase + phase_inc/2) % (2 * math.pi)
                i, q = math.cos(phase), math.sin(phase)
                bs = complex_struct.pack(i, q)
                fout.write(bs)
