#include "socket/socket_manager.h"
#include "common/logging.h"
#include "socket/connection_netv4.h"
#include "socket/connection_unix_domain.h"
#include "socket/listen_netv4.h"
#include "socket/listen_unix_domain.h"
#include "socket/socket_api.h"
#include "utils/net_utils.h"
#include <bitset>
#include <random>
#include <string.h>
#include <unistd.h>

static void MakeRandId(std::string &id)
{
    std::bitset<160> bit;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    static std::default_random_engine e(tv.tv_sec + tv.tv_usec + getpid());
    static std::uniform_int_distribution<unsigned> u(0, 1);
    for (uint32_t i = 0; i < 160; i++)
    {
        bit[i] = u(e);
    }

    std::string idstr = bit.to_string();
    for (size_t i = 0; i < idstr.size() / 8; i++)
    {
        std::string tmp(idstr.begin() + i * 8, idstr.begin() + i * 8 + 8);
        unsigned int slice = std::stol(tmp, 0, 2);
        char buff[20] = {0};
        sprintf(buff, "%02x", slice);
        id += buff;
    }
}

SocketListen::SocketListen()
{
    event_listener_ = nullptr;
    MakeRandId(listen_id_);
}

int SocketListen::Init(event_base *eventbase, const struct sockaddr *sa, int socklen)
{
    if (nullptr != event_listener_)
    {
        return -1;
    }
    event_listener_ = evconnlistener_new_bind(eventbase, &SocketManager::listener_callback, this,
                                              LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, sa, socklen);
    if (nullptr == event_listener_)
    {
        return -2;
    }
    return 0;
}

void SocketListen::Destory()
{
    if (nullptr != event_listener_)
    {
        evconnlistener_free(event_listener_);
    }
    event_listener_ = nullptr;
}

SocketConnection::SocketConnection()
{
    data_source_ = DataSource::kNone;
    is_connected_ = false;
    fd_ = -1;
    buffer_event_ = nullptr;
    MakeRandId(connection_id_);
}

SocketConnection::~SocketConnection()
{
    data_source_ = DataSource::kNone;
    is_connected_ = false;
    buffer_event_ = nullptr;
}

int SocketConnection::Init(event_base *eventbase, evutil_socket_t fd)
{
    buffer_event_ = bufferevent_socket_new(eventbase, fd, BEV_OPT_DEFER_CALLBACKS);
    if (nullptr == buffer_event_)
    {
        return -1;
    }
    bufferevent_setcb(buffer_event_, &SocketManager::read_callback, nullptr,
                      &SocketManager::event_callback, &connection_id_);
    bufferevent_enable(buffer_event_, EV_READ | EV_WRITE);
    if (-1 != fd)
    {
        fd_ = fd;
        is_connected_ = true;
        evutil_make_socket_nonblocking(fd);
    }
    return 0;
}

void SocketConnection::Destroy()
{
    if (nullptr != buffer_event_)
    {
        bufferevent_free(buffer_event_);
    }
    buffer_event_ = nullptr;
    is_connected_ = false;
}

int SocketConnection::WriteMsg(const std::string &bytes_msg)
{
    if (!is_connected_)
    {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lck(write_mutex_);
        write_data_ += bytes_msg;
    }
    auto ret = WriteData();
    if (ret < 0)
    {
        return ret - 10;
    }
    return 0;
}

int SocketConnection::WriteMsg(const std::vector<std::string> &bytes_msgs)
{
    if (!is_connected_)
    {
        return -1;
    }
    std::string data;
    for (auto &msg : bytes_msgs)
    {
        data += msg;
    }
    {
        std::lock_guard<std::mutex> lck(write_mutex_);
        write_data_ += data;
    }
    auto ret = WriteData();
    if (ret < 0)
    {
        return ret - 10;
    }
    return 0;
}

int SocketConnection::ReadData(const std::string &data, std::vector<std::pair<std::shared_ptr<google::protobuf::Message>, Priority>> &msgs)
{
    std::lock_guard<std::mutex> lck(read_mutex_);
    int ret = 0;
    read_data_ += data;
    Priority priority;
    std::shared_ptr<google::protobuf::Message> msg;
    do
    {
        priority = Priority::kPriority_Low_0;
        msg.reset();
        ret = Bytes2Proto(read_data_, priority, msg);
        if (ret < 0)
        {
            if (read_data_.size() > sizeof(uint32_t))
            {
                uint32_t length = 0;
                memcpy(&length, read_data_.data(), sizeof(uint32_t));
                length = le32toh(length) + sizeof(uint32_t);
                if (read_data_.size() > length + sizeof(uint32_t))
                {
                    read_data_.erase(0, length + sizeof(uint32_t));
                }
                else
                {
                    read_data_.clear();
                }
            }
            else
            {
                read_data_.clear();
            }
        }
        else if (ret > 0)
        {
            read_data_.erase(0, ret);
            msgs.push_back(std::make_pair(msg, priority));
        }

    } while (0 != ret);

    if (this->read_data_.capacity() > this->read_data_.size() * 20)
    {
        this->read_data_.shrink_to_fit();
    }

    return 0;
}

int SocketConnection::WriteData()
{
    if (!is_connected_)
    {
        return -1;
    }
    std::lock_guard<std::mutex> lck(write_mutex_);
    int ret = bufferevent_write(buffer_event_, write_data_.c_str(), write_data_.size());
    if (0 != ret)
    {
        return -2;
    }
    std::string().swap(write_data_);
    return 0;
}

SocketManager::SocketManager()
{
    disconnect_callback_ = nullptr;
    event_base_ = nullptr;
    event_base_ = event_base_new();
    if (nullptr == event_base_)
    {
        return;
    }
    event_set_log_callback(
        [](int severity, const char *msg)
        {
            switch (severity)
            {
            case _EVENT_LOG_DEBUG:
            {
                DEBUGLOG("libevent error: {}", msg);
                break;
            }
            case _EVENT_LOG_MSG:
            {
                INFOLOG("libevent error: {}", msg);
                break;
            }
            case _EVENT_LOG_WARN:
            {
                WARNLOG("libevent error: {}", msg);
                break;
            }
            case _EVENT_LOG_ERR:
            {
                ERRORLOG("libevent error: {}", msg);
                break;
            }
            default:
            {
                TRACELOG("libevent error: {}", msg);
                break;
            }
            };
        });
}

SocketManager::~SocketManager()
{
    if (nullptr != event_base_)
    {
        event_base_free(event_base_);
    }
    event_base_ = nullptr;
}

std::shared_ptr<SocketConnection> SocketManager::GetConnection(const std::string &connection_id)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(connection_id);
    if (it == connections_.end())
    {
        return nullptr;
    }
    connections_.erase(it);
    return it->second;
}

void SocketManager::ThreadStart()
{
    event_thread_ = std::thread(std::bind(&SocketManager::ThreadWork, this));
    event_thread_.detach();

    scan_thread_ = std::thread([this]()
                               {
        std::unordered_map<std::string, std::shared_ptr<SocketConnection>> connections;
        while (continue_runing_)
        {
            std::unique_lock<std::mutex> locker(scan_mutex_);
            scan_condition_.wait_for(locker, std::chrono::seconds(10));
            if (!continue_runing_)
            {
                return;
            }
            {
                std::lock_guard<std::mutex> lock(connections_mutex_);
                connections = connections_;
            }
            for (auto &item : connections)
            {
                if(nullptr == item.second)
                {
                    DeleteConnection(item.first);
                }
                else if(!item.second->IsConnected())
                {
                    DeleteConnection(item.first);
                }
                else if(item.second->GetLastRecvIntervalTime() > 10 * 60)
                {
                    DeleteConnection(item.first);
                }
            }
            connections.clear();
        } });
}

void SocketManager::ThreadWork()
{
    event_base_dispatch(event_base_);
}

void SocketManager::ThreadStop()
{
    if (nullptr != event_base_)
    {
        event_base_loopbreak(event_base_);
    }
}

int SocketManager::Listen(const std::string &addr, in_port_t port)
{
    if (nullptr == event_base_)
    {
        return -1;
    }
    std::shared_ptr<ListenNetv4> listen = std::make_shared<ListenNetv4>();
    auto ret = listen->Init(event_base_, addr, port);
    if (ret < 0)
    {
        return ret - 1000;
    }
    ret = AddListen(listen);
    if (ret < 0)
    {
        return ret - 2000;
    }
    return 0;
}

int SocketManager::Listen(const std::string &unix_domain_path)
{
    if (nullptr == event_base_)
    {
        return -1;
    }
    std::shared_ptr<ListenUnixDomain> listen = std::make_shared<ListenUnixDomain>();
    auto ret = listen->Init(event_base_, unix_domain_path);
    if (ret < 0)
    {
        return ret - 1000;
    }
    ret = AddListen(listen);
    if (ret < 0)
    {
        return ret - 2000;
    }
    return 0;
}
int SocketManager::Connect(const std::string &addr, in_port_t port, std::shared_ptr<SocketConnection> &out_connection)
{
    in_addr_t addr_t = 0;
    if (!Str2IntIPv4(addr, addr_t))
    {
        return -1000;
    }
    return Connect(addr_t, port, out_connection);
}
int SocketManager::Connect(in_addr_t addr, in_port_t port, std::shared_ptr<SocketConnection> &out_connection)
{
    if (nullptr == event_base_)
    {
        return -1;
    }
    std::shared_ptr<ConnectionNetv4> connection = std::make_shared<ConnectionNetv4>();
    auto ret = connection->Init(event_base_, addr, port);
    if (ret < 0)
    {
        return ret - 100;
    }
    if (!connection->IsConnected())
    {
        return -3;
    }
    ret = AddConnection(connection);
    if (ret < 0)
    {
        return ret - 200;
    }
    out_connection.reset();
    out_connection = connection;
    return 0;
}

int SocketManager::Connect(std::string &unix_domain_path, std::shared_ptr<SocketConnection> &out_connection)
{
    if (nullptr == event_base_)
    {
        return -1;
    }
    std::shared_ptr<ConnectionUnixDomain> connection = std::make_shared<ConnectionUnixDomain>();
    auto ret = connection->Init(event_base_, unix_domain_path);
    if (ret < 0)
    {
        return ret - 1000;
    }
    if (!connection->IsConnected())
    {
        return -3;
    }
    ret = AddConnection(connection);
    if (ret < 0)
    {
        return ret - 10;
    }
    out_connection.reset();
    out_connection = connection;
    return 0;
}
void SocketManager::Clear()
{
    {
        std::lock_guard<std::mutex> lock(listens_mutex_);
        listens_.clear();
    }
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
    }
}

void SocketManager::DisConnect(const std::string &connection_id)
{
    DeleteConnection(connection_id);
}

int SocketManager::AddListen(std::shared_ptr<SocketListen> listen)
{
    if (nullptr == listen)
    {
        return -1;
    }
    {
        std::lock_guard<std::mutex> lock(listens_mutex_);
        if (listens_.cend() != listens_.find(listen->listen_id()))
        {
            return -2;
        }
        listens_.insert(std::make_pair(listen->listen_id(), listen));
    }
    return 0;
}

void SocketManager::DeleteListen(const std::string &listen_id)
{
    std::lock_guard<std::mutex> lock(listens_mutex_);
    auto it = listens_.find(listen_id);
    if (listens_.end() != it)
    {
        listens_.erase(it);
    }
}
std::shared_ptr<SocketConnection> SocketManager::FindConnectionById(const std::string &connection_id)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(connection_id);
    if (connections_.cend() != it)
    {
        return it->second;
    }
    return nullptr;
}

int SocketManager::AddConnection(std::shared_ptr<SocketConnection> connection)
{
    if (nullptr == connection)
    {
        return -1;
    }
    if (!connection->IsConnected())
    {
        return -2;
    }
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        if (connections_.cend() != connections_.find(connection->connection_id()))
        {
            return -3;
        }
        connections_.insert(std::make_pair(connection->connection_id(), connection));
    }
    return 0;
}

void SocketManager::DeleteConnection(const std::string &connection_id)
{
    std::lock_guard<std::mutex> lock(connections_mutex_);
    auto it = connections_.find(connection_id);
    if (connections_.end() != it)
    {
        connections_.erase(it);
        if (nullptr != disconnect_callback_)
        {
            disconnect_callback_(connection_id);
        }
    }
}

void SocketManager::listener_callback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ptr)
{
    std::shared_ptr<SocketConnection> connextion = std::make_shared<SocketConnection>();
    if (0 == connextion->Init(Singleton<SocketManager>::instance()->event_base_, fd))
    {
        Singleton<SocketManager>::instance()->AddConnection(connextion);
    }
}

void SocketManager::read_callback(bufferevent *bufevent, void *ptr)
{
    if (nullptr == bufevent || nullptr == ptr)
    {
        return;
    }
    char buf[BUFSIZ] = {0};
    size_t size = bufferevent_read(bufevent, buf, sizeof(buf));
    if (0 == size)
    {
        return;
    }
    MsgData msg;
    std::string * connection_id = (std::string *)ptr;
    msg.connection = Singleton<SocketManager>::instance()->FindConnectionById(*connection_id);
    if (nullptr == msg.connection)
    {
        Singleton<SocketManager>::instance()->DeleteConnection(*connection_id);
        return;
    }
    std::vector<std::pair<std::shared_ptr<google::protobuf::Message>, Priority>> msgs;
    if (0 != msg.connection->ReadData(std::string(buf, size), msgs))
    {
        return;
    }
    if (msgs.empty())
    {
        return;
    }
    for (auto &item : msgs)
    {
        msg.msg = item.first;
        msg.priority = item.second;
        Singleton<ProtobufProcess>::instance()->AddProcessData(msg);
    }
}

void SocketManager::event_callback(bufferevent *bufevent, short events, void *ptr)
{
    if (events & BEV_EVENT_CONNECTED)
    {
        DEBUGLOG("bufferevent connect operation finished");
    }
    if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF | BEV_EVENT_TIMEOUT | BEV_EVENT_READING | BEV_EVENT_WRITING))
    {
        if (events & BEV_EVENT_ERROR)
        {
            ERRORLOG("bufferevent unrecoverable error encountered");
        }
        if (events & BEV_EVENT_EOF)
        {
            ERRORLOG("bufferevent eof file reached");
        }
        if (events & BEV_EVENT_TIMEOUT)
        {
            ERRORLOG("bufferevent user-specified timeout reached");
        }
        if (events & BEV_EVENT_READING)
        {
            DEBUGLOG("bufferevent error encountered while reading");
        }
        if (events & BEV_EVENT_WRITING)
        {
            DEBUGLOG("bufferevent error encountered while writing");
        }
        if (nullptr != ptr)
        {
            std::string connection_id = *(std::string *)ptr;
            std::shared_ptr<SocketConnection> connection = Singleton<SocketManager>::instance()->FindConnectionById(connection_id);
            if (nullptr == connection)
            {
                Singleton<SocketManager>::instance()->DeleteConnection(connection_id);
                return;
            }
            connection->is_connected_ = false;
        }
    }
}
