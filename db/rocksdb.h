#ifndef UENC_DB_ROCKSDB_H_
#define UENC_DB_ROCKSDB_H_

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include <mutex>

class BackgroundErrorListener : public rocksdb::EventListener
{
public:
    void OnBackgroundError(rocksdb::BackgroundErrorReason reason, rocksdb::Status* bg_error) override;
};

class RocksDBReader;
class RocksDBReadWriter;
class RocksDB
{
public:
    RocksDB();
    ~RocksDB();
    RocksDB(RocksDB &&) = delete;
    RocksDB(const RocksDB &) = delete;
    RocksDB &operator=(RocksDB &&) = delete;
    RocksDB &operator=(const RocksDB &) = delete;

    void SetDBPath(const std::string &db_path);
    bool InitDB(rocksdb::Status &ret_status);
    void DestoryDB();
    bool IsInitSuccess();

private:
    friend class RocksDBReader;
    friend class RocksDBReadWriter;
    std::string db_path_;
    rocksdb::TransactionDB *db_;
    std::mutex is_init_success_mutex_;
    bool is_init_success_;
};

class RocksDBReader
{
public:
    RocksDBReader(std::shared_ptr<RocksDB> rocksdb);
    ~RocksDBReader() = default;
    RocksDBReader(RocksDBReader &&) = delete;
    RocksDBReader(const RocksDBReader &) = delete;
    RocksDBReader &operator=(RocksDBReader &&) = delete;
    RocksDBReader &operator=(const RocksDBReader &) = delete;

    bool MultiReadData(const std::vector<rocksdb::Slice> &keys, std::vector<std::string> &values, std::vector<rocksdb::Status> &ret_status);
    bool ReadData(const std::string &key, std::string &value, rocksdb::Status &ret_status);

private:
    rocksdb::ReadOptions read_options_;
    std::shared_ptr<RocksDB> rocksdb_;
};

class RocksDBReadWriter
{
public:
    RocksDBReadWriter(std::shared_ptr<RocksDB> db, const std::string &txn_name);
    ~RocksDBReadWriter();
    RocksDBReadWriter(RocksDBReadWriter &&) = delete;
    RocksDBReadWriter(const RocksDBReadWriter &) = delete;
    RocksDBReadWriter &operator=(RocksDBReadWriter &&) = delete;
    RocksDBReadWriter &operator=(const RocksDBReadWriter &) = delete;

    bool TransactionInit();
    bool TransactionCommit(rocksdb::Status &ret_status);
    bool TransactionRollBack(rocksdb::Status &ret_status);

    bool MultiReadData(const std::vector<rocksdb::Slice> &keys, std::vector<std::string> &values, std::vector<rocksdb::Status> &ret_status);
    bool MergeValue(const std::string &key, const std::string &value, rocksdb::Status &ret_status, bool first_or_last = false);
    bool RemoveMergeValue(const std::string &key, const std::string &value, rocksdb::Status &ret_status);
    bool ReadData(const std::string &key, std::string &value, rocksdb::Status &ret_status);
    bool WriteData(const std::string &key, const std::string &value, rocksdb::Status &ret_status);
    bool DeleteData(const std::string &key, rocksdb::Status &ret_status);

private:
    bool ReadForUpdate(const std::string &key, std::string &value, rocksdb::Status &ret_status);

    std::string txn_name_;
    std::shared_ptr<RocksDB> rocksdb_;
    rocksdb::Transaction *txn_;
    rocksdb::WriteOptions write_options_;
    rocksdb::ReadOptions read_options_;
};

#endif
