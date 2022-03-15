#ifndef UENC_UTILS_MAGICSINGLETON_HPP_
#define UENC_UTILS_MAGICSINGLETON_HPP_

#include <memory>
#include <mutex>

template <typename T>
class MagicSingleton
{
public:
    //获取全局单例对象
    template <typename... Args>
    static std::shared_ptr<T> GetInstance(Args &&...args)
    {
        if (!instance_)
        {
            std::lock_guard<std::mutex> gLock(mutex_);
            if (nullptr == instance_)
            {
                instance_ = std::make_shared<T>(std::forward<Args>(args)...);
            }
        }
        return instance_;
    }

    //主动析构单例对象（一般不需要主动析构，除非特殊需求）
    static void DesInstance()
    {
        if (instance_)
        {
            instance_.reset();
            instance_ = nullptr;
        }
    }

private:
    explicit MagicSingleton();
    MagicSingleton(const MagicSingleton &) = delete;
    MagicSingleton &operator=(const MagicSingleton &) = delete;
    ~MagicSingleton();

private:
    static std::shared_ptr<T> instance_;
    static std::mutex mutex_;
};

template <typename T>
std::shared_ptr<T> MagicSingleton<T>::instance_ = nullptr;

template <typename T>
std::mutex MagicSingleton<T>::mutex_;

#endif // _MAGICSINGLETON_H_
