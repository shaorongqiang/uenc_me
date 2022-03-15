#ifndef UENC_SOCKET_SOCKET_API_H_
#define UENC_SOCKET_SOCKET_API_H_

#include "common/version.h"
#include "define.h"
#include "proto/common.pb.h"
#include "protobuf_process.h"
#include "socket_manager.h"
#include "utils/singleton.hpp"
#include <google/protobuf/message.h>

int SocketInit();
void SocketDestory();

int Bytes2Proto(const std::string &bytes, Priority &priority, std::shared_ptr<google::protobuf::Message> &out_msg);

void Proto2Bytes(const std::string &msg_byte, const std::string &type, Priority priority, Compress compress, Encrypt encrypt, std::string &out_bytes);

int WriteMessage(std::shared_ptr<SocketConnection> connection, const std::string &msg_byte, const std::string &type, Priority priority, Compress compress = Compress::kCompress_True, Encrypt encrypt = Encrypt::kEncrypt_Unencrypted);

template <typename T>
int WriteMessage(std::shared_ptr<SocketConnection> connection, T msg, Priority priority, Compress compress = Compress::kCompress_True, Encrypt encrypt = Encrypt::kEncrypt_Unencrypted)
{
    return WriteMessage(connection, msg.SerializeAsString(), msg.GetDescriptor()->name(), priority, compress, encrypt);
}

template <typename T>
void RegisterCallback(std::function<int(const std::shared_ptr<T> &msg, std::shared_ptr<SocketConnection> connection)> cb)
{
    Singleton<ProtobufProcess>::instance()->RegisterCallback<T>(cb);
}

#endif
