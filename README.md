# reveng-sencor-mt9020c

Reverse engineering of the **infrared (IR) remote protocol** of the
**Sencor MT9020C** portable air conditioner.

The goal is to fully decode the IR frame the remote sends so the air
conditioner can be controlled from a microcontroller / smart-home setup
(e.g. an ESP with an IR LED) instead of the original remote.

> ⚠️ **Status: work in progress.** Most of the frame is decoded. The
> checksum byte (Block 8) is *not* fully solved yet — see
> [Open questions](#open-questions).

## Protocol at a glance

| Property              | Value                                  |
| --------------------- | -------------------------------------- |
| Carrier / modulation  | PDM (pulse-distance modulation)        |
| Frame length          | 112 bits                               |
| LOW pulse width        | ~500 µs                                |
| HIGH pulse, bit `0`   | ~350 µs                                |
| HIGH pulse, bit `1`   | ~1.17 ms                               |

The 112-bit frame is split into 8 logical blocks:

| Block | Bits | Meaning                                         |
| ----- | ---- | ----------------------------------------------- |
| 1     | 40   | Static header (constant)                        |
| 2     | 8    | Power on/off                                    |
| 3     | 8    | Mode (Cool/Heat, Dry, Fan, Hold)                |
| 4     | 8    | Temperature (`31 − value`, big-endian)          |
| 5     | 8    | Fan speed / sleep / swing                       |
| 6     | 24   | Unknown extra data                              |
| 7     | 8    | Unit (°C / °F)                                  |
| 8     | 8    | Checksum (algorithm not confirmed)              |

Full field-by-field breakdown and the decoded sample table live in
[`docs/protocol.md`](docs/protocol.md).

## Repository layout

```
.
├── README.md              # this file — project overview
├── docs/
│   ├── protocol.md        # full frame format + decoded capture table
│   └── checksum.md        # checksum algorithm analysis (unconfirmed)
├── tools/
│   └── checksum.js        # brute-forcer for the checksum constant
└── captures/
    ├── full-pulseview.sr  # raw sigrok logic-analyzer capture
    ├── full-pulseview.pvs # PulseView session file
    ├── decoded-bits.txt   # captured frames decoded to raw bit strings
    ├── sample-1.png       # screenshot of a captured waveform
    └── sample-2.png       # screenshot of a captured waveform
```

## How the data was captured

The IR signal was demodulated with an IR receiver and recorded with a
[sigrok](https://sigrok.org/) logic analyzer, then inspected in
[PulseView](https://sigrok.org/wiki/PulseView).

To open the raw capture:

```bash
pulseview captures/full-pulseview.sr
```

`captures/decoded-bits.txt` contains the resulting frames as raw bit
strings (one per line), which are the input for the decoding work in
`docs/`.

## Tools

Run the checksum brute-forcer (requires Node.js):

```bash
node tools/checksum.js
```

It tries every additive constant `0x00..0xFF` against the captured packets
and reports which constants reproduce the observed checksums.

## Open questions

- **Block 8 (checksum):** the modelled algorithm (mirror bits → sum →
  add constant → mirror) does not validate across all captures. See
  [`docs/checksum.md`](docs/checksum.md).
- **Block 6:** purpose of these 24 bits is unknown; one bit appears to
  toggle at 32 °C.
- **Bit/byte ordering** of the frame is not fully confirmed.

## Disclaimer

This is an independent, non-commercial reverse-engineering project for
interoperability and educational purposes. "Sencor" and "MT9020C" are
trademarks of their respective owners; this project is not affiliated with
or endorsed by them.
