#define usb_serial Serial
#define nbiot_serial Serial1
#define rs232_serial Serial2

//PORTS
#define RXD1 18
#define TXD1 17

#define RXD2 16
#define TXD2 15

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

// QUEUE MANIPULATION
#define PUNCH_QUEUE_SIZE 20
#define PUNCH_BUFFER_SIZE 5

// CRYPTO
#define HAVE_ECC
#define HAVE_ECC_ENCRYPT

#define ECC_KEY_SIZE 32
#define ECC_KEY_CURVE ECC_SECP256K1
#define MAX_MESSAGE_SIZE 1024
#define MAX_SIGNATURE_SIZE 256
#define MAX_DER_BUFF_SIZE 512

// LOGIC
#define SOCKET_TIMEOUT 10
#define MIN_STATUS_DELAY 5
#define MAX_STATUS_DELAY 15
#define INIT_MAIN_LOOP_DELAY 10
#define MAX_SIGNAL_VALUE 103
#define SYSTEM_STATS_MEASURE_DELAY 3
#define MAX_SI_DATA_SIZE 100