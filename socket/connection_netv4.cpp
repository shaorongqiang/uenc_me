#include "socket/connection_netv4.h"
#include "utils/net_utils.h"
#include <string.h>

ConnectionNetv4::ConnectionNetv4()
{
    data_source_ = DataSource::kNETV4;
}

int ConnectionNetv4::Init(event_base *eventbase, in_addr_t addr, in_port_t port)
{
    auto ret = SocketConnection::Init(eventbase, -1);
    if (ret < 0)
    {
        return ret - 10;
    }
    addr_ = addr;
    port_ = port;
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_addr.s_addr = addr_;
    sock_addr_.sin_port = htons(port_);
    ret = bufferevent_socket_connect(buffer_event_, (sockaddr *)&sock_addr_, sizeof(sock_addr_));
    if (ret < 0)
    {
        return -1;
    }
    fd_ = bufferevent_getfd(buffer_event_);
    is_connected_ = true;
    return 0;
}
