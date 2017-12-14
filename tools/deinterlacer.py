#! /usr/bin/env python3

import argparse
import pathlib


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-n", "--ninput",
        required=True,
        type=int,
    )
    parser.add_argument(
        "-s", "--select",
        nargs="+",
        required=True,
        type=int,
    )
    parser.add_argument(
        "-i", "--input",
        required=True,
        type=pathlib.Path,
    )
    parser.add_argument(
        "-o", "--output",
        required=True,
        type=pathlib.Path,
    )
    args = parser.parse_args()

    filename = args.output.name
    if len(args.select) > 1 and "%d" not in filename:
        parser.error("Selected more than one channel and the output filename does not contain '%d'")
    directory = args.output.absolute().parent
    output_files = {c: directory / (filename.replace("%d", str(c))) for c in args.select}

    file_objs = {oc: of.open("wb") for oc, of in output_files.items()}
    with args.input.open("rb") as fin:
        while fin:
            length = args.ninput * 8
            data = fin.read(length)
            if len(data) != length:
                print("Short read of {}".format(length))
                break
            for c, f in file_objs.items():
                f.write(data[c*8:c*8+8])

    # Maybe finally: or something...
    for f in file_objs.values():
        f.close()
