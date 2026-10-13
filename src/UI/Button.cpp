#include "Button.h"
#include "../Text/FontTable.h"

#include <algorithm>
#include <cmath>

Button::Button(std::function<void()> cb)
    : OnPressed(std::move(cb))
{
}

std::list<Widget::ToRender> Button::Render(
    const double& W,
    const double& H,
    const std::unordered_map<std::string, Data>&)
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

bool Button::OnMouseDown(const double&, const double&, const MouseButton&)
{
    bPressed = true;
    return true;
}

bool Button::OnMouseUp(const double&, const double&, const MouseButton&)
{
    bool fire = bPressed && bHovered;
    bPressed = false;

    if (fire && OnPressed)
        OnPressed();

    return true;
}

bool Button::OnClick(const double&, const double&, const MouseButton&)
{
    return true;
}

void Button::OnMouseMove(const double&, const double&)
{
    bHovered = true;
}

void Button::OnMouseLeave()
{
    bHovered = false;
    bPressed = false;
}

void Button::EnsureCache(int W, int H)
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

    _cache[0] = WidgetVecp::MakeRRect(
        W, H, NormalColor, BorderColor, CornerRadius, BorderThickness, 0b1111, Sub);

    _cache[1] = WidgetVecp::MakeRRect(
        W, H, HoveredColor, BorderColor, CornerRadius, BorderThickness, 0b1111, Sub);

    _cache[2] = WidgetVecp::MakeRRect(
        W, H, PressedColor, BorderColor, CornerRadius, BorderThickness, 0b1111, Sub);
}