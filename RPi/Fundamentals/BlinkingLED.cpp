/*
  Super Simple program to Blink three LEDs on a breadboard using the GPIO pins.
  Build using: g++ BlinkingLED.cpp -o LEDctrl -lwiringPi and then ./LEDctrl
*/

#include <iostream>
extern "C"
{
#include <wiringPi.h>
}
#include <csignal>
using namespace std;
bool RUNNING = true;

void handler(int s)
{
    cout << "Closing Program: Detected Ctrl-C signal \n";
    RUNNING = false;
}

void LEDCtrl(int color, int time)
{
    digitalWrite(color, HIGH);
    delay(time);
    digitalWrite(color, LOW);
    delay(time);
}


int main()
{
    std::signal(SIGINT, handler);

    wiringPiSetupGpio(); //setting up the wiring Pi library.

    int RedLED = 17; //Red LED to pin 17
    int GreenLED = 27; //Green LED to pin 27
    int BlueLED = 22; //Blue LED to pin 22
    pinMode(RedLED, OUTPUT); //setting pins for each LED.
    pinMode(GreenLED, OUTPUT);
    pinMode(BlueLED, OUTPUT);

    int time = 1000;
    while(RUNNING)
    {
        LEDCtrl(RedLED, time);
        LEDCtrl(GreenLED, time);
        LEDCtrl(BlueLED, time);

    }

    cout << "Program ending...\n";
}
