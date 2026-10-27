# Firmware image on ESP-IDF. Configured by platform.mk:
#   IDF_TARGET, SDKCONFIG, JSB_BOARD, JSB_VERSION, JSB_ENGINE
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(jsbedded)
