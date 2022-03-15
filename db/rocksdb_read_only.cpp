#include "db/rocksdb_read_only.h"
#include "common/logging.h"
#include <dirent.h>

void ReadOnlyBackgroundErrorListener::OnBackgroundError(rocksdb::BackgroundErrorReason reason, rocksdb::Status *bg_error)
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

RocksDBReadOnly::RocksDBReadOnly()
{
    db_ = nullptr;
    std::lock_guard<std::mutex> lock(is_init_success_mutex_);
    is_init_success_ = false;
}

RocksDBReadOnly::~RocksDBReadOnly()
{
    DestoryDB();
    db_ = nullptr;
    std::lock_guard<std::mutex> lock(is_init_success_mutex_);
    is_init_success_ = false;
}

void RocksDBReadOnly::SetDBPath(const std::string &db_path)
{
    db_path_ = db_path;
}

bool RocksDBReadOnly::InitDB(rocksdb::Status &ret_status)
{
    if (is_init_success_)
    {
        return false;
    }
    rocksdb::Options options;
    options.create_if_missing = false;
    options.listeners.push_back(std::make_shared<ReadOnlyBackgroundErrorListener>());
    const std::string kSecondaryPath = db_path_ + "_read_only";
    ret_status = rocksdb::DB::OpenAsSecondary(options, db_path_, kSecondaryPath, &db_);
    if (ret_status.ok())
    {
        std::lock_guard<std::mutex> lock(is_init_success_mutex_);
        is_init_success_ = true;
    }
    else
    {
        ERRORLOG("rocksdb {} OpenAsSecondary failed code:({}),subcode:({}),severity:({}),info:({})",
                 db_path_, ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
    return is_init_success_;
}
void RocksDBReadOnly::DestoryDB()
{

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
    {
        std::lock_guard<std::mutex> lock(is_init_success_mutex_);
        is_init_success_ = false;
    }
    db_ = nullptr;
}

bool RocksDBReadOnly::MultiReadData(const std::vector<rocksdb::Slice> &keys, std::vector<std::string> &values, std::vector<rocksdb::Status> &ret_status)
{
    ret_status.clear();
    if (!is_init_success_)
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
    rocksdb::Status catch_status = db_->TryCatchUpWithPrimary();
    if (!catch_status.ok())
    {
        ERRORLOG("rocksdb TryCatchUpWithPrimary failed code:({}),subcode:({}),severity:({}),info:({})",
                 catch_status.code(), catch_status.subcode(), catch_status.severity(), catch_status.ToString());
    }

    ret_status = db_->MultiGet(read_options_, keys, &values);
    bool flag = true;
    for(size_t i = 0; i < ret_status.size(); ++i)
    {
        auto status = ret_status.at(i);
        if (!status.ok())
        {
            flag = false;
            std::string key;
            if(keys.size() > i)
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

bool RocksDBReadOnly::ReadData(const std::string &key, std::string &value, rocksdb::Status &ret_status)
{
    if (!is_init_success_)
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
    rocksdb::Status catch_status = db_->TryCatchUpWithPrimary();
    if (!catch_status.ok())
    {
        ERRORLOG("rocksdb TryCatchUpWithPrimary failed key:{} code:({}),subcode:({}),severity:({}),info:({})",
                 key, catch_status.code(), catch_status.subcode(), catch_status.severity(), catch_status.ToString());
    }

    ret_status = db_->Get(read_options_, key, &value);
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
