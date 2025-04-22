# Oracon - Software for ESP32 end stations
Desired to implement splits transmission from SI stations used in orienteering.

## Usage
 1. Download Platformio
 2. Clone the git repo
 3. Create / download from the gate the secrets.h file and place it in the /src folder
 4. Build and upload

## secrets.h example
File used for device configuration, should not be versioned!
```
#define SERVER_IP "1.1.1.1"
#define SERVER_PORT "6666"

#define DEVICE_ID 1005
#define DEVICE_PRIVATE_KEY "-----BEGIN PRIVATE KEY-----MHcCAQEEIP71RUbA3fVub/Y+0izX/Y+NbRAELbsxExTtYZc/PPSUoAoGCCqGSM49AwEHoUQDQgAEF0d9dsD8yAfmiPovU30vCEOJYusEVL5/DQVesTAE9HOsc2NCDPFVxKCWrYnqmM1vfmgPZPPzERJ7VXZYlFwV8g==-----END PRIVATE KEY-----"
#define DEVICE_PUBLIC_KEY "-----BEGIN PUBLIC KEY-----MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEF0d9dsD8yAfmiPovU30vCEOJYusEVL5/DQVesTAE9HOsc2NCDPFVxKCWrYnqmM1vfmgPZPPzERJ7VXZYlFwV8g==-----END PUBLIC KEY-----"
#define SERVER_PUBLIC_KEY "-----BEGIN PUBLIC KEY-----MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEM+5Ho/L9UJnrdwLVBxbs0Ju6VTf/PI2V6gHrMj3hf4DCY2XlL4cqBN+rUeEotkRYqPsvz5vzS6HW98DpIwcDRw==-----END PUBLIC KEY-----"

#define INIT_STATUS_DELAY 20
```
## Debugging and configuring
Use the provided `defines.h` file for adjusting parameters such as delays, verbose output and so on. Further explanation TBA.
