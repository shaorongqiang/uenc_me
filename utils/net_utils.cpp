#include "net_utils.h"
#include <errno.h>
#include <ifaddrs.h>
#include <string.h>
#include <unistd.h>

uint32_t GetAdler32(const std::string &bytes)
{
    const uint32_t MOD_ADLER = 65521;
    uint32_t a = 1, b = 0;

    for (size_t index = 0; index < bytes.size(); ++index)
    {
        a = (a + (uint8_t)(bytes.at(index))) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    uint32_t adler32 = (b << 16) | a;
    return adler32;
}

//获取本地Ip
bool GetLocalIpv4(std::vector<uint64_t> &ips)
{
    struct ifaddrs *if_addr_ptr = nullptr;
    if (0 != getifaddrs(&if_addr_ptr))
    {
        return false;
    }
    uint32_t locale_ip = 0;
    std::string addr;
    for (; nullptr != if_addr_ptr; if_addr_ptr = if_addr_ptr->ifa_next)
    {
        if (if_addr_ptr->ifa_addr->sa_family == AF_INET)
        {
            locale_ip = ((struct sockaddr_in *)if_addr_ptr->ifa_addr)->sin_addr.s_addr;
            uint32_t ip = htonl(locale_ip);
            if (INADDR_LOOPBACK != ip && INADDR_BROADCAST != ip)
            {
                ips.push_back(ntohl(ip));
            }
        }
    }
    freeifaddrs(if_addr_ptr);
    return true;
}

bool GetIpv4AndPortByFd(int fd, in_addr_t &ip, in_port_t &port)
{
    ip = 0;
    port = 0;
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    memset(&addr, 0, sizeof(addr));
    int ret = getpeername(fd, (struct sockaddr *)&addr, &len);
    if (0 == ret)
    {
        ip = addr.sin_addr.s_addr;
        port = addr.sin_port;
        return true;
    }
    else
    {
        return false;
    }
}

bool Int2StrIPv4(in_addr_t ip, std::string &out)
{
    char buf[16] = {0};
    const char *ret = inet_ntop(AF_INET, (void *)&ip, buf, sizeof(buf));
    if (nullptr == ret)
    {
        return false;
    }
    out = buf;
    return true;
}

bool Str2IntIPv4(const std::string &ip, in_addr_t &out)
{
    int ret = inet_pton(AF_INET, ip.c_str(), &out);
    if (ret < 0)
    {
        return false;
    }
    return true;
}

bool Int2StrIPv6(uint8_t ip[16], std::string &out)
{
    char buf[INET6_ADDRSTRLEN] = {0};
    const char *ret = inet_ntop(AF_INET6, (void *)&ip, buf, sizeof(buf));
    if (nullptr == ret)
    {
        return false;
    }
    out = buf;
    return true;
}

bool Str2IntIPv6(const std::string &ip, uint8_t out[16])
{
    int ret = inet_pton(AF_INET6, ip.c_str(), &out);
    if (ret < 0)
    {
        return false;
    }
    return true;
}
