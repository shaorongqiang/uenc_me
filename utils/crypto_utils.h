#ifndef UENC_UTILS_CRYPTO_UTILS_H__
#define UENC_UTILS_CRYPTO_UTILS_H__

#include <cryptopp/eccrypto.h>
#include <cryptopp/sha.h>
#include <string>

bool Bytes2Hex(const std::string &bytes, std::string &hex, bool to_uppercase = false);
bool Hex2Bytes(const std::string &hex, std::string &bytes);

bool Base58Encode(const std::string &bytes, std::string &base58);
bool Base58Decode(const std::string &base58, std::string &bytes);

bool Base64Encode(const std::string &bytes, std::string &base64);
bool Base64Decode(const std::string &base64, std::string &bytes);

bool GetMd5Hash(const std::string &bytes, std::string &hash, bool to_uppercase = false);
bool GetSha1Hash(const std::string &bytes, std::string &hash, bool to_uppercase = false);
bool GetSha256Hash(const std::string &bytes, std::string &hash, bool to_uppercase = false);
bool GetRipemd160Hash(const std::string &bytes, std::string &hash, bool to_uppercase = false);

#endif
