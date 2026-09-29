
#include <thread>
#include <atomic>

#include <functional>
#include <tuple>
#include <stdexcept>
#include <mutex>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>

#include "core/core.h"
#include "World/Camera.h"
#include "World/World.h"
#include "render/RenderSystem.h"
#include "render/window_bridges/winapi_opengl.h"
#include "World/StaticMeshActor.h"
#include "render/opengl/include.h"
#include "input/InputSystem.h"

#include "game/GameSystem.h"

#include <memory>

#include "../test/pmtiles.hpp"

Ref<World> main_world;
Ref<UI> main_ui;
Ref<Camera> main_camera;

class MyActor : public StaticMeshActor
{
    static Ref<Mesh> GetMesh()
    {
        static SpinLocker sp;
        static Ref<Mesh> mesh = nullptr;
        auto lock = sp.lock_guard();
        if(mesh)
        {
            return mesh;
        }

        mesh = MakeRef<Mesh>();
        mesh->name = "Cube";

        std::vector<MVector3f> positions = {MVector3f(-0.5f, -0.5f, 0.5f),
          MVector3f(0.5f, -0.5f, 0.5f),
          MVector3f(0.5f, 0.5f, 0.5f),
          MVector3f(-0.5f, 0.5f, 0.5f),
          MVector3f(-0.5f, -0.5f, -0.5f),
          MVector3f(0.5f, -0.5f, -0.5f),
          MVector3f(0.5f, 0.5f, -0.5f),
          MVector3f(-0.5f, 0.5f, -0.5f)};
        std::vector<MVector3f> normals = {MVector3f(0.0f, 0.0f, 1.0f),
          MVector3f(0.0f, 0.0f, -1.0f),
          MVector3f(0.0f, 1.0f, 0.0f),
          MVector3f(0.0f, -1.0f, 0.0f),
          MVector3f(1.0f, 0.0f, 0.0f),
          MVector3f(-1.0f, 0.0f, 0.0f)};
        std::vector<MVector3f> texCoords = {MVector3f(0.0f, 0.0f, 0.0f),
          MVector3f(1.0f, 0.0f, 0.0f),
          MVector3f(1.0f, 1.0f, 0.0f),
          MVector3f(0.0f, 1.0f, 0.0f)};
        std::vector<uint32_t> indices = {
          0, 1, 2, 2, 3, 0, 5, 4, 7, 7, 6, 5, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4, 1, 5, 6, 6, 2, 1, 4, 0, 3, 3, 7, 4};

        // Маппинг индексов для UV: 0, 1, 2 для первого треугольника, и 2, 3, 0 для второго
        std::vector<uint32_t> uvIndices = {0, 1, 2, 2, 3, 0};

        mesh->v.resize(36);
        for (size_t i = 0; i < indices.size(); i++)
        {
            uint32_t posIndex = indices[i];
            uint32_t normalIndex = i / 6;
            uint32_t texIndex = uvIndices[i % 6];

            mesh->v[i].v = positions[posIndex] * 0.01;
            mesh->v[i].vn = normals[normalIndex];
            mesh->v[i].vt = texCoords[texIndex];
        }
        mesh->f.resize(36);
        for (uint32_t i = 0; i < 36; i++)
        {
            mesh->f[i] = i;
        }
        return mesh;
    }
public:
    MyActor(const std::string &name = "StaticMeshActor") : StaticMeshActor(name)
    {
        Ref<StaticMeshComponent> comp = GetOwnedMeshComponent();
        comp->mesh = GetMesh();
    }

    virtual void Update(float dt) override
    {
        return;
    }

    MVector3f base_pos = {0.0, 0.0, 0.0};
    MVector3f target_pos = {2.0, 0.0, 0.0};
};

int main()
{
    std::ofstream log_file("log.log");
    WLOG_ATTACH_STREAM(WL_INFO, log_file);
    WLOG_ATTACH_STREAM(WL_DEBUG, log_file);
    WLOG_ATTACH_STREAM(WL_WARNING, log_file);
    WLOG_ATTACH_STREAM(WL_FATAL, log_file);
    WLOG_ATTACH_STREAM(WL_INFO, std::cout);
    WLOG_ATTACH_STREAM(WL_DEBUG, std::cout);
    WLOG_ATTACH_STREAM(WL_WARNING, std::cout);
    WLOG_ATTACH_STREAM(WL_FATAL, std::cout);

    std::ofstream prof_file("prof.log");
    WLOG_ATTACH_STREAM(WL_PROFILER, prof_file);


    auto r_system = ISystem::Get<RenderSystem>();
    auto c_system = ISystem::Get<CoreSystem>();
    auto g_system = ISystem::Get<GameSystem>();
    auto i_system = ISystem::Get<InputSystem>();

    main_world = MakeRef<World>();
    main_camera = MakeRef<Camera>();

    main_world->SetDefaultCamera(main_camera);

    main_camera->SetPosition({10.0, 0.0, 0.0});
    main_camera->SetTarget({0.0, 0.0, 0.0});

    auto GameWindow = r_system->MakeWindow("game");

    static Ref<RenderGraphBuilder> RGB = RenderGraphBuilder::MakeFromTarget(GameWindow);
    r_system->AddRGB(RGB);
    
    for(int i = 0; i < 250000; ++i)
    {
        const float dis = 12.5;
        auto actor = MakeRef<MyActor>("test");
        actor->base_pos = {
            rand() / float(RAND_MAX) * 2.0f * dis - dis,
            rand() / float(RAND_MAX) * 2.0f * dis - dis,
            rand() / float(RAND_MAX) * 2.0f * dis - dis
        };
        actor->target_pos = {
            rand() / float(RAND_MAX) * 2.0f * dis - dis,
            rand() / float(RAND_MAX) * 2.0f * dis - dis,
            rand() / float(RAND_MAX) * 2.0f * dis - dis
        };
        auto pos = (actor->base_pos + actor->target_pos) * 0.5;
        while(dis < !pos)
        {
            actor->base_pos = {
                rand() / float(RAND_MAX) * 2.0f * dis - dis,
                rand() / float(RAND_MAX) * 2.0f * dis - dis,
                rand() / float(RAND_MAX) * 2.0f * dis - dis
            };
            actor->target_pos = {
                rand() / float(RAND_MAX) * 2.0f * dis - dis,
                rand() / float(RAND_MAX) * 2.0f * dis - dis,
                rand() / float(RAND_MAX) * 2.0f * dis - dis
            };
            pos = (actor->base_pos + actor->target_pos) * 0.5;
        }
        actor->SetWorldPosition(pos);
        actor->SetCanTick(false);
        main_world->AddObject(actor);
        RGB->SetDirty();
    }

    g_system->AddWorld("main_world", main_world);
    g_system->ActivateWorld("main_world");

    r_system->Start();
    i_system->Start();
    Ref<IKeyboard> kb = i_system->GetKeyboard();
    auto root = RGB->Root();
    int w;
    int h;
    GameWindow->GetSize(w, h);
    auto worldRender = RGBWorldRender::Make(main_world, main_camera, w, h);
    root->BindInput(0, worldRender, 0);

    using ts_t = decltype(std::chrono::high_resolution_clock::now().time_since_epoch());
    ts_t ts{ts_t(0)};
    size_t i = 0;
    double T = 0;
    while (true)
    {
        i_system->Process();
        if (kb->IsKeyDown(KeyCode::Escape))
            break;

        GameWindow->GetSize(w, h);
        worldRender->_width = w;
        worldRender->_height = h;
        ts_t nts = std::chrono::high_resolution_clock::now().time_since_epoch();
        float dt = 0;
        if (ts.count() != 0)
        {
            dt = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(nts - ts).count();
        }
        T += dt;
        main_camera->SetPosition({10.0f * cosf(T / wm::PI * 0.5), 10.0f * sinf(T / wm::PI * 0.5), 0.0});
        main_camera->SetTarget({0.0, 0.0, 0.0});
        ts = nts;
        printf("%f                  \r", 1.0 / dt);
        {
            auto r_ret = r_system->Process();
            if (r_ret != 0)
                break;
        }
        if(i % 100 == 0)
        {
            WL_TYMETRACE_PUSH_TO_WLOG;
        }
    }
}