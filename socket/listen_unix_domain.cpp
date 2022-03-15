#include "listen_unix_domain.h"
#include <unistd.h>

int ListenUnixDomain::Init(event_base *eventbase, const std::string &unix_domain_path)
{
    if (nullptr == eventbase)
    {
        return -1;
    }
    unix_domain_path_ = unix_domain_path;
    if (unix_domain_path_.empty())
    {
        return -2;
    }
    if (unix_domain_path_.size() > sizeof(sockaddr_un::sun_path))
    {
        return -3;
    }
    if (unlink(unix_domain_path_.data()) && errno != ENOENT)
    {
        return -4;
    }
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sun_family = AF_UNIX;
    strncpy(sock_addr_.sun_path, unix_domain_path_.data(), unix_domain_path_.length());
    size_t len = offsetof(struct sockaddr_un, sun_path) + unix_domain_path_.size();
    auto ret = SocketListen::Init(eventbase, (const struct sockaddr *)&sock_addr_, len);
    if(ret < 0)
    {
        return ret - 100;
    }
    return 0;
}
