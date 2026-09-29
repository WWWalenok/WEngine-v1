#pragma once
#include <chrono>
#include <unordered_set>
#include "../core/core.h"
#include "../World/World.h"

class GameSystem : public ISystem
{
    DECLARE_SYSTEM(GameSystem);
    GameSystem() :
        mainThread(1),
        pool(32)
    {
        mainThread.send_delayed_repetable({1, 100}, this, &GameSystem::Update);
    }
    using ts_t = decltype(std::chrono::high_resolution_clock::now().time_since_epoch());
    ts_t ts {ts_t(0)};
    void Update()
    {
        WL_START_TYMETRACE;
        ts_t nts = std::chrono::high_resolution_clock::now().time_since_epoch();
        float dt = 0;
        if(ts.count() != 0)
        {
            dt = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>> (nts - ts).count();
        }
        ts = nts;

        for (auto w : activeWorlds)
        {
            w->Update(dt);
        }

    }

    ThreadPool mainThread;
    ThreadPool pool;

    std::unordered_map<std::string, Ref<World>> worlds;

    std::unordered_set<Ref<World>> activeWorlds;
public:

    ThreadPool& Pool() { return pool; };

    void AddWorld(std::string name, Ref<World> world)
    {
        worlds.insert({name, world});
    }

    void ActivateWorld(std::string name)
    {
        auto f = worlds.find(name);
        if(f != worlds.end())
            activeWorlds.insert(f->second);
    }

    void DeactivateWorld(std::string name)
    {
        auto f = worlds.find(name);
        if(f != worlds.end())
            activeWorlds.erase(f->second);
    }

};