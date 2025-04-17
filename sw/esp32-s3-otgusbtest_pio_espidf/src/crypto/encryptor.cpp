#include "encryptor.h"
/*
Examples: https://github.com/wolfSSL/wolfssl-examples/blob/master/ecc/
Docs: https://www.wolfssl.com/documentation/manuals/wolfssl/ecc_8h.html
 */

void encryptData(const std::string &data, ecc_key &pubKey, byte *out, word32 &outLength)
{
    int ret = 0;
    WC_RNG rng;

    // Init rng
    ret = wc_InitRng(&rng);

    if (ret != 0)
    {
        throw std::invalid_argument("ENCRYPT: Failed to init RNG");
    }

    byte outBuffer[MAX_MESSAGE_SIZE];
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
        throw std::invalid_argument("ENCRYPT: Failed to make ephemeral key, ret code: " + std::to_string(ret));
    }

    //TODO: Padding

    ret = wc_ecc_encrypt(&ephemeralKey, &pubKey, reinterpret_cast<const byte *>(data.data()), data.size(), outBuffer, &outLength, nullptr);

    if (ret == 0)
    {
        // Success
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
        return;
    }
    throw std::invalid_argument("DECRYPT: Failed to decrypt data, ret code: " + std::to_string(ret));
}

void generateSignature(const std::string &data, const ecc_key &privKey, byte *signature, word32 & outLength)
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
    if (ret == 0)
    {
        return;
    }
    throw std::invalid_argument("SIGNATURE: Failed to generate signature ret code: " + std::to_string(ret));
}

bool verifySignature(const byte *signature, word32 sigLength, const std::string &data, const ecc_key &pubKey)
{
    int ret = wc_SignatureVerify(WC_HASH_TYPE_SHA256, WC_SIGNATURE_TYPE_ECC, reinterpret_cast<const byte *>(data.data()), data.size(), signature, sigLength, &pubKey, sizeof(pubKey));

    if (ret == 0)
    {
        return true;
    }
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