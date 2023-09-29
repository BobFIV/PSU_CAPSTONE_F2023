#include <Arduino.h>
#include <ArduinoJson.h>
#include "wifiSetup.h"
#include "driver/gpio.h"
#include "oneM2MPrimitives.h"

// https://esp32tutorials.com/esp32-push-button-esp-idf-digital-input/
// This implements a "smart Light".
// The hardware consists of an ESP32 or compatible alternative.
// This implementation uses FreeRTOS tasks
// task 1 -> monitor the button presses
// task 2 -> toggle the LED
// task 3 -> registration and setup code for oneM2M





SemaphoreHandle_t sem_toggle; // give a semaphore to toggle led


gpio_num_t LED_GPIO = GPIO_NUM_27;
gpio_num_t BUTTON_GPIO = GPIO_NUM_33;
gpio_num_t LED_BUILTIN = GPIO_NUM_2;

#define BUTTON_DN  1 
#define BUTTON_UP  0 
#define POLL_DELAY  50
#define POLL_COMMANDS_DELAY 2000 // 2 seconds

static bool registered = false;

void enable_internal_led(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  if (registered)
  {
    createContentInstance("status", String(HIGH) );
  }
  else
  {
    Serial.println("Not registered");
  }

}

void disable_internal_led(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  if (registered)
  {
    createContentInstance("status", String(LOW) );
  }
  else
  {
    Serial.println("Not registered");
  }

}

int get_internal_led_state(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  return digitalRead(LED_BUILTIN);
}

void toggle_internal_led(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  if (registered)
  {
    createContentInstance("status", String(!digitalRead(LED_BUILTIN)) );
  }
  else
  {
    Serial.println("Not registered");
  }
}



// enable external led
void enable_external_led(void)
{
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, HIGH);
}

// disable external led (turn off)
void disable_external_led(void)
{
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, LOW);
}

// toggle external led
void toggle_external_led(void)
{
  pinMode(LED_GPIO, OUTPUT);
  digitalWrite(LED_GPIO, !digitalRead(LED_GPIO));
} 

// get the current state of the led
int get_external_led_state(void)
{
  pinMode(LED_GPIO, OUTPUT);
  return digitalRead(LED_GPIO);
}

// enable both leds
void enable_leds(void)
{
  enable_internal_led();
  enable_external_led();
}

// disable both leds
void disable_leds(void)
{
  disable_internal_led();
  disable_external_led();
} 

// toggle both leds
void toggle_leds(void)
{
  toggle_internal_led();
  toggle_external_led();
}

// The following states/transitions are used to implement toggle behavior
// button_state is set (changed) to DOWN when input_state is DOWN and current button_state is UP (button pressed)
// button_state is set (changed) to UP when input_state is UP and current button_state is DOWN (button released)
void task_checkbuttonstate(void *pvParameters)
{
  int button_state = gpio_get_level(BUTTON_GPIO);

  for (;;)
  {
    int input_state = gpio_get_level(BUTTON_GPIO);

    if (input_state == BUTTON_DN && button_state == BUTTON_UP)
    {
      button_state = BUTTON_DN;
      Serial.println("Button Press ");
      xSemaphoreGive(sem_toggle); // This will signal an event to change the LED state
    }
    else if (input_state == BUTTON_UP && button_state == BUTTON_DN)
    {
      button_state = BUTTON_UP;
    }
    
    vTaskDelay(POLL_DELAY); // Delay to yield processor and avoid switch bounce on transitions
  }
}
// task 2 from description above
void task_toggle(void *pvParameters)
{
  // turn off both leds
  disable_leds();
  
  for (;;)
  {
    if (xSemaphoreTake(sem_toggle, portMAX_DELAY) == pdTRUE)
    { 
      toggle_leds();
    }
  }
}


void registerTo_oneM2M_CSE(void)
{
   if (Register())
    {
      registered = true;
      // Create the remaining resources. This occurs even if the AE is already registered just to make sure
      // that all of the needed resources are present. A better design would be to check for
      // the existence of the resources before creating them and only create them if they are not present.
      createContainer("status");       // Status Container
      createContainer("command");      // Command Container
      subscribeToContainer("command"); // subscription to Command container
    }
    else
    {
      // Registration failed - schedule for a later time
    }
}
// task 3 from description above
void task_registerTo_oneM2M_CSE(void *pvParameters)
{
  for (;;)
  {
    registerTo_oneM2M_CSE();
    vTaskSuspend(NULL); // End this task
  }
}

void setup()
{
  Serial.begin(115200);

  setNotificationHandler(parseNotification);
  setDisableLed(disable_leds);
  setEnableLed(enable_leds);

  setupWifi(); // this will reconnect to existing configuration or wait for new setup

  // xTaskCreatePinnedToCore(task_function, task_name, stack_depth, function_parameters, priority, task_handle_address, task_affinity)

  gpio_pad_select_gpio(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_INPUT_OUTPUT);

  gpio_pad_select_gpio(BUTTON_GPIO);
  gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

  vSemaphoreCreateBinary(sem_toggle);
  xSemaphoreTake(sem_toggle, 0);

  registerTo_oneM2M_CSE(); 
  //xTaskCreate(task_registerTo_oneM2M_CSE, "Task Register to CSE", 4096, NULL, 2, NULL);
  xTaskCreate(task_toggle, "Task Toggle", 4096, NULL, 2, NULL);
  xTaskCreate(task_checkbuttonstate, "Task Button State", 1024, NULL, 2, NULL);
  

}


void loop()
{
  // put your main code here, to run repeatedly:
  
}