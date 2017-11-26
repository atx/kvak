#! /usr/bin/env python3

import argparse
import pathlib as path
import struct

from matplotlib import pyplot


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-i", "--input",
        required=True,
        type=path.Path,
    )
    parser.add_argument(
        "-l", "--limit",
        type=int,
        default=10000
    )
    args = parser.parse_args()

    figure = pyplot.gcf()
    figure.show()
    figure.canvas.draw()

    counter = args.limit
    with args.input.open("rb") as fin:
        while fin:
            data = fin.read(counter * 4)
            floats = struct.unpack("{}f".format(len(data) // 4), data)
            counter -= len(floats)
            i, q = floats[::2], floats[1::2]
            pyplot.plot(i, q, linestyle="", marker="o")
            pyplot.xlim([-1.3, 1.3])
            pyplot.ylim([-1.3, 1.3])

            if counter <= 0:
                print("Rendering")
                counter = args.limit
                figure.canvas.draw()
                figure.clear()
