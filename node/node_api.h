#ifndef UENC_NODE_NODE_API_H_
#define UENC_NODE_NODE_API_H_

#include "node/peer_node.h"

int NodeInit();
void NodeDestroy();

void Register2PublicNode();

void SendMessageToNode(const std::string &base58addr, const std::string &msg, const std::string &type, Priority priority,
                       Compress compress = Compress::kCompress_True, Encrypt encrypt = Encrypt::kEncrypt_Unencrypted);

template <typename T>
void SendMessageToNode(const std::string &base58addr, const T &msg, Priority priority,
                       Compress compress = Compress::kCompress_True, Encrypt encrypt = Encrypt::kEncrypt_Unencrypted)
{
    return SendMessageToNode(base58addr, msg.SerializeAsString(), msg.GetDescriptor()->name(), priority, compress, encrypt);
}

#endif
