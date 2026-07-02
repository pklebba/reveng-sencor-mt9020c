// Sencor MT9020C — vendor-neutral IR protocol core.
//
// Pure C++: NO ESPHome / Arduino / Home Assistant includes. This is the single
// source of truth for the 14-byte LSB-first frame and its PDM timing envelope,
// ported 1:1 from tools/encode.py and firmware/src/main.cpp buildFrame(). It
// compiles off-device with plain g++ (see firmware/tests/sencor_core_test.cpp)
// and is reused by the ESPHome adapter in sencor_climate.*.
//
// Protocol reference: ../../../../docs/protocol.md
#pragma once

#include <cstdint>
#include <vector>

namespace sencor {

// Raw wire values (== the protocol bytes themselves). The adapter maps these to
// Home Assistant enums; the core stays agnostic and knows only the magistrala.
enum class Mode : uint8_t {
  HEAT = 0x01,
  DRY = 0x02,
  COOL = 0x03,
  FAN = 0x07,
  HOLD = 0x08,
};

enum class Fan : uint8_t {
  AUTO = 0x00,
  SLEEP = 0x01,
  LOW = 0x02,
  MED = 0x03,
  HIGH = 0x05,
};

// PDM pulse timings in microseconds (nominal, from docs/protocol.md).
constexpr uint16_t LEADER_MARK = 3150;
constexpr uint16_t LEADER_SPACE = 1575;
constexpr uint16_t BIT_MARK = 500;
constexpr uint16_t SPACE_0 = 345;
constexpr uint16_t SPACE_1 = 1160;
constexpr uint16_t STOP_MARK = BIT_MARK;

// Byte-level constants.
constexpr uint8_t SWING_BITS = 0x38;  // all three swing bits set together
constexpr uint8_t UNIT_F = 0xC9;      // byte 12 value when display unit is °F

constexpr int N_BYTES = 14;
constexpr int N_BITS = N_BYTES * 8;   // 112

// Total timing entries: leader(2) + 112 bits * 2 + trailing stop(1) = 227.
constexpr int N_TIMINGS = 2 + N_BITS * 2 + 1;

// A complete state snapshot. The temperature is ALWAYS in Celsius; unit_f is a
// display-only flag on the AC and does not change the setpoint.
struct Settings {
  bool power = true;
  Mode mode = Mode::COOL;
  int temp_c = 23;
  Fan fan = Fan::AUTO;
  bool swing = false;
  bool unit_f = false;
};

// Clamp the setpoint to the per-mode range (heat 13..26, all others 16..32).
int clamp_temp(Mode mode, int temp_c);

// Checksum = sum(bytes[0..12]) & 0xFF (byte 13 is the result, ignored on input).
uint8_t checksum(const uint8_t b[N_BYTES]);

// Build the 14 LSB-first frame bytes (including the trailing checksum).
void build_frame(const Settings &s, uint8_t out[N_BYTES]);

// Inverse of build_frame. Returns false if the checksum does not verify; on
// success fills `out` with the decoded settings.
bool decode_frame(const uint8_t in[N_BYTES], Settings &out);

// Encode settings to a signed timing list: +value = mark(µs), -value = space(µs).
// The signed convention maps directly onto ESPHome RemoteTransmitData
// mark()/space() and is trivial to assert in off-device tests.
std::vector<int> encode(const Settings &s);

}  // namespace sencor
