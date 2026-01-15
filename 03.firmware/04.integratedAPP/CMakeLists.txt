# Handy Keyboard Firmware - Integrated Application
# ESP-IDF Project for ESP32-P4

cmake_minimum_required(VERSION 3.16)

# ============================================================================
# ESP32-P4 TinyUSB HID_COUNT Kconfig Patch
# ============================================================================
# ESP32-P4 supports up to 15 endpoints (8 IN/OUT + 7 IN), so we can safely
# use more HID interfaces than the default esp_tinyusb Kconfig allows (0-4).
# This patch extends the range to 0-8 for ESP32-P4 projects.
# Reference: https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/api-reference/peripherals/usb_device.html
#
set(TINYUSB_KCONFIG_PATH "${CMAKE_SOURCE_DIR}/managed_components/espressif__esp_tinyusb/Kconfig")
if(EXISTS "${TINYUSB_KCONFIG_PATH}")
    file(READ "${TINYUSB_KCONFIG_PATH}" KCONFIG_CONTENT)
    # Check if patch is needed (range 0 4 -> range 0 8)
    string(FIND "${KCONFIG_CONTENT}" "range 0 4" RANGE_FOUND)
    if(NOT RANGE_FOUND EQUAL -1)
        string(REPLACE "range 0 4" "range 0 8" KCONFIG_CONTENT "${KCONFIG_CONTENT}")
        file(WRITE "${TINYUSB_KCONFIG_PATH}" "${KCONFIG_CONTENT}")
        message(STATUS "Patched esp_tinyusb Kconfig: HID_COUNT range extended to 0-8 for ESP32-P4")
    endif()
endif()

# ESP-IDF components
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(handy_keyboard)

# Suppress ESP-IDF Bluedroid stringop-truncation warnings (GitHub issue #9398)
# This is a known ESP-IDF 5.5.1 issue, not our code
# Must be AFTER project() to override ESP-IDF's -Werror=all
# Only for GCC, as Clang doesn't support this warning option
if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
    idf_build_set_property(COMPILE_OPTIONS "-Wno-error=stringop-truncation" APPEND)
else()
    # Clang specific suppressions
    idf_build_set_property(COMPILE_OPTIONS "-Wno-error=null-dereference" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "-Wno-error=unknown-warning-option" APPEND)
endif()

# Add ESP-IDF component dependencies to LVGL for PPA support
# LVGL's managed component doesn't properly declare these dependencies
if(TARGET __idf_lvgl__lvgl)
    # esp_driver_ppa - PPA hardware driver
    idf_component_get_property(ppa_include esp_driver_ppa INCLUDE_DIRS)
    target_include_directories(__idf_lvgl__lvgl PRIVATE ${ppa_include})
    idf_component_get_property(ppa_lib esp_driver_ppa COMPONENT_LIB)
    target_link_libraries(__idf_lvgl__lvgl PRIVATE ${ppa_lib})

    # esp_mm - Memory management (esp_cache.h)
    idf_component_get_property(mm_include esp_mm INCLUDE_DIRS)
    target_include_directories(__idf_lvgl__lvgl PRIVATE ${mm_include})
    idf_component_get_property(mm_lib esp_mm COMPONENT_LIB)
    target_link_libraries(__idf_lvgl__lvgl PRIVATE ${mm_lib})
endif()
