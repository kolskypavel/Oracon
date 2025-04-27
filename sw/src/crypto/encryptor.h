#pragma once

#include <string>
#include <stdexcept>
#include <Arduino.h>

#include "wolfssl.h"
#include <wolfssl/wolfcrypt/signature.h>
#include <wolfssl/wolfcrypt/asn_public.h>
#include <wolfssl/wolfcrypt/rsa.h>
#include <wolfssl/wolfcrypt/aes.h>
#include "defines.h"

std::string addPKCS7Padding(const std::string &data);

void removePKCS7Padding(std::string &data);

void encryptDataRsa(const std::string &data, RsaKey &key, byte *out, word32 &outLength);

void decryptDataRsa(const byte *data, word32 dataLength, RsaKey &key, std::string &out);

void encryptDataAes(const std::string &data, Aes &key, byte *out, word32 &outLength);

void decryptDataAes(const byte *data, word32 dataLength, Aes &key, std::string &out);

void generateSignature(const std::string &data, const RsaKey &privKey, byte *signature, word32 &outLength);

bool verifySignature(const std::string &data, const RsaKey &key, const byte *signature, word32 sigLength);

// Loads given key in PEM format (MUST include ---BEGIN header)
RsaKey loadKey(const char *keyPem, bool isPrivate);

/*
 *  Following things modified in wolfssl
 *   #define WOLFCRYPT_ONLY
 *   #define WOLFSSL_PUB_PEM_TO_DER
 *   # logging function disabled
 */
