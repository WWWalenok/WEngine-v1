#pragma once

#include "lockfreelist.h"
#include "function.hpp"
#include "data.h"
#include "allocator.h"
#include <chrono>

static std::atomic<size_t> tot_ref_count{1};
static std::atomic<size_t> tot_ref_count_mes{1};
static std::atomic<size_t> tot_ref_count_rsp{1};
class ThreadPool
{
    unsigned char THCOUNT = 1;

    struct Rational
    {
        constexpr Rational() = default;
        template<typename T1, typename T2>
        constexpr Rational(T1 _num, T2 _den) : num(_num), den(_den)
        {
        }
        int64_t num = 0;
        int64_t den = 0;

        inline bool operator==(const Rational& x) const
        {
            return num == x.num && den == x.den;
        }
        inline bool operator!=(const Rational& x) const
        {
            return num != x.num || den != x.den;
        }
        inline bool operator<(const Rational& x) const
        {
            return q2d() < x.q2d();
        }
        inline bool operator>(const Rational& x) const
        {
            return q2d() > x.q2d();
        }

        template<typename T>
        inline T operator*(const T& x) const
        {
            if (!den)
                return {};
            return (x * num) / den;
        }

        template<typename T>
        inline T operator/(const T& x) const
        {
            if (!num)
                return {};
            return (x * den) / num;
        }

        template<typename T1, typename T2 = T1>
        static constexpr inline T2 rescale(const T1& x, const Rational& from, const Rational& to)
        {
            const Rational v{to.den * from.num, to.num * from.den};
            if (v.den == 1)
                return T2(x * v.num);
            else if (v.num == 1)
                return T2(x) / T2(v.den);

            int64_t a = v.num, b = v.den;
            while (a && b)
                if (a > b)
                    a %= b;
                else
                    b %= a;
            a += b;
            return T2(x * (v.num / a)) / T2(v.den / a);
        }

        inline bool isEmpty() const
        {
            return num == 0 || den == 0;
        }
        inline double q2d() const
        {
            return double(num) / double(den);
        }
        inline Rational inv() const
        {
            return Rational{den, num};
        }
    };

    struct IDataStroageBase
    {
        std::atomic<size_t> ref_count{1};
        std::atomic<bool> not_completed{true};
        std::atomic<bool> repetable{false};


        IDataStroageBase(){ tot_ref_count.fetch_add(1); }
        virtual ~IDataStroageBase(){
            tot_ref_count.fetch_sub(1);
        }

        IDataStroageBase(const IDataStroageBase&) = delete;
        IDataStroageBase& operator=(const IDataStroageBase&) = delete;

        void add_ref() noexcept
        {
            ref_count.fetch_add(1, std::memory_order_relaxed);
        }

        void release() noexcept
        {
            if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
                destroy_self();
        }

        void complit()
        {
            not_completed.store(false);
        }
        void stop()
        {
            repetable.store(false);
        }

        void wait()
        {
            while (true)
            {
                if (!not_completed.load())
                    return;
                for (unsigned short i = 0; i < 0x40 && not_completed.load(); ++i)
                    ;
                for (unsigned short i = 0; i < 0x4000 && not_completed.load(); ++i)
                    std::this_thread::yield();
                while (not_completed.load())
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        }
        virtual void destroy_self() noexcept = 0;
    };

    template<typename T>
    struct DataStroage : public IDataStroageBase
    {
        srv_function<void(T&)> callback = nullptr;
        T data;

        void destroy_self() noexcept override
        {
            delete this;
        }
    };

    template<>
    struct DataStroage<void> : public IDataStroageBase
    {
        srv_function<void()> callback = nullptr;

        void destroy_self() noexcept override
        {
            delete this;
        }
    };

    struct IMessage
    {
        IDataStroageBase* data_base = nullptr;

        size_t callaed = 0;
        size_t tid = 0;
        std::atomic<size_t> s_counter{0};
        Rational delay_val{0, 0};
        size_t last_ts_val = 0;

        IMessage() {
            tot_ref_count_mes.fetch_add(1);
        }

        IMessage(const IMessage&) = delete;
        IMessage& operator=(const IMessage&) = delete;

        IMessage* inc()
        {
            ++s_counter;
            return this;
        }
        bool dec()
        {
            return (--s_counter) == 0;
        }

        bool repetable()
        {
            auto* d = getDataStroage();
            return d ? d->repetable.load() : false;
        }
        void set_repetable(bool val)
        {
            auto* d = getDataStroage();
            if (d)
                d->repetable.store(val);
        }
        Rational delay()
        {
            return delay_val;
        }
        size_t ts()
        {
            return last_ts_val;
        }
        void set_ts(size_t ts)
        {
            last_ts_val = ts;
        }

        IDataStroageBase* getDataStroage() noexcept
        {
            return data_base;
        }

        virtual void call() = 0;

        virtual ~IMessage()
        {
            if (data_base)
            {
                data_base->release();
                data_base = nullptr;
            }
            tot_ref_count_mes.fetch_sub(1);
        }
    };

    template<typename T, typename... ARGS>
    struct Message : public IMessage
    {
        std::tuple<std::decay_t<ARGS>...> vars;
        srv_function<T(ARGS...)> f = nullptr;
        DataStroage<T>* data = nullptr;

        Message(srv_function<T(ARGS...)> f, DataStroage<T>* d, decltype(vars) new_vars) : vars(new_vars), f(f), data(d)
        {
            this->data_base = d;
        }

        void SetCallback(srv_function<void(T&)> callback)
        {
            if (data)
                data->callback = callback;
        }

        virtual void call() override
        {
            if (f && data)
            {
                data->data = std::apply([this](ARGS... args) {
                    return (f)(args...);
                }, vars);
                if (data->callback)
                    data->callback(data->data);
            }
            if (data)
                data->complit();
        }
    };

    template<typename... ARGS>
    struct Message<void, ARGS...> : public IMessage
    {
        std::tuple<std::decay_t<ARGS>...> vars;
        srv_function<void(ARGS...)> f = nullptr;
        DataStroage<void>* data = nullptr;

        Message(srv_function<void(ARGS...)> f, DataStroage<void>* d, decltype(vars) new_vars) : vars(new_vars), f(f), data(d)
        {
            this->data_base = d;
        }

        void SetCallback(srv_function<void()> callback)
        {
            if (data)
                data->callback = callback;
        }

        virtual void call() override
        {
            if (f && data)
            {
                std::apply([this](ARGS... args) {
                    (f)(args...);
                }, vars);
                if (data->callback)
                    data->callback();
            }
            if (data)
                data->complit();
        }
    };

    template<typename T, typename O, typename... ARGS>
    struct MessageO : public IMessage
    {
        std::tuple<std::decay_t<ARGS>...> vars;
        O* o = nullptr;
        T (O::*fo)(ARGS...) = nullptr;
        DataStroage<T>* data = nullptr;

        MessageO(O* o, T (O::*fo)(ARGS...), DataStroage<T>* d, decltype(vars) new_vars) : vars(new_vars), o(o), fo(fo), data(d)
        {
            this->data_base = d;
        }

        void SetCallback(srv_function<void(T&)> callback)
        {
            if (data)
                data->callback = callback;
        }

        virtual void call() override
        {
            if (o && fo && data)
            {
                data->data = std::apply([this](ARGS... args) {
                    return (o->*fo)(args...);
                }, vars);
                if (data->callback)
                    data->callback(data->data);
            }
            if (data)
                data->complit();
        }
    };

    template<typename O, typename... ARGS>
    struct MessageO<void, O, ARGS...> : public IMessage
    {
        std::tuple<std::decay_t<ARGS>...> vars;
        O* o = nullptr;
        void (O::*fo)(ARGS...) = nullptr;
        DataStroage<void>* data = nullptr;

        MessageO(O* o, void (O::*fo)(ARGS...), DataStroage<void>* d, decltype(vars) new_vars) : vars(new_vars), o(o), fo(fo), data(d)
        {
            this->data_base = d;
        }

        void SetCallback(srv_function<void()> callback)
        {
            if (data)
                data->callback = callback;
        }

        virtual void call() override
        {
            if (o && fo && data)
            {
                std::apply([this](ARGS... args) {
                    (o->*fo)(args...);
                }, vars);
                if (data->callback)
                    data->callback();
            }
            if (data)
                data->complit();
        }
    };

    std::vector<std::thread> pool;
    std::atomic_bool run{true};
    std::atomic_int run_thread_count{0};

    size_t buffer_mask = 0xff;

    LockFreeList<IMessage*> buffers[32];
    size_t buffer_offset = 0;
    size_t buffer_offset_push = 0;

    SpinLocker rm_locker;
    bool greedy;

public:
    bool Process()
    {
        WL_START_TYMETRACE;
        for (int prio = 0; prio < sizeof(buffers) / sizeof(buffers[0]); ++prio)
        {
            auto& buffer = buffers[prio];
            if (buffer.Size() == 0)
                continue;

            int i = 0;
            auto __end__ = buffer.end();
            for (auto it = buffer.begin(); it != __end__; ++it)
            {
                ThreadPool::IMessage*& mess = *it;
                if (!mess || (!mess->repetable() && mess->callaed))
                {
                    continue;
                }
                if (mess->repetable())
                {
                    const auto del = mess->delay();
                    auto now = std::chrono::high_resolution_clock::now();
                    constexpr Rational base{1, int(1e9)};
                    const size_t ts = Rational::rescale(std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count(), base, del);
                    const size_t tts = mess->ts() + 1;
                    if (mess->delay().den && (tts > ts))
                        continue;

                    if (ts - tts > 3)
                        mess->set_ts(ts);
                    else
                        mess->set_ts(tts);
                }
                mess->call();
                ++mess->callaed;
                if (!mess->repetable())
                {
                    if(mess)
                        delete mess;
                    mess = nullptr;
                    buffer.RemoveNode(it.val);
                }
                ++i;
            }
            if (i > 0)
                return true;
        }
        return false;
    }

private:
    static void ThreadPoolThread(ThreadPool* self, int id)
    {
        self->run_thread_count++;
        if (id == 0)
        {
            while (self->run.load())
            {
                {
                    WL_START_CUSTOM_TYMETRACE(ThreadPoolThread clear);
                    for (int prio = 0; prio < sizeof(buffers) / sizeof(buffers[0]); ++prio)
                    {
                        auto& buffer = self->buffers[prio];
                        if (buffer.Size())
                        {
                            buffer.CearDeleted();
                            buffer.Free();
                        }
                    }
                }
                if (self->Process() == 0)
                {
                    WL_START_CUSTOM_TYMETRACE(ThreadPoolThread sleep);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
        else
        {
            while (self->run.load())
                if (self->Process() == 0)
                {
                    WL_START_CUSTOM_TYMETRACE(ThreadPoolThread sleep);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
        }
        self->run_thread_count--;
    }

public:
    ThreadPool(unsigned char count = 16, bool greedy = true) : THCOUNT(count), greedy(greedy)
    {
        pool.resize(THCOUNT);
        for (int i = 0; i < THCOUNT; i++)
        {
            pool[i] = std::thread(ThreadPoolThread, this, i);
            pool[i].detach();
        }
    }

    template<typename T>
    struct Response
    {
        DataStroage<T>* resp_data = nullptr;
        srv_function<void(T&)> callback = nullptr;

        explicit Response(DataStroage<T>* new_resp_data) noexcept : resp_data(new_resp_data)
        {
            tot_ref_count_rsp.fetch_add(1);
        }

        Response(const Response& o) : resp_data(o.resp_data), callback(o.callback)
        {
            if (resp_data)
                resp_data->add_ref();
        }

        Response(Response&& o) noexcept : resp_data(o.resp_data), callback(std::move(o.callback))
        {
            o.resp_data = nullptr;
        }

        Response& operator=(const Response& o)
        {
            if (this != &o)
            {
                if (resp_data)
                    resp_data->release();
                resp_data = o.resp_data;
                callback = o.callback;
                if (resp_data)
                    resp_data->add_ref();
            }
            return *this;
        }

        Response& operator=(Response&& o) noexcept
        {
            if (this != &o)
            {
                if (resp_data)
                    resp_data->release();
                resp_data = o.resp_data;
                callback = std::move(o.callback);
                o.resp_data = nullptr;
            }
            return *this;
        }

        ~Response()
        {
            if (resp_data)
            {
                resp_data->release();
                resp_data = nullptr;
            }
            tot_ref_count_rsp.fetch_sub(1);
        }

        void SetCallback(srv_function<void(T&)> cb)
        {
            this->callback = cb;
            if (resp_data)
                resp_data->callback = cb;
        }

        inline void wait()
        {
            if (resp_data)
                resp_data->wait();
        }
        inline void stop()
        {
            if (resp_data)
                resp_data->repetable.store(false);
        }
        bool completed()
        {
            return resp_data && !resp_data->not_completed.load();
        }

        T& get()
        {
            wait();
            return resp_data->data;
        }

        operator T&()
        {
            return get();
        }
        operator const T&() const
        {
            return const_cast<Response*>(this)->get();
        }
    };

    template<>
    struct Response<void>
    {
        DataStroage<void>* resp_data = nullptr;
        srv_function<void()> callback = nullptr;

        explicit Response(DataStroage<void>* new_resp_data) noexcept : resp_data(new_resp_data)
        {
        }

        Response(const Response& o) : resp_data(o.resp_data), callback(o.callback)
        {
            if (resp_data)
                resp_data->add_ref();
        }

        Response(Response&& o) noexcept : resp_data(o.resp_data), callback(std::move(o.callback))
        {
            o.resp_data = nullptr;
        }

        Response& operator=(const Response& o)
        {
            if (this != &o)
            {
                if (resp_data)
                    resp_data->release();
                resp_data = o.resp_data;
                callback = o.callback;
                if (resp_data)
                    resp_data->add_ref();
            }
            return *this;
        }

        Response& operator=(Response&& o) noexcept
        {
            if (this != &o)
            {
                if (resp_data)
                    resp_data->release();
                resp_data = o.resp_data;
                callback = std::move(o.callback);
                o.resp_data = nullptr;
            }
            return *this;
        }

        ~Response()
        {
            if (resp_data)
            {
                resp_data->release();
                resp_data = nullptr;
            }
        }

        void SetCallback(srv_function<void()> cb)
        {
            this->callback = cb;
            if (resp_data)
                resp_data->callback = cb;
        }

        inline void wait()
        {
            if (resp_data)
                resp_data->wait();
        }
        inline void stop()
        {
            if (resp_data)
                resp_data->repetable.store(false);
        }
        bool completed()
        {
            return resp_data && !resp_data->not_completed.load();
        }
    };

private:
    template<typename T>
    static DataStroage<T>* make_shared_data()
    {
        DataStroage<T>* d = new DataStroage<T>();
        d->add_ref();
        return d;
    }

    template<typename T, typename... ARGS>
    Response<T> impSend(int prio, srv_function<T(ARGS...)> f, srv_function<void(T&)> callback, Rational delay, const ARGS&... vars)
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2 - prio;
        if (prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if (prio < 0)
            prio = 0;

        auto& buffer = buffers[prio];
        DataStroage<T>* data = make_shared_data<T>();
        Message<T, ARGS...>* m = new Message<T, ARGS...>(f, data, {vars...});
        m->set_repetable(delay.den != 0);
        m->delay_val = delay;
        if (callback)
            m->SetCallback(callback);
        buffer.AddNode(m);
        return Response<T>{data};
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> impSend(int prio, O* owner, T (O::*func)(ARGS...), srv_function<void(T&)> callback, Rational delay, const ARGS&... vars)
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2 - prio;
        if (prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if (prio < 0)
            prio = 0;

        auto& buffer = buffers[prio];
        DataStroage<T>* data = make_shared_data<T>();
        auto m = new MessageO<T, O, ARGS...>(owner, func, data, {vars...});
        m->set_repetable(delay.den != 0);
        m->delay_val = delay;
        if (callback)
            m->SetCallback(callback);
        buffer.AddNode(m);
        return Response<T>{data};
    }

    template<typename... ARGS>
    Response<void> impSend(int prio, srv_function<void(ARGS...)> f, srv_function<void()> callback, Rational delay, const ARGS&... vars)
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2 - prio;
        if (prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if (prio < 0)
            prio = 0;

        auto& buffer = buffers[prio];
        DataStroage<void>* data = make_shared_data<void>();
        auto m = new Message<void, ARGS...>(f, data, {vars...});
        m->set_repetable(delay.den != 0);
        m->delay_val = delay;
        if (callback)
            m->SetCallback(callback);
        buffer.AddNode(m);
        return Response<void>{data};
    }

    template<typename O, typename... ARGS>
    Response<void> impSend(int prio, O* owner, void (O::*func)(ARGS...), srv_function<void()> callback, Rational delay, const ARGS&... vars)
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2 - prio;
        if (prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if (prio < 0)
            prio = 0;

        auto& buffer = buffers[prio];
        DataStroage<void>* data = make_shared_data<void>();
        auto m = new MessageO<void, O, ARGS...>(owner, func, data, {vars...});
        m->set_repetable(delay.den != 0);
        m->delay_val = delay;
        if (callback)
            m->SetCallback(callback);
        buffer.AddNode(m);
        return Response<void>{data};
    }

public:
    //------------------------------------------------------------------------------------------------------------------------------------------------
    // send functions
    //------------------------------------------------------------------------------------------------------------------------------------------------

    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> operator()(int prio, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> operator()(int prio, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, nullptr, {0, 0}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> operator()(int prio, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, nullptr, {0, 0}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send(int prio, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send(int prio, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, nullptr, {0, 0}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send(int prio, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, nullptr, {0, 0}, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_with_callback(int prio, C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, {0, 0}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, {0, 0}, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_with_callback(int prio, O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, callback, {0, 0}, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_with_callback(int prio, O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, callback, {0, 0}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send_repetable(int prio, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {0, 1}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {0, 1}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send_repetable(int prio, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, nullptr, {0, 1}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send_repetable(int prio, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, nullptr, {0, 1}, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_repetable_with_callback(int prio, C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, {0, 1}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, {0, 1}, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_repetable_with_callback(int prio, O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, callback, {0, 1}, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_repetable_with_callback(int prio, O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, callback, {0, 1}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send_delayed_repetable(int prio, unsigned int delay_ms, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send_delayed_repetable(int prio, unsigned int delay_ms, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, nullptr, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send_delayed_repetable(int prio, unsigned int delay_ms, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, nullptr, {delay_ms, 1000}, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(int prio, unsigned int delay_ms, C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(int prio, unsigned int delay_ms, O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, callback, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_delayed_repetable_with_callback(int prio, unsigned int delay_ms, O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, callback, {delay_ms, 1000}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send_delayed_repetable(int prio, Rational delay, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, delay, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, delay, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send_delayed_repetable(int prio, Rational delay, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, nullptr, delay, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send_delayed_repetable(int prio, Rational delay, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, nullptr, delay, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(int prio, Rational delay, C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, delay, vars...);
        else
            return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, delay, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(int prio, Rational delay, O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(prio, o, f, callback, delay, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_delayed_repetable_with_callback(int prio, Rational delay, O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(prio, o, f, callback, delay, vars...);
    }



    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> operator()(C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> operator()(O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, nullptr, {0, 0}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> operator()(O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, nullptr, {0, 0}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send(C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send(O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, nullptr, {0, 0}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send(O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, nullptr, {0, 0}, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_with_callback(C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, {0, 0}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, {0, 0}, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_with_callback(O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, callback, {0, 0}, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_with_callback(O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, callback, {0, 0}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send_repetable(C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {0, 1}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {0, 1}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send_repetable(O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, nullptr, {0, 1}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send_repetable(O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, nullptr, {0, 1}, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_repetable_with_callback(C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, {0, 1}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, {0, 1}, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_repetable_with_callback(O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, callback, {0, 1}, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_repetable_with_callback(O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, callback, {0, 1}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send_delayed_repetable(unsigned int delay_ms, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send_delayed_repetable(unsigned int delay_ms, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, nullptr, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send_delayed_repetable(unsigned int delay_ms, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, nullptr, {delay_ms, 1000}, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(unsigned int delay_ms, C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(unsigned int delay_ms, O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, callback, {delay_ms, 1000}, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_delayed_repetable_with_callback(unsigned int delay_ms, O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, callback, {delay_ms, 1000}, vars...);
    }


    template<typename C, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T> send_delayed_repetable(Rational delay, C f, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, delay, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, delay, vars...);
    }

    template<typename O, typename T, typename... ARGS>
    Response<T> send_delayed_repetable(Rational delay, O* o, T (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, nullptr, delay, vars...);
    }

    template<typename O, typename... ARGS>
    Response<void> send_delayed_repetable(Rational delay, O* o, void (O::*f)(ARGS...), const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, nullptr, delay, vars...);
    }


    template<typename C, typename Call, typename... ARGS, typename T = std::invoke_result_t<std::decay_t<C>&, ARGS...>, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(Rational delay, C f, Call callback, const ARGS&... vars)
    {
        if constexpr (std::is_void_v<T>)
            return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, delay, vars...);
        else
            return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, delay, vars...);
    }

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T> send_delayed_repetable_with_callback(Rational delay, O* o, T (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, T, ARGS...>(0, o, f, callback, delay, vars...);
    }

    template<typename O, typename Call, typename... ARGS, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void> send_delayed_repetable_with_callback(Rational delay, O* o, void (O::*f)(ARGS...), Call callback, const std::decay_t<ARGS>&... vars)
    {
        return impSend<O, ARGS...>(0, o, f, callback, delay, vars...);
    }

    //------------------------------------------------------------------------------------------------------------------------------------------------
    // send functions
    //------------------------------------------------------------------------------------------------------------------------------------------------


    ~ThreadPool()
    {
        run.store(false);
        bool wait = true;
        while (wait)
        {
            wait = (run_thread_count.load() > 0);
            for (int i = 0; i < THCOUNT; i++)
            {
                if (pool[i].joinable())
                {
                    pool[i].join();
                    wait = true;
                }
            }
            if (wait)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
};