#ifndef UENC_NODE_PEER_NODE_H_
#define UENC_NODE_PEER_NODE_H_

#include "socket/socket_api.h"
#include <event.h>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// 距离上次传送数据多少时间未收到新报文判断为开始检测，
#define HEART_TIME 60
// 检测开始每多少时间发送心跳包，
#define HEART_INTVL 100
// 发送几次心跳包对方未响应则close连接，
#define HEART_PROBES 6

struct Node
{
    std::string pub;
    std::string sign;
    std::string base58addr;
    std::string public_base58addr;
    std::string version;
    uint32_t local_ip;
    uint16_t listen_port;
    uint32_t public_ip;
    uint16_t public_port;
    uint64_t height;
    uint64_t sign_fee;
    uint64_t package_fee;
    uint32_t heart_time;
    uint32_t heart_probes;
    bool is_public_node;
    std::shared_ptr<SocketConnection> connection;
    Node()
    {
        Clear();
    }
    void Clear()
    {
        std::string().swap(pub);
        std::string().swap(sign);
        std::string().swap(base58addr);
        std::string().swap(public_base58addr);
        std::string().swap(version);
        is_public_node = false;
        listen_port = 0;
        public_ip = 0;
        public_port = 0;
        height = 0;
        sign_fee = 0;
        package_fee = 0;
        ResetHeart();
    }
    bool is_connected() const
    {
        return nullptr != connection && connection->IsConnected();
    }
    void ResetHeart()
    {
        heart_time = HEART_TIME;
        heart_probes = HEART_PROBES;
    }
    bool operator==(const Node &node) const
    {
        return base58addr == node.base58addr;
    }
    bool operator>(const Node &node) const
    {
        return base58addr > node.base58addr;
    }
};

class PeerNode
{
public:
    PeerNode() = default;
    ~PeerNode() = default;
    PeerNode(PeerNode &&) = delete;
    PeerNode(const PeerNode &) = delete;
    PeerNode &operator=(PeerNode &&) = delete;
    PeerNode &operator=(const PeerNode &) = delete;

    void ThreadStart();
    void ThreadWork();
    void ThreadStop();

    Node self_node();
    int InitSelfNode();
    void SetSelfNodePublicBase58Addr(const std::string &base58addr);
    void SetSelfNodePublicIp(uint32_t public_ip);
    void SetSelfNodePublicPort(uint16_t public_port);
    void SetSelfNodeFee(uint64_t fee);
    void SetSelfNodePackageFee(uint64_t package_fee);

    bool FindNodeByBase58Addr(const std::string &base58addr, Node &out_node);
    bool GetAllNodes(std::vector<Node> &out_nodes);
    bool GetAllPublicNodes(std::vector<Node> &out_nodes);
    bool GetNodesByPublicBase58Addr(const std::string &base58addr, std::vector<Node> &out_nodes);
    bool AddNode(const Node &node);
    void DeleteNodeByBase58Addr(const std::string &base58addr);
    bool UpdateNode(const Node &node);
    bool UpdateNodeConnect(const std::string &base58addr, std::shared_ptr<SocketConnection> connection);
    bool UpdateNodeHeight(const std::string &base58addr, uint64_t height);
    bool UpdateNodeSignFee(const std::string &base58addr, uint64_t sign_fee);
    bool UpdateNodePackageFee(const std::string &base58addr, uint64_t package_fee);
    void UpdateNodesByPublicBase58Addr(const std::string &public_base58addr, const std::vector<Node> &nodes);
    void ConnectPublicList();

private:

    std::thread thread_;
    bool continue_runing_;
    std::mutex sync_node_mutex_;
    std::condition_variable sync_node_condition_;

    std::mutex nodes_mutex_;
    std::unordered_map<std::string, Node> all_node_map_;
    std::set<std::string> pub_node_;

    std::mutex self_node_mutex_;
    Node self_node_;
};

#endif
