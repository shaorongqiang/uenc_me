#include "db/rocksdb.h"
#include "common/logging.h"
#include <sstream>

static std::vector<std::string> Split(const std::string &arg, char delim)
{
    std::vector<std::string> splits;
    std::stringstream ss(arg);
    std::string item;
    while (std::getline(ss, item, delim))
    {
        splits.push_back(item);
    }
    return splits;
}

void BackgroundErrorListener::OnBackgroundError(rocksdb::BackgroundErrorReason reason, rocksdb::Status *bg_error)
{
    if (bg_error != nullptr)
    {
        ERRORLOG("RocksDB Background Error {} code:({}),subcode:({}),severity:({}),info:({})", reason,
                 bg_error->code(), bg_error->subcode(), bg_error->severity(), bg_error->ToString());
    }
    else
    {
        ERRORLOG("RocksDB Background Error {}", reason);
    }
}

RocksDB::RocksDB()
{
    db_ = nullptr;
    is_init_success_ = false;
}

RocksDB::~RocksDB()
{
    db_ = nullptr;
    std::lock_guard<std::mutex> lock(is_init_success_mutex_);
    is_init_success_ = false;
}

void RocksDB::SetDBPath(const std::string &db_path)
{
    db_path_ = db_path;
}

bool RocksDB::InitDB(rocksdb::Status &ret_status)
{
    if (is_init_success_)
    {
        return false;
    }

    rocksdb::Options options;
    options.create_if_missing = true;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    auto listener = std::make_shared<BackgroundErrorListener>();
    options.listeners.push_back(listener);
    rocksdb::TransactionDBOptions txn_db_options;
    ret_status = rocksdb::TransactionDB::Open(options, txn_db_options, db_path_, &db_);
    if (ret_status.ok())
    {
        is_init_success_ = true;
    }
    else
    {
        ERRORLOG("rocksdb {} Open failed code:({}),subcode:({}),severity:({}),info:({})",
                 db_path_, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return is_init_success_;
}
void RocksDB::DestoryDB()
{
    {
        DEBUGLOG("DestoryDB");
        std::lock_guard<std::mutex> lock(is_init_success_mutex_);
        is_init_success_ = false;
    }

    rocksdb::Status ret_status;
    if (nullptr != db_)
    {
        ret_status = db_->Close();
        if (!ret_status.ok())
        {
            ERRORLOG("rocksdb {} Close failed code:({}),subcode:({}),severity:({}),info:({})",
                     db_path_, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
            return;
        }
        delete db_;
    }
    db_ = nullptr;
}
bool RocksDB::IsInitSuccess()
{
    std::lock_guard<std::mutex> lock(is_init_success_mutex_);
    return is_init_success_;
}

RocksDBReader::RocksDBReader(std::shared_ptr<RocksDB> rocksdb)
{
    rocksdb_ = rocksdb;
}

bool RocksDBReader::MultiReadData(const std::vector<rocksdb::Slice> &keys, std::vector<std::string> &values, std::vector<rocksdb::Status> &ret_status)
{
    ret_status.clear();
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status.push_back(rocksdb::Status::Aborted());
        return false;
    }
    if (keys.empty())
    {
        ERRORLOG("key is empty");
        ret_status.push_back(rocksdb::Status::Aborted());
        return false;
    }
    ret_status = rocksdb_->db_->MultiGet(read_options_, keys, &values);
    bool flag = true;
    for (size_t i = 0; i < ret_status.size(); ++i)
    {
        auto status = ret_status.at(i);
        if (!status.ok())
        {
            flag = false;
            std::string key;
            if (keys.size() > i)
            {
                key = keys.at(i).data();
            }
            if (status.IsNotFound())
            {
                TRACELOG("rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                         key, status.code(), status.subcode(), status.severity(), status.ToString());
            }
            else
            {
                ERRORLOG("rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                         key, status.code(), status.subcode(), status.severity(), status.ToString());
            }
        }
    }
    return flag;
}

bool RocksDBReader::ReadData(const std::string &key, std::string &value, rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    ret_status = rocksdb_->db_->Get(read_options_, key, &value);
    if (ret_status.ok())
    {
        return true;
    }
    if (ret_status.IsNotFound())
    {
        TRACELOG("rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    else
    {
        ERRORLOG("rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return false;
}

RocksDBReadWriter::RocksDBReadWriter(std::shared_ptr<RocksDB> rocksdb, const std::string &txn_name)
{
    txn_ = nullptr;
    rocksdb_ = rocksdb;
    txn_name_ = txn_name;
}

RocksDBReadWriter::~RocksDBReadWriter()
{
}
bool RocksDBReadWriter::TransactionInit()
{
    txn_ = rocksdb_->db_->BeginTransaction(write_options_);
    if (nullptr == txn_)
    {
        ERRORLOG("{} rocksdb begintransaction fail", txn_name_);
        return false;
    }
    return true;
}

bool RocksDBReadWriter::TransactionCommit(rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    ret_status = txn_->Commit();
    if (!ret_status.ok())
    {
        ERRORLOG("{} transction commit failed code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
        return false;
    }
    delete txn_;
    txn_ = nullptr;
    return true;
}
bool RocksDBReadWriter::TransactionRollBack(rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    auto status = txn_->Rollback();
    if (!status.ok())
    {
        ERRORLOG("{} transction rollback failed code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, status.code(), status.subcode(), status.severity(), status.ToString());
        return false;
    }
    delete txn_;
    txn_ = nullptr;
    return true;
}
bool RocksDBReadWriter::MultiReadData(const std::vector<rocksdb::Slice> &keys, std::vector<std::string> &values, std::vector<rocksdb::Status> &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status.push_back(rocksdb::Status::Aborted());
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status.push_back(rocksdb::Status::Aborted());
        return false;
    }
    if (keys.empty())
    {
        ERRORLOG("key is empty");
        ret_status.push_back(rocksdb::Status::Aborted());
        return false;
    }
    ret_status.clear();
    ret_status = txn_->MultiGet(read_options_, keys, &values);
    bool flag = true;
    for (size_t i = 0; i < ret_status.size(); ++i)
    {
        auto status = ret_status.at(i);
        if (!status.ok())
        {
            flag = false;
            std::string key;
            if (keys.size() > i)
            {
                key = keys.at(i).data();
            }
            if (status.IsNotFound())
            {
                TRACELOG("{} rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                         txn_name_, key, status.code(), status.subcode(), status.severity(), status.ToString());
            }
            else
            {
                ERRORLOG("{} rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                         txn_name_, key, status.code(), status.subcode(), status.severity(), status.ToString());
            }
        }
    }
    return flag;
}

bool RocksDBReadWriter::MergeValue(const std::string &key, const std::string &value, rocksdb::Status &ret_status, bool first_or_last)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (value.empty())
    {
        ERRORLOG("value is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    std::string ret_value;
    if (ReadForUpdate(key, ret_value, ret_status))
    {
        std::vector<std::string> split_values = Split(ret_value, '_');
        auto it = std::find(split_values.begin(), split_values.end(), value);
        if (split_values.end() == it)
        {
            if (first_or_last)
            {
                std::vector<std::string> tmp_values;
                tmp_values.push_back(value);
                tmp_values.insert(tmp_values.end(), split_values.begin(), split_values.end());
                split_values.swap(tmp_values);
            }
            else
            {
                split_values.push_back(value);
            }
        }
        ret_value.clear();
        for (auto split_value : split_values)
        {
            if (split_value.empty())
            {
                continue;
            }
            ret_value += split_value;
            ret_value += "_";
        }
        return WriteData(key, ret_value, ret_status);
    }
    else
    {
        if (ret_status.IsNotFound())
        {
            return WriteData(key, value, ret_status);
        }
    }
    return false;
}

bool RocksDBReadWriter::RemoveMergeValue(const std::string &key, const std::string &value, rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (value.empty())
    {
        ERRORLOG("value is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    std::string ret_value;
    if (ReadForUpdate(key, ret_value, ret_status))
    {
        std::vector<std::string> split_values = Split(ret_value, '_');
        for (;;)
        {
            auto it = std::find(split_values.begin(), split_values.end(), value);
            if (it == split_values.begin())
            {
                break;
            }
            split_values.erase(it);
        }
        ret_value.clear();
        for (auto split_value : split_values)
        {
            if (split_value.empty())
            {
                continue;
            }
            ret_value += split_value;
            ret_value += "_";
        }
        if (ret_value.empty())
        {
            return DeleteData(key, ret_status);
        }
        return WriteData(key, ret_value, ret_status);
    }
    else
    {
        if (ret_status.IsNotFound())
        {
            return true;
        }
    }
    return false;
}
bool RocksDBReadWriter::ReadData(const std::string &key, std::string &value, rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    ret_status = txn_->Get(read_options_, key, &value);
    if (ret_status.ok())
    {
        return true;
    }
    if (ret_status.IsNotFound())
    {
        TRACELOG("{} rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    else
    {
        ERRORLOG("{} rocksdb ReadData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return false;
}

bool RocksDBReadWriter::WriteData(const std::string &key, const std::string &value, rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (value.empty())
    {
        ERRORLOG("value is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    ret_status = txn_->Put(key, value);
    if (ret_status.ok())
    {
        return true;
    }
    if (ret_status.IsNotFound())
    {
        TRACELOG("{} rocksdb WriteData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    else
    {
        ERRORLOG("{} rocksdb WriteData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return false;
}
bool RocksDBReadWriter::DeleteData(const std::string &key, rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    ret_status = txn_->Delete(key);
    if (ret_status.ok())
    {
        return true;
    }
    if (ret_status.IsNotFound())
    {
        TRACELOG("{} rocksdb DeleteData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    else
    {
        ERRORLOG("{} rocksdb DeleteData failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return false;
}
bool RocksDBReadWriter::ReadForUpdate(const std::string &key, std::string &value, rocksdb::Status &ret_status)
{
    if (!rocksdb_->IsInitSuccess())
    {
        ERRORLOG("rocksdb not init");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (nullptr == txn_)
    {
        ERRORLOG("transaction is null");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    if (key.empty())
    {
        ERRORLOG("key is empty");
        ret_status = rocksdb::Status::Aborted();
        return false;
    }
    ret_status = txn_->GetForUpdate(read_options_, key, &value);
    if (ret_status.ok())
    {
        return true;
    }
    if (ret_status.IsNotFound())
    {
        TRACELOG("{} rocksdb ReadForUpdate failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    else
    {
        ERRORLOG("{} rocksdb ReadForUpdate failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 txn_name_, key, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return false;
}
