#include "socket/connection_unix_domain.h"

ConnectionUnixDomain::ConnectionUnixDomain()
{
    data_source_ = DataSource::kUnixDomain;
}

int ConnectionUnixDomain::Init(event_base *eventbase, const std::string &unix_domain_path)
{
    auto ret = SocketConnection::Init(eventbase, -1);
    if (ret < 0)
    {
        return ret - 10;
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
    memset(&sock_addr_, 0, sizeof(sock_addr_));
    sock_addr_.sun_family = AF_UNIX;
    strncpy(sock_addr_.sun_path, unix_domain_path_.data(), unix_domain_path_.size());
    int len = offsetof(struct sockaddr_un, sun_path) + unix_domain_path_.size();
    ret = bufferevent_socket_connect(buffer_event_, (sockaddr *)&sock_addr_, len);
    if (ret < 0)
    {
        return -4;
    }
    fd_ = bufferevent_getfd(buffer_event_);
    is_connected_ = true;
    return 0;
}
