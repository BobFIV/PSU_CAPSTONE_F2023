# Final Demo
## Components:
### Demo Specific Components:
1. L298N Motor Driver
2. 12V DC Barrel Cable
3. HO Scale Locomotive
4. HO Scale Hopper Cart
5. HO Scale Tracks
6. two of Red, Green, and Blue Paper circles. Fruit/Vegetable pictures on them are optional, but suggested.
7. IR Proximity Sensor (Could also use a hall effect sensor with a magnet)
8. Rubber Bands
9. Multiple Battery Packs

Optional:  
1. Depending on lighting: Red, Blue and Green LEDs. Dimly lit areas are best, but bright areas can work with LEDs.
2. Black tablecloth. The tablecloth masks the background and thus makes the color recognition better.
### Tools: 
1. Phillips Head screw driver
2. Soldering Iron

### Project Components:
1. one nRF9160-DK with firmware loaded
2. one Raspberry Pi 4B with software downloaded
3. one ESP32 with firmware loaded

## Set-up:
1. Create a sensored track by creating two holes for the LEDs in the track. 
2. Take an IR Proximity sensor. Plug the sensor OUT into GPIO pin 27 on the Raspberry Pi. 
3. Plug VCC and GND from the IR sensor into the respective +3V3 and GND pins on the Raspberry Pi.
4. Calibrate the sensor. _Note: With the sensor we had, it would only recognize white. Thus, we had to add white paper underneath the train, which made the system very precise._
     - Try running either the locomotive or hopper cart over. If neither turn the LED on, try waving a lighter colored object.
     - Use a phillips head screw driver to loosen or tighten the potentiometer.
6. Take the 12V DC Barrel Cable. Snip off the Barrel part. You should see two wires (likely red and black) within.
     - Plug the Positive (RED) into the 12V IN of the L298N Motor Driver
     - Plug the Negative (BLACK) into the GND of the L298N Motor Driver (OR a GND Pin of the Raspberry Pi)
     - Connect a GND pin from the Raspberry Pi to the L298N Motor Driver. **The GNDs should be unified between the Raspberry Pi and L298N.**
     - _Note: Soldering wires here is suggested if you have the experience and technical knowledge. It will make wire management significantly easier. Solder a Male-to-Male breadboard wire to each end of the Positive and Negative wires of the 12V wires. This makes L298N connection much simpler._
7. Connect GPIO Pin 13 of the Raspberry Pi into the EN2 pin of the L298N
8. Connect GPIO Pin 26 of the Raspberry Pi into IN3 of the L298N.
9. Connect the power cables of the track into Motor 2 of the L298N. _Note: You should only be using one of the two motors of the L298N._
10. Put the Trains on the track.
11. PLace the battery pack onto the HO Scale train cart.
12. Connect the ESP32 to the HO Train Cart using a rubber band. Power it using a battery pack laoded on the HO train cart.
13. Run uart_peripheral.py on the Raspberry Pi by running `python3 uart_peripheral.py` in the terminal of the Raspberry Pi. 
14. Power on the nRF 9160 DK.
15. Place the HO Scale Locomotive and HO Scale Hopper Cart onto the tracks. Ensure they are on properly.
16. If all devices are connected via BLE, you should see constant RGB values in the terminal you ran uart_peripheral.py in.
17. In a.py should have the Demo_v1_dumb.py program. If it does not, copy it over.
18. Run control.py by running `python3 control.py` in a **different** terminal window than uart_peripheral.py
19. Press RUN on the webapp. The train should begin moving.
20. Run a firmware update by uploading the `Demo_v2_smart.py` program onto the webapp. Fruit recognition should be working now.
