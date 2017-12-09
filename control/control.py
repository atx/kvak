#! /usr/bin/env python3

# TODO: Make this a standard reasonable python package

import argparse
import capnp
import pathlib
import time

# Uuh, don't forget to embed this in the python package later TODO
kvak = capnp.load(str(pathlib.Path(__file__).absolute().parent.parent / "schema.capnp"))


def do_watch(service, args):
    channels = service.listChannels().wait().list
    while True:
        for i, ch in enumerate(channels):
            info = ch.getInfo().wait().info
            print("Channel {}".format(i))
            print(" Frequency offset = {:.4f}".format(info.frequencyOffset))
            print(" Timing offset =    {:.4f}".format(info.timingOffset))
            print(" Power level =      {:.4f}".format(info.powerLevel))
            print(" Muted =            {}".format(info.isMuted))
        # TODO: Compensate
        time.sleep(args.refresh_rate)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-s", "--server",
        required=True,
    )
    subparsers = parser.add_subparsers(dest="operation")

    parser_watch = subparsers.add_parser("watch")
    parser_watch.add_argument(
        "-r", "--refresh-rate",
        type=float,
        default=1.0
    )

    args = parser.parse_args()

    client = capnp.TwoPartyClient(args.server)
    service = client.bootstrap().cast_as(kvak.Service)

    {
        "watch": do_watch
    }[args.operation](service, args)
