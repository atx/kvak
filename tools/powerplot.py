#! /usr/bin/env python3

import argparse
import numpy


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-f", "--file",
        required=True,
    )
    parser.add_argument(
        "-c", "--chunk-size",
        default=1024,
        type=int,
    )
    args = parser.parse_args()

    with open(args.file, "rb") as fin:
        while True:
            samples = numpy.fromfile(fin, dtype=numpy.float32,
                                     count=args.chunk_size)
            if not len(samples):
                break
            print(numpy.average(abs(samples)))
