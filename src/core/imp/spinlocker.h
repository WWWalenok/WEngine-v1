#pragma once

#ifndef __SPINLOCKER_H__
#define __SPINLOCKER_H__

#include <thread>
#include <atomic>
#include <chrono>

#ifndef LOCKER_LOG
#define LOCKER_LOG(...)
#endif

class SpinLocker {
public:
    struct lg_t {
        explicit lg_t(SpinLocker* parent) : parent_(parent) { parent_->lock(); }
        ~lg_t() { parent_->unlock(); }
        lg_t(const lg_t&) = delete;
        lg_t& operator=(const lg_t&) = delete;
    private:
        SpinLocker* parent_;
    };

    struct tlg_t {
        explicit tlg_t(SpinLocker* parent) : parent_(parent), locked(parent_->try_lock()) {}
        ~tlg_t() { if(locked) parent_->unlock(); }
        tlg_t(const lg_t&) = delete;
        tlg_t& operator=(const lg_t&) = delete;
        operator bool() const { return locked; }
    private:
        SpinLocker* parent_;
        bool locked;
    };


    SpinLocker() = default;
    ~SpinLocker() = default;

    bool lockedThisThread() const {
        return owner_.load(std::memory_order_acquire) == std::this_thread::get_id();
    }

    // Попытка захвата без блокировки
    bool try_lock() {
        auto current_id = std::this_thread::get_id();
        auto expected = std::thread::id();

        if (owner_.compare_exchange_strong(expected, current_id,
                                           std::memory_order_acquire,
                                           std::memory_order_relaxed)) {
            recursion_count_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        if (owner_.load(std::memory_order_acquire) == current_id) {
            recursion_count_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        return false;
    }

    void lock(int wait_ms = 0) {
        while (!try_lock()) {
            if (wait_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(wait_ms));
            } else {
                std::this_thread::yield();
            }
        }
    }

    void unlock() {
        auto current_id = std::this_thread::get_id();

        if (owner_.load(std::memory_order_acquire) != current_id) {
            LOCKER_LOG("warning : unlock from non-owner thread\n");
            return;
        }

        int prev = recursion_count_.fetch_sub(1, std::memory_order_relaxed);
        if (prev == 1) {
            recursion_count_.store(0, std::memory_order_relaxed);
            owner_.store(std::thread::id(), std::memory_order_release);
        }
    }

    void force_unlock() {
        if (owner_.load(std::memory_order_acquire) != std::this_thread::get_id()) {
            LOCKER_LOG("warning : force_unlock from non-owner thread\n");
        }
        recursion_count_.store(0, std::memory_order_relaxed);
        owner_.store(std::thread::id(), std::memory_order_release);
    }

    lg_t lock_guard() { return lg_t(this); }
    tlg_t try_lock_guard() { return tlg_t(this); }

    bool is_locked() const { return owner_.load(std::memory_order_acquire) != std::thread::id(); }


private:
    std::atomic<std::thread::id> owner_{std::thread::id()};
    std::atomic<int> recursion_count_{0};
};

#endif // __SPINLOCKER_H__