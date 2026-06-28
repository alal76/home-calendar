"""
preview_page — tiny external component that adds a /preview route to the
device's built-in web server, rendering an 800x480 SVG that mirrors the
e-paper layout. Reads event data from globals at request time.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import web_server_base
from esphome.const import CONF_ID

DEPENDENCIES = ["web_server_base"]
AUTO_LOAD = ["web_server_base"]

preview_page_ns = cg.esphome_ns.namespace("preview_page")
PreviewPage = preview_page_ns.class_("PreviewPage", cg.Component)

CONF_WEB_SERVER_BASE_ID = "web_server_base_id"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PreviewPage),
        cv.GenerateID(CONF_WEB_SERVER_BASE_ID): cv.use_id(
            web_server_base.WebServerBase
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_WEB_SERVER_BASE_ID])
    var = cg.new_Pvariable(config[CONF_ID], paren)
    await cg.register_component(var, config)
