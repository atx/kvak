#! /usr/bin/env python3

import argparse
import collections
import colorama
import functools
import pathlib
import re
import subprocess
import sys
import tempfile


TestCase = collections.namedtuple(
    "TestCase", [
        "filename",
        "expected_crc_fail", "expected_crc_ok",
        "reference_crc_fail", "reference_crc_ok",
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
    cases = [
        TestCase(
            "data.1.cf32",
            4313, 24409,
            3275, 14818,
            "77f6354a9cb8637b249749ca02cb1c6b"
        ),
        TestCase(
            "data.2.cf32",
            3156, 71825,
            3090, 43268,
            "d8babbf8c1f55087e45501406d7fd1c2"
        ),
        TestCase(
            "data.3.cf32",
            4417, 27494,
            4563, 19217,
            "9f731ac58a3d7405a79e5681ede70d1f"
        ),
        TestCase(
            "data.4.cf32",
            2027, 119063,
            2027, 119071,
            "99fbdf2dedf97c32b0871f7affa8c9f7"
        ),
        TestCase(
            "data.5.cf32",
            803, 106561,
            1109, 98266,
            "51ac2db3ba00094a2930cfcca3b3d790"
        ),
        TestCase(
            "data.6.cf32",
            3488, 97991,
            3518, 98150,
            "362b06e7766bd9da94c9b71b8daee8e7"
        ),
        TestCase(
            "data.7.cf32",
            4689, 39106,
            1091, 6231,
            "5a3b79c97326769daf7235449476215b"
        ),
        TestCase(
            "data.8.cf32",
            824, 118364,
            906, 111220,
            "626600ac153197da87f479e6c2b7d036"
        ),
    ]

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--verify",
        action="store_true"
    )
    args = parser.parse_args()

    # TODO: Make this smarter
    base_dir = pathlib.Path(__file__).absolute().parent
    kvak_bin = base_dir.parent / "build/kvak"
    if not kvak_bin.exists():
        print("Couldn't find the kvak binary in {}".format(kvak_bin))
        sys.exit(1)
    print_info("Using kvak binary from {}".format(kvak_bin))

    data_dir = base_dir / "data"

    regex_fail = re.compile(b"CRC COMP.*WRONG")
    regex_ok = re.compile(b"CRC COMP.*OK")

    with tempfile.NamedTemporaryFile() as fdemod:
        for case in cases:
            data_file = data_dir / case.filename
            if not data_file.exists():
                print_fail("Failed to find input file {}".format(data_file))
                print_fail(
                    "Unfortunately, as the data comes from a public network, "
                    "adding it to the repository would expose me to legal risk "
                    "*wink wink*"
                )
                continue

            if args.verify:
                print_info("Running md5sum on {}".format(data_file))
                md5sum = subprocess.check_output(
                    ["md5sum", str(data_file)]
                ).split()[0].decode()
                if md5sum != case.md5:
                    print_fail("MD5 mismatch detected({} != {})".format(md5sum, case.md5))

            print_info("Running kvak on {}".format(data_file))
            subprocess.check_output(
                [str(kvak_bin), "-i", str(data_file), "-o", fdemod.name,
                 "-b", "false"]
            )
            print_info("Running tetra-rx on {}".format(fdemod.name))
            osmotetra_output = subprocess.check_output(
                ["tetra-rx", fdemod.name],
                stderr=subprocess.DEVNULL,  # tetra-rx outputs a lot of crap
            )

            crc_fail_n = len(regex_fail.findall(osmotetra_output))
            crc_ok_n = len(regex_ok.findall(osmotetra_output))
            d_fail = crc_fail_n - case.expected_crc_fail
            d_ok = crc_ok_n - case.expected_crc_ok
            passed = d_ok >= 0 and d_fail >= -d_ok

            print_fn = print_ok if passed else print_fail
            print_fn("Found {: 7d} wrong (expected {: 7d}, reference {: 7d})"
                     .format(crc_fail_n, case.expected_crc_fail, case.reference_crc_fail))
            print_fn("Found {: 7d} ok    (expected {: 7d}, reference {: 7d})"
                     .format(crc_ok_n, case.expected_crc_ok, case.reference_crc_ok))
            if passed and (d_ok != 0 or d_fail != 0):
                print_warn("Measured values don't match expectations")
