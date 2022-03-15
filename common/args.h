#ifndef UENC_COMMON_ARGS_H_
#define UENC_COMMON_ARGS_H_

#include <string>

class Args
{
public:
    Args(int argc, char *argv[]);
    ~Args() = default;
    Args(Args &&) = delete;
    Args(const Args &) = delete;
    Args &operator=(Args &&) = delete;
    Args &operator=(const Args &) = delete;

    const std::string &cfg_file_name() const { return cfg_file_name_; }
    double sign_fee() const { return sign_fee_; }
    double pack_fee() const { return pack_fee_; }

private:
    void ParseCommand(int argc, char *argv[]);
    std::string cfg_file_name_;
    double sign_fee_;
    double pack_fee_;
    bool show_menu_;
    bool daemon_;
};

#endif
