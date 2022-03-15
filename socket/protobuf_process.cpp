#include "protobuf_process.h"
#include "socket_api.h"
#include <functional>

void ProtobufProcess::AddProcessData(const MsgData &msg)
{
    {
        std::lock_guard<std::mutex> lck(process_mutex_);
        process_queue_.push(std::move(msg));
    }
    if (!process_queue_.empty())
    {
        process_condition_.notify_all();
    }
}

void ProtobufProcess::AddProcessData(const std::vector<MsgData> &msgs)
{
    {
        std::lock_guard<std::mutex> lck(process_mutex_);
        for (auto &msg : msgs)
        {
            process_queue_.push(msg);
        }
    }
    if (!process_queue_.empty())
    {
        process_condition_.notify_all();
    }
}

void ProtobufProcess::ThreadStart(std::uint32_t thread_num)
{
    continue_wait_ = true;
    for (size_t i = 0; i < thread_num; ++i)
    {
        threads_.push_back(std::thread(std::bind(&ProtobufProcess::ThreadWork, this)));
        threads_.back().detach();
    }
}
void ProtobufProcess::ThreadWork()
{
    pthread_setname_np(pthread_self(), "uenc_work");
    MsgData msg;
    while (continue_wait_)
    {
        std::unique_lock<std::mutex> process_locker(process_mutex_);
        while (process_queue_.empty())
        {
            if (!continue_wait_)
            {
                return;
            }
            process_condition_.wait(process_locker);
            if (!continue_wait_)
            {
                return;
            }
        }
        msg = std::move(process_queue_.top());
        process_queue_.pop();
        process_locker.unlock();
        Handle(msg);
        msg.Clear();
    }
}

void ProtobufProcess::ThreadStop()
{
    continue_wait_ = false;
    process_condition_.notify_all();
}

int ProtobufProcess::Handle(const MsgData &msg)
{
    if(nullptr == msg.msg)
    {
        return -1;
    }
    std::string name = msg.msg->GetDescriptor()->name();
    auto it = protocbs_.find(name);
    if (it != protocbs_.end())
    {
        return it->second(msg.msg, msg.connection);
    }
    else
    {
        return -2;
    }
}
