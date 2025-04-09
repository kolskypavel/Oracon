#include <string>

#define HAVE_ECC
#define HAVE_ECC_ENCRYPT

std::string encryptData(const std::string &data, const ecc_key &key);

std::string decryptData(const std::string &data, const ecc_key &key);

std::string generateSignature(const std::string &data, const ecc_key &key);

bool validateSignature(const std::string &signature, const std::string &data, const ecc_key &key);