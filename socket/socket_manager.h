#ifndef UENC_SOCKET_SOCKET_MANAGER_H_
#define UENC_SOCKET_SOCKET_MANAGER_H_

#include <google/protobuf/message.h>
#include "socket/define.h"
#include <condition_variable>
#include <event.h>
#include <event2/listener.h>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <unordered_map>
#include <vector>

class SocketListen
{
public:
    SocketListen();
    virtual ~SocketListen() = default;
    SocketListen(SocketListen &&) = delete;
    SocketListen(const SocketListen &) = delete;
    SocketListen &operator=(SocketListen &&) = delete;
    SocketListen &operator=(const SocketListen &) = delete;
    const std::string &listen_id() { return listen_id_; }

    int Init(event_base *eventbase, const struct sockaddr *sa, int socklen);
    void Destory();

protected:
    evconnlistener *event_listener_;
    evutil_socket_t listen_fd_;
    std::string listen_id_;

private:
};

class SocketManager;
class SocketConnection
{
public:
    SocketConnection();
    virtual ~SocketConnection();
    SocketConnection(SocketConnection &&) = delete;
    SocketConnection(const SocketConnection &) = delete;
    SocketConnection &operator=(SocketConnection &&) = delete;
    SocketConnection &operator=(const SocketConnection &) = delete;

    int Init(event_base *eventbase, evutil_socket_t fd);
    void Destroy();
    int WriteMsg(const std::string &bytes_msg);
    int WriteMsg(const std::vector<std::string> &bytes_msgs);
    bool IsConnected() { return is_connected_; }
    time_t GetLastRecvIntervalTime() { return time(nullptr) - last_received_time_; }
    const std::string &connection_id() { return connection_id_; }
    DataSource data_source() { return data_source_; }
    evutil_socket_t fd() { return fd_; }

protected:
    DataSource data_source_;
    bufferevent *buffer_event_;
    bool is_connected_;
    evutil_socket_t fd_;

private:
    friend class SocketManager;
    int ReadData(const std::string &data, std::vector<std::pair<std::shared_ptr<google::protobuf::Message>, Priority>> &msgs);
    int WriteData();

    time_t last_received_time_;
    std::string connection_id_;

    std::mutex read_mutex_;
    std::string read_data_;

    std::mutex write_mutex_;
    std::string write_data_;
};

class SocketManager
{
public:
    SocketManager();
    ~SocketManager();
    SocketManager(SocketManager &&) = delete;
    SocketManager(const SocketManager &) = delete;
    SocketManager &operator=(SocketManager &&) = delete;
    SocketManager &operator=(const SocketManager &) = delete;
    void SetDisConnectCallBack(std::function<void(const std::string &connection_id)> disconnect_callback) { disconnect_callback_ = disconnect_callback; }
    std::shared_ptr<SocketConnection> GetConnection(const std::string &connection_id);

    void ThreadStart();
    void ThreadWork();
    void ThreadStop();

    int Listen(const std::string &addr, in_port_t port);
    int Listen(const std::string &unix_domain_path);
    int Connect(const std::string &addr, in_port_t port, std::shared_ptr<SocketConnection> &out_connection);
    int Connect(in_addr_t addr, in_port_t port, std::shared_ptr<SocketConnection> &out_connection);
    int Connect(std::string &unix_domain_path, std::shared_ptr<SocketConnection> &out_connection);
    void Clear();
    void DisConnect(const std::string &connection_id);

private:
    int AddListen(std::shared_ptr<SocketListen> listen);
    void DeleteListen(const std::string &listen_id);

    std::shared_ptr<SocketConnection> FindConnectionById(const std::string &connection_id);
    int AddConnection(std::shared_ptr<SocketConnection> connection);
    void DeleteConnection(const std::string &connection_id);

private:
    std::thread scan_thread_;
    bool continue_runing_;
    std::mutex scan_mutex_;
    std::condition_variable scan_condition_;

    std::thread event_thread_;
    event_base *event_base_;
    std::mutex listens_mutex_;
    std::unordered_map<std::string, std::shared_ptr<SocketListen>> listens_;
    std::mutex connections_mutex_;
    std::unordered_map<std::string, std::shared_ptr<SocketConnection>> connections_;
    std::function<void(const std::string &connection_id)> disconnect_callback_;

    friend class SocketListen;
    friend class SocketConnection;
    static void listener_callback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int len, void *ptr);
    static void read_callback(bufferevent *bufevent, void *ptr);
    static void event_callback(bufferevent *bufevent, short events, void *ptr);
};

#endif
