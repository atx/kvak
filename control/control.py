#! /usr/bin/env python3

# TODO: Make this a standard reasonable python package

import argparse
import asyncio
import capnp
import math
import pathlib
import time
try:
    import urwid
except ImportError:
    pass  # Oh well, we don't support TUI then

# TODO: This should eventually be made to a "proper" python package or something


def load_capnp_schema():
    here = pathlib.Path(__file__).absolute().parent
    search_paths = [
        here.parent / "share" / "kvak" / "schema.capnp",
        here.parent / "schema.capnp",
    ]
    for path in search_paths:
        if path.exists():
            return capnp.load(str(path))
    raise RuntimeError("Unable to find the schema.capnp file!")

kvak = load_capnp_schema()


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


class DecibelProgressBar(urwid.ProgressBar):

    def __init__(self, normal, complete,
                 min_db=0.0, max_db=100.0, current=0, satt=None):
        super().__init__(normal, complete, done=(max_db - min_db), satt=satt)
        # Unfortunately, ProgressBar by itself does not support negative ranges,
        # so we have to hack around it
        self.min_db = min_db
        self.max_db = max_db
        self.decibels = current

    def get_text(self):
        return "{:.0f} dB".format(self.decibels)

    @property
    def decibels(self):
        return self.current + self.min_db

    @decibels.setter
    def decibels(self, current):
        # The _set_completion and friends should be left alone
        # as they are used by the internal code and wrapping them to do
        # the offsetting leads to unpredictable shit happening.
        self.current = current - self.min_db


class ChannelWidget(urwid.WidgetWrap):

    def __init__(self, n, channel):
        self.channel = channel
        self.title = urwid.Text(
            ("title", "Channel #{}".format(n)),
            align="center"
        )
        # We want to keep track about what our button says and do precisely that
        # (toggling could be problematic because of the latency)
        self._muting = False
        # ProgressBar does not like negative numbers... TODO
        self._min_db = -40
        self._max_db = 20
        self.pg_power = DecibelProgressBar(
            "pg normal power", "pg complete power",
            -40, 20, 0,
            "pg smooth"
        )
        self.mute_button = urwid.Button("?")
        urwid.connect_signal(self.mute_button, "click", self._toggle_mute)

        super().__init__(urwid.Pile([
            self.title, urwid.Text(""),
            self.pg_power,
            self.mute_button
        ]))

    def _toggle_mute(self, ev):
        self.channel.mute(self._muting)

    def fetch_new_data(self):
        info = self.channel.getInfo().wait().info

        db = math.log10(info.powerLevel) * 10
        self.pg_power.decibels = db

        self.mute_button.set_label("Unmute" if info.isMuted else "Mute")
        self._muting = not info.isMuted


def do_tui(service, args):

    pallete = [
        ("title",   "black",        "white",        "standout"),
        ('pg normal power',    'white',      'black', 'standout'),
        ('pg complete power',  'white',      'dark magenta'),
        ('pg smooth',           'dark magenta', 'black')
    ]

    channels = service.listChannels().wait().list

    cws = [
        ChannelWidget(n, c) for n, c in enumerate(channels)
    ]
    flow = urwid.GridFlow(cws, 20, 3, 1, "center")

    async def periodic_update():
        while True:
            for cw in cws:
                cw.fetch_new_data()
            await asyncio.sleep(1)

    asyncio.ensure_future(periodic_update())
    top = urwid.Filler(flow, "top", top=3)
    event_loop = urwid.AsyncioEventLoop()
    loop = urwid.MainLoop(top, pallete, event_loop=event_loop)
    loop.run()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-s", "--server",
        default="127.0.0.1:6677",
    )
    subparsers = parser.add_subparsers(dest="operation")
    subparsers.required = True

    parser_watch = subparsers.add_parser("watch")
    parser_watch.add_argument(
        "-r", "--refresh-rate",
        type=float,
        default=1.0
    )

    parser_tui = subparsers.add_parser("tui")

    args = parser.parse_args()

    client = capnp.TwoPartyClient(args.server)
    service = client.bootstrap().cast_as(kvak.Service)

    {
        "watch": do_watch,
        "tui": do_tui
    }[args.operation](service, args)
