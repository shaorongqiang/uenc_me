#ifndef UENC_ACCOUNT_ACCOUNT_H_
#define UENC_ACCOUNT_ACCOUNT_H_

#include <cryptopp/eccrypto.h>
#include <cryptopp/sha.h>

namespace Account
{
class PublicKey;
class PrivateKey
{
public:
    int GenerateNewKey();
    int LoadFromFileName(const std::string &filename);
    int LoadFromBytes(const std::string &bytes);
    void SaveToFile(const std::string &directory);

    bool GenerateSign(const std::string &bytes, std::string &out_signature);
    void Decrypt(const std::string &bytes, std::string &out_crypt);

    bool IsValid() const;
    void Clear();

    const std::string &filename() { return filename_; }
    const std::string &bytes() { return bytes_; }
    const std::string &base64() { return base64_; }
    const std::string &hex() { return hex_; }

private:
    int Init();
    friend class PublicKey;
    std::string filename_;
    std::string bytes_;
    std::string base64_;
    std::string hex_;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::PrivateKey key_;
};

class PublicKey
{
public:
    int LoadFromPrivateKey(const PrivateKey &private_key);
    int LoadFromFileName(const std::string &filename);
    int LoadFromBytes(const std::string &bytes);
    void SaveToFile(const std::string &directory);

    std::string GetBase58addr();

    int VerifySign(const std::string &bytes, const std::string &signature);
    void Encrypt(const std::string &bytes, std::string &out_crypt);

    bool IsValid();
    void Clear();

    const std::string &filename() { return filename_; }
    const std::string &bytes() { return bytes_; }
    const std::string &base64() { return base64_; }
    const std::string &hex() { return hex_; }

private:
    bool Init();
    std::string filename_;
    std::string bytes_;
    std::string base64_;
    std::string hex_;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::PublicKey key_;
};

class AccountAddr
{
public:
    int GenerateAccount();
    void SaveAccount(const std::string &directory, const std::string &prefix = std::string(), const std::string &suffix = std::string());
    int LoadAccount(const std::string &private_key_filename);
    bool IsEmptry();
    void Clear();
    const std::string &base58addr() { return base58addr_; }
    PrivateKey &private_key() { return private_key_; }
    PublicKey &public_key() { return public_key_; }

private:
    std::string base58addr_;
    PrivateKey private_key_;
    PublicKey public_key_;
};

int CheckBase58Addr(const std::string &base58_addr);

}; // namespace Account

#endif
