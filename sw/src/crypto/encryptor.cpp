#include "encryptor.h"
/*
Examples: https://github.com/wolfSSL/wolfssl-examples/blob/master/ecc/
Docs: https://www.wolfssl.com/documentation/manuals/wolfssl/ecc_8h.html
 */

std::string addPKCS7Padding(const std::string &data)
{
    std::string padded = data;
    size_t padLen = MESSAGE_BLOCK_SIZE - (data.size() % MESSAGE_BLOCK_SIZE);
    if (padLen == 0)
        padLen = MESSAGE_BLOCK_SIZE; // Full block padding if already aligned

    return padded.append(padLen, static_cast<char>(padLen));
}

void removePKCS7Padding(std::string &data)
{
    if (data.empty())
    {
        throw std::invalid_argument("Data is empty, cannot remove padding");
    }

    // Get the value of the last byte
    unsigned char padLen = static_cast<unsigned char>(data.back());

    // Validate padding length
    if (padLen == 0 || padLen > MESSAGE_BLOCK_SIZE)
    {
        throw std::invalid_argument("Invalid padding length");
    }

    for (size_t i = 0; i < padLen; ++i)
    {
        if (data[data.size() - 1 - i] != static_cast<char>(padLen))
        {
            throw std::invalid_argument("Invalid padding bytes");
        }
    }

    // Remove padding
    data.resize(data.size() - padLen);
}

void encryptData(const std::string &data, ecc_key &pubKey, byte *out, word32 &outLength)
{
    int ret = 0;
    WC_RNG rng;

    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        throw std::invalid_argument("ENCRYPT: Failed to init RNG");
    }
    ecc_key ephemeralKey;

    // Init key
    ret = wc_ecc_init(&ephemeralKey);
    if (ret != 0)
    {
        throw std::invalid_argument("ENCRYPT: Failed to init ephemeral key, ret code: " + std::to_string(ret));
    }

    // Make new 256b ephemeral key
    ret = wc_ecc_make_key(&rng, 32, &ephemeralKey);
    if (ret != 0)
    {
        wc_FreeRng(&rng);
        wc_ecc_free(&ephemeralKey);
        throw std::invalid_argument("ENCRYPT: Failed to make ephemeral key, ret code: " + std::to_string(ret));
    }

    // TODO: Padding
    std::string padded = addPKCS7Padding(data);

    ret = wc_ecc_set_rng(&ephemeralKey, &rng);
    if (ret != 0)
    {
        wc_FreeRng(&rng);
        wc_ecc_free(&ephemeralKey);
        throw std::invalid_argument("ENCRYPT: Failed to set RNG for a key");
    }

    ret = wc_ecc_encrypt(&ephemeralKey, &pubKey, reinterpret_cast<const byte *>(padded.data()), padded.size(), out, &outLength, nullptr);
    if (ret == 0)
    {
        // Success
        wc_FreeRng(&rng);
        wc_ecc_free(&ephemeralKey);
        return;
    }
    throw std::invalid_argument("ENCRYPT: Failed to encrypt given data, ret code: " + std::to_string(ret));
}

void decryptData(const byte *data, word32 dataLength, ecc_key &privKey, std::string &out)
{

    byte outBuffer[MAX_MESSAGE_SIZE];
    word32 outLength;

    // The public key should be included in message - https://www.wolfssl.com/forums/topic1926-confusion-on-wceccencrypt-and-wceccdecrypt.html
    int ret = wc_ecc_decrypt(&privKey, nullptr, reinterpret_cast<const byte *>(data), dataLength, outBuffer, &outLength, NULL);
    if (ret == 0)
    {
        out = std::string(reinterpret_cast<char *>(outBuffer), outLength);
        removePKCS7Padding(out);
        return;
    }
    throw std::invalid_argument("DECRYPT: Failed to decrypt data, ret code: " + std::to_string(ret));
}

void generateSignature(const std::string &data, const ecc_key &privKey, byte *signature, word32 &outLength)
{
    int ret = 0;
    WC_RNG rng;

    // Init rng
    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        throw std::invalid_argument("SIGNATURE: Failed to init RNG, ret code: " + std::to_string(ret));
    }

    ret = wc_SignatureGenerate(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, reinterpret_cast<const byte *>(data.data()), data.size(), signature, &outLength, &privKey, sizeof(privKey), &rng);
    wc_FreeRng(&rng);
    if (ret == 0)
    {
        return;
    }
    throw std::invalid_argument("SIGNATURE: Failed to generate signature ret code: " + std::to_string(ret));
}

bool verifySignature(const std::string &data, const ecc_key &key, const byte *signature, word32 sigLength)
{
    int ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, reinterpret_cast<const byte *>(data.data()), data.size(), signature, sigLength, &key, sizeof(key));

    if (ret == 0)
    {
        return true;
    }
    ESP_LOGE("SIGN", "Failed to verify signature, err: %d", ret);
    return false;
}

ecc_key loadKey(const char *keyPem, bool isPrivate)
{
    ecc_key key;
    int ret = 0;
    word32 idx = 0;

    byte derBuff[MAX_DER_BUFF_SIZE];

    // Initialize the ECC key structure
    ret = wc_ecc_init(&key);
    if (ret != 0)
    {
        throw std::runtime_error("Failed to initialize ECC key");
    }

    // Convert to DER
    if (isPrivate)
    {
        ret = wc_KeyPemToDer(reinterpret_cast<const unsigned char *>(keyPem), strlen(keyPem), derBuff, MAX_DER_BUFF_SIZE, nullptr);
    }
    else
    {
        ret = wc_PubKeyPemToDer(reinterpret_cast<const unsigned char *>(keyPem), strlen(keyPem), derBuff, MAX_DER_BUFF_SIZE);
    }

    if (ret < 0)
    {
        throw std::runtime_error("Failed to convert PEM key to DER, ret code: " + std::to_string(ret));
    }

    if (isPrivate)
    {
        // Load the key
        ret = wc_EccPrivateKeyDecode(derBuff, &idx, &key, ret);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to decode private key from DER, ret code: " + std::to_string(ret));
        }
    }
    else
    {
        ret = wc_EccPublicKeyDecode(derBuff, &idx, &key, ret);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to decode public key from DER, ret code: " + std::to_string(ret));
        }
    }
    return key;
}