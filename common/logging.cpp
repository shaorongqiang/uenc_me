
#include "common/logging.h"
#include "common/config.h"
#include "utils/singleton.hpp"
#include <execinfo.h>
#include <iostream>
#include <signal.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

static std::shared_ptr<spdlog::logger> g_sink[LOGEND] = {nullptr};
static const char *g_sink_names[LOGEND] = {"ebpc"};
static const char *g_level_strs_[] = {"trace", "debug", "info", "warn", "error", "critical", "off"};
static void SystemErrorHandler(int signum);
static spdlog::level::level_enum GetLogLevel(const std::string &level);

bool LogInit()
{
    auto conf = Singleton<Config>::instance();
    spdlog::level::level_enum level = GetLogLevel(conf->log_level());
    return LogInit(conf->log_path(), level, conf->log_console_out());
}

bool LogInit(const std::string &path, spdlog::level::level_enum level, bool console_out)
{
    if (spdlog::level::level_enum::off == level)
    {
        return true;
    }
    try
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        std::vector<spdlog::sink_ptr> sinks;
        if (console_out)
            sinks.push_back(console_sink);
        for (int i = LOGMAIN; i < LOGEND; i++)
        {
            sinks.push_back(std::make_shared<spdlog::sinks::daily_file_sink_mt>(path + "/" + g_sink_names[i] + ".log", 0, 0, false, 7));
            g_sink[i] = std::make_shared<spdlog::logger>(g_sink_names[i], begin(sinks), end(sinks));
            //设置日志最低级别
            g_sink[i]->set_level(level);
            //当出现err级别的日志时立刻将缓存数据写入文件
            g_sink[i]->flush_on(spdlog::level::trace);
            //设置日志输出格式
            g_sink[i]->set_pattern("[%Y-%m-%d %H:%M:%S.%e][-%o][%t][%@:%!]%^[%l]:%v%$");
            //设置错误处理
            g_sink[i]->set_error_handler(
                [i](const std::string &msg)
                {
                    std::cout << " An error occurred in the " << g_sink_names[i] << " log system: " << msg << std::endl;
                    exit(-1);
                });
            sinks.pop_back();
        }
        //当程序产生崩溃时打印函数调用堆栈
        signal(SIGSEGV, SystemErrorHandler);
        signal(SIGABRT, SystemErrorHandler);
        return true;
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
        LogFini();
        return false;
    }
    catch (...)
    {
        std::cout << "Log initialization failed" << std::endl;
        LogFini();
        return false;
    }
}

spdlog::level::level_enum GetLogLevel(const std::string &level)
{
    for (size_t i = 0; i < sizeof(g_level_strs_) / sizeof(const char *); ++i)
    {
        if (strcasecmp(g_level_strs_[i], level.c_str()) == 0)
        {
            return (spdlog::level::level_enum)i;
        }
    }
    return spdlog::level::warn;
}
void SystemErrorHandler(int signum)
{
    const int len = 1024;
    void *func[len];
    signal(signum, SIG_DFL);
    size_t size = backtrace(func, len);
    char **funs = backtrace_symbols(func, size);
    CRITICALLOG("System error, Stack trace:");
    for (size_t i = 0; i < size; ++i)
        CRITICALLOG("{}: {}", i, funs[i]);
    free(funs);
}
std::shared_ptr<spdlog::logger> GetSink(LOGSINK sink)
{
    if (sink >= LOGMAIN && sink < LOGEND)
        return g_sink[sink];
    return g_sink[LOGMAIN];
}

void LogFini() { spdlog::shutdown(); }
