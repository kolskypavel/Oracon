#include <Arduino.h>
#include "encryptor.h"
#include "defines.h"
#include "system/systemstats.h"

void test_encrypt_decrypt(DeviceStatus &status)
{
    const std::string data = "TEST DATA";
    std::string decrypted;
    byte out[100];
    word32 outLength;

    try
    {
        encryptData(data, status.key, out, outLength);

        decryptData(out, outLength, status.key, decrypted);
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("CRYPTO TEST", "Failed to encrypt/decrypt data");
    }

    if (decrypted != data)
    {
        ESP_LOGE("CRYPTO TEST", "Output mismatch!");
        return;
    }
    ESP_LOGI("CRYPTO TEST", "Success");
}

void test_signature(const DeviceStatus &status)
{
    const std::string message = "SIGNATURE TEST MESSAGE";
    byte signature[MAX_SIGNATURE_SIZE];
    word32 sigLength = sizeof(signature);
    bool isValid = false;

    try
    {
        // Sign the message
        generateSignature(message, status.key, signature, sigLength);

        // Verify the signature
        verifySignature(signature, sigLength, message, status.key);
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("CRYPTO TEST", "Failed to sign/verify data");
    }

    if (!isValid)
    {
        ESP_LOGE("CRYPTO TEST", "Signature verification failed!");
        return;
    }
    ESP_LOGI("CRYPTO TEST", "Signature test success");
}

void runCryptoTest(DeviceStatus &status)
{
    test_encrypt_decrypt(status);
    test_signature(status);
}