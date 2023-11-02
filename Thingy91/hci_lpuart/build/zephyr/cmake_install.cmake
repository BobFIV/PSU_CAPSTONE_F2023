# Install script for directory: C:/ncs/v2.4.2/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/ncs/toolchains/31f4403e35/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/arch/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/lib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/soc/arm/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/boards/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/subsys/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/drivers/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/nrf/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/hostap/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/mcuboot/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/mbedtls/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/trusted-firmware-m/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/cjson/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/azure-sdk-for-c/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/cirrus-logic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/openthread/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/memfault-firmware-sdk/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/canopennode/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/chre/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/cmsis/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/fatfs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/hal_nordic/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/st/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/hal_wurthelektronik/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/libmetal/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/liblc3/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/littlefs/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/loramac-node/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/lvgl/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/lz4/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/mipi-sys-t/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/nanopb/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/nrf_hw_models/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/open-amp/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/picolibc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/segger/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/tinycrypt/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/TraceRecorder/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/uoscore-uedhoc/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/zcbor/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/zscilib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/nrfxlib/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/modules/connectedhomeip/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/kernel/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/cmake/flash/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/cmake/usage/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("C:/Users/yuchi/Desktop/PSU_CAPSTONE_F2023-main/Thingy91/hci_lpuart/build/zephyr/cmake/reports/cmake_install.cmake")
endif()

