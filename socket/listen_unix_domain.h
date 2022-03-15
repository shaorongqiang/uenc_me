#ifndef UENC_SOCKET_LISTEN_UNIX_DOMAIN_H_
#define UENC_SOCKET_LISTEN_UNIX_DOMAIN_H_

#include "socket_manager.h"
#include <sys/un.h>

class ListenUnixDomain : public SocketListen
{
public:
    ListenUnixDomain() = default;
    virtual ~ListenUnixDomain() = default;
    ListenUnixDomain(ListenUnixDomain &&) = delete;
    ListenUnixDomain(const ListenUnixDomain &) = delete;
    ListenUnixDomain &operator=(ListenUnixDomain &&) = delete;
    ListenUnixDomain &operator=(const ListenUnixDomain &) = delete;

    int Init(event_base *eventbase, const std::string &unix_domain_path);

private:
    sockaddr_un sock_addr_;
    std::string unix_domain_path_;
};

#endif
