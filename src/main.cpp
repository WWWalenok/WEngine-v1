
#include <thread>
#include <atomic>

#include <functional>
#include <tuple>
#include <stdexcept>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

#include "core/core.h"

// auto* core = ISystem::Get<CoreSystem>();
// auto* render = ISystem::Get<RenderSystem>();

struct test
{
    int         a;
    int         b;
    std::string c;

    std::vector<std::string> d;
};

struct test2 {
    test t;
};

DECLARE_DATA_STRUCT_AND_SERIALIZE(test, a, b, c, d)

DECLARE_DATA_TEMPLATE_TYPE(srv_function, __I(typename... T), __I(T...));


int main()
{
    int a = 8;
    srv_function<int()> g = [a](){ return 1; };
    {
        std::vector<Data> t2;
        for(int i = 0; i < 100; ++i)
        {
            struct mdeallocator{ bool operator()(void*){return true;}};
            Ref<test, mdeallocator> b1 = new test;
            Ref<test2> b2 = new test2;
            Data t = g;
            if((test2*)b2)
            {
                printf("");
            }
            t2.push_back(b1);
            t2.push_back(t);

            constexpr auto gg = pipe::Helper<test>::ID();
        }
    }
}
