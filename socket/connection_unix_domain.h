#ifndef UENC_SOCKET_CONNECTION_UNIX_DOMAIN_H_
#define UENC_SOCKET_CONNECTION_UNIX_DOMAIN_H_

#include "socket/socket_manager.h"
#include <sys/un.h>

class ConnectionUnixDomain : public SocketConnection
{
public:
    ConnectionUnixDomain();
    virtual ~ConnectionUnixDomain() = default;
    ConnectionUnixDomain(ConnectionUnixDomain &&) = delete;
    ConnectionUnixDomain(const ConnectionUnixDomain &) = delete;
    ConnectionUnixDomain &operator=(ConnectionUnixDomain &&) = delete;
    ConnectionUnixDomain &operator=(const ConnectionUnixDomain &) = delete;

    int Init(event_base *eventbase, const std::string &unix_domain_path_);

private:
    sockaddr_un sock_addr_;
    std::string unix_domain_path_;
};
#endif
