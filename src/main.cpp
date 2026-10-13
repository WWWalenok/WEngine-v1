#include "render/RenderGraphBuilder.h"

#include "UI/UI.h"
#include "Text/FontTable.h"


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

#include "core/CoreSystem.h"
#include "render/IWindowHelper.h"
#include "render/IRHIBridge.h"
#include "render/IRHIHelper.h"

#include "render/RenderSystem.h"
#include "World/StaticMeshActor.h"
#include "input/InputSystem.h"
#include "graphic/Mesh.h"

#include "graphic/Map.h"

#include "game/GameSystem.h"

#include <memory>

Ref<World> main_world;
Ref<Camera> main_camera;
Ref<CanvasWidget> Hud;

const float dis = 5;
const int min_link_count = 1;

static Ref<Mesh> GetStarMesh()
{
    static SpinLocker sp;
    static Ref<Mesh> mesh = nullptr;
    auto lock = sp.lock_guard();
    if (mesh)
        return mesh;

    mesh = MakeRef<Mesh>();
    mesh->name = "StarCube";

    const std::vector<MVector3f> positions = {{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}};
    const std::vector<MVector3f> normals = {{0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}};
    const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 5, 4, 7, 7, 6, 5, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4, 1, 5, 6, 6, 2, 1, 4, 0, 3, 3, 7, 4};

    mesh->v.resize(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
    {
        mesh->v[i].v = positions[indices[i]]; // unit cube, size via transform
        mesh->v[i].vn = normals[i / 6];
        mesh->v[i].vt = MVector3f(0, 0, 0);
    }
    mesh->f.resize(indices.size());
    for (uint32_t i = 0; i < indices.size(); ++i)
        mesh->f[i] = i;
    return mesh;
}

static Ref<Mesh> GetShipMesh()
{
    static SpinLocker sp;
    static Ref<Mesh> mesh = nullptr;
    auto lock = sp.lock_guard();
    if (mesh)
        return mesh;

    mesh = MakeRef<Mesh>();
    mesh->name = "ShipArrow";

    const std::vector<MVector3f> pos = {
      {1.0f, 0.0f, 0.0f}, // 0 nose (+X)
      {-0.5f, 0.35f, 0.35f}, // 1 base
      {-0.5f, -0.35f, 0.35f}, // 2
      {-0.5f, -0.35f, -0.35f}, // 3
      {-0.5f, 0.35f, -0.35f}, // 4
    };
    const std::vector<MVector3f> nrm = {{1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}};
    const std::vector<uint32_t> idx = {
      0,
      1,
      2,
      0,
      2,
      3,
      0,
      3,
      4,
      0,
      4,
      1, // sides
      1,
      4,
      3,
      3,
      2,
      1 // base
    };

    mesh->v.resize(idx.size());
    for (size_t i = 0; i < idx.size(); ++i)
    {
        mesh->v[i].v = pos[idx[i]];
        mesh->v[i].vn = nrm[i / 3];
        mesh->v[i].vt = MVector3f(0, 0, 0);
    }
    mesh->f.resize(idx.size());
    for (uint32_t i = 0; i < idx.size(); ++i)
        mesh->f[i] = i;
    return mesh;
}

static Ref<Mesh> GetLinkMesh()
{
    static SpinLocker sp;
    static Ref<Mesh> mesh = nullptr;
    auto lock = sp.lock_guard();
    if (mesh)
        return mesh;

    mesh = MakeRef<Mesh>();
    mesh->name = "LinkBox";

    // Тот же юнит-куб [-0.5..0.5]^3 — вдоль +X будем растягивать.
    const std::vector<MVector3f> positions = {{-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}};
    const std::vector<MVector3f> normals = {{0, 0, 1}, {0, 0, -1}, {0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}};
    const std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 5, 4, 7, 7, 6, 5, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4, 1, 5, 6, 6, 2, 1, 4, 0, 3, 3, 7, 4};

    mesh->v.resize(indices.size());
    for (size_t i = 0; i < indices.size(); ++i)
    {
        mesh->v[i].v = positions[indices[i]];
        mesh->v[i].vn = normals[i / 6];
        mesh->v[i].vt = MVector3f(0, 0, 0);
    }
    mesh->f.resize(indices.size());
    for (uint32_t i = 0; i < indices.size(); ++i)
        mesh->f[i] = i;

    return mesh;
}

static const MVector3f StarMaterialsColors[] = {
  {1.0f, 0.90f, 0.50f}, // yellow
  {1.0f, 0.60f, 0.30f}, // orange
  {0.95f, 0.95f, 1.00f}, // white
  {0.55f, 0.75f, 1.00f}, // blue
  {1.0f, 0.40f, 0.40f}, // red
};
static constexpr int kStarMaterials = sizeof(StarMaterialsColors) / sizeof(StarMaterialsColors[0]);
static const MVector3f ShipMaterialsColors[] = {
  {0.75f, 0.90f, 1.00f},
  {0.95f, 0.85f, 0.55f},
  {0.60f, 1.00f, 0.70f},
  {0.00f, 0.20f, 1.00f},
};
static constexpr int kShipMaterials = sizeof(ShipMaterialsColors) / sizeof(ShipMaterialsColors[0]);

static Ref<Material> GetStarMaterial(int k)
{
    static SpinLocker sp;
    static Ref<Material> mats[kStarMaterials];
    static bool init = false;
    auto lock = sp.lock_guard();
    if (!init)
    {
        for (int i = 0; i < kStarMaterials; ++i)
        {
            auto m = MakeRef<Material>();
            m->name = "StarMat" + std::to_string(i);
            m->maps["diffuse"] = MakeRef<Map>(StarMaterialsColors[i]);
            mats[i] = m;
        }
        init = true;
    }
    return mats[((k % kStarMaterials) + kStarMaterials) % kStarMaterials];
}

static Ref<Material> GetShipMaterial(int k)
{
    static SpinLocker sp;
    static Ref<Material> mats[kShipMaterials];
    static bool init = false;
    auto lock = sp.lock_guard();
    if (!init)
    {
        for (int i = 0; i < kShipMaterials; ++i)
        {
            auto m = MakeRef<Material>();
            m->name = "ShipMat" + std::to_string(i);
            m->maps["diffuse"] = MakeRef<Map>(ShipMaterialsColors[i]);
            mats[i] = m;
        }
        init = true;
    }
    return mats[((k % kShipMaterials) + kShipMaterials) % kShipMaterials];
}

static Ref<Material> GetLinkMaterial()
{
    static SpinLocker sp;
    static Ref<Material> mat = nullptr;
    auto lock = sp.lock_guard();
    if (mat)
        return mat;
    mat = MakeRef<Material>();
    mat->name = "LinkMat";
    mat->maps["diffuse"] = MakeRef<Map>(MVector3f{0.35f, 0.55f, 1.0f});
    return mat;
}

namespace data_core {

template <class T, class D>
bool operator<(const Pointer<T, D>& lhs,
               const Pointer<T, D>& rhs) noexcept
{
    return lhs.get() < rhs.get();
}

} // namespace data_core


template<typename S>
static auto select_random(const S &s, size_t n) {
    auto it = std::begin(s);
    // 'advance' the iterator n times
    auto r = rand() % n;
    std::advance(it, r);
    return it;
}

class StarSystem;
class StarActor : public StaticMeshActor
{
public:
    StarActor(int material_index = 0, const std::string& name = "Star") : StaticMeshActor(name)
    {
        auto comp = GetOwnedMeshComponent();
        comp->mesh = GetStarMesh();
        comp->material = GetStarMaterial(material_index);
    }

    void Build_neabre();

    float Radius = 1.0f; // gameplay data, visual size = world scale
    // Ref<StarActor> _neabre[4];
    std::set<Ref<StarActor>> _neabre;
};

class ShipActor;

class StarSystem
{
public:
    static StarSystem& Get()
    {
        static StarSystem s;
        return s;
    }

    void SetWorld(Ref<World> w)
    {
        _world = w;
    }
    Ref<World> GetWorld() const
    {
        return _world;
    }

    void AddStar(Ref<StarActor> ref)
    {
        _star_list.push_back(ref);
        auto pos = ref->GetWorldPosition();
        auto r = ref->Radius;
        _stars.Insert(ref,  {{pos[0] - r, pos[1] - r, pos[2] - r}, {pos[0] + r, pos[1] + r, pos[2] + r}});
    }

    Ref<StarActor> RandomStar(Ref<StarActor> base = nullptr) const
    {
        if (_stars.IsEmpty())
            return nullptr;
        for (int i = 0; i < 32; ++i)
        {
            auto s = _star_list[std::rand() % _star_list.size()];
            if (s != base)
                return s;
        }
        return nullptr;
    }

    void SpawnShip(Ref<StarActor> from, Ref<StarActor> to);

    void SpawnShipAtRandomStar()
    {
        auto a = RandomStar(nullptr);
        if (!a)
            return;
        auto b = *std::next(a->_neabre.begin(), rand() % a->_neabre.size());
        if (!b)
            return;
        SpawnShip(a, b);
    }

    Ref<World> _world;
    std::vector<Ref<StarActor>> _star_list;
    TypedRTree<Ref<StarActor>, float, 3> _stars;
    std::vector<Ref<ShipActor>> _ships;
};

class ShipActor : public StaticMeshActor
{
public:
    ShipActor(int material_index = 0, const std::string& name = "Ship") : StaticMeshActor(name)
    {
        auto comp = GetOwnedMeshComponent();
        comp->mesh = GetShipMesh();
        comp->material = GetShipMaterial(material_index);
        SetCanTick(true);
    }

    void DepartTo(Ref<StarActor> from, Ref<StarActor> to)
    {
        if (!to)
            return;
        _prew = _target;
        _target = to;

        _startPos = GetWorldPosition();
        _endPos = to->GetWorldPosition();

        MVector3f d = _endPos - _startPos;
        float dist = !d; // magnitude
        _duration = std::max(0.001f, dist / std::max(0.001f, Speed));
        _elapsed = 0.0f;

        if (dist > 1e-5f)
        {
            MVector3f n = d / dist;
            float yaw_deg = std::atan2(n[1], n[0]) * 180.0f / (float)wm::PI;
            float pitch_deg = std::atan2(-n[2], std::sqrt(n[0] * n[0] + n[1] * n[1])) * 180.0f / (float)wm::PI;
            SetWorldRotation(MVector3f(0.0f, pitch_deg, yaw_deg));
        }
    }
    virtual void Update(float dt) override
    {
        if (!_target)
            return;
        if (_duration <= 0.0f)
        {
            _duration = 1.0;
            return;
        }

        _elapsed += dt;
        float t = std::min(1.0f, _elapsed / _duration);

        SetWorldPosition(_startPos + (_endPos - _startPos) * t);

        if (t >= 1.0f)
        {
            Ref<StarActor> next = nullptr;
            for(int i = 0; i < 100 && !next; ++i)
            {
                next = *select_random(_target->_neabre, _target->_neabre.size());
                if(next == _prew && _target->_neabre.size() > 1)
                {
                    next = nullptr;
                }
            }
            if(!next)
            {
                next = StarSystem::Get().RandomStar(_target);
            };
            DepartTo(_target, next);
            
        }
    }

    float Speed = 0.1f;

private:
    Ref<StarActor> GetTargetStar() const
    {
        return nullptr;
    } // placeholder
    Ref<StarActor> _target = nullptr;
    Ref<StarActor> _prew = nullptr;
    MVector3f _startPos;
    MVector3f _endPos;
    float _duration = 0.0f;
    float _elapsed = 0.0f;
};

void StarSystem::SpawnShip(Ref<StarActor> from, Ref<StarActor> to)
{
    if (!_world || !from || !to)
        return;
    int mi = std::rand() % kShipMaterials;
    auto ship = MakeRef<ShipActor>(mi, "ship");
    ship->SetWorldPosition(from->GetWorldPosition());
    ship->Speed = rand() / float(RAND_MAX) * 0.25 + 0.25;
    float s = (rand() / float(RAND_MAX) * 0.5f + 0.75f) * 0.015;
    ship->SetWorldScale({s, s, s});
    ship->DepartTo(from, to);
    _world->AddObject(ship);
    _ships.push_back(ship);
}

void StarActor::Build_neabre()
{
}

class LinkActor : public StaticMeshActor
{
public:
    LinkActor(Ref<StarActor> a, Ref<StarActor> b, const std::string& name = "link")
        : StaticMeshActor(name)
    {
        auto comp = GetOwnedMeshComponent();
        comp->mesh = GetLinkMesh();
        comp->material = GetLinkMaterial();

        const MVector3f pa = a->GetWorldPosition();
        const MVector3f pb = b->GetWorldPosition();

        const MVector3f mid = (pa + pb) * 0.5f;
        const MVector3f d = pb - pa;
        const float len = !d; // magnitude (см. существующий код)

        SetWorldPosition(mid);

        if (len > 1e-5f)
        {
            const MVector3f n = d / len;
            // Z-up: yaw вокруг Z, pitch вокруг Y — та же конвенция, что у ShipActor.
            const float yaw_deg   = std::atan2(n[1], n[0]) * 180.0f / (float)wm::PI;
            const float pitch_deg = std::atan2(-n[2], std::sqrt(n[0] * n[0] + n[1] * n[1])) * 180.0f / (float)wm::PI;
            SetWorldRotation(MVector3f(0.0f, pitch_deg, yaw_deg));
        }

        const float thickness = 0.0015f;
        SetWorldScale({len, thickness, thickness});

        SetCanTick(false);
    }
};

int main()
{
    vecp::font_t font{};
    std::vector<uint8_t> blob;
    auto f = fopen("F:\\Repos\\WEngine-v1\\ARIAL.wfn0", "rb");
    fseek(f, 0L, SEEK_END);
    auto file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);
    blob.resize(file_size);
    fread(blob.data(), file_size, 1, f);

    FontTable::Get().LoadVecpFont("ARIAL", blob.data(), blob.size());
    
    auto ARIAL_16 = FontTable::Get().GetFont("ARIAL", 16);

    FontTable::Get().SetDefault("ARIAL");

    std::ofstream log_file("log.log");
    WLOG_ATTACH_STREAM(WL_INFO, log_file);
    WLOG_ATTACH_STREAM(WL_DEBUG, log_file);
    WLOG_ATTACH_STREAM(WL_WARNING, log_file);
    WLOG_ATTACH_STREAM(WL_FATAL, log_file);
    WLOG_ATTACH_STREAM(WL_INFO, std::cout);
    WLOG_ATTACH_STREAM(WL_WARNING, std::cout);
    WLOG_ATTACH_STREAM(WL_FATAL, std::cout);

    std::ofstream prof_file("prof.log");
    WLOG_ATTACH_STREAM(WL_PROFILER, prof_file);

    srand(time(nullptr) ^ size_t(&log_file));


    auto r_system = ISystem::Get<RenderSystem>();
    auto c_system = ISystem::Get<CoreSystem>();
    auto g_system = ISystem::Get<GameSystem>();
    auto i_system = ISystem::Get<InputSystem>();

    main_world = MakeRef<World>();
    main_camera = MakeRef<Camera>();
    Hud = MakeRef<CanvasWidget>();
    auto btn = MakeRef<Button>();
    auto bar = MakeRef<FillBar>();
    btn->BorderThickness = 10;
    btn->CornerRadius = 25;

    Hud->Add(btn, CanvasWidget::Slot(
        0,
        {{25, 25}, CanvasWidget::Slot::Absolute},
        {{200, 75}, CanvasWidget::Slot::Absolute}
    ));

    Hud->Add(bar, CanvasWidget::Slot(
        0,
        {{-25, -25}, CanvasWidget::Slot::Absolute},
        {{-25, -25}, CanvasWidget::Slot::Absolute}
    ));

    main_world->SetDefaultCamera(main_camera);
    StarSystem::Get().SetWorld(main_world);

    main_camera->SetPosition({10.0, 0.0, 0.0});
    main_camera->SetTarget({0.0, 0.0, 0.0});

    auto GameWindow = r_system->MakeWindow("game");

    static Ref<RenderGraphBuilder> RGB = RenderGraphBuilder::MakeFromTarget(GameWindow);
    r_system->AddRGB(RGB);

    std::unordered_set<Ref<StarActor>> all_stars;
    for (int i = 0; i < 2500; ++i)
    {
        auto actor = MakeRef<StarActor>(std::rand() % kStarMaterials, "star");
        MVector3f pos = (MVector3f{rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis} + MVector3f{rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis}) * 0.5;
        while (dis * dis < pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2])
        {
            pos = (MVector3f{rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis} + MVector3f{rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis, rand() / float(RAND_MAX) * 2.0f * dis - dis}) * 0.5;
        }
        actor->SetWorldPosition(pos);
        float s = (rand() / float(RAND_MAX) * 0.5f + 0.75f) * 0.01;
        actor->Radius = s;
        actor->SetWorldScale({s, s, s});
        actor->SetCanTick(false);
        main_world->AddObject(actor);
        StarSystem::Get().AddStar(actor);
        RGB->SetDirty();
        all_stars.insert(actor);
    }
    {
        auto wrappers = StarSystem::Get()._stars.GetAllWrappers();
        auto AllStarCount = wrappers.size();
        for(int i = 0 ; i < AllStarCount * 2; ++i)
        {
            auto W = wrappers[std::rand() % AllStarCount];
            auto S = W->GetData();
            auto pos = S->GetWorldPosition();
            auto res = StarSystem::Get()._stars.KNNSearchWrappers({pos[0], pos[1], pos[2]}, AllStarCount * 0.02);
            for(auto w : res)
            {
                auto s = w->GetData();
                if(s == S)
                    continue;
                auto pos2 = s->GetWorldPosition();
                auto delta = pos2 - pos;
                auto dist = !delta;
                pos2 = pos + delta * (dis / dist * 0.02 + 0.94);
                s->SetWorldPosition(pos2);
                auto r = s->Radius;
                w->SetBoundingBox({{pos2[0] - r, pos2[1] - r, pos2[2] - r}, {pos2[0] + r, pos2[1] + r, pos2[2] + r}});
            }
            RGB->SetDirty();
        }
    }
    std::list<Ref<StarActor>> q;
    q.push_back(StarSystem::Get().RandomStar());
    bool is_first = true;
    auto StartStar = q.front();
    auto StartStarPos = StartStar->GetWorldPosition();
    auto AllStarsCount = StarSystem::Get()._star_list.size();
    auto AllStars = StarSystem::Get()._stars.KNNSearch({StartStarPos[0], StartStarPos[1], StartStarPos[2]}, AllStarsCount);
    size_t AllStarsIter = 0;
    while (!q.empty() || !all_stars.empty())
    {
        if(q.empty())
        {
            for(; AllStarsIter < AllStarsCount; ++AllStarsIter)
            {
                auto s = AllStars[AllStarsIter];
                if (s->_neabre.size() == 0)
                {
                    q.push_back(s);
                    break;
                }
            }
        }
        auto S = q.front();
        q.pop_front();
        all_stars.erase(S);
        if(S->_neabre.size() > min_link_count)
            continue;
        auto pos = S->GetWorldPosition();
        int O = min_link_count * 5;
        auto res = StarSystem::Get()._stars.KNNSearch({pos[0], pos[1], pos[2]}, O);
        while(S->_neabre.empty() && !is_first)
        {
            auto res2 = StarSystem::Get()._stars.KNNSearch({pos[0], pos[1], pos[2]}, O);
            O *= 2;
            for(auto s : res2)
            {
                if(s != S && s->_neabre.size() > 0)
                {
                    S->_neabre.insert(s);
                    s->_neabre.insert(S);
                    break;
                }
            }
        }
        int t = min_link_count;
        const auto Size = res.size();
        while(S->_neabre.size() < min_link_count)
        {
            for(size_t j = 0; j < Size && S->_neabre.size() < min_link_count; ++j)
            {
                auto s = res[j];
                if(s != S && s->_neabre.size() < t)
                {
                    S->_neabre.insert(s);
                    s->_neabre.insert(S);
                    q.push_back(s);
                }
            }
            ++t;
        }
        is_first = false;
    }

    {
        size_t link_count = 0;
        for (auto& s : StarSystem::Get()._star_list)
        {
            for (auto& nb : s->_neabre)
            {
                if (!nb)
                    continue;
                if (s.get() < nb.get()) // берём ровно один раз на пару
                {
                    auto link = MakeRef<LinkActor>(s, nb);
                    main_world->AddObject(link);
                    ++link_count;
                }
            }
        }
        RGB->SetDirty();
    }

    for (int i = 0; i < 1000; ++i)
    {
        StarSystem::Get().SpawnShipAtRandomStar();
    }

    g_system->AddWorld("main_world", main_world);
    g_system->ActivateWorld("main_world");

    r_system->Start();
    i_system->Start();
    Ref<IKeyboard> kb = i_system->GetKeyboard();
    auto root = RGB->Root();
    auto mix = RGBMultipassNode::Make();
    mix->AddInput("world");
    mix->AddInput("ui");
    root->BindInput(0, mix, 0);
    auto worldRender = RGBWorldRender::Make(main_world, main_camera);
    auto uiRender = RGBWidgetRender::Make(Hud);
    
    mix->BindInput("world", worldRender, 0);
    mix->BindInput("ui", uiRender, 0);

    using ts_t = decltype(std::chrono::high_resolution_clock::now().time_since_epoch());
    ts_t ts{ts_t(0)};
    size_t i = 0;
    double T = 0;
    static bool run = true;

    i_system->Events().OnKeyPressed(KeyCode::Escape).Subscribe(srv_function<void()>([]() {
        run = false;
    }));

    static float K = dis * 1.1;
    static float V = 1.0;
    static float A = 0.2;
    float dt = 0;
    float fps = 0;
    i_system->Events().OnKeyDown(KeyCode::D).Subscribe(srv_function<void(float)>([](float dt) {
        V += dt * 0.1;
    }));

    i_system->Events().OnKeyDown(KeyCode::A).Subscribe(srv_function<void(float)>([](float dt) {
        V -= dt * 0.1;
    }));

    i_system->Events().OnKeyDown(KeyCode::W).Subscribe(srv_function<void(float)>([](float dt) {
        K -= dt;
    }));

    i_system->Events().OnKeyDown(KeyCode::S).Subscribe(srv_function<void(float)>([](float dt) {
        K += dt;
    }));

    i_system->Events().OnKeyDown(KeyCode::Q).Subscribe(srv_function<void(float)>([](float dt) {
        A -= dt * 0.1;
    }));

    i_system->Events().OnKeyDown(KeyCode::E).Subscribe(srv_function<void(float)>([](float dt) {
        A += dt * 0.1;
    }));

    i_system->Events().OnKeyPressed(KeyCode::Space).Subscribe(srv_function<void()>([]() {
        StarSystem::Get().SpawnShipAtRandomStar();
    }));

    i_system->Events().OnMouseMoved.Subscribe(srv_function<void(int, int)>([](int x, int y) {
        Hud->OnMouseMove(x, y);
    }));

    i_system->Events().OnMouseButtonPressed.Subscribe(srv_function<void(MouseButton)>([](MouseButton btn) {
        Hud->OnMouseDown(-1, -1, btn);
    }));

    i_system->Events().OnMouseButtonReleased.Subscribe(srv_function<void(MouseButton)>([](MouseButton btn) {
        Hud->OnClick(-1, -1, btn);
        Hud->OnMouseUp(-1, -1, btn);
    }));

    btn->OnPressed = [](){
        V = 0;
    };
    while (run)
    {
        i_system->Process();
        ts_t nts = std::chrono::high_resolution_clock::now().time_since_epoch();

        if (ts.count() != 0)
        {
            dt = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>>(nts - ts).count();
        }
        T += dt * V;
        V -= (V > 0 ? 1 : -1) * std::min(dt * 0.001f + std::abs(V) * 0.002f, std::abs(V));

        main_camera->SetPosition({K * cosf(T / wm::PI * 0.5) * cosf(A), K * sinf(T / wm::PI * 0.5) * cosf(A), K * sinf(A)});
        main_camera->SetTarget({0.0, 0.0, 0.0});
        ts = nts;
        if (dt > 0)
        {
            if (fps == 0)
            {
                fps = 1.0 / dt;
            }
            else
            {
                fps = (1.0 / dt + fps) * 0.5;
            }
        }
        printf("%f %f %f %f \r", fps, K, V, A);
        {
            auto r_ret = r_system->Process();
            if (r_ret != 0)
                break;
        }
        if (i % 100 == 0)
        {
            WL_TYMETRACE_PUSH_TO_WLOG;
        }
    }
}