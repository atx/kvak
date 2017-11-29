#! /usr/bin/env python3

import argparse
import pathlib as path
import pyqtgraph as pg
import struct
import time


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
    parser.add_argument(
        "-s", "--sleep",
        type=float,
        default=0.001,
    )
    args = parser.parse_args()

    pw = pg.plot()

    counter = args.limit
    with args.input.open("rb") as fin:
        border = 0.01
        while fin:
            data = fin.read(counter * 4)
            floats = struct.unpack("{}f".format(len(data) // 4), data)
            i, q = floats[::2], floats[1::2]
            pw.plot(
                i, q, clear=True, pen=None,
                symbol="o", symbolBrush="FF0"
            )

            border = max(max(i), max(i), abs(min(i)), abs(min(q)), border)
            pw.setXRange(-border, border)
            pw.setYRange(-border, border)

            pg.QtGui.QApplication.processEvents()

            time.sleep(args.sleep)
