// Off-device test for the vendor-neutral Sencor core. Proves the core compiles
// and runs with plain g++ (no ESPHome/Arduino), and matches the byte-for-byte
// regression vectors captured from the real remote (mirror of
// tools/encode.py _VECTORS, including the HOLD mode the ESPHome adapter omits).
//
//   g++ -std=c++17 -Wall -o /tmp/sencor_test
//       firmware/tests/sencor_core_test.cpp && /tmp/sencor_test
//
// The .cpp is #included directly so there is nothing to link separately.
#include "../esphome/components/sencor_mt9020c/sencor_protocol.h"
#include "../esphome/components/sencor_mt9020c/sencor_protocol.cpp"

#include <cstdio>
#include <string>

using sencor::Fan;
using sencor::Mode;
using sencor::Settings;

static int g_failures = 0;

static std::string hex(const uint8_t *b, int n) {
  std::string s;
  char buf[4];
  for (int i = 0; i < n; i++) {
    std::snprintf(buf, sizeof(buf), "%02X", b[i]);
    if (i)
      s += ' ';
    s += buf;
  }
  return s;
}

static void check_vector(const char *label, const Settings &s,
                         const char *expected) {
  uint8_t frame[sencor::N_BYTES];
  sencor::build_frame(s, frame);
  std::string got = hex(frame, sencor::N_BYTES);
  bool ok = (got == expected);
  std::printf("  [%s] %-28s %s\n", ok ? "ok" : "FAIL", label, got.c_str());
  if (!ok) {
    std::printf("         expected %s\n", expected);
    g_failures++;
  }
}

int main() {
  std::printf("Sencor core regression vectors:\n");

  // Vectors copied verbatim from tools/encode.py _VECTORS (frame # in comments).
  check_vector("cool 18 auto",
               Settings{true, Mode::COOL, 18, Fan::AUTO, false, false},
               "23 CB 26 01 00 24 03 0D 00 00 00 00 00 49");      // frame 0
  check_vector("cool 32 auto (over-range)",
               Settings{true, Mode::COOL, 32, Fan::AUTO, false, false},
               "23 CB 26 01 00 24 03 00 00 00 00 01 00 3D");      // frame 14
  check_vector("dry 23 auto",
               Settings{true, Mode::DRY, 23, Fan::AUTO, false, false},
               "23 CB 26 01 00 24 02 08 00 00 00 00 00 43");      // frame 15
  check_vector("fan 23 high swing",
               Settings{true, Mode::FAN, 23, Fan::HIGH, true, false},
               "23 CB 26 01 00 24 07 08 3D 00 00 00 00 85");      // frame 21
  check_vector("heat 18 high unitF",
               Settings{true, Mode::HEAT, 18, Fan::HIGH, false, true},
               "23 CB 26 01 00 24 01 08 05 00 00 00 C9 10");      // frame 31
  check_vector("hold 23 low",
               Settings{true, Mode::HOLD, 23, Fan::LOW, false, false},
               "23 CB 26 01 00 24 08 08 02 00 00 00 00 4B");      // frame 35
  check_vector("cool 23 sleep",
               Settings{true, Mode::COOL, 23, Fan::SLEEP, false, false},
               "23 CB 26 01 00 24 03 08 01 00 00 00 00 45");      // frame 57
  check_vector("power off",
               Settings{false, Mode::COOL, 23, Fan::AUTO, false, false},
               "23 CB 26 01 00 20 03 08 00 00 00 00 00 40");      // frame 68

  // Round-trip: decode(encode(x)) preserves the settings (mirror of encode.py).
  {
    Settings in{true, Mode::HEAT, 20, Fan::MED, true, false};
    uint8_t frame[sencor::N_BYTES];
    sencor::build_frame(in, frame);
    Settings out;
    bool ok = sencor::decode_frame(frame, out) && out.power &&
              out.mode == Mode::HEAT && out.temp_c == 20 && out.fan == Fan::MED &&
              out.swing && !out.unit_f;
    std::printf("  [%s] round-trip decode (heat 20 med swing)\n",
                ok ? "ok" : "FAIL");
    if (!ok)
      g_failures++;
  }

  // Bad checksum must be rejected.
  {
    uint8_t frame[sencor::N_BYTES];
    sencor::build_frame(Settings{}, frame);
    frame[13] ^= 0xFF;  // corrupt
    Settings out;
    bool ok = !sencor::decode_frame(frame, out);
    std::printf("  [%s] rejects bad checksum\n", ok ? "ok" : "FAIL");
    if (!ok)
      g_failures++;
  }

  // Timing envelope shape: 227 entries, leader first, stop mark last.
  {
    auto t = sencor::encode(Settings{});
    bool ok = (int) t.size() == sencor::N_TIMINGS && t.front() == +3150 &&
              t[1] == -1575 && t.back() == +500;
    std::printf("  [%s] timing envelope (%zu entries)\n", ok ? "ok" : "FAIL",
                t.size());
    if (!ok)
      g_failures++;
  }

  std::printf(g_failures ? "FAILURES: %d\n" : "PASS\n", g_failures);
  return g_failures ? 1 : 0;
}
