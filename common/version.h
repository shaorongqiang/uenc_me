#ifndef UENC_COMMON_VERSION_H_
#define UENC_COMMON_VERSION_H_

#include <string>

const std::string g_msg_version = "1";

#ifdef __linux__
const std::string g_os_version = "1";
#elif _WIN32
const std::string g_os_version = "2";
#elif _IOS
const std::string g_os_version = "3";
#elif __ANDROID__
const std::string g_os_version = "4";
#else
const std::string g_os_version = "0";
#endif

const std::string g_software_version = _VERSION;

#ifdef PRIMARYCHAIN
const std::string g_net_version = "p";
#elif TESTCHAIN
const std::string g_net_version = "t";
#else
const std::string g_net_version = "d";
#endif

const std::string g_version =  g_os_version + "_" + g_software_version + "_" + g_net_version;
#endif
