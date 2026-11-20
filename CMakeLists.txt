cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Version reported by esp_app_get_description() and shown in the boot log.
set(PROJECT_VER "0.4.1")

project(GooPad)
