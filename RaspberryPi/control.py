import os
import subprocess
import paho.mqtt.client as mqtt

MQTT_BROKER = '52.14.47.13'
MQTT_PORT = 1883
MQTT_TOPIC = 'update'
A_PY = 'A.py'
BACKUP_A_PY = 'A_backup.py'
a_py_process = None

def on_connect(client, userdata, flags, rc):
    print("Connected to MQTT Broker")
    client.subscribe(MQTT_TOPIC)

def on_message(client, userdata, msg):
    global a_py_process
    if a_py_process:
        a_py_process.terminate()
        a_py_process.wait()
    os.remove("A_backup.py")
    os.rename(A_PY, BACKUP_A_PY)

    with open(A_PY, 'w') as file:
        file.write(str(msg.payload.decode()))
    a_py_process = subprocess.Popen(['python', A_PY])

def main():
    global a_py_process
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    a_py_process = subprocess.Popen(['python', A_PY])
    client.loop_forever()

main()