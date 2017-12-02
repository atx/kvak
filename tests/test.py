#! /usr/bin/env python3

import collections
import pathlib
import re
import subprocess
import sys
import tempfile


TestCase = collections.namedtuple("TestCase",
                                  ["filename", "expected_crc_fail",
                                   "expected_crc_ok"])


if __name__ == "__main__":
    cases = [
        TestCase(
            "data.1.cf32",
            4854,
            25714,
        )
    ]

    # TODO: Make this smarter
    base_dir = pathlib.Path(__file__).absolute().parent
    kvak_bin = base_dir.parent / "build/kvak"
    if not kvak_bin.exists():
        print("Couldn't find the kvak binary in {}".format(kvak_bin))
        sys.exit(1)

    data_dir = base_dir / "data"

    regex_fail = re.compile(b"CRC COMP.*WRONG")
    regex_ok = re.compile(b"CRC COMP.*OK")

    with tempfile.NamedTemporaryFile() as fdemod:
        for case in cases:
            data_file = data_dir / case.filename
            if not data_file.exists():
                print("Failed to find input file {}".format(data_file))
                continue

            print("Running kvak on {}".format(data_file))
            subprocess.check_output(
                [str(kvak_bin), "-i", str(data_file), "-o", fdemod.name]
            )
            print("Running tetra-rx on {}".format(fdemod.name))
            osmotetra_output = subprocess.check_output(
                ["tetra-rx", fdemod.name],
                stderr=subprocess.DEVNULL,  # tetra-rx outputs a lot of crap
            )

            crc_fail_n = len(regex_fail.findall(osmotetra_output))
            crc_ok_n = len(regex_ok.findall(osmotetra_output))

            print("Found {} wrong (expected {})".format(crc_fail_n, case.expected_crc_fail))
            print("Found {} ok    (expected {})".format(crc_ok_n, case.expected_crc_ok))
