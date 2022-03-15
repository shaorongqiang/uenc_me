#ifndef UENC_SOCKET_DEFINE_H_
#define UENC_SOCKET_DEFINE_H_

#include <string>

enum DataSource : uint8_t
{
    kNone = 0,
    kLocal,
    kUnixDomain,
    kNETV4,
    kNETV6,
};

enum Compress : uint8_t
{
    kCompress_False = 0,
    kCompress_True = 1
};

enum Encrypt : uint8_t
{
    kEncrypt_Unencrypted = 0,
    kEncrypt_SingleEncryption = 1,
    kEncrypt_TwoWay_Encryption = 2,
};

enum Priority : uint8_t
{
    kPriority_Low_0 = 0,
    kPriority_Low_1 = 2,
    kPriority_Low_2 = 4,

    kPriority_Middle_0 = 5,
    kPriority_Middle_1 = 8,
    kPriority_Middle_2 = 10,

    kPriority_High_0 = 11,
    kPriority_High_1 = 14,
    kPriority_High_2 = 15,
};

#endif
