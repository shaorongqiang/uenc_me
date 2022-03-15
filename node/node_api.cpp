#include "node/node_api.h"
#include "common/config.h"
#include "node/msg_process.h"
#include "socket/socket_api.h"
#include "utils/singleton.hpp"
#include <random>
#include <thread>

static void RegisterCallback()
{
    RegisterCallback<RegisterNodeReq>(HandlerRegisterNodeReq);
    RegisterCallback<RegisterNodeAck>(HandlerRegisterNodeAck);
    RegisterCallback<SyncNodeReq>(HandlerSyncNodeReq);
    RegisterCallback<SyncNodeAck>(HandlerSyncNodeAck);
    RegisterCallback<ConnectNodeReq>(HandlerConnectNodeReq);
    RegisterCallback<TransMsgReq>(HandlerTransMsgReq);
    RegisterCallback<BroadcaseMsgReq>(HandlerBroadcaseMsgReq);
    RegisterCallback<PingReq>(HandlerPingReq);
    RegisterCallback<PongReq>(HandlerPongReq);
    RegisterCallback<EchoReq>(HandlerEchoReq);
    RegisterCallback<EchoAck>(HandlerEchoAck);
    RegisterCallback<UpdateFeeReq>(HandlerUpdateFeeReq);
    RegisterCallback<UpdatePackageFeeReq>(HandlerUpdatePackageFeeReq);
    RegisterCallback<NodeHeightChangedReq>(HandlerNodeHeightChangedReq);
}

int NodeInit()
{
    RegisterCallback();
    auto peer_node = Singleton<PeerNode>::instance();
    auto ret = peer_node->InitSelfNode();
    if (0 != ret)
    {
        return ret - 10;
    }
    peer_node->ThreadStart();
    return 0;
}
void NodeDestroy()
{
    Singleton<PeerNode>::instance()->ThreadStop();
}

void Register2PublicNode()
{
    std::vector<PublicNode> public_nodes;
    if (!Singleton<Config>::instance()->public_node_list(public_nodes))
    {
        return;
    }
    while (!public_nodes.empty())
    {
        int index = rand() % public_nodes.size();
        PublicNode node = public_nodes.at(index);
        public_nodes.erase(public_nodes.begin() + index);
        if (0 == SendRegisterNodeReq(node.ip, node.port))
        {
            return;
        }
    }
}

void SendMessageToNode(const std::string &base58addr, const std::string &bytes_msg, const std::string &type, Priority priority, Compress compress, Encrypt encrypt)
{
    std::string msg;
    Proto2Bytes(bytes_msg, type, priority, compress, encrypt, msg);
    Node node;
    if (Singleton<PeerNode>::instance()->FindNodeByBase58Addr(base58addr, node) && node.is_connected())
    {
        node.connection->WriteMsg(msg);
    }
    else
    {
        SendTransMsgReq(base58addr, msg, priority, compress, encrypt);
    }
}
