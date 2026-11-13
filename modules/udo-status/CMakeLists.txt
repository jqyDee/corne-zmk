# TEMPORARY Task 1 spike: echo raw HID reports back to the host.
if(CONFIG_RAW_HID)
  target_sources(app PRIVATE spike/echo.c)
endif()
