#pragma once

#include <list>
#include <mutex>
#include <unordered_map>
static void Tester() {
    printf("");
}
#define Check(X) { if(!(X)) { printf("\n    Check: " #X " failed!\n"); Tester(); } }
class Allocator
{

    template<typename T, unsigned int Stride = ((sizeof(T) - 1) / 4 + 1) * 4, bool USE_DESTRUCTOR = true>
    class TAllocator
    {
        constexpr static uint32_t USEMASK = 0b10000000000000000000000000000000U;
        static_assert(sizeof(T) <= Stride, "Stride too small");
        struct FElement
        {
            union
            {
                unsigned char Buffer[Stride];
                T Data;
            };
            uint32_t Next;
            ~FElement()
            {
            }
        };
        static_assert(sizeof(T) <= sizeof(FElement), "FElement too small");

        struct FChunk;
        struct FChunkHeader
        {
            size_t Alloced;
            FElement* FirstFree;
            TAllocator* Parent;
            FChunk* Next;
        };

        constexpr static size_t HeaderSize = sizeof(FChunkHeader);

        struct FChunk
        {
            FChunkHeader Header;
            union
            {
                unsigned char ByteBuffer[1];
                FElement ElementBuffer[1];
            };

            FChunk() = delete;

            static FChunk* Make(size_t Count, TAllocator* Parent)
            {
                FChunk* Ptr = nullptr;
                if (sizeof(FElement) * Count > 0x8000000)
                    Count = 0x8000000 / sizeof(FElement);
                if (Count > 0)
                {
                    Ptr = (FChunk*)malloc(HeaderSize + sizeof(FElement) * Count);
                    if (Ptr)
                    {
                        for (int i = 0; i < Count; ++i)
                        {
                            Ptr->ElementBuffer[i].Next = i + 1;
                        }
                        Ptr->Header.Parent = Parent;
                        Ptr->Header.Alloced = Count;
                        Ptr->Header.FirstFree = Ptr->ElementBuffer;
                        Ptr->Header.Next = nullptr;
                        return Ptr;
                    }
                }
                return nullptr;
            }

            ~FChunk()
            {
                if (Header.Next)
                    (*Header.Next).~FChunk();
                free(this);
            }

            inline operator bool()
            {
                return Header.Alloced > 0;
            }

            inline bool IsFull()
            {
                return Header.FirstFree == nullptr;
            }

            void* Alloc()
            {
                if (Header.FirstFree == nullptr)
                    return nullptr;

                auto Return = Header.FirstFree;
                Header.FirstFree = Header.FirstFree->Next >= Header.Alloced ? nullptr : ElementBuffer + Header.FirstFree->Next;
                memset(Return, 0, sizeof(FElement));
                Return->Next = (Return - ElementBuffer) | USEMASK;
                return Return;
            }

            void Free(void* Ptr)
            {
                const size_t ByteOffset = (unsigned char*)Ptr - (unsigned char*)ElementBuffer;
                const size_t OffsetError = ByteOffset % sizeof(FElement);
                Check(OffsetError == 0);

                const size_t Id = ByteOffset / sizeof(FElement);
                Check(Id >= 0 && Id < Header.Alloced);

                ((FElement*)Ptr)->Next = Header.FirstFree - ElementBuffer;
                Header.FirstFree = ((FElement*)Ptr);
            }
        };

        FChunk* Head;

        size_t CurSize;

        std::mutex Access;

        TAllocator() : CurSize(0x1000 / sizeof(FElement))
        {
        }

    public:
        void* Alloc()
        {
            void* Ret = nullptr;
            if (!Head)
                Head = FChunk::Make(CurSize, this);
            FChunk* Cur = Head;
            for (; Cur->Header.Next != nullptr; Cur = Cur->Header.Next)
            {
                Ret = Cur->Alloc();
                if (Ret)
                    return Ret;
            }
            Ret = Cur->Alloc();
            if (Ret)
                return Ret;
            Cur->Header.Next = FChunk::Make(CurSize * 2, this);
            CurSize = Cur->Header.Next->Header.Alloced;
            return Cur->Header.Next->Alloc();
        }

        void Free(void* Ptr)
        {
            FElement* Element = (FElement*)Ptr;
            if(Element->Next & USEMASK)
            {
                FChunk* Chunk = (FChunk*)(((unsigned char*)(Element - (Element->Next ^ USEMASK))) - HeaderSize);
                Check(Chunk->Header.Parent == this);

                Chunk->Free(Ptr);
            }
            else
            {
                printf("DOUBLE FREE");
            }
        }

        inline void lock()
        {
            Access.lock();
        }

        inline void unlock()
        {
            Access.unlock();
        }

        ~TAllocator()
        {
            (*Head).~FChunk();
        }

        static TAllocator& Get()
        {
            static TAllocator Instance;
            return Instance;
        }
    };

public:
    template<typename T, unsigned int Stride = ((sizeof(T) - 1) / 4 + 1) * 4>
    static inline T* Alloc()
    {
        static auto& Static = TAllocator<T, Stride>::Get();
        Static.lock();
        auto Ret = (T*)Static.Alloc();
        Static.unlock();
        return Ret;
    }

    template<typename T, unsigned int Stride = ((sizeof(T) - 1) / 4 + 1) * 4>
    static inline void Free(T* Ptr)
    {
        if (!Ptr)
            return;

        static auto& Static = TAllocator<T, Stride>::Get();
        Static.lock();
        Static.Free(Ptr);
        Static.unlock();
    }

    template<typename T, typename... ARGS, unsigned int Stride = ((sizeof(T) - 1) / 4 + 1) * 4>
    static inline T* New(const ARGS&... Args)
    {
        T* Return = Alloc<T, Stride>();

        if (Return)
        {
            new (Return) T(Args...);
        }

        return Return;
    }

    template<typename T, unsigned int Stride = ((sizeof(T) - 1) / 4 + 1) * 4>
    static inline void Delete(T* Ptr)
    {
        if (!Ptr)
            return;

        if constexpr (std::is_destructible<T>::value)
            (*Ptr).~T();

        Free<T, Stride>(Ptr);
    }
};
#undef Check

