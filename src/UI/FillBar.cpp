#include "FillBar.h"

#include <algorithm>
#include <cmath>

FillBar::FillBar(double v, bool horiz, bool invert)
    : Value(std::clamp(v, 0.0, 1.0))
    , bHorizontal(horiz)
    , bInvert(invert)
{
}

void FillBar::SetValue(double v)
{
    Value = std::clamp(v, 0.0, 1.0);
}

std::list<Widget::ToRender> FillBar::Render(
    const double& W,
    const double& H,
    const std::unordered_map<std::string, Data>&)
{
    const int iW = std::max(1, (int)std::ceil(W));
    const int iH = std::max(1, (int)std::ceil(H));

    std::list<ToRender> out;

    EnsureBgCache(iW, iH);

    if (_bg)
        out.push_back({_bg, nullptr, 0.0, 0.0, W, H, 0});

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
            out.push_back({_fill, nullptr, fx, fy, fw, fh, 1});
    }

    return out;
}

unsigned char FillBar::CornersForState() const
{
    if (bHorizontal)
        return bInvert ? (unsigned char)0b0110   // rt|rb
                       : (unsigned char)0b1001;  // lt|lb
    else
        return bInvert ? (unsigned char)0b0011   // lb|rb
                       : (unsigned char)0b1100;  // lt|rt
}

void FillBar::EnsureBgCache(int W, int H)
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

    if (h == _bgHash && _bg)
        return;

    _bgHash = h;

    _bg = WidgetVecp::MakeRRect(
        W,
        H,
        BgColor,
        BorderColor,
        CornerRadius,
        BorderThickness,
        0b1111,
        Sub);
}

void FillBar::EnsureFillCache(int W, int H)
{
    using WidgetVecp::Mix;

    size_t h = 0;

    h = Mix(h, (size_t)W);
    h = Mix(h, (size_t)H);
    h = Mix(h, (size_t)CornerRadius);
    h = Mix(h, (size_t)Sub);
    h = Mix(h, (size_t)FillColor);

    if (h == _fillHash && _fill)
        return;

    _fillHash = h;

    _fill = WidgetVecp::MakeRRect(
        W,
        H,
        FillColor,
        0,          // borderColor
        CornerRadius,
        0,          // borderThickness
        0b1111,
        Sub);
}