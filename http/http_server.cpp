
#include "http_server.h"
#include "../common/config.h"
#include "node/peer_node.h"
#include "utils/net_utils.h"
#include <functional>
#include <unistd.h>

std::thread HttpServer::listen_thread;
std::map<const std::string, HttpCallBack> HttpServer::cbs;
HttpServer http_server;

HttpServer::HttpServer()
{
    start();
}
void HttpServer::work()
{
    using namespace httplib;

    Server svr;

    for (auto item : cbs)
    {
        svr.Get(item.first.c_str(), item.second);
        svr.Post(item.first.c_str(), item.second);
    }

    int port = 8080;
    svr.listen("0.0.0.0", port);
}

void HttpServer::start()
{
    registerAllCallback();
    listen_thread = std::thread(HttpServer::work);
    listen_thread.detach();
}

void HttpServer::registerAllCallback()
{
    registerCallback("/info", api_info);
}

void api_info(const Request &req, Response &res)
{
    std::ostringstream oss;
    std::vector<Node> nodes;
    Singleton<PeerNode>::instance()->GetAllNodes(nodes);
    nodes.insert(nodes.begin(), Singleton<PeerNode>::instance()->self_node());
    std::string public_ip;
    std::string local_ip;
    for (auto &item : nodes)
    {
        if (!Int2StrIPv4(item.public_ip, public_ip))
        {
            public_ip.clear();
        }
        if (!Int2StrIPv4(item.local_ip, local_ip))
        {
            local_ip.clear();
        }
        oss
            << "  public_ip(" << public_ip << ")"
            << "  public_port(" << item.public_port << ")"
            << "  public_base58addr(" << item.public_base58addr << ")"
            << "  local_ip(" << local_ip << ")"
            << "  listen_port(" << item.listen_port << ")"
            << "  base58addr(" << item.base58addr << ")"
            << "  is_public(" << std::boolalpha << item.is_public_node << ")"
            << "  height( " << item.height << " )"
            << "  sign_fee(" << item.sign_fee << ")"
            << "  package_fee(" << item.package_fee << ")"
            << "  is_connected(" << std::boolalpha << item.is_connected() << ")"
            << "  version(" << item.version << ")"
            << std::endl;
    }
    res.set_content(oss.str(), "text/plain");
}
