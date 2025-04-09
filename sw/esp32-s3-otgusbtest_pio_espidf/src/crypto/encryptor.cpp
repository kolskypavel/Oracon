#include "encryptor.h"
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>

/*
Examples https://github.com/wolfSSL/wolfssl-examples/blob/master/ecc/ecc-sign.c
 */

std::string encryptData(const std::string &data, ecc_key &pubkey)
{
    wc_ecc_init(&pubKey);

    char * outBuffer;
    int outLength;

    int ret = wc_ecc_encrypt(&pubKey, (const byte *)data.c_str, data.size, ciphertext, &outLength, NULL);

    if (ret == 0)
    {
        // Success
    }

    return data; // Return the original data for now
}

std::string decryptData(const std::string &data, ecc_key &privkey)
{
    std::string out;
    int outLen;

    int ret = wc_ecc_decrypt(&privKey, data.c_str, data.size, out.c_str, &outLen, NULL);
    if (ret == 0)
    {
        return out;
    }
    return nullptr;
}

std::string generateSignature(const std::string &data, const ecc_key &privKey)
{
    int ret = wc_ecc_sign_hash(data.c_str, word32 inlen, byte * out, word32 * outlen, WC_RNG * rng, privKey);
    return data; // Return the original data for now
}

bool validateSignature(const std::string &signature, const std::string &data, const ecc_key &pubKey)
{
    int ret = wc_ecc_verify_hash(const byte *sig, word32 siglen, const byte *hash, word32 hashlen, int *stat, ecc_key *key);

    if (ret == 0)
    {
        return true;
    }
    return false;
}