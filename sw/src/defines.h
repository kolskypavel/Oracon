#define usb_serial Serial
#define nbiot_serial Serial1
#define rs232_serial Serial2
#define srr_serial Serial2

// #define srr_serial Serial3

// LEDS
#define MILLIS_TO_MICROS(a) (int)(a * 1000)
#define LED_TASK_PRIORITY 15
#define STATUS_LED_ERROR_FREQ 2

// SERIAL COMMUNICATION
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

#define NB_IOT_SERIAL_BAUDRATE 115200
#define SI_RS232_SERIAL_BAUDRATE 4800
#define SI_SRR_SERIAL_BAUDRATE 38400

// BATTERY
#define MIN_BATTERY_LEVEL 3.3
#define BATTERY_ADC_STEPS 4095

#define BATTERY_LEVEL_OK 60
#define BATTERY_LEVEL_MEDIUM 30

// SIGNAL
#define MAX_SIGNAL_VALUE 113
#define SIGNAL_LEVEL_OK 70
#define SIGNAL_LEVEL_MEDIUM 100

// STRINGS
#define WHITESPACES " \t\n\r\f\v"

// QUEUE MANIPULATION
#define MAX_SI_DATA_SIZE 50
#define SI_RECORD_SIZE 19
#define PUNCH_QUEUE_SIZE 20
#define PUNCH_BUFFER_SIZE 5
#define QUEUE_DATA_FILE_NAME "/queue_data.bin"
#define QUEUE_METADATA_FILE_NAME "/queue_info.bin"

// SOCKET - adjust timeouts in case of slow connection
#define SOCKET_READ_TIMEOUT 10
#define SOCKET_HTTP_TIMEOUT 20
#define APN "\"IP\",\"lpwa.vodafone.com\""

// -------- CONFIGURE TO MATCH YOUR SCHEME ---------------

// PINS
#define RX_NBIOT_PIN 16
#define TX_NBIOT_PIN 15

#define RX_RS232_PIN 2
#define TX_RS232_PIN 1

#define RX_SRR_PIN 12
#define TX_SRR_PIN 13

#define STATUS_LED_R_PIN 7
#define STATUS_LED_G_PIN 8
#define STATUS_LED_B_PIN 9

#define SIGNAL_LED_R_PIN 39
#define SIGNAL_LED_G_PIN 40
#define SIGNAL_LED_B_PIN 41

#define BATTERY_LED_R_PIN 36
#define BATTERY_LED_G_PIN 37
#define BATTERY_LED_B_PIN 38

#define CHG_PIN 17
#define STBY_PIN 18

#define BATTERY_MEASURE_PORT 14

// LOGIC
#define MAIN_LOOP_DELAY 1
#define STATUS_DELAY 300
#define INIT_NBIOT_DELAY 15           // Initial delay for the NB-IOT module, based on the docs
#define SYSTEM_STATS_MEASURE_DELAY 10 // How often should system stats (signal and battery) be measured

// HTTP
#define HTTP_TIMEOUT 5

// TESTING
#define TEST_ORACON_SERIAL_VERBOSE // Prints the received / sent data to a serial
// #define TEST_NO_SI_TASKS                          // Don't start the tasks for receiving SI data
#define USE_SRR
#define TEST_SI_SERIAL_VERBOSE // Print info from SI serial reads
#define TEST_QUEUE_VERBOSE     // Print info from QUEUE