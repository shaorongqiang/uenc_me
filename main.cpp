#include "common/args.h"
#include "common/config.h"
#include "common/logging.h"
#include "node/node_api.h"
#include "socket/socket_api.h"
#include <iostream>
#include <signal.h>
#include "account/account_manager.h"
#include "db/db_api.h"
#include "socket/socket_api.h"

static std::mutex g_program_exit_mutex;
static std::condition_variable g_program_exit_condition;

void ProgramExit(int signum)
{
    std::cout << "start exit program, please wait!!!" << std::endl;
    g_program_exit_condition.notify_all();
}

int main(int argc, char *argv[])
{
    signal(SIGINT, ProgramExit);
    signal(SIGUSR2, ProgramExit);
    std::unique_lock<std::mutex> locker(g_program_exit_mutex);
    Args arg(argc, argv);
    auto conf = Singleton<Config>::instance();
    auto acc_mgr = Singleton<AccountManager>::instance();
    int ret;
    conf->set_file_name(arg.cfg_file_name());
    if (!conf->LoadFile())
    {
        std::cout << "config load fail" << std::endl;
        goto ret;
    }
    if (!LogInit())
    {
        std::cout << "log init fail" << std::endl;
        goto ret;
    }
    acc_mgr->SetDirectory(conf->cert_path());
    ret = acc_mgr->LoadAccount();
    if(0 != ret)
    {
        std::cout << "account init fail:" << ret << std::endl;
        goto ret;
    }
    if (!DBInit())
    {
        std::cout << "db init fail:" << std::endl;
        goto ret;
    }
    ret = SocketInit();
    if(0 != ret)
    {
        std::cout << "socket init fail:" << ret << std::endl;
        goto ret;
    }
    ret = NodeInit();
    if(0 != ret)
    {
        std::cout << "node init fail:" << ret << std::endl;
        goto ret;
    }
    pthread_setname_np(pthread_self(), "uenc_main");
    g_program_exit_condition.wait(locker);
    g_program_exit_mutex.unlock();
    WARNLOG("program begin exit");
ret:
    NodeDestroy();
    SocketDestory();
    DBDestory();
    Singleton<Config>::instance()->WriteFile();
    LogFini();
    return 0;
}
