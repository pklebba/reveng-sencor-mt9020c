// Sencor MT9020C — ESPHome adapter.
//
// Thin binding of the vendor-neutral sencor:: core to ESPHome's climate_ir. All
// Home Assistant <-> protocol enum mapping lives in the .cpp; the core stays
// agnostic. TX-only for now (on_receive is intentionally not implemented — the
// core already ships decode_frame() for a future receiver wiring).
#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

#include "sencor_protocol.h"

namespace esphome {
namespace sencor_mt9020c {

class SencorClimate : public climate_ir::ClimateIR {
 public:
  SencorClimate()
      : climate_ir::ClimateIR(13.0f, 32.0f, 1.0f,   // visual min / max / step
                              /*supports_dry=*/true,
                              /*supports_fan_only=*/true) {}

  void set_carrier_frequency(uint32_t hz) { this->carrier_frequency_ = hz; }

 protected:
  void transmit_state() override;
  climate::ClimateTraits traits() override;

  // OFF must keep the last active mode in the emitted frame (the real remote's
  // power-off frame still carries a mode — see capture frame 68).
  sencor::Mode last_active_mode_{sencor::Mode::COOL};
  uint32_t carrier_frequency_{38000};
};

}  // namespace sencor_mt9020c
}  // namespace esphome
