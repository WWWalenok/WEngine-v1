#include "CanvasWidget.h"

void CanvasWidget::Add(Ref<Widget> widget, const Slot& slot)
{
    slots[widget] = slot;
    bDirty = true;
}

void CanvasWidget::Remove(Ref<Widget> widget)
{
    slots.erase(widget);
    hoveredWidgets.erase(widget);

    if (capturedWidget == widget)
        capturedWidget = nullptr;

    bDirty = true;
}

void CanvasWidget::SetPos(Ref<Widget> widget, const MVector2f& pos, const Slot::Type& type)
{
    auto it = slots.find(widget);
    if (it == slots.end())
        return;

    it->second.pos.data = pos;
    it->second.pos.type = type;
    bDirty = true;
}

void CanvasWidget::SetSize(Ref<Widget> widget, const MVector2f& size, const Slot::Type& type)
{
    auto it = slots.find(widget);
    if (it == slots.end())
        return;

    it->second.size.data = size;
    it->second.size.type = type;
    bDirty = true;
}

void CanvasWidget::SetAnchor(
    Ref<Widget> widget,
    const MVector2f& pos_min,
    const Slot::Type& type_min,
    const MVector2f& pos_max,
    const Slot::Type& type_max)
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

void CanvasWidget::SetZOrder(Ref<Widget> widget, int z)
{
    auto it = slots.find(widget);
    if (it == slots.end())
        return;

    it->second.z_order = z;
    bDirty = true;
}

std::vector<Ref<Widget>> CanvasWidget::Childs()
{
    std::vector<Ref<Widget>> out;
    out.reserve(slots.size());

    for (auto& kv : slots)
        out.push_back(kv.first);

    return out;
}

std::list<Widget::ToRender> CanvasWidget::Render(
    const double& W,
    const double& H,
    const std::unordered_map<std::string, Data>& meta)
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
    }

    return out;
}

bool CanvasWidget::OnClick(const double& X, const double& Y, const MouseButton& btn)
{
    bool handled = false;
    DispatchPoint(X, Y, InputEvent::Click, btn, handled);
    return handled;
}

bool CanvasWidget::OnMouseDown(const double& X, const double& Y, const MouseButton& btn)
{
    bool handled = false;
    Ref<Widget> w = DispatchPoint(X, Y, InputEvent::MouseDown, btn, handled);

    if (handled)
        capturedWidget = w;

    return handled;
}

bool CanvasWidget::OnMouseUp(const double& X, const double& Y, const MouseButton& btn)
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

void CanvasWidget::OnMouseMove(const double& NX, const double& NY)
{
    LastMouseX = NX;
    LastMouseY = NY;
    bHasMousePos = true;

    if (LastW <= 0.0 || LastH <= 0.0)
        return;

    RebuildChildTree(LastW, LastH);

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

void CanvasWidget::OnMouseWhell(const double& V)
{
    if (!bHasMousePos)
        return;

    if (LastW <= 0.0 || LastH <= 0.0)
        return;

    RebuildChildTree(LastW, LastH);

    auto res = child.QueryPoint({LastMouseX, LastMouseY});
    SortByZDescending(res);

    for (auto& w : res)
        w->OnMouseWhell(V);
}

Ref<Widget> CanvasWidget::DispatchPoint(
    double X,
    double Y,
    InputEvent ev,
    const MouseButton& btn,
    bool& handled)
{
    if (X < 0 || Y < 0)
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

void CanvasWidget::SortByZDescending(std::vector<Ref<Widget>>& widgets) const
{
    std::sort(widgets.begin(), widgets.end(), [this](Ref<Widget> a, Ref<Widget> b) {
        auto ia = slots.find(a);
        auto ib = slots.find(b);

        int za = (ia != slots.end()) ? ia->second.z_order : 0;
        int zb = (ib != slots.end()) ? ib->second.z_order : 0;

        return za > zb;
    });
}

double CanvasWidget::ResolveValue(double v, Slot::Type t, double axis)
{
    double r = (t == Slot::Reletive) ? v * axis : v;

    if (v < 0.0)
        r += axis;

    return r;
}

double CanvasWidget::ResolveSize(double v, Slot::Type t, double axis)
{
    double r = (t == Slot::Reletive) ? v * axis : v;
    return r;
}

void CanvasWidget::ResolveSlot(
    const Slot& slot,
    double W,
    double H,
    double& x,
    double& y,
    double& w,
    double& h)
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

    if (w < 0)
    {
        x += w;
        w = -w;
    }

    if (h < 0)
    {
        y += h;
        h = -h;
    }
}

void CanvasWidget::RebuildChildTree(double W, double H)
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