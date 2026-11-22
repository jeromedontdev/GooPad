cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/version.cmake)
if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "6.1")
    set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/third_party/esp-homekit-sdk")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Version reported by esp_app_get_description() and shown in the boot log.
set(PROJECT_VER "0.7.2")

project(GooPad)
