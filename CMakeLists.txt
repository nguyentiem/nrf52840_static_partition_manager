#
# Copyright (c) 2019 Peter Bigot Consulting, LLC
#
# SPDX-License-Identifier: Apache-2.0
#

cmake_minimum_required(VERSION 3.20.0)
set(PM_STATIC_YML_FILE ${CMAKE_CURRENT_SOURCE_DIR}/pm_static.yml)
message(STATUS "PM_STATIC_YML_FILE: ${PM_STATIC_YML_FILE}")

# if (CONFIG_SETTINGS_FCB OR CONFIG_SETTINGS_NVS)
#   ncs_add_partition_manager_config(pm.yml.settings)
# endif()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(littlefs)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
