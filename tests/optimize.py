#! /usr/bin/env python3

import argparse
import concurrent.futures
import itertools
import numpy as np
import os
import pathlib
import random
import re
import subprocess
import tempfile
import time


regex_ok = re.compile(b"CRC COMP.*OK")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d", "--data-dir",
        type=pathlib.Path,
        required=True
    )
    parser.add_argument(
        "-k", "--kvak-binary",
        type=pathlib.Path,
        default=pathlib.Path("../build/kvak")
    )
    parser.add_argument(
        "--jobs",
        type=int,
        default=os.cpu_count()
    )
    parser.add_argument(
        "--explore-by",
        type=float,
        default=0.20
    )
    parser.add_argument(
        "--explore-decay",
        type=float,
        default=0.9
    )
    parser.add_argument(
        "--verbose",
        action="store_true"
    )
    parser.add_argument(
        "--population",
        type=int,
        default=30
    )
    parser.add_argument(
        "--survivors",
        type=int,
        default=10
    )
    args = parser.parse_args()

    gardner_base_p = 0.0000225
    gardner_base_i = 0.0000025
    costas_base_p = 0.00212494
    costas_base_i = 0.0000944085

    param_middle = np.array([gardner_base_p, gardner_base_i,
                             costas_base_p, costas_base_i])
    explore_steps = param_middle * args.explore_by

    all_params_to_score = []

    print_debug = print if args.verbose else (lambda *args, **kwargs: None)

    def score_parameter_set(g_p, g_i, c_p, c_i):

        def score_for_file(fdata):
            with tempfile.NamedTemporaryFile() as fdemod:
                print_debug(" kvak on", fdata, "to", fdemod.name)
                subprocess.check_output(
                    [str(args.kvak_binary),
                     "-i", str(fdata),
                     "-o", fdemod.name,
                     "-b", "false"],
                    env={
                        "KVAK_GARDNER_P": str(g_p),
                        "KVAK_GARDNER_I": str(g_i),
                        "KVAK_COSTAS_PHASE": str(c_p),
                        "KVAK_COSTAS_FREQ": str(c_i)
                    },
                    stderr=subprocess.DEVNULL
                )
                print_debug(" running tetra-rx on", fdemod.name, "from", fdata)
                osmotetra_output = subprocess.check_output(
                    ["tetra-rx", fdemod.name],
                    stderr=subprocess.DEVNULL
                )
                crc_ok_n = len(regex_ok.findall(osmotetra_output))
                print_debug(" score@{} = {}".format(fdata, crc_ok_n))
                return crc_ok_n

        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
            files = list(args.data_dir.glob("*.cf32"))
            return sum(executor.map(score_for_file, files))

    param_low = param_middle - explore_steps
    param_high = param_middle + explore_steps

    population = [param_middle]  # For reference
    population.extend(
        [np.random.normal() * (param_high - param_low) + param_low
         for _ in range(args.population)]
    )

    best_score = 0  # For logging
    best_history = []

    for round_no in itertools.count():
        print("ROUND #{}".format(round_no))

        round_scores = []

        for i, el in enumerate(population):
            print(" #{} @{} {}".format(round_no, i, el))

            t_start = time.monotonic()

            g_p, g_i, c_p, c_i = el
            score = score_parameter_set(g_p, g_i, c_p, c_i)

            t_delta = time.monotonic() - t_start
            print("  score = {}".format(score))
            if best_score:
                print("  relative score = {}".format(score / best_score))
            print("  t_delta = {}".format(t_delta))
            print("  Expected to finish this round in {:.1f} minutes".format(t_delta * (len(population) - i - 1) / 60))

            round_scores.append((score, el))

        round_scores.sort(reverse=True, key=lambda x: x[0])
        print(" Current best = {}".format(round_scores[0]))
        best_history.append(round_scores[0])
        print("  History {}".format([x[0] for x in best_history]))

        if round_scores[0][0] > best_score:
            best_score = round_scores[0][0]

        # Make new population
        scores, population = map(list, zip(*round_scores[:args.survivors]))

        while len(population) < args.population:
            src1 = random.choice(population)
            src2 = random.choice(population)
            combo = np.random.randint(2, size=[len(src1)])
            new = src1*combo + src2*(1-combo)
            new += np.random.normal(size=explore_steps.size) * explore_steps

            population.append(new)

        explore_steps *= args.explore_decay
