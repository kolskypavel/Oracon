#define usb_serial Serial
#define nbiot_serial Serial1
#define rs232_serial Serial2

// PORTS
#define RX_NBIOT 16
#define TX_NBIOT 15

#define RX_RS232 18
#define TX_RS232 17 

// LEDS
#define MILLIS_TO_MICROS(a) (int)(a * 1000)
#define STATUS_LED_FLICK_TIME_MS 100
#define STATUS_LED_ERROR_TIME_MS 500

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

// BATTERY
#define MAX_BATTERY_LEVEL 3.7
#define BATTERY_MEASURE_PORT 12

#define BATTERY_LEVEL_OK 60
#define BATTERY_LEVEL_MEDIUM 30

// SIGNAL
#define SIGNAL_LEVEL_OK 70
#define SIGNAL_LEVEL_MEDIUM 100

// STRINGS
#define WHITESPACES " \t\n\r\f\v"

// QUEUE MANIPULATION
#define PUNCH_QUEUE_SIZE 20
#define PUNCH_BUFFER_SIZE 5

// CRYPTO
#define MESSAGE_BLOCK_SIZE 128
#define MAX_MESSAGE_SIZE 2048
#define MAX_SIGNATURE_SIZE 256
#define MAX_DER_BUFF_SIZE 512

// SOCKET
#define SOCKET_OPEN_TIMEOUT 5
#define SOCKET_CONNECT_TIMEOUT 10
#define SOCKET_READ_TIMEOUT 15
#define SOCKET_READ_MODE "2" // 2 - ascii, 3 - hex
#define SOCKET_READ_SIZE 200

// LOGIC
#define MIN_STATUS_DELAY 10
#define MAX_STATUS_DELAY 30
#define INIT_MAIN_LOOP_DELAY 10
#define MAX_SIGNAL_VALUE 103
#define SYSTEM_STATS_MEASURE_DELAY 10
#define MAX_SI_DATA_SIZE 50
#define SI_RECORD_SIZE 19

// TESTING
// #define LIMIT_NB_IOT_SERIAL                    // Checks for max length of received message
#define NO_SETUP_TIMEOUT                          // Delay the device after startup to load NB-IOT module
#define TEST_WOLFCRYPT                            // Perform test of encrypt/decrypt and signing
//#define TEST_WOLFCRYPT_GENERATE_KEY             // Generates a key for the test
#define TEST_ORACON_SERIAL_VERBOSE                // Prints the received / sent data to a serial
#define TEST_ORACON_NO_ENCRYPTION                 // Runs unencrypted verison of protocol
// #define TEST_ORACON_NO_SIGNATURE_VERIFICATION  // Doesn't verify signature from the server
#define TEST_NO_SI_TASKS                          // Don't start the tasks for receiving SI data
#define TEST_SI_SERIAL_VERBOSE                    // Print info from SI serial reads
#define TEST_ORACON_NO_TIMEOUT                    // Don't timeout on the socket connection