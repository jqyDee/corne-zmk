# udo-status: show `udo status` from the host on the right (peripheral) nice!view.
# host -> raw HID -> central_relay.c -> GATT -> peripheral_service.c -> screen/
if(CONFIG_UDO_STATUS)
  zephyr_include_directories(include)
  target_sources(app PRIVATE src/packet.c src/event.c)

  if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    # Needs zmk-raw-hid (raw_hid_adapter shield on the central build)
    if(CONFIG_RAW_HID)
      target_sources(app PRIVATE src/central_relay.c)
    endif()
  else()
    target_sources(app PRIVATE src/peripheral_service.c)

    # Replaces the stock nice!view peripheral screen; needs
    # CONFIG_NICE_VIEW_WIDGET_STATUS=n for this build (see build.yaml).
    if(CONFIG_ZMK_DISPLAY AND CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
       AND NOT CONFIG_NICE_VIEW_WIDGET_STATUS)
      target_sources(app PRIVATE
        src/screen/custom_status_screen.c
        src/screen/peripheral_status.c
        src/screen/udo_widget.c
        src/screen/udo_view.c
        src/screen/util.c
        src/screen/bolt.c
      )
    endif()
  endif()
endif()
