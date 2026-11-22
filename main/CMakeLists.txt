set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/fa-icons.json"
    "${CMAKE_CURRENT_LIST_DIR}/../tools/make_fa_index.py")

execute_process(
    COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../tools/make_fa_index.py"
        "${CMAKE_CURRENT_LIST_DIR}/fa-icons.json" "${CMAKE_CURRENT_LIST_DIR}/fa-index.json.gz"
    COMMAND_ERROR_IS_FATAL ANY
)

set(goopad_sources
        "main.c"
        "goopad.c"
        "goopad_ui.c"
        "goopad_tls.c"
        "hw_audio.c"
        "board.c"
        "hw_display.c"
        "hw_touch.c"
        "hw_sdcard.c"
        "hw_gpio.c"
        "hw_wifi.c"
)
if(CONFIG_GOOPAD_HOMEKIT)
    list(APPEND goopad_sources "goopad_homekit.c" "goopad_homekit_credentials.c"
         "goopad_homekit_storage.c")
endif()

set(homekit_requirements)
# Requirements are evaluated before sdkconfig is loaded, so gate on the SDK
# version rather than CONFIG_GOOPAD_HOMEKIT.
if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "6.1")
    list(APPEND homekit_requirements esp_hap_core esp_hap_apple_profiles espressif__libsodium)
endif()

idf_component_register(
    SRCS ${goopad_sources}
    EMBED_TXTFILES "index.html" "fa-icons.json"
    EMBED_FILES "fa-index.json.gz"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_driver_gpio
        esp_driver_i2s
        esp_driver_i2c
        esp_driver_ledc
        esp_driver_sdspi
        esp_driver_spi
        esp_lcd
        fatfs
        sdmmc
        spi_flash
        esp_psram
        esp_hw_support
        esp_timer
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        esp_lvgl_port
        esp_lcd_touch_gt911
        esp_http_server
        esp_https_server
        esp_http_client
        app_update
        mbedtls
        esp_partition
        ${homekit_requirements}
)
