#include "listen_netv4.h"
#include "utils/net_utils.h"
#include <string.h>

int ListenNetv4::Init(event_base *eventbase, const std::string &addr, in_port_t port)
{
    in_addr_t addr_t = 0;
    if (!Str2IntIPv4(addr, addr_t))
    {
        return -1;
    }
    auto ret = Init(eventbase, addr_t, port);
    if (ret < 0)
    {
        return ret - 100;
    }
    return ret;
}

int ListenNetv4::Init(event_base *eventbase, in_addr_t addr, in_port_t port)
{
    addr_ = addr;
    port_ = port;
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sin_family = AF_INET;
    sock_addr_.sin_addr.s_addr = addr_;
    sock_addr_.sin_port = htons(port_);
    auto ret = SocketListen::Init(eventbase, (const struct sockaddr *)&sock_addr_, sizeof(sock_addr_));
    if(ret < 0)
    {
        return ret - 10;
    }
    return 0;
}
