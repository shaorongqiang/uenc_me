#include "common/config.h"
#include "common/logging.h"
#include "utils/net_utils.h"
#include "utils/singleton.hpp"
#include <filesystem>
#include <fstream>

const std::string kCfgUnixDomainPath("unix_domain_path");
const std::string kCfgDBPath("db_path");
const std::string kCfgCertPath("cert_path");

const std::string kCfgSignFee("sign_fee");
const std::string kCfgPackageFee("package_fee");

const std::string kCfgLogPath("log_path");
const std::string kCfgLogLevel("log_level");
const std::string kCfgLogConsoleOut("log_console_out");

const std::string kCfgSyncBlockTime("sync_block_time");
const std::string kCfgSyncBlockHeight("sync_block_height");
const std::string kCfgIsPublicNode("is_public_node");

const std::string kCfgKRefreshTime("k_refresh_time");
const std::string kCfgLocalIp("local_ip");
const std::string kCfgListenIp("listen_ip");
const std::string kCfgListenPort("listen_port");
const std::string kCfgWorkThreadNum("work_thread_num");

const std::string kCfgPublicNode("public_node");
const std::string kCfgPublicNodeIp("ip");
const std::string kCfgPublicNodePort("port");

void from_json(const nlohmann::json &json, PublicNode &public_node)
{
    json.at(kCfgPublicNodeIp).get_to(public_node.ip);
    json.at(kCfgPublicNodePort).get_to(public_node.port);
}
void to_json(nlohmann::json &json, const PublicNode &public_node)
{
    json[kCfgPublicNodeIp] = public_node.ip;
    json[kCfgPublicNodePort] = public_node.port;
}

Config::Config()
{
    SetDefaultVal();
}

bool Config::LoadFile()
{
    std::filesystem::path dir(file_name_);
    if (std::filesystem::exists(dir) &&
        std::filesystem::file_type::regular == std::filesystem::directory_entry(dir).status().type())
    {
        return ReadFile();
    }
    return WriteFile();
}
bool Config::WriteFile()
{
    if (file_name_.empty())
    {
        return false;
    }
    try
    {
        UpdateToJson();
        std::ofstream fconf(file_name_);
        fconf << config_json_.dump(4);
        fconf.close();
        return true;
    }
    catch (const std::exception &e)
    {
        return false;
    }
}

bool Config::public_node_list(std::vector<PublicNode> &public_nodes)
{
    std::lock_guard<std::mutex> lock(public_node_list_mutex_);
    for(auto &public_node : public_node_list_)
    {
        public_nodes.push_back(public_node);
    }
    return !public_nodes.empty();
}

void Config::add_public_node(const PublicNode &public_node)
{
    std::lock_guard<std::mutex> lock(public_node_list_mutex_);
    public_node_list_.insert(public_node);
}

bool Config::ReadFile()
{
    std::ifstream fconf(file_name_);
    fconf >> config_json_;
    fconf.close();
    return UpdateToVar();
}

void Config::SetDefaultVal()
{
    unix_domain_path_ = "/tmp/uenc.socket";
    db_path_ = "./data.db";
    cert_path_ = "./cert";

    sign_fee_ = 0;
    package_fee_ = 0;

    log_path_ = "./logs";
    log_level_ = "trace";
    log_console_out_ = false;

    sync_block_time_ = 100;
    sync_block_height_ = 200;
    is_public_node_ = false;

    k_refresh_time_ = 10;
    std::vector<uint64_t> ips;
    if (GetLocalIpv4(ips) && (!ips.empty()))
    {
        Int2StrIPv4(ips.at(0), local_ip_);
    }
    listen_ip_ = "0.0.0.0";
    work_thread_num_ = 10;

    std::vector<std::string> public_node_ip;
#ifdef PRIMARYCHAIN
    public_node_ip.push_back("115.29.149.102");
    public_node_ip.push_back("183.192.41.205");
    public_node_ip.push_back("221.130.95.110");
    public_node_ip.push_back("139.224.208.43");
    public_node_ip.push_back("221.130.95.78");
    public_node_ip.push_back("211.139.121.162");
    public_node_ip.push_back("36.152.253.86");
    public_node_ip.push_back("47.105.219.186");
    public_node_ip.push_back("139.224.210.202");
    public_node_ip.push_back("221.178.209.10");
    public_node_ip.push_back("36.153.194.74");
    public_node_ip.push_back("47.101.190.209");
    public_node_ip.push_back("36.152.125.110");
    public_node_ip.push_back("36.153.135.206");
    public_node_ip.push_back("112.124.25.146");
    public_node_ip.push_back("36.153.199.186");
    public_node_ip.push_back("221.130.95.102");
    public_node_ip.push_back("221.178.209.14");
    public_node_ip.push_back("211.139.122.22");
    public_node_ip.push_back("36.153.198.242");
    public_node_ip.push_back("47.106.247.169");
    public_node_ip.push_back("36.154.216.186");
    public_node_ip.push_back("35.73.172.97");
    public_node_ip.push_back("36.154.220.78");
    public_node_ip.push_back("47.104.254.152");
    public_node_ip.push_back("36.153.198.234");
    public_node_ip.push_back("36.153.195.250");
    public_node_ip.push_back("36.154.220.146");
    public_node_ip.push_back("36.154.220.150");
    public_node_ip.push_back("36.153.198.246");
    public_node_ip.push_back("112.25.165.14");
    public_node_ip.push_back("36.154.216.190");
    public_node_ip.push_back("101.133.238.221");
    public_node_ip.push_back("221.178.209.102");
    public_node_ip.push_back("106.15.237.182");
    public_node_ip.push_back("47.100.60.66");
    public_node_ip.push_back("36.153.197.34");
    public_node_ip.push_back("47.108.65.199");
    public_node_ip.push_back("47.114.175.31");
    public_node_ip.push_back("36.154.220.154");
    public_node_ip.push_back("8.133.177.14");
    public_node_ip.push_back("36.154.220.166");
    public_node_ip.push_back("221.130.94.21");
    public_node_ip.push_back("36.153.196.254");
    public_node_ip.push_back("221.178.209.98");
    listen_port_ = 11187;
#elif TESTCHAIN
    public_node_ip.push_back("120.79.216.93");
    public_node_ip.push_back("47.108.52.94");
    listen_port_ = 11188;
#else
    public_node_ip.push_back("36.153.129.46");
    public_node_ip.push_back("36.153.133.250");
    public_node_ip.push_back("36.153.128.106");
    public_node_ip.push_back("36.153.133.206");
    listen_port_ = 11189;
#endif
    public_node_list_.clear();
    PublicNode public_node;
    for (const auto &ip : public_node_ip)
    {
        public_node.ip = ip;
        public_node.port = listen_port_;
        public_node_list_.insert(public_node);
    }
}

bool Config::UpdateToJson()
{
    config_json_[kCfgUnixDomainPath] = unix_domain_path_;
    config_json_[kCfgDBPath] = db_path_;
    config_json_[kCfgCertPath] = cert_path_;

    config_json_[kCfgSignFee] = sign_fee_;
    config_json_[kCfgPackageFee] = package_fee_;

    config_json_[kCfgLogPath] = log_path_;
    config_json_[kCfgLogLevel] = log_level_;
    config_json_[kCfgLogConsoleOut] = log_console_out_;

    config_json_[kCfgSyncBlockTime] = sync_block_time_;
    config_json_[kCfgSyncBlockHeight] = sync_block_height_;
    config_json_[kCfgIsPublicNode] = is_public_node_;

    config_json_[kCfgKRefreshTime] = k_refresh_time_;
    config_json_[kCfgLocalIp] = local_ip_;
    config_json_[kCfgListenIp] = listen_ip_;
    config_json_[kCfgListenPort] = listen_port_;
    config_json_[kCfgWorkThreadNum] = work_thread_num_;

    nlohmann::json public_node_list_json;
    nlohmann::json public_node_json;
    for (auto &public_node : public_node_list_)
    {
        public_node_json = public_node;
        public_node_list_json.push_back(public_node_json);
    }
    config_json_[kCfgPublicNode] = public_node_list_json;
    return true;
}

bool Config::UpdateToVar()
{
    //将配置文件中的参数保存到成员变量中
    if (config_json_.end() != config_json_.find(kCfgUnixDomainPath))
    {
        config_json_.at(kCfgUnixDomainPath).get_to(unix_domain_path_);
    }
    if (config_json_.end() != config_json_.find(kCfgDBPath))
    {
        config_json_.at(kCfgDBPath).get_to(db_path_);
    }
    if (config_json_.end() != config_json_.find(kCfgCertPath))
    {
        config_json_.at(kCfgCertPath).get_to(cert_path_);
    }
    if (config_json_.end() != config_json_.find(kCfgSignFee))
    {
        config_json_.at(kCfgSignFee).get_to(sign_fee_);
    }
    if (config_json_.end() != config_json_.find(kCfgPackageFee))
    {
        config_json_.at(kCfgPackageFee).get_to(package_fee_);
    }
    if (config_json_.end() != config_json_.find(kCfgLogPath))
    {
        config_json_.at(kCfgLogPath).get_to(log_path_);
    }
    if (config_json_.end() != config_json_.find(kCfgLogLevel))
    {
        config_json_.at(kCfgLogLevel).get_to(log_level_);
    }
    if (config_json_.end() != config_json_.find(kCfgLogConsoleOut))
    {
        config_json_.at(kCfgLogConsoleOut).get_to(log_console_out_);
    }
    if (config_json_.end() != config_json_.find(kCfgSyncBlockTime))
    {
        config_json_.at(kCfgSyncBlockTime).get_to(sync_block_time_);
    }
    if (config_json_.end() != config_json_.find(kCfgSyncBlockHeight))
    {
        config_json_.at(kCfgSyncBlockHeight).get_to(sync_block_height_);
    }
    if (config_json_.end() != config_json_.find(kCfgIsPublicNode))
    {
        config_json_.at(kCfgIsPublicNode).get_to(is_public_node_);
    }
    if (config_json_.end() != config_json_.find(kCfgKRefreshTime))
    {
        config_json_.at(kCfgKRefreshTime).get_to(k_refresh_time_);
    }
    if (config_json_.end() != config_json_.find(kCfgLocalIp))
    {
        config_json_.at(kCfgLocalIp).get_to(listen_ip_);
    }
    if (config_json_.end() != config_json_.find(kCfgListenIp))
    {
        config_json_.at(kCfgListenIp).get_to(listen_ip_);
    }
    if (config_json_.end() != config_json_.find(kCfgListenPort))
    {
        config_json_.at(kCfgListenPort).get_to(listen_port_);
    }
    if (config_json_.end() != config_json_.find(kCfgWorkThreadNum))
    {
        config_json_.at(kCfgWorkThreadNum).get_to(work_thread_num_);
    }
    if (config_json_.end() != config_json_.find(kCfgPublicNode))
    {
        public_node_list_.clear();
        std::vector<nlohmann::json> public_node_list_json;
        config_json_.at(kCfgPublicNode).get_to(public_node_list_json);
        PublicNode public_node;
        for (auto &public_node_json : public_node_list_json)
        {
            try
            {
                public_node_json.get_to(public_node);
                public_node_list_.insert(public_node);
            }
            catch (...)
            {
                continue;
            }
        }
    }
    if (public_node_list_.empty())
    {
        return false;
    }
    return true;
}
