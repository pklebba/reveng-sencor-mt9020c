# ESPHome IR hub

[`sencor-ir-hub.yaml`](sencor-ir-hub.yaml) is a starter ESPHome config that turns
the IR hub ([`../HARDWARE.md`](../HARDWARE.md)) into a Home Assistant device. It
builds the verified Sencor frame in a lambda and transmits it via the ESP32 RMT
peripheral; an attached TSOP receiver logs incoming frames.

This is the eventual target of the project (the PlatformIO firmware in `../src`
was the prototype that proved the protocol). It is **untested on hardware** — it
waits for the components to arrive.

## What you get in Home Assistant

- **Mode** (cool/heat/dry/fan/hold) and **Fan** (auto/sleep/low/med/high) selects
- **Temperature** number (16–32 °C)
- **Power** and **Swing** switches
- **Carrier** number (34–42 kHz) for tuning, and a **Resend** button

Every change rebuilds the full 14-byte frame (it's a complete state snapshot) and
transmits it.

## Setup

1. Install ESPHome (`pip install esphome`, or the HA add-on).
2. Create `secrets.yaml` in this folder (copy `secrets.yaml.example`) and fill in
   WiFi + keys. Generate the API key with `openssl rand -base64 32`.
3. Flash over USB the first time, then OTA:

   ```bash
   cd firmware/esphome
   esphome run sencor-ir-hub.yaml
   ```

4. Add the discovered device in Home Assistant (Settings → Devices).

## Notes

- Pins match the firmware/hardware: TX `GPIO4`, RX `GPIO5`.
- `dump: raw` prints received frames to the ESPHome log — point the real remote at
  the TSOP to capture/learn other devices later.
- A full `climate` entity (thermostat card) would need a custom `climate_ir`
  component; this starter uses plain select/number/switch entities, which work out
  of the box and are easy to extend.
