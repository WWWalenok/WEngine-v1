#pragma once

#include "Widget.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class CanvasWidget : public Widget
{
public:
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

        Slot(
            int z_order = 0,
            Point pos = {{0.0f, 0.0f}, Absolute},
            Point size = {{0.0f, 0.0f}, Absolute},
            Point anchor_min = {{0.0f, 0.0f}, Reletive},
            Point anchor_max = {{1.0f, 1.0f}, Reletive})
            : z_order(z_order)
            , pos(pos)
            , size(size)
            , anchor_min(anchor_min)
            , anchor_max(anchor_max)
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

    void Add(Ref<Widget> widget, const Slot& slot);
    void Remove(Ref<Widget> widget);

    void SetPos(Ref<Widget> widget, const MVector2f& pos, const Slot::Type& type);
    void SetSize(Ref<Widget> widget, const MVector2f& size, const Slot::Type& type);
    void SetAnchor(
        Ref<Widget> widget,
        const MVector2f& pos_min,
        const Slot::Type& type_min,
        const MVector2f& pos_max,
        const Slot::Type& type_max);
    void SetZOrder(Ref<Widget> widget, int z);

    virtual std::vector<Ref<Widget>> Childs() override;

    virtual std::list<ToRender> Render(
        const double& W,
        const double& H,
        const std::unordered_map<std::string, Data>& meta) override;

    virtual bool OnClick(const double& X, const double& Y, const MouseButton& btn) override;
    virtual bool OnMouseDown(const double& X, const double& Y, const MouseButton& btn) override;
    virtual bool OnMouseUp(const double& X, const double& Y, const MouseButton& btn) override;
    virtual void OnMouseMove(const double& NX, const double& NY) override;
    virtual void OnMouseWhell(const double& V) override;

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

    Ref<Widget> DispatchPoint(
        double X,
        double Y,
        InputEvent ev,
        const MouseButton& btn,
        bool& handled);

    void SortByZDescending(std::vector<Ref<Widget>>& widgets) const;

    static double ResolveValue(double v, Slot::Type t, double axis);
    static double ResolveSize(double v, Slot::Type t, double axis);

    static void ResolveSlot(
        const Slot& slot,
        double W,
        double H,
        double& x,
        double& y,
        double& w,
        double& h);

    void RebuildChildTree(double W, double H);
};