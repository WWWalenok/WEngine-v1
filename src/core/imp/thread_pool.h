#pragma once

#include "lockfreelist.h"
#include "function.hpp"
#include <chrono>

class ThredaPool
{
    unsigned char THCOUNT = 1;

    struct Rational
    {
        constexpr Rational() = default;
        template<typename T1, typename T2>
        constexpr Rational(T1 _num, T2 _den) : num(_num), den(_den) {}
        int64_t num = 0;
        int64_t den = 0;
        inline bool operator ==(const Rational& x) const
        {
            return num == x.num && den == x.den;
        }
    
        inline bool operator !=(const Rational& x) const
        {
            return num != x.num || den != x.den;
        }
    
        inline bool operator <(const Rational& x) const
        {
            return q2d() < x.q2d();
        }
    
        inline bool operator >(const Rational& x) const
        {
            return q2d() > x.q2d();
        }
    
        template<typename T> inline T operator *(const T& x) const
        {
            if(!den)
                return {};
            return (x * num) / den;
        }
    
        template<typename T> inline T operator /(const T& x) const
        {
            if(!num)
                return {};
            return (x * den) / num;
        }
    
        template<typename T1, typename T2 = T1> static constexpr inline T2 rescale(const T1& x, const Rational& from, const Rational& to)
        {
            const Rational v{to.den * from.num, to.num * from.den};
            if (v.den == 1)
                return T2(x * v.num);
            else if (v.num == 1)
                return T2(x) / T2(v.den);
            
            int64_t a = v.num, b = v.den;
            while (a && b) if (a > b) a%=b; else b%=a;
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
        inline Rational inv() const { return Rational{ den, num }; }
    };
    
    struct ResponseData
    {
        std::vector<uint8_t> data;
        size_t tid;
        std::atomic<bool> not_completed{true};
        std::atomic<bool> repetable{false};
        Rational delay;
        size_t last_ts = 0;
        void* owner = 0;

        inline void completed() { if(callback) callback(owner, data.data()); not_completed.store(false);}
        void (*callback)(void*, uint8_t*);
    };

    template<typename context_type, typename T>
    static void ResponseDataCallabc(void* context, uint8_t* data)
    {
        auto self = (context_type*)context;
        if(self && self->callback)
        {
            if constexpr (std::is_same_v<T, void>)
                self->callback();
            else if(data)
                self->callback(*((T*)data));
        }
    }
    

    struct IMessage
    {
        ResponseData* resp_data;
        size_t callaed = 0;

        bool repetable() { return (resp_data && resp_data->repetable.load()); }
        Rational delay() { return (resp_data) ? resp_data->delay : Rational{0, 1}; }
        size_t ts() { return (resp_data) ? resp_data->last_ts : 0; }
        void set_ts(size_t ts) { if(resp_data) resp_data->last_ts = ts; }
        virtual void callimp(std::vector<uint8_t>* data, size_t* tid) = 0;

        void call()
        {
            callimp(&(resp_data->data), &(resp_data->tid));
            if(!repetable())
            {
                resp_data->completed();
            }
        }
    };

    template<typename T, typename... ARGS>
    struct Message : public IMessage
    {
        std::tuple<ARGS...> vars;
        srv_function<T(ARGS...)> f;

        srv_function<void(T&)> callback = nullptr;

        void SetCallback(srv_function<void(T&)> callback)
        {
            this->callback = callback;
            resp_data->owner = this;
            resp_data->callback = ResponseDataCallabc<Message<T, ARGS...>, T>;
        }

        virtual void callimp(std::vector<uint8_t>* data, size_t* tid) override
        {
            if(f)
            {
                auto r = std::apply(f, vars);
                *tid = typeid(T).hash_code();
                data->resize(sizeof(r));
                *((T*)(data->data())) = std::move(r);
            }
        }
    };

    template<typename... ARGS>
    struct Message<void, ARGS...> : public IMessage
    {
        std::tuple<ARGS...> vars;
        srv_function<void(ARGS...)> f;

        srv_function<void()> callback = nullptr;

        void SetCallback(srv_function<void()> callback)
        {
            this->callback = callback;
            resp_data->owner = this;
            resp_data->callback = ResponseDataCallabc<Message<void, ARGS...>, void>;
        }

        virtual void callimp(std::vector<uint8_t>* data, size_t* tid) override
        {
            if(f)
            {
                std::apply(f, vars);
                *tid = 0;
                data->resize(0);
            }
        }
    };

    template<typename O, typename T, typename... ARGS>
    struct OMessage : public IMessage
    {
        std::tuple<ARGS...> vars;
        O* o;
        T (O::*f)(ARGS...);

        srv_function<void(T&)> callback = nullptr;

        void SetCallback(srv_function<void(T&)> callback)
        {
            this->callback = callback;
            resp_data->owner = this;
            resp_data->callback = ResponseDataCallabc<OMessage<O, T, ARGS...>, T>;
        }

        virtual void callimp(std::vector<uint8_t>* data, size_t* tid) override
        {
            if(o && f)
            {
                auto r = std::apply([this](ARGS... args){ return (o->*f)(args...); }, vars);
                *tid = typeid(T).hash_code();
                data->resize(sizeof(r));
                *((T*)(data->data())) = std::move(r);
            }
        }
    };


    template<typename O, typename... ARGS>
    struct OMessage<O, void, ARGS...> : public IMessage
    {
        std::tuple<ARGS...> vars;
        O* o;
        void (O::*f)(ARGS...);

        srv_function<void()> callback = nullptr;

        void SetCallback(srv_function<void()> callback)
        {
            this->callback = callback;
            resp_data->owner = this;
            resp_data->callback = ResponseDataCallabc<OMessage<O, void, ARGS...>, void>;
        }

        virtual void callimp(std::vector<uint8_t>* data, size_t* tid) override
        {
            if(o && f)
            {
                std::apply([this](ARGS... args){ (o->*f)(args...); }, vars);
                *tid = 0;
                data->resize(0);
            }
        }
    };

    std::vector<std::thread> pool;
    std::atomic_bool run{true};
    
    size_t buffer_mask = 0xff;
    
    LockFreeList<IMessage*> buffers[32];
    size_t buffer_offset = 0;
    size_t buffer_offset_push = 0;

    SpinLocker rm_locker;
    
    bool greedy;

public:
    bool Process()
    {
        for(int prio = 0; prio < sizeof(buffers) / sizeof(buffers[0]); ++prio)
        {
            auto& buffer = buffers[prio];
            int i = 0;
            if(rm_locker.try_lock())
            {
                i = 1;
                while(i)
                {
                    i = 0;
                    for(auto mess : buffer)
                    {
                        if(!mess->repetable() && mess->callaed)
                        {
                            buffer.RemoveNode(mess);
                            ++i;
                            break;
                        }
                    }
                }
                rm_locker.unlock();
            }
            for(auto mess : buffer)
            {
                if(!mess->repetable() && mess->callaed)
                    continue;
                if(mess->repetable())
                {
                    const auto del = mess->delay();
                    auto now = std::chrono::high_resolution_clock::now();
                    const size_t ts = Rational::rescale(
                        std::chrono::duration_cast<std::chrono::nanoseconds>(
                            now.time_since_epoch()
                        ).count(), 
                        {1, int(1e9)}, 
                        del
                    );
                    const size_t tts = mess->ts() + 1;
                    if(mess->delay().den && (tts > ts))
                        continue;

                    if(ts - tts > 3)
                        mess->set_ts(ts);
                    else
                        mess->set_ts(tts);
                }
                mess->call();
                ++mess->callaed;
                ++i;
            }
            if(i > 0) return true;
        }
        return false;
    }
private:
    static void ThredaPoolThread(ThredaPool* self, int id)
    {
        if(id != 0 || !self->greedy)
            while(self->run.load()) if(self->Process() == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        else
            while(self->run.load()) if(self->Process() == 0) std::this_thread::yield();
    }

public:
    ThredaPool(unsigned char count = 16, bool greedy = true) : THCOUNT(count), greedy(greedy)
    {
        pool.resize(THCOUNT);
        for(int i = 0; i < THCOUNT; i++)
        {
            pool[i] = std::thread(ThredaPoolThread, this, i);
            pool[i].detach();
        }
    }
    
    template<typename T>
    struct Response
    {
        ResponseData* _data;
        srv_function<void(T&)> callback = nullptr;

        void SetCallback(srv_function<void(T&)> callback)
        {
            this->callback = callback;
            _data->owner = this;
            _data->callback = ResponseDataCallabc<Response<T>, T>;
        }
        
        inline void wait() 
        { 
            auto& not_completed = _data->not_completed;
            while (true) 
            {
                if (!not_completed.load()) return;

                while (not_completed.load());
            }
        }
                
        inline void stop() 
        { 
            _data->repetable.store(false);
        }

        bool completed()
        {
            return !_data->not_completed.load();
        }
        
        T get()
        {
            wait();
            auto tid = _data->tid;
            auto data = std::move(_data->data);
            delete _data;
            if(tid != typeid(T).hash_code())
            {
                throw std::runtime_error("incomparable values");
            }
            return std::move(*((T*)(data.data())));
        }

        operator T()
        {
            return std::move(get());
        }
    };

    template<>
    struct Response<void>
    {
        ResponseData* _data;
        srv_function<void()> callback = nullptr;

        void SetCallback(srv_function<void()> callback)
        {
            this->callback = callback;
            _data->owner = this;
            _data->callback = ResponseDataCallabc<Response<void>, void>;
        }
        
        inline void wait() 
        { 
            auto& not_completed = _data->not_completed;
            while (true) 
            {
                if (!not_completed.load()) return;

                while (not_completed.load());
            }
        }
                
        inline void stop() 
        {
            _data->repetable.store(false);
        }

        bool completed()
        {
            return !_data->not_completed.load();
        }
    };

private:

    template<typename T, typename... ARGS>
    Response<T> impSend(int prio, srv_function<T(ARGS...)> f, srv_function<void(T&)> callback, Rational delay, ARGS... vars) 
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2-prio;
        if(prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if(prio < 0)
            prio = 0;
        
        auto& buffer = buffers[prio];
        auto m = new Message<T, ARGS...>();
        m->vars = {vars...};
        m->f = f;
        m->resp_data = new ResponseData();
        m->resp_data->repetable = delay.den != 0;
        m->resp_data->delay = delay;
        if(callback)
            m->SetCallback(callback);
        buffer.AddNode(m);

        return Response<T>{m->resp_data};
    }
    
    template<typename O, typename T, typename... ARGS>
    Response<T> impSend(int prio, O* owner, T (O::*func)(ARGS...), srv_function<void(T&)> callback, Rational delay, ARGS... vars) 
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2-prio;
        if(prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if(prio < 0)
            prio = 0;

        auto& buffer = buffers[prio];
        auto m = new OMessage<O, T, ARGS...>();
        m->vars = {vars...};
        m->f = func;
        m->o = owner;
        m->resp_data = new ResponseData();
        m->resp_data->repetable = delay != 0;
        m->resp_data->delay = delay;
        if(callback)
            m->SetCallback(callback);
        buffer.AddNode(m);

        return Response<T>{m->resp_data};
    }

    template<typename... ARGS>
    Response<void> impSend(int prio, srv_function<void(ARGS...)> f, srv_function<void()> callback, Rational delay, ARGS... vars) 
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2-prio;
        if(prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if(prio < 0)
            prio = 0;
        
        auto& buffer = buffers[prio];
        auto m = new Message<void, ARGS...>();
        m->vars = {vars...};
        m->f = f;
        m->resp_data = new ResponseData();
        m->resp_data->repetable = delay.den != 0;
        m->resp_data->delay = delay;
        if(callback)
            m->SetCallback(callback);
        buffer.AddNode(m);

        return Response<void>{m->resp_data};
    }
    
    template<typename O, typename... ARGS>
    Response<void> impSend(int prio, O* owner, void (O::*func)(ARGS...), srv_function<void()> callback, Rational delay, ARGS... vars) 
    {
        prio = (sizeof(buffers) / sizeof(buffers[0])) / 2-prio;
        if(prio > sizeof(buffers) / sizeof(buffers[0]) - 1)
            prio = sizeof(buffers) / sizeof(buffers[0]) - 1;
        if(prio < 0)
            prio = 0;

        auto& buffer = buffers[prio];
        auto m = new OMessage<O, void, ARGS...>();
        m->vars = {vars...};
        m->f = func;
        m->o = owner;
        m->resp_data = new ResponseData();
        m->resp_data->repetable = delay != 0;
        m->resp_data->delay = delay;
        if(callback)
            m->SetCallback(callback);
        buffer.AddNode(m);

        return Response<void>{m->resp_data};
    }

public:

    //------------------------------------------------------------------------------------------------------------------------------------------------
    // send functions
    //------------------------------------------------------------------------------------------------------------------------------------------------

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     operator()(int prio, C f, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  operator()(int prio, C f, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     operator()(int prio, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, nullptr, {0, 0}, vars...);}


    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send(int prio, C f, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send(int prio, C f, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send(int prio, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, nullptr, {0, 0}, vars...);}

    
    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_with_callback(int prio, C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, {0, 0}, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_with_callback(int prio, C f, Call callback, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, {0, 0}, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_with_callback(int prio, O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, callback, {0, 0}, vars...);}
    

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send_repetable(int prio, C f, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {0, 1}, vars...);}
    
    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send_repetable(int prio, C f, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {0, 1}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send_repetable(int prio, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, nullptr, {0, 1}, vars...);}


    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_repetable_with_callback(int prio, C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, {0, 1}, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_repetable_with_callback(int prio, C f, Call callback, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, {0, 1}, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_repetable_with_callback(int prio, O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, callback, {0, 1}, vars...);}
    

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send_delayed_repetable(int prio, unsigned int delay_ms, C f, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);}
    
    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send_delayed_repetable(int prio, unsigned int delay_ms, C f, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send_delayed_repetable(int prio, unsigned int delay_ms, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, nullptr, {delay_ms, 1000}, vars...);}

    
    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(int prio, unsigned int delay_ms, C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_delayed_repetable_with_callback(int prio, unsigned int delay_ms, C f, Call callback, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(int prio, unsigned int delay_ms, O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, callback, {delay_ms, 1000}, vars...);}
    

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send_delayed_repetable(int prio, Rational delay, C f, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), nullptr, delay vars...);}
    
    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send_delayed_repetable(int prio, Rational delay, C f, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), nullptr, delay, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send_delayed_repetable(int prio, Rational delay, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, nullptr, delay, vars...);}

    
    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(int prio, Rational delay, C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(prio, srv_function<T(ARGS...)>(f), callback, delay, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_delayed_repetable_with_callback(int prio, Rational delay, C f, Call callback, ARGS... vars) { return impSend<ARGS...>(prio, srv_function<void(ARGS...)>(f), callback, delay, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(int prio, Rational delay, O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(prio, o, f, callback, delay, vars...);}



    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     operator()(C f, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  operator()(C f, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     operator()(O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, nullptr, {0, 0}, vars...);}


    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send(C f, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send(C f, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {0, 0}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send(O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, nullptr, {0, 0}, vars...);}

    
    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_with_callback(C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, {0, 0}, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_with_callback(C f, Call callback, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, {0, 0}, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_with_callback(O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, callback, {0, 0}, vars...);}
    

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send_repetable(C f, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {0, 1}, vars...);}
    
    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send_repetable(C f, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {0, 1}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send_repetable(O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, nullptr, {0, 1}, vars...);}


    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_repetable_with_callback(C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, {0, 1}, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_repetable_with_callback(C f, Call callback, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, {0, 1}, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_repetable_with_callback(O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, callback, {0, 1}, vars...);}
    

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send_delayed_repetable(unsigned int delay_ms, C f, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);}
    
    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send_delayed_repetable(unsigned int delay_ms, C f, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, {delay_ms, 1000}, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send_delayed_repetable(unsigned int delay_ms, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, nullptr, {delay_ms, 1000}, vars...);}

    
    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(unsigned int delay_ms, C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_delayed_repetable_with_callback(unsigned int delay_ms, C f, Call callback, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, {delay_ms, 1000}, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(unsigned int delay_ms, O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, callback, {delay_ms, 1000}, vars...);}
    

    template<typename C, typename T, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>>
    Response<T>     send_delayed_repetable(Rational delay, C f, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), nullptr, delay vars...);}
    
    template<typename C, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>>
    Response<void>  send_delayed_repetable(Rational delay, C f, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), nullptr, delay, vars...);}

    template<typename O, typename T, typename... ARGS>
    Response<T>     send_delayed_repetable(Rational delay, O* o, T (O::*f)(ARGS...), ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, nullptr, delay, vars...);}

    
    template<typename C, typename T, typename Call, typename... ARGS, typename Check = srv_function<T(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(Rational delay, C f, Call callback, ARGS... vars) { return impSend<T, ARGS...>(0, srv_function<T(ARGS...)>(f), callback, delay, vars...);}

    template<typename C, typename Call, typename... ARGS, typename Check = srv_function<void(ARGS...)>::template check_callable<C>, typename Check2 = srv_function<void()>::template check_callable<Call>>
    Response<void>  send_delayed_repetable_with_callback(Rational delay, C f, Call callback, ARGS... vars) { return impSend<ARGS...>(0, srv_function<void(ARGS...)>(f), callback, delay, vars...);}

    template<typename O, typename T, typename Call, typename... ARGS, typename Check2 = srv_function<void(T&)>::template check_callable<Call>>
    Response<T>     send_delayed_repetable_with_callback(Rational delay, O* o, T (O::*f)(ARGS...), Call callback, ARGS... vars) { return impSend<O, T, ARGS...>(0, o, f, callback, delay, vars...);}

    //------------------------------------------------------------------------------------------------------------------------------------------------
    // send functions
    //------------------------------------------------------------------------------------------------------------------------------------------------

    ~ThredaPool()
    {
        run.store(false);
        for(int i = 0; i < THCOUNT; i++)
        {
            try
            {
                if(pool[i].joinable())
                    pool[i].join();
            }
            catch(const std::exception& e){}
        }
    }

};
