#pragma once


#ifndef __SPINLOCKER_H__
#define __SPINLOCKER_H__
#include <thread>
#include <atomic>

#ifndef LOCKER_LOG
#define LOCKER_LOG(...)
#endif

struct SpinLocker {

    struct lg_t
    {
        lg_t(SpinLocker* parent) : parent(parent) {parent->lock();}
        ~lg_t() { parent->unlock(); }

    private:
        SpinLocker* parent;
    };

    SpinLocker() : tid({}) { hSpin = 0; hCount = 0; tid.ptr = 0; }
    
    bool lockedThisThread() { return tid.t_id == std::this_thread::get_id(); }

    bool try_lock() 
    {
        if(tid.t_id == std::this_thread::get_id())
        {
            ++hCount;
            onLock();
            return true;
        }
        
        int Ex = 0;
        int Del = 1;
        if (hSpin.compare_exchange_weak(Ex, Del) == false)
            return false;
        tid.t_id = std::this_thread::get_id();
        hCount = 1;
        onLock();
        return true;
    }

    void unlock() 
    {
        if(hSpin.load() == 0)
        {
            LOCKER_LOG("warning : unlock not locked item\n");
            return;
        }
        else if(tid.t_id != std::this_thread::get_id())
        {
            LOCKER_LOG("warning : unlock from other thread\n");
        }
        --hCount;
        if(hCount.load() > 0)
        {
            onUnlock();
            return;
        }
        tid.ptr = 0;
        hSpin = 0;
        hCount = 0;
        onUnlock();
    }

    void forse_unlock() 
    {
        if(hSpin.load() == 0)
        {
            LOCKER_LOG("warning : forse_unlock not locked item\n");
            return;
        }
        if(tid.t_id != std::this_thread::get_id() && tid.ptr != 0)
        {
            LOCKER_LOG("warning : forse_unlock from other thread\n");
        }
        tid.ptr = 0;
        hSpin = 0;
        hCount = 0;
        onUnlock();
    }

    void lock(int Ms = 0) 
    {
        if(tid.t_id == std::this_thread::get_id())
        {
            ++hCount;
            onLock();
            return;
        }
        for (;;) 
        {
            int Ex = 0;
            int Del = 1;
            if (hSpin.compare_exchange_weak(Ex, Del) == false) 
            {
                if(Ms)
                    std::this_thread::sleep_for(std::chrono::milliseconds(Ms));
                else
                    std::this_thread::yield();
                continue;
            }
            break;
        }
        tid.t_id = std::this_thread::get_id();
        hCount = 1;
        onLock();
    }
    
    lg_t lock_guard() { return lg_t(this); };

    bool is_locked() { return hSpin.load() == 1; }

protected:
    virtual void onLock() {}
    virtual void onUnlock() {}

private:
    
    union tid_t
    {
        std::thread::id t_id;
        void* ptr = 0;
    } tid;
    
    std::atomic<int>     hSpin;
    std::atomic<int>     hCount;
    
};

#endif // __SPINLOCKER_H__