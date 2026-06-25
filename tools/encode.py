#!/usr/bin/env python3
"""
Sencor MT9020C — IR frame encoder.

Builds a transmittable IR frame from human settings — the inverse of
``decode.py``. Emits the 14 frame bytes, the 112-bit stream, and a raw
pulse-timing list (microseconds, mark/space alternating) suitable for an
IR transmitter such as an ESP32 + IR LED (e.g. IRremoteESP8266 ``sendRaw``).

The protocol is documented in ../docs/protocol.md. Bytes are LSB-first; the
checksum is ``sum(bytes 0..12) mod 256``. The carrier is assumed to be the
usual ~38 kHz (the capture is already demodulated, so the exact frequency
isn't recoverable from it).

Usage:
    python3 tools/encode.py --mode cool --temp 22 --fan auto
    python3 tools/encode.py --power off
    python3 tools/encode.py --mode heat --temp 18 --fan high --swing --unit f
    python3 tools/encode.py --selftest
"""
import argparse
import sys

# --- Pulse timing (microseconds), nominal values from docs/protocol.md ----
LEADER_MARK = 3150
LEADER_SPACE = 1575
BIT_MARK = 500
SPACE_0 = 345
SPACE_1 = 1160
STOP_MARK = BIT_MARK

HEADER = [0x23, 0xCB, 0x26, 0x01, 0x00]
MODE = {"heat": 0x01, "dry": 0x02, "cool": 0x03, "fan": 0x07, "hold": 0x08}
FAN = {"auto": 0x00, "sleep": 0x01, "low": 0x02, "med": 0x03, "high": 0x05}
SWING_BITS = 0x38
UNIT_F = 0xC9


def build_bytes(power=True, mode="cool", temp=23, fan="auto",
                swing=False, unit="c"):
    """Return the 14 LSB-first frame bytes for the given settings."""
    if mode not in MODE:
        raise ValueError(f"mode must be one of {list(MODE)}")
    if fan not in FAN:
        raise ValueError(f"fan must be one of {list(FAN)}")

    b = list(HEADER) + [0] * 9            # bytes 5..13 filled below
    b[5] = 0x20 | (0x04 if power else 0)  # power: bit2 = on
    b[6] = MODE[mode]

    # temperature: base - T, with 32 C handled by the over-range flag (byte 11)
    base = 26 if mode == "heat" else 31
    flag32 = (mode != "heat" and temp >= 32)
    b[7] = 0 if flag32 else (base - temp) & 0xFF
    b[11] = 0x01 if flag32 else 0x00

    b[8] = FAN[fan] | (SWING_BITS if swing else 0)
    b[12] = UNIT_F if unit == "f" else 0x00
    b[13] = sum(b[:13]) & 0xFF            # checksum
    return b


def bytes_to_bits(b):
    """14 bytes -> 112-bit on-wire stream (LSB-first within each byte)."""
    bits = []
    for byte in b:
        for j in range(8):
            bits.append((byte >> j) & 1)
    return bits


def bits_to_timings(bits):
    """112 bits -> raw mark/space timing list in microseconds (PDM)."""
    t = [LEADER_MARK, LEADER_SPACE]
    for bit in bits:
        t.append(BIT_MARK)
        t.append(SPACE_1 if bit else SPACE_0)
    t.append(STOP_MARK)                   # trailing stop mark
    return t


# --- Round-trip decode (mirror of decode.py) for the self-test ------------
_MODE_R = {v: k for k, v in MODE.items()}
_FAN_R = {v: k for k, v in FAN.items()}


def decode_bytes(b):
    mode = _MODE_R[b[6]]
    base = 26 if mode == "heat" else 31
    temp = base - b[7] + (1 if b[11] & 1 else 0)
    return {
        "power": bool(b[5] & 0x04),
        "mode": mode,
        "temp": temp,
        "fan": _FAN_R[b[8] & 0x07],
        "swing": bool(b[8] & SWING_BITS),
        "unit": "f" if b[12] == UNIT_F else "c",
        "checksum_ok": (sum(b[:13]) & 0xFF) == b[13],
    }


# Regression vectors: settings -> exact bytes observed in the real capture
# (see docs/protocol.md "Verified examples"; frame # in comments).
_VECTORS = [
    (dict(power=True, mode="cool", temp=18, fan="auto"),
     "23 CB 26 01 00 24 03 0D 00 00 00 00 00 49"),                 # frame 0
    (dict(power=True, mode="cool", temp=32, fan="auto"),
     "23 CB 26 01 00 24 03 00 00 00 00 01 00 3D"),                 # frame 14
    (dict(power=True, mode="dry", temp=23, fan="auto"),
     "23 CB 26 01 00 24 02 08 00 00 00 00 00 43"),                 # frame 15
    (dict(power=True, mode="fan", temp=23, fan="high", swing=True),
     "23 CB 26 01 00 24 07 08 3D 00 00 00 00 85"),                 # frame 21
    (dict(power=True, mode="heat", temp=18, fan="high", unit="f"),
     "23 CB 26 01 00 24 01 08 05 00 00 00 C9 10"),                 # frame 31
    (dict(power=True, mode="hold", temp=23, fan="low"),
     "23 CB 26 01 00 24 08 08 02 00 00 00 00 4B"),                 # frame 35
    (dict(power=True, mode="cool", temp=23, fan="sleep"),
     "23 CB 26 01 00 24 03 08 01 00 00 00 00 45"),                 # frame 57
    (dict(power=False, mode="cool", temp=23, fan="auto"),
     "23 CB 26 01 00 20 03 08 00 00 00 00 00 40"),                 # frame 68
]


def selftest():
    ok = True
    for settings, expected in _VECTORS:
        got = " ".join("%02X" % x for x in build_bytes(**settings))
        status = "ok" if got == expected else "FAIL"
        if got != expected:
            ok = False
        print(f"  [{status}] {settings}")
        if got != expected:
            print(f"         expected {expected}")
            print(f"         got      {got}")
    # round-trip: decode(encode(x)) preserves the settings
    rt = dict(power=True, mode="heat", temp=20, fan="med", swing=True, unit="c")
    d = decode_bytes(build_bytes(**rt))
    if not (d["checksum_ok"] and d["power"] and d["mode"] == "heat"
            and d["temp"] == 20 and d["fan"] == "med" and d["swing"]):
        ok = False
        print(f"  [FAIL] round-trip: {d}")
    else:
        print(f"  [ok] round-trip decode: {d}")
    print("PASS" if ok else "FAILURES")
    return ok


def main():
    p = argparse.ArgumentParser(description="Encode a Sencor MT9020C IR frame.")
    p.add_argument("--power", choices=["on", "off"], default="on")
    p.add_argument("--mode", choices=list(MODE), default="cool")
    p.add_argument("--temp", type=int, default=23, help="setpoint in °C (16-32)")
    p.add_argument("--fan", choices=list(FAN), default="auto")
    p.add_argument("--swing", action="store_true")
    p.add_argument("--unit", choices=["c", "f"], default="c",
                   help="display unit only; temp is always given in °C")
    p.add_argument("--selftest", action="store_true",
                   help="encode known captures and verify byte-for-byte")
    a = p.parse_args()

    if a.selftest:
        sys.exit(0 if selftest() else 1)

    b = build_bytes(power=(a.power == "on"), mode=a.mode, temp=a.temp,
                    fan=a.fan, swing=a.swing, unit=a.unit)
    bits = bytes_to_bits(b)
    timings = bits_to_timings(bits)

    print("bytes :", " ".join("%02X" % x for x in b))
    print("bits  :", "".join(map(str, bits)))
    print("checksum:", "OK" if (sum(b[:13]) & 0xFF) == b[13] else "BAD")
    print(f"raw ({len(timings)} timings, µs):")
    print("  [" + ", ".join(str(t) for t in timings) + "]")


if __name__ == "__main__":
    main()
