
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

#include <memory>

template<typename T>
std::string pointer_to_string(const T* v)
{
    if(v)
        return std::to_string(*v);
    return "nullptr";
}

void radix_sotr(void* _dst, void* _src, unsigned int C, unsigned int N)
{
    unsigned char* dst = (unsigned char*)_dst;
    unsigned char* src = (unsigned char*)_src;
    unsigned char* buffer = (unsigned char*)malloc(C * N);

    unsigned char* buff[2] = {
        dst,
        buffer
    };

    unsigned int counter[256];
    unsigned int i;
    unsigned int m;
    unsigned int k = 0;
    unsigned int l = 1;

    for (i = 0; i < sizeof(counter) / sizeof(counter[0]); i++)
        counter[i] = 0;
    
    for (i = 0; i < C; i++)
        counter[src[i * N]]++;

    for (i = 1; i < sizeof(counter) / sizeof(counter[0]); i++)
        counter[i] += counter[i - 1];

    for (i = C - 1; i < C; i--) {
        const unsigned char j = src[i * N];
        memcpy(&dst[(counter[j] - 1) * N], &src[i * N], N);
        counter[j]--;
    }

    for(unsigned int n = 1; n < N; ++n)
    {
        for (i = 0; i < sizeof(counter) / sizeof(counter[0]); i++)
            counter[i] = 0;

        for (i = 0; i < C; i++)
            counter[buff[k][i * N + n]]++;

        for (i = 1; i < sizeof(counter) / sizeof(counter[0]); i++)
            counter[i] += counter[i - 1];

        for (i = C - 1; i < C; i--) {
            const unsigned char j = buff[k][i * N + n];
            memcpy(&buff[l][(counter[j] - 1) * N], &buff[k][i * N], N);
            counter[j]--;
        }
        k = k ^ 1;
        l = l ^ 1;
    }

    if(dst != buff[k])
    {
        memcpy(dst, buff[k], N * C);
    }

    free(buffer);

}

#pragma test
int main()
{

    std::vector<unsigned int> test(16551); 
    std::vector<unsigned int> test2(test.size());
    auto seed = time(nullptr) ^ (size_t)((void*)&test) ^ *(size_t*)(printf);
    printf("0x%08llX\n", seed);
    srand(seed);
    for(int i = 0; i < test.size(); ++i)
    {
        test[i] = rand();
    }

    radix_sotr(test2.data(), test.data(), test.size(), 4);

    for(int i = 0; i < test2.size() && i < 500; ++i)
    {
        std::cout << int(test2[i]) << " ";
    }

    return 0;

#define print(X) std::cout << #X ": " << (int*)X << " ==> " << pointer_to_string((int*)X) << std::endl
    int* ptr;
    WeakRef<int> b;
    {
        ptr = new int(1);
        Ref<int> a(ptr);
        b = a;

        print(ptr);
        print(b);
        print(a);
        std::cout << "----------------\n";
    }
    

    print(ptr);
    print(b);
    std::cout << "----------------\n";
    Ref<int> a0;
    {
        Data data = int(999);
        Ref<int> a = data;
        Ref<float> a1 = data;
        b = a;
        a0 = b.lock();

        print(ptr);
        print(b);
        print(a);
        print(a0);
        std::cout << "a1" ": " << (float*)a1 << " ==> " << pointer_to_string((float*)a1) << std::endl;
        std::cout << "----------------\n";
    }

    print(ptr);
    print(b);
    print(a0);
    std::cout << "----------------\n";
}










