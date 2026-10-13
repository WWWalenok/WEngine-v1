#include "GroupBox.h"

#include <algorithm>
#include <utility>

void GroupBox::Add(
    Ref<Widget> w,
    SizeMode mode,
    double size,
    double weight)
{
    items.push_back({std::move(w), size, mode, weight});
}

void GroupBox::AddFixed(Ref<Widget> w, double px)
{
    Add(std::move(w), SizeMode::Absolute, px);
}

void GroupBox::AddRelative(Ref<Widget> w, double frac)
{
    Add(std::move(w), SizeMode::Relative, frac);
}

void GroupBox::AddWeighted(Ref<Widget> w, double wt)
{
    Add(std::move(w), SizeMode::Auto, 0.0, wt);
}

void GroupBox::Remove(Ref<Widget> w)
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

void GroupBox::Clear()
{
    items.clear();
    hovered = nullptr;
}

std::vector<Ref<Widget>> GroupBox::Childs()
{
    std::vector<Ref<Widget>> out;
    out.reserve(items.size());

    for (auto& it : items)
        out.push_back(it.widget);

    return out;
}

std::list<Widget::ToRender> GroupBox::Render(
    const double& W,
    const double& H,
    const std::unordered_map<std::string, Data>& meta)
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

bool GroupBox::OnClick(const double& X, const double& Y, const MouseButton& btn)
{
    return DispatchPoint(X, Y, EEvent::Click, btn);
}

bool GroupBox::OnMouseDown(const double& X, const double& Y, const MouseButton& btn)
{
    return DispatchPoint(X, Y, EEvent::Down, btn);
}

bool GroupBox::OnMouseUp(const double& X, const double& Y, const MouseButton& btn)
{
    return DispatchPoint(X, Y, EEvent::Up, btn);
}

void GroupBox::OnMouseMove(const double& NX, const double& NY)
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

void GroupBox::OnMouseWhell(const double& V)
{
    if (hovered)
        hovered->OnMouseWhell(V);
}

std::vector<GroupBox::Rect> GroupBox::ComputeLayout(double W, double H) const
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
        {
            if (items[i].mode == SizeMode::Auto)
                sizes[i] = remaining * (items[i].weight / weight_sum);
        }
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

bool GroupBox::DispatchPoint(double X, double Y, EEvent ev, const MouseButton& btn)
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