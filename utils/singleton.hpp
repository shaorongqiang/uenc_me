#ifndef UENC_UTILS_SINGLETON_HPP_
#define UENC_UTILS_SINGLETON_HPP_

template <typename T>
class Singleton
{
private:
    static T *instance_;
    Singleton() {}

public:
    static T *instance() { return instance_; }
};

template <typename T>
T *Singleton<T>::instance_ = new T;

#endif
