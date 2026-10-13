#ifndef __EVENT_H__
#define __EVENT_H__

#include "data.h"
#include "function.hpp"
#include "data.h"
#include "function.hpp"

#include <unordered_map>
#include <vector>
#include <tuple>
#include <cstddef>
#include <utility>
#include <type_traits>

class Slot
{
public:
    using Wrapper = srv_function<void(const std::vector<Data>&)>;

    // ---- Подписка ----
    template<typename... ARGS>
    size_t Subscribe(srv_function<void(ARGS...)> func)
    {
        // Обёртка: превращает вектор Data в кортеж ARGS... и вызывает func.
        Wrapper wrapper = [fn = func](const std::vector<Data>& data) mutable
        {
            // Защита от несовпадения сигнатур (пользователь Emit'ит не то).
            if (data.size() != sizeof...(ARGS))
                return;

            std::tuple<std::decay_t<ARGS>...> tpl;

            size_t i = 0;
            std::apply([&](auto&&... args)
            {
                // ВАЖНО: ++i внутри фолда, а не после.
                ((data[i].to(args), ++i), ...);
            }, tpl);

            std::apply(fn, tpl);
        };

        // Генерируем уникальный id с линейным пробированием.
        size_t id = MakeBaseId<ARGS...>(func);
        while (subscribers.find(id) != subscribers.end())
            id = MixId(id);

        subscribers.emplace(id, std::move(wrapper));
        return id;
    }

    // ---- Отписка ----
    void Unsubscribe(size_t id)
    {
        subscribers.erase(id);
    }

    void Clear() { subscribers.clear(); }
    size_t Count() const { return subscribers.size(); }

    // ---- Генерация события ----
    template<typename... ARGS>
    void Emit(ARGS&&... args)
    {
        if (subscribers.empty())
            return;

        std::vector<Wrapper> snapshot;
        snapshot.reserve(subscribers.size());
        for (auto& [_, w] : subscribers)
            snapshot.push_back(w);
        
        std::vector<Data> data;
        data.reserve(sizeof...(ARGS));
        (data.emplace_back(std::forward<ARGS>(args)), ...);

        for (auto& w : snapshot)
            w(data);
    }

private:
    template<typename... ARGS>
    static size_t MakeBaseId(const srv_function<void(ARGS...)>& func)
    {
        // TGID(ARGS...) — типовая сигнатура, func.tid() — уникальный id инстанса.
        if constexpr (sizeof...(ARGS) > 0)
        {
            size_t id = TGID(ARGS...) ^ func.tid();
            id |= 1;
            return id;
        }
        else
        {
            size_t id = func.tid();
            id |= 1;
            return id;

        }
    }

    // Простой шаг хеша для разрешения коллизий (сплитмикс).
    static size_t MixId(size_t id)
    {
        id ^= id >> 33;
        id *= 0xff51afd7ed558ccdULL;
        id ^= id >> 33;
        id *= 0xc4ceb9fe1a85ec53ULL;
        id ^= id >> 33;
        if (id == 0) id = 1;
        return id;
    }

    std::unordered_map<size_t, Wrapper> subscribers;
};



#endif // __EVENT_H__