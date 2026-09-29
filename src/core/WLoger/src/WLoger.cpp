#define _CRT_SECURE_NO_WARNINGS
#include "../include/WLoger.h"
#include <signal.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <stdarg.h>
#include <queue>
#include <list>

namespace __tc
{
        constexpr double hrt2ns = std::chrono::high_resolution_clock::period::num * 1e9 / double(std::chrono::high_resolution_clock::period::den);
        constexpr double st2ns = std::chrono::system_clock::period::num * 1e9 / double(std::chrono::system_clock::period::den);
        constexpr double hrt2us = std::chrono::high_resolution_clock::period::num * 1e6 / double(std::chrono::high_resolution_clock::period::den);
        constexpr double st2us = std::chrono::system_clock::period::num * 1e6 / double(std::chrono::system_clock::period::den);
        constexpr double hrt2ms = std::chrono::high_resolution_clock::period::num * 1e3 / double(std::chrono::high_resolution_clock::period::den);
        constexpr double st2ms = std::chrono::system_clock::period::num * 1e3 / double(std::chrono::system_clock::period::den);
        constexpr double hrt2s = std::chrono::high_resolution_clock::period::num * 1e0 / double(std::chrono::high_resolution_clock::period::den);
        constexpr double st2s = std::chrono::system_clock::period::num * 1e0 / double(std::chrono::system_clock::period::den);
        constexpr double st2hrt = double(std::chrono::system_clock::period::num * std::chrono::high_resolution_clock::period::den)
                / double(std::chrono::system_clock::period::den * std::chrono::high_resolution_clock::period::num);
        constexpr double hrt2st = double(std::chrono::high_resolution_clock::period::num * std::chrono::system_clock::period::den)
                / double(std::chrono::high_resolution_clock::period::den * std::chrono::system_clock::period::num);
}; // namespace __tc

void __wloger_INIT_NATIVE();
void __wloger_INIT();

std::vector<std::thread> analize_threads;
std::thread send_thread;
bool stop_sender = false;

struct __profiler_t
{
        size_t acc = 0;
        size_t l_acc = 0;
        size_t cnt = 0;
#ifdef WL_TYMETRACE_THREAD_SEPARATED
        size_t thid = 0;
#endif
        std::string name = "";

        void lock()
        {
                while (flag.test_and_set(std::memory_order_acquire))
                        ;
        }
        void unlock()
        {
                flag.clear(std::memory_order_release);
        }

        std::atomic_flag flag{};

        __profiler_t()
        {
                flag.clear();
        }
};

struct __profiler_srack_t
{
        std::vector<__profiler_t*> stack;

        void lock()
        {
                while (flag.test_and_set(std::memory_order_acquire))
                        ;
        }
        void unlock()
        {
                flag.clear(std::memory_order_release);
        }

        std::atomic_flag flag{};

        __profiler_srack_t()
        {
                flag.clear();
        }
};

struct __wlog__locker
{
        void lock()
        {
                while (flag.test_and_set(std::memory_order_acquire))
                        ;
        }
        void unlock()
        {
                flag.clear(std::memory_order_release);
        }

        __wlog__locker()
        {
                flag.clear();
        }

        std::atomic_flag flag{};

        struct __wlog__lock_guard_t
        {
                __wlog__lock_guard_t(__wlog__locker* ori)
                {
                        parent = ori;
                        parent->lock();
                }
                __wlog__locker* parent;
                ~__wlog__lock_guard_t()
                {
                        parent->unlock();
                }
        };

        __wlog__lock_guard_t lock_guard()
        {
                return __wlog__lock_guard_t(this);
        }
};

struct Guard
{
#ifdef WL_TYMETRACE_THREAD_SEPARATED
        std::unordered_map<size_t, __profiler_t*> __profiler = std::unordered_map<size_t, __profiler_t*>();
#else
        std::unordered_map<unsigned int, __profiler_t*> __profiler = std::unordered_map<unsigned int, __profiler_t*>();
#endif
        std::unordered_map<size_t, __profiler_srack_t*> __profiler_stack = std::unordered_map<size_t, __profiler_srack_t*>();

        __wlog__locker* locker = new __wlog__locker();

        Guard()
        {
                __wloger_INIT();
        }

        ~Guard()
        {
                if (!stop_sender)
                {
                        stop_sender = true;
                        for (auto& analize_threads : analize_threads)
                                if (analize_threads.joinable())
                                        analize_threads.join();
                        if (send_thread.joinable())
                                send_thread.join();
                }
        }
};

Guard guard = Guard();



__WL_START_TYMETRACE_guard_t::__WL_START_TYMETRACE_guard_t(unsigned int&& h, std::string&& func) : h(h)
{
        union tid_t
        {
            std::thread::id t_id;
            size_t ptr = 0;
        } tid;
        tid.t_id =  std::this_thread::get_id();

        guard.locker->lock();

#ifdef WL_TYMETRACE_THREAD_SEPARATED
        auto r = guard.__profiler.emplace(tid.ptr ^ (h << 32), nullptr);
#else
        auto r = guard.__profiler.emplace(h, nullptr);
#endif
        auto r_s = guard.__profiler_stack.emplace(tid.ptr, nullptr);

        if (r.second)
        {
                r.first->second = new __profiler_t();
                r.first->second->acc = 0;
                r.first->second->l_acc = 0;
                r.first->second->cnt = 0;
                r.first->second->name = func;
#ifdef WL_TYMETRACE_THREAD_SEPARATED
                r.first->second->thid = tid.ptr;
#endif
        }
        imp__profiler = r.first->second;
        if (r_s.second)
        {
                r_s.first->second = new __profiler_srack_t();
        }
        imp__profiler_srack = r_s.first->second;
        r_s.first->second->stack.push_back(r.first->second);

        guard.locker->unlock();
        __WL__TIMER__START = std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

__WL_START_TYMETRACE_guard_t::~__WL_START_TYMETRACE_guard_t()
{
        auto acc = std::chrono::high_resolution_clock::now().time_since_epoch().count() - __WL__TIMER__START;

        auto r = (__profiler_t*)imp__profiler;
        auto r_s = (__profiler_srack_t*)imp__profiler_srack;
        
        r_s->lock();
        r_s->stack.pop_back();
        if (r_s->stack.size())
        {
                r_s->stack.back()->lock();
                r_s->stack.back()->l_acc += acc;
                r_s->stack.back()->unlock();
        }
        r_s->unlock();

        r->lock();
        r->acc += acc;
        r->cnt += 1;
        r->unlock();
}

#if WLOG_COMPILER_GCC
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)
#endif

unsigned char __wlog_level = 0xffu;

unsigned char __wlog_get_log_level()
{
        return __wlog_level;
}

void __wlog_set_log_level(unsigned int val)
{
        __wlog_level = val;
}

__generate_prefix_func_type __wloger_generate_prefix_func;

__MESSAGE __BAD_BUFFER(nullptr);

struct __wlog__logger_data
{
        std::vector<std::ostream*> out_streams;
        std::string name;
        std::vector<wlmesasge_t*> messages;
        __wlog__locker locker;
};

__wlog__logger_data* __logers[0x100];

struct __MESSAGE_DATA
{
        std::stringstream message;
        size_t ns;
        std::string file_name;
        std::string func_name;
        std::string cond_str;
        unsigned int line;
        unsigned int level;
        uint32_t straem_id;
        bool in_process;

        void lock()
        {
                while (flag.test_and_set(std::memory_order_acquire))
                        ;
        }
        void unlock()
        {
                flag.clear(std::memory_order_release);
        }

        __MESSAGE_DATA()
        {
                flag.clear();
        }

        std::atomic_flag flag{};

        struct lock_guard_t
        {
                lock_guard_t(__MESSAGE_DATA* ori)
                {
                        parent = ori;
                        parent->lock();
                }
                __MESSAGE_DATA* parent;
                ~lock_guard_t()
                {
                        parent->unlock();
                }
        };

        lock_guard_t lock_guard()
        {
                return lock_guard_t(this);
        }
};

__MESSAGE::__MESSAGE(void* dt)
{
        data = dt;
        if (!data)
                return;
        auto message = (__MESSAGE_DATA*)data;
        message->lock_guard();
        message->in_process = true;
}

void __MESSAGE::print(std::string str)
{
        if (!data)
                return;
        auto message = (__MESSAGE_DATA*)data;
        message->lock_guard();
        message->in_process = true;

        message->message << str;
}

__MESSAGE::~__MESSAGE()
{
        if (!data)
                return;
        auto message = (__MESSAGE_DATA*)data;
        message->lock_guard();
        message->in_process = false;
        __logers[message->level]->locker.lock();
        __logers[message->level]->messages.push_back(message);
        __logers[message->level]->locker.unlock();
#ifdef WLOGER_LAZY
        WLOG_FLUSH;
#endif
}

void __wloger_generate_loger(unsigned int level, std::string name)
{
        if (__logers[level] == nullptr)
        {
                __logers[level] = new __wlog__logger_data();
                __logers[level]->name = name;
        }
}

void __wloger_rename_loger(unsigned int level, std::string name)
{
        if (__logers[level] == nullptr)
        {
                __logers[level] = new __wlog__logger_data();
        }
        __logers[level]->name = name;
}

bool __wloger_cond(unsigned int level)
{
        return __logers[level] != nullptr;
}

bool __wloger_attach_stream(unsigned int level, std::ostream* stream)
{
        if (__wloger_cond(level))
        {
                auto* strams = &__logers[level]->out_streams;
                bool unic = true;
                for (int i = 0; i < strams->size() && unic; i++)
                        unic = (stream != strams->operator[](i));
                if (unic)
                        __logers[level]->out_streams.push_back(stream);
        }
        else
                return false;
        return true;
}

bool __wloger_detach_stream(unsigned int level, std::ostream* stream)
{
        if (__wloger_cond(level))
        {
                auto s = &__logers[level]->out_streams;
                for (int i = 0; i < s->size(); i++)
                        if (s->operator[](i) == stream)
                        {
                                s->erase(s->begin() + i);
                                return true;
                        }
        }
        return false;
}

static std::string __base_generate_prefix_func(
        unsigned int level, std::string file, std::string func, std::string cond, unsigned int line, size_t ns, uint32_t straem_id)
{
        size_t lenght = file.length();
        std::string _file = "";
        for (size_t i = 0; i < lenght; i++)
        {
                _file.append(1, file[i]);
                if (file[i] == '\\' || file[i] == '/')
                        _file.clear();
        };

        static const size_t st_offset = size_t(wlogger_start_data_time.time_since_epoch().count() * __tc::st2hrt) % size_t(1e10);
        int us = size_t((ns - wlogger_start_ns + st_offset) * __tc::hrt2us) % 1000000;

        auto tm = wlogger_start_data_time + std::chrono::system_clock::duration(size_t((ns - wlogger_start_ns) * __tc::hrt2st));

        std::time_t tp = std::chrono::system_clock::to_time_t(tm);
        char buff[256];
        auto lct = std::localtime(&tp);
        auto count = snprintf(buff, 256, "[%.2i:%.2i:%.2i.%.6i] %8d ", lct->tm_hour, lct->tm_min, lct->tm_sec, us, straem_id);

        std::string out;
        out.append(buff);
        out.append(_file);
        out.append(":");
        out.append(std::to_string(line));
        out.append(" ");
        if (func.size() > 33)
                func = "..." + func.substr(func.size() - 30, func.size());
        out.append(func);
        if (cond == "true")
                out.append(" ");
        else
        {
                out.append("if(");
                out.append(cond);
                out.append("): ");
        }

        if (__wloger_cond(level))
        {
                auto name = __logers[level]->name;
                if (name.length() > 0)
                {
                        out.append(name);
                        out.append(": ");
                }
        }
        return out;
}

std::string generate_prefix_func_from_message(wlmesasge_t* m)
{
        return __wloger_generate_prefix_func(m->level, m->file_name, m->func_name, m->cond_str, m->line, m->ns, m->straem_id);
}

static __wlog__locker __wloager_analize_locker;

struct __wloager_analize_res_t
{
        size_t id;
        std::unordered_map<std::ostream*, std::string> str_buffers;
};

std::list<__wloager_analize_res_t> __wloager_analize_ress;

void __wloger_analize()
{
        static __wlog__locker locker1;

        std::vector<unsigned int> loger__buffers__levels;
        std::vector<std::vector<wlmesasge_t*>> loger__buffers__queue;
        std::vector<uint32_t> loger__buffers__lasts;
        size_t totmescount = 0;
        static size_t id1 = 0;
        locker1.lock();
        __wloager_analize_res_t res;
        res.id = id1;
        loger__buffers__queue.reserve(32);
        loger__buffers__lasts.reserve(32);
        for (int level = 0; level < 0x100; ++level)
                if (__logers[level] != nullptr)
                {
                        auto data = __logers[level];
                        {
                                data->locker.lock();
                                if (data->messages.size() > 0)
                                {
                                        totmescount += data->messages.size();
                                        loger__buffers__queue.push_back(std::move(data->messages));
                                        loger__buffers__levels.push_back(level);
                                        data->messages = std::vector<wlmesasge_t*>();
                                        loger__buffers__lasts.push_back(0);
                                }
                                data->locker.unlock();
                        }
                }
        if (loger__buffers__queue.size() > 0)
                ++id1;
        locker1.unlock();
        if (loger__buffers__queue.size() == 0)
                return;

        std::unordered_map<std::ostream*, std::string>& str_buffers = res.str_buffers;

        for (int level = 0; level < 0x100; ++level)
                if (__logers[level] != nullptr)
                        for (auto stream : __logers[level]->out_streams)
                        {
                                str_buffers[stream] = "";
                        }

        size_t size = loger__buffers__queue.size();

        struct HeapItem
        {
                wlmesasge_t* message;
                size_t buffer_idx; // Индекс буфера в loger__buffers__queue
                size_t next_msg_idx; // Индекс следующего сообщения в этом буфере

                HeapItem(wlmesasge_t* msg, size_t buf_idx, size_t next_idx) : message(msg), buffer_idx(buf_idx), next_msg_idx(next_idx) {}
        };

        // Компаратор для min-heap (по возрастанию ns)
        auto heap_cmp = [](const HeapItem& a, const HeapItem& b)
        {
                return a.message->ns > b.message->ns; // Для min-heap используем "больше"
        };

        using MinHeap = std::priority_queue<HeapItem, std::vector<HeapItem>, decltype(heap_cmp)>;
        MinHeap heap(heap_cmp);

        // Инициализация кучи: первый элемент каждого буфера
        for (size_t i = 0; i < loger__buffers__queue.size(); ++i)
        {
                if (!loger__buffers__queue[i].empty())
                {
                        heap.emplace(loger__buffers__queue[i][0],
                                     i,
                                     1 // Следующее сообщение будет по индексу 1
                        );
                }
        }

        std::string str_buffer;
        str_buffer.reserve(1024);
        while (!heap.empty())
        {
                HeapItem item = heap.top();
                heap.pop();
                wlmesasge_t* best = item.message;
                const size_t buf_idx = item.buffer_idx;
                const size_t next_idx = item.next_msg_idx;
                auto& buffer = best->message;

                if (next_idx < loger__buffers__queue[buf_idx].size())
                {
                        heap.emplace(loger__buffers__queue[buf_idx][next_idx], buf_idx, next_idx + 1);
                }

                if (!buffer)
                        continue;
                str_buffer.clear();
                str_buffer.append(generate_prefix_func_from_message(best));
                int i = 0;
                std::string str = " ";
                std::string ostr = " ";
                std::getline(buffer, str);
                while (buffer)
                {
                        ostr = str;
                        std::getline(buffer, str);
                        if (buffer || ostr.size() > 0)
                        {
                                if (i != 0)
                                        str_buffer.append("                  ");

                                str_buffer.append(ostr).append("\n");
                                i++;
                        }
                }

                for (auto stream : __logers[best->level]->out_streams)
                {
                        if (str_buffers[stream].size() == 0)
                                str_buffers[stream].reserve(1024 * 1024);
                        str_buffers[stream].append(str_buffer);
                }

                buffer.clear();
                delete best;
                best = nullptr;
        }
        __wloager_analize_locker.lock();
        __wloager_analize_ress.push_back(std::move(res));
        __wloager_analize_locker.unlock();
}

void __wloger_send()
{
        static size_t id1 = 0;
        while (true)
        {
                __wloager_analize_locker.lock();

                if (__wloager_analize_ress.size() == 0)
                {
                        __wloager_analize_locker.unlock();
                        return;
                }

                __wloager_analize_ress.sort([](const __wloager_analize_res_t& a, const __wloager_analize_res_t& b) { return a.id < b.id; });

                if (__wloager_analize_ress.front().id != id1)
                {
                        __wloager_analize_locker.unlock();
                        return;
                }

                ++id1;

                __wloager_analize_res_t t = std::move(__wloager_analize_ress.front());

                __wloager_analize_ress.pop_front();

                __wloager_analize_locker.unlock();

                for (auto stream_pair : t.str_buffers)
                {
                        (*stream_pair.first) << stream_pair.second;
                        stream_pair.second = "";
                        stream_pair.first->flush();
                };
        }
}

void __wloger_analizer()
{

        bool _stop_sender = false;

        while (!_stop_sender)
        {
                _stop_sender = stop_sender;
                __wloger_analize();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
}

void __wloger_sender()
{

        bool _stop_sender = false;

        while (!_stop_sender)
        {
                _stop_sender = stop_sender;
                __wloger_send();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
}

std::string __wlog_profiler_get_stat()
{
        auto buff = std::stringstream();
        int i = 0;
        buff << "profiling statistic:[";
        for (auto& el : guard.__profiler)
        {
                auto& t = el.second;
                t->lock();
                buff << (i == 0 ? "" : ",") << "{"
                     << "\"Name\": \"" << t->name << "\","
#ifdef WL_TYMETRACE_THREAD_SEPARATED
                     << "\"Thread\": \"" << t->thid << "\","
#endif
                     << "\"Time_resolution\": \"ms\","
                     << "\"Total_calls\": " << t->cnt << ","
                     << "\"Total\": " << t->acc * __tc::hrt2ms << ","
                     << "\"OwnTotal\": " << (t->acc - t->l_acc) * __tc::hrt2ms << ","
                     << "\"Avg\": " << t->acc * __tc::hrt2ms / double(t->cnt) << ","
                     << "\"OwnAvg\": " << (t->acc - t->l_acc) * __tc::hrt2ms / double(t->cnt) << ""
                     << "}";
                t->unlock();
                ++i;
        }
        buff << "]";

        return buff.str();
}

void __wlog_profiler_push_stat()
{
        __wloger_generate_loger_buffer(WL_PROFILER, true, "true", "", "", 0) << __wlog_profiler_get_stat();
        __wlog_force_flush_buffers();
}

void __wlog_force_flush_buffers()
{
        __wloger_analize();
        __wloger_send();
}

typedef void (*__sig_fn_t)(int);

void __wloger_INIT()
{
        for (int i = 0; i < 0x100; ++i)
                __logers[i] = nullptr;
        __wloger_generate_prefix_func = __base_generate_prefix_func;
        WLOG_GENERATE_LOGER(WL_FATAL, "FATAL");
        WLOG_GENERATE_LOGER(WL_ERROR, "ERROR");
        WLOG_GENERATE_LOGER(WL_WARNING, "WARNING");
        WLOG_GENERATE_LOGER(WL_INFO, "INFO");
        WLOG_GENERATE_LOGER(WL_DEBUG, "DEBUG");
        WLOG_GENERATE_LOGER(WL_PROFILER, "PROFILER");
#ifndef WLOGER_LAZY
        for (int i = 0; i < 3; i++)
                analize_threads.push_back(std::thread(&__wloger_analizer));
        send_thread = std::thread(&__wloger_sender);
#endif
        __wloger_INIT_NATIVE();
}

__MESSAGE __wloger_generate_loger_buffer(unsigned int level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line)
{
        size_t ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        if (cond && __wlog_level >= level && __logers[level] != nullptr)
        {

                auto id = std::this_thread::get_id();
                uint32_t _id = *((uint32_t*)((void*)(&id)));

                wlmesasge_t* message = new wlmesasge_t;
                message->file_name = file;
                message->func_name = func;
                message->cond_str = cond_str;
                message->level = level;
                message->line = line;
                message->straem_id = _id;
                message->ns = ns;
                return __MESSAGE(message);
        }
        return __MESSAGE(nullptr);
}

void __wloger_printf(unsigned char level, bool cond, const char* cond_str, const char* file, const char* func, unsigned int line, const char* format, ...)
{
        size_t ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        if (!cond || !(__wlog_level >= level) || __logers[level] == nullptr)
                return;
        va_list args;
        va_start(args, format);
        int len = vsnprintf(NULL, 0, format, args);
        va_end(args);
        std::vector<char> buff(len + 5, 0);
        va_start(args, format);
        vsnprintf(buff.data(), len + 2, format, args);
        va_end(args);

        auto id = std::this_thread::get_id();
        uint32_t _id = *((uint32_t*)((void*)(&id)));

        wlmesasge_t* message = new wlmesasge_t;
        message->file_name = file;
        message->func_name = func;
        message->cond_str = cond_str;
        message->level = level;
        message->line = line;
        message->straem_id = _id;
        message->ns = ns;

        __MESSAGE(message).print(buff.data());
}

#include <fstream>

#if WLOG_OS_WINDOWS
const char sep = '\\';
#else
const char sep = '/';
#endif
void __wloger_generate_log_files(std::string path)
{
        std::time_t tp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        auto lct = std::localtime(&tp);
        char time_buff[512];
        snprintf(time_buff, 512, "Y%.2iM%.2iD%.2iTh%.2im%.2is%.2i", lct->tm_year % 100, lct->tm_mon, lct->tm_mday, lct->tm_hour, lct->tm_min, lct->tm_sec);
        for (int level = 0; level < 0x100; ++level)
                if (__logers[level] != nullptr)
                {
                        auto name = __logers[level]->name;
                        auto filename = path;

                        filename.append(name).append("_").append(time_buff).append(".log");
                        std::ofstream* fout = new std::ofstream(filename);

                        __wloger_attach_stream(level, fout);
                }
}

void __WLogerShutdown()
{
#ifndef WLOGER_LAZY
        if (!stop_sender)
        {
                stop_sender = true;
                for (auto& analize_threads : analize_threads)
                        if (analize_threads.joinable())
                                analize_threads.join();
                if (send_thread.joinable())
                        send_thread.join();
        }
#endif
        __wlog_force_flush_buffers();
}

#ifndef __OBJC__
void __wloger_INIT_NATIVE()
{
#define SIGNAL_HANDLER(SIGNAL)                                                                                                                              \
        static const __sig_fn_t __##SIGNAL##__base_handler = signal(SIGNAL,                                                                                 \
                                                                    [](int)                                                                                 \
                                                                    {                                                                                       \
                                                                            __wloger_generate_loger_buffer(WL_FATAL, true, "true", "SIGNAL_HANDLER", "", 0) \
                                                                                    << "Unhandled exception: " #SIGNAL;                                     \
                                                                            __WLogerShutdown();                                                             \
                                                                            signal(SIGNAL, __##SIGNAL##__base_handler);                                     \
                                                                            raise(SIGNAL);                                                                  \
                                                                    })

        // #ifdef SIGINT
        //     SIGNAL_HANDLER(SIGINT);
        // #endif
        // #ifdef SIGQUIT
        //     SIGNAL_HANDLER(SIGQUIT);
        // #endif
        // #ifdef SIGILL
        //     SIGNAL_HANDLER(SIGILL);
        // #endif
        // #ifdef SIGTRAP
        //     SIGNAL_HANDLER(SIGTRAP);
        // #endif
        // #ifdef SIGABRT
        //     SIGNAL_HANDLER(SIGABRT);
        // #endif
        // #ifdef SIGFPE
        //     SIGNAL_HANDLER(SIGFPE);
        // #endif
        // #ifdef SIGKILL
        //     SIGNAL_HANDLER(SIGKILL);
        // #endif
        // #ifdef SIGSEGV
        //     SIGNAL_HANDLER(SIGSEGV);
        // #endif
        // #ifdef SIGTERM
        //     SIGNAL_HANDLER(SIGTERM);
        // #endif

#undef SIGNAL_HANDLER
}
#endif
