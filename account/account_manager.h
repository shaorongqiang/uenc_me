#ifndef UENC_ACCOUNT_ACCOUNT_MANAGER_H_
#define UENC_ACCOUNT_ACCOUNT_MANAGER_H_

#include "account/account.h"
#include <vector>

class AccountManager
{
public:
    AccountManager() = default;
    ~AccountManager() = default;
    AccountManager(AccountManager &&) = delete;
    AccountManager(const AccountManager &) = delete;
    AccountManager &operator=(AccountManager &&) = delete;
    AccountManager &operator=(const AccountManager &) = delete;

    void SetDirectory(const std::string &directory);

    int LoadAccount();
    int GenerateDefaultAccount();
    int GenerateAccount(size_t count);

    std::string GetDefaultAccount();
    std::vector<std::string> GetAccountList();
    bool GetAccountByAddr(const std::string &base58addr, Account::AccountAddr &account);

private:
    std::string directory_;
    std::mutex account_mutex_;
    std::string defaultaddr_;
    std::unordered_map<std::string, Account::AccountAddr> account_list_;
};

#endif
