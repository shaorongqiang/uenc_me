
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include "httplib.h"
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

using namespace httplib;

typedef std::function<void(const Request &, Response &)> HttpCallBack;
class HttpServer
{
public:
    HttpServer();
    ~HttpServer() = default;

    static void work();
    static void start();
    static void registerAllCallback();
    static void registerCallback(std::string pattern, HttpCallBack handler)
    {
        HttpServer::cbs[pattern] = handler;
    }

private:
    static std::thread listen_thread;
    static std::map<const std::string, HttpCallBack> cbs;
};

void api_info(const Request &req, Response &res);

#endif
