#ifndef UENC_SOCKET_LISTEN_NETV6_H_
#define UENC_SOCKET_LISTEN_NETV6_H_

#include "socket_manager.h"

class ListenNetv6 : public SocketListen
{
public:
    ListenNetv6() = default;
    virtual ~ListenNetv6() = default;
    ListenNetv6(ListenNetv6 &&) = delete;
    ListenNetv6(const ListenNetv6 &) = delete;
    ListenNetv6 &operator=(ListenNetv6 &&) = delete;
    ListenNetv6 &operator=(const ListenNetv6 &) = delete;

    int Init(event_base *eventbase, const std::string &addr, in_port_t port);

private:
    sockaddr_in6 sock_addr_;
    uint8_t addr_[16];
    in_port_t port_; //本地字节序
};

#endif
