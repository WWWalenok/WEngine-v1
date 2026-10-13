#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "../graphic/Image.h"
#include "../Text/DisplayText.h"
#include "../render/IWindowHelper.h"
#include "../input/IInputHelper.h"

class Widget : public CoreObject
{
public:
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

    virtual std::list<ToRender> Render(
        const double& W,
        const double& H,
        const std::unordered_map<std::string, Data>& meta = {}) = 0;

    virtual void OnUpdate();
    virtual void OnResize();

    virtual bool OnClick(const double& X, const double& Y, const MouseButton& btn);
    virtual bool OnMouseDown(const double& X, const double& Y, const MouseButton& btn);
    virtual bool OnMouseUp(const double& X, const double& Y, const MouseButton& btn);
    virtual void OnMouseMove(const double& NX, const double& NY);
    virtual void OnMouseLeave();
    virtual void OnMouseWhell(const double& V);

    virtual bool OnKeyPress(const KeyCode& Key);
    virtual bool OnKeyDown(const KeyCode& Key);
    virtual bool OnKeyUp(const KeyCode& Key);

    virtual std::vector<Ref<Widget>> Childs();
};