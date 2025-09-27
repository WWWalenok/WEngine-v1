#pragma once

#include <type_traits>
template<typename... _Signature>
struct srv_function;

constexpr static size_t srv_function_GETID(const void* v, int size = 0)
{
    unsigned int hash = 0x811c9dc5;
    unsigned int h1 = 0x959954B9;
    const char* name = (const char*)v;
    if(size == 0)
    {
        while (*name)
        {
            h1 = *name + (h1 << 5) + (h1 << 7) + (h1 << 17) - h1;
            hash ^= (unsigned char)*name++;
            hash *= 0x01000193;
        }
    }
    else
    {
        while (size)
        {
            h1 = *name + (h1 << 5) + (h1 << 7) + (h1 << 17) - h1;
            hash ^= (unsigned char)*name++;
            hash *= 0x01000193;
            --size;
        }
    }

    return (hash | (size_t(h1) << 32)) & 0xFFFFFFFFFFFFFFFEULL | 0x1ULL;
}

template<typename Ret, typename... Vars> 
struct srv_function<Ret(Vars...)>
{
    template<typename T, typename... ARGS>
    struct Callable{
        virtual size_t gid() = 0;
        virtual T doit(ARGS...) = 0;
        virtual Callable<T, ARGS...>* copy() const = 0;
    };

    template<typename Call, typename T, typename... ARGS>
    struct ImplCallable : public Callable<T, ARGS...> {
        static_assert(std::is_invocable_r<T, std::decay_t<Call>&, ARGS...>::value, "Uncorrect Caller");

        ImplCallable(Call _val) : val(_val) {}
        ~ImplCallable() {if(callable) delete callable; }
        Call val;
        virtual size_t gid()
        {
            if constexpr (std::is_pointer_v<std::decay_t<Call>>)
            {
                return srv_function_GETID(&val, sizeof(val)) | 0x1ULL;
            }
            else
            {
                static size_t stat = (size_t)(&stat);
                return srv_function_GETID(&stat, sizeof(stat)) | 0x1ULL;
            }
        };
        virtual T doit(ARGS... args) override { 
            if constexpr (sizeof...(ARGS))
            {
                if constexpr (std::is_same_v<T, void>)
                    val(args...); 
                else
                    return val(args...); 
            }
            else
            {
                if constexpr (std::is_same_v<T, void>)
                    val(); 
                else
                    return val(); 
            }
        }

        virtual Callable<T, ARGS...>* copy() const override { 
            return new ImplCallable<Call, T, ARGS...>(val);
        }
    };
    Callable<Ret, Vars...>* callable = nullptr;

    template<typename Call>
    using check_callable = std::enable_if_t<std::conjunction<
        std::is_invocable_r<Ret, std::decay_t<Call>&, Vars...>
    >::value>;

    srv_function() {
        callable = nullptr;
    }

    template<typename Call, typename Check = check_callable<Call>>
    srv_function(Call val) {
        callable = new ImplCallable<Call, Ret, Vars...>(val);
    }

    srv_function(nullptr_t) {
        callable = nullptr;
    }

    srv_function(const srv_function<Ret(Vars...)>& val) {
        callable = val.callable->copy();
    }

    size_t tid() const { return callable ? callable->gid() : 0; }

    Ret operator()(Vars... args) { 
        if constexpr (std::is_same_v<Ret, void>)
            callable->doit(args...); 
        else
            return callable->doit(args...); 
    }

    operator bool() { return callable; }
    template<typename _Fty2>
    bool operator ==(const srv_function<_Fty2>& other) { return tid() == other.tid(); }
    template<typename _Fty2>
    bool operator !=(const srv_function<_Fty2>& other) { return tid() != other.tid(); }
};