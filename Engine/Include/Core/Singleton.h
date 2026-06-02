#pragma once

namespace Yogi
{

template <typename T>
class YG_API Singleton
{
public:
    static bool IsInitialized() { return T::s_instance != nullptr; }

    static void Shutdown()
    {
        delete T::s_instance;
        T::s_instance = nullptr;
    }

    Singleton(const Singleton&)            = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton()  = default;
    ~Singleton() = default;

    static T& Get()
    {
        if (!T::s_instance)
            T::s_instance = new T();
        return *T::s_instance;
    }
};

} // namespace Yogi
