#pragma once

#include <string>
#include <stdexcept>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/ecc.h>
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include "defines.h"

bool encryptData(const std::string &data, ecc_key &key, byte *out, word32 &outLength);

bool decryptData(const byte *data, word32 dataLength, ecc_key &key, std::string &out);

bool generateSignature(const std::string &data, const ecc_key &privKey, byte *signature, word32 outLength);

bool validateSignature(const byte *signature, word32 sigLength, const std::string &data, const ecc_key &key);

ecc_key loadKey(const char *keyPem, bool isPrivate);

/*
 *  Following things modified in wolfssl
 *   #undef HAVE___UINT128_T
 *   #define HAVE_ECC_ENCRYPT
 *   #define HAVE_HKDF
 *   #define WOLFCRYPT_ONLY
 *   # logging function disabled
 */
