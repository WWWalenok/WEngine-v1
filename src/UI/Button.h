#pragma once

#include "Widget.h"
#include "WidgetDefaults.h"

#include <functional>
#include <list>
#include <string>
#include <unordered_map>

class Button : public Widget
{
public:
    int CornerRadius = 5;
    int BorderThickness = 2;
    int Sub = 2;

    unsigned int NormalColor = WidgetVecp::PackRGBA(150, 150, 150);
    unsigned int HoveredColor = WidgetVecp::PackRGBA(190, 190, 190);
    unsigned int PressedColor = WidgetVecp::PackRGBA(110, 110, 110);
    unsigned int BorderColor = WidgetVecp::PackRGBA(40, 40, 40);

    std::function<void()> OnPressed;

    bool bHovered = false;
    bool bPressed = false;

    Button() = default;
    explicit Button(std::function<void()> cb);

    virtual std::list<ToRender> Render(
        const double& W,
        const double& H,
        const std::unordered_map<std::string, Data>& meta) override;

    virtual bool OnMouseDown(const double& X, const double& Y, const MouseButton& btn) override;
    virtual bool OnMouseUp(const double& X, const double& Y, const MouseButton& btn) override;
    virtual bool OnClick(const double& X, const double& Y, const MouseButton& btn) override;
    virtual void OnMouseMove(const double& NX, const double& NY) override;
    virtual void OnMouseLeave() override;

private:
    Ref<Image> _cache[3];
    size_t _hash = 0;

    void EnsureCache(int W, int H);
};