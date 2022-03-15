#ifndef UENC_SOCKET_LISTEN_NETV4_H_
#define UENC_SOCKET_LISTEN_NETV4_H_

#include "socket/socket_manager.h"

class ListenNetv4 : public SocketListen
{
public:
    ListenNetv4() = default;
    virtual ~ListenNetv4() = default;
    ListenNetv4(ListenNetv4 &&) = delete;
    ListenNetv4(const ListenNetv4 &) = delete;
    ListenNetv4 &operator=(ListenNetv4 &&) = delete;
    ListenNetv4 &operator=(const ListenNetv4 &) = delete;

    int Init(event_base *eventbase, const std::string &addr, in_port_t port);
    int Init(event_base *eventbase, in_addr_t addr, in_port_t port);

private:
    sockaddr_in sock_addr_;
    in_addr_t addr_;
    in_port_t port_; //本地字节序
};

#endif
