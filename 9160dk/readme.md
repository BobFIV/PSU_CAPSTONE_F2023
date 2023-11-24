# nRF9160: IPE PROGRAM
Pennsylvania State University Fall 2023 Capstone Project
Sponsored by Exacta Global Smart Solutions  
  
The Nordic Semicondutor nRF9160 Development Kit contains two systems on board: the nRF9160 SoC (main) and nRF52840 (board controller).  
See the readme in each board for deeper explanation here:
- [52840](hci_lpuart) runs the lower-levels of the BLE stack. Very few changes from the [original Nordic sample](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/bluetooth/hci_lpuart/README.html)
- [9160](IPE_OTA_BLE) runs the IPE-OTA-BLE program, which manages MQTT over LTE, oneM2M communications, OTA updates and the upper layers of the BLE stack. If you are interested in details regarding the 9160 DK's code, you are likely searching here.

Directions to build the programs are also provided in the respective documents. Build and flash the 52840 (hci_lpuart) program **first** before building and flashing the 9160 (IPE_OTA) program.
## Building the files
As of November 2023, these programs **must** be compiled specifying the board version, located on the back of your nRF9160-DK. Simply hitting build on nRF connect extension on VS Code **will cause the program to crash.**
### Instructions (Windows)
#### Prerequisites
You will need the following programs installed:
- [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop)
- [Python](https://www.python.org/downloads/)
- [CMake](https://cmake.org/download/)

Ensure these programs are in your PATH variable.
#### Setting up the file directory
1. After you've installed nRF Connect for Desktop, you'll find a directory called "nordicsemi" (or "ncs" by default) on your computer.
2. Inside this "nordicsemi" directory, you'll see a subdirectory named after your nRF Connect version (e.g., "v2.4.2" for version 2.4.2).
3. Within this version directory, you should create a new directory for your project, giving it a name that's easy for you to recognize. Let's call this directory "Capstone."
4. The final path to your project directory would look something like this: ```C:\ncs\v2.4.2\zephyr\Capstone```.
5. Copy the contents of this directory into that directory (i.e. transfer the IPE_OTA and hci_lpuart folders into the directory).
#### Building HCI_LPUART
1. Open windows powershell. Navigate to the HCI_LPUART directory, such as ```cd C:\ncs\v2.4.2\zephyr\Capstone\hci_lpuart```
2. Run west build with the version specified. For board version 1.1.0, it is the following: ```west build --board nrf9160dk_nrf52840@1.1.0```
3. Ensure SW10 is set to nRF52. 
4. Flash using west or VS code.
#### Building IPE_OTA
1. Navigate to the IPE_OTA directory in powershell, such as ```cd C:\ncs\v2.4.2\zephyr\Capstone\IPE_OTA```
2. Run west build with the version specified. For board version 1.1.0, it is the following: ```west build --board nrf9160dk_nrf9160_ns@1.1.0```
3. Ensure SW10 is set to nRF91.
4. Flash using west or VS code.

Although I primarily used Visual Studio Code (VS Code) for writing and editing code, I had to use the 'west' tool for building the code. However, when it came to flashing the code onto the device, I made use of the nRF Connect's VS Code extension. Additionally, running an initial build in VS Code, even without the flashing step, proved to be useful. This is because compiling within VS Code highlights errors directly in the code files themselves, providing a more intuitive way to identify and fix issues compared to just relying on the terminal output

