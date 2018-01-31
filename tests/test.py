#! /usr/bin/env python3

import argparse
import collections
import colorama
import concurrent.futures
import functools
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile


TestCase = collections.namedtuple(
    "TestCase", [
        "filename",
        "expected_crc_ok", "reference_crc_ok",
        "md5"
    ]
)


def colored_print(color):
    @functools.wraps(print)
    def ret(value, *args, **kwargs):
        print(colorama.Style.BRIGHT + color + str(value), *args, colorama.Style.RESET_ALL, **kwargs)
    return ret


print_info = colored_print(colorama.Fore.LIGHTBLUE_EX)
print_ok = colored_print(colorama.Fore.LIGHTGREEN_EX)
print_warn = colored_print(colorama.Fore.LIGHTYELLOW_EX)
print_fail = colored_print(colorama.Fore.LIGHTRED_EX)


if __name__ == "__main__":
    base_dir = pathlib.Path(__file__).absolute().parent

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--verify",
        action="store_true"
    )
    parser.add_argument(
        "-t", "--test-set",
        type=pathlib.Path,
        default=(base_dir / "tests.json")
    )
    parser.add_argument(
        "-d", "--data-dir",
        type=pathlib.Path,
        default=(base_dir / "data")
    )
    parser.add_argument(
        "--update",
        action="store_true",
    )
    parser.add_argument(
        "--kvak-output",
        action="store_true",
    )
    parser.add_argument(
        "-j", "--jobs",
        default=os.cpu_count(),
    )
    parser.add_argument(
        "-k", "--kvak",
        type=pathlib.Path,
        default=(base_dir.parent / "build/kvak")
    )
    args = parser.parse_args()

    with args.test_set.open("r") as fin:
        cases = [TestCase(**d) for d in json.load(fin)]

    # TODO: Make this smarter
    if not args.kvak.exists():
        print("Couldn't find the kvak binary in {}".format(args.kvak))
        sys.exit(1)
    print_info("Using kvak binary from {}".format(args.kvak))

    data_dir = base_dir / "data"

    regex_ok = re.compile(b"CRC COMP.*OK")

    def make_full_path(case):
        return data_dir / case.filename

    for case in cases:
        data_file = make_full_path(case)

        if not data_file.exists():
            print_fail("Failed to find input file {}".format(data_file))
            print_fail(
                "Unfortunately, as the data comes from a public network, "
                "adding it to the repository would expose me to legal risk "
                "*wink wink*"
            )
            sys.exit(1)

        if not args.verify:
            continue

        print_info("Running md5sum on {}".format(data_file))
        md5sum = subprocess.check_output(
            ["md5sum", str(data_file)]
        ).split()[0].decode()
        if md5sum != case.md5:
            print_fail("{}: MD5 mismatch detected({} != {})"
                       .format(case.filename, md5sum, case.md5))
        else:
            print_ok("{}: MD5 OK".format(case.filename))

    def run_test(data_file):
        with tempfile.NamedTemporaryFile() as fdemod:
            print_info("Running kvak on {}".format(data_file))
            subprocess.check_output(
                [str(args.kvak), "-i", str(data_file), "-o", fdemod.name,
                 "-b", "false"],
                stderr=(subprocess.STDERR if args.kvak_output else subprocess.DEVNULL)
            )
            print_info("Running tetra-rx on {}".format(fdemod.name))
            osmotetra_output = subprocess.check_output(
                ["tetra-rx", fdemod.name],
                stderr=subprocess.DEVNULL,  # tetra-rx outputs a lot of crap
            )

            return len(regex_ok.findall(osmotetra_output))

    total_ok = 0
    total_expected_ok = 0
    total_reference_ok = 0

    cases_new = []

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        result_gen = executor.map(run_test, (make_full_path(c) for c in cases))
        for case, crc_ok_n in zip(cases, result_gen):

            total_ok += crc_ok_n
            total_expected_ok += case.expected_crc_ok
            total_reference_ok += case.reference_crc_ok

            cases_new.append(TestCase(
                filename=case.filename,
                expected_crc_ok=crc_ok_n,
                reference_crc_ok=case.reference_crc_ok,
                md5=case.md5,
            ))

            print_fn = print_ok if crc_ok_n >= case.expected_crc_ok else print_fail
            print_fn(
                "{: <16}-> found {: 7d} ok    (expected {: 7d} ({:.3f}), reference {: 7d} ({:.3f}))"
                .format(case.filename, crc_ok_n,
                        case.expected_crc_ok, crc_ok_n / case.expected_crc_ok,
                        case.reference_crc_ok, crc_ok_n / case.reference_crc_ok)
            )
            if crc_ok_n != case.expected_crc_ok:
                print_warn("Measured values don't match expectations")

    print_fn = print_ok if total_ok >= total_expected_ok else print_fail
    print_fn(
        "In total, found {: 7d} ok (expected {: 7d} ({:.3f}), reference {: 7d} ({:.3f}))"
        .format(total_ok,
                total_expected_ok, total_ok / total_expected_ok,
                total_reference_ok, total_ok / total_reference_ok)
    )

    if args.update:
        with args.test_set.open("w") as fout:
            json.dump([c._asdict() for c in cases_new], fout, indent=2)
