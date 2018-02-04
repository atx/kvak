#! /usr/bin/env python3

import argparse
import os
import subprocess
import sys
import time


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-m", "--min-threads",
        type=int,
        default=1,
    )
    parser.add_argument(
        "-x", "--max-threads",
        type=int,
        default=os.cpu_count(),
    )
    parser.add_argument(
        "-t", "--tries",
        type=int,
        default=5,
    )
    parser.add_argument(
        "-k", "--kvak-bin",
        required=True,
    )
    parser.add_argument(
        "-n", "--nchannels",
        type=int,
        required=True,
    )
    parser.add_argument(
        "-i", "--input-file",
        required=True,
    )
    parser.add_argument(
        "--kvak-output",
        action="store_true",
    )
    args = parser.parse_args()

    speeds = {v: [] for v in range(args.min_threads, args.max_threads + 1)}

    for ntry in range(args.tries):
        print("#{}".format(ntry), file=sys.stderr)
        for thrcnt in speeds.keys():

            t_start = time.monotonic()
            subprocess.Popen(
                [args.kvak_bin,
                 "-i", args.input_file,
                 "-n", str(args.nchannels),
                 "-o", "/dev/null",
                 "-b", "false",
                 "--force-single-file"],
                stderr=(None if args.kvak_output else subprocess.DEVNULL),
                env={"OMP_NUM_THREADS": str(thrcnt)}
            ).wait()
            t_diff = time.monotonic() - t_start

            print(" {: 2d} -> {:.4f}".format(thrcnt, t_diff), file=sys.stderr)
            speeds[thrcnt].append(t_diff)

    result_avgs = {
        v: (sum(r) / len(r)) for v, r in speeds.items()
    }

    for v, r in result_avgs.items():
        print("{}\t{}".format(v, r))
