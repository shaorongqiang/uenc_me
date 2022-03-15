#ifndef UENC_DB_DB_API_H_
#define UENC_DB_DB_API_H_

#include "db/rocksdb.h"
#include <string>
#include <vector>

bool DBInit();
void DBDestory();

enum DBStatus
{
    DB_SUCCESS = 0,    //成功
    DB_ERROR = 1,      //错误
    DB_PARAM_NULL = 2, //参数不合法
    DB_NOT_FOUND = 3,  //没找到
    DB_IS_EXIST = 4
};
class DBReader
{
public:
    DBReader();
    virtual ~DBReader() = default;
    DBReader(DBReader &&) = delete;
    DBReader(const DBReader &) = delete;
    DBReader &operator=(DBReader &&) = delete;
    DBReader &operator=(const DBReader &) = delete;


    //获取节点最高高度
    DBStatus GetNodeHeight(uint64_t &node_height);

    virtual DBStatus MultiReadData(const std::vector<std::string> &keys, std::vector<std::string> &values);
    virtual DBStatus ReadData(const std::string &key, std::string &value);

private:
    RocksDBReader db_reader_;
};

class DBReadWriter : public DBReader
{
public:
    DBReadWriter(const std::string &txn_name = std::string());
    virtual ~DBReadWriter();
    DBReadWriter(DBReadWriter &&) = delete;
    DBReadWriter(const DBReadWriter &) = delete;
    DBReadWriter &operator=(DBReadWriter &&) = delete;
    DBReadWriter &operator=(const DBReadWriter &) = delete;

    DBStatus ReTransactionInit();
    DBStatus TransactionCommit();

    //设置节点最高高度
    DBStatus SetNodeHeight(uint64_t node_height);

private:
    DBStatus TransactionRollBack();
    virtual DBStatus MultiReadData(const std::vector<std::string> &keys, std::vector<std::string> &values);
    virtual DBStatus ReadData(const std::string &key, std::string &value);
    DBStatus MergeValue(const std::string &key, const std::string &value, bool first_or_last = false);
    DBStatus RemoveMergeValue(const std::string &key, const std::string &value);
    DBStatus WriteData(const std::string &key, const std::string &value);
    DBStatus DeleteData(const std::string &key);

    RocksDBReadWriter db_read_writer_;
    bool auto_oper_trans;
};

#endif
