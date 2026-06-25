# Sencor MT9020C — IR Protocol

Decoded and **verified** from the raw logic capture in
[`../captures/full-pulseview.sr`](../captures/full-pulseview.sr). Every claim
below is reproducible with [`../tools/decode.py`](../tools/decode.py), which
decodes the capture into frames and checks the checksum on all 69 of them.

```
$ python3 tools/decode.py
captures/full-pulseview.sr: 8 MHz, 69 frames
...
checksum valid: 69/69 frames
```

## Signal characteristics

Single channel **D0** from an IR receiver: idle HIGH, active-LOW bursts.
Modulation is **pulse-distance (PDM)** — a fixed LOW mark followed by a HIGH
space whose length encodes the bit. Captured at 8 MHz; the cluster centers
below have a standard deviation under 11 µs.

| Symbol               | Duration | Meaning            |
| -------------------- | -------: | ------------------ |
| Leader mark (LOW)    | 3149 µs  | start of frame     |
| Leader space (HIGH)  | 1575 µs  |                    |
| Bit mark (LOW)       |  498 µs  | fixed, every bit   |
| Space — short (HIGH) |  345 µs  | logical **0**      |
| Space — long (HIGH)  | 1161 µs  | logical **1**      |

Each frame is **112 bits** = 14 bytes. **Bytes are transmitted LSB-first**
(least-significant bit first on the wire) — the key fact the original manual
analysis got wrong.

## Frame layout

14 bytes, shown as decoded LSB-first values:

| Byte  | Field         | Notes                                                |
| :---: | ------------- | ---------------------------------------------------- |
| 0–4   | Header        | Constant `23 CB 26 01 00`                            |
| 5     | Power         | bit 2 = on → `24` ON / `20` OFF                      |
| 6     | Mode          | `1` Heat, `2` Dry, `3` Cool, `7` Fan, `8` Hold       |
| 7     | Temperature   | `base − T`; base 31 (Cool/Dry/Fan/Hold) or 26 (Heat) |
| 8     | Fan / Swing   | `&0x07` speed; `&0x38` = swing                       |
| 9–10  | Timer (?)     | Always `00` in this capture                          |
| 11    | bit 0         | 32 °C over-range flag                                |
| 12    | Unit          | `00` °C / `C9` °F                                    |
| 13    | Checksum      | `(sum of bytes 0–12) mod 256`                        |

## Field decoding

### Byte 5 — Power
`0x24` = on, `0x20` = off. The only difference is **bit 2** (`0x04`); the rest
of the byte (`0x20`) is constant.

### Byte 6 — Mode
| Value | Mode |
| :---: | ---- |
| `0x01` | Heat |
| `0x02` | Dry  |
| `0x03` | Cool |
| `0x07` | Fan  |
| `0x08` | Hold |

Cool and Heat are **distinct codes** (`03` vs `01`). No separate "auto" mode
appears in this capture.

### Byte 7 — Temperature
Linear, one step per °C, counting **down** as the setpoint goes up. The base
is mode-dependent:

- **Cool / Dry / Fan / Hold:** `T(°C) = 31 − byte7`
- **Heat:** `T(°C) = 26 − byte7`

32 °C is a special case: `byte7 = 0` **plus** bit 0 of byte 11.
The mode-dependent base is confirmed by the two captures sent in °F:

| Frame | Mode | byte7 | → °C | → °F (shown on remote) |
| ----- | ---- | :---: | :--: | :--------------------: |
| 31    | Heat | `08`  | 18   | **64 °F** ✓            |
| 66    | Cool | `08`  | 23   | **73 °F** ✓            |

In Dry / Fan / Hold modes the temperature isn't shown on the remote, but the
byte is still present (decodes to 23 °C here).

### Byte 8 — Fan speed & swing
Two independent sub-fields:

- **Fan speed** = `byte8 & 0x07`:
  `0` Auto, `1` Sleep (low speed + dimmed display), `2` Low, `3` Medium,
  `5` High.
- **Swing** = `byte8 & 0x38`: all three bits set (`0x38`) whenever swing is
  active in the capture. Example: High + swing = `0x05 | 0x38 = 0x3D`.

### Bytes 9–10 — Timer (unconfirmed)
Always `00`. The timer function was never exercised in this capture, so these
are the most likely — but unproven — candidates for the timer value.

### Byte 11 — Over-range flag
Bit 0 is set only at 32 °C (together with `byte7 = 0`). All other bits unused
here.

### Byte 12 — Unit
`0x00` = °C, `0xC9` = °F.

### Byte 13 — Checksum
**`checksum = (sum of bytes 0–12) mod 256`.** Holds for all 69 frames. No
magic constant, no bit mirroring — the earlier difficulty came entirely from
reading the bytes MSB-first.

## Verified examples

Header bytes (`23 CB 26 01 00`) omitted; all values LSB-first hex. `#` is the
frame index in the capture.

| #  | b5 | b6 | b7 | b8 | b11 | b12 | b13 | Decoded |
| -- | -- | -- | -- | -- | --- | --- | --- | ------- |
| 0  | 24 | 03 | 0D | 00 | 00  | 00  | 49  | ON, Cool, 18 °C, fan Auto |
| 14 | 24 | 03 | 00 | 00 | 01  | 00  | 3D  | ON, Cool, 32 °C, fan Auto |
| 15 | 24 | 02 | 08 | 00 | 00  | 00  | 43  | ON, Dry, 23 °C, fan Auto |
| 18 | 24 | 07 | 08 | 02 | 00  | 00  | 4A  | ON, Fan, 23 °C, fan Low |
| 17 | 24 | 07 | 08 | 05 | 00  | 00  | 4D  | ON, Fan, 23 °C, fan High |
| 21 | 24 | 07 | 08 | 3D | 00  | 00  | 85  | ON, Fan, 23 °C, fan High + swing |
| 23 | 24 | 01 | 0D | 02 | 00  | 00  | 49  | ON, Heat, 13 °C, fan Low |
| 30 | 24 | 01 | 08 | 05 | 00  | 00  | 47  | ON, Heat, 18 °C, fan High |
| 31 | 24 | 01 | 08 | 05 | 00  | C9  | 10  | ON, Heat, 64 °F, fan High |
| 35 | 24 | 08 | 08 | 02 | 00  | 00  | 4B  | ON, Hold, 23 °C, fan Low |
| 57 | 24 | 03 | 08 | 01 | 00  | 00  | 45  | ON, Cool, 23 °C, fan Sleep |
| 66 | 24 | 03 | 08 | 38 | 00  | C9  | 45  | ON, Cool, 73 °F, fan Auto + swing |
| 68 | 20 | 03 | 08 | 00 | 00  | 00  | 40  | OFF, Cool, 23 °C, fan Auto |

Run `python3 tools/decode.py --bits` to dump all 69 frames with their raw bits.
To build a frame from settings (for transmission), use
[`../tools/encode.py`](../tools/encode.py); its `--selftest` reproduces the
captured frames above byte-for-byte.

## Remaining unknowns

- **Timer (bytes 9–10):** not exercised in the capture; encoding unverified.
- **Swing (`byte8 & 0x38`):** always `0x38` when on — whether the three bits
  carry distinct meaning (e.g. swing range/direction) is untested.
- **Auto mode / "Quiet" / display-off**, and any other buttons not pressed
  during capture, are undocumented.
- **Repeat behaviour:** each button press produced one frame here; whether the
  remote repeats frames when a key is held is not captured.
