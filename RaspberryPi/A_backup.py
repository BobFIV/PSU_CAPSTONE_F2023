from select import POLLWRNORM
import time
import json, _json
from unittest import case
from tabulate import tabulate
import RPi.GPIO as GPIO
import time
import paho.mqtt.client as mqtt


broker_address = "52.14.47.13"  # Replace with your MQTT broker's address
port = 1883  # Default MQTT port

topic = "trainLocation"  # Replace with your desired topic


timecounter = 0
min_assert_time = 0.03  # Minimum time the sensor should be asserted (in seconds)
min_lap_time = 5  # Minimum time for a lap (in seconds)
post_lap_delay = 5  # Delay after a lap before reasserting control_pin
GPIO.setmode(GPIO.BCM)

orginList = []
postionList = []
firstlooptime = 50
RGBmessage = ''
TrainStatus = False
#Lap average of 10.85
counting = False
CommandGoal = ''

motorPin = 26
PWM = 13  # PWM control pin connected to the EN pin of the motor driver
GPIO.setup(motorPin, GPIO.OUT)
speed = 50
GPIO.setup(PWM, GPIO.OUT)
pwm = GPIO.PWM(PWM, 1000)
pwm.start(0)  # Start with 0% duty cycle (motor off)
RGBr = 0
RGBg = 0
RGBb = 0
sensor_activated_time = 0

def detect_color(rgb_str):
    global RGBr, RGBg, RGBb
    # Split the RGB string into individual components
    if rgb_str == '' or rgb_str == 'ping!':
        return "None"
    # Split the string by commas to separate the RGB components
    rgb_components = rgb_str.split(', ')

    # Split each component by colon and take the second element, then convert to int
    r, g, b = map(lambda x: int(x.split(': ')[1]), rgb_components)
    RGBr = r
    RGBg = g
    RGBb = b
    # Define color ranges (adjust these based on your requirements)
    red_range = range(150, 256)
    green_range = range(150, 256)
    blue_range = range(150, 256)

    # Check if the values fall within specific ranges to determine the color
    if r in red_range and g not in green_range and b not in blue_range:
        return "Red"
    elif g in green_range and r not in red_range and b not in blue_range:
        return "Green"
    elif b in blue_range and r not in red_range and g not in green_range:
        return "Blue"
    else:
        if r in red_range and g in green_range and b not in blue_range:
            if r>g :
                return "Red"
            else:
                return "Green"
        if r in red_range and g not in green_range and b in blue_range:
            if r>b :
                return "Red"
            else:
                return "Blue"
        if r not in red_range and g in green_range and b in blue_range:
            if b>g :
                return "Blue"
            else:
                return "Green"
        if r in red_range and g in green_range and b in blue_range:
            if b>g and b>r:
                return "Blue"
            elif g>b and g>r:
                return "Green"
            else:
                return "Red"
            
        print("not color we want: r: "+str(r)+" g: "+str(g)+" b: "+str(b))
        return "None"
    




def sensor_callback(channel):
    global start_time, sensor_activated_time, counting, timecounter, firstlooptime, speed
    client = mqtt.Client()
    client.connect(broker_address, port, 60)
    if GPIO.input(27):  # Sensor is triggered
        sensor_activated_time = time.time()
    else:  # Sensor is not triggered
        current_time = time.time()
        if sensor_activated_time and (current_time - sensor_activated_time) >= min_assert_time:
            if not counting:
                print("Starting counter...")
                client.publish("train/calibration","CAL")
                start_time = current_time
                counting = True
            else:
                if (current_time - start_time) >= 5:
                    elapsed_time = current_time - start_time
                    firstlooptime = elapsed_time
                    print(f"Passing sensor. Elapsed time: {elapsed_time:.2f} seconds")
                    counting = False
                    timecounter = 0
                    client.publish("train/calibration","CAL")
                    #pwm.ChangeDutyCycle(0)  # Turn off control_pin
                    #time.sleep(1)  # Wait for the post lap delay
                    #motor_go(speed)  # Turn on control_pin for next lap
                sensor_activated_time = None



def update_RGB_from_BLE(value):
    # Listen for BLE messages and update R, G, B values accordingly
    # maybe I would move whole file to uart_peripheral.py, I 
    rgb_str = '{}'.format(bytearray(value).decode())
    currentColor = detect_color(rgb_str)
    return currentColor


def firstloop(expected_time):
    global RGBmessage, speed
    start_time = time.time()
    motor_go(speed)
    print("start first loop")
    i=0
    while (time.time() - start_time) < firstlooptime:
        with open("datafile.txt", 'rb') as f:
            RGBmessage = f.readline()
            temp = update_RGB_from_BLE(RGBmessage)
            orginList.append([i, temp])
            i+=0.5
            time.sleep(0.5)
    for x in range(1, len(orginList), 1):
        if orginList[x-1]!= None and (orginList[x] == orginList[x-1]):
            postionList[x-1] = orginList[x-1]
    pwm.ChangeDutyCycle(0)    

def update_goals(inputGoal,startTime):
    #reutrn times time = 0 if we don't have a goal
    time = 0
    if inputGoal == "APPLE":
        print("The goal is apple")
    elif inputGoal == "LIME":
        print("The goal is green Lime")
    elif inputGoal == "BLUEBERRY":
        print("The goal is blueberries")
    else:
        print("error: Unknown goal")
    
    for i in range(0, len(postionList), 1):
        if postionList[i][0] == startTime:

            for j in range(i, len(postionList), 1):
                if inputGoal == postionList[j][1]:
                    time = postionList[j][0]
                    break
            break
    #reutrn times, time = 0 if we don't have a goal
    return time 

def on_message(client, userdata, message):
    global CommandGoal, speed
    print(f"Received message on topic {message.topic}: {message.payload.decode()}")
    command = message.payload.decode()
    if command == "RUN":
        CommandGoal = "RUN"
        motor_go(speed) 
    elif command == "STOP":
        pwm.ChangeDutyCycle(0)
        CommandGoal = "STOP"
    elif command == "RED":
        CommandGoal = "APPLE"
    elif command == "GREEN":
        CommandGoal = "LIME"
    elif command == "BLUE":
        CommandGoal = "BLUEBERRY"

def motor_go(speed):
    # Ensure speed is within the valid range
    if 0 <= speed <= 100:
        GPIO.output(motorPin, GPIO.HIGH)  # Set motorPin high for forward motion
        pwm.ChangeDutyCycle(speed)  # Set PWM duty cycle
    else:
        # Stop the motor if an invalid speed is given
        GPIO.output(motorPin, GPIO.LOW)
        pwm.ChangeDutyCycle(0)

def main():
    global RGBmessage, timecounter, CommandGoal, speed
    GPIO.setmode(GPIO.BCM)
    #train part:
    GPIO.setup(27, GPIO.IN)
    GPIO.setup(motorPin, GPIO.OUT)
    GPIO.setup(PWM, GPIO.OUT)
    GPIO.add_event_detect(27, GPIO.BOTH, callback=sensor_callback)
    #em que tee tee
    client = mqtt.Client()
    client.connect(broker_address, port, 60)
    client.loop_start()
    
    client.subscribe("train/command")
    client.subscribe("train/target_color")
    client.on_message = on_message
    print("MQTT Online")
    #motor_go(speed) 
    try:
        client = mqtt.Client()
        client.connect(broker_address, port, 60)
        
        while(True):
            with open("datafile.txt", 'rb') as f:   #read data file and get the RGB message
                RGBmessage = f.readline()
            rgb_str = '{}'.format(bytearray(RGBmessage).decode())
            tempRGB = detect_color(rgb_str)
            temptime = update_goals(CommandGoal,timecounter)   #set the time we need keep running for the goal
            print("\n")

            print("*** LOOP: "+tempRGB+" ***")
            if(CommandGoal == "APPLE"): #add mqtt essage
                if(tempRGB == "Red"):
                    pwm.ChangeDutyCycle(0)
                    print("we stop at "+tempRGB)
                    client.publish("train/status","STOP")
                    time.sleep(3)   
                    client.publish("train/status","RUN")
                    motor_go(speed)
            if(CommandGoal == "LIME"): #add mqtt essage
                if(tempRGB == "Green"):
                    pwm.ChangeDutyCycle(0)
                    print("we stop at "+tempRGB)
                    client.publish("train/status","STOP")
                    time.sleep(3)   
                    client.publish("train/status","RUN")
                    motor_go(speed)   
            if(CommandGoal == "BLUEBERRY"): #add mqtt essage
                if(tempRGB == "Blue"):
                    pwm.ChangeDutyCycle(0)
                    print("we stop at "+tempRGB)
                    client.publish("train/status","STOP")
                    time.sleep(3)   
                    client.publish("train/status","RUN")
                    motor_go(speed)
            if(CommandGoal == "RUN"):
                
                motor_go(speed)
            if(CommandGoal == "STOP"):
                #client.publish( "train/status","STOP")
                pwm.ChangeDutyCycle(0)

            
    except KeyboardInterrupt:
        print("Train code stopped")
        pwm.stop()

        
    finally:
        pwm.stop()
        GPIO.cleanup()


if __name__ == '__main__':
    #GPIO.setmode(GPIO.BCM)
    main()
