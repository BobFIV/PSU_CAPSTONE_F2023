# Install script for directory: C:/nordicsemi/v2.4.1/nrf

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
  set(CMAKE_OBJDUMP "C:/nordicsemi/toolchains/31f4403e35/opt/zephyr-sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/ext/cmake_install.cmake")
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/lib/cmake_install.cmake")
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/samples/cmake_install.cmake")
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/subsys/cmake_install.cmake")
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/modules/cmake_install.cmake")
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/drivers/cmake_install.cmake")
  include("C:/Users/yuchi/Downloads/9160Release/build/mcuboot/modules/nrf/tests/cmake_install.cmake")

endif()

