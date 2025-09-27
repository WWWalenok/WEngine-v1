#pragma once


#ifndef LOCKFREELIST_H
#define LOCKFREELIST_H
#include <vector>
#include <mutex>
#include "spinlocker.h"


template<typename T>
// typedef int T;
class LockFreeList
{
public:

    struct ListItem : public SpinLocker {
    private:
        bool need_delete = false;
        bool need_remove = false;
        bool ready_remove = false;

        SpinLocker edit_lock;

        ListItem* m_next;
        ListItem* m_next_del;
        ListItem* m_prew;

        virtual void onUnlock() override 
        {
            if(need_delete)
                delete this;
        }
        
    public:
        ListItem(T val) : SpinLocker(), val(val), m_next(nullptr), m_prew(nullptr), m_next_del(nullptr) {}

        T val;

        void SetNext(ListItem* val)
        {
            edit_lock.lock();
            m_next = val;
            edit_lock.unlock();
        }

        void SetPrew(ListItem* val)
        {
            edit_lock.lock();
            m_prew = val;
            edit_lock.unlock();
        }

        void SetNextDel(ListItem* val)
        {
            edit_lock.lock();
            m_next_del = val;
            edit_lock.unlock();
        }

        ListItem* Next()
        {
            return m_next;
        }

        ListItem* Prew()
        {
            return m_prew;
        }

        ListItem* NextDel()
        {
            return m_next_del;
        }

        operator T&()  { return val; }
        operator const T&() const { return val; }

        void SetDeleted()
        {
            edit_lock.lock();
            need_delete = true;
            edit_lock.unlock();
        }

        void SetNeedRemove()
        {
            edit_lock.lock();
            need_remove = true;
            edit_lock.unlock();
        }

        void SetReadyRemove()
        {
            edit_lock.lock();
            ready_remove = true;
            edit_lock.unlock();
        }

        bool ReadyRemove()
        {
            return ready_remove;
        }

        bool NeedRemove()
        {
            return need_remove;
        }

        bool Deleted()
        {
            return need_delete;
        }

        bool Valid()
        {
            return !ready_remove && !need_remove && !need_delete;
        }

        
    private:
        ~ListItem() { }
    };

    struct Iterator { 

        void __upd__()
        {
            while(val) { 
                while(val && (val->Deleted() || val->ReadyRemove()))
                    val = val->Next();
                if(!val)
                    return;
                if(val->try_lock()) 
                {
                    if(val->NeedRemove())
                    {
                        val->SetReadyRemove();
                        val->unlock();
                    }
                    else
                        return; 
                }
                val = val->Next(); 
            }
        }

        Iterator(LockFreeList* p, ListItem* _) : val(_), p(p) { 
            __upd__();
        };
        LockFreeList* p;
        ~Iterator() { }
        ListItem* val;
        Iterator& operator++() {
            auto old = val;
            val = val->Next();
            if(old->NeedRemove())
                old->SetReadyRemove();
            old->unlock();
            __upd__();
            return *this; 
        }
        bool operator!=(const Iterator & other) const { return val != other.val;  }
        bool operator==(const Iterator & other) const { return val == other.val;  }
        T& operator*() { return val->val; }
        const T& operator*() const { return val->val; }
    };

    LockFreeList() {

    }

    LockFreeList(T first) {
        if(first)
            AddNode(first);
    }
    
    LockFreeList(std::initializer_list<T> in) {
        for(auto& i: in)
            AddNode(i);
    }

    ~LockFreeList()
    {
        Clear();
    }

    ListItem* AddNode(T n) {
        ListItem* node = new ListItem(n);
        if(m_last)
        {
            node->SetPrew(m_last);
            m_last->SetNext(node);
            m_last = node;
        }
        else
        {
            m_last = m_first = node;
        }
        return node;
    }

    std::vector<ListItem*> AddNodes(std::initializer_list<T> ns) {
        std::vector<ListItem*> nodes;
        nodes.reserve(ns.size());
        for(const auto& n : ns)
            nodes.push_back(new ListItem(n));
        list_locker.lock(1);
        for(auto node : nodes)
        {
            if(m_last)
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

    bool RemoveNode(T n) {
        ListItem* c = m_first;
        list_locker.lock(1);
        while(c)
        {
            if(n == c->val)
            {
                list_locker.unlock();
                return RemoveNode(c);
            }
            c = c->Next();
        }

        list_locker.unlock();
        return false;
    };

    bool RemoveNode(ListItem* n) {
        if(!n)
            return false;
        list_locker.lock(1);
        n->lock();
        
        auto prew = n->Prew();
        auto next = n->Next();

        if(prew)
            prew->SetNext(next);
        if(next)
            next->SetPrew(prew);

        if(m_first == n)
            m_first = next;
        if(m_last == n)
            m_last = prew;
        n->SetNextDel(m_delete_list);
        m_delete_list = n;

        n->SetNeedRemove();

        n->unlock();
        list_locker.unlock();
        return true;
    };

    bool Clear() {
        list_locker.lock(1);
        ListItem* c = m_first;
        while(c)
        {
            auto o = c;
            c = c->Next();
            RemoveNode(o);
        }

        list_locker.unlock();

        CearDeleted();
        return true;
    };

    Iterator begin() { CearDeleted(); return Iterator(this, m_first); }
    Iterator end()   { return Iterator(this, nullptr); }

    // ListItemIteratorHelper ii = ListItemIteratorHelper(this);

private:

    void CearDeleted() {
        if(list_locker.try_lock())
        {
            while(m_delete_list && m_delete_list->ReadyRemove() && m_delete_list->try_lock())
            {
                ListItem* s = m_delete_list;
                m_delete_list = m_delete_list->NextDel();
                s->SetDeleted();
                s->unlock();
            }
            list_locker.unlock();
        }
    }

    ListItem* m_delete_list = nullptr;

    ListItem* m_first = nullptr;
    ListItem* m_last = nullptr;
    SpinLocker list_locker;
};


#endif // LOCKFREELIST_H
