#ifndef UENC_COMMON_CONFIG_H_
#define UENC_COMMON_CONFIG_H_

#include <nlohmann/json.hpp>
#include <set>
#include <mutex>

struct PublicNode
{
    std::string ip; //公网节点ip
    uint16_t port;  //公网节点端口
    bool operator<(const PublicNode &p) const
    {
        if(ip < p.ip)
        {
            return true;
        }
        else
        {
            return port < p.port;
        }
    }
};

class Config
{
public:
    Config();
    ~Config() = default;
    Config(Config &&) = delete;
    Config(const Config &) = delete;
    Config &operator=(Config &&) = delete;
    Config &operator=(const Config &) = delete;

    bool LoadFile();
    //写入配置文件
    bool WriteFile();
    const std::string &file_name() { return file_name_; }
    void set_file_name(const std::string &file_name) { file_name_ = file_name; }

    const std::string &unix_domain_path() const { return unix_domain_path_; }
    const std::string &db_path() const { return db_path_; }
    const std::string &cert_path() const { return cert_path_; }

    uint32_t sign_fee() const { return sign_fee_; }
    uint32_t package_fee() const { return package_fee_; }

    const std::string &log_path() const { return log_path_; }
    const std::string &log_level() const { return log_level_; }
    bool log_console_out() const { return log_console_out_; }
    uint32_t sync_block_time() const { return sync_block_time_; }
    uint32_t sync_block_height() const { return sync_block_height_; }
    bool is_public_node() const { return is_public_node_; }
    uint16_t k_refresh_time() const { return k_refresh_time_; }
    const std::string &local_ip() const { return local_ip_; }
    const std::string &listen_ip() const { return listen_ip_; }
    uint16_t listen_port() const { return listen_port_; }
    uint16_t work_thread_num() const { return work_thread_num_; }
    bool public_node_list(std::vector<PublicNode> &public_nodes);
    void add_public_node(const PublicNode &public_node);

private:
    bool ReadFile();
    void SetDefaultVal();
    //将成员变量更新到config_json_中
    bool UpdateToJson();
    bool UpdateToVar();
    std::string file_name_;
    nlohmann::json config_json_;

    std::string unix_domain_path_;
    std::string db_path_;
    std::string cert_path_;

    uint32_t sign_fee_;
    uint32_t package_fee_;

    std::string log_path_;  //日志存储路径
    std::string log_level_; //最低记录的日志级别
    bool log_console_out_;  //日志是否输出到控制台

    uint32_t sync_block_time_;   //区块同步时间
    uint32_t sync_block_height_; //每次同步区块的高度
    bool is_public_node_;        //当前节点是否为公网节点

    uint32_t k_refresh_time_; // k桶刷新时间
    std::string local_ip_;    //当前设备ip
    std::string listen_ip_;
    uint16_t listen_port_;     //用于protobuf通信的端口
    uint32_t work_thread_num_; //工作线程的数量

    std::mutex public_node_list_mutex_;
    std::set<PublicNode> public_node_list_;
};

#endif
