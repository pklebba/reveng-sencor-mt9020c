// Sencor MT9020C — vendor-neutral IR protocol core (see sencor_protocol.h).
#include "sencor_protocol.h"

#include <algorithm>

namespace sencor {

static const uint8_t HEADER[5] = {0x23, 0xCB, 0x26, 0x01, 0x00};

int clamp_temp(Mode mode, int t) {
  if (mode == Mode::HEAT)
    return std::min(std::max(t, 13), 26);
  return std::min(std::max(t, 16), 32);
}

uint8_t checksum(const uint8_t b[N_BYTES]) {
  uint16_t sum = 0;
  for (int i = 0; i < 13; i++)
    sum += b[i];
  return sum & 0xFF;
}

void build_frame(const Settings &s, uint8_t out[N_BYTES]) {
  for (int i = 0; i < 5; i++)
    out[i] = HEADER[i];
  for (int i = 5; i < N_BYTES; i++)
    out[i] = 0;

  out[5] = 0x20 | (s.power ? 0x04 : 0x00);          // power: bit 2 = on
  out[6] = static_cast<uint8_t>(s.mode);

  // Temperature: base - T, with 32 °C handled by the over-range flag (byte 11).
  int t = clamp_temp(s.mode, s.temp_c);
  int base = (s.mode == Mode::HEAT) ? 26 : 31;
  bool flag32 = (s.mode != Mode::HEAT && t >= 32);
  out[7] = flag32 ? 0 : static_cast<uint8_t>((base - t) & 0xFF);
  out[11] = flag32 ? 0x01 : 0x00;

  out[8] = static_cast<uint8_t>(s.fan) | (s.swing ? SWING_BITS : 0);
  out[12] = s.unit_f ? UNIT_F : 0x00;               // display unit only

  out[13] = checksum(out);
}

bool decode_frame(const uint8_t b[N_BYTES], Settings &out) {
  if (checksum(b) != b[13])
    return false;

  out.mode = static_cast<Mode>(b[6]);
  int base = (out.mode == Mode::HEAT) ? 26 : 31;
  out.temp_c = base - b[7] + ((b[11] & 1) ? 1 : 0);
  out.power = (b[5] & 0x04) != 0;
  out.fan = static_cast<Fan>(b[8] & 0x07);
  out.swing = (b[8] & SWING_BITS) != 0;
  out.unit_f = (b[12] == UNIT_F);
  return true;
}

std::vector<int> encode(const Settings &s) {
  uint8_t f[N_BYTES];
  build_frame(s, f);

  std::vector<int> t;
  t.reserve(N_TIMINGS);
  t.push_back(+LEADER_MARK);
  t.push_back(-LEADER_SPACE);
  for (int i = 0; i < N_BYTES; i++)
    for (int j = 0; j < 8; j++) {                   // LSB-first within each byte
      t.push_back(+BIT_MARK);
      t.push_back(-((f[i] >> j) & 1 ? SPACE_1 : SPACE_0));
    }
  t.push_back(+STOP_MARK);                          // trailing stop mark
  return t;
}

}  // namespace sencor
