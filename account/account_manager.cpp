#include "account/account_manager.h"
#include "common/logging.h"
#include <filesystem>

const std::string g_default_file_prefix("default.");
const std::string g_file_suffix(".private.key");

void AccountManager::SetDirectory(const std::string &directory)
{
    directory_ = directory;
    std::filesystem::path dir(directory_);
    if (std::filesystem::exists(dir) &&
        std::filesystem::file_type::directory != std::filesystem::directory_entry(dir).status().type())
    {
        std::filesystem::remove(dir);
    }
    std::filesystem::create_directories(directory_);
}

std::string AccountManager::GetDefaultAccount()
{
    return defaultaddr_;
}

std::vector<std::string> AccountManager::GetAccountList()
{
    std::vector<std::string> account_list;
    for (auto &item : account_list_)
    {
        account_list.push_back(item.second.base58addr());
    }
    return account_list;
}

bool AccountManager::GetAccountByAddr(const std::string &base58addr, Account::AccountAddr &account)
{
    std::lock_guard<std::mutex> lock(account_mutex_);
    for(auto &item : account_list_)
    {
        if(item.second.base58addr() == base58addr)
        {
            account = item.second;
            return true;
        }
    }
    return false;
}
int AccountManager::LoadAccount()
{
    {

        std::lock_guard<std::mutex> lock(account_mutex_);
        std::filesystem::path dir(directory_);
        if (!std::filesystem::exists(dir))
        {
            std::filesystem::create_directories(dir);
        }
        std::filesystem::directory_entry entry(dir);
        if (entry.status().type() != std::filesystem::file_type::directory)
        {
            return -1;
        }
        std::filesystem::directory_iterator list(dir);
        std::string filename;
        Account::AccountAddr account;
        for (auto &it : list)
        {
            //类型必须为常规类型
            if (std::filesystem::file_type::regular != std::filesystem::directory_entry(it.path()).status().type())
            {
                continue;
            }
            //文件名称大于后缀长度
            std::string name = it.path().filename().string();
            if (name.length() <= g_file_suffix.length())
            {
                continue;
            }
            //文件后缀必须为 file_suffix
            if (0 != name.compare(name.length() - g_file_suffix.length(), g_file_suffix.length(), g_file_suffix))
            {
                continue;
            }
            filename = std::filesystem::canonical(it.path()).string();
            auto ret = account.LoadAccount(filename);
            if(ret < 0)
            {
                return ret - 10000;
            }
            if(defaultaddr_.empty() && (0 == name.compare(0, g_default_file_prefix.length(), g_default_file_prefix)))
            {
                defaultaddr_ = account.base58addr();
            }
            account_list_.insert(std::make_pair(account.base58addr(), account));
        }
    }
    if(defaultaddr_.empty() || account_list_.empty())
    {
        auto ret = GenerateDefaultAccount();
        if(ret < 0)
        {
            return ret - 20000;
        }
    }

    return 0;
}

int AccountManager::GenerateDefaultAccount()
{

    if (!defaultaddr_.empty())
    {
        return 0;

    }
    Account::AccountAddr account;
    auto ret = account.GenerateAccount();
    if(ret < 0)
    {
        return ret - 1000;
    }
    account.SaveAccount(directory_, std::string("default."));
    {
        std::lock_guard<std::mutex> lock(account_mutex_);
        account_list_.insert(std::make_pair(account.base58addr(), account));
    }
    defaultaddr_ = account.base58addr();
    return 0;
}

int AccountManager::GenerateAccount(size_t count)
{
    std::lock_guard<std::mutex> lock( account_mutex_);
    std::filesystem::create_directories(directory_);
    size_t size = account_list_.size();
    Account::AccountAddr account;
    for (size_t i = 0; i < count; i++)
    {
        auto ret = account.GenerateAccount();
        if(ret < 0)
        {
            return ret - 100;
        }
        account.SaveAccount(directory_);
        account_list_.insert(std::make_pair(account.base58addr(), account));
        account.Clear();
    }
    return 0;
}
