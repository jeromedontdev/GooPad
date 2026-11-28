file(GLOB browser_font_sources "${CMAKE_CURRENT_LIST_DIR}/fonts/web/*.woff2")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/fa-icons.json"
    "${CMAKE_CURRENT_LIST_DIR}/index.html"
    "${CMAKE_CURRENT_LIST_DIR}/../tools/make_fa_index.py"
    "${CMAKE_CURRENT_LIST_DIR}/../tools/make_web_resources.py"
    "${CMAKE_CURRENT_LIST_DIR}/fonts/NOTICE.txt"
    ${browser_font_sources})

execute_process(
    COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../tools/make_web_resources.py"
        "${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/goopad_web_manifest.h"
    COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND "${PYTHON}" "${CMAKE_CURRENT_LIST_DIR}/../tools/make_fa_index.py"
        "${CMAKE_CURRENT_LIST_DIR}/fa-icons.json" "${CMAKE_CURRENT_LIST_DIR}/fa-index.json.gz"
        "${CMAKE_CURRENT_BINARY_DIR}/fa-catalog.bin"
    COMMAND_ERROR_IS_FATAL ANY
)

set(goopad_sources
        "main.c"
        "goopad.c"
        "goopad_companion.c"
        "goopad_config_storage.c"
        "goopad_homekit_storage.c"
        "goopad_network.c"
        "goopad_alerts.c"
        "goopad_alarm.c"
        "goopad_time.c"
        "goopad_news.c"
        "goopad_glass.c"
        "goopad_market.c"
        "goopad_ui.c"
        "goopad_fonts.c"
        "goopad_styled.c"
        "goopad_unicode.c"
        "goopad_web.c"
        "goopad_diagnostics.c"
        "lvgl_memory.c"
        "goopad_tls.c"
        "goopad_update.c"
        "hw_wifi.c"
)
set(GOOPAD_BOARD_DIR "" CACHE PATH "External GooPad board backend directory")
set(board_sources board.c hw_display.c hw_touch.c hw_sdcard.c hw_audio.c hw_gpio.c)
if(GOOPAD_BOARD_DIR)
    get_filename_component(GOOPAD_BOARD_DIR "${GOOPAD_BOARD_DIR}" ABSOLUTE)
    foreach(source IN LISTS board_sources)
        if(NOT EXISTS "${GOOPAD_BOARD_DIR}/${source}")
            message(FATAL_ERROR "Board backend missing ${GOOPAD_BOARD_DIR}/${source}; refusing Guition driver fallback")
        endif()
        list(APPEND goopad_sources "${GOOPAD_BOARD_DIR}/${source}")
    endforeach()
    if(NOT EXISTS "${GOOPAD_BOARD_DIR}/board.h")
        message(FATAL_ERROR "Board backend requires board.h")
    endif()
else()
    list(APPEND goopad_sources ${board_sources})
endif()
file(GLOB goopad_font_sources "${CMAKE_CURRENT_LIST_DIR}/fonts/goopad_font_essential_*.c")
list(APPEND goopad_sources ${goopad_font_sources})
if(CONFIG_GOOPAD_HOMEKIT)
    list(APPEND goopad_sources "goopad_homekit.c" "goopad_homekit_credentials.c")
endif()

set(homekit_requirements)
# Requirements are evaluated before sdkconfig is loaded, so gate on the SDK
# version rather than CONFIG_GOOPAD_HOMEKIT.
if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "6.1")
    list(APPEND homekit_requirements esp_hap_core esp_hap_apple_profiles espressif__libsodium)
endif()

idf_component_register(
    SRCS ${goopad_sources}
    EMBED_TXTFILES "fonts/NOTICE.txt"
    EMBED_FILES "recovery.html" "fa-index.json.gz"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_driver_gpio
        esp_driver_i2s
        esp_driver_usb_serial_jtag
        esp_driver_uart
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
        lwip
        esp_event
        esp_lvgl_port
        esp_lcd_touch_gt911
        esp_http_server
        esp_https_server
        esp_http_client
        esp_app_format
        app_update
        mbedtls
        esp_partition
        espressif__expat
        ${homekit_requirements}
)

if(GOOPAD_CAPACITY_TEST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE GOOPAD_CAPACITY_TEST=1)
endif()
if(GOOPAD_RENDER_DIAGNOSTIC)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE GOOPAD_RENDER_DIAGNOSTIC=1)
endif()
if(GOOPAD_BOARD_DIR)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC GOOPAD_BOARD_HEADER="${GOOPAD_BOARD_DIR}/board.h")
endif()

# The custom allocator is referenced from LVGL, which can be scanned after
# the main archive by the linker. Force its object into the link.
if(CONFIG_LV_USE_CUSTOM_MALLOC)
    target_link_options(${COMPONENT_LIB} INTERFACE "-Wl,-u,lv_malloc_core")
endif()
