#include "Widget.h"

void Widget::OnUpdate()
{
}

void Widget::OnResize()
{
}

bool Widget::OnClick(const double&, const double&, const MouseButton&)
{
    return false;
}

bool Widget::OnMouseDown(const double&, const double&, const MouseButton&)
{
    return false;
}

bool Widget::OnMouseUp(const double&, const double&, const MouseButton&)
{
    return false;
}

void Widget::OnMouseMove(const double&, const double&)
{
}

void Widget::OnMouseLeave()
{
}

void Widget::OnMouseWhell(const double&)
{
}

bool Widget::OnKeyPress(const KeyCode&)
{
    return false;
}

bool Widget::OnKeyDown(const KeyCode&)
{
    return false;
}

bool Widget::OnKeyUp(const KeyCode&)
{
    return false;
}

std::vector<Ref<Widget>> Widget::Childs()
{
    return {};
}