# udo-status: show `udo status` from the host on the right (peripheral) nice!view.
# host -> raw HID -> central_relay.c -> GATT -> peripheral_service.c -> screen/
if(CONFIG_UDO_STATUS)
  zephyr_include_directories(include)
  target_sources(app PRIVATE src/packet.c src/event.c)
endif()
