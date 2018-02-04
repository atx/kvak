# qpsk
QPSK for https://brmlab.cz/project/sdr/tetra (written as a final assignment for A8B14ADP)

## Usage

```
$ kvak --help
Usage: kvak [OPTION...]
pi/4-DQPSK multi-channel demodulator

  -b, --bind=ADDR            Bind to ADDR:PORT
  -c, --chunk-size=CHUNK     Chunk size
  -f, --fifo                 Explicitly create output FIFOs instead of files
  -i, --input=INPUT          Input file path
      --loop                 Loop the input file
  -m, --muted                Start all channels muted
  -n, --nchannels=NCHANNELS  Number of channels
  -o, --output=OUTPUT        Output file paths (use %d for channel number
                             substitution)
  -v, --verbose              Enable verbose debugging
  -?, --help                 Give this help list
      --usage                Give a short usage message
  -V, --version              Print program version

Mandatory or optional arguments to long options are also mandatory or optional
for any corresponding short options.

Report bugs to Josef Gajdusek <atx@atx.name>.
```

### Usual invocation

First, start the [channelizer](jenda.hrach.eu/gitweb/?p=fcl;a=summary).

```
$ rtl_sdr -f 425400e3 -s 1.8e6 -g 42 -p 37 - | ./fcl -b localhost -p 3333 -n 72 -s 50 -f "./fir.py 1.8e6 18.5e3 1151 rcos" -c 26,14,64,15,4,50,25,37,39,60,21,28,52,56,10,61,20,5,38,53 -t 1 -i U8 -o /tmp/fcl1.ch
```

Then, start the demodulator.

```
$ kvak -i /tmp/fcl1.ch -o /tmp/tetra.1.%d.bits -n 20 --fifo
```

Afterwards, tetra-rx can be launched on the FIFOs.

```
$ tetra-rx /tmp/tetra.1.18.bits
```

### Control

The repository also contains a TUI for controlling the demodulator using its RPC interface.

```
$ kvakctl -s 127.0.0.1:6677 tui
```

![output](https://user-images.githubusercontent.com/3966931/35777606-45e60cfa-09b1-11e8-82e2-c17826ee8b67.gif)

## Building and installing

```
$ mkdir build
$ cd build/
$ cmake ..
$ make
$ sudo make install
```

The repository also contains the metadata necessary to build a Debian package.

```
# apt install debhelper lsb-release cmake build-essential capnproto libcapnp-dev python3-numpy
# dpkg-buildpackage -uc -us
```

## Documentation

A more detailed documentation can be found [here](https://blog.atx.name/kvak/)
