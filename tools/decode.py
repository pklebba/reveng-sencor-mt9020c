#!/usr/bin/env python3
"""
Sencor MT9020C — IR capture decoder.

Decodes a sigrok/PulseView ``.sr`` logic capture of the demodulated IR signal
into protocol frames and human-readable settings, and verifies the checksum.

The capture is a single active-LOW channel (D0) carrying the output of an IR
receiver. The modulation is pulse-distance (PDM): a fixed ~500 us LOW mark
followed by a HIGH space whose length encodes the bit (short = 0, long = 1).
Bytes are transmitted **LSB-first**.

Usage:
    python3 tools/decode.py [capture.sr]      # default: captures/full-pulseview.sr
    python3 tools/decode.py --bits capture.sr # also print raw 112-bit frames
"""
import sys
import zipfile

# --- Pulse timing thresholds (microseconds) -------------------------------
# Measured cluster centers: leader mark 3149, bit mark 498, space-0 345,
# space-1 1161, leader space 1575. Thresholds sit safely between clusters.
LEADER_MARK_US = 2000   # LOW longer than this starts a frame
BIT_THRESHOLD_US = 750  # HIGH space longer than this is a '1'
GAP_US = 5000           # HIGH longer than this ends a frame (inter-frame gap)
FRAME_BITS = 112

# --- Field tables ---------------------------------------------------------
MODE = {0x01: "Heat", 0x02: "Dry", 0x03: "Cool", 0x07: "Fan", 0x08: "Hold"}
FAN = {0x00: "Auto", 0x01: "Sleep", 0x02: "Low", 0x03: "Med", 0x05: "High"}


def read_samplerate(z):
    """Return sample rate in Hz from the sigrok metadata (e.g. '8 MHz')."""
    meta = z.read("metadata").decode()
    for line in meta.splitlines():
        if line.startswith("samplerate"):
            val = line.split("=", 1)[1].strip()
            num, _, unit = val.partition(" ")
            mult = {"Hz": 1, "kHz": 1e3, "MHz": 1e6, "GHz": 1e9}[unit]
            return float(num) * mult
    raise RuntimeError("samplerate not found in metadata")


def run_length_encode(z, srate):
    """Yield (level, duration_us) runs for channel D0 across the whole capture."""
    try:
        import numpy as np
    except ImportError:
        sys.exit("This script needs numpy:  pip install numpy")

    names = sorted((n for n in z.namelist() if n.startswith("logic-1-")),
                   key=lambda n: int(n.split("-")[-1]))
    us_per_sample = 1e6 / srate
    cur_level, cur_len = None, 0
    for name in names:
        arr = np.frombuffer(z.read(name), dtype=np.uint8) & 1  # D0 = bit 0
        edges = np.nonzero(np.diff(arr))[0]
        starts = np.concatenate(([0], edges + 1))
        ends = np.concatenate((edges + 1, [len(arr)]))
        for lv, ln in zip(arr[starts].tolist(), (ends - starts).tolist()):
            if lv == cur_level:
                cur_len += ln
            else:
                if cur_level is not None:
                    yield cur_level, cur_len * us_per_sample
                cur_level, cur_len = lv, ln
    if cur_level is not None:
        yield cur_level, cur_len * us_per_sample


def decode_frames(runs):
    """Turn the run list into a list of 112-bit frames (transmission order)."""
    frames, cur = [], None
    runs = list(runs)
    i, n = 0, len(runs)
    while i < n:
        level, dur = runs[i]
        if level == 0 and dur > LEADER_MARK_US:          # leader mark
            if cur:
                frames.append(cur)
            cur = []
            i += 1
            if i < n and runs[i][0] == 1:                # skip leader space
                i += 1
            continue
        if cur is not None and level == 0 and i + 1 < n and runs[i + 1][0] == 1:
            space = runs[i + 1][1]
            if space > GAP_US:                           # frame end
                frames.append(cur)
                cur = None
            else:
                cur.append(1 if space > BIT_THRESHOLD_US else 0)
            i += 2
            continue
        i += 1
    if cur:
        frames.append(cur)
    return [f for f in frames if len(f) == FRAME_BITS]


def frame_to_bytes(bits):
    """Pack 112 bits into 14 bytes, LSB-first within each byte."""
    out = []
    for i in range(0, len(bits), 8):
        byte = 0
        for j, bit in enumerate(bits[i:i + 8]):
            byte |= bit << j          # LSB-first
        out.append(byte)
    return out


def checksum_ok(b):
    return (sum(b[:13]) & 0xFF) == b[13]


def describe(b):
    power = "ON" if b[5] & 0x04 else "OFF"
    mode = MODE.get(b[6], "?%02X" % b[6])
    base = 26 if mode == "Heat" else 31
    tc = base - b[7] + (1 if b[11] & 1 else 0)
    unit = "F" if b[12] == 0xC9 else "C"
    temp = round(tc * 9 / 5 + 32) if unit == "F" else tc
    fan = FAN.get(b[8] & 0x07, "?%02X" % (b[8] & 0x07))
    swing = "swing" if b[8] & 0x38 else ""
    chk = "OK" if checksum_ok(b) else "BAD"
    return f"{power:3} {mode:4} {temp:>2}°{unit}  fan={fan:5} {swing:5}  chk={chk}"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    show_bits = "--bits" in sys.argv
    path = args[0] if args else "captures/full-pulseview.sr"

    with zipfile.ZipFile(path) as z:
        srate = read_samplerate(z)
        frames = decode_frames(run_length_encode(z, srate))

    print(f"{path}: {srate/1e6:g} MHz, {len(frames)} frames\n")
    ok = 0
    for k, bits in enumerate(frames):
        b = frame_to_bytes(bits)
        ok += checksum_ok(b)
        hexs = " ".join("%02X" % x for x in b)
        print(f"{k:2d}: {hexs}  |  {describe(b)}")
        if show_bits:
            print("    " + "".join(map(str, bits)))
    print(f"\nchecksum valid: {ok}/{len(frames)} frames")


if __name__ == "__main__":
    main()
