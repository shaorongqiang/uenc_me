#include "node/peer_node.h"
#include "account/account_manager.h"
#include "common/config.h"
#include "db/db_api.h"
#include "node/msg_process.h"
#include "node/node_api.h"
#include "utils/net_utils.h"

void PeerNode::ThreadStart()
{
    continue_runing_ = true;
    thread_ = std::thread(std::bind(&PeerNode::ThreadWork, this));
    thread_.detach();
}

void PeerNode::ThreadWork()
{
    uint32_t k_refresh_time = Singleton<Config>::instance()->k_refresh_time();
    std::vector<std::string> pub_node;
    while (continue_runing_)
    {
        {
            std::lock_guard<std::mutex> lck(nodes_mutex_);
            for (auto &node : pub_node_)
            {
                pub_node.push_back(node);
            }
        }
        if (pub_node_.empty())
        {
            Register2PublicNode();
        }
        else if (self_node_.is_public_node)
        {
            ConnectPublicList();
            for (auto &node : pub_node)
            {
                SendSyncNodeReq(node);
            }
        }
        pub_node.clear();
        std::unique_lock<std::mutex> locker(sync_node_mutex_);
        sync_node_condition_.wait_for(locker, std::chrono::seconds(k_refresh_time));
    }
}

void PeerNode::ThreadStop()
{
    continue_runing_ = false;
    sync_node_condition_.notify_all();
}

Node PeerNode::self_node()
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    return self_node_;
}

int PeerNode::InitSelfNode()
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    if (!self_node_.base58addr.empty())
    {
        return 0;
    }
    auto conf = Singleton<Config>::instance();
    auto acc_mgr = Singleton<AccountManager>::instance();
    self_node_.base58addr = acc_mgr->GetDefaultAccount();
    Account::AccountAddr account;
    if (!acc_mgr->GetAccountByAddr(self_node_.base58addr, account))
    {
        return -1;
    }
    self_node_.pub = account.public_key().bytes();
    if (!account.private_key().GenerateSign(self_node_.base58addr, self_node_.sign))
    {
        return -2;
    }
    self_node_.version = g_version;
    self_node_.listen_port = conf->listen_port();
    if (!Str2IntIPv4(conf->local_ip(), self_node_.local_ip))
    {
        return -3;
    }
    auto status = DBReader().GetNodeHeight(self_node_.height);
    if (DBStatus::DB_SUCCESS != status && DBStatus::DB_NOT_FOUND != status)
    {
        return -4;
    }
    self_node_.sign_fee = conf->sign_fee();
    self_node_.package_fee = conf->package_fee();
    self_node_.is_public_node = conf->is_public_node();
    self_node_.version = g_version;

    Singleton<SocketManager>::instance()->SetDisConnectCallBack(
        [this](const std::string &connection_id)
        {
            std::lock_guard<std::mutex> lck(nodes_mutex_);
            for (auto &item : all_node_map_)
            {
                if ((nullptr != item.second.connection) &&
                    (item.second.connection->connection_id() == connection_id))
                {
                    if (item.second.is_public_node)
                    {
                        pub_node_.erase(item.first);
                    }
                    all_node_map_.erase(item.first);
                    return;
                }
            }
        });
    return 0;
}

void PeerNode::SetSelfNodePublicBase58Addr(const std::string &base58addr)
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    self_node_.public_base58addr = base58addr;
}

void PeerNode::SetSelfNodePublicIp(uint32_t public_ip)
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    self_node_.public_ip = public_ip;
}

void PeerNode::SetSelfNodePublicPort(uint16_t public_port)
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    self_node_.public_port = public_port;
}

void PeerNode::SetSelfNodeFee(uint64_t sign_fee)
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    self_node_.sign_fee = sign_fee;
}
void PeerNode::SetSelfNodePackageFee(uint64_t package_fee)
{
    std::lock_guard<std::mutex> lock(self_node_mutex_);
    self_node_.package_fee = package_fee;
}

bool PeerNode::FindNodeByBase58Addr(const std::string &base58addr, Node &out_node)
{
    if (base58addr.empty())
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    if (all_node_map_.end() == all_node_map_.find(base58addr))
    {
        return false;
    }
    out_node = all_node_map_[base58addr];
    return true;
}

bool PeerNode::GetAllNodes(std::vector<Node> &out_nodes)
{
    out_nodes.clear();
    out_nodes.reserve(all_node_map_.size());
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    for (auto itr = all_node_map_.cbegin(); itr != all_node_map_.cend(); ++itr)
    {
        out_nodes.push_back(itr->second);
    }
    return !out_nodes.empty();
}

bool PeerNode::GetAllPublicNodes(std::vector<Node> &out_nodes)
{
    out_nodes.clear();
    out_nodes.reserve(pub_node_.size());
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    for (auto &addr : pub_node_)
    {
        auto it = all_node_map_.find(addr);
        if (all_node_map_.end() != it)
        {
            out_nodes.push_back(it->second);
        }
    }
    return !out_nodes.empty();
}

bool PeerNode::GetNodesByPublicBase58Addr(const std::string &base58addr, std::vector<Node> &out_nodes)
{
    if (base58addr.empty())
    {
        return false;
    }
    out_nodes.clear();
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    for (auto &item : all_node_map_)
    {
        if (item.second.public_base58addr == base58addr)
        {
            out_nodes.push_back(item.second);
        }
    }
    return !out_nodes.empty();
}

bool PeerNode::AddNode(const Node &node)
{
    if (node.base58addr.empty())
    {
        return false;
    }
    {
        std::lock_guard<std::mutex> lck(nodes_mutex_);
        if (all_node_map_.end() != all_node_map_.find(node.base58addr))
        {
            return false;
        }
        else
        {
            all_node_map_[node.base58addr] = node;
            if (node.is_public_node)
            {
                pub_node_.insert(node.base58addr);
            }
        }
    }
    /*if (node.is_public_node)
    {
        PublicNode public_node;
        if (Int2StrIPv4(node.public_ip, public_node.ip))
        {
            public_node.port = node.public_port;
            Singleton<Config>::instance()->add_public_node(public_node);
        }
    }*/
    return true;
}

void PeerNode::DeleteNodeByBase58Addr(const std::string &base58addr)
{
    if (base58addr.empty())
    {
        return;
    }
    std::string connection_id;
    {
        std::lock_guard<std::mutex> lck(nodes_mutex_);
        auto it = all_node_map_.find(base58addr);
        if (it != all_node_map_.end())
        {
            if(it->second.is_public_node)
            {
                pub_node_.erase(it->first);
            }
            all_node_map_.erase(it->first);
            if(it->second.is_connected())
            {
                connection_id = it->second.connection->connection_id();
            }
        }
    }
    Singleton<SocketManager>::instance()->DisConnect(connection_id);
}

bool PeerNode::UpdateNode(const Node &node)
{
    if (node.base58addr.empty())
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    auto it = all_node_map_.find(node.base58addr);
    if (all_node_map_.end() == it)
    {
        return false;
    }
    it->second.public_base58addr = node.public_base58addr;
    it->second.public_ip = node.public_ip;
    it->second.public_port = node.public_port;
    it->second.height = node.height;
    it->second.sign_fee = node.sign_fee;
    it->second.package_fee = node.package_fee;
    return true;
}
bool PeerNode::UpdateNodeConnect(const std::string &base58addr, std::shared_ptr<SocketConnection> connection)
{
    if (base58addr.empty())
    {
        return false;
    }
    Node node;
    {
        std::lock_guard<std::mutex> lck(nodes_mutex_);
        auto it = all_node_map_.find(base58addr);
        if (all_node_map_.end() == it)
        {
            return false;
        }
        node = it->second;
    }
    if(node.is_connected())
    {
        Singleton<SocketManager>::instance()->DisConnect(node.connection->connection_id());
    }
    {
        std::lock_guard<std::mutex> lck(nodes_mutex_);
        all_node_map_[node.base58addr] = node;
    }
    return true;
}

bool PeerNode::UpdateNodeHeight(const std::string &base58addr, uint64_t height)
{
    if (base58addr.empty())
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    auto it = all_node_map_.find(base58addr);
    if (all_node_map_.end() == it)
    {
        return false;
    }
    it->second.height = height;
    return true;
}

bool PeerNode::UpdateNodeSignFee(const std::string &base58addr, uint64_t sign_fee)
{
    if (base58addr.empty())
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    auto it = all_node_map_.find(base58addr);
    if (all_node_map_.end() == it)
    {
        return false;
    }
    it->second.sign_fee = sign_fee;
    return true;
}

bool PeerNode::UpdateNodePackageFee(const std::string &base58addr, uint64_t package_fee)
{
    if (base58addr.empty())
    {
        return false;
    }
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    auto it = all_node_map_.find(base58addr);
    if (all_node_map_.end() == it)
    {
        return false;
    }
    it->second.package_fee = package_fee;
    return true;
}

void PeerNode::UpdateNodesByPublicBase58Addr(const std::string &public_base58addr, const std::vector<Node> &nodes)
{
    std::lock_guard<std::mutex> lck(nodes_mutex_);
    std::vector<std::string> addrs1;
    std::vector<std::string> addrs2;
    for (auto &node : nodes)
    {
        if (node.base58addr.empty())
        {
            continue;
        }
        addrs1.push_back(node.base58addr);
        auto it = all_node_map_.find(node.base58addr);
        if (all_node_map_.end() != it)
        {
            it->second.public_base58addr = node.public_base58addr;
            it->second.public_ip = node.public_ip;
            it->second.public_port = node.public_port;
            it->second.height = node.height;
            it->second.sign_fee = node.sign_fee;
            it->second.package_fee = node.package_fee;
        }
        else
        {
            all_node_map_[node.base58addr] = node;
        }
    }
    for (auto &item : all_node_map_)
    {
        if (item.second.public_base58addr == public_base58addr)
        {
            addrs2.push_back(item.second.base58addr);
        }
    }
    std::sort(addrs1.begin(), addrs1.end());
    std::sort(addrs2.begin(), addrs2.end());
    std::vector<std::string> v_diff;
    std::set_difference(addrs1.begin(), addrs1.end(), addrs2.begin(), addrs2.end(), std::back_inserter(v_diff));
    for (auto &base58addr : v_diff)
    {
        auto it = all_node_map_.find(base58addr);
        if (all_node_map_.end() != it)
        {
            all_node_map_.erase(it);
        }
    }
}

void PeerNode::ConnectPublicList()
{
    if(!self_node_.is_public_node)
    {
        return;
    }
    std::vector<Node> nodes;
    {
        std::lock_guard<std::mutex> lck(nodes_mutex_);
        for (auto &addr : pub_node_)
        {
            auto it = all_node_map_.find(addr);
            if (all_node_map_.end() != it)
            {
                nodes.push_back(it->second);
            }
        }
    }
    for (auto &item : nodes)
    {
        if(item.is_connected())
        {
            continue;
        }
        auto socket_manager = Singleton<SocketManager>::instance();
        std::shared_ptr<SocketConnection> connection;
        auto ret = socket_manager->Connect(item.local_ip, item.listen_port, connection);
        if (0 != ret)
        {
            continue;
        }
        UpdateNodeConnect(item.base58addr, connection);
        SendConnectNodeReq(connection);
    }
}
