import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir

AUTO_LOAD = ["climate_ir"]
CODEOWNERS = ["@siperek95"]

CONF_CARRIER_FREQUENCY = "carrier_frequency"

sencor_ns = cg.esphome_ns.namespace("sencor_mt9020c")
SencorClimate = sencor_ns.class_("SencorClimate", climate_ir.ClimateIR)

# TX-only schema (no receiver_id): the component only transmits; the TSOP stays
# a standalone remote_receiver with `dump: raw` for learning other remotes.
CONFIG_SCHEMA = climate_ir.climate_ir_schema(SencorClimate).extend(
    {
        cv.Optional(CONF_CARRIER_FREQUENCY, default="38kHz"): cv.All(
            cv.frequency, cv.int_range(min=30000, max=60000)
        ),
    }
)


async def to_code(config):
    var = await climate_ir.new_climate_ir(config)
    cg.add(var.set_carrier_frequency(int(config[CONF_CARRIER_FREQUENCY])))
