#include "socket/connection_netv6.h"
#include "utils/net_utils.h"
#include <string.h>

ConnectionNetv6::ConnectionNetv6()
{
    data_source_ = DataSource::kNETV6;
}

int ConnectionNetv6::Init(event_base *eventbase, const std::string &addr, in_port_t port)
{
    auto ret = SocketConnection::Init(eventbase, -1);
    if (ret < 0)
    {
        return ret - 10;
    }

    if (!Str2IntIPv6(addr, addr_))
    {
        return -1;
    }
    port_ = port;
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin6_family = AF_INET6;
    memcpy(&sock_addr_.sin6_addr, addr_, sizeof(addr_));
    sock_addr_.sin6_port = htons(port_);
    ret = bufferevent_socket_connect(buffer_event_, (sockaddr *)&sock_addr_, sizeof(sock_addr_));
    if (ret < 0)
    {
        return -2;
    }
    fd_ = bufferevent_getfd(buffer_event_);
    is_connected_ = true;
    return 0;
}
