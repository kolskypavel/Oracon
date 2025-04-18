/**
 * Used libraries:
 * https://github.com/luc-github/esp32-usb-serial/
 */

#include <Arduino.h>
#include <Preferences.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp32_usb_serial.h"
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>

#include "led/statusled.hpp"
#include "si/si_parser.h"
#include "defines.h"
#include "system/systemstats.h"
#include "protocol/protocol_message.h"
#include "protocol/socket.h"
#include "protocol/exceptions.h"

#include "crypto/test_crypto.h"

// DO NOT INCLUDE in VCS
#include "secrets.h"
#include <wolfssl/wolfcrypt/logging.h>

SemaphoreHandle_t device_disconnected_sem;
std::unique_ptr<CdcAcmDevice> vcp;
bool isConnected = false;
bool usbReady = false;
TaskHandle_t xHandle;

StatusLED status_led, network_led;
QueueHandle_t punchQueue;

// Timers
unsigned long statusStartSeconds;
unsigned long statusCurrSeconds;
unsigned long measureStartSeconds;
unsigned long measureCurrSeconds;

SIRecord punches[PUNCH_BUFFER_SIZE];
bool punchesSent = true;
int received = 0;

// STATUS
DeviceStatus currStatus;
ProtocolMessage message;

Preferences prefs;

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

  // Check if received data matches SI data
  if (data[1] == BYTE_STX && data[2] == BYTE_PUNCH_DATA && data[19] == BYTE_ETX)
  {
    ESP_LOGI("rx_callback", "Received SI-Card data from USB serial"); // TODO: verify checksum
    parseSIdata(data, data_len);
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

static void rs232_serial_task(void *pvParameter)
{
  uint8_t buffer[MAX_SI_DATA_SIZE];
  boolean finished = true;

  while (true)
  {
    finished = true;
    // Clear buffer
    for (int c = 0; c < MAX_SI_DATA_SIZE; c++)
    {
      buffer[c] = 0;
    }

    if (rs232_serial.available())
    {
      int i = 0;
      while (rs232_serial.available())
      {
        // Prevent buffer overflow
        if (i >= MAX_SI_DATA_SIZE)
        {
          finished = false;
          break;
        }

        buffer[i] = rs232_serial.read();
        i++;
        delay(2);
      }
      if (finished)
      {
        ESP_LOGI("RS232", "Received SI-Card data from r232 serial");
        SIRecord record = parseSIdata(buffer, i);

        if (xQueueSend(punchQueue, &record, 0) != pdPASS)
        {
          ESP_LOGE("RS232", "Queue is full");
          // TODO: signal
        }
      }
    }
  }
}

void initTasks()
{
  BaseType_t res = xTaskCreate(
      rs232_serial_task, "rs232_serial_task",
      ESP_USB_SERIAL_TASK_SIZE, nullptr, ESP_USB_SERIAL_TASK_PRIORITY, nullptr);

  if (res != pdPASS)
  {
    throw std::runtime_error("Failed to init RS232 task");
  }

  //  res = xTaskCreatePinnedToCore(
  //   esp_usb_serial_connection_task, "esp_usb_serial_task",
  //   ESP_USB_SERIAL_TASK_SIZE, NULL, ESP_USB_SERIAL_TASK_PRIORITY, &xHandle,
  //   ESP_USB_SERIAL_TASK_CORE);

  //   if (res != pdPASS || !xHandle)
  //   {
  //     throw std::runtime_error("Failed to init USB task");
  //   }

  // if (ESP_OK != usb_serial_init())
  // {
  //   Serial.println("Initialization failed");
  // }
  // else
  // {
  //   if (ESP_OK != usb_serial_create_task())
  //   {
  //     Serial.println("Task Creation failed");
  //   }
  //   else
  //   {
  //     Serial.println("Success");
  //   }
  //   device_disconnected_sem = xSemaphoreCreateBinary();
  //   if (device_disconnected_sem == NULL)
  //   {
  //     Serial.println("Semaphore creation failed");
  //     return;
  //   }
  //   BaseType_t res = xTaskCreatePinnedToCore(
  //       esp_usb_serial_connection_task, "esp_usb_serial_task",
  //       ESP_USB_SERIAL_TASK_SIZE, NULL, ESP_USB_SERIAL_TASK_PRIORITY, &xHandle,
  //       ESP_USB_SERIAL_TASK_CORE);

  //   if (res != pdPASS || !xHandle)
  //   {
  //     Serial.println("Task creation failed");
  //     return;
  //   }
  //   Serial.println("USB Serial Connection Task created successfully");
  // }
  // usbReady = true;
}

void initStatus()
{
  currStatus.socketStatus = SocketStatus::SOCKET_OFF;
  currStatus.punchesReceived = 0;

  // If config values are stored in memory, use them, otherwise use the preset
  if (prefs.isKey("statusDelay"))
  {
    currStatus.config.statusDelay = prefs.getUChar("statusDelay");
  }
  else
  {
    currStatus.config.statusDelay = INIT_STATUS_DELAY;
  }

  currStatus.deviceId = DEVICE_ID;
  currStatus.key = loadKey(DEVICE_PRIVATE_KEY, true);
  currStatus.serverKey = loadKey(SERVER_PUBLIC_KEY, false);

  ESP_LOGI("INIT", "Status init successful");
}

void setup()
{
  // Init serial ports
  usb_serial.begin(115200);
  rs232_serial.begin(SI_RS232_SERIAL_BAUDRATE, SERIAL_8N1, RXD1, TXD1);
  nbiot_serial.begin(NB_IOT_SERIAL_BAUDRATE, SERIAL_8N1, RXD2, TXD2);

  delay(10);
  nbiot_serial.println();

  // INIT LEDS
  status_led = StatusLED(42, 0, 1, 1, 2, 2, StatusLED::RGB_COMMON_CATHODE);
  network_led = StatusLED(6, 3, 4, 4, 5, 5, StatusLED::RGB_COMMON_CATHODE);
  status_led.setEnabled(true);
  network_led.setEnabled(true);

  // INIT TIMERS
  statusStartSeconds = getCurrentTime();
  measureStartSeconds = getCurrentTime();

  // INIT QUEUE
  punchQueue = xQueueCreate(PUNCH_QUEUE_SIZE, sizeof(SIRecord));

  // INIT PREFS
  prefs.begin("config", false);

  try
  {
    // INIT STATUS
    initStatus();

    // INIT SOCKET
    initSocket(currStatus);

    // INIT TASKS
    // initTasks();
  }
  catch (const std::runtime_error &ex)
  {
    // Failed to init
    currStatus.runStatus = RunStatus::INIT_ERROR;
    status_led.setColorPreset(StatusLED::RED);
    ESP_LOGE("INIT", "Failed to init, cause: %s", ex.what());
  }

  // test_encrypt_decrypt(currStatus);
  test_signature(currStatus);
  // Intial delay for NB-IOT module
  // vTaskDelay(pdMS_TO_TICKS(INIT_MAIN_LOOP_DELAY * 1000));
}

// Receives punches till no punches are left or the buffer is full
void receivePunches()
{
  int count = 0;
  while (count < PUNCH_BUFFER_SIZE && xQueueReceive(punchQueue, &punches[count], 0) == pdPASS)
  {
    count++;
  }
  received = count;
}

// if (usbReady)
// {
//   handle();
//   vTaskDelay(pdMS_TO_TICKS(10));
// }

void loop()
{
  status_led.show();
  network_led.show();

  if (currStatus.runStatus != RunStatus::INIT_ERROR)
  {
    try
    {
      measureCurrSeconds = getCurrentTime();
      if ((measureCurrSeconds - measureStartSeconds) > SYSTEM_STATS_MEASURE_DELAY)
      {
        currStatus.updateBatteryLevel();
        getSignalStrength(currStatus);
      }
      measureStartSeconds = getCurrentTime();

      // MAIN LOOP
      ESP_LOGI("MAIN", "Socket status: %d", currStatus.socketStatus);

      switch (currStatus.socketStatus)
      {
      case SocketStatus::SOCKET_AUTHENTICATED:
      {

        status_led.setColorPreset(StatusLED::GREEN);

        // PUNCHES?
        if (punchesSent)
        {
          receivePunches();
          if (received > 0)
          {
            punchesSent = false; // In case exception gets thrown, so the records don't get lost
            punchesSent = sendPunches(currStatus, punches, received);
          }
        }
        // STATUS?
        statusCurrSeconds = getCurrentTime();

        if ((statusCurrSeconds - statusStartSeconds) > currStatus.config.statusDelay)
        {
          ESP_LOGI("STATUS", "Time period elapsed");

          sendStatus(currStatus, prefs);
          statusStartSeconds = getCurrentTime();
        }
      }
      break;

      case SocketStatus::SOCKET_CONNECTED:
        authenticateDevice(currStatus);
        break;

      case SocketStatus::SOCKET_OFF:
        connectSocket(currStatus);
        break;
      }
    }
    // Non-fatal errors
    catch (const std::invalid_argument &ex)
    {
      // TODO: blink led or something
      ESP_LOGE("INVALID_ARGUMENT", "Error: %s", ex.what());
    }

    // Connection error -> disconnect socket
    catch (const SocketException &ex)
    {
      ESP_LOGE("SOCKET_EXCEPTION", "Error: %s", ex.what());

      if (currStatus.socketStatus == SocketStatus::SOCKET_AUTHENTICATED ||
          currStatus.socketStatus == SocketStatus::SOCKET_CONNECTED)
      {
        closeSocket(currStatus);
      }
      currStatus.runStatus = RunStatus::SOCKET_ERROR;
      currStatus.socketStatus = SocketStatus::SOCKET_OFF;
      // TODO: set out the status LEDs
      status_led.setColorPreset(StatusLED::ORANGE);
    }
  }
  vTaskDelay(pdMS_TO_TICKS(3000));
}