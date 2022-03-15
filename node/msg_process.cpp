#include "node/msg_process.h"
#include "account/account.h"
#include "common/logging.h"
#include "node/node_api.h"
#include "node/peer_node.h"
#include "utils/net_utils.h"

int SendRegisterNodeReq(std::string addr, uint16_t port)
{
    RegisterNodeReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    req.set_is_get_nodelist(self_node.is_public_node);
    NodeInfo *node = req.mutable_node();
    node->set_pub(self_node.pub);
    node->set_sign(self_node.sign);
    node->set_base58addr(self_node.base58addr);
    node->set_public_base58addr(self_node.public_base58addr);
    node->set_local_ip(self_node.local_ip);
    node->set_listen_port(self_node.listen_port);
    node->set_public_ip(self_node.public_ip);
    node->set_public_port(self_node.public_port);
    node->set_is_public_node(self_node.is_public_node);
    node->set_height(self_node.height);
    node->set_sign_fee(self_node.sign_fee);
    node->set_package_fee(self_node.package_fee);
    node->set_version(self_node.version);

    auto socket_manager = Singleton<SocketManager>::instance();
    if(nullptr != self_node.connection)
    {
        socket_manager->DisConnect(self_node.connection->connection_id());
        self_node.connection.reset();
    }
    self_node.connection = std::make_shared<SocketConnection>();
    auto ret = socket_manager->Connect(addr, port, self_node.connection);
    if (0 != ret)
    {
        return ret - 10000;
    }
    return WriteMessage(self_node.connection, req, Priority::kPriority_High_2);
}

void SendRegisterNodeAck(const std::shared_ptr<SocketConnection> &connection, bool get_subnode)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    std::vector<Node> nodelist;
    peer_node->GetAllPublicNodes(nodelist);
    if (get_subnode && self_node.is_public_node)
    {
        std::vector<Node> subnodelist;
        if (peer_node->GetNodesByPublicBase58Addr(self_node.base58addr, subnodelist))
        {
            nodelist.insert(nodelist.end(), subnodelist.begin(), subnodelist.end());
        }
    }
    nodelist.push_back(self_node);
    RegisterNodeAck ack;
    for (auto &node : nodelist)
    {
        NodeInfo *nodeinfo = ack.add_nodes();
        nodeinfo->set_pub(node.pub);
        nodeinfo->set_sign(node.sign);
        nodeinfo->set_base58addr(node.base58addr);
        nodeinfo->set_public_base58addr(node.public_base58addr);
        nodeinfo->set_local_ip(node.local_ip);
        nodeinfo->set_listen_port(node.listen_port);
        nodeinfo->set_public_ip(node.public_ip);
        nodeinfo->set_public_port(node.public_port);
        nodeinfo->set_is_public_node(node.is_public_node);
        nodeinfo->set_height(node.height);
        nodeinfo->set_sign_fee(node.sign_fee);
        nodeinfo->set_package_fee(node.package_fee);
        nodeinfo->set_version(node.version);
    }
    WriteMessage(connection, ack, Priority::kPriority_High_2);
}

void SendSyncNodeReq(const std::string &base58addr)
{
    SyncNodeReq req;
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    req.add_base58addrs(self_node.base58addr);
    std::vector<Node> nodelist;
    if (!peer_node->GetNodesByPublicBase58Addr(self_node.base58addr, nodelist))
    {
        return;
    }
    for (auto &node : nodelist)
    {
        NodeInfo *nodeinfo = req.add_nodes();
        nodeinfo->set_pub(node.pub);
        nodeinfo->set_sign(node.sign);
        nodeinfo->set_base58addr(node.base58addr);
        nodeinfo->set_public_base58addr(node.public_base58addr);
        nodeinfo->set_local_ip(node.local_ip);
        nodeinfo->set_listen_port(node.listen_port);
        nodeinfo->set_public_ip(node.public_ip);
        nodeinfo->set_public_port(node.public_port);
        nodeinfo->set_is_public_node(node.is_public_node);
        nodeinfo->set_height(node.height);
        nodeinfo->set_sign_fee(node.sign_fee);
        nodeinfo->set_package_fee(node.package_fee);
        nodeinfo->set_version(node.version);
    }
    SendMessageToNode(base58addr, req, Priority::kPriority_High_2);
}

void SendSyncNodeAck(const std::string &base58addr)
{
    SyncNodeAck ack;
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    ack.add_base58addrs(self_node.base58addr);
    std::vector<Node> nodelist;
    if (!peer_node->GetNodesByPublicBase58Addr(self_node.base58addr, nodelist))
    {
        return;
    }
    for (auto &node : nodelist)
    {
        NodeInfo *nodeinfo = ack.add_nodes();
        nodeinfo->set_pub(node.pub);
        nodeinfo->set_sign(node.sign);
        nodeinfo->set_base58addr(node.base58addr);
        nodeinfo->set_public_base58addr(node.public_base58addr);
        nodeinfo->set_local_ip(node.local_ip);
        nodeinfo->set_listen_port(node.listen_port);
        nodeinfo->set_public_ip(node.public_ip);
        nodeinfo->set_public_port(node.public_port);
        nodeinfo->set_is_public_node(node.is_public_node);
        nodeinfo->set_height(node.height);
        nodeinfo->set_sign_fee(node.sign_fee);
        nodeinfo->set_package_fee(node.package_fee);
        nodeinfo->set_version(node.version);
    }
    return SendMessageToNode(base58addr, ack, Priority::kPriority_High_2);
}

void SendConnectNodeReq(std::shared_ptr<SocketConnection> connection)
{
    ConnectNodeReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    NodeInfo *node = req.mutable_node();
    node->set_pub(self_node.pub);
    node->set_sign(self_node.sign);
    node->set_base58addr(self_node.base58addr);
    node->set_public_base58addr("");
    node->set_local_ip(self_node.local_ip);
    node->set_listen_port(self_node.listen_port);
    node->set_public_ip(0);
    node->set_public_port(0);
    node->set_is_public_node(self_node.is_public_node);
    node->set_height(self_node.height);
    node->set_sign_fee(self_node.sign_fee);
    node->set_package_fee(self_node.package_fee);
    node->set_version(g_version);
    WriteMessage(connection, req, Priority::kPriority_High_2);
}

void SendBroadcaseMsgReq(const std::string &msg, Priority priority)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();

    BroadcaseMsgReq req;
    NodeInfo *node_info = req.mutable_from();
    node_info->set_base58addr(self_node.base58addr);
    node_info->set_is_public_node(self_node.is_public_node);
    req.set_data(msg);
    req.set_priority((uint8_t)priority);
    std::string msg_data = req.SerializeAsString();
    std::string type = req.GetDescriptor()->name();

    std::string bytes;
    Proto2Bytes(req.SerializeAsString(), req.GetDescriptor()->name(), priority,
                Compress::kCompress_True, Encrypt::kEncrypt_Unencrypted, bytes);

    if (self_node.is_public_node)
    {
        std::vector<Node> nodelist;
        peer_node->GetAllPublicNodes(nodelist);
        std::vector<Node> subnodelist;
        if (peer_node->GetNodesByPublicBase58Addr(self_node.public_base58addr, subnodelist))
        {
            nodelist.insert(nodelist.end(), subnodelist.begin(), subnodelist.end());
        }
        for (auto &node : nodelist)
        {
            if (nullptr != node.connection)
            {
                continue;
            }
            if (node.connection->IsConnected())
            {
                continue;
            }
            node.connection->WriteMsg(bytes);
        }
    }
    else
    {
        Node node;
        if (!peer_node->FindNodeByBase58Addr(self_node.public_base58addr, node))
        {
            return;
        }
        WriteMessage(node.connection, req, priority);
    }
}

void SendTransMsgReq(const std::string &dest_base58addr, const std::string &bytes_msg, Priority priority, Compress compress, Encrypt encrypt)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    TransMsgReq req;
    req.set_data(bytes_msg);
    req.set_priority((uint8_t)priority);
    NodeInfo *destnode = req.mutable_dest();
    destnode->set_base58addr(dest_base58addr);
    Node node;
    if (self_node.is_public_node)
    {
        if (!peer_node->FindNodeByBase58Addr(dest_base58addr, node))
        {
            return;
        }
        if (node.is_public_node)
        {
            node.connection->WriteMsg(bytes_msg);
        }
        else
        {
            if (!peer_node->FindNodeByBase58Addr(node.public_base58addr, node))
            {
                return;
            }
            WriteMessage(node.connection, req, priority);
        }
    }
    else
    {
        if (!peer_node->FindNodeByBase58Addr(self_node.public_base58addr, node))
        {
            return;
        }
        WriteMessage(node.connection, req, priority);
    }
}

void SendPingReq(const std::string &base58addr)
{
    PingReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    req.set_base58addr(self_node.base58addr);
    return SendMessageToNode(base58addr, req, Priority::kPriority_High_2);
}

void SendPongReq(const std::string &base58addr)
{
    PongReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    req.set_base58addr(self_node.base58addr);
    req.set_height(self_node.height);
    return SendMessageToNode(base58addr, req, Priority::kPriority_High_2);
}

void SendEchoReq(const std::string &base58addr)
{
    EchoReq req;
    req.set_base58addr(Singleton<PeerNode>::instance()->self_node().base58addr);
    return SendMessageToNode(base58addr, req, Priority::kPriority_High_2);
}

void SendEchoAck(const std::string &base58addr)
{
    EchoAck ack;
    ack.set_base58addr(Singleton<PeerNode>::instance()->self_node().base58addr);
    return SendMessageToNode(base58addr, ack, Priority::kPriority_High_2);
}

void SendUpdateFeeReq()
{
    UpdateFeeReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    req.set_base58addr(self_node.base58addr);
    req.set_fee(self_node.sign_fee);
    SendBroadcaseMsgReq(req.SerializeAsString(), Priority::kPriority_Low_0);
}

void SendUpdatePackageFeeReq()
{
    UpdatePackageFeeReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    req.set_base58addr(self_node.base58addr);
    req.set_package_fee(self_node.package_fee);
    SendBroadcaseMsgReq(req.SerializeAsString(), Priority::kPriority_Low_0);
}

void SendNodeHeightChangedReq()
{
    NodeHeightChangedReq req;
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    req.set_base58addr(self_node.base58addr);
    req.set_height(self_node.height);
    SendBroadcaseMsgReq(req.SerializeAsString(), Priority::kPriority_High_2);
}

int HandlerRegisterNodeReq(const std::shared_ptr<RegisterNodeReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    auto nodeinfo = msg->node();
    if (nodeinfo.base58addr() == self_node.base58addr)
    {
        return -1;
    }
    Account::PublicKey key;
    auto ret = key.LoadFromBytes(nodeinfo.pub());
    if (ret < 0)
    {
        return ret - 100;
    }
    if (key.GetBase58addr() != nodeinfo.base58addr())
    {
        return -2;
    }
    ret = key.VerifySign(nodeinfo.base58addr(), nodeinfo.sign());
    if (ret < 0)
    {
        return ret - 200;
    }
    Node register_node;
    register_node.pub = nodeinfo.pub();
    register_node.sign = nodeinfo.sign();
    register_node.base58addr = nodeinfo.base58addr();
    register_node.public_base58addr = self_node.public_base58addr;
    register_node.local_ip = nodeinfo.local_ip();
    register_node.listen_port = nodeinfo.listen_port();
    register_node.public_ip = 0;
    register_node.public_port = 0;
    if(nullptr != connection && connection->IsConnected())
    {
        GetIpv4AndPortByFd(connection->fd(), register_node.public_ip, register_node.public_port);
    }
    register_node.height = nodeinfo.height();
    register_node.sign_fee = nodeinfo.sign_fee();
    register_node.package_fee = nodeinfo.package_fee();
    register_node.is_public_node = nodeinfo.is_public_node();
    register_node.version = nodeinfo.version();
    register_node.connection = connection;
    if (!peer_node->AddNode(register_node))
    {
        return -3;
    }
    SendRegisterNodeAck(connection, self_node.is_public_node);
    return 0;
}

int HandlerRegisterNodeAck(const std::shared_ptr<RegisterNodeAck> &msg, std::shared_ptr<SocketConnection> connection)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    Node node;
    std::string public_base58addr;
    for (auto &nodeinfo : msg->nodes())
    {
        if (nodeinfo.base58addr() == self_node.base58addr)
        {
            peer_node->SetSelfNodePublicBase58Addr(nodeinfo.public_base58addr());
            peer_node->SetSelfNodePublicIp(nodeinfo.public_ip());
            peer_node->SetSelfNodePublicPort(nodeinfo.public_port());
            public_base58addr = nodeinfo.public_base58addr();
            break;
        }
    }
    for (auto &nodeinfo : msg->nodes())
    {
        if (nodeinfo.base58addr() == self_node.base58addr)
        {
            continue;
        }
        node.Clear();
        node.pub = nodeinfo.pub();
        node.sign = nodeinfo.sign();
        node.base58addr = nodeinfo.base58addr();
        node.public_base58addr = nodeinfo.public_base58addr();
        node.local_ip = nodeinfo.local_ip();
        node.listen_port = nodeinfo.listen_port();
        node.public_ip = nodeinfo.public_ip();
        node.public_port = nodeinfo.public_port();
        node.height = nodeinfo.height();
        node.sign_fee = nodeinfo.sign_fee();
        node.package_fee = nodeinfo.package_fee();
        node.is_public_node = nodeinfo.is_public_node();
        node.version = nodeinfo.version();
        if (nodeinfo.base58addr() == public_base58addr)
        {
            node.connection = connection;
        }
        if (!peer_node->AddNode(node))
        {
            peer_node->UpdateNode(node);
        }
    }
    peer_node->ConnectPublicList();
    return 0;
}

int HandlerSyncNodeReq(const std::shared_ptr<SyncNodeReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    Node node;
    std::vector<Node> nodes;
    if (msg->base58addrs_size() <= 0)
    {
        return -1;
    }
    auto base58addr = msg->base58addrs(0);
    for (auto &nodeinfo : msg->nodes())
    {
        if (nodeinfo.base58addr() != self_node.base58addr)
        {
            continue;
        }
        node.Clear();
        node.pub = nodeinfo.pub();
        node.sign = nodeinfo.sign();
        node.base58addr = nodeinfo.base58addr();
        node.public_base58addr = nodeinfo.public_base58addr();
        node.local_ip = nodeinfo.local_ip();
        node.listen_port = nodeinfo.listen_port();
        node.public_ip = nodeinfo.public_ip();
        node.public_port = nodeinfo.public_port();
        node.height = nodeinfo.height();
        node.sign_fee = nodeinfo.sign_fee();
        node.package_fee = nodeinfo.package_fee();
        node.is_public_node = nodeinfo.is_public_node();
        node.version = nodeinfo.version();
        nodes.push_back(node);
    }
    peer_node->UpdateNodesByPublicBase58Addr(base58addr, nodes);
    SendSyncNodeAck(base58addr);
    return 0;
}

int HandlerSyncNodeAck(const std::shared_ptr<SyncNodeAck> &msg, std::shared_ptr<SocketConnection> connection)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    Node node;
    std::vector<Node> nodes;
    if (msg->base58addrs_size() <= 0)
    {
        return -1;
    }
    auto base58addr = msg->base58addrs(0);
    for (auto &nodeinfo : msg->nodes())
    {
        if (nodeinfo.base58addr() != self_node.base58addr)
        {
            continue;
        }
        node.Clear();
        node.pub = nodeinfo.pub();
        node.sign = nodeinfo.sign();
        node.base58addr = nodeinfo.base58addr();
        node.public_base58addr = nodeinfo.public_base58addr();
        node.local_ip = nodeinfo.local_ip();
        node.listen_port = nodeinfo.listen_port();
        node.public_ip = nodeinfo.public_ip();
        node.public_port = nodeinfo.public_port();
        node.height = nodeinfo.height();
        node.sign_fee = nodeinfo.sign_fee();
        node.package_fee = nodeinfo.package_fee();
        node.is_public_node = nodeinfo.is_public_node();
        node.version = nodeinfo.version();
        nodes.push_back(node);
    }
    peer_node->UpdateNodesByPublicBase58Addr(base58addr, nodes);
    return 0;
}

int HandlerConnectNodeReq(const std::shared_ptr<ConnectNodeReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    auto &nodeinfo = msg->node();
    if (nodeinfo.base58addr() == self_node.base58addr)
    {
        return -1;
    }
    Node node;
    if (peer_node->FindNodeByBase58Addr(nodeinfo.base58addr(), node))
    {
        if (node.is_connected())
        {
            return -2;
        }
        else
        {
            peer_node->DeleteNodeByBase58Addr(nodeinfo.base58addr());
        }
    }
    node.Clear();
    node.pub = nodeinfo.pub();
    node.sign = nodeinfo.sign();
    node.base58addr = nodeinfo.base58addr();
    node.public_base58addr.clear();
    node.local_ip = nodeinfo.local_ip();
    node.listen_port = nodeinfo.listen_port();
    node.public_ip = 0;
    node.public_port = 0;
    if(nullptr != connection && connection->IsConnected())
    {
        GetIpv4AndPortByFd(connection->fd(), node.public_ip, node.public_port);
    }
    node.height = nodeinfo.height();
    node.sign_fee = nodeinfo.sign_fee();
    node.package_fee = nodeinfo.package_fee();
    node.is_public_node = nodeinfo.is_public_node();
    node.version = nodeinfo.version();
    node.connection = connection;
    if(!peer_node->AddNode(node))
    {
        peer_node->UpdateNodeConnect(node.base58addr, connection);
    }
    return 0;
}

int HandlerTransMsgReq(const std::shared_ptr<TransMsgReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    Node self_node = Singleton<PeerNode>::instance()->self_node();
    auto &nodeinfo = msg->dest();
    const std::string &msg_bytes = msg->data();
    const std::string &dest_node_base58addr = nodeinfo.base58addr();
    if (dest_node_base58addr == self_node.base58addr)
    {
        MsgData msg_data;
        Bytes2Proto(msg_bytes, msg_data.priority, msg_data.msg);
        Singleton<ProtobufProcess>::instance()->Handle(msg_data);
        return 0;
    }
    Priority priority = (Priority)(msg->priority() & 0xE);
    SendMessageToNode(dest_node_base58addr, msg_bytes, std::string(), priority);
    return 0;
}

int HandlerBroadcaseMsgReq(const std::shared_ptr<BroadcaseMsgReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    auto peer_node = Singleton<PeerNode>::instance();
    Node self_node = peer_node->self_node();
    MsgData msg_data;
    Bytes2Proto(msg->data(), msg_data.priority, msg_data.msg);
    auto ret = Singleton<ProtobufProcess>::instance()->Handle(msg_data);
    if(ret < 0)
    {
        return ret;
    }
    std::vector<Node> nodelist;
    if (self_node.is_public_node)
    {
        peer_node->GetNodesByPublicBase58Addr(self_node.base58addr, nodelist);
        if (!msg->from().is_public_node())
        {
            std::vector<Node> pubnodelist;
            peer_node->GetAllPublicNodes(pubnodelist);
            nodelist.insert(nodelist.end(), pubnodelist.begin(), pubnodelist.end());
        }
    }
    return 0;
}

int HandlerPingReq(const std::shared_ptr<PingReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    SendPongReq(msg->base58addr());
    return 0;
}

int HandlerPongReq(const std::shared_ptr<PongReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    Singleton<PeerNode>::instance()->UpdateNodeHeight(msg->base58addr(), msg->height());
    return 0;
}

int HandlerEchoReq(const std::shared_ptr<EchoReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    SendEchoAck(msg->base58addr());
    return 0;
}

int HandlerEchoAck(const std::shared_ptr<EchoAck> &msg, std::shared_ptr<SocketConnection> connection)
{
    std::cout << msg->base58addr() << std::endl;
    return 0;
}

int HandlerUpdateFeeReq(const std::shared_ptr<UpdateFeeReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    Singleton<PeerNode>::instance()->UpdateNodeSignFee(msg->base58addr(), msg->fee());
    return 0;
}

int HandlerUpdatePackageFeeReq(const std::shared_ptr<UpdatePackageFeeReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    Singleton<PeerNode>::instance()->UpdateNodePackageFee(msg->base58addr(), msg->package_fee());
    return 0;
}

int HandlerNodeHeightChangedReq(const std::shared_ptr<NodeHeightChangedReq> &msg, std::shared_ptr<SocketConnection> connection)
{
    Singleton<PeerNode>::instance()->UpdateNodeHeight(msg->base58addr(), msg->height());
    return 0;
}
