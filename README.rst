Pennsylvania State University CAPSTONE FALL 2023
################################################

Sponsored by `Exacta Global Smart Solutions <https://www.exactagss.com/>`_

.. contents::
   :local:
   :depth: 2

Implementing Device Management through cellular IoT following the oneM2M standard.

Overview
*********
This repository contains code and instructions for implementing cellular-based device management on the ESP32 and the Raspberry Pi through the nrF9160-dk.



Components
**********
The following components make up our IoT system:

* nrF-9160DK
* AI-Thinker ESP32-CAM
* Raspberry Pi 4B
* Django WebApp
* `MQTT Broker <https://mosquitto.org/>`_
* `ACME CSE <https://github.com/ankraft/ACME-oneM2M-CSE>`_

Any BLE capable device running the Nordic UART Service (NUS) *should* be compatible with the project.

Acknowledgements
****************
Thank you to Exacta Global Smart Solutions for sponsoring and guiding us through the project. Without their expertise on oneM2M, and IoT solutions this project would not have been possible.

Thank you to Dave Gamble and other contributors for the cJSON library, used in our project.

Thank you to Andreas Kraft and the other contributors to the ACME CSE.



