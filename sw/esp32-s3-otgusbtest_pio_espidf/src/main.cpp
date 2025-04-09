/**
 * @source https://github.com/luc-github/esp32-usb-serial/
 */

#include <Arduino.h>
#include <FastLED.h>

// #include "stled.h"
#include "led/statusled.hpp"
#include "si/si_parser.h"
#include "defines.h"
#include "system/systemstats.h"

#include "esp32_usb_serial.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>

#define ESP_USB_SERIAL_BAUDRATE 38400 // 115200
#define ESP_USB_SERIAL_DATA_BITS (8)
#define ESP_USB_SERIAL_PARITY \
  (0) // 0: 1 stopbit, 1: 1.5 stopbits, 2: 2 stopbits
#define ESP_USB_SERIAL_STOP_BITS \
  (0) // 0: None, 1: Odd, 2: Even, 3: Mark, 4: Space

#define ESP_USB_SERIAL_RX_BUFFER_SIZE 512
#define ESP_USB_SERIAL_TX_BUFFER_SIZE 128
#define ESP_USB_SERIAL_TASK_SIZE 4096
#define ESP_USB_SERIAL_TASK_CORE 1
#define ESP_USB_SERIAL_TASK_PRIORITY 10

SemaphoreHandle_t device_disconnected_sem;
std::unique_ptr<CdcAcmDevice> vcp;
bool isConnected = false;
bool usbReady = false;
TaskHandle_t xHandle;

uint8_t test[100];
StatusLED status_led, network_led;
QueueHandle_t punchQueue;

// STATUS
bool authenticated = false;
Status currStatus;

// KEYS
ecc_key privateKey;
ecc_key publicKey;

/**
 * @brief Data received callback
 */
bool rx_callback(const uint8_t *data, size_t data_len, void *arg)
{

  // dump received data to serial
  Serial.println("Received data length: " + String(data_len));
  for (int i = 0; i < data_len; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(",");
  }
  Serial.println();

  if (data[1] == 0x02 && data[2] == 0xD3 && data[19] == 0x03)
  {                                                                   // check STX, 0xD3 & ETX
    ESP_LOGI("rx_callback", "Received SI-Card data from USB serial"); // TODO: verify checksum
    si_parse(data, data_len);
    si_dumpdata();
    si_clear_data();
  }
  else
  {
    ESP_LOGI("rx_callback", "Received data is not SI-Card data");
  }
  return true;
}

/**
 * @brief Device event callback
 *
 * Apart from handling device disconnection it doesn't do anything useful
 */
void handle_event(const cdc_acm_host_dev_event_data_t *event, void *user_ctx)
{
  switch (event->type)
  {
  case CDC_ACM_HOST_ERROR:
    Serial.printf("CDC-ACM error has occurred, err_no = %d\n",
                  event->data.error);
    break;
  case CDC_ACM_HOST_DEVICE_DISCONNECTED:
    Serial.println("Device suddenly disconnected");
    xSemaphoreGive(device_disconnected_sem);
    isConnected = false;
    break;
  case CDC_ACM_HOST_SERIAL_STATE:
    Serial.printf("Serial state notif 0x%04X\n",
                  event->data.serial_state.val);
    break;
  case CDC_ACM_HOST_NETWORK_CONNECTION:
    Serial.println("Network connection established");
    break;
  default:
    Serial.println("Unknown event");
    break;
  }
}

void connectDevice()
{
  if (!usbReady || isConnected)
  {
    return;
  }
  const cdc_acm_host_device_config_t dev_config = {
      .connection_timeout_ms = 5000, // 5 seconds, enough time to plug the
                                     // device in or experiment with timeout
      .out_buffer_size = ESP_USB_SERIAL_TX_BUFFER_SIZE,
      .in_buffer_size = ESP_USB_SERIAL_RX_BUFFER_SIZE,
      .event_cb = handle_event,
      .data_cb = rx_callback,
      .user_arg = NULL,
  };
  cdc_acm_line_coding_t line_coding = {
      .dwDTERate = ESP_USB_SERIAL_BAUDRATE,
      .bCharFormat = ESP_USB_SERIAL_STOP_BITS,
      .bParityType = ESP_USB_SERIAL_PARITY,
      .bDataBits = ESP_USB_SERIAL_DATA_BITS,
  };
  // You don't need to know the device's VID and PID. Just plug in any device
  // and the VCP service will pick correct (already registered) driver for the
  // device
  Serial.println("Opening any VCP device...");
  vcp = std::unique_ptr<CdcAcmDevice>(esp_usb::VCP::open(&dev_config));

  if (vcp == nullptr)
  {
    Serial.println("Failed to open VCP device, retrying...");
    return;
  }

  vTaskDelay(10);

  Serial.println("USB detected");

  if (vcp->line_coding_set(&line_coding) == ESP_OK)
  {
    Serial.println("USB Connected");
    isConnected = true;
    uint16_t vid = esp_usb::getVID();
    uint16_t pid = esp_usb::getPID();
    Serial.printf("USB device with VID: 0x%04X (%s), PID: 0x%04X (%s) found\n",
                  vid, esp_usb::getVIDString(), pid, esp_usb::getPIDString());
    xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
    vTaskDelay(10);

    vcp = nullptr;
  }
  else
  {
    Serial.println("USB device not identified");
  }
}

// this task only handle connection
static void esp_usb_serial_connection_task(void *pvParameter)
{
  (void)pvParameter;
  while (1)
  {
    /* Delay */
    vTaskDelay(pdMS_TO_TICKS(10));
    if (!usbReady)
    {
      break;
    }
    connectDevice();
  }
  /* A task should NEVER return */
  vTaskDelete(NULL);
}

void handle()
{
  if (!usbReady)
    return;
  if (Serial.available())
  {
    size_t size = Serial.available();
    uint8_t *data = (uint8_t *)malloc(size);
    if (data)
    {
      size = Serial.readBytes(data, size);
      if (vcp && vcp->tx_blocking(data, size) == ESP_OK)
      {
        if (!(vcp && vcp->set_control_line_state(true, true) == ESP_OK))
        {
          Serial.println("Failed set line");
        }
      }
      else
      {
        Serial.println("Failed to send message");
      }
      free(data);
    }
  }
}

//Gets the current counter value
int getStatusTime(){

}

void setup()
{
  usb_serial.begin(115200);

  // INIT LEDS
  status_led = StatusLED(42, 0, 1, 1, 2, 2, StatusLED::RGB_COMMON_CATHODE);
  network_led = StatusLED(6, 3, 4, 4, 5, 5, StatusLED::RGB_COMMON_CATHODE);

  status_led.setEnabled(true);
  status_led.setPulse(1);
  status_led.setColor(255, 0, 0);
  network_led.setEnabled(true);
  network_led.setPulse(2);
  network_led.setColor(0, 255, 0);

  if (ESP_OK != usb_serial_init())
  {
    Serial.println("Initialisation failed");
  }
  else
  {
    if (ESP_OK != usb_serial_create_task())
    {
      Serial.println("Task Creation failed");
    }
    else
    {
      Serial.println("Success");
    }
    device_disconnected_sem = xSemaphoreCreateBinary();
    if (device_disconnected_sem == NULL)
    {
      Serial.println("Semaphore creation failed");
      return;
    }
    BaseType_t res = xTaskCreatePinnedToCore(
        esp_usb_serial_connection_task, "esp_usb_serial_task",
        ESP_USB_SERIAL_TASK_SIZE, NULL, ESP_USB_SERIAL_TASK_PRIORITY, &xHandle,
        ESP_USB_SERIAL_TASK_CORE);
    if (res != pdPASS || !xHandle)
    {
      Serial.println("Task creation failed");
      return;
    }
    Serial.println("USB Serial Connection Task created successfully");
  }
  usbReady = true;

  // status_led = StatusLED(1);
  // status_led.setEnabled(true);
  // status_led.setPulse(2);
  // status_led.setColor(0, 0, 255);

  // FastLED.addLeds<WS2812B, 48, GRB>(leds, 1);
  // stled_setup();
  nbiot_serial.begin(115200, SERIAL_8N1, RXD1, TXD1);
  rs232_serial.begin(4800, SERIAL_8N1, RXD2, TXD2);

  // INIT QUEUE
  punchQueue = xQueueCreate(QUEUE_SIZE, sizeof(SIRecord));

  // INIT KEYS
}

void loop()
{
  status_led.show();
  network_led.show();
  // stled_loop();

  if (usbReady)
  {
    handle();
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  if (Serial.available())
  {
    char c = Serial.read();
    Serial.print("writing to serial1: ");
    Serial.println(c);
    Serial1.write(c);
  }

  if (rs232_serial.available())
  {
    for (int i = 0; i < 100; i++)
    {
      test[i] = 0;
    }
    Serial.println("reading from rs232_serial: ");
    int i = 1;
    while (rs232_serial.available())
    {
      test[i] = rs232_serial.read();
      i++;
      delay(2);
      // Serial.print(Serial1.read(), HEX);
    }
    // Serial.println("done reading from rs232_serial: ");
    ESP_LOGI("rx_callback", "Received SI-Card data from r232 serial");
    for (int i = 0; i < 100; i++)
    {
      Serial.print(test[i], HEX);
      Serial.print(",");
    }
    si_parse(test, i);
    network_led.indicate(255, 0, 0, StatusLED::SOLID, 0.5, 1000);
    si_dumpdata();
    si_clear_data();
  }


  // MAIN LOOP

  // CONNECT -> AUTH PHASE
  if (!authenticated)
  {
    
  }
  else
  {
    // PUNCHES?
    if (queue.empty())
    {

    }
    // STATUS?
    // if(getStatusTime > ){

    // }

    // CONF?
    
  }
}