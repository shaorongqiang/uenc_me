#include <arpa/inet.h>
#include <cryptopp/files.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/oids.h>
#include <cryptopp/osrng.h>
#include <cryptopp/ripemd.h>
#include <libbase58.h>
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "common/logging.h"
#include "utils/crypto_utils.h"
#include <cryptopp/adler32.h>
#include <cryptopp/base64.h>
#include <cryptopp/md5.h>
#include <cryptopp/zlib.h>
#include <zlib.h>

bool Bytes2Hex(const std::string &bytes, std::string &hex, bool to_uppercase)
{
    hex.clear();
    CryptoPP::HexEncoder hex_encoder(nullptr, to_uppercase);
    hex_encoder.Attach(new CryptoPP::StringSink(hex));
    hex_encoder.Put((const CryptoPP::byte *)bytes.data(), bytes.length());
    hex_encoder.MessageEnd();
    return true;
}

bool Hex2Bytes(const std::string &hex, std::string &bytes)
{
    bytes.clear();
    CryptoPP::HexDecoder hex_decoder;
    hex_decoder.Attach(new CryptoPP::StringSink(bytes));
    hex_decoder.Put((const CryptoPP::byte *)hex.data(), hex.length());
    hex_decoder.MessageEnd();
    return true;
}

bool Base58Encode(const std::string &bytes, std::string &base58)
{
    base58.clear();
    size_t b58sz = bytes.length();
    b58sz = b58sz * 138 / 100 + 1;
    char * b58 = new char[b58sz]{0};
    bool flag = b58enc(b58, &b58sz, bytes.data(), bytes.length());
    if (flag)
    {
        base58 = std::string(b58, strlen(b58));
    }
    delete [] b58;
    return flag;
}

bool Base58Decode(const std::string &base58, std::string &bytes)
{
    bytes.clear();
    size_t binsz = base58.length();
    char * bin = new char[binsz] {0};
    bool flag = b58tobin(bin, &binsz, base58.data(), base58.length());
    if (flag)
    {
        bytes = std::string(bin + sizeof(bin) - binsz, binsz);
    }
    delete [] bin;
    return flag;
}

bool Base64Encode(const std::string &bytes, std::string &base64)
{
    base64.clear();
    CryptoPP::Base64Encoder base64_encode(nullptr, false);
    base64_encode.Attach(new CryptoPP::StringSink(base64));
    base64_encode.Put((const CryptoPP::byte *)bytes.data(), bytes.length());
    base64_encode.MessageEnd();
    return true;
}

bool Base64Decode(const std::string &base64, std::string &bytes)
{
    bytes.clear();
    CryptoPP::Base64Decoder base64_decoder;
    base64_decoder.Attach(new CryptoPP::StringSink(bytes));
    base64_decoder.Put((const CryptoPP::byte *)base64.data(), base64.length());
    base64_decoder.MessageEnd();
    return true;
}


bool GetMd5Hash(const std::string &bytes, std::string &hash, bool to_uppercase)
{
    hash.clear();
    CryptoPP::Weak::MD5 md5;
    CryptoPP::HashFilter hashfilter(md5);
    hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), to_uppercase));
    hashfilter.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
    hashfilter.MessageEnd();
    return true;
}

bool GetSha1Hash(const std::string &bytes, std::string &hash, bool to_uppercase)
{
    hash.clear();
    CryptoPP::SHA1 sha1;
    CryptoPP::HashFilter hashfilter(sha1);
    hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), to_uppercase));
    hashfilter.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
    hashfilter.MessageEnd();
    return true;
}

bool GetSha256Hash(const std::string &bytes, std::string &hash, bool to_uppercase)
{
    hash.clear();
    CryptoPP::SHA256 sha256;
    CryptoPP::HashFilter hashfilter(sha256);
    hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), to_uppercase));
    hashfilter.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
    hashfilter.MessageEnd();
    return true;
}

bool GetRipemd160Hash(const std::string &bytes, std::string &hash, bool to_uppercase)
{
    hash.clear();
    CryptoPP::RIPEMD160 ripemd160;
    CryptoPP::HashFilter hashfilter(ripemd160);
    hashfilter.Attach(new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash), to_uppercase));
    hashfilter.Put(reinterpret_cast<const uint8_t *>(bytes.c_str()), bytes.length());
    hashfilter.MessageEnd();
    return true;
}
