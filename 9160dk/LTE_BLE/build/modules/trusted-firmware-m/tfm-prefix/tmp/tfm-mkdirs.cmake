# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/nordicsemi/v2.4.1/modules/tee/tf-m/trusted-firmware-m"
  "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/tfm"
  "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix"
  "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix/tmp"
  "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix/src/tfm-stamp"
  "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix/src"
  "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix/src/tfm-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix/src/tfm-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/nordicsemi/v2.4.1/zephyr/capstone/LTE_BLE/build/modules/trusted-firmware-m/tfm-prefix/src/tfm-stamp${cfgdir}") # cfgdir has leading slash
endif()
