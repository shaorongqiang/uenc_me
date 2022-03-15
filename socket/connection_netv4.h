#ifndef UENC_SOCKET_CONNECTION_NETV4_H_
#define UENC_SOCKET_CONNECTION_NETV4_H_

#include "socket/socket_manager.h"

class ConnectionNetv4 : public SocketConnection
{
public:
    ConnectionNetv4();
    virtual ~ConnectionNetv4() = default;
    ConnectionNetv4(ConnectionNetv4 &&) = delete;
    ConnectionNetv4(const ConnectionNetv4 &) = delete;
    ConnectionNetv4 &operator=(ConnectionNetv4 &&) = delete;
    ConnectionNetv4 &operator=(const ConnectionNetv4 &) = delete;

    int Init(event_base *eventbase, in_addr_t addr, in_port_t port);

private:
    sockaddr_in sock_addr_;
    in_addr_t addr_;
    in_port_t port_; //本地字节序
};
#endif
