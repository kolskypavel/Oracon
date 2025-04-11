#include <unity.h>
#include <Arduino.h>
#include "encryptor.h"

ecc_key key;

void test_encrypt_decrypt(void)
{
    const std::string data = "TEST DATA";
    byte out[100];
    word32 outLength;

    if (!encryptData(data, key, out, outLength))
    {
        TEST_FAIL_MESSAGE("Failed to encrypt data");
    }

    std::string decrypted;

    if (!decryptData(out, outLength, key, decrypted))
    {
        TEST_FAIL_MESSAGE("Failed to encrypt data");
    }

    TEST_ASSERT_EQUAL_STRING(decrypted.c_str(), data.c_str());
}

void setup()
{
    delay(2000); // service delay
    UNITY_BEGIN();

    RUN_TEST(test_encrypt_decrypt);

    UNITY_END(); // stop unit testing
}

void setUp(void)
{
    int ret = 0;
    WC_RNG rng;
    ret = wc_ecc_init(&key);

    if (ret != 0)
    {
        TEST_FAIL_MESSAGE("Failed to init privKey");
    }

    // Init rng
    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        wc_FreeRng(&rng);
        TEST_FAIL_MESSAGE("Failed to init rng");
    }

    ret = wc_ecc_make_key(&rng, 32, &key);

    if (ret != 0)
    {
        wc_FreeRng(&rng);
        TEST_FAIL_MESSAGE("Failed to make key");
    }

    wc_FreeRng(&rng);
}

void tearDown(void)
{
    wc_ecc_free(&key);
    wolfCrypt_Cleanup();
}

void loop()
{
}