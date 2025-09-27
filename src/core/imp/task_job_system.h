#pragma once

#include "lockfreelist.h"
#include "function.hpp"

class TaskJobSystem
{
public:

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
    
    enum TaskReturnCode : int
    {
        RSLEEP,            // Need to wait and try again (use sleep, the delay is not set)
        RAGAIN,            // Need to run again (if delay is set, it is after_call after its end)
        RDEFAULT = RAGAIN, // Need to run again (if delay is set, it is after_call after
                           // its end)
        RDELAY = RAGAIN,   // Need to run again (if delay is set, it is after_call after
                           // its end)
        RSLEEP_OR_DELAY,   // Need to wait and try again (use sleep or delay)
        RIMMEDIATELY,      // Need to run again IMMEDIATELY
        RSTOP,             // Need to stop task
        RERROR,            // UNDEFINED BEHAVIOR
        REMPTY             // UNDEFINED BEHAVIOR
    };

    struct ITask
    {
        Rational delay = {0, 0};
        size_t delay_ts = 0;
        Rational sleep = {1, 1000};
        size_t sleep_ts = 0;

        void ToStop() { to_stop = true; }
        bool Stopped() { return to_stop; }

        virtual bool can_call() = 0;
        virtual void after_call() = 0;
        virtual int call() = 0;

        virtual void stopped() = 0;
    private:
        bool to_stop = false;
    };

    template<typename... ARGS>
    struct Task : public ITask
    {
        template<typename C, typename Check = srv_function<int(ARGS...)>::template check_callable<C>>
        Task(C f, ARGS... vars) : f(f), vars({vars...}) {}

        std::tuple<ARGS...> vars;
        srv_function<int(ARGS...)> f;
        
        virtual int call()
        {
            if(f)
                return std::apply([this](ARGS... args){ return (f)(args...); }, vars);
            return REMPTY;
        };

        virtual bool can_call() { return true; };
        virtual void after_call() {};
        virtual void stopped() {};
    };

    template<typename O, typename... ARGS>
    struct Task : public ITask
    {
        Task(O* o, int (O::*f)(ARGS...), ARGS... vars) : o(o), f(f), vars({vars...}) {}

        std::tuple<ARGS...> vars;
        O* o;
        int (O::*f)(ARGS...);
        
        virtual int call()
        {
            if(o && f)
                return std::apply([this](ARGS... args){ return (o->*f)(args...); }, vars);
            return REMPTY;
        };

        virtual bool can_call() { return true; };
        virtual void after_call() {};
        virtual void stopped() {};
    };

    struct IJob
    {
        Rational delay = {0, 1};

        virtual bool can_call() = 0;
        virtual void after_call() = 0;
        virtual int call() = 0;
    };

    template<typename... ARGS>
    struct Job : public IJob
    {
        template<typename C, typename Check = srv_function<int(ARGS...)>::template check_callable<C>>
        Job(C f, ARGS... vars) : f(f), vars({vars...}) {}

        std::tuple<ARGS...> vars;
        srv_function<void(ARGS...)> f;
        
        virtual void call()
        {
            if(f) std::apply([this](ARGS... args){ (f)(args...); }, vars);
        };

        virtual bool can_call() { return true; };
        virtual void after_call() {};
    };

    template<typename O, typename... ARGS>
    struct Job : public IJob
    {
        Job(O* o, void (O::*f)(ARGS...), ARGS... vars) : o(o), f(f), vars({vars...}) {}

        std::tuple<ARGS...> vars;
        O* o;
        void (O::*f)(ARGS...);
        
        virtual void call()
        {
            if(o && f) std::apply([this](ARGS... args){ (o->*f)(args...); }, vars);
        };

        virtual bool can_call() { return true; };
        virtual void after_call() {};
    };

private:
    std::vector<std::thread> taskpool;
    std::vector<std::thread> jobpool;
    std::vector<std::thread> anypool;
    std::atomic_bool run{true};

    LockFreeList<ITask*> tasks;
    LockFreeList<IJob*> jobs;

    SpinLocker rm_locker;
    
    bool greedy;

    unsigned char TASKTHCOUNT = 0;
    unsigned char JOBTHCOUNT = 0;
    unsigned char ANYTHCOUNT = 1;
public:

    bool JobProcess() 
    {
        auto cts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for(auto mess : jobs)
        {
            if(mess->can_call())
            {
                jobs.RemoveNode(mess);
                mess->call();
                mess->after_call();
            }
            return true;
        }
        return false;
    }

    bool TaskProcess()
    {
        int i = 0;
        auto cts = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        for(auto mess : tasks)
        {
            if(mess->Stopped())
                continue;
            const auto del = mess->delay;
            const size_t del_ts = Rational::rescale(cts, { std::chrono::high_resolution_clock::period::num, std::chrono::high_resolution_clock::period::den}, del);
            const size_t del_tts = mess->delay_ts + 1;
            if(del.den && del.num && (del_tts > del_ts))
                continue;            
            const auto sle = mess->sleep;
            const size_t sle_ts = Rational::rescale(cts, { std::chrono::high_resolution_clock::period::num, std::chrono::high_resolution_clock::period::den}, del);
            const size_t sle_tts = mess->sleep_ts + 1;
            if(sle.den && sle.num && (sle_tts > sle_ts))
                continue;
            if(mess->can_call())
            {
                auto ret = (TaskReturnCode)(mess->call());
                switch (ret)
                {
                case TaskReturnCode::RAGAIN: {
                    if(del.den && del.num)
                    {
                        if(del_ts - del_tts > 3)
                            mess->delay_ts = del_ts;
                        else
                            mess->delay_ts = del_tts;
                    }
                    break;
                }
                case TaskReturnCode::RSLEEP: {
                    if(sle.den && sle.num)
                    {
                        if(sle_ts - sle_tts > 3)
                            mess->sleep_ts = sle_ts;
                        else
                            mess->sleep_ts = sle_tts;
                    }
                    break;
                }
                case TaskReturnCode::RSLEEP_OR_DELAY: {
                    if(del.den && del.num)
                    {
                        if(del_ts - del_tts > 3)
                            mess->delay_ts = del_ts;
                        else
                            mess->delay_ts = del_tts;
                    }
                    if(sle.den && sle.num)
                    {
                        if(sle_ts - sle_tts > 3)
                            mess->sleep_ts = sle_ts;
                        else
                            mess->sleep_ts = sle_tts;
                    }
                    break;
                }
                case TaskReturnCode::RSTOP : {
                    mess->ToStop();
                    mess->stopped();
                    tasks.RemoveNode(mess);
                    break;
                }
                default:
                    break;
                }

                ++i;
                mess->after_call();
            }
        }
        return i > 0;
    }

    bool AnyProcess()
    {
        if(!JobProcess())
            TaskProcess();
    }
private:
    static void JobThredaPoolThread(TaskJobSystem* self, int id)
    {
        while(self->run.load()) if(self->JobProcess() == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    static void TaskThredaPoolThread(TaskJobSystem* self, int id)
    {
        while(self->run.load()) if(self->TaskProcess() == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    static void AnyTaskThredaPoolThread(TaskJobSystem* self, int id)
    {
        if(id != 0 || !self->greedy)
            while(self->run.load()) if(self->AnyProcess() == 0) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        else
            while(self->run.load()) if(self->AnyProcess() == 0) std::this_thread::yield();
    }

public:
    TaskJobSystem(unsigned char any = 8, unsigned char task = 2, unsigned char job = 2, bool greedy = true) 
        : TASKTHCOUNT(task)
        , JOBTHCOUNT(job)
        , ANYTHCOUNT(any)
        , greedy(greedy)
    {
        taskpool.resize(TASKTHCOUNT);
        jobpool.resize(JOBTHCOUNT);
        anypool.resize(ANYTHCOUNT);
        for(int i = 0; i < TASKTHCOUNT; i++)
        {
            taskpool[i] = std::thread(TaskThredaPoolThread, this, i);
            taskpool[i].detach();
        }
        for(int i = 0; i < JOBTHCOUNT; i++)
        {
            jobpool[i] = std::thread(JobThredaPoolThread, this, i);
            jobpool[i].detach();
        }
        for(int i = 0; i < ANYTHCOUNT; i++)
        {
            anypool[i] = std::thread(AnyTaskThredaPoolThread, this, i);
            anypool[i].detach();
        }
    }

private:

public:

   
    ~TaskJobSystem()
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
