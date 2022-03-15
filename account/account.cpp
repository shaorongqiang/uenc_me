#include "account/account.h"
#include "common/logging.h"
#include "utils/crypto_utils.h"
#include <cryptopp/base64.h>
#include <cryptopp/files.h>
#include <cryptopp/hex.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/ripemd.h>
#include <filesystem>
#include <libbase58.h>

int Account::PrivateKey::GenerateNewKey()
{
    CryptoPP::AutoSeededRandomPool prng;
    key_.Initialize(prng, CryptoPP::ASN1::secp256r1());
    if (!IsValid())
    {
        return -1;
    }
    auto ret = Init();
    if (0 != ret)
    {
        return ret - 10;
    }
    return 0;
}

int Account::PrivateKey::LoadFromFileName(const std::string &filename)
{
    Clear();
    filename_ = filename;
    key_.Load(CryptoPP::FileSource(filename_.c_str(), true).Ref());
    if (!IsValid())
    {
        return -1;
    }

    auto ret = Init();
    if (0 != ret)
    {
        return ret - 10;
    }
    return 0;
}

int Account::PrivateKey::LoadFromBytes(const std::string &bytes)
{
    Clear();
    if (!Bytes2Hex(bytes, hex_))
    {
        return -1;
    }
    CryptoPP::HexDecoder decoder;
    decoder.Put((CryptoPP::byte *)hex_.data(), hex_.size());
    decoder.MessageEnd();
    CryptoPP::Integer x;
    x.Decode(decoder, decoder.MaxRetrievable());
    key_.Initialize(CryptoPP::ASN1::secp256r1(), x);
    if (!IsValid())
    {
        return -2;
    }

    auto ret = Init();
    if (0 != ret)
    {
        return ret - 10;
    }

    return 0;
}

void Account::PrivateKey::SaveToFile(const std::string &filename)
{
    key_.Save(CryptoPP::FileSink(filename.c_str(), true).Ref());
}

bool Account::PrivateKey::GenerateSign(const std::string &bytes, std::string &out_signature)
{
    out_signature.erase();
    try
    {
        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::StringSource sign(bytes, true,
                                    new CryptoPP::SignerFilter(prng, CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::Signer(key_),
                                                               new CryptoPP::StringSink(out_signature)));
        return !out_signature.empty();
    }
    catch (...)
    {
        return false;
    }
}

void Account::PrivateKey::Decrypt(const std::string &bytes, std::string &out_crypt)
{
    try
    {
        CryptoPP::ECIES<CryptoPP::ECP>::Decryptor Decryptor(key_);
        CryptoPP::AutoSeededRandomPool rng;
        size_t recovered_len = Decryptor.MaxPlaintextLength(bytes.length());
        char *recovered = new char[recovered_len]{0};
        Decryptor.Decrypt(rng, (CryptoPP::byte *)bytes.data(), bytes.length(), (CryptoPP::byte *)(recovered));
        out_crypt = std::string(recovered, recovered_len);
    }
    catch (...)
    {
        return;
    }
}

bool Account::PrivateKey::IsValid() const
{
    CryptoPP::AutoSeededRandomPool prng;
    return key_.Validate(prng, 3);
}

void Account::PrivateKey::Clear()
{
    std::string().swap(filename_);
    std::string().swap(bytes_);
    std::string().swap(base64_);
    std::string().swap(hex_);
    key_ = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::PrivateKey();
}

int Account::PrivateKey::Init()
{
    bytes_.clear();
    const CryptoPP::Integer &x = key_.GetPrivateExponent();
    for (int i = x.ByteCount() - 1; i >= 0; i--)
    {
        bytes_ += x.GetByte(i);
    }

    hex_.clear();
    if (!Bytes2Hex(bytes_, hex_))
    {
        return -1;
    }

    base64_.clear();
    if (!Base64Encode(bytes_, base64_))
    {
        return -2;
    }
    return 0;
}

int Account::PublicKey::LoadFromPrivateKey(const PrivateKey &private_key)
{
    Clear();
    if (!private_key.IsValid())
    {
        return -1;
    }
    private_key.key_.MakePublicKey(key_);
    if (!IsValid())
    {
        return -2;
    }
    auto ret = Init();
    if (0 != ret)
    {
        return ret - 10;
    }
    return 0;
}

int Account::PublicKey::LoadFromFileName(const std::string &filename)
{
    Clear();
    key_.Load(CryptoPP::FileSource(filename.c_str(), true).Ref());
    if (!IsValid())
    {
        return -1;
    }
    auto ret = Init();
    if (0 != ret)
    {
        return ret - 10;
    }
    return 0;
}

int Account::PublicKey::LoadFromBytes(const std::string &bytes)
{
    Clear();
    if (bytes.empty())
    {
        return -1;
    }
    try
    {
        char c1 = bytes.at(0);
        char c2 = bytes.at(1);

        hex_.clear();
        if (!Bytes2Hex(bytes, hex_))
        {
            return -1;
        }

        CryptoPP::HexDecoder hex_decoder;
        hex_decoder.Put((CryptoPP::byte *)hex_.c_str() + 4, hex_.size() - 4);
        hex_decoder.MessageEnd();
        CryptoPP::ECP::Point pt;
        size_t len = hex_decoder.MaxRetrievable();
        if ((size_t)(c1 + c2) != len)
        {
            return -2;
        }

        pt.identity = false;
        pt.x.Decode(hex_decoder, c1);
        pt.y.Decode(hex_decoder, c2);
        key_.Initialize(CryptoPP::ASN1::secp256r1(), pt);

        if (!IsValid())
        {
            return -3;
        }
    }
    catch (...)
    {
        return -4;
    }
    auto ret = Init();
    if (0 != ret)
    {
        return ret - 10;
    }
    return 0;
}
void Account::PublicKey::SaveToFile(const std::string &filename)
{
    key_.Save(CryptoPP::FileSink(filename.c_str(), true).Ref());
}

std::string Account::PublicKey::GetBase58addr()
{
    std::string bytes;
    CryptoPP::SHA256 sha256;
    CryptoPP::HashFilter hashfilter1(sha256);
    hashfilter1.Attach(new CryptoPP::StringSink(bytes));
    hashfilter1.Put(reinterpret_cast<const uint8_t *>(bytes_.c_str()), bytes_.length());
    hashfilter1.MessageEnd();

    std::string ripemd160_bytes;
    CryptoPP::RIPEMD160 ripemd160;
    CryptoPP::HashFilter hashfilter2(ripemd160);
    hashfilter2.Attach(new CryptoPP::StringSink(ripemd160_bytes));
    hashfilter2.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
    hashfilter2.MessageEnd();

    char b58c[2048] = {0};
    size_t b58c_sz = sizeof(b58c);
    uint8_t ver = 0x00;
    b58_sha256_impl = [](void *hash, const void *data, size_t datasz)
    {
        std::string bytes((char *)data, datasz);
        std::string sha256bytes;
        CryptoPP::SHA256 sha256;
        CryptoPP::HashFilter hashfilter1(sha256);
        hashfilter1.Attach(new CryptoPP::StringSink(sha256bytes));
        hashfilter1.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
        hashfilter1.MessageEnd();
        memcpy(hash, sha256bytes.data(), sha256bytes.size());
        return true;
    };
    if (!b58check_enc(b58c, &b58c_sz, ver, ripemd160_bytes.data(), ripemd160_bytes.length()))
    {
        return "";
    }
    return std::string(b58c, b58c_sz - 1);
}

int Account::PublicKey::VerifySign(const std::string &bytes, const std::string &signature)
{
    bool result = false;
    try
    {
        CryptoPP::StringSource(signature + bytes, true,
                               new CryptoPP::SignatureVerificationFilter(CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::Verifier(key_),
                                                                         new CryptoPP::ArraySink((CryptoPP::byte *)&result, sizeof(result))));
    }
    catch (...)
    {
        return -1;
    }
    if (!result)
    {
        return -2;
    }
    return 0;
}

void Account::PublicKey::Encrypt(const std::string &bytes, std::string &out_crypt)
{
    CryptoPP::ECIES<CryptoPP::ECP>::Encryptor Encryptor(key_);

    size_t length = bytes.length() + 1;
    size_t cipher_length = Encryptor.CiphertextLength(length);
    CryptoPP::byte *cipher = new CryptoPP::byte[cipher_length]{0};

    CryptoPP::AutoSeededRandomPool rng;
    Encryptor.Encrypt(rng, (const CryptoPP::byte *)(bytes.data()), length, cipher);
    out_crypt = std::string((char *)cipher, cipher_length);
    delete[] cipher;
    cipher = nullptr;
}

bool Account::PublicKey::IsValid()
{
    CryptoPP::AutoSeededRandomPool prng;
    return key_.Validate(prng, 3);
}

void Account::PublicKey::Clear()
{
    std::string().swap(filename_);
    std::string().swap(bytes_);
    std::string().swap(base64_);
    std::string().swap(hex_);
    key_ = CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA1>::PublicKey();
}

bool Account::PublicKey::Init()
{
    bytes_.clear();
    const CryptoPP::ECP::Point &q = key_.GetPublicElement();
    const CryptoPP::Integer &qx = q.x;
    const CryptoPP::Integer &qy = q.y;
    char c1 = qx.ByteCount();
    char c2 = qy.ByteCount();
    bytes_ += c1;
    bytes_ += c2;
    for (int i = qx.ByteCount() - 1; i >= 0; i--)
    {
        bytes_ += qx.GetByte(i);
    }
    for (int i = qy.ByteCount() - 1; i >= 0; i--)
    {
        bytes_ += qy.GetByte(i);
    }

    hex_.clear();
    if (!Bytes2Hex(bytes_, hex_))
    {
        return -1;
    }

    base64_.clear();
    if (!Base64Encode(bytes_, base64_))
    {
        return -2;
    }
    return 0;
}

int Account::AccountAddr::GenerateAccount()
{
    Clear();
    auto ret = private_key_.GenerateNewKey();
    if (ret < 0)
    {
        return ret - 100;
    }
    ret = public_key_.LoadFromPrivateKey(private_key_);
    if (ret < 0)
    {
        return ret - 200;
    }
    base58addr_ = public_key_.GetBase58addr();
    return 0;
}

void Account::AccountAddr::SaveAccount(const std::string &directory, const std::string &prefix, const std::string &suffix)
{
    std::string filename = directory + "/" + prefix + base58addr_ + suffix;
    private_key_.SaveToFile(filename + ".private.key");
    public_key_.SaveToFile(filename + ".public.key");
}

int Account::AccountAddr::LoadAccount(const std::string &private_key_filename)
{
    Clear();
    auto ret = private_key_.LoadFromFileName(private_key_filename);
    if (ret < 0)
    {
        return ret - 100;
    }

    ret = public_key_.LoadFromPrivateKey(private_key_);
    if (ret < 0)
    {
        return ret - 200;
    }
    base58addr_ = public_key_.GetBase58addr();
    return 0;
}

bool Account::AccountAddr::IsEmptry()
{
    return (!base58addr_.empty()) && private_key_.IsValid() && public_key_.IsValid();
}

void Account::AccountAddr::Clear()
{
    base58addr_.clear();
    private_key_.Clear();
    public_key_.Clear();
}

int Account::CheckBase58Addr(const std::string &base58_addr)
{
    if (base58_addr.length() != 33 && base58_addr.length() != 34)
    {
        return -1;
    }

    char decodebuf[1024] = {0};
    size_t decodebuflen = sizeof(decodebuf);
    if (!b58tobin(decodebuf, &decodebuflen, base58_addr.data(), base58_addr.length()))
    {
        return -2;
    }

    auto sha256_impl = [](void *hash, const void *data, size_t datasz)
    {
        std::string bytes((char *)data, datasz);
        std::string sha256bytes;
        CryptoPP::SHA256 sha256;
        CryptoPP::HashFilter hashfilter1(sha256);
        hashfilter1.Attach(new CryptoPP::StringSink(sha256bytes));
        hashfilter1.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
        hashfilter1.MessageEnd();
        memcpy(hash, sha256bytes.data(), sha256bytes.size());
        return true;
    };

    uint8_t hash[1024] = {0};
    uint8_t buf[0x20] = {0};
    if (!(sha256_impl(buf, decodebuf, decodebuflen - 4) && sha256_impl(hash, buf, sizeof(buf))))
    {
        return false;
    }
    if (0 != memcmp(((unsigned char *)decodebuf) + decodebuflen - 4, hash, 4))
    {
        return -4;
    }
    return 0;
}
