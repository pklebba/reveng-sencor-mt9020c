# reveng-sencor-mt9020c

Reverse engineering of the **infrared (IR) remote protocol** of the
**Sencor MT9020C** portable air conditioner.

The goal is to fully decode the IR frame the remote sends so the air
conditioner can be controlled from a microcontroller / smart-home setup
(e.g. an ESP with an IR LED) instead of the original remote.

> ✅ **Status: protocol decoded and verified.** All 14 frame bytes are
> mapped and the checksum is solved — every one of the 69 captured frames
> validates. See [`docs/protocol.md`](docs/protocol.md). Remaining gaps are
> only buttons that weren't pressed during capture (timer, auto mode).

## Protocol at a glance

| Property             | Value                                   |
| -------------------- | --------------------------------------- |
| Modulation           | PDM (pulse-distance modulation)         |
| Frame length         | 112 bits (14 bytes)                     |
| Bit order            | **LSB-first**                           |
| Bit mark (LOW)       | ~500 µs (fixed)                         |
| Space, bit `0`       | ~345 µs                                 |
| Space, bit `1`       | ~1.16 ms                                |
| Checksum             | `sum(bytes 0–12) mod 256`               |

Frame bytes (LSB-first):

| Byte  | Field       | Notes                                          |
| :---: | ----------- | ---------------------------------------------- |
| 0–4   | Header      | Constant `23 CB 26 01 00`                      |
| 5     | Power       | `24` on / `20` off                             |
| 6     | Mode        | `1` Heat · `2` Dry · `3` Cool · `7` Fan · `8` Hold |
| 7     | Temperature | `base − T` (base 31, or 26 in Heat)            |
| 8     | Fan / Swing | `&0x07` speed, `&0x38` swing                   |
| 9–10  | Timer (?)   | unused in capture                              |
| 11    | 32 °C flag  | bit 0                                          |
| 12    | Unit        | `00` °C / `C9` °F                              |
| 13    | Checksum    | sum of bytes 0–12                              |

Full field-by-field breakdown, verified examples and the corrections to the
original analysis live in [`docs/protocol.md`](docs/protocol.md).

## Repository layout

```
.
├── README.md              # this file — project overview
├── docs/
│   └── protocol.md        # verified frame format + decoding
├── tools/
│   ├── decode.py          # decodes a .sr capture → frames + checksum check
│   └── encode.py          # builds a transmittable IR frame from settings
├── firmware/              # ESP32 IR transmitter + HTTP control (PlatformIO)
│   ├── platformio.ini
│   └── src/main.cpp
└── captures/
    ├── full-pulseview.sr  # raw sigrok logic-analyzer capture (8 MHz, D0)
    ├── full-pulseview.pvs # PulseView session file
    ├── sample-1.png       # screenshot of a captured waveform
    └── sample-2.png       # screenshot of a captured waveform
```

## Decoding the capture

The IR signal was demodulated with an IR receiver and recorded with a
[sigrok](https://sigrok.org/) logic analyzer (8 MHz, channel D0), then
inspected in [PulseView](https://sigrok.org/wiki/PulseView).

Decode it into frames and verify every checksum (needs Python + numpy):

```bash
python3 tools/decode.py            # decoded settings, one line per frame
python3 tools/decode.py --bits     # also dump the raw 112-bit frames
```

To open the raw capture in a GUI instead:

```bash
pulseview captures/full-pulseview.sr
```

## Sending commands

`tools/encode.py` builds a frame from settings and prints the bytes, the
112-bit stream and a raw pulse-timing list (µs, mark/space) ready for an IR
transmitter such as an ESP32 + IR LED (e.g. IRremoteESP8266 `sendRaw`, ~38 kHz
carrier):

```bash
python3 tools/encode.py --mode cool --temp 22 --fan auto
python3 tools/encode.py --mode heat --temp 18 --fan high --swing --unit f
python3 tools/encode.py --power off
python3 tools/encode.py --selftest   # verifies output against real captures
```

To actually drive the air conditioner, [`firmware/`](firmware/) has an ESP32
sketch that builds the frame on the MCU and transmits it via an IR LED, with a
small HTTP API (`curl "http://<esp-ip>/ac?mode=cool&temp=22&fan=auto"`).

## Remaining unknowns

- **Timer (bytes 9–10):** never exercised in the capture; encoding unverified.

See [`docs/protocol.md`](docs/protocol.md#remaining-unknowns) for details.

## Disclaimer

This is an independent, non-commercial reverse-engineering project for
interoperability and educational purposes. "Sencor" and "MT9020C" are
trademarks of their respective owners; this project is not affiliated with
or endorsed by them.
