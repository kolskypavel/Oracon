#include "encryptor.h"
/*
Docs: https://www.wolfssl.com/documentation/manuals/wolfssl/
 */

std::string addPKCS7Padding(const std::string &data)
{
    std::string padded = data;
    size_t padLen = AES_BLOCK_SIZE - (data.size() % AES_BLOCK_SIZE);
    if (padLen == 0)
        padLen = AES_BLOCK_SIZE; // Full block padding if already aligned

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
    if (padLen == 0 || padLen > AES_BLOCK_SIZE)
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

void generateAesKey(uint8_t *out)
{
    WC_RNG rng;
    int ret = wc_InitRng(&rng);
    if (ret != 0)
    {
        throw std::runtime_error("Failed to initialize RNG");
    }

    ret = wc_RNG_GenerateBlock(&rng, out, AES_KEY_SIZE);
    wc_FreeRng(&rng);
    if (ret != 0)
    {
        throw std::runtime_error("Failed to generate AES key");
    }
    ESP_LOGI("AES", "Successfuly generated AES key");
}

void encryptDataRsa(const byte *data, word32 dataLength, RsaKey &key, byte *out, int &outLength)
{
    int ret = 0;
    WC_RNG rng;

    ret = wc_InitRng(&rng);
    if (ret != 0)
    {
        throw std::invalid_argument("RSA ENCRYPT: Failed to init RNG");
    }

    outLength = wc_RsaPublicEncrypt(data, dataLength, out, outLength, &key, &rng);
    if (outLength < 0)
    {
        throw std::invalid_argument("RSA encryption failed, ret code: " + std::to_string(ret));
    }
}

void encryptDataAes(const std::string &data, const byte *aesKey, byte *out, word32 &outLength)
{
    int ret = 0;
    WC_RNG rng;
    Aes aes;

    ret = wc_InitRng(&rng);
    if (ret != 0)
    {
        throw std::invalid_argument("AES ENCRYPT: Failed to init RNG");
    }

    byte iv[AES_BLOCK_SIZE];

    ret = wc_RNG_GenerateBlock(&rng, iv, AES_BLOCK_SIZE);
    if (ret != 0)
    {
        wc_FreeRng(&rng);
        throw std::invalid_argument("AES ENCRYPT: Failed to generate IV");
    }

    ret = wc_AesSetKey(&aes, aesKey, AES_BLOCK_SIZE, iv, AES_ENCRYPTION);
    if (ret != 0)
    {
        wc_FreeRng(&rng);
        throw std::invalid_argument("AES ENCRYPT: Failed to set IV");
    }
    // Add iv to the message
    memcpy(out, iv, AES_BLOCK_SIZE);

    std::string padded = addPKCS7Padding(data);

    outLength = padded.size();
    ret = wc_AesCbcEncrypt(&aes, out + AES_BLOCK_SIZE, reinterpret_cast<const byte *>(padded.data()), padded.size());
    wc_FreeRng(&rng);

    if (ret != 0)
    {
        throw std::invalid_argument("AES ENCRYPT: Failed to encrypt data, ret code: " + std::to_string(ret));
    }
    outLength += AES_BLOCK_SIZE; // Add the IV length
}

void decryptDataAes(const byte *data, word32 dataLength, const byte *aesKey, std::string &out)
{
    Aes aes;
    // Get first bytes as iv
    if (dataLength <= AES_BLOCK_SIZE)
    {
        throw std::invalid_argument("AES DECRYPT: Data length is too short to contain IV and ciphertext");
    }

    byte iv[AES_BLOCK_SIZE];
    memcpy(iv, data, AES_BLOCK_SIZE);

    int ret = wc_AesSetKey(&aes, aesKey, AES_BLOCK_SIZE, iv, AES_DECRYPTION);
    if (ret != 0)
    {
        throw std::invalid_argument("AES DECRYPT: Failed to set IV / key");
    }

    byte decrypted[MAX_MESSAGE_SIZE];
    word32 decryptedLength = dataLength - AES_BLOCK_SIZE;

    ret = wc_AesCbcDecrypt(&aes, decrypted, data + AES_BLOCK_SIZE, decryptedLength);
    if (ret != 0)
    {
        throw std::invalid_argument("AES DECRYPT: Failed to decrypt data, ret code: " + std::to_string(ret));
    }

    out = std::string(reinterpret_cast<char *>(decrypted), decryptedLength);
    removePKCS7Padding(out);
}

void generateSignature(const std::string &data, const RsaKey &privKey, byte *signature, word32 &outLength)
{
    int ret = 0;
    WC_RNG rng;

    // Init rng
    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        throw std::invalid_argument("SIGNATURE: Failed to init RNG, ret code: " + std::to_string(ret));
    }
    
    ret = wc_SignatureGenerate_ex(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC, reinterpret_cast<const byte *>(data.data()), data.size(), signature, &outLength, &privKey, sizeof(privKey), &rng, 0);
    wc_FreeRng(&rng);
    if (ret == 0)
    {
        return;
    }
    throw std::invalid_argument("SIGNATURE: Failed to generate signature ret code: " + std::to_string(ret));
}

bool verifySignature(const std::string &data, const RsaKey &key, const byte *signature, word32 sigLength)
{
    int ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_RSA_W_ENC, reinterpret_cast<const byte *>(data.data()), data.size(), signature, sigLength, &key, sizeof(key));

    if (ret == 0)
    {
        ESP_LOGI("SIGN", "Successfully verified signature");
        return true;
    }
    ESP_LOGE("SIGN", "Failed to verify signature, err: %d", ret);
    return false;
}

RsaKey *loadKey(const char *keyPem, bool isPrivate)
{
    int ret = 0;
    word32 idx = 0;

    // Allocate memory
    RsaKey *key = (RsaKey *)XMALLOC(sizeof(RsaKey), NULL, DYNAMIC_TYPE_RSA);

    if (!key)
    {
        throw std::runtime_error("Failed to allocate memory for RSA key");
    }

    byte derBuff[MAX_DER_BUFF_SIZE];

    // Initialize the RSA key structure
    ret = wc_InitRsaKey(key, nullptr);
    if (ret != 0)
    {
        throw std::runtime_error("Failed to initialize RSA key");
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
        // Load the private RSA key
        ret = wc_RsaPrivateKeyDecode(derBuff, &idx, key, ret);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to decode private RSA key from DER, ret code: " + std::to_string(ret));
        }
    }
    else
    {
        // Load the public RSA key
        ret = wc_RsaPublicKeyDecode(derBuff, &idx, key, ret);
        if (ret != 0)
        {
            throw std::runtime_error("Failed to decode public RSA key from DER, ret code: " + std::to_string(ret));
        }
    }
    return key;
}