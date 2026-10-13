#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../graphic/Image.h"
#include "../Text/FontTable.h"
#include "../Text/DisplayText.h"
#include "../render/IWindowHelper.h"
#include "../input/IInputHelper.h"

struct Widget : public CoreObject
{
    Widget() = default;

    struct ToRender
    {
        Ref<Image> data;
        Ref<DisplayText> text;
        double X;
        double Y;
        double W;
        double H;
        int Z;
    };

    virtual std::list<ToRender> Render(const double& W, const double& H, const std::unordered_map<std::string, Data>& meta = {}) = 0;
    virtual void OnUpdate()
    {
    }
    virtual void OnResize()
    {
    }

    // return true if captured. X, Y — координаты в локальной системе виджета.
    virtual bool OnClick(const double& X, const double& Y, const MouseButton& btn)
    {
        return false;
    }
    virtual bool OnMouseDown(const double& X, const double& Y, const MouseButton& btn)
    {
        return false;
    }
    virtual bool OnMouseUp(const double& X, const double& Y, const MouseButton& btn)
    {
        return false;
    }
    virtual void OnMouseMove(const double& NX, const double& NY)
    {
    }
    virtual void OnMouseLeave()
    {
    }
    virtual void OnMouseWhell(const double& V)
    {
    }

    virtual bool OnKeyPress(const KeyCode& Key)
    {
        return false;
    }
    virtual bool OnKeyDown(const KeyCode& Key)
    {
        return false;
    }
    virtual bool OnKeyUp(const KeyCode& Key)
    {
        return false;
    }

    virtual std::vector<Ref<Widget>> Childs()
    {
        return {};
    }
};

// =====================================================================
// CanvasWidget — «холст» со слотами
// =====================================================================
struct CanvasWidget : public Widget
{
    struct Slot
    {
        enum Type
        {
            Absolute,
            Reletive
        };

        struct Point
        {
            MVector2f data = {0.0f, 0.0f};
            Type type = Absolute;

            operator MVector2f&()
            {
                return data;
            }
            operator const MVector2f&() const
            {
                return data;
            }
        };

        Slot(int z_order = 0, Point pos = {{0.0f, 0.0f}, Absolute}, Point size = {{0.0f, 0.0f}, Absolute}, Point anchor_min = {{0.0f, 0.0f}, Reletive}, Point anchor_max = {{1.0f, 1.0f}, Reletive}) : z_order(z_order), pos(pos), size(size), anchor_min(anchor_min), anchor_max(anchor_max)
        {
        }

        int z_order = 0;
        Point pos{{0.0f, 0.0f}, Absolute};
        Point size{{0.0f, 0.0f}, Absolute};
        Point anchor_min{{0.0f, 0.0f}, Reletive};
        Point anchor_max{{1.0f, 1.0f}, Reletive};
    };

    std::unordered_map<Ref<Widget>, Slot> slots;
    TypedRTree<Ref<Widget>, double, 2> child;

    // ------------------------------------------------------------------
    // Слоты
    // ------------------------------------------------------------------
    void Add(Ref<Widget> widget, const Slot& slot)
    {
        slots[widget] = slot;
        bDirty = true;
    }

    void Remove(Ref<Widget> widget)
    {
        slots.erase(widget);
        hoveredWidgets.erase(widget);
        if (capturedWidget == widget)
            capturedWidget = nullptr;
        bDirty = true;
    }

    void SetPos(Ref<Widget> widget, const MVector2f& pos, const Slot::Type& type)
    {
        auto it = slots.find(widget);
        if (it == slots.end())
            return;
        it->second.pos.data = pos;
        it->second.pos.type = type;
        bDirty = true;
    }

    void SetSize(Ref<Widget> widget, const MVector2f& size, const Slot::Type& type)
    {
        auto it = slots.find(widget);
        if (it == slots.end())
            return;
        it->second.size.data = size;
        it->second.size.type = type;
        bDirty = true;
    }

    void SetAnchor(Ref<Widget> widget, const MVector2f& pos_min, const Slot::Type& type_min, const MVector2f& pos_max, const Slot::Type& type_max)
    {
        auto it = slots.find(widget);
        if (it == slots.end())
            return;
        it->second.anchor_min.data = pos_min;
        it->second.anchor_min.type = type_min;
        it->second.anchor_max.data = pos_max;
        it->second.anchor_max.type = type_max;
        bDirty = true;
    }

    void SetZOrder(Ref<Widget> widget, int z)
    {
        auto it = slots.find(widget);
        if (it == slots.end())
            return;
        it->second.z_order = z;
        bDirty = true;
    }

    virtual std::vector<Ref<Widget>> Childs() override
    {
        std::vector<Ref<Widget>> out;
        out.reserve(slots.size());
        for (auto& kv : slots)
            out.push_back(kv.first);
        return out;
    }

    // ------------------------------------------------------------------
    // Рендер
    // ------------------------------------------------------------------
    virtual std::list<ToRender> Render(const double& W, const double& H, const std::unordered_map<std::string, Data>& meta) override
    {
        LastW = W;
        LastH = H;
        RebuildChildTree(W, H);

        std::list<ToRender> out;

        std::vector<std::pair<Ref<Widget>, const Slot*>> sorted;
        sorted.reserve(slots.size());
        for (auto& kv : slots)
            sorted.push_back({kv.first, &kv.second});
        std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            return a.second->z_order < b.second->z_order;
        });

        for (auto& kv : sorted)
        {
            Ref<Widget> widget = kv.first;

            const Slot& slot = *kv.second;

            double cx, cy, cw, ch;
            ResolveSlot(slot, W, H, cx, cy, cw, ch);

            auto rets = widget->Render(cw, ch, meta);
            for (auto& ret : rets)
            {
                ret.X += cx;
                ret.Y += cy;
                ret.Z += slot.z_order;
                out.push_back(ret);
            }
            
            if(widget.__data()->s_counter.load() == 1)
            {
                WLI << "WTF";
            }
        }

        return out;
    }

    // ------------------------------------------------------------------
    // События
    // ------------------------------------------------------------------
    virtual bool OnClick(const double& X, const double& Y, const MouseButton& btn) override
    {
        bool handled = false;
        DispatchPoint(X, Y, InputEvent::Click, btn, handled);
        return handled;
    }

    virtual bool OnMouseDown(const double& X, const double& Y, const MouseButton& btn) override
    {
        bool handled = false;
        Ref<Widget> w = DispatchPoint(X, Y, InputEvent::MouseDown, btn, handled);
        if (handled)
            capturedWidget = w;
        return handled;
    }

    virtual bool OnMouseUp(const double& X, const double& Y, const MouseButton& btn) override
    {
        if (capturedWidget)
        {
            auto it = slots.find(capturedWidget);
            double cx = 0, cy = 0, cw = 0, ch = 0;
            if (it != slots.end())
                ResolveSlot(it->second, LastW, LastH, cx, cy, cw, ch);

            auto ret = capturedWidget->OnMouseUp(X - cx, Y - cy, btn);
            capturedWidget = nullptr;
            return ret;
        }

        bool handled = false;
        DispatchPoint(X, Y, InputEvent::MouseUp, btn, handled);
        return handled;
    }

    virtual void OnMouseMove(const double& NX, const double& NY) override
    {
        LastMouseX = NX;
        LastMouseY = NY;
        bHasMousePos = true;

        if (LastW <= 0.0 || LastH <= 0.0)
            return;
        RebuildChildTree(LastW, LastH);

        // Если кто-то захватил мышь — направляем всё ему.
        if (capturedWidget)
        {
            auto it = slots.find(capturedWidget);
            if (it != slots.end())
            {
                double cx, cy, cw, ch;
                ResolveSlot(it->second, LastW, LastH, cx, cy, cw, ch);
                capturedWidget->OnMouseMove(NX - cx, NY - cy);
            }
            return;
        }

        auto res = child.QueryPoint({NX, NY});
        std::unordered_set<Ref<Widget>> newHovered(res.begin(), res.end());

        // Ушли с этих виджетов — посылаем OnMouseLeave.
        for (auto w : hoveredWidgets)
            if (!newHovered.count(w))
                w->OnMouseLeave();

        SortByZDescending(res);
        for (auto& w : res)
        {
            auto it = slots.find(w);
            if (it == slots.end())
                continue;
            double cx, cy, cw, ch;
            ResolveSlot(it->second, LastW, LastH, cx, cy, cw, ch);
            w->OnMouseMove(NX - cx, NY - cy);
        }

        hoveredWidgets = std::move(newHovered);
    }

    virtual void OnMouseWhell(const double& V) override
    {
        if (!bHasMousePos)
            return;
        if (LastW <= 0.0 || LastH <= 0.0)
            return;
        RebuildChildTree(LastW, LastH);

        auto res = child.QueryPoint({LastMouseX, LastMouseY});
        SortByZDescending(res);
        for (auto& w : res)
        {
            w->OnMouseWhell(V);
            // без обработки «captured» — колесо не захватывается
        }
    }

private:
    double LastW = -1.0;
    double LastH = -1.0;
    bool bDirty = true;

    double LastMouseX = 0.0;
    double LastMouseY = 0.0;
    bool bHasMousePos = false;

    std::unordered_set<Ref<Widget>> hoveredWidgets;
    Ref<Widget> capturedWidget = nullptr;

    enum class InputEvent
    {
        Click,
        MouseDown,
        MouseUp
    };

    // Возвращает виджет, который обработал событие, и выставляет handled.
    Ref<Widget> DispatchPoint(double X, double Y, InputEvent ev, const MouseButton& btn, bool& handled)
    {
        if(X < 0 || Y < 0)
        {
            X = LastMouseX;
            Y = LastMouseY;
        }
        handled = false;
        if (LastW <= 0.0 || LastH <= 0.0)
            return nullptr;
        RebuildChildTree(LastW, LastH);

        auto res = child.QueryPoint({X, Y});
        SortByZDescending(res);

        for (auto w : res)
        {
            auto it = slots.find(w);
            if (it == slots.end())
                continue;

            double cx, cy, cw, ch;
            ResolveSlot(it->second, LastW, LastH, cx, cy, cw, ch);

            double lx = X - cx;
            double ly = Y - cy;

            bool h = false;
            switch (ev)
            {
                case InputEvent::Click:
                    h = w->OnClick(lx, ly, btn);
                    break;
                case InputEvent::MouseDown:
                    h = w->OnMouseDown(lx, ly, btn);
                    break;
                case InputEvent::MouseUp:
                    h = w->OnMouseUp(lx, ly, btn);
                    break;
            }
            if (h)
            {
                handled = true;
                return w;
            }
        }
        return nullptr;
    }

    void SortByZDescending(std::vector<Ref<Widget>>& widgets) const
    {
        std::sort(widgets.begin(), widgets.end(), [this](Ref<Widget> a, Ref<Widget> b) {
            auto ia = slots.find(a);
            auto ib = slots.find(b);
            int za = (ia != slots.end()) ? ia->second.z_order : 0;
            int zb = (ib != slots.end()) ? ib->second.z_order : 0;
            return za > zb;
        });
    }

    static double ResolveValue(double v, Slot::Type t, double axis)
    {
        double r = (t == Slot::Reletive) ? v * axis : v;
        if (v < 0.0)
            r += axis;
        return r;
    }

    static double ResolveSize(double v, Slot::Type t, double axis)
    {
        double r = (t == Slot::Reletive) ? v * axis : v;
        return r;
    }

    static void ResolveSlot(const Slot& slot, double W, double H, double& x, double& y, double& w, double& h)
    {
        double ax = ResolveValue(slot.anchor_min.data[0], slot.anchor_min.type, W);
        double ay = ResolveValue(slot.anchor_min.data[1], slot.anchor_min.type, H);
        double bx = ResolveValue(slot.anchor_max.data[0], slot.anchor_max.type, W);
        double by = ResolveValue(slot.anchor_max.data[1], slot.anchor_max.type, H);

        double aw = bx - ax;
        double ah = by - ay;

        double px = ResolveValue(slot.pos.data[0], slot.pos.type, aw);
        double py = ResolveValue(slot.pos.data[1], slot.pos.type, ah);

        x = ax + px;
        y = ay + py;
        w = ResolveSize(slot.size.data[0], slot.size.type, aw);
        h = ResolveSize(slot.size.data[1], slot.size.type, ah);

        if(w < 0)
        {
            x += w;
            w = -w;
        }
        if(h < 0)
        {
            y += h;
            h = -h;
        }
    }

    // Полная пересборка RTree при изменении геометрии или размеров.
    void RebuildChildTree(double W, double H)
    {
        if (!bDirty && W == LastW && H == LastH)
            return;

        child.Clear();
        for (auto& kv : slots)
        {
            double x, y, w, h;
            ResolveSlot(kv.second, W, H, x, y, w, h);
            child.Insert(kv.first, RTBoundingBox<double, 2>({x, y}, {x + w, y + h}));
        }

        LastW = W;
        LastH = H;
        bDirty = false;
    }
};

namespace WidgetDefaults
{
    inline Ref<Image> SolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        // Ключ упаковываем в 32 бита, чтобы не плодить маппинги.
        uint32_t key = (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | uint32_t(a);
        static std::unordered_map<uint32_t, Ref<Image>> cache;
        auto it = cache.find(key);
        if (it != cache.end())
            return it->second;

        uint8_t px[4] = {r, g, b, a};
        auto img = MakeRef<Image>();
        img->LoadFromMemory(px, sizeof(px), 1, 1, PixelFormat::RGBA_8);
        cache[key] = img;
        return img;
    }
}

namespace WidgetVecp
{
    // На little-endian RGBA_8 uint32 = 0xAABBGGRR.
    inline unsigned int PackRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        return (unsigned int(a) << 24) | (unsigned int(b) << 16)
             | (unsigned int(g) << 8)  | unsigned int(r);
    }
    inline unsigned int PackRGBA(const MVector3f& c, float a = 1.0f)
    {
        auto cl = [](float v) {
            int i = (int)(v * 255.0f + 0.5f);
            if (i < 0) i = 0; if (i > 255) i = 255;
            return (uint8_t)i;
        };
        return PackRGBA(cl(c[0]), cl(c[1]), cl(c[2]), cl(a));
    }

    // corners: 0b1000 lt | 0b0100 rt | 0b0010 rb | 0b0001 lb
    // Заливка + рамка одним куском.
    inline Ref<Image> MakeRRect(int W, int H,
                                unsigned int fillColor,
                                unsigned int borderColor,
                                int cornerRadius,
                                int borderThickness,
                                unsigned char corners = 0b1111,
                                int sub = 3)
    {
        if (W <= 0 || H <= 0)
            return nullptr;

        std::vector<uint8_t> buf(size_t(W) * size_t(H) * 4u, 0);
        unsigned int* px = reinterpret_cast<unsigned int*>(buf.data());

        if (borderThickness > 0 && borderColor != 0)
        {
            const int b  = borderThickness;
            const int iW = W - 2 * b;
            const int iH = H - 2 * b;
            const int ir = std::max(0, cornerRadius - b);
            vecp::PaintRRect(px, borderColor, 0, 0, W, H, cornerRadius, cornerRadius, 0, W, corners, sub);
            vecp::FillRRect(px, fillColor, b, b, iW, iH, ir, W, corners, sub);
        }
        else
        {
            vecp::FillRRect(px, fillColor, 0, 0, W, H,
                            cornerRadius, W, corners, sub);
        }

        auto img = MakeRef<Image>();
        img->LoadFromMemory(buf.data(), buf.size(),
                            (uint32_t)W, (uint32_t)H, PixelFormat::RGBA_8);
        return img;
    }

    // Небольшой миксер для хэширования параметров в ленивых кэшах.
    inline size_t Mix(size_t h, size_t v)
    {
        return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }
}

// =====================================================================
// Button
// =====================================================================
struct Button : public Widget
{
    // ---- VECP-параметры ----
    int      CornerRadius    = 5;
    int      BorderThickness = 2;
    int      Sub             = 2;

    unsigned int NormalColor  = WidgetVecp::PackRGBA(150, 150, 150);
    unsigned int HoveredColor = WidgetVecp::PackRGBA(190, 190, 190);
    unsigned int PressedColor = WidgetVecp::PackRGBA(110, 110, 110);
    unsigned int BorderColor  = WidgetVecp::PackRGBA( 40,  40,  40);

    std::function<void()> OnPressed;

    bool bHovered = false;
    bool bPressed = false;

    Button() = default;
    explicit Button(std::function<void()> cb) : OnPressed(std::move(cb)) {}

    virtual std::list<ToRender> Render(const double& W, const double& H,
                                       const std::unordered_map<std::string, Data>&) override
    {
        const int iW = std::max(1, (int)std::ceil(W));
        const int iH = std::max(1, (int)std::ceil(H));
        EnsureCache(iW, iH);

        Ref<Image> img = bPressed ? _cache[2]
                       : bHovered ? _cache[1]
                                  : _cache[0];

        std::list<ToRender> out;
        // if (img)
        //     out.push_back({ img, nullptr, 0.0, 0.0, W, H, 0 });
        Ref<DisplayText> text = MakeRef<DisplayText>();
        text->SetText("qwertyasdfgzxcvbQWERYT");
        text->SetDefaultFont(FontTable::Get().GetFont("", 16));

        out.push_back({nullptr, text, 0.0, 0.0, W, H, 1});
        return out;
    }

    virtual bool OnMouseDown(const double&, const double&, const MouseButton&) override
    { bPressed = true; return true; }

    virtual bool OnMouseUp(const double&, const double&, const MouseButton&) override
    {
        bool fire = bPressed && bHovered;
        bPressed = false;
        if (fire && OnPressed) OnPressed();
        return true;
    }

    virtual bool OnClick(const double&, const double&, const MouseButton&) override
    { return true; }

    virtual void OnMouseMove(const double&, const double&) override
    { bHovered = true; }

    virtual void OnMouseLeave() override
    { bHovered = false; bPressed = false; }

private:
    Ref<Image> _cache[3];
    size_t     _hash = 0;

    void EnsureCache(int W, int H)
    {
        size_t h = 0;
        using WidgetVecp::Mix;
        h = Mix(h, (size_t)W);
        h = Mix(h, (size_t)H);
        h = Mix(h, (size_t)CornerRadius);
        h = Mix(h, (size_t)BorderThickness);
        h = Mix(h, (size_t)Sub);
        h = Mix(h, (size_t)NormalColor);
        h = Mix(h, (size_t)HoveredColor);
        h = Mix(h, (size_t)PressedColor);
        h = Mix(h, (size_t)BorderColor);

        if (h == _hash && _cache[0] && _cache[1] && _cache[2])
            return;
        _hash = h;

        _cache[0] = WidgetVecp::MakeRRect(W, H, NormalColor,  BorderColor, CornerRadius, BorderThickness, 0b1111, Sub);
        _cache[1] = WidgetVecp::MakeRRect(W, H, HoveredColor, BorderColor, CornerRadius, BorderThickness, 0b1111, Sub);
        _cache[2] = WidgetVecp::MakeRRect(W, H, PressedColor, BorderColor, CornerRadius, BorderThickness, 0b1111, Sub);
    }
};

// =====================================================================
// FillBar
// =====================================================================
struct FillBar : public Widget
{
    // ---- VECP-параметры ----
    int      CornerRadius    = 6;
    int      BorderThickness = 1;
    int      Sub             = 3;

    unsigned int BgColor     = WidgetVecp::PackRGBA( 60,  60,  60);
    unsigned int FillColor   = WidgetVecp::PackRGBA( 80, 200, 100);
    unsigned int BorderColor = WidgetVecp::PackRGBA( 20,  20,  20);

    double Value     = 0.0;
    bool   bHorizontal = true;
    bool   bInvert     = false;

    FillBar() = default;
    FillBar(double v, bool horiz = true, bool invert = false)
        : Value(std::clamp(v, 0.0, 1.0))
        , bHorizontal(horiz)
        , bInvert(invert)
    {}

    void SetValue(double v) { Value = std::clamp(v, 0.0, 1.0); }

    virtual std::list<ToRender> Render(const double& W, const double& H,
                                       const std::unordered_map<std::string, Data>&) override
    {
        const int iW = std::max(1, (int)std::ceil(W));
        const int iH = std::max(1, (int)std::ceil(H));

        std::list<ToRender> out;

        EnsureBgCache(iW, iH);
        if (_bg)
            out.push_back({ _bg, nullptr, 0.0, 0.0, W, H, 0 });

        if (Value > 0.0)
        {
            double fw = bHorizontal ? W * Value : W;
            double fh = bHorizontal ? H : H * Value;
            double fx = (bHorizontal && bInvert) ? W - fw : 0.0;
            double fy = (!bHorizontal && bInvert) ? H - fh : 0.0;

            const int pW = std::max(1, (int)std::ceil(fw));
            const int pH = std::max(1, (int)std::ceil(fh));
            EnsureFillCache(pW, pH);

            if (_fill)
                out.push_back({ _fill, nullptr, fx, fy, fw, fh, 1 });
        }
        return out;
    }

private:
    Ref<Image> _bg;
    Ref<Image> _fill;
    size_t     _bgHash   = 0;
    size_t     _fillHash = 0;

    // lt=0b1000, rt=0b0100, rb=0b0010, lb=0b0001.
    // Скруглены только внешние углы заливки — те, что примыкают к краю бара.
    unsigned char CornersForState() const
    {
        if (bHorizontal)
            return bInvert ? (unsigned char)0b0110   // rt|rb
                           : (unsigned char)0b1001;  // lt|lb
        else
            return bInvert ? (unsigned char)0b0011   // lb|rb
                           : (unsigned char)0b1100;  // lt|rt
    }

    void EnsureBgCache(int W, int H)
    {
        using WidgetVecp::Mix;
        size_t h = 0;
        h = Mix(h, (size_t)W);
        h = Mix(h, (size_t)H);
        h = Mix(h, (size_t)CornerRadius);
        h = Mix(h, (size_t)BorderThickness);
        h = Mix(h, (size_t)Sub);
        h = Mix(h, (size_t)BgColor);
        h = Mix(h, (size_t)BorderColor);

        if (h == _bgHash && _bg) return;
        _bgHash = h;

        _bg = WidgetVecp::MakeRRect(W, H, BgColor, BorderColor,
                                    CornerRadius, BorderThickness,
                                    0b1111, Sub);
    }

    void EnsureFillCache(int W, int H)
    {
        using WidgetVecp::Mix;
        size_t h = 0;
        h = Mix(h, (size_t)W);
        h = Mix(h, (size_t)H);
        h = Mix(h, (size_t)CornerRadius);
        h = Mix(h, (size_t)Sub);
        h = Mix(h, (size_t)FillColor);

        if (h == _fillHash && _fill) return;
        _fillHash = h;

        _fill = WidgetVecp::MakeRRect(W, H, FillColor,
                                      0, /* borderColor */
                                      CornerRadius, 0, /* borderThickness */
                                      0b1111, Sub);
    }
};

// =====================================================================
// GroupBox — вертикальная / горизонтальная раскладка
// =====================================================================
struct GroupBox : public Widget
{
    enum class Orientation
    {
        Horizontal,
        Vertical
    };
    enum class SizeMode
    {
        Absolute,
        Relative,
        Auto
    };

    struct Item
    {
        Ref<Widget> widget;
        double size = 0.0; // px (Absolute) / доля (Relative) / игнорируется (Auto)
        SizeMode mode = SizeMode::Auto;
        double weight = 1.0; // только для Auto
    };

    Orientation orientation = Orientation::Vertical;
    double spacing = 0.0;
    double padding = 0.0;
    std::vector<Item> items;

    // ---- Управление ----
    void Add(Ref<Widget> w, SizeMode mode = SizeMode::Auto, double size = 0.0, double weight = 1.0)
    {
        items.push_back({std::move(w), size, mode, weight});
    }
    void AddFixed(Ref<Widget> w, double px)
    {
        Add(std::move(w), SizeMode::Absolute, px);
    }
    void AddRelative(Ref<Widget> w, double frac)
    {
        Add(std::move(w), SizeMode::Relative, frac);
    }
    void AddWeighted(Ref<Widget> w, double wt = 1)
    {
        Add(std::move(w), SizeMode::Auto, 0.0, wt);
    }

    void Remove(Ref<Widget> w)
    {
        auto it = std::remove_if(items.begin(), items.end(), [&](const Item& i) {
            return i.widget == w;
        });
        if (it != items.end())
        {
            if (hovered == w)
                hovered = nullptr;
            items.erase(it, items.end());
        }
    }

    void Clear()
    {
        items.clear();
        hovered = nullptr;
    }

    virtual std::vector<Ref<Widget>> Childs() override
    {
        std::vector<Ref<Widget>> out;
        out.reserve(items.size());
        for (auto& it : items)
            out.push_back(it.widget);
        return out;
    }

    // ---- Рендер ----
    virtual std::list<ToRender> Render(const double& W, const double& H, const std::unordered_map<std::string, Data>& meta) override
    {
        LastW = W;
        LastH = H;

        std::list<ToRender> out;
        auto rects = ComputeLayout(W, H);

        for (size_t i = 0; i < items.size(); ++i)
        {
            const Rect& r = rects[i];
            auto rets = items[i].widget->Render(r.w, r.h, meta);
            for (auto& ret : rets)
            {
                ret.X += r.x;
                ret.Y += r.y;
                out.push_back(ret);
            }
        }
        return out;
    }

    // ---- События ----
    virtual bool OnClick(const double& X, const double& Y, const MouseButton& btn) override
    {
        return DispatchPoint(X, Y, EEvent::Click, btn);
    }
    virtual bool OnMouseDown(const double& X, const double& Y, const MouseButton& btn) override
    {
        return DispatchPoint(X, Y, EEvent::Down, btn);
    }
    virtual bool OnMouseUp(const double& X, const double& Y, const MouseButton& btn) override
    {
        return DispatchPoint(X, Y, EEvent::Up, btn);
    }

    virtual void OnMouseMove(const double& NX, const double& NY) override
    {
        if (LastW <= 0.0 || LastH <= 0.0)
            return;
        auto rects = ComputeLayout(LastW, LastH);

        for (size_t i = 0; i < items.size(); ++i)
        {
            const Rect& r = rects[i];
            if (NX >= r.x && NX <= r.x + r.w && NY >= r.y && NY <= r.y + r.h)
            {
                if (hovered != items[i].widget)
                {
                    if (hovered)
                        hovered->OnMouseLeave();
                    hovered = items[i].widget;
                }
                items[i].widget->OnMouseMove(NX - r.x, NY - r.y);
                return;
            }
        }
        if (hovered)
        {
            hovered->OnMouseLeave();
            hovered = nullptr;
        }
    }

    virtual void OnMouseWhell(const double& V) override
    {
        if (hovered)
            hovered->OnMouseWhell(V);
    }

private:
    double LastW = 0.0;
    double LastH = 0.0;
    Ref<Widget> hovered = nullptr;

    struct Rect
    {
        double x, y, w, h;
    };

    std::vector<Rect> ComputeLayout(double W, double H) const
    {
        std::vector<Rect> rects(items.size());
        if (items.empty())
            return rects;

        const bool isV = (orientation == Orientation::Vertical);

        const double innerW = std::max(0.0, W - 2.0 * padding);
        const double innerH = std::max(0.0, H - 2.0 * padding);
        const double axis_len = isV ? innerH : innerW;

        const double spacing_total = spacing * double(items.size() - 1);
        const double avail = std::max(0.0, axis_len - spacing_total);

        std::vector<double> sizes(items.size(), 0.0);
        double weight_sum = 0.0;
        double consumed = 0.0;

        for (size_t i = 0; i < items.size(); ++i)
        {
            const auto& it = items[i];
            if (it.mode == SizeMode::Absolute)
            {
                sizes[i] = it.size;
                consumed += sizes[i];
            }
            else if (it.mode == SizeMode::Relative)
            {
                sizes[i] = it.size * axis_len;
                consumed += sizes[i];
            }
            else
            {
                weight_sum += it.weight;
            }
        }

        const double remaining = std::max(0.0, avail - consumed);
        if (weight_sum > 0.0)
        {
            for (size_t i = 0; i < items.size(); ++i)
                if (items[i].mode == SizeMode::Auto)
                    sizes[i] = remaining * (items[i].weight / weight_sum);
        }

        double pos = padding;
        for (size_t i = 0; i < items.size(); ++i)
        {
            Rect r;
            if (isV)
            {
                r.x = padding;
                r.y = pos;
                r.w = innerW;
                r.h = sizes[i];
            }
            else
            {
                r.x = pos;
                r.y = padding;
                r.w = sizes[i];
                r.h = innerH;
            }
            rects[i] = r;
            pos += sizes[i] + spacing;
        }
        return rects;
    }

    enum class EEvent
    {
        Click,
        Down,
        Up
    };

    bool DispatchPoint(double X, double Y, EEvent ev, const MouseButton& btn)
    {
        if (LastW <= 0.0 || LastH <= 0.0)
            return false;
        auto rects = ComputeLayout(LastW, LastH);

        for (size_t i = 0; i < items.size(); ++i)
        {
            const Rect& r = rects[i];
            if (X >= r.x && X <= r.x + r.w && Y >= r.y && Y <= r.y + r.h)
            {
                const double lx = X - r.x;
                const double ly = Y - r.y;
                switch (ev)
                {
                    case EEvent::Click:
                        return items[i].widget->OnClick(lx, ly, btn);
                    case EEvent::Down:
                        return items[i].widget->OnMouseDown(lx, ly, btn);
                    case EEvent::Up:
                        return items[i].widget->OnMouseUp(lx, ly, btn);
                }
            }
        }
        return false;
    }
};
