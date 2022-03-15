#ifndef UENC_DB_ROCKSDB_READ_ONLY_H_
#define UENC_DB_ROCKSDB_READ_ONLY_H_

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include <mutex>

class ReadOnlyBackgroundErrorListener : public rocksdb::EventListener
{
public:
    void OnBackgroundError(rocksdb::BackgroundErrorReason reason, rocksdb::Status* bg_error) override;
};

class RocksDBReadOnly
{
public:
    RocksDBReadOnly();
    ~RocksDBReadOnly();
    RocksDBReadOnly(RocksDBReadOnly &&) = delete;
    RocksDBReadOnly(const RocksDBReadOnly &) = delete;
    RocksDBReadOnly &operator=(RocksDBReadOnly &&) = delete;
    RocksDBReadOnly &operator=(const RocksDBReadOnly &) = delete;

    void SetDBPath(const std::string &db_path);
    bool InitDB(rocksdb::Status &ret_status);
    void DestoryDB();
    bool IsInitSuccess();

    bool MultiReadData(const std::vector<rocksdb::Slice> &keys, std::vector<std::string> &values, std::vector<rocksdb::Status> &ret_status);
    bool ReadData(const std::string &key, std::string &value, rocksdb::Status &ret_status);

private:
    rocksdb::ReadOptions read_options_;
    rocksdb::DB *db_;
    std::string db_path_;
    std::mutex is_init_success_mutex_;
    bool is_init_success_;
};

#endif 
