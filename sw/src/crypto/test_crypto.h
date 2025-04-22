#include <Arduino.h>
#include "encryptor.h"
#include "defines.h"
#include "system/systemstats.h"

ecc_key key;
WC_RNG rng;

void setupCryptoTest()
{
    if (wc_ecc_init(&key) != 0)
    {
        throw std::invalid_argument("Failed to initialize ECC key");
    }
    if (wc_InitRng(&rng) != 0)
    {
        throw std::invalid_argument("Failed to initialize RNG");
    }

    if (wc_ecc_make_key(&rng, 32, &key) != 0)
    {
        wc_FreeRng(&rng);
        throw std::invalid_argument("Failed to generate ECC key");
    }
}
void test_encrypt_decrypt()
{
    const std::string data = "TEST DATA";
    std::string decrypted;
    byte out[MAX_MESSAGE_SIZE];
    word32 outLength = MAX_MESSAGE_SIZE;

    try
    {
        encryptData(data, key, out, outLength);

        decryptData(out, outLength, key, decrypted);
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("CRYPTO TEST", "Failed to encrypt/decrypt data %s", ex.what());
    }

    if (decrypted != data)
    {
        ESP_LOGE("CRYPTO TEST", "Output mismatch!");
        return;
    }
    ESP_LOGI("CRYPTO TEST", "Success");
}

void test_signature()
{
    const std::string message = "SIGNATURE TEST MESSAGE";
    byte signature[MAX_SIGNATURE_SIZE];
    word32 sigLength = sizeof(signature);
    bool isValid = false;

    try
    {
        // Sign the message
        generateSignature(message, key, signature, sigLength);

        // Verify the signature
        isValid = verifySignature(message, key, signature, sigLength);
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("CRYPTO TEST", "Failed to sign/verify data %s", ex.what());
    }

    if (!isValid)
    {
        ESP_LOGE("CRYPTO TEST", "Signature verification failed!");
        return;
    }
    ESP_LOGI("CRYPTO TEST", "Signature test success");
}

void testCrypto(const DeviceStatus &status)
{
    setupCryptoTest();
   test_encrypt_decrypt();
    test_signature();

    wc_FreeRng(&rng);
}
