#pragma once


#include <type_traits>

class ISystem
{
public:
    template<typename T, typename Check = std::enable_if_t<std::is_base_of_v<ISystem, T>>>
    static T* Get()
    {
        static T _;
        return &_;
    }
};

#define DECLARE_SYSTEM(T) public: static T* Get() { return ISystem::Get<T>(); } friend class ISystem;