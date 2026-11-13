# udo-status: show `udo status` from the host on the right (peripheral) nice!view.
# host -> raw HID -> central_relay.c -> GATT -> peripheral_service.c -> screen/
# Switch: CONFIG_UDO_STATUS (config/corne.conf), see Kconfig.
if(CONFIG_UDO_STATUS OR CONFIG_UDO_PERIPHERAL_SCREEN)
  zephyr_include_directories(include)
endif()

if(CONFIG_UDO_STATUS)
  target_sources(app PRIVATE src/packet.c src/event.c)
  if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE src/central_relay.c)
  else()
    target_sources(app PRIVATE src/peripheral_service.c)
  endif()
endif()

# Right screen (replaces the stock nice!view peripheral widget).
if(CONFIG_UDO_PERIPHERAL_SCREEN)
  target_sources(app PRIVATE
    src/screen/custom_status_screen.c
    src/screen/peripheral_status.c
    src/screen/util.c
    src/screen/bolt.c
  )
  if(CONFIG_UDO_STATUS)
    target_sources(app PRIVATE src/screen/udo_widget.c src/screen/udo_view.c)
  else()
    target_sources(app PRIVATE src/screen/art.c)
  endif()
endif()
