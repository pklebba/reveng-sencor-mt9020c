// Sencor MT9020C — ESPHome adapter (see sencor_climate.h).
#include "sencor_climate.h"

#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace sencor_mt9020c {

static const char *const TAG = "sencor_mt9020c";

// ---- Home Assistant -> neutral core mapping (adapter-only) -----------------

static sencor::Fan ha_to_fan(const optional<climate::ClimateFanMode> &f) {
  if (!f.has_value())
    return sencor::Fan::AUTO;
  switch (*f) {
    case climate::CLIMATE_FAN_QUIET:
      return sencor::Fan::SLEEP;
    case climate::CLIMATE_FAN_LOW:
      return sencor::Fan::LOW;
    case climate::CLIMATE_FAN_MEDIUM:
      return sencor::Fan::MED;
    case climate::CLIMATE_FAN_HIGH:
      return sencor::Fan::HIGH;
    default:
      return sencor::Fan::AUTO;
  }
}

// Map a powered climate mode to the Sencor mode. The Sencor "hold" mode has no
// standard climate equivalent, so it is not exposed (see traits()); the core
// still supports it for completeness / decoding.
static sencor::Mode ha_to_mode(climate::ClimateMode m) {
  switch (m) {
    case climate::CLIMATE_MODE_HEAT:
      return sencor::Mode::HEAT;
    case climate::CLIMATE_MODE_DRY:
      return sencor::Mode::DRY;
    case climate::CLIMATE_MODE_FAN_ONLY:
      return sencor::Mode::FAN;
    case climate::CLIMATE_MODE_COOL:
    default:
      return sencor::Mode::COOL;
  }
}

void SencorClimate::transmit_state() {
  sencor::Settings s;

  if (this->mode == climate::CLIMATE_MODE_OFF) {
    s.power = false;
    s.mode = this->last_active_mode_;   // retain the last real mode
  } else {
    s.power = true;
    s.mode = ha_to_mode(this->mode);
    this->last_active_mode_ = s.mode;
  }

  s.temp_c = (int) lroundf(this->target_temperature);  // core clamps per mode
  s.fan = ha_to_fan(this->fan_mode);
  s.swing = (this->swing_mode == climate::CLIMATE_SWING_VERTICAL);
  s.unit_f = false;                     // display-only flag, not exposed to HA

  const std::vector<int> timings = sencor::encode(s);

  auto transmit = this->transmitter_->transmit();
  auto *data = transmit.get_data();
  data->set_carrier_frequency(this->carrier_frequency_);
  data->reserve(timings.size());
  for (int v : timings) {
    if (v > 0)
      data->mark(v);
    else
      data->space(-v);
  }
  transmit.perform();

  ESP_LOGD(TAG, "TX power=%d mode=0x%02X temp=%d fan=0x%02X swing=%d", s.power,
           static_cast<uint8_t>(s.mode), s.temp_c, static_cast<uint8_t>(s.fan),
           s.swing);
}

climate::ClimateTraits SencorClimate::traits() {
  auto traits = climate::ClimateTraits();
  // current-temp support is a feature flag in 2026.6+ (only if a sensor is set)
  if (this->sensor_ != nullptr)
    traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_visual_min_temperature(13);
  traits.set_visual_max_temperature(32);
  traits.set_visual_temperature_step(1);
  // Exactly the Sencor modes — no CLIMATE_MODE_HEAT_COOL (the base would force
  // it); "hold" stays unmapped (supported by the core, not exposed to HA).
  traits.set_supported_modes({
      climate::CLIMATE_MODE_OFF,
      climate::CLIMATE_MODE_COOL,
      climate::CLIMATE_MODE_HEAT,
      climate::CLIMATE_MODE_DRY,
      climate::CLIMATE_MODE_FAN_ONLY,
  });
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_AUTO);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_QUIET);   // = Sencor "sleep"
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_LOW);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_MEDIUM);
  traits.add_supported_fan_mode(climate::CLIMATE_FAN_HIGH);
  traits.add_supported_swing_mode(climate::CLIMATE_SWING_OFF);
  traits.add_supported_swing_mode(climate::CLIMATE_SWING_VERTICAL);
  return traits;
}

}  // namespace sencor_mt9020c
}  // namespace esphome
