#ifndef UENC_SOCKET_PROTOBUF_PROCESS_H_
#define UENC_SOCKET_PROTOBUF_PROCESS_H_

#include "define.h"
#include "socket/socket_manager.h"
#include <condition_variable>
#include <functional>
#include <google/protobuf/message.h>
#include <mutex>
#include <queue>
#include <thread>

struct MsgData
{
    Priority priority;
    std::shared_ptr<google::protobuf::Message> msg;
    std::shared_ptr<SocketConnection> connection;

    void Clear()
    {
        priority = Priority::kPriority_Low_0;
        msg.reset();
        connection.reset();
    }
    MsgData()
    {
        Clear();
    }
    bool operator<(const MsgData &msg_data) const
    {
        return priority < msg_data.priority;
    }
};

class ProtobufProcess
{
public:
    ProtobufProcess() = default;
    ~ProtobufProcess() = default;
    ProtobufProcess(ProtobufProcess &&) = delete;
    ProtobufProcess(const ProtobufProcess &) = delete;
    ProtobufProcess &operator=(ProtobufProcess &&) = delete;
    ProtobufProcess &operator=(const ProtobufProcess &) = delete;

    void AddProcessData(const MsgData &msg);
    void AddProcessData(const std::vector<MsgData> &msgs);

    void ThreadStart(std::uint32_t thread_num);
    void ThreadWork();
    void ThreadStop();

    int Handle(const MsgData &data);

    template <typename T>
    void RegisterCallback(std::function<int(const std::shared_ptr<T> &, std::shared_ptr<SocketConnection>)> cb)
    {
        protocbs_[T::descriptor()->name()] = [cb](const std::shared_ptr<google::protobuf::Message> &msg, std::shared_ptr<SocketConnection> connection)
        {
            return cb(std::static_pointer_cast<T>(msg), connection);
        };
    }

private:
    std::map<const std::string, std::function<int(const std::shared_ptr<google::protobuf::Message> &, std::shared_ptr<SocketConnection>)>> protocbs_;

    std::vector<std::thread> threads_;
    bool continue_wait_;
    std::mutex process_mutex_;
    std::condition_variable process_condition_;
    std::priority_queue<MsgData> process_queue_;
};

#endif
