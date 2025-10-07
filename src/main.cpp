
#include <thread>
#include <atomic>

#include <functional>
#include <tuple>
#include <stdexcept>
#include <mutex>
#include <vector>
#include <iostream>
#include <string>

#include "core/core.h"
// #include "render/window_bridges/winapi_opengl.h"
// #include "render/RenderSystem.h"

// auto* core = ISystem::Get<CoreSystem>();
// auto* render = ISystem::Get<RenderSystem>();

class test_t
{
    int w, h;
    bool updated = false;

    template<typename T>
    class test_t_property : public Property<T> {
        test_t* _owner;
    public:
        explicit test_t_property(test_t* owner, std::function<T()> getter, std::function<void(const T&)> setter) : _owner(owner), Property(getter, setter) {}

        test_t_property<T>& operator=(const T& value) override { _owner->NeedUpdate(); Property<T>::operator=(value); return *this; }
    };

public:
    ReadOnlyProperty<bool> Updated { 
        [this]() { return updated; }
    };
    void NeedUpdate() { updated = true; }
    void Update() { updated = false; }

    ReadOnlyProperty<int> S {
        [this]() -> int { return w * h; }
    };
    test_t_property<int> W {
        this,
        [this]() -> int { return w; },
        [this](const int & _) { w = _; }
    };
    test_t_property<int> H {
        this,
        [this]() -> int { return h; },
        [this](const int & _) { h = _; }
    };
};



int main()
{
    WeakProperty<int> t1;
    test_t test;
    t1 = test.H;
    t1 = &test.H;
    t1 = WeakProperty<int>(test.H);
    t1 = WeakProperty<int>(&test.H);

    std::cout << (!test.Updated ? "updated": "need update") << "\n"; test.Update();
    test.W = 10;
    std::cout << (!test.Updated ? "updated": "need update") << "\n"; test.Update();
    t1 = 20;
    std::cout << (!test.Updated ? "updated": "need update") << "\n"; test.Update();
    // test.S = 5;
    int w = test.W;
    int h = t1;
    std::cout << (!test.Updated ? "updated": "need update") << "\n"; test.Update();
    int s = test.S;
    std::cout << (!test.Updated ? "updated": "need update") << "\n"; test.Update();

    std::cout << h << ", " << w << ", " << s;
}










