#define usb_serial Serial
#define nbiot_serial Serial1
#define rs232_serial Serial2

#define RXD1 18
#define TXD1 17

#define RXD2 16
#define TXD2 15

#define SERVER_IP "192.168.1.1"
#define PUBLIC_KEY "XXX"
#define PRIVATE_KEY "XXX"

#define MILLIS_TO_MICROS(a) (int)(a*1000)
#define STATUS_LED_FLICK_TIME_MS 100
#define STATUS_LED_ERROR_TIME_MS 500

// QUEUE MANIPULATION¨
#define QUEUE_SIZE 20