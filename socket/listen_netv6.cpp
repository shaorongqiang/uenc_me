#include "listen_netv6.h"
#include "utils/net_utils.h"
#include <string.h>

int ListenNetv6::Init(event_base *eventbase, const std::string &addr, in_port_t port)
{
    if (!Str2IntIPv6(addr, addr_))
    {
        return -1;
    }
    port_ = port;
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin6_family = AF_INET6;
    memcpy(&sock_addr_.sin6_addr, addr_, sizeof(addr_));
    sock_addr_.sin6_port = htons(port_);
    auto ret = SocketListen::Init(eventbase, (const struct sockaddr *)&sock_addr_, sizeof(sock_addr_));
    {
        return ret - 100;
    }
    return true;
}
