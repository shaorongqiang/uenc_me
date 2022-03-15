#include "db/db_api.h"
#include "common/config.h"
#include "common/logging.h"
#include "utils/magicsingleton.hpp"
#include "utils/singleton.hpp"

const std::string kNodeHeightKey = "NodeHeight_";
const std::string kPledgeAddrKey = "PledgeAddr_";
const std::string kAddrUtxoKey = "AddrUtxo_";
const std::string kHeight2BlockHashKey = "Height2BlockHash_";
const std::string kBlockHash2BlcokRawKey = "BlockHash2BlockRaw_";
const std::string kTxHash2BlockHashKey = "TxHash2BlockHash_";
const std::string kUsePledeUtxo2TxHashKey = "UsePledgeUtxo2TxHash_";
const std::string kUseUtxo2TxHashKey = "UseUtxo2TxHash_";
const std::string kAwardTotalKey = "AwardTotal_";

bool DBInit()
{
    rocksdb::Status ret_status;
    MagicSingleton<RocksDB>::GetInstance()->SetDBPath(Singleton<Config>::instance()->db_path());
    if (!MagicSingleton<RocksDB>::GetInstance()->InitDB(ret_status))
    {
        ERRORLOG("rocksdb init fail {}", ret_status.ToString());
        return false;
    }
    return true;
}

void DBDestory()
{
    MagicSingleton<RocksDB>::GetInstance()->DestoryDB();
    MagicSingleton<RocksDB>::DesInstance();
}

DBReader::DBReader() : db_reader_(MagicSingleton<RocksDB>::GetInstance())
{
}

//获取最高块
DBStatus DBReader::GetNodeHeight(uint64_t &node_height)
{
    node_height = 0;
    std::string value;
    auto ret = ReadData(kNodeHeightKey, value);
    if (DBStatus::DB_SUCCESS == ret)
    {
        node_height = std::stoull(value);
    }
    return ret;
}

DBStatus DBReader::MultiReadData(const std::vector<std::string> &keys, std::vector<std::string> &values)
{
    if (keys.empty())
    {
        return DBStatus::DB_PARAM_NULL;
    }
    std::vector<rocksdb::Slice> db_keys;
    std::string value;
    for (auto key : keys)
    {
        db_keys.push_back(rocksdb::Slice(key));
    }
    std::vector<rocksdb::Status> ret_status;
    if (db_reader_.MultiReadData(db_keys, values, ret_status))
    {
        if (db_keys.size() != values.size())
        {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::DB_SUCCESS;
    }
    else
    {
        for (auto status : ret_status)
        {
            if (!status.ok())
            {
                continue;
            }
            if (status.IsNotFound())
            {
                return DBStatus::DB_NOT_FOUND;
            }
            else
            {
                if (status.code() == rocksdb::Status::Code::kIOError)
                {
                    DBDestory();
                    exit(-1);
                }
            }
        }
    }
    return DBStatus::DB_ERROR;
}

DBStatus DBReader::ReadData(const std::string &key, std::string &value)
{
    value.clear();
    if (key.empty())
    {
        return DBStatus::DB_PARAM_NULL;
    }
    rocksdb::Status ret_status;
    if (db_reader_.ReadData(key, value, ret_status))
    {
        return DBStatus::DB_SUCCESS;
    }
    else if (ret_status.IsNotFound())
    {
        value.clear();
        return DBStatus::DB_NOT_FOUND;
    }
    else
    {
        if (ret_status.code() == rocksdb::Status::Code::kIOError)
        {
            DBDestory();
            exit(-1);
        }
    }
    return DBStatus::DB_ERROR;
}

DBReadWriter::DBReadWriter(const std::string &txn_name) : db_read_writer_(MagicSingleton<RocksDB>::GetInstance(), txn_name)
{
    auto_oper_trans = true;
    db_read_writer_.TransactionInit();
}

DBReadWriter::~DBReadWriter()
{
    if (auto_oper_trans)
    {
        rocksdb::Status ret_status;
        db_read_writer_.TransactionRollBack(ret_status);
        ERRORLOG("TransactionRollBack code:({}),subcode:({}),severity:({}),info:({})",
                 ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    }
}

DBStatus DBReadWriter::ReTransactionInit()
{
    auto ret = TransactionRollBack();
    if (DBStatus::DB_SUCCESS != ret)
    {
        return ret;
    }
    auto_oper_trans = true;
    if (!db_read_writer_.TransactionInit())
    {
        ERRORLOG("transction init error");
        return DBStatus::DB_ERROR;
    }
    return DBStatus::DB_SUCCESS;
}

DBStatus DBReadWriter::TransactionCommit()
{
    rocksdb::Status ret_status;
    if (db_read_writer_.TransactionCommit(ret_status))
    {
        auto_oper_trans = false;
        return DBStatus::DB_SUCCESS;
    }
    ERRORLOG("TransactionCommit faild: code:({}),subcode:({}),severity:({}),info:({})",
             ret_status.code(), ret_status.subcode(), ret_status.severity(), ret_status.ToString());
    return DBStatus::DB_ERROR;
}

DBStatus DBReadWriter::SetNodeHeight(uint64_t node_height)
{
    return WriteData(kNodeHeightKey, std::to_string(node_height));
}

DBStatus DBReadWriter::TransactionRollBack()
{
    if (auto_oper_trans)
    {
        rocksdb::Status ret_status;
        if (!db_read_writer_.TransactionRollBack(ret_status))
        {
            ERRORLOG("transction rollback code:{} info:{}", ret_status.code(), ret_status.ToString());
            return DBStatus::DB_ERROR;
        }
    }
    return DBStatus::DB_SUCCESS;
}

DBStatus DBReadWriter::MultiReadData(const std::vector<std::string> &keys, std::vector<std::string> &values)
{
    if (keys.empty())
    {
        return DBStatus::DB_PARAM_NULL;
    }
    std::vector<rocksdb::Slice> db_keys;
    for (auto key : keys)
    {
        db_keys.push_back(key);
    }
    std::vector<rocksdb::Status> ret_status;
    if (db_read_writer_.MultiReadData(db_keys, values, ret_status))
    {
        if (db_keys.size() != values.size())
        {
            return DBStatus::DB_ERROR;
        }
        return DBStatus::DB_SUCCESS;
    }
    else
    {
        for (auto status : ret_status)
        {
            if (!status.ok())
            {
                continue;
            }
            if (status.IsNotFound())
            {
                return DBStatus::DB_NOT_FOUND;
            }
            else
            {
                if (status.code() == rocksdb::Status::Code::kIOError)
                {
                    DBDestory();
                    exit(-1);
                }
            }
        }
    }
    return DBStatus::DB_ERROR;
}

DBStatus DBReadWriter::ReadData(const std::string &key, std::string &value)
{
    value.clear();
    if (key.empty())
    {
        return DBStatus::DB_PARAM_NULL;
    }
    rocksdb::Status ret_status;
    if (db_read_writer_.ReadData(key, value, ret_status))
    {
        return DBStatus::DB_SUCCESS;
    }
    else if (ret_status.IsNotFound())
    {
        value.clear();
        return DBStatus::DB_NOT_FOUND;
    }
    else
    {
        if (ret_status.code() == rocksdb::Status::Code::kIOError)
        {
            DBDestory();
            exit(-1);
        }
    }
    return DBStatus::DB_ERROR;
}

DBStatus DBReadWriter::MergeValue(const std::string &key, const std::string &value, bool first_or_last)
{
    rocksdb::Status ret_status;
    if (db_read_writer_.MergeValue(key, value, ret_status, first_or_last))
    {
        return DBStatus::DB_SUCCESS;
    }
    else
    {
        if (ret_status.code() == rocksdb::Status::Code::kIOError)
        {
            DBDestory();
            exit(-1);
        }
    }
    return DBStatus::DB_ERROR;
}

DBStatus DBReadWriter::RemoveMergeValue(const std::string &key, const std::string &value)
{
    rocksdb::Status ret_status;
    if (db_read_writer_.RemoveMergeValue(key, value, ret_status))
    {
        return DBStatus::DB_SUCCESS;
    }
    else
    {
        if (ret_status.code() == rocksdb::Status::Code::kIOError)
        {
            DBDestory();
            exit(-1);
        }
    }
    return DBStatus::DB_ERROR;
}

DBStatus DBReadWriter::WriteData(const std::string &key, const std::string &value)
{
    rocksdb::Status ret_status;
    if (db_read_writer_.WriteData(key, value, ret_status))
    {
        return DBStatus::DB_SUCCESS;
    }
    return DBStatus::DB_ERROR;
}

DBStatus DBReadWriter::DeleteData(const std::string &key)
{
    rocksdb::Status ret_status;
    if (db_read_writer_.DeleteData(key, ret_status))
    {
        return DBStatus::DB_SUCCESS;
    }
    return DBStatus::DB_ERROR;
}
