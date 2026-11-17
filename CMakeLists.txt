cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Version reported by esp_app_get_description() and shown in the boot log.
# Bump this when tagging a release.
set(PROJECT_VER "1.0.0")

project(esp32s3_4848s040_hwtest)
