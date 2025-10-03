/**
 * Used libraries:
 * https://github.com/luc-github/esp32-usb-serial/
 * StatusLED by David Rothbauer
 */

#include <Arduino.h>
#include <Preferences.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <esp_task_wdt.h>
#include "esp32_usb_serial.h"

#include "led/statusled.h"
#include "si/si_parser.h"
#include "si/si_queue.h"
#include "defines.h"
#include "system/systemstats.h"
#include "protocol/socket.h"
#include "protocol/exceptions.h"

// DO NOT INCLUDE in VCS
#include "secrets.h"

// USB handling - library esp32_usb_serial
SemaphoreHandle_t device_disconnected_sem;
std::unique_ptr<CdcAcmDevice> vcp;
bool isConnected = false;
bool usbReady = false;
TaskHandle_t xHandle;

StatusLED status_led, signal_led, battery_led;

// Timers
unsigned long statusStartSeconds;
unsigned long statusCurrSeconds;
unsigned long measureStartSeconds;
unsigned long measureCurrSeconds;
uint8_t received = 0;

// Punches sending
SIRecord punches[PUNCH_BUFFER_SIZE];

// Status + prefs
DeviceStatus currStatus;
Preferences prefs;

/**
 * @brief Data received callback
 */
bool rx_callback(const uint8_t *data, size_t data_len, void *arg)
{

#ifdef TEST_SI_SERIAL_VERBOSE
  // dump received data to serial
  Serial.println("Received data length: " + String(data_len));
  for (int i = 0; i < data_len; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print(",");
  }
  Serial.println();
#endif

  // Check if received data matches SI data - first byte is always FF (skip)
  if (data[1] == BYTE_STX && data[2] == BYTE_PUNCH_DATA && data[19] == BYTE_ETX)
  {
    SIRecord record;

    // Try to parse data
    if (parseSIdata(data + 1, record))
    {
#ifdef TEST_SI_SERIAL_VERBOSE
      ESP_LOGI("USB", "Parsed SI-Card data from USB serial:[S %d,C %d, T %s]",
               record.stationNumber,
               record.cardNumber,
               record.time);
#endif
      if (!enqueueRecord(record))
      {
        ESP_LOGE("USB", "Queue is full");
        status_led.setMode(StatusLED::FLASH, STATUS_LED_ERROR_FREQ);
      }
      else
      {
        status_led.setMode(StatusLED::SOLID, 0);
      }
    }
  }
#ifdef TEST_SI_SERIAL_VERBOSE
  else
  {
    ESP_LOGI("USB", "Received data is not SI-Card data");
  }
#endif
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
// and the VCP service will pick correct (already registered) driver for the device
#ifdef TEST_SI_SERIAL_VERBOSE

#endif
  vcp = std::unique_ptr<CdcAcmDevice>(esp_usb::VCP::open(&dev_config));

  if (vcp == nullptr)
  {
    return;
  }

  vTaskDelay(10);

  if (vcp->line_coding_set(&line_coding) == ESP_OK)
  {
    isConnected = true;
    uint16_t vid = esp_usb::getVID();
    uint16_t pid = esp_usb::getPID();
#ifdef TEST_SI_SERIAL_VERBOSE
    ESP_LOGW("USB", "Device with VID: 0x%04X (%s), PID: 0x%04X (%s) found\n",
             vid, esp_usb::getVIDString(), pid, esp_usb::getPIDString());
#endif
    xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
    vTaskDelay(10);

    vcp = nullptr;
  }
  else
  {
#ifdef TEST_SI_SERIAL_VERBOSE
    ESP_LOGW("USB", "USB device not identified");
#endif
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

      int read = 0;
      while (rs232_serial.available())
      {
        // Prevent buffer overflow
        if (read >= MAX_SI_DATA_SIZE)
        {
          finished = false;
          break;
        }

        buffer[read] = rs232_serial.read();
        read++;
        delay(3);
      }

#ifdef TEST_SI_SERIAL_VERBOSE
      ESP_LOGI("RS232", "Received data: %s", dataToHex(buffer, read).c_str());
#endif

      if (finished &&
          read >= SI_RECORD_SIZE &&
          buffer[0] == BYTE_STX &&
          buffer[1] == BYTE_PUNCH_DATA &&
          buffer[18] == BYTE_ETX)
      {
        SIRecord record;

        // Try to parse data
        if (parseSIdata(buffer, record))
        {
#ifdef TEST_SI_SERIAL_VERBOSE
          ESP_LOGI("RS232", "Parsed SI-Card data from r232 serial:[S %d,C %d, T %s]",
                   record.stationNumber,
                   record.cardNumber,
                   record.time);
#endif
          if (!enqueueRecord(record))
          {
            ESP_LOGE("RS232", "Queue is full");

            status_led.setMode(StatusLED::PULSE, STATUS_LED_ERROR_FREQ);
          }
          else
          {
            status_led.setMode(StatusLED::SOLID, 0);
          }
        }
      }
#ifdef TEST_SI_SERIAL_VERBOSE
      else
      {
        ESP_LOGI("RS232", "Received data is not SI-Card data");
      }
#endif
    }
    delay(1); // Prevent WDT from reset
  }
  // Fail safe - task shouldn't return
  vTaskDelete(nullptr);
}

static void led_show_task(void *pvParameter)
{
  while (true)
  {
    battery_led.show();
    status_led.show();
    signal_led.show();
    delay(1); // Prevent WDT from reset
  }

  // Fail safe - task shouldn't return
  vTaskDelete(nullptr);
}

void initTasks()
{
  BaseType_t res = xTaskCreate(
      led_show_task, "led_show_task",
      2048, nullptr, LED_TASK_PRIORITY, nullptr);

  if (res != pdPASS)
  {
    throw std::runtime_error("Failed to init LED task");
  }

#ifndef TEST_NO_SI_TASKS
  res = xTaskCreate(
      rs232_serial_task, "rs232_serial_task",
      4096, nullptr, ESP_USB_SERIAL_TASK_PRIORITY, nullptr);

  if (res != pdPASS)
  {
    throw std::runtime_error("Failed to init RS232 task");
  }

  if (ESP_OK != usb_serial_init())
  {
    throw std::runtime_error("USB initialization failed");
  }
  else
  {
    if (ESP_OK != usb_serial_create_task())
    {
      throw std::runtime_error("USB serial task failed");
    }

    device_disconnected_sem = xSemaphoreCreateBinary();
    if (device_disconnected_sem == NULL)
    {
      throw std::runtime_error("USB semaphore init failed");
    }
    res = xTaskCreatePinnedToCore(
        esp_usb_serial_connection_task, "esp_usb_serial_task",
        ESP_USB_SERIAL_TASK_SIZE, NULL, ESP_USB_SERIAL_TASK_PRIORITY, &xHandle,
        ESP_USB_SERIAL_TASK_CORE);

    if (res != pdPASS || !xHandle)
    {
      throw std::runtime_error("USB connection task init failed");
    }
  }
  usbReady = true;
#endif

  ESP_LOGI("TASKS", "Tasks init successfuly");
}

void initLEDs()
{
  status_led = StatusLED(STATUS_LED_R_PIN, 0, STATUS_LED_G_PIN, 1, STATUS_LED_B_PIN, 2, StatusLED::RGB_COMMON_CATHODE);
  signal_led = StatusLED(SIGNAL_LED_R_PIN, 3, SIGNAL_LED_G_PIN, 4, SIGNAL_LED_B_PIN, 5, StatusLED::RGB_COMMON_CATHODE);
  battery_led = StatusLED(BATTERY_LED_R_PIN, 6, BATTERY_LED_G_PIN, 7, BATTERY_LED_B_PIN, 8, StatusLED::RGB_COMMON_CATHODE);

  battery_led.setMode(StatusLED::MODE::SOLID, 0);
  signal_led.setMode(StatusLED::MODE::SOLID, 0);
  status_led.setMode(StatusLED::MODE::SOLID, 0);

  // Set blue -> init phase
  battery_led.setColorPreset(StatusLED::COLOR_PRESET::BLUE);
  status_led.setColorPreset(StatusLED::COLOR_PRESET::BLUE);
  signal_led.setColorPreset(StatusLED::COLOR_PRESET::BLUE);

  battery_led.show();
  status_led.show();
  signal_led.show();
}

void initStatus()
{
  currStatus.httpStatus = HttpStatus::HTTP_OFF;
  currStatus.punchesReceived = 0;
  currStatus.signal = 113;
  currStatus.battery = 0;

  ESP_LOGI("INIT", "Status init successful");
}

void setup()
{
  // Watchdog timer tuning
  //  esp_task_wdt_init(18, true);  // Timeout in seconds, panic enabled
  //  esp_task_wdt_add(NULL);

  // Init serial ports
  usb_serial.begin(115200);
  rs232_serial.begin(SI_RS232_SERIAL_BAUDRATE, SERIAL_8N1, RX_RS232_PIN, TX_RS232_PIN);
  nbiot_serial.begin(NB_IOT_SERIAL_BAUDRATE, SERIAL_8N1, RX_NBIOT_PIN, TX_NBIOT_PIN);

  pinMode(BOOST_ENABLE, OUTPUT);
  digitalWrite(BOOST_ENABLE, 1);

  // INIT LEDS
  initLEDs();

  // INIT BATTERY MEASUREMET
  pinMode(BATTERY_MEASURE_PORT, INPUT);

  // INIT TIMERS
  statusStartSeconds = getCurrentTime();
  measureStartSeconds = getCurrentTime();

  // INIT PREFS
  prefs.begin("config", false);

  // Intial delay for NB-IOT module
#ifndef NO_SETUP_TIMEOUT
  delay(INIT_NBIOT_DELAY * 1000);
#endif

  // Clear buffer
  delay(10);
  nbiot_serial.println();

  try
  {
    // INIT QUEUE
    initQueue();

    // INIT STATUS
    initStatus();

    // INIT TASKS
    initTasks();

    currStatus.init = true;
  }
  catch (const std::runtime_error &ex)
  {
    // Failed to init
    currStatus.init = false;
    status_led.setColorPreset(StatusLED::RED);
    ESP_LOGE("INIT", "Failed to init, cause: %s", ex.what());
  }

#ifdef TEST_WOLFCRYPT
  testCrypto(currStatus);
#endif
}

void setLeds()
{
  // Battery LED
  if (currStatus.battery >= BATTERY_LEVEL_OK)
  {
    battery_led.setColorPreset(StatusLED::COLOR_PRESET::GREEN);
  }
  else if (currStatus.battery >= BATTERY_LEVEL_MEDIUM)
  {
    battery_led.setColorPreset(StatusLED::COLOR_PRESET::ORANGE);
  }
  else
  {
    battery_led.setColorPreset(StatusLED::COLOR_PRESET::RED);
  }

  // Signal LED
  if (currStatus.signal <= SIGNAL_LEVEL_OK)
  {
    signal_led.setColorPreset(StatusLED::COLOR_PRESET::GREEN);
  }
  else if (currStatus.signal <= SIGNAL_LEVEL_MEDIUM)
  {
    signal_led.setColorPreset(StatusLED::COLOR_PRESET::ORANGE);
  }
  else
  {
    signal_led.setColorPreset(StatusLED::COLOR_PRESET::RED);
  }
}

void loop()
{
  setLeds();

  if (currStatus.init)
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
      if (currStatus.httpStatus == HttpStatus::HTTP_INIT)
      {
        status_led.setColorPreset(StatusLED::GREEN);

        if (received == 0)
        {
          receiveRecords(punches, received);
        }

        if (received > 0)
        {
          ESP_LOGI("MAIN", "Sending punches");
          if (sendPunches(currStatus, punches, received))
          {
            removeRecords(received);
            received = 0;
          }
        }
        // STATUS?
        statusCurrSeconds = getCurrentTime();

        if ((statusCurrSeconds - statusStartSeconds) > STATUS_DELAY)
        {
          ESP_LOGI("STATUS", "Time period elapsed");

          sendStatus(currStatus, prefs);
          statusStartSeconds = getCurrentTime();
        }
      }

      else
      {
        status_led.setColorPreset(StatusLED::ORANGE);
        initHttp(currStatus);
      }
    }
    // Non-fatal errors - can recover without restarting socket
    catch (const std::exception &ex)
    {
      // TODO: signal out
      ESP_LOGE("INVALID_ARGUMENT", "Error: %s", ex.what());
      clearInBuffer();
    }
    delay(MAIN_LOOP_DELAY * 1000);
  }
  delay(1000);
}