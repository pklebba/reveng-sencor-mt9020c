# ESPHome IR hub

[`sencor-ir-hub.yaml`](sencor-ir-hub.yaml) turns the IR hub
([`../HARDWARE.md`](../HARDWARE.md)) into a Home Assistant device with a proper
**thermostat card**. The Sencor frame is built by a local custom component,
[`components/sencor_mt9020c/`](components/sencor_mt9020c), which wraps a
**vendor-neutral C++ protocol core** ([`sencor_protocol.h`](components/sencor_mt9020c/sencor_protocol.h)/`.cpp`)
in a thin [`climate_ir`](https://esphome.io/components/climate/climate_ir/)
adapter ([`sencor_climate.*`](components/sencor_mt9020c/sencor_climate.h)). No
frame logic lives in the YAML.

The core is a direct port of the proven encoder
([`../src/main.cpp`](../src/main.cpp) `buildFrame()` / [`../../tools/encode.py`](../../tools/encode.py))
and has **no ESPHome/Arduino includes**, so it compiles and is unit-tested
off-device (see Testing below).

## What you get in Home Assistant

A single **climate** entity (thermostat card) with:

- **Modes:** Off, Cool, Heat, Dry, Fan-only
- **Fan:** Auto, Quiet, Low, Medium, High
- **Swing:** Off / Vertical
- **Target temperature:** 13–32 °C (step 1)

Every change rebuilds the full 14-byte frame (a complete state snapshot) and
transmits it via the ESP32 RMT peripheral at 38 kHz.

### Mapping decisions

| Sencor feature | Home Assistant | Note |
| -------------- | -------------- | ---- |
| fan *sleep* | fan **Quiet** | closest semantic fit |
| mode *hold* (0x08) | — | **not exposed** (no standard climate mode); the core still supports it |
| °C / °F flag | — | display-only on the AC; **dropped**, frames always encode °C |
| per-mode temp range | one 13–32 slider | the device clamps (heat 13–26, others 16–32) at transmit time |

The component is **transmit-only** for now. The TSOP receiver stays configured
with `dump: raw` to learn other remotes; decoding incoming Sencor frames to sync
HA state is a future step (the core already ships `decode_frame()` for it).

## Setup

1. Install ESPHome (`pip install esphome`, or the HA add-on).
2. Create `secrets.yaml` in this folder (copy `secrets.yaml.example`) and fill in
   WiFi + keys. Generate the API key with `openssl rand -base64 32`.
   (`secrets.yaml` is gitignored — never commit it.)
3. Flash over USB the first time, then OTA:

   ```bash
   cd firmware/esphome
   esphome run sencor-ir-hub.yaml
   ```

4. Add the discovered device in Home Assistant (Settings → Devices).

## Testing

The neutral core is verified independently of ESPHome, byte-for-byte against the
captured regression vectors:

```bash
g++ -std=c++17 -Wall -o /tmp/sencor_test \
    firmware/tests/sencor_core_test.cpp && /tmp/sencor_test
```

Validate and build the ESPHome config:

```bash
cd firmware/esphome
esphome config sencor-ir-hub.yaml     # schema / codegen check
esphome compile sencor-ir-hub.yaml    # full firmware build
```

## Notes

- Pins match the firmware/hardware: TX `GPIO4`, RX `GPIO5`.
- Carrier is set in YAML (`carrier_frequency: 38kHz`); sweep 36–40 kHz if the AC
  is unreliable (38 kHz is confirmed working on the real unit).
- `dump: raw` prints received frames to the ESPHome log — point the real remote
  at the TSOP to capture/learn other devices later.
