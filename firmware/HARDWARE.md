# Hardware — IR hub build

Wiring for the target build: a standalone IR hub controlling the Sencor MT9020C
(with room for more IR devices later). Brain is an **ESP32-S3-DevKitC-1**; IR
LEDs are driven by a 2N7000 MOSFET, plus an optional TSOP receiver for decoding
real remotes.

## Bill of materials

| Part | Qty | Notes |
| ---- | --- | ----- |
| ESP32-S3-DevKitC-1 (N16R8) | 1 | any S3; N16R8 octal PSRAM blocks GPIO35/36/37 |
| TSAL6400 IR LED (940 nm) | 1–3 | more = wider coverage |
| 2N7000 N-MOSFET | 1 | low-side switch (parallel 2+ for many LEDs) |
| Resistor 33 Ω | 1 per LED | LED current limit (~100 mA from 5 V) |
| Resistor 220 Ω | 1 | MOSFET gate series |
| Resistor 10 kΩ | 1 | MOSFET gate pulldown |
| Resistor 100 Ω | 1 | optional TSOP VCC filter |
| TSOP38238 / TSOP4838 | 1 | 38 kHz IR receiver (optional) |
| Ceramic 100 nF | 1–2 | TSOP + ESP decoupling |
| Electrolytic 470 µF / ≥16 V | 1 | 5 V bulk (IR/WiFi current spikes) |
| Enclosure (e.g. Kradex Z76) | 1 | LEDs poke **out** — black plastic blocks 940 nm |
| 5 V USB supply + cable | 1 | powers the board |

## Pin assignment

| ESP32-S3 | Use | Note |
| -------- | --- | ---- |
| **GPIO4** | IR TX (MOSFET gate) | safe pin |
| **GPIO5** | IR RX (TSOP OUT) | safe pin |
| **5V** (VBUS) | LED anodes + 470 µF | more range than 3V3 |
| **3V3** | TSOP VCC | **3.3 V, not 5 V** (keeps OUT at 3.3 V) |
| **GND** | common ground | several GND pins on the board |

Avoid: GPIO35/36/37 (octal PSRAM on N16R8), 0/45/46 (strapping), 19/20 (USB),
43/44 (UART0), 26–34 (SPI flash).

## Transmitter (TX) — low-side switch on 2N7000

```
   +5V ──┬───────────┬──────┬──────┬─────────
         │           │      │      │
      [470µF]      [33Ω]  [33Ω]  [33Ω]      <- one resistor per LED
         │           │      │      │
        GND      anode▼ anode▼ anode▼
                 TSAL   TSAL   TSAL          <- aim in different directions
                 6400   6400   6400
                cath.▲  cath.▲  cath.▲
                     └──────┴──────┘
                            │  (all cathodes joined)
                          DRAIN
                            │
   GPIO4 ─[220Ω]─┬──── GATE │  2N7000  (flat face toward you, legs down:
                 │          │           Source – Gate – Drain)
              [10kΩ]      SOURCE
                 │          │
                GND ───────┴──────── GND
```

- **33 Ω/LED** from 5 V ≈ 100 mA each: `(5 − 1.35 − 0.3) / 0.1`. Calmer: 47 Ω (~75 mA).
- **220 Ω** in series with the gate; **10 kΩ** gate→GND pulldown (LED off at reset/boot).
- IR has low duty (carrier only during marks), so one 2N7000 handles ~3 LEDs.
  For much higher power, parallel two MOSFETs.

## Receiver (RX) — TSOP (optional)

```
   3V3 ──┬──────────────► VCC ┐
         │                    │
      [100nF]              TSOP38238/4838
         │                    │
        GND ◄──────────────  GND
                              │
   GPIO5 ◄────────────────── OUT
```

- Power the TSOP from **3.3 V** so its OUT is 3.3 V (safe for the S3 GPIO).
- **100 nF right at the TSOP** (VCC↔GND, short legs) — without it the output is
  noisy (1 µs spikes that flood the receiver).
- Optional cleaner filter (datasheet): **100 Ω in series with VCC + 470 µF**.

## Power / decoupling

- **470 µF** between **5V and GND** near the LEDs — buffers IR/WiFi current spikes
  (prevents brownout).
- **100 nF** between **3V3 and GND** near the ESP.

## Bring-up order

1. **Prototype on a breadboard with one LED** to confirm the AC responds:
   ```
   GPIO4 ─[220Ω]─ GATE 2N7000;  SOURCE → GND;  10kΩ gate→GND
   +5V ─[33Ω]─ anode TSAL6400 ─ cathode → DRAIN
   ```
   Wire the TSOP too (GPIO5 / 3V3 / GND / 100 nF) to verify the TX by loopback.
2. Works → add 2 more LEDs, move to perfboard, mount in the enclosure with the
   LEDs poking through drilled holes.

## ESPHome (later)

`remote_transmitter` on **GPIO4** (carrier 38 kHz), `remote_receiver` on
**GPIO5**. For the N16R8: `board_build.arduino.memory_type: qio_opi`.
Protocol details: [`../docs/protocol.md`](../docs/protocol.md).
