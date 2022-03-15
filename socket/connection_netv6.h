#ifndef UENC_SOCKET_CONNECTION_NETV6_H_
#define UENC_SOCKET_CONNECTION_NETV6_H_

#include "socket/socket_manager.h"

class ConnectionNetv6 : public SocketConnection
{
public:
    ConnectionNetv6();
    virtual ~ConnectionNetv6() = default;
    ConnectionNetv6(ConnectionNetv6 &&) = delete;
    ConnectionNetv6(const ConnectionNetv6 &) = delete;
    ConnectionNetv6 &operator=(ConnectionNetv6 &&) = delete;
    ConnectionNetv6 &operator=(const ConnectionNetv6 &) = delete;

    int Init(event_base *eventbase, const std::string &addr, in_port_t port);

private:
    sockaddr_in6 sock_addr_;
    uint8_t addr_[16];
    in_port_t port_; //本地字节序
};
#endif
