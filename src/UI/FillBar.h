#pragma once

#include "Widget.h"
#include "WidgetDefaults.h"

#include <list>
#include <string>
#include <unordered_map>

class FillBar : public Widget
{
public:
    int CornerRadius = 6;
    int BorderThickness = 1;
    int Sub = 3;

    unsigned int BgColor = WidgetVecp::PackRGBA(60, 60, 60);
    unsigned int FillColor = WidgetVecp::PackRGBA(80, 200, 100);
    unsigned int BorderColor = WidgetVecp::PackRGBA(20, 20, 20);

    double Value = 0.0;
    bool bHorizontal = true;
    bool bInvert = false;

    FillBar() = default;
    FillBar(double v, bool horiz = true, bool invert = false);

    void SetValue(double v);

    virtual std::list<ToRender> Render(
        const double& W,
        const double& H,
        const std::unordered_map<std::string, Data>& meta) override;

private:
    Ref<Image> _bg;
    Ref<Image> _fill;
    size_t _bgHash = 0;
    size_t _fillHash = 0;

    unsigned char CornersForState() const;

    void EnsureBgCache(int W, int H);
    void EnsureFillCache(int W, int H);
};