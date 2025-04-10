#include "encryptor.h"
/*
Examples: https://github.com/wolfSSL/wolfssl-examples/blob/master/ecc/
Docs: https://www.wolfssl.com/documentation/manuals/wolfssl/ecc_8h.html
 */

bool encryptData(const std::string &data, ecc_key &pubKey, byte &out, word32 outLength)
{
    int ret = 0;
    WC_RNG rng;

    // Init rng
    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        return false;
    }

    byte outBuffer[MAX_MESSAGE_SIZE];
    ecc_key ephemeralKey;

    // Init key
    ret = wc_ecc_init(&ephemeralKey);
    if (ret != 0)
    {
        return false;
    }

    // Make new 256b ephemeral key
    ret = wc_ecc_make_key(&rng, 32, &ephemeralKey);
    if (ret != 0)
    {
        return false;
    }

    ret = wc_ecc_encrypt(&ephemeralKey, &pubKey, reinterpret_cast<const byte *>(data.data()), data.size(), outBuffer, &outLength, nullptr);

    if (ret == 0)
    {
        // Success
        return true;
    }
    return false;
}

bool decryptData(const byte * data, word32 dataLength, ecc_key &privKey, const std::string &out)
{

    byte outBuffer[MAX_MESSAGE_SIZE];
    word32 outLength;

    // The public key should be included in message - https://www.wolfssl.com/forums/topic1926-confusion-on-wceccencrypt-and-wceccdecrypt.html
    int ret = wc_ecc_decrypt(&privKey, nullptr, reinterpret_cast<const byte *>(data), dataLength, outBuffer, &outLength, NULL);
    if (ret == 0)
    {
        return true;
    }
    return false;
}

bool generateSignature(const std::string &data, const ecc_key &privKey, byte *signature, word32 outLength)
{
    int ret = 0;
    WC_RNG rng;

    // Init rng
    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        return false;
    }

    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, reinterpret_cast<const byte *>(data.data()), data.size(), signature, &outLength, &privKey, sizeof(privKey), &rng);
    if (ret != 0)
    {
        return true;
    }
    return false;
}

bool validateSignature(const byte &signature, word32 sigLength, const std::string &data, const ecc_key &pubKey)
{
    int ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, reinterpret_cast<const byte *>(data.data()), data.size(), &signature, sigLength, &pubKey, sizeof(pubKey));

    if (ret == 0)
    {
        return true;
    }
    return false;
}