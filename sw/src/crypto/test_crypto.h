#include <Arduino.h>
#include "encryptor.h"
#include "defines.h"
#include "system/systemstats.h"

RsaKey key;
WC_RNG rng;

void setupCryptoTest(const DeviceStatus &status)
{
#ifdef TEST_WOLFCRYPT_GENERATE_KEY
    if (wc_InitRsaKey(&key, NULL) != 0)
    {
        throw std::invalid_argument("Failed to initialize RSA key");
    }
    if (wc_InitRng(&rng) != 0)
    {
        throw std::invalid_argument("Failed to initialize RNG");
    }

    if (wc_MakeRsaKey(&key, 2048, 65537, &rng) != 0)
    {
        wc_FreeRng(&rng);
        wc_FreeRsaKey(&key);
        throw std::invalid_argument("Failed to generate RSA key");
    }
#else
    key = status.privateKey;
#endif
}

void testAesEncryptDecrypt()
{
    const std::string plaintext = "AES TEST DATA";
    std::string decrypted;
    byte encrypted[MAX_MESSAGE_SIZE];
    word32 encryptedLength = MAX_MESSAGE_SIZE;

    try
    {
        // Set the AES key from a predefined string (16 bytes for AES-128)
        const byte predefinedKey[16] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6'};

        // Encrypt the plaintext
        encryptDataAes(plaintext, predefinedKey, encrypted, encryptedLength);
        ESP_LOGI("CRYPTO TEST", "Successfully encrypted data with AES, length %d", encryptedLength);

        // Decrypt the encrypted data
        decryptDataAes(encrypted, encryptedLength, predefinedKey, decrypted);
        ESP_LOGI("CRYPTO TEST", "Successfully decrypted data with AES");
    }
    catch (const std::invalid_argument &ex)
    {
        ESP_LOGE("CRYPTO TEST", "Failed to encrypt/decrypt AES data %s", ex.what());
    }

    if (decrypted != plaintext)
    {
        ESP_LOGE("CRYPTO TEST", "AES output mismatch!");
        return;
    }
    ESP_LOGI("CRYPTO TEST", "AES encryption/decryption success");
}

// void testRsaEncryptDecrypt()
// {
//     const std::string data = "TEST DATA";
//     std::string decrypted;
//     byte out[MAX_MESSAGE_SIZE];
//     word32 outLength = MAX_MESSAGE_SIZE;

//     try
//     {
//         encryptDataRsa(data, key, out, outLength);
//         ESP_LOGI("CRYPTO TEST", "Successfully encrypted data");

//         decryptDataRsa(out, outLength, key, decrypted);
//     }
//     catch (const std::invalid_argument &ex)
//     {
//         ESP_LOGE("CRYPTO TEST", "Failed to encrypt/decrypt data %s", ex.what());
//     }

//     if (decrypted != data)
//     {
//         ESP_LOGE("CRYPTO TEST", "Output mismatch!");
//         return;
//     }
//     ESP_LOGI("CRYPTO TEST", "Success");
// }

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
    setupCryptoTest(status);
    //  testRsaEncryptDecrypt();
    test_signature();
    testAesEncryptDecrypt();

    wc_FreeRng(&rng);
#ifdef TEST_WOLFCRYPT_GENERATE_KEY
    wc_FreeRsaKey(&key);
#endif
}