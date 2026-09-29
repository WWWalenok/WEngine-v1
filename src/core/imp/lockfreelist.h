#pragma once


#ifndef LOCKFREELIST_H
#define LOCKFREELIST_H
#include <vector>
#include <mutex>
#include <fstream>
#include "spinlocker.h"
#include "../WLoger/include/WLoger.h"
constexpr size_t __s = sizeof(std::atomic_bool);

template<typename T>
// typedef int T;
class LockFreeList
{
    static std::atomic<size_t>& ListItemBaseCounter(){ static std::atomic<size_t> _; return _; }
    struct ListItemBase : public SpinLocker
    {
    private:
        unsigned int magic = 0x4C464C49;
        friend class LockFreeList;
        std::atomic_bool need_delete = false;
        std::atomic_bool need_remove = false;
        std::atomic_bool ready_remove = false;
        std::atomic_int used = 0;

        std::atomic<ListItemBase *> m_next = nullptr;
        std::atomic<ListItemBase *> m_next_del = nullptr;

        ListItemBase *Next()
        {
            return m_next.load();
        }

        ListItemBase *NextDel()
        {
            return m_next_del.load();
        }
        
        void SetDeleted()
        {
            need_delete.store(true);
        }

        void SetNeedRemove()
        {
            need_remove.store(true);
        }

        void SetReadyRemove()
        {
            ready_remove.store(true);
        }
    public:
        ListItemBase() : SpinLocker(), m_next(nullptr), m_next_del(nullptr)
        {
            ListItemBaseCounter()++;
        }

        bool ReadyRemove()
        {
            return ready_remove.load();
        }

        bool NeedRemove()
        {
            return need_remove.load();
        }

        bool Deleted()
        {
            if (magic != 0x4C464C49)
                return true;
            return need_delete.load();
        }

        bool Valid()
        {
            return magic == 0x4C464C49 && !ready_remove.load() && !need_delete.load();
        }

        ~ListItemBase()
        {
            ListItemBaseCounter()--;
            if (magic != 0x4C464C49)
                printf("wtf\n");
        }
    };

    bool __get_next_and_lock__(ListItemBase* cur, ListItemBase*& next)
    {
        cur->used.fetch_add(1);
        next = cur->m_next.load();
        while(next)
        {
            next->used.fetch_add(1);
            if(next->try_lock())
            {
                next->used.fetch_sub(1);
                if(next->NeedRemove())
                {
                    if(m_root.try_lock())
                    {
                        ListItemBase* old = next;
                        ListItemBase* expected = next;
                        ListItemBase* desired = next->m_next.load();
                        if (cur->m_next.compare_exchange_strong(expected, desired))
                        {
                            if(!old->ready_remove.load())
                            {
                                m_list_size.fetch_sub(1);
                                m_delete_list_size.fetch_add(1);
                                old->m_next_del.store(m_root.m_next_del.load());
                                m_root.m_next_del.store(old);
                                old->m_next.store(nullptr);
                                old->ready_remove.store(true);
                            }
                            next = desired;
                            old->unlock();
                        }
                        else
                        {
                            next = cur->m_next.load();
                            old->unlock();
                        }
                        m_root.unlock();
                    }
                    else
                    {
                        next->unlock();
                        if(!next->ReadyRemove())
                        {
                            cur->used.fetch_sub(1);
                            cur = next;
                            cur->used.fetch_add(1);
                        }
                        next = cur->m_next.load();
                    }
                }
                else
                {
                    cur->used.fetch_sub(1);
                    return true;
                }
            }
            else
            {
                cur->used.fetch_sub(1);
                cur = next;
                next = cur->m_next.load();
            }
        }
        cur->used.fetch_sub(1);
        return false;
    }

public:

    struct ListItem : public ListItemBase
    {
    private:
        friend class LockFreeList;
    public:
        ListItem(T val) : ListItemBase(), val(val) {}

        T val;

        operator T &()
        {
            return val;
        }
        operator const T &() const
        {
            return val;
        }
    };

    struct Iterator
    {
        Iterator(LockFreeList *p, ListItem *_) : val(_), p(p) { };
        LockFreeList *p;
        ~Iterator()
        {
            if (val)
            {
                val->unlock();
            }
        }
        ListItem *val;
        Iterator &operator++()
        {
            ListItemBase* out;
            p->__get_next_and_lock__(val, out);
            val->unlock();
            val = static_cast<ListItem*>(out);
            return *this;
        }
        bool operator!=(const Iterator &other) const
        {
            return val != other.val;
        }
        bool operator==(const Iterator &other) const
        {
            return val == other.val;
        }
        T &operator*()
        {
            return val->val;
        }
        const T &operator*() const
        {
            return val->val;
        }
    };

    LockFreeList()
    {
    }

    LockFreeList(T first)
    {
        if (first)
            AddNode(first);
    }

    LockFreeList(std::initializer_list<T> in)
    {
        for (auto &i : in)
            AddNode(i);
    }

    ~LockFreeList()
    {
        m_root.lock();
        m_delete_root.lock();

        Clear();
        size_t i = 0;
        ListItemBase* cur = m_root.m_next;

        m_delete_root.unlock();
        m_root.unlock();
        while(cur)
        {
            ++i;
            cur = cur->m_next.load();
        }
        if(i)
            printf("%llu\n", i);
        cur = m_root.m_next_del;
        i = 0;
        while(cur)
        {
            ++i;
            cur = cur->m_next_del.load();
        }
        if(i)
            printf("%llu\n", i);
    }

    ListItem *AddNode(T n)
    {
        ListItem *node = new ListItem(n);
        m_root.lock();
        m_list_size.fetch_add(1);
        node->m_next.store(m_root.m_next.load());
        m_root.m_next.store(node);
        m_root.unlock();
        return node;
    }

    std::vector<ListItem *> AddNodes(std::initializer_list<T> ns)
    {
        std::vector<ListItem *> nodes;
        nodes.reserve(ns.size());
        for (const auto &n : ns)
            nodes.push_back(new ListItem(n));
        list_locker.lock(1);
        for (auto node : nodes)
        {
            if (m_last)
            {
                node->SetPrew(m_last);
                m_last->SetNext(node);
                m_last = node;
            }
            else
            {
                m_last = m_first = node;
            }
        }
        list_locker.unlock();
        return nodes;
    }

    bool RemoveNode(T n)
    {
        ListItem *c = m_first;
        list_locker.lock(1);
        while (c)
        {
            if (n == c->val)
            {
                list_locker.unlock();
                return RemoveNode(c);
            }
            c = c->Next();
        }

        list_locker.unlock();
        return false;
    };

    bool RemoveNode(ListItem *&n)
    {
        if (!n)
            return false;
        n->SetNeedRemove();
        return true;
    };

    bool Clear()
    {
        auto& Counter = ListItemBaseCounter();
        m_root.lock();
        m_delete_root.lock();
        ListItemBase* cur = m_root.m_next;
        while(cur)
        {
            cur->SetNeedRemove();
            cur = cur->m_next.load();
        }
        ListItemBase* out;
        __get_next_and_lock__(&m_root, out);

        CearDeleted();
        Free();
        m_delete_root.unlock();
        m_root.unlock();
        return true;
    };

    Iterator begin()
    {
        ListItemBase* out;
        if(__get_next_and_lock__(&m_root, out))
            return Iterator(this, static_cast<ListItem*>(out));
        return Iterator(this, nullptr);
    }

    Iterator end()
    {
        return Iterator(this, nullptr);
    }

    void Free()
    {
        while (m_delete_root.try_lock())
        {
            auto l_delete_list_size = m_delete_list_size.load();
            auto l_list_size = m_list_size.load();

            if((m_to_free_list_size * 10 < l_delete_list_size || m_to_free_list_size <= 64) && l_list_size > 0)
            {
                m_delete_root.unlock();
                return;
            }
            ListItem* m_to_free_list = (ListItem*)m_delete_root.m_next.load();
            if (m_to_free_list && m_to_free_list->used.load() == 0 && m_to_free_list->Deleted() && m_to_free_list->try_lock())
            {
                m_delete_root.m_next.store(m_to_free_list->m_next_del);
                --m_to_free_list_size;
                m_to_free_list->m_next.store(nullptr);
                m_to_free_list->m_next_del.store(nullptr);

                m_to_free_list->unlock();
                delete m_to_free_list;
                m_to_free_list = (ListItem*)m_delete_root.m_next.load();
                if(m_to_free_list == nullptr)
                    m_delete_root.m_next_del.store(nullptr);
                m_delete_root.unlock();
            }
            else
            {
                m_delete_root.unlock();
                return;
            }
        }
    }

    void CearDeleted()
    {
        auto root_lock = m_root.try_lock_guard();
        auto delete_lock = m_delete_root.try_lock_guard();
        if(!root_lock || !delete_lock)
            return;
        
        auto l_delete_list_size = (std::min)(m_delete_list_size.load() * 95, m_list_size.load() * 100);
        while (l_delete_list_size < m_delete_list_size.load() * 100)
        {
            ListItemBase * m_delete_list = m_root.m_next_del.load();
            if (m_delete_list && m_delete_list->used.load() == 0 && m_delete_list->ReadyRemove() && m_delete_list->try_lock())
            {
                ListItemBase *s = m_delete_list;
                m_delete_list = m_delete_list->m_next_del.load();
                m_root.m_next_del.store(m_delete_list);
                m_delete_list_size.fetch_sub(1);

                s->m_next_del.store(nullptr);
                s->SetDeleted();

                ++m_to_free_list_size;
                auto tail = m_delete_root.m_next_del.load();
                if(tail)
                {
                    tail->m_next_del.store(s);
                }
                else
                {
                    m_delete_root.m_next.store(s);
                }
                m_delete_root.m_next_del.store(s);

                s->unlock();
            }
            else
            {
                return;
            }
        }
    }

    long long Size() const{ 
        return m_list_size.load();
    }

private:

    std::atomic<long long> m_list_size = 0;
    std::atomic<long long> m_delete_list_size = 0;
    long long m_to_free_list_size = 0;

    ListItemBase m_root = ListItemBase();
    ListItemBase m_delete_root = ListItemBase();
};


#endif // LOCKFREELIST_H
