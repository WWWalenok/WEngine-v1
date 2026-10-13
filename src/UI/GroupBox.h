#pragma once

#include "Widget.h"

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

class GroupBox : public Widget
{
public:
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
        double size = 0.0;
        SizeMode mode = SizeMode::Auto;
        double weight = 1.0;
    };

    Orientation orientation = Orientation::Vertical;
    double spacing = 0.0;
    double padding = 0.0;
    std::vector<Item> items;

    void Add(
        Ref<Widget> w,
        SizeMode mode = SizeMode::Auto,
        double size = 0.0,
        double weight = 1.0);

    void AddFixed(Ref<Widget> w, double px);
    void AddRelative(Ref<Widget> w, double frac);
    void AddWeighted(Ref<Widget> w, double wt = 1);

    void Remove(Ref<Widget> w);
    void Clear();

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
    double LastW = 0.0;
    double LastH = 0.0;
    Ref<Widget> hovered = nullptr;

    struct Rect
    {
        double x, y, w, h;
    };

    std::vector<Rect> ComputeLayout(double W, double H) const;

    enum class EEvent
    {
        Click,
        Down,
        Up
    };

    bool DispatchPoint(double X, double Y, EEvent ev, const MouseButton& btn);
};