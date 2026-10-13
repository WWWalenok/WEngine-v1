#include <math.h>
#include <cmath>
#include <float.h>
#include <algorithm>
#include <thread>
#include <cstring>

#include "../include/VECP.h"
#include "VECP_element.h"

#include <stdio.h>
#include <iostream>
#include <strstream>
#include <unordered_map>

#ifdef __min
#undef __min
#endif
#ifdef __max
#undef __max
#endif

#define __min(a, b) ((a) < (b) ? (a) : (b))
#define __max(a, b) ((a) > (b) ? (a) : (b))

#define _begin_
#define _end_
#define COUT(V) #V " = " << V << ", "

uint32_t __get_offset(vecp::e_elememt_type type)
{
    switch (type)
    {
        case vecp::e_elememt_type::MOVE_A:
            return 2;
        case vecp::e_elememt_type::MOVE_R:
            return 2;
        case vecp::e_elememt_type::CLOSE_MOVE_A:
            return 2;
        case vecp::e_elememt_type::CLOSE_MOVE_R:
            return 2;
        case vecp::e_elememt_type::CLOSE:
            return 0;
        case vecp::e_elememt_type::LINE_A:
            return 2;
        case vecp::e_elememt_type::LINE_R:
            return 2;
        case vecp::e_elememt_type::LINE_H_A:
            return 1;
        case vecp::e_elememt_type::LINE_H_R:
            return 1;
        case vecp::e_elememt_type::LINE_W_A:
            return 1;
        case vecp::e_elememt_type::LINE_W_R:
            return 1;
        case vecp::e_elememt_type::BEZIE2_A:
            return 4;
        case vecp::e_elememt_type::BEZIE2_R:
            return 4;
        case vecp::e_elememt_type::BEZIE3_A:
            return 6;
        case vecp::e_elememt_type::BEZIE3_R:
            return 6;
        case vecp::e_elememt_type::BEZIE4_A:
            return 8;
        case vecp::e_elememt_type::BEZIE4_R:
            return 8;
        case vecp::e_elememt_type::BEZIE2_S_A:
            return 2;
        case vecp::e_elememt_type::BEZIE2_S_R:
            return 2;
        case vecp::e_elememt_type::BEZIE3_S_A:
            return 4;
        case vecp::e_elememt_type::BEZIE3_S_R:
            return 4;
        case vecp::e_elememt_type::BEZIE4_S_A:
            return 6;
        case vecp::e_elememt_type::BEZIE4_S_R:
            return 6;
        case vecp::e_elememt_type::ARC2_A:
            return 4;
        case vecp::e_elememt_type::ARC2_R:
            return 4;
        case vecp::e_elememt_type::ARC3_A:
            return 6;
        case vecp::e_elememt_type::ARC3_R:
            return 6;
        case vecp::e_elememt_type::ARC2_S_A:
            return 2;
        case vecp::e_elememt_type::ARC2_S_R:
            return 2;
        case vecp::e_elememt_type::ARC3_S_A:
            return 4;
        case vecp::e_elememt_type::ARC3_S_R:
            return 4;
        case vecp::e_elememt_type::ARCSVG_A:
            return 7;
        case vecp::e_elememt_type::ARCSVG_R:
            return 7;
        case vecp::e_elememt_type::ARCPURE:
            return 8;
        default:
            return 0;
    }
}

// =====================================================================
// bbox
// =====================================================================
struct bb_t
{
    double min_x, max_x, min_y, max_y;

    bb_t()
    {
        min_x = DBL_MAX;
        min_y = DBL_MAX;
        max_x = DBL_MIN;
        max_y = DBL_MIN;
    }

    void upd(const point_t& p)
    {
        upd(p.x, p.y);
    }

    void upd(const double& x, const double& y)
    {
        min_x = __min(min_x, x);
        max_x = __max(max_x, x);
        min_y = __min(min_y, y);
        max_y = __max(max_y, y);
    }

    bool in(const double& x, const double& y)
    {
        return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
    }
};

// =====================================================================
// cross_t
// =====================================================================
struct cross_t
{
    signed char dir = 0;
    double v = 0;
    cross_t& operator*(const double& d)
    {
        v *= d;
        return *this;
    }
};

// =====================================================================
// interpolated_t — ресэмплинг с адаптивным шагом
// =====================================================================
struct interpolated_t
{
    size_t c = 0;
    size_t a = 0;
    double* x = nullptr;
    double* y = nullptr;
    signed char dir = 1;

    interpolated_t()
    {
    }

    void add(const double& _x, const double& _y)
    {
        if (c >= a)
        {
            size_t o = a;
            if (a == 0)
                a = 2;
            a = a * 2;
            double* n;
            n = new double[a];
            if (x)
                memcpy(n, x, o * sizeof(n[0]));
            delete[] x;
            x = n;
            n = new double[a];
            if (y)
                memcpy(n, y, o * sizeof(n[0]));
            delete[] y;
            y = n;
        }
        x[c] = _x;
        y[c] = _y;
        c++;
    }

    void make(VecpElement_ptr el, double _tol = 0.125, VecpElement_ptr deb = nullptr)
    {
        bb_t tester;
        double bx, by;
        el->Get_xy_by_t(0, &bx, &by);
        tester.upd(bx, by);
        add(bx, by);

        double a = 0, l = 0;
        double t1 = 0, dt = 1.0;
        size_t step = 0;
        int depth = -1;
        double mx, my, ex, ey;

        while (1)
        {
            el->Get_xy_by_t(t1 + dt * 0.5, &mx, &my);
            el->Get_xy_by_t(t1 + dt, &ex, &ey);
            a = (ex - bx) * (my - by) - (ey - by) * (mx - bx);
            l = (ex - bx) * (ex - bx) + (ey - by) * (ey - by);
            if (a * a / l < _tol * _tol || depth >= 20)
            {
                if (t1 + dt >= 1)
                    break;
                bx = ex;
                by = ey;
                tester.upd(bx, by);
                add(bx, by);
                t1 += dt;
                unsigned char _t = (step >> depth) & 0b1;
                if (_t)
                {
                    depth--;
                    dt *= 2.0;
                }
                else
                {
                    step = step | (0b1 << depth);
                }
                if (depth == -1)
                    break;
            }
            else
            {
                depth++;
                dt *= 0.5;
                step = step | (0b1 << depth);
                step = step ^ (0b1 << depth);
            }
        }
        el->Get_xy_by_t(1, &bx, &by);
        add(bx, by);
    }

    void make_offset(VecpElement_ptr el, double offset, double _tol = 0.125)
    {
        if (offset < 0)
            dir = (signed char)(-1);
        double bx, by, nx, ny;
        el->Get_xy_by_t(0, &bx, &by);
        el->Get_nxny_by_t(0, &nx, &ny);
        bx += nx * offset;
        by += ny * offset;
        add(bx, by);

        double a = 0, l = 0;
        double t1 = 0, dt = 1.0;
        size_t step = 0;
        int depth = -1;
        double mx, my, ex, ey;

        while (1)
        {
            el->Get_xy_by_t(t1 + dt * 0.5, &mx, &my);
            el->Get_nxny_by_t(t1 + dt * 0.5, &nx, &ny);
            mx += nx * offset;
            my += ny * offset;
            el->Get_xy_by_t(t1 + dt, &ex, &ey);
            el->Get_nxny_by_t(t1 + dt, &nx, &ny);
            ex += nx * offset;
            ey += ny * offset;
            a = (ex - bx) * (my - by) - (ey - by) * (mx - bx);
            l = (ex - bx) * (ex - bx) + (ey - by) * (ey - by);
            if (a * a / l < _tol * _tol || depth >= 20)
            {
                if (t1 + dt >= 1)
                    break;
                bx = ex;
                by = ey;
                add(bx, by);
                t1 += dt;
                unsigned char _t = (step >> depth) & 0b1;
                if (_t)
                {
                    depth--;
                    dt *= 2.0;
                }
                else
                {
                    step = step | (0b1 << depth);
                }
                if (depth == -1)
                    break;
            }
            else
            {
                depth++;
                dt *= 0.5;
                step = step | (0b1 << depth);
                step = step ^ (0b1 << depth);
            }
        }
        el->Get_xy_by_t(1, &bx, &by);
        el->Get_nxny_by_t(1, &nx, &ny);
        bx += nx * offset;
        by += ny * offset;
        add(bx, by);
    }

    void clear()
    {
        delete[] x;
        x = nullptr;
        delete[] y;
        y = nullptr;
        c = 0;
        a = 0;
    }

    ~interpolated_t()
    {
        clear();
    }

    void get_begin(double& _x, double& _y) const
    {
        if (!c)
            return;
        _x = x[0];
        _y = y[0];
    }
    void get_end(double& _x, double& _y) const
    {
        if (!c)
            return;
        _x = x[c - 1];
        _y = y[c - 1];
    }

    cross_t Get_x_by_y(double target_y)
    {
        double x0, x1, y0, y1;
        get_begin(x0, y0);
        get_end(x1, y1);
        if ((y0 - target_y) * (y1 - target_y) > 0)
            return {0, NAN};

        signed char d = y[1] > y[0] ? dir : -dir;
        if (c == 2)
            return {d, x[0] + (x[1] - x[0]) * ((target_y - y[0]) / (y[1] - y[0]))};

        const bool increasing = (y1 > y0);
        int i0 = 0, i1 = (int)c - 1;
        while (i1 - i0 > 1)
        {
            int i = (i0 + i1) >> 1;
            if ((increasing && (y[i] < target_y)) || (!increasing && (y[i] > target_y)))
                i0 = i;
            else
                i1 = i;
        }
        return {d, x[i0] + (x[i1] - x[i0]) * (target_y - y[i0]) / (y[i1] - y[i0])};
    }

    cross_t Get_y_by_x(double target_x)
    {
        double x0, x1, y0, y1;
        get_begin(x0, y0);
        get_end(x1, y1);
        if ((x0 - target_x) * (x1 - target_x) > 0)
            return {0, NAN};

        signed char d = x[1] > x[0] ? dir : -dir;
        if (c == 2)
            return {d, y[0] + (y[1] - y[0]) * ((target_x - x[0]) / (x[1] - x[0]))};

        const bool increasing = (x1 > x0);
        int i0 = 0, i1 = (int)c - 1;
        while (i1 - i0 > 1)
        {
            int i = (i0 + i1) >> 1;
            if ((increasing && (x[i] < target_x)) || (!increasing && (x[i] > target_x)))
                i0 = i;
            else
                i1 = i;
        }
        return {d, y[i0] + (y[i1] - y[i0]) * (target_x - x[i0]) / (x[i1] - x[i0])};
    }
};

// =====================================================================
// Геометрические утилиты
// =====================================================================
static bool segments_intersect(const point_t& p1, const point_t& p2, const point_t& p3, const point_t& p4)
{
    const auto dx12 = p2.x - p1.x, dy12 = p2.y - p1.y;
    const auto dx34 = p4.x - p3.x, dy34 = p4.y - p3.y;
    const auto o1 = dx12 * (p3.y - p1.y) - dy12 * (p3.x - p1.x);
    const auto o2 = dx12 * (p4.y - p1.y) - dy12 * (p4.x - p1.x);
    if (o1 * o2 > 0)
        return false;
    const auto o3 = dx34 * (p1.y - p3.y) - dy34 * (p1.x - p3.x);
    const auto o4 = dx34 * (p2.y - p3.y) - dy34 * (p2.x - p3.x);
    if (o1 * o2 < 0 && o3 * o4 < 0)
        return true;

    auto in_bbox = [](const point_t& a, const point_t& b, const point_t& p) {
        return p.x >= (a.x < b.x ? a.x : b.x) && p.x <= (a.x > b.x ? a.x : b.x) && p.y >= (a.y < b.y ? a.y : b.y) && p.y <= (a.y > b.y ? a.y : b.y);
    };
    if (o1 == 0 && in_bbox(p1, p2, p3))
        return true;
    if (o2 == 0 && in_bbox(p1, p2, p4))
        return true;
    if (o3 == 0 && in_bbox(p3, p4, p1))
        return true;
    if (o4 == 0 && in_bbox(p3, p4, p2))
        return true;
    return false;
}

static bool is_on_segment(int Ax, int Ay, int Bx, int By, int Px, int Py)
{
    return (Px >= (Ax < Bx ? Ax : Bx)) && (Px <= (Ax > Bx ? Ax : Bx)) && (Py >= (Ay < By ? Ay : By)) && (Py <= (Ay > By ? Ay : By));
}

// =====================================================================
// border_t / subpath_t
// =====================================================================
struct subpath_t;

struct border_t
{
    subpath_t* base_path = nullptr;
    int counterid = 0;
    int incounterid = 0;
    VecpElement_ptr base_element = nullptr;
    border_t* pre_border = nullptr;
    border_t* post_border = nullptr;

    interpolated_t interpol;
    interpolated_t interpol_in;
    interpolated_t interpol_out;

    VecpElement_ptr add_elements[20]{};
    interpolated_t interpol_add[20];

    bool hide_in = false;
    bool hide_out = false;
    double inner_start = 0, outer_start = 0;
    double inner_end = 1, outer_end = 1;

    ~border_t()
    {
        for (int i = 0; i < 20; ++i)
        {
            delete add_elements[i];
            add_elements[i] = nullptr;
        }
    }
};

struct subpath_t
{
    int status = 0;
    std::vector<VecpElement_ptr> splited;
    std::vector<std::vector<border_t*>> counters;
    std::vector<border_t*> borders;
    vecp::counter_t* base = nullptr;
    transform_matrix tm{};
    bb_t bb;

    ~subpath_t()
    {
        for (auto& e : splited)
        {
            delete e;
            e = nullptr;
        }
        for (auto* b : borders)
        {
            delete b;
            b = nullptr;
        }
    }
};

// =====================================================================
// check_border — из нового алгоритма
// =====================================================================
void check_border(border_t* ret, double offset_inner, double offset_outer, double tol)
{
    auto base_element = ret->base_element;

    double bx, ex, by, ey, ibx, iex, iby, iey, obx, oex, oby, oey;
    base_element->Get_xy_by_t(0, &bx, &by);
    base_element->Get_xy_by_t(1, &ex, &ey);
    base_element->Get_xy_by_t_offset(-offset_inner, 0, &ibx, &iby);
    base_element->Get_xy_by_t_offset(-offset_inner, 1, &iex, &iey);
    base_element->Get_xy_by_t_offset(offset_outer, 0, &obx, &oby);
    base_element->Get_xy_by_t_offset(offset_outer, 1, &oex, &oey);

    ret->hide_in = segments_intersect({bx, by}, {ibx, iby}, {ex, ey}, {iex, iey});
    ret->hide_out = segments_intersect({bx, by}, {obx, oby}, {ex, ey}, {oex, oey});

    if (!ret->hide_in)
        ret->interpol_in.make_offset(ret->base_element, -offset_inner, tol);
    if (!ret->hide_out)
        ret->interpol_out.make_offset(ret->base_element, offset_outer, tol);
}

// =====================================================================
// generate_border — из нового алгоритма (с tpm-триммингом)
// =====================================================================
void generate_border(border_t* ret, double offset_inner, double offset_outer, int corner_type, double tol)
{
    auto pre_border = ret->pre_border;
    auto post_border = ret->post_border;
    auto base_element = ret->base_element;
    auto pre_element = pre_border ? pre_border->base_element : nullptr;
    auto post_element = post_border ? post_border->base_element : nullptr;

    double bx, ex, by, ey;
    double ibx, iex, ipbx, ipex, iby, iey, ipby, ipey;
    double obx, oex, opbx, opex, oby, oey, opby, opey;

    base_element->Get_xy_by_t(0, &bx, &by);
    base_element->Get_xy_by_t(1, &ex, &ey);
    base_element->Get_xy_by_t_offset(-offset_inner, 0, &ibx, &iby);
    base_element->Get_xy_by_t_offset(-offset_inner, 1, &iex, &iey);
    base_element->Get_xy_by_t_offset(offset_outer, 0, &obx, &oby);
    base_element->Get_xy_by_t_offset(offset_outer, 1, &oex, &oey);

    if (pre_element)
    {
        pre_element->Get_xy_by_t_offset(-offset_inner, 1, &ipbx, &ipby);
        pre_element->Get_xy_by_t_offset(offset_outer, 1, &opbx, &opby);
    }
    else
    {
        ipbx = opbx = bx;
        ipby = opby = by;
    }

    if (post_element)
    {
        post_element->Get_xy_by_t_offset(-offset_inner, 0, &ipex, &ipey);
        post_element->Get_xy_by_t_offset(offset_outer, 0, &opex, &opey);
    }
    else
    {
        ipex = opex = ex;
        ipey = opey = ey;
    }

    double nbx = (obx - ibx) / (offset_inner + offset_outer);
    double nby = (oby - iby) / (offset_inner + offset_outer);
    double nex = (oex - iex) / (offset_inner + offset_outer);
    double ney = (oey - iey) / (offset_inner + offset_outer);
    double npbx = (opbx - ipbx) / (offset_inner + offset_outer);
    double npby = (opby - ipby) / (offset_inner + offset_outer);

    if ((nbx == 0 && nby == 0) || (nex == 0 && ney == 0))
        throw 1;

    bool hide_in = ret->hide_in;
    bool hide_out = ret->hide_out;

    // ---- tpm: обрезка интерполяции по пересечению с соседним бордером ----
    struct __helper
    {
        static point_t find_intersection(point_t p1, point_t p2, point_t p3, point_t p4)
        {
            const double dx1 = p2.x - p1.x, dy1 = p2.y - p1.y;
            const double dx2 = p4.x - p3.x, dy2 = p4.y - p3.y;
            const double D = dx1 * dy2 - dx2 * dy1;
            if (D == 0)
            {
                if (is_on_segment((int)p3.x, (int)p3.y, (int)p4.x, (int)p4.y, (int)p1.x, (int)p1.y))
                    return {p1.x, p1.y};
                if (is_on_segment((int)p3.x, (int)p3.y, (int)p4.x, (int)p4.y, (int)p2.x, (int)p2.y))
                    return {p2.x, p2.y};
                if (is_on_segment((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, (int)p3.x, (int)p3.y))
                    return {p3.x, p3.y};
                return {p4.x, p4.y};
            }
            const double num = (p3.x - p1.x) * dy2 - (p3.y - p1.y) * dx2;
            const double t = num / D;
            return {p1.x + t * dx1, p1.y + t * dy1};
        }

        static void tpm(interpolated_t& a, interpolated_t& b, bool ba, bool bb_)
        {
            bb_t bba, bbb;
            for (size_t i = 0; i < a.c; ++i)
                bba.upd(a.x[i], a.y[i]);
            for (size_t i = 0; i < b.c; ++i)
                bbb.upd(b.x[i], b.y[i]);

            int pi = -1;
            bool find = false;
            point_t f;
            int fi = 0, fj = 0;

            for (int i = 0; i < (int)a.c - 1 && !find; ++i)
            {
                point_t a1{a.x[i], a.y[i]};
                point_t a2{a.x[i + 1], a.y[i + 1]};
                for (int j = 0; j < (int)b.c - 1; ++j)
                {
                    point_t b1{b.x[j], b.y[j]};
                    point_t b2{b.x[j + 1], b.y[j + 1]};
                    if (segments_intersect(a1, a2, b1, b2))
                    {
                        f = find_intersection(a1, a2, b1, b2);
                        fi = i;
                        fj = j;
                        find = true;
                        break;
                    }
                }
                pi = i;
            }

            if (!find)
                return;

            if (ba)
            {
                double* nx = new double[a.c - fi];
                double* ny = new double[a.c - fi];
                for (size_t k = fi; k < a.c; ++k)
                {
                    nx[k - fi] = a.x[k];
                    ny[k - fi] = a.y[k];
                }
                delete[] a.x;
                delete[] a.y;
                a.x = nx;
                a.y = ny;
                a.c = a.c - fi;
                a.a = a.c;
                a.x[0] = f.x;
                a.y[0] = f.y;
            }
            else
            {
                double* nx = new double[fi + 2];
                double* ny = new double[fi + 2];
                for (int k = 0; k <= fi; ++k)
                {
                    nx[k] = a.x[k];
                    ny[k] = a.y[k];
                }
                delete[] a.x;
                delete[] a.y;
                a.x = nx;
                a.y = ny;
                a.c = fi + 2;
                a.a = a.c;
                a.x[a.c - 1] = f.x;
                a.y[a.c - 1] = f.y;
            }
            if (bb_)
            {
                double* nx = new double[b.c - fj];
                double* ny = new double[b.c - fj];
                for (size_t k = fj; k < b.c; ++k)
                {
                    nx[k - fj] = b.x[k];
                    ny[k - fj] = b.y[k];
                }
                delete[] b.x;
                delete[] b.y;
                b.x = nx;
                b.y = ny;
                b.c = b.c - fj;
                b.a = b.c;
                b.x[0] = f.x;
                b.y[0] = f.y;
            }
            else
            {
                double* nx = new double[fj + 2];
                double* ny = new double[fj + 2];
                for (int k = 0; k <= fj; ++k)
                {
                    nx[k] = b.x[k];
                    ny[k] = b.y[k];
                }
                delete[] b.x;
                delete[] b.y;
                b.x = nx;
                b.y = ny;
                b.c = fj + 2;
                b.a = b.c;
                b.x[b.c - 1] = f.x;
                b.y[b.c - 1] = f.y;
            }
        }
    };

    if (!hide_in && pre_border)
    {
        auto sb = pre_border;
        while (sb && sb->hide_in)
            sb = sb->pre_border;
        if (sb)
            __helper::tpm(ret->interpol_in, sb->interpol_in, true, false);
    }
    if (!hide_out && pre_border)
    {
        auto sb = pre_border;
        while (sb && sb->hide_out)
            sb = sb->pre_border;
        if (sb)
            __helper::tpm(ret->interpol_out, sb->interpol_out, true, false);
    }
    if (!hide_in && post_border)
    {
        auto sb = post_border;
        while (sb && sb->hide_in)
            sb = sb->post_border;
        if (sb)
            __helper::tpm(ret->interpol_in, sb->interpol_in, false, true);
    }
    if (!hide_out && post_border)
    {
        auto sb = post_border;
        while (sb && sb->hide_out)
            sb = sb->post_border;
        if (sb)
            __helper::tpm(ret->interpol_out, sb->interpol_out, false, true);
    }

    ret->interpol_in.get_begin(ibx, iby);
    ret->interpol_in.get_end(iex, iey);
    ret->interpol_out.get_begin(obx, oby);
    ret->interpol_out.get_end(oex, oey);

    // Snap-подгонка соседних точек
    auto snap2 = [&](double& v1x, double& v1y, double& v2x, double& v2y, interpolated_t& a, bool endA, interpolated_t& b, bool begB) {
        double di = (v1x - v2x) * (v1x - v2x) + (v1y - v2y) * (v1y - v2y);
        if (di > 0 && di < tol * tol)
        {
            v1x = v2x = (v1x + v2x) * 0.5;
            v1y = v2y = (v1y + v2y) * 0.5;
            if (a.c)
            {
                a.x[endA ? a.c - 1 : 0] = v1x;
                a.y[endA ? a.c - 1 : 0] = v1y;
            }
            if (b.c)
            {
                b.x[begB ? 0 : b.c - 1] = v2x;
                b.y[begB ? 0 : b.c - 1] = v2y;
            }
        }
    };

    if (pre_border)
    {
        pre_border->interpol_in.get_end(ipbx, ipby);
        pre_border->interpol_out.get_end(opbx, opby);
        double t;
        snap2(ipbx, ipby, ibx, iby, pre_border->interpol_in, true, ret->interpol_in, true);
        snap2(opbx, opby, obx, oby, pre_border->interpol_out, true, ret->interpol_out, true);
    }
    else
    {
        ipbx = opbx = bx;
        ipby = opby = by;
    }

    if (post_border)
    {
        post_border->interpol_in.get_begin(ipex, ipey);
        post_border->interpol_out.get_begin(opex, opey);
        snap2(ipex, ipey, iex, iey, post_border->interpol_in, false, ret->interpol_in, false);
        snap2(opex, opey, oex, oey, post_border->interpol_out, false, ret->interpol_out, false);
    }
    else
    {
        ipex = opex = ex;
        ipey = opey = ey;
    }

    int add_id = 0;

    // ============ corner start (0x10..0x60 / default arc) ============
    if ((corner_type & 0xf0) == 0x10)
    {
        if (!pre_border)
        {
            VecpLineElement t{obx, oby, ibx, iby};
            add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            VecpLineElement t{iex, iey, oex, oey};
            add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }
    else if ((corner_type & 0xf0) == 0x20)
    {
        if (!pre_border)
        {
            double npby = -(obx - ibx) / 2.0, npbx = (oby - iby) / 2.0;
            VecpLineElement t1{obx, oby, obx + npbx, oby + npby};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{obx + npbx, oby + npby, ibx + npbx, iby + npby};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t3{ibx + npbx, iby + npby, ibx, iby};
            add_id += t3.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            double npey = (oex - iex) / 2.0, npex = -(oey - iey) / 2.0;
            VecpLineElement t1{iex, iey, iex + npex, iey + npey};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{iex + npex, iey + npey, oex + npex, oey + npey};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t3{oex + npex, oey + npey, oex, oey};
            add_id += t3.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }
    else if ((corner_type & 0xf0) == 0x30)
    {
        if (!pre_border)
        {
            double npby = -(obx - ibx) / 2.0, npbx = (oby - iby) / 2.0;
            VecpLineElement t1{obx, oby, (ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby, ibx, iby};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            double npey = (oex - iex) / 2.0, npex = -(oey - iey) / 2.0;
            VecpLineElement t1{iex, iey, (iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey, oex, oey};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }
    else if ((corner_type & 0xf0) == 0x40)
    {
        if (!pre_border)
        {
            double npby = (obx - ibx) / 2.0, npbx = -(oby - iby) / 2.0;
            VecpLineElement t1{obx, oby, (ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby, ibx, iby};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            double npey = -(oex - iex) / 2.0, npex = (oey - iey) / 2.0;
            VecpLineElement t1{iex, iey, (iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey, oex, oey};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }
    else if ((corner_type & 0xf0) == 0x50)
    {
        if (!pre_border)
        {
            double npby = (obx - ibx) / 2.0, npbx = -(oby - iby) / 2.0;
            VecpLineElement t1{obx, oby, (ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby, ibx, iby};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            double npey = (oex - iex) / 2.0, npex = -(oey - iey) / 2.0;
            VecpLineElement t1{iex, iey, (iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey, oex, oey};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }
    else if ((corner_type & 0xf0) == 0x60)
    {
        if (!pre_border)
        {
            double npby = -(obx - ibx) / 2.0, npbx = (oby - iby) / 2.0;
            VecpLineElement t1{obx, oby, (ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(ibx + obx) * 0.5 + npbx, (iby + oby) * 0.5 + npby, ibx, iby};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            double npey = -(oex - iex) / 2.0, npex = (oey - iey) / 2.0;
            VecpLineElement t1{iex, iey, (iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey};
            add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
            VecpLineElement t2{(iex + oex) * 0.5 + npex, (iey + oey) * 0.5 + npey, oex, oey};
            add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }
    else
    {
        if (!pre_border)
        {
            VecpArcElement t{(obx + ibx) * 0.5, (oby + iby) * 0.5, -nbx, -nby, (offset_inner + offset_outer) * 0.5, (offset_inner + offset_outer) * 0.5, 0, 1};
            add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
        if (!post_border)
        {
            VecpArcElement t{(oex + iex) * 0.5, (oey + iey) * 0.5, nex, ney, (offset_inner + offset_outer) * 0.5, (offset_inner + offset_outer) * 0.5, 0, 1};
            add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
        }
    }

    // ============ corner end (0x1..0x2 / default arc) ============
    if ((corner_type & 0xf) == 0x1)
    {
        if (pre_border)
        {
            if (!hide_in && !pre_border->hide_in && (ibx != ipbx || iby != ipby))
            {
                VecpLineElement t{ibx, iby, ipbx, ipby};
                add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
            }
            if (!hide_out && !pre_border->hide_out && (obx != opbx || oby != opby))
            {
                VecpLineElement t{opbx, opby, obx, oby};
                add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
            }
        }
    }
    else if ((corner_type & 0xf) == 0x2)
    {
        if (pre_border)
        {
            double l = sqrt(npbx * npbx + npby * npby);
            npbx /= l;
            npby /= l;
            double k = nbx * npbx + nby * npby;
            npbx /= k;
            npby /= k;

            if (!hide_in && !pre_border->hide_in && (ibx != ipbx || iby != ipby))
            {
                double nx = bx - npbx * offset_inner, ny = by - npby * offset_inner;
                VecpLineElement t1{ibx, iby, nx, ny};
                add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
                VecpLineElement t2{nx, ny, ipbx, ipby};
                add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
            }
            if (!hide_out && !pre_border->hide_out && (obx != opbx || oby != opby))
            {
                double nx = bx + npbx * offset_outer, ny = by + npby * offset_outer;
                VecpLineElement t1{opbx, opby, nx, ny};
                add_id += t1.Resample_element(ret->add_elements + add_id, 20 - add_id);
                VecpLineElement t2{nx, ny, obx, oby};
                add_id += t2.Resample_element(ret->add_elements + add_id, 20 - add_id);
            }
        }
    }
    else
    {
        if (pre_border)
        {
            if (!hide_in && !pre_border->hide_in && (ibx != ipbx || iby != ipby))
            {
                double k = (1 - (nbx * npbx + nby * npby)) * 0.5;
                if (-nby * npbx + nbx * npby < 0)
                    k = -k;
                VecpArcElement t{bx, by, -nbx, -nby, offset_inner, offset_inner, 0, k};
                add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
            }
            if (!hide_out && !pre_border->hide_out && (obx != opbx || oby != opby))
            {
                double k = (1 - (nbx * npbx + nby * npby)) * 0.5;
                if (-nby * npbx + nbx * npby < 0)
                    k = -k;
                VecpArcElement t{bx, by, npbx, npby, offset_outer, offset_outer, 0, -k};
                add_id += t.Resample_element(ret->add_elements + add_id, 20 - add_id);
            }
        }
    }

    // Построить интерполяции для добавочных элементов и обнулить их.
    for (int i = 0; i < 20; ++i)
    {
        if (ret->add_elements[i])
        {
            ret->interpol_add[i].make(ret->add_elements[i], tol);
            delete ret->add_elements[i];
            ret->add_elements[i] = nullptr;
        }
    }
}

// =====================================================================
// LoadBinary / ToBinary
// =====================================================================
namespace vecp
{

void LoadBinary(vector_t* _target, const void* _data, const size_t size)
{
    const unsigned char* data = (const unsigned char*)_data;
    size_t offset = 0;
    vector_t& target = *_target;
    target.size = size;
#define READ(to, count)                   \
    {                                     \
        if (offset + count > target.size) \
            throw 1;                      \
        memcpy(to, data + offset, count); \
        offset += count;                  \
    }
    READ(_target, sizeof(vector_t_data));
    if (target.size > size)
        throw 1;

    if (target.metadata_size)
    {
        target.metadata = new uint8_t[target.metadata_size];
        READ(target.metadata, target.metadata_size);
        target.metadata_allocated = target.metadata_size;
    }
    if (target.properties_size)
    {
        target.properties = new uint8_t[target.properties_size];
        READ(target.properties, target.properties_size);
        target.properties_allocated = target.properties_size;
    }

    if (target.counters_size)
    {
        target.counters = new counter_t[target.counters_size];
        target.counters_allocated = target.counters_size;
        for (int i = 0; i < target.counters_size; ++i)
        {
            auto& cnt = target.counters[i];
            READ(&cnt, sizeof(counter_t_data));
            if (cnt.metadata_size)
            {
                cnt.metadata = new uint8_t[cnt.metadata_size];
                READ(cnt.metadata, cnt.metadata_size);
                cnt.metadata_allocated = cnt.metadata_size;
            }
            if (cnt.properties_size)
            {
                cnt.properties = new uint8_t[cnt.properties_size];
                READ(cnt.properties, cnt.properties_size);
                cnt.properties_allocated = cnt.properties_size;
            }
            if (cnt.elements_size)
            {
                cnt.elements = new e_elememt_type[cnt.elements_size];
                READ(cnt.elements, cnt.elements_size * sizeof(e_elememt_type));
                cnt.elements_allocated = cnt.elements_size;
            }
            if (cnt.data_size)
            {
                cnt.data = new double[cnt.data_size];
                READ(cnt.data, cnt.data_size * sizeof(double));
                cnt.data_allocated = cnt.data_size;
            }
        }
    }
#undef READ
}

size_t ToBinary(const vector_t* _target, void* _data, size_t allocated)
{
    uint8_t* data = (uint8_t*)_data;
    size_t offset = 0;
    const vector_t& target = *_target;
#define WRITE(from, count)                        \
    {                                             \
        if (_data && offset + count <= allocated) \
            memcpy(data + offset, from, count);   \
        offset += count;                          \
    }

    WRITE(_target, sizeof(vector_t_data));
    if (target.metadata_size)
        WRITE(target.metadata, target.metadata_size);
    if (target.properties_size)
        WRITE(target.properties, target.properties_size);

    if (target.counters_size)
    {
        for (int i = 0; i < target.counters_size; ++i)
        {
            const auto& cnt = target.counters[i];
            auto cnt_off = offset;
            WRITE(&cnt, sizeof(counter_t_data));
            if (cnt.metadata_size)
                WRITE(cnt.metadata, cnt.metadata_size);
            if (cnt.properties_size)
                WRITE(cnt.properties, cnt.properties_size);
            if (cnt.elements_size)
                WRITE(cnt.elements, cnt.elements_size * sizeof(e_elememt_type));
            if (cnt.data_size)
                WRITE(cnt.data, cnt.data_size * sizeof(double));
            if (_data)
            {
                ((counter_t_data*)(data + cnt_off))->size = (uint32_t)(offset - cnt_off);
            }
        }
    }

    if (_data)
    {
        vector_t_data* res = (vector_t_data*)_data;
        res->size = (uint32_t)offset;
        res->f_type = 0x564c5045;
    }
#undef WRITE
    return offset;
}

// =====================================================================
// fill helpers
// =====================================================================
void fill(uint8_t* _col, const void* bcol)
{
    auto c = (const uint8_t*)bcol;
    _col[0] = c[0];
    _col[1] = c[1];
    _col[2] = c[2];
    _col[3] = c[3];
}
void fill(uint16_t* _col, const void* bcol)
{
    auto c = (const uint8_t*)bcol;
    _col[0] = uint16_t(c[0] * 0x100);
    _col[1] = uint16_t(c[1] * 0x100);
    _col[2] = uint16_t(c[2] * 0x100);
    _col[3] = uint16_t(c[3] * 0x100);
}
void fill(float* _col, const void* bcol)
{
    auto c = (const uint8_t*)bcol;
    _col[0] = c[0] / 255.f;
    _col[1] = c[1] / 255.f;
    _col[2] = c[2] / 255.f;
    _col[3] = c[3] / 255.f;
}
void fill(double* _col, const void* bcol)
{
    auto c = (const uint8_t*)bcol;
    _col[0] = c[0] / 255.;
    _col[1] = c[1] / 255.;
    _col[2] = c[2] / 255.;
    _col[3] = c[3] / 255.;
}

bb_t GetVecBB(const vector_t& vector)
{
    bb_t out_bb;

    point_t p1, p2, p3, p4;
    point_t a1, a2, a3, a4, a5;
    point_t r1, r2, r3, r4, r5;
    point_t la;

    VecpElement_ptr resamples[16];

    for (auto c = vector.counters; c < vector.counters + vector.counters_size; ++c)
    {
        bb_t bb;
        auto data = c->data;
        subpath_t subpath;
        subpath.base = c;
        uint32_t offset = 0;
        uint32_t counterid = 0;
        subpath.counters.push_back({});

        bool newline = true;
        bool next_newline = true;

        auto tm = transform_matrix::mult(*((transform_matrix*)(&vector.t_xx)), *((transform_matrix*)(&c->t_xx)));
        const double brot = atan2(tm.xy, tm.xx);
        const double co = cos(brot);
        const double si = sin(brot);

        subpath.tm = {
          tm.xx * co + tm.xy * si,
          tm.xy * co - tm.xx * si,
          tm.x,
          tm.yx * co + tm.yy * si,
          tm.yy * co - tm.yx * si,
          tm.y,
        };

        double offx = tm.x / subpath.tm.xx;
        double offy = tm.y / subpath.tm.yy;

        double ox = offx, oy = offy, bx = offx, by = offy, px = 0, py = 0;
        bool closed = true;
        VecpElement_ptr last = nullptr;

        for (auto e = c->elements; e < c->elements + c->elements_size; ++e)
        {
            if (next_newline)
            {
                newline = true;
                next_newline = false;
            }
            VecpElement_ptr cur = nullptr;
            if (offset >= c->data_size && *e != vecp::e_elememt_type::CLOSE)
                break;

            a1 = {bx, by};
            if(offset + 1 < c->data_size)
                a2 = {data[offset + 0] * co - data[offset + 1] * si, data[offset + 1] * co + data[offset + 0] * si};
            else
                a2 = {0.0, 0.0};
            if(offset + 3 < c->data_size)
                a3 = {data[offset + 2] * co - data[offset + 3] * si, data[offset + 3] * co + data[offset + 2] * si};
            else
                a3 = {0.0, 0.0};
            if(offset + 5 < c->data_size)
                a4 = {data[offset + 4] * co - data[offset + 5] * si, data[offset + 5] * co + data[offset + 4] * si};
            else
                a4 = {0.0, 0.0};
            if(offset + 7 < c->data_size)
                a5 = {data[offset + 6] * co - data[offset + 7] * si, data[offset + 7] * co + data[offset + 6] * si};
            else
                a5 = {0.0, 0.0};

            double d;
            r1 = {bx, by};
            r2 = r1 + a2;
            r3 = r1 + a3;
            r4 = r1 + a4;
            r5 = r1 + a5;

            a2.x += offx;
            a3.x += offx;
            a4.x += offx;
            a5.x += offx;
            a2.y += offy;
            a3.y += offy;
            a4.y += offy;
            a5.y += offy;

            auto snap = [&](point_t& p) {
                d = (p.x - ox) * (p.x - ox) + (p.y - oy) * (p.y - oy);
                if (d < 1e-10)
                {
                    p.x = ox;
                    p.y = oy;
                }
            };
            snap(a1);
            snap(a2);
            snap(a3);
            snap(a4);
            snap(a5);
            snap(r1);
            snap(r2);
            snap(r3);
            snap(r4);
            snap(r5);

            la = {bx * 2 - px, by * 2 - py};

            using namespace vecp;

            switch (*e)
            {
                default:
                    break;

                case e_elememt_type::MOVE_A:
                    closed = closed && (bx == ox && by == oy);
                    px = a1.x;
                    py = a1.y;
                    bx = a2.x;
                    by = a2.y;
                    ox = bx;
                    oy = by;
                    newline = true;
                    break;
                case e_elememt_type::MOVE_R:
                    closed = closed && (bx == ox && by == oy);
                    px = r1.x;
                    py = r1.y;
                    bx = r2.x;
                    by = r2.y;
                    ox = bx;
                    oy = by;
                    newline = true;
                    break;
                case e_elememt_type::CLOSE:
                    if (!(bx == ox && by == oy))
                        cur = new VecpLineElement({bx, by, ox, oy});
                    bx = ox;
                    by = oy;
                    next_newline = true;
                    break;
                case e_elememt_type::CLOSE_MOVE_A:
                    if (!(bx == ox && by == oy))
                        cur = new VecpLineElement({bx, by, ox, oy});
                    px = a1.x;
                    py = a1.y;
                    bx = a2.x;
                    by = a2.y;
                    ox = bx;
                    oy = by;
                    next_newline = true;
                    break;
                case e_elememt_type::CLOSE_MOVE_R:
                    if (!(bx == ox && by == oy))
                        cur = new VecpLineElement({bx, by, ox, oy});
                    px = r1.x;
                    py = r1.y;
                    bx = r2.x;
                    by = r2.y;
                    ox = bx;
                    oy = by;
                    next_newline = true;
                    break;
                case e_elememt_type::LINE_A:
                    cur = new VecpLineElement({a1.x, a1.y, a2.x, a2.y});
                    px = a1.x;
                    py = a1.y;
                    bx = a2.x;
                    by = a2.y;
                    break;
                case e_elememt_type::LINE_H_A: {
                    point_t _t{data[offset + 0] * co - (by - offy) * si + offx, (by - offy) * co + data[offset + 0] * si + offy};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::LINE_W_A: {
                    point_t _t{(bx - offx) * co - data[offset + 0] * si + offx, data[offset + 0] * co + (bx - offx) * si + offy};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::BEZIE2_A:
                    cur = new VecpBezie2Element({a1.x, a1.y, a2.x, a2.y, a3.x, a3.y});
                    px = a2.x;
                    py = a2.y;
                    bx = a3.x;
                    by = a3.y;
                    bb.upd(a2);
                    break;
                case e_elememt_type::BEZIE3_A:
                    cur = new VecpBezie3Element({a1.x, a1.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y});
                    px = a3.x;
                    py = a3.y;
                    bx = a4.x;
                    by = a4.y;
                    bb.upd(a2);
                    bb.upd(a3);
                    break;
                case e_elememt_type::BEZIE4_A:
                    cur = new VecpBezie4Element({a1.x, a1.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y, a5.x, a5.y});
                    px = a4.x;
                    py = a4.y;
                    bx = a5.x;
                    by = a5.y;
                    bb.upd(a2);
                    bb.upd(a3);
                    bb.upd(a4);
                    break;
                case e_elememt_type::BEZIE2_S_A:
                    cur = new VecpBezie2Element({a1.x, a1.y, la.x, la.y, a2.x, a2.y});
                    px = la.x;
                    py = la.y;
                    bx = a2.x;
                    by = a2.y;
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE3_S_A:
                    cur = new VecpBezie3Element({a1.x, a1.y, la.x, la.y, a2.x, a2.y, a3.x, a3.y});
                    px = a2.x;
                    py = a2.y;
                    bx = a3.x;
                    by = a3.y;
                    bb.upd(a2);
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE4_S_A:
                    cur = new VecpBezie4Element({a1.x, a1.y, la.x, la.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y});
                    px = a3.x;
                    py = a3.y;
                    bx = a4.x;
                    by = a4.y;
                    bb.upd(a2);
                    bb.upd(a3);
                    bb.upd(la);
                    break;
                case e_elememt_type::LINE_R:
                    cur = new VecpLineElement({r1.x, r1.y, r2.x, r2.y});
                    px = r1.x;
                    py = r1.y;
                    bx = r2.x;
                    by = r2.y;
                    break;
                case e_elememt_type::LINE_H_R: {
                    point_t _t{data[offset + 0] * co + bx, data[offset + 0] * si + by};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::LINE_W_R: {
                    point_t _t{-data[offset + 0] * si + bx, data[offset + 0] * co + by};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::BEZIE2_R:
                    cur = new VecpBezie2Element({r1.x, r1.y, r2.x, r2.y, r3.x, r3.y});
                    px = r2.x;
                    py = r2.y;
                    bx = r3.x;
                    by = r3.y;
                    bb.upd(r2);
                    break;
                case e_elememt_type::BEZIE3_R:
                    cur = new VecpBezie3Element({r1.x, r1.y, r2.x, r2.y, r3.x, r3.y, r4.x, r4.y});
                    px = r3.x;
                    py = r3.y;
                    bx = r4.x;
                    by = r4.y;
                    bb.upd(r2);
                    bb.upd(r3);
                    break;
                case e_elememt_type::BEZIE4_R:
                    cur = new VecpBezie4Element({r1.x, r1.y, r2.x, r2.y, r3.x, r3.y, r4.x, r4.y, r5.x, r5.y});
                    px = r4.x;
                    py = r4.y;
                    bx = r5.x;
                    by = r5.y;
                    bb.upd(r2);
                    bb.upd(r3);
                    bb.upd(r4);
                    break;
                case e_elememt_type::BEZIE2_S_R:
                    cur = new VecpBezie2Element({r1.x, r1.y, la.x, la.y, r2.x, r2.y});
                    px = la.x;
                    py = la.y;
                    bx = r2.x;
                    by = r2.y;
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE3_S_R:
                    cur = new VecpBezie3Element({r1.x, r1.y, la.x, la.y, r2.x, r2.y, r3.x, r3.y});
                    px = r2.x;
                    py = r2.y;
                    bx = r3.x;
                    by = r3.y;
                    bb.upd(r2);
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE4_S_R:
                    cur = new VecpBezie4Element({r1.x, r1.y, la.x, la.y, r2.x, r2.y, r3.x, r3.y, r4.x, r4.y});
                    px = r3.x;
                    py = r3.y;
                    bx = r4.x;
                    by = r4.y;
                    bb.upd(r2);
                    bb.upd(r3);
                    bb.upd(la);
                    break;
                case e_elememt_type::ARCPURE:
                    cur = new VecpArcElement({data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3], data[offset + 4], data[offset + 5], data[offset + 6], data[offset + 7]});
                    {
                        double tx, ty;
                        cur->Get_xy_by_t(1, &tx, &ty);
                        bx = tx;
                        by = ty;
                    }
                    break;
                case e_elememt_type::ARCSVG_A:
                case e_elememt_type::ARCSVG_R:
                    // (оставлено без изменений — SVG-дуги не используются для шрифтов,
                    //  а логика совпадает с исходной. При необходимости допишите парсинг.)
                    break;
                case e_elememt_type::ARC2_A:
                case e_elememt_type::ARC2_R:
                case e_elememt_type::ARC3_A:
                case e_elememt_type::ARC3_R:
                case e_elememt_type::ARC2_S_A:
                case e_elememt_type::ARC2_S_R:
                case e_elememt_type::ARC3_S_A:
                case e_elememt_type::ARC3_S_R: {
                    if (*e == e_elememt_type::ARC2_A)
                    {
                        p1 = a1;
                        p2 = a2;
                        p3 = a2;
                        p4 = a3;
                        bb.upd(a2);
                    }
                    else if (*e == e_elememt_type::ARC2_R)
                    {
                        p1 = r1;
                        p2 = r2;
                        p3 = r2;
                        p4 = r3;
                        bb.upd(r2);
                    }
                    else if (*e == e_elememt_type::ARC3_A)
                    {
                        p1 = a1;
                        p2 = a2;
                        p3 = a3;
                        p4 = a4;
                        bb.upd(a2);
                        bb.upd(a3);
                    }
                    else if (*e == e_elememt_type::ARC3_R)
                    {
                        p1 = r1;
                        p2 = r2;
                        p3 = r3;
                        p4 = r4;
                        bb.upd(r2);
                        bb.upd(r3);
                    }
                    else if (*e == e_elememt_type::ARC2_S_A)
                    {
                        p1 = a1;
                        p2 = la;
                        p3 = la;
                        p4 = a2;
                        bb.upd(p2);
                    }
                    else if (*e == e_elememt_type::ARC2_S_R)
                    {
                        p1 = r1;
                        p2 = la;
                        p3 = la;
                        p4 = r2;
                        bb.upd(p2);
                    }
                    else if (*e == e_elememt_type::ARC3_S_A)
                    {
                        p1 = a1;
                        p2 = la;
                        p3 = a2;
                        p4 = a3;
                        bb.upd(p2);
                        bb.upd(a2);
                    }
                    else
                    {
                        p1 = r1;
                        p2 = la;
                        p3 = r2;
                        p4 = r3;
                        bb.upd(p2);
                        bb.upd(r2);
                    }

                    point_t n = p2 - p1;
                    point_t m = p3 - p1;
                    point_t k = p4 - p1;
                    double ln = n.len();
                    point_t Y = n / -ln;
                    point_t X = {Y.y, -Y.x};
                    point_t A{X * m, Y * m};
                    point_t B{X * k, Y * k};
                    point_t D = B - A;

                    double cx = 0, cy = 0, ex = 0, ey = 0, a = 0, b = 0, k0 = 0, k1 = 0, v = 0;

                    if (D.y * D.y > 1e-16)
                    {
                        double K = D.x / D.y;
                        v = B.x * (K * B.y - B.x) / (K * B.y - 2 * B.x);
                        double bk = (v * v * B.y) / ((v - B.x) * K);
                        if (bk <= 0)
                            throw 1;
                        cx = X.x * v + p1.x;
                        cy = X.y * v + p1.y;
                        ex = v > 0 ? -X.x : X.x;
                        ey = v > 0 ? -X.y : X.y;
                        a = v > 0 ? v : -v;
                        b = sqrt(bk);
                    }
                    else if (D.x * D.x > 1e-16)
                    {
                        v = B.x;
                        cx = X.x * v + p1.x;
                        cy = X.y * v + p1.y;
                        ex = v > 0 ? -X.x : X.x;
                        ey = v > 0 ? -X.y : X.y;
                        a = v > 0 ? v : -v;
                        b = B.y > 0 ? B.y : -B.y;
                    }
                    else if (D.x * D.x <= 1e-16 && B.y * B.y <= 1e-16)
                    {
                        v = B.x / 2;
                        cx = X.x * v + p1.x;
                        cy = X.y * v + p1.y;
                        ex = v > 0 ? -X.x : X.x;
                        ey = v > 0 ? -X.y : X.y;
                        a = v > 0 ? v : -v;
                        b = a;
                    }
                    else
                        throw 1;

                    k0 = 0;
                    point_t L{p4.x - cx, p4.y - cy};
                    auto tbal = Y * L;
                    k1 = (tbal < 0) ? -0.5 - (X * L) / (2 * v) : -1.5 + (X * L) / (2 * v);

                    cur = new VecpArcElement({cx, cy, ex, ey, a, b, k0, k1});
                    px = p3.x;
                    py = p3.y;
                    bx = p4.x;
                    by = p4.y;
                }
                break;
            }

            offset += __get_offset(*e);

            if (cur)
            {
                int count = cur->Resample_element(resamples, 16);
                for (int kk = 0; kk < count; ++kk)
                {
                    double X, Y;
                    resamples[kk]->Get_xy_by_t(0, &X, &Y);
                    bb.upd(X, Y);
                    resamples[kk]->Get_xy_by_t(1, &X, &Y);
                    bb.upd(X, Y);
                }
                delete cur;
            }

            bb.upd(bx, by);
        }
        
        const double sloop = subpath.tm.yx / subpath.tm.xx;
        if (sloop < 0)
        {
            bb.min_y = bb.min_y + sloop * bb.max_y;
            bb.max_y = bb.max_y - sloop * bb.max_y;
        }
        if (sloop > 0)
        {
            bb.min_y = bb.min_y - sloop * bb.max_y;
            bb.max_y = bb.max_y + sloop * bb.max_y;
        }

        float __k = __max(c->border_inner, c->border_outer);
        bb.min_y -= __k;
        bb.max_y += __k;
        bb.min_x -= __k;
        bb.max_x += __k;
        bb.min_y *= subpath.tm.yy;
        bb.max_y *= subpath.tm.yy;
        bb.min_x *= subpath.tm.xx;
        bb.max_x *= subpath.tm.xx;

        out_bb.upd(bb.min_x, bb.min_y);
        out_bb.upd(bb.max_x, bb.max_y);
    }

    return out_bb;
}

#define DBG 0
#if DBG
const uint8_t dim = 4u;
typedef uint8_t T;
#else
template<typename T, uint8_t dim>
#endif
void Render_imp(void* _target, const uint32_t& out_height, const uint32_t& out_width, const vector_t& vector, const uint8_t& SUB, const double& tolerance, uint64_t line_size, const int& alpha_id)
{
    constexpr T ZERO = (T(0) - T(0)) * (T(0));
    const double DELTA = 1.0 / double(SUB);
    if (line_size == 0)
        line_size = out_width * dim * sizeof(T);
    T* target = (T*)_target;

    struct __Fill_comparer
    {
        static bool comp(const cross_t& a, const cross_t& b)
        {
            return a.v < b.v;
        }
    };

    struct __filler
    {
        // ---- find_inters_* ----
        static inline int find_inters_h_b(cross_t* cross, const size_t& ssize, const double& tx, const double& scale_w, const std::vector<border_t*>& borders)
        {
            int c_size = 0;
            for (size_t i = 0; i < ssize; ++i)
            {
                auto& el = *borders[i];
                {
                    auto y = cross[c_size] = el.interpol_in.Get_x_by_y(tx) * scale_w;
                    if (y.dir)
                        ++c_size;
                }
                {
                    auto y = cross[c_size] = el.interpol_out.Get_x_by_y(tx) * scale_w;
                    if (y.dir)
                        ++c_size;
                }
                for (int k = 0; k < 20; ++k)
                    if (el.interpol_add[k].c > 0)
                    {
                        auto y = cross[c_size] = el.interpol_add[k].Get_x_by_y(tx) * scale_w;
                        if (y.dir)
                            ++c_size;
                    }
            }
            if (c_size)
                std::sort(cross, cross + c_size, __Fill_comparer::comp);
            if (c_size & 1)
            {
                constexpr auto __m = std::numeric_limits<double>::max();
                double md = __m;
                int si = -1;
                for (int i = 0; i < c_size - 1; ++i)
                {
                    auto d = cross[i].v - cross[i + 1].v;
                    if (d * d < md)
                    {
                        si = i;
                        md = d * d;
                    }
                }
                --c_size;
                if (c_size > 1)
                {
                    cross[si].v = (cross[si].v + cross[si + 1].v) * 0.5;
                    for (int i = si + 1; i < c_size; ++i)
                        cross[i] = cross[i + 1];
                }
            }
            return c_size;
        }

        static inline int find_inters_h(cross_t* cross, const size_t& ssize, const double& tx, const double& scale_w, const std::vector<border_t*>& borders)
        {
            int c_size = 0;
            for (size_t i = 0; i < ssize; ++i)
            {
                auto y = cross[c_size] = borders[i]->interpol.Get_x_by_y(tx) * scale_w;
                if (y.dir)
                    ++c_size;
            }
            if (c_size)
                std::sort(cross, cross + c_size, __Fill_comparer::comp);
            if (c_size & 1)
            {
                constexpr auto __m = std::numeric_limits<double>::max();
                double md = __m;
                int si = -1;
                for (int i = 0; i < c_size - 1; ++i)
                {
                    auto d = cross[i].v - cross[i + 1].v;
                    if (d * d < md)
                    {
                        si = i;
                        md = d * d;
                    }
                }
                --c_size;
                if (c_size > 1)
                {
                    cross[si].v = (cross[si].v + cross[si + 1].v) * 0.5;
                    for (int i = si + 1; i < c_size; ++i)
                        cross[i] = cross[i + 1];
                }
            }
            return c_size;
        }

        static inline int find_inters_w_b(cross_t* cross, const size_t& ssize, const double& ty, const double& scale_h, const std::vector<border_t*>& borders)
        {
            int c_size = 0;
            for (size_t i = 0; i < ssize; ++i)
            {
                auto& el = *borders[i];
                {
                    auto y = cross[c_size] = el.interpol_in.Get_y_by_x(ty) * scale_h;
                    if (y.dir)
                        ++c_size;
                }
                {
                    auto y = cross[c_size] = el.interpol_out.Get_y_by_x(ty) * scale_h;
                    if (y.dir)
                        ++c_size;
                }
                for (int k = 0; k < 20; ++k)
                    if (el.interpol_add[k].c > 0)
                    {
                        auto y = cross[c_size] = el.interpol_add[k].Get_y_by_x(ty) * scale_h;
                        if (y.dir)
                            ++c_size;
                    }
            }
            if (c_size)
                std::sort(cross, cross + c_size, __Fill_comparer::comp);
            if (c_size & 1)
            {
                constexpr auto __m = std::numeric_limits<double>::max();
                double md = __m;
                int si = -1;
                for (int i = 0; i < c_size - 1; ++i)
                {
                    auto d = cross[i].v - cross[i + 1].v;
                    if (d * d < md)
                    {
                        si = i;
                        md = d * d;
                    }
                }
                --c_size;
                if (c_size > 1)
                {
                    cross[si].v = (cross[si].v + cross[si + 1].v) * 0.5;
                    for (int i = si + 1; i < c_size; ++i)
                        cross[i] = cross[i + 1];
                }
            }
            return c_size;
        }

        static inline int find_inters_w(cross_t* cross, const size_t& ssize, const double& ty, const double& scale_h, const std::vector<border_t*>& borders)
        {
            int c_size = 0;
            for (size_t i = 0; i < ssize; ++i)
            {
                auto y = cross[c_size] = borders[i]->interpol.Get_y_by_x(ty) * scale_h;
                if (y.dir)
                    ++c_size;
            }
            if (c_size)
                std::sort(cross, cross + c_size, __Fill_comparer::comp);
            if (c_size & 1)
            {
                constexpr auto __m = std::numeric_limits<double>::max();
                double md = __m;
                int si = -1;
                for (int i = 0; i < c_size - 1; ++i)
                {
                    auto d = cross[i].v - cross[i + 1].v;
                    if (d * d < md)
                    {
                        si = i;
                        md = d * d;
                    }
                }
                --c_size;
                if (c_size > 1)
                {
                    cross[si].v = (cross[si].v + cross[si + 1].v) * 0.5;
                    for (int i = si + 1; i < c_size; ++i)
                        cross[i] = cross[i + 1];
                }
            }
            return c_size;
        }

        static inline void _fill_h(double* out, const int& c_size, const int64_t& h, const int64_t& sh, cross_t* cross, const size_t& ssize, const size_t& width, const size_t& height, const double& sloop_factor_h, const double& static_offset_w, const size_t& SUB, const double& DELTA, const double& OUT_DELTA)
        {
            if (c_size == 0)
                return;
            int _s = (int)cross[0].v - 1;
            int _e = (int)cross[c_size - 1].v + 2;
            int K = 0, counter = 0;
            for (int w = _s; w < _e; ++w)
                for (int sw = 0; sw < SUB; ++sw)
                {
                    double W = w + sw * DELTA + DELTA * 0.5f;
                    while (K != c_size && cross[K].v < W)
                    {
                        counter += cross[K].dir;
                        ++K;
                    }
                    if (counter != 0)
                    {
                        const double offset = int64_t(sloop_factor_h * (W - static_offset_w) * SUB) * DELTA;
                        int _h = int(h + sh * DELTA + DELTA * 0.5f - offset);
                        if (_h >= 0 && _h < height && w >= 0 && w < width)
                            out[width * _h + w] += OUT_DELTA;
                    }
                }
        }

        static inline void _fill_w(double* out, const int& c_size, const int64_t& w, const int64_t& sw, cross_t* cross, const size_t& ssize, const size_t& width, const size_t& height, const double& sloop_factor_h, const double& static_offset_w, const size_t& SUB, const double& DELTA, const double& OUT_DELTA)
        {
            if (c_size == 0)
                return;
            double W = w + sw * DELTA + DELTA * 0.5f;
            const double offset = int64_t(sloop_factor_h * (W - static_offset_w) * SUB) * DELTA;
            int _s = (int)cross[0].v - 1;
            int _e = (int)cross[c_size - 1].v + 2;
            int K = 0, counter = 0;
            for (int h = _s; h < _e; ++h)
                for (int sh = 0; sh < SUB; ++sh)
                {
                    double H = h + sh * DELTA + DELTA * 0.5f;
                    while (K != c_size && cross[K].v < H)
                    {
                        counter += cross[K].dir;
                        ++K;
                    }
                    if (counter != 0)
                    {
                        int _h = int(H - offset);
                        if (_h >= 0 && _h < height && w >= 0 && w < width)
                            out[width * _h + w] += OUT_DELTA;
                    }
                }
        }

        static void fill_fun(double* fiil_coof, const uint32_t& out_height, const uint32_t& out_width, const subpath_t* sp, const vector_t& /*vec*/, int64_t s, int64_t e, uint8_t SUB, bool hor, bool is_border)
        {
            if (sp->tm.yy == 0 || sp->tm.xx == 0 || sp->tm.xy * sp->tm.xy > 1e-20)
                return;

            auto& counter = *sp->base;
            const double offse_inner = counter.border_inner;
            const double offse_outer = counter.border_outer;

            const double scale_w = sp->tm.xx;
            const double scale_h = sp->tm.yy;
            const double sloop = sp->tm.yx / sp->tm.xx;
            const double soff = sp->tm.y;

            const double sf_h = 1.0 / scale_h;
            const double sf_w = 1.0 / scale_w;
            const double DELTA = 1.0 / double(SUB);
            const double OUT_DELTA = DELTA * DELTA * 0.5;

            const size_t ssize = sp->splited.size();
            auto& borders = sp->borders;

            cross_t* cross = new cross_t[ssize * 10];

            if (hor)
            {
                if (is_border)
                    for (int64_t h = s; h < e; ++h)
                        for (int64_t sh = 0; sh < SUB; ++sh)
                            _fill_h(fiil_coof, find_inters_h_b(cross, ssize, (h + sh * DELTA + DELTA * 0.5) * sf_h, scale_w, borders), h, sh, cross, ssize, out_width, out_height, sloop, soff, SUB, DELTA, OUT_DELTA);
                else
                    for (int64_t h = s; h < e; ++h)
                        for (int64_t sh = 0; sh < SUB; ++sh)
                            _fill_h(fiil_coof, find_inters_h(cross, ssize, (h + sh * DELTA + DELTA * 0.5) * sf_h, scale_w, borders), h, sh, cross, ssize, out_width, out_height, sloop, soff, SUB, DELTA, OUT_DELTA);
            }
            else
            {
                if (is_border)
                    for (int64_t w = s; w < e; ++w)
                        for (int64_t sw = 0; sw < SUB; ++sw)
                            _fill_w(fiil_coof, find_inters_w_b(cross, ssize, (w + sw * DELTA + DELTA * 0.5) * sf_w, scale_h, borders), w, sw, cross, ssize, out_width, out_height, sloop, soff, SUB, DELTA, OUT_DELTA);
                else
                    for (int64_t w = s; w < e; ++w)
                        for (int64_t sw = 0; sw < SUB; ++sw)
                            _fill_w(fiil_coof, find_inters_w(cross, ssize, (w + sw * DELTA + DELTA * 0.5) * sf_w, scale_h, borders), w, sw, cross, ssize, out_width, out_height, sloop, soff, SUB, DELTA, OUT_DELTA);
            }

            delete[] cross;
        }
    };

    point_t p1, p2, p3, p4;
    point_t a1, a2, a3, a4, a5;
    point_t r1, r2, r3, r4, r5;
    point_t la;

    VecpElement_ptr resamples[16];

    double* fiil_coof = new double[size_t(out_height) * out_width];
    T color[4];

    for (auto c = vector.counters; c < vector.counters + vector.counters_size; ++c)
    {
        auto data = c->data;
        subpath_t subpath;
        subpath.base = c;
        uint32_t offset = 0;
        uint32_t counterid = 0;
        subpath.counters.push_back({});

        bool newline = true;
        bool next_newline = true;

        bb_t bb;

        auto tm = transform_matrix::mult(*((transform_matrix*)(&vector.t_xx)), *((transform_matrix*)(&c->t_xx)));
        const double brot = atan2(tm.xy, tm.xx);
        const double co = cos(brot);
        const double si = sin(brot);

        subpath.tm = {
          tm.xx * co + tm.xy * si,
          tm.xy * co - tm.xx * si,
          tm.x,
          tm.yx * co + tm.yy * si,
          tm.yy * co - tm.yx * si,
          tm.y,
        };

        double offx = tm.x / subpath.tm.xx;
        double offy = tm.y / subpath.tm.yy;

        double ox = offx, oy = offy, bx = offx, by = offy, px = 0, py = 0;
        bool closed = true;
        VecpElement_ptr last = nullptr;

        const double TOL_BASE = tolerance / ((subpath.tm.xx > subpath.tm.yy) ? subpath.tm.xx : subpath.tm.yy) * (1.0 + DELTA) * 0.5;

        for (auto e = c->elements; e < c->elements + c->elements_size; ++e)
        {
            if (next_newline)
            {
                newline = true;
                next_newline = false;
            }
            VecpElement_ptr cur = nullptr;
            if (offset >= c->data_size && *e != vecp::e_elememt_type::CLOSE)
                break;

            a1 = {bx, by};
            if (offset + 1 < c->data_size)
                a2 = {data[offset + 0] * co - data[offset + 1] * si, data[offset + 1] * co + data[offset + 0] * si};
            else
                a2 = {0.0, 0.0};
            if (offset + 3 < c->data_size)
                a3 = {data[offset + 2] * co - data[offset + 3] * si, data[offset + 3] * co + data[offset + 2] * si};
            else
                a3 = {0.0, 0.0};
            if (offset + 5 < c->data_size)
                a4 = {data[offset + 4] * co - data[offset + 5] * si, data[offset + 5] * co + data[offset + 4] * si};
            else
                a4 = {0.0, 0.0};
            if (offset + 7 < c->data_size)
                a5 = {data[offset + 6] * co - data[offset + 7] * si, data[offset + 7] * co + data[offset + 6] * si};
            else
                a5 = {0.0, 0.0};

            double d;
            r1 = {bx, by};
            r2 = r1 + a2;
            r3 = r1 + a3;
            r4 = r1 + a4;
            r5 = r1 + a5;

            a2.x += offx;
            a3.x += offx;
            a4.x += offx;
            a5.x += offx;
            a2.y += offy;
            a3.y += offy;
            a4.y += offy;
            a5.y += offy;

            auto snap = [&](point_t& p) {
                d = (p.x - ox) * (p.x - ox) + (p.y - oy) * (p.y - oy);
                if (d < 1e-10)
                {
                    p.x = ox;
                    p.y = oy;
                }
            };
            snap(a1);
            snap(a2);
            snap(a3);
            snap(a4);
            snap(a5);
            snap(r1);
            snap(r2);
            snap(r3);
            snap(r4);
            snap(r5);

            la = {bx * 2 - px, by * 2 - py};

            using namespace vecp;

            switch (*e)
            {
                default:
                    break;

                case e_elememt_type::MOVE_A:
                    closed = closed && (bx == ox && by == oy);
                    px = a1.x;
                    py = a1.y;
                    bx = a2.x;
                    by = a2.y;
                    ox = bx;
                    oy = by;
                    newline = true;
                    break;
                case e_elememt_type::MOVE_R:
                    closed = closed && (bx == ox && by == oy);
                    px = r1.x;
                    py = r1.y;
                    bx = r2.x;
                    by = r2.y;
                    ox = bx;
                    oy = by;
                    newline = true;
                    break;
                case e_elememt_type::CLOSE:
                    if (!(bx == ox && by == oy))
                        cur = new VecpLineElement({bx, by, ox, oy});
                    bx = ox;
                    by = oy;
                    next_newline = true;
                    break;
                case e_elememt_type::CLOSE_MOVE_A:
                    if (!(bx == ox && by == oy))
                        cur = new VecpLineElement({bx, by, ox, oy});
                    px = a1.x;
                    py = a1.y;
                    bx = a2.x;
                    by = a2.y;
                    ox = bx;
                    oy = by;
                    next_newline = true;
                    break;
                case e_elememt_type::CLOSE_MOVE_R:
                    if (!(bx == ox && by == oy))
                        cur = new VecpLineElement({bx, by, ox, oy});
                    px = r1.x;
                    py = r1.y;
                    bx = r2.x;
                    by = r2.y;
                    ox = bx;
                    oy = by;
                    next_newline = true;
                    break;
                case e_elememt_type::LINE_A:
                    cur = new VecpLineElement({a1.x, a1.y, a2.x, a2.y});
                    px = a1.x;
                    py = a1.y;
                    bx = a2.x;
                    by = a2.y;
                    break;
                case e_elememt_type::LINE_H_A: {
                    point_t _t{data[offset + 0] * co - (by - offy) * si + offx, (by - offy) * co + data[offset + 0] * si + offy};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::LINE_W_A: {
                    point_t _t{(bx - offx) * co - data[offset + 0] * si + offx, data[offset + 0] * co + (bx - offx) * si + offy};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::BEZIE2_A:
                    cur = new VecpBezie2Element({a1.x, a1.y, a2.x, a2.y, a3.x, a3.y});
                    px = a2.x;
                    py = a2.y;
                    bx = a3.x;
                    by = a3.y;
                    bb.upd(a2);
                    break;
                case e_elememt_type::BEZIE3_A:
                    cur = new VecpBezie3Element({a1.x, a1.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y});
                    px = a3.x;
                    py = a3.y;
                    bx = a4.x;
                    by = a4.y;
                    bb.upd(a2);
                    bb.upd(a3);
                    break;
                case e_elememt_type::BEZIE4_A:
                    cur = new VecpBezie4Element({a1.x, a1.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y, a5.x, a5.y});
                    px = a4.x;
                    py = a4.y;
                    bx = a5.x;
                    by = a5.y;
                    bb.upd(a2);
                    bb.upd(a3);
                    bb.upd(a4);
                    break;
                case e_elememt_type::BEZIE2_S_A:
                    cur = new VecpBezie2Element({a1.x, a1.y, la.x, la.y, a2.x, a2.y});
                    px = la.x;
                    py = la.y;
                    bx = a2.x;
                    by = a2.y;
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE3_S_A:
                    cur = new VecpBezie3Element({a1.x, a1.y, la.x, la.y, a2.x, a2.y, a3.x, a3.y});
                    px = a2.x;
                    py = a2.y;
                    bx = a3.x;
                    by = a3.y;
                    bb.upd(a2);
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE4_S_A:
                    cur = new VecpBezie4Element({a1.x, a1.y, la.x, la.y, a2.x, a2.y, a3.x, a3.y, a4.x, a4.y});
                    px = a3.x;
                    py = a3.y;
                    bx = a4.x;
                    by = a4.y;
                    bb.upd(a2);
                    bb.upd(a3);
                    bb.upd(la);
                    break;
                case e_elememt_type::LINE_R:
                    cur = new VecpLineElement({r1.x, r1.y, r2.x, r2.y});
                    px = r1.x;
                    py = r1.y;
                    bx = r2.x;
                    by = r2.y;
                    break;
                case e_elememt_type::LINE_H_R: {
                    point_t _t{data[offset + 0] * co + bx, data[offset + 0] * si + by};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::LINE_W_R: {
                    point_t _t{-data[offset + 0] * si + bx, data[offset + 0] * co + by};
                    cur = new VecpLineElement({a1.x, a1.y, _t.x, _t.y});
                    px = a1.x;
                    py = a1.y;
                    bx = _t.x;
                    by = _t.y;
                }
                break;
                case e_elememt_type::BEZIE2_R:
                    cur = new VecpBezie2Element({r1.x, r1.y, r2.x, r2.y, r3.x, r3.y});
                    px = r2.x;
                    py = r2.y;
                    bx = r3.x;
                    by = r3.y;
                    bb.upd(r2);
                    break;
                case e_elememt_type::BEZIE3_R:
                    cur = new VecpBezie3Element({r1.x, r1.y, r2.x, r2.y, r3.x, r3.y, r4.x, r4.y});
                    px = r3.x;
                    py = r3.y;
                    bx = r4.x;
                    by = r4.y;
                    bb.upd(r2);
                    bb.upd(r3);
                    break;
                case e_elememt_type::BEZIE4_R:
                    cur = new VecpBezie4Element({r1.x, r1.y, r2.x, r2.y, r3.x, r3.y, r4.x, r4.y, r5.x, r5.y});
                    px = r4.x;
                    py = r4.y;
                    bx = r5.x;
                    by = r5.y;
                    bb.upd(r2);
                    bb.upd(r3);
                    bb.upd(r4);
                    break;
                case e_elememt_type::BEZIE2_S_R:
                    cur = new VecpBezie2Element({r1.x, r1.y, la.x, la.y, r2.x, r2.y});
                    px = la.x;
                    py = la.y;
                    bx = r2.x;
                    by = r2.y;
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE3_S_R:
                    cur = new VecpBezie3Element({r1.x, r1.y, la.x, la.y, r2.x, r2.y, r3.x, r3.y});
                    px = r2.x;
                    py = r2.y;
                    bx = r3.x;
                    by = r3.y;
                    bb.upd(r2);
                    bb.upd(la);
                    break;
                case e_elememt_type::BEZIE4_S_R:
                    cur = new VecpBezie4Element({r1.x, r1.y, la.x, la.y, r2.x, r2.y, r3.x, r3.y, r4.x, r4.y});
                    px = r3.x;
                    py = r3.y;
                    bx = r4.x;
                    by = r4.y;
                    bb.upd(r2);
                    bb.upd(r3);
                    bb.upd(la);
                    break;
                case e_elememt_type::ARCPURE:
                    cur = new VecpArcElement({data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3], data[offset + 4], data[offset + 5], data[offset + 6], data[offset + 7]});
                    {
                        double tx, ty;
                        cur->Get_xy_by_t(1, &tx, &ty);
                        bx = tx;
                        by = ty;
                    }
                    break;
                case e_elememt_type::ARCSVG_A:
                case e_elememt_type::ARCSVG_R:
                    // (оставлено без изменений — SVG-дуги не используются для шрифтов,
                    //  а логика совпадает с исходной. При необходимости допишите парсинг.)
                    break;
                case e_elememt_type::ARC2_A:
                case e_elememt_type::ARC2_R:
                case e_elememt_type::ARC3_A:
                case e_elememt_type::ARC3_R:
                case e_elememt_type::ARC2_S_A:
                case e_elememt_type::ARC2_S_R:
                case e_elememt_type::ARC3_S_A:
                case e_elememt_type::ARC3_S_R: {
                    if (*e == e_elememt_type::ARC2_A)
                    {
                        p1 = a1;
                        p2 = a2;
                        p3 = a2;
                        p4 = a3;
                        bb.upd(a2);
                    }
                    else if (*e == e_elememt_type::ARC2_R)
                    {
                        p1 = r1;
                        p2 = r2;
                        p3 = r2;
                        p4 = r3;
                        bb.upd(r2);
                    }
                    else if (*e == e_elememt_type::ARC3_A)
                    {
                        p1 = a1;
                        p2 = a2;
                        p3 = a3;
                        p4 = a4;
                        bb.upd(a2);
                        bb.upd(a3);
                    }
                    else if (*e == e_elememt_type::ARC3_R)
                    {
                        p1 = r1;
                        p2 = r2;
                        p3 = r3;
                        p4 = r4;
                        bb.upd(r2);
                        bb.upd(r3);
                    }
                    else if (*e == e_elememt_type::ARC2_S_A)
                    {
                        p1 = a1;
                        p2 = la;
                        p3 = la;
                        p4 = a2;
                        bb.upd(p2);
                    }
                    else if (*e == e_elememt_type::ARC2_S_R)
                    {
                        p1 = r1;
                        p2 = la;
                        p3 = la;
                        p4 = r2;
                        bb.upd(p2);
                    }
                    else if (*e == e_elememt_type::ARC3_S_A)
                    {
                        p1 = a1;
                        p2 = la;
                        p3 = a2;
                        p4 = a3;
                        bb.upd(p2);
                        bb.upd(a2);
                    }
                    else
                    {
                        p1 = r1;
                        p2 = la;
                        p3 = r2;
                        p4 = r3;
                        bb.upd(p2);
                        bb.upd(r2);
                    }

                    point_t n = p2 - p1;
                    point_t m = p3 - p1;
                    point_t k = p4 - p1;
                    double ln = n.len();
                    point_t Y = n / -ln;
                    point_t X = {Y.y, -Y.x};
                    point_t A{X * m, Y * m};
                    point_t B{X * k, Y * k};
                    point_t D = B - A;

                    double cx = 0, cy = 0, ex = 0, ey = 0, a = 0, b = 0, k0 = 0, k1 = 0, v = 0;

                    if (D.y * D.y > 1e-16)
                    {
                        double K = D.x / D.y;
                        v = B.x * (K * B.y - B.x) / (K * B.y - 2 * B.x);
                        double bk = (v * v * B.y) / ((v - B.x) * K);
                        if (bk <= 0)
                            throw 1;
                        cx = X.x * v + p1.x;
                        cy = X.y * v + p1.y;
                        ex = v > 0 ? -X.x : X.x;
                        ey = v > 0 ? -X.y : X.y;
                        a = v > 0 ? v : -v;
                        b = sqrt(bk);
                    }
                    else if (D.x * D.x > 1e-16)
                    {
                        v = B.x;
                        cx = X.x * v + p1.x;
                        cy = X.y * v + p1.y;
                        ex = v > 0 ? -X.x : X.x;
                        ey = v > 0 ? -X.y : X.y;
                        a = v > 0 ? v : -v;
                        b = B.y > 0 ? B.y : -B.y;
                    }
                    else if (D.x * D.x <= 1e-16 && B.y * B.y <= 1e-16)
                    {
                        v = B.x / 2;
                        cx = X.x * v + p1.x;
                        cy = X.y * v + p1.y;
                        ex = v > 0 ? -X.x : X.x;
                        ey = v > 0 ? -X.y : X.y;
                        a = v > 0 ? v : -v;
                        b = a;
                    }
                    else
                        throw 1;

                    k0 = 0;
                    point_t L{p4.x - cx, p4.y - cy};
                    auto tbal = Y * L;
                    k1 = (tbal < 0) ? -0.5 - (X * L) / (2 * v) : -1.5 + (X * L) / (2 * v);

                    cur = new VecpArcElement({cx, cy, ex, ey, a, b, k0, k1});
                    px = p3.x;
                    py = p3.y;
                    bx = p4.x;
                    by = p4.y;
                }
                break;
            }

            offset += __get_offset(*e);

            if (cur)
            {
                int count = cur->Resample_element(resamples, 16);
                for (int kk = 0; kk < count; ++kk)
                {
                    subpath.borders.push_back(new border_t());
                    border_t* back = newline ? nullptr : subpath.borders[subpath.borders.size() - 2];
                    border_t* bor = subpath.borders.back();
                    bor->base_element = resamples[kk];
                    if (!newline)
                    {
                        back->post_border = bor;
                        bor->pre_border = back;
                    }
                    else
                    {
                        if (subpath.counters[counterid].size() > 0)
                        {
                            counterid++;
                            subpath.counters.push_back({});
                        }
                        bor->pre_border = nullptr;
                    }
                    bor->base_path = &subpath;
                    bor->counterid = counterid;
                    bor->incounterid = (int)subpath.counters[counterid].size();
                    bor->interpol.make(bor->base_element, TOL_BASE, cur);
                    subpath.splited.push_back(resamples[kk]);
                    subpath.counters[counterid].push_back(bor);
                    newline = false;
                    last = resamples[kk];
                }
                delete cur;
            }

            bb.upd(bx, by);
        }

        subpath.status = (closed && bx == ox && by == oy) ? 1 : 0;

        // ---- border generation ----
        int s = (int)subpath.borders.size();
        if (c->border_inner > 0 && c->border_outer > 0)
        {
            for (int j = 0; j < s; ++j)
            {
                if ((subpath.status & 0b1) > 0)
                {
                    if (subpath.borders[j]->pre_border == nullptr)
                    {
                        subpath.borders[j]->pre_border = subpath.counters[subpath.borders[j]->counterid].back();
                        subpath.borders[j]->pre_border->post_border = subpath.borders[j];
                    }
                }
            }
            for (int j = 0; j < s; ++j)
                check_border(subpath.borders[j], c->border_inner, c->border_outer, TOL_BASE);
            for (int j = 0; j < s; ++j)
                generate_border(subpath.borders[j], c->border_inner, c->border_outer, c->corner_type, TOL_BASE);
        }

        // ---- bbox adjustments ----
        const double sloop = subpath.tm.yx / subpath.tm.xx;
        if (sloop < 0)
        {
            bb.min_y = bb.min_y + sloop * bb.max_y;
            bb.max_y = bb.max_y - sloop * bb.max_y;
        }
        if (sloop > 0)
        {
            bb.min_y = bb.min_y - sloop * bb.max_y;
            bb.max_y = bb.max_y + sloop * bb.max_y;
        }

        float __k = __max(c->border_inner, c->border_outer);
        bb.min_y -= __k;
        bb.max_y += __k;
        bb.min_x -= __k;
        bb.max_x += __k;
        bb.min_y *= subpath.tm.yy;
        bb.max_y *= subpath.tm.yy;
        bb.min_x *= subpath.tm.xx;
        bb.max_x *= subpath.tm.xx;
        bb.min_y -= 50;
        bb.max_y += 50;
        bb.min_x -= 50;
        bb.max_x += 50;

        // ---- fill ----
        double tcolor[4];
        bool has_alpha = alpha_id >= 0 && alpha_id < dim;
        const auto& _transp = c->transparent;

        fill(tcolor, &c->background_color);
        double alfa = has_alpha ? _transp * tcolor[alpha_id] : _transp;

        if (alfa > 0)
        {
            for (int i = 0; i < out_height * out_width; ++i)
                fiil_coof[i] = 0;
            __filler::fill_fun(fiil_coof, out_height, out_width, &subpath, vector, (int64_t)bb.min_y, (int64_t)bb.max_y, SUB, true, false);
            __filler::fill_fun(fiil_coof, out_height, out_width, &subpath, vector, (int64_t)bb.min_x, (int64_t)bb.max_x, SUB, false, false);

            fill(color, &c->background_color);
            for (int h = 0; h < (int)out_height; ++h)
            {
                T* line = target + out_width * h * dim;
                for (int w = 0; w < (int)out_width; ++w)
                {
                    double coof = fiil_coof[out_width * h + w];
                    if (coof > 1)
                        coof = 1;
                    T* cur = line + w * dim;
                    if (coof)
                    {
                        coof *= alfa;
                        if (coof == 1)
                            for (int d = 0; d < dim; ++d)
                                cur[d] = color[d];
                        else
                        {
                            T curalfha = has_alpha ? cur[alpha_id] : 0;
                            T colalfha = has_alpha ? color[alpha_id] : 0;
                            for (int d = 0; d < dim; ++d)
                            {
                                if (!has_alpha || d == alpha_id)
                                    cur[d] = cur[d] + (color[d] - cur[d]) * coof;
                                else if (has_alpha && curalfha != ZERO)
                                    cur[d] = (cur[d] * curalfha * (1 - coof) + color[d] * colalfha * coof) / (curalfha * (1 - coof) + colalfha * coof);
                                else
                                    cur[d] = color[d];
                            }
                        }
                    }
                }
            }
        }

        // ---- border ----
        const auto& _btransp = c->border_transparent;
        fill(tcolor, &c->border_color);
        alfa = has_alpha ? _btransp * tcolor[alpha_id] : _btransp;

        if ((c->border_inner != 0 || c->border_outer != 0) && alfa > 0)
        {
            for (int i = 0; i < out_height * out_width; ++i)
                fiil_coof[i] = 0;
            __filler::fill_fun(fiil_coof, out_height, out_width, &subpath, vector, (int64_t)bb.min_y, (int64_t)bb.max_y, SUB, true, true);
            __filler::fill_fun(fiil_coof, out_height, out_width, &subpath, vector, (int64_t)bb.min_x, (int64_t)bb.max_x, SUB, false, true);

            fill(color, &c->border_color);
            for (int h = 0; h < (int)out_height; ++h)
            {
                T* line = target + out_width * h * dim;
                for (int w = 0; w < (int)out_width; ++w)
                {
                    double coof = fiil_coof[out_width * h + w];
                    if (coof > 1)
                        coof = 1;
                    T* cur = line + w * dim;
                    if (coof)
                    {
                        coof *= alfa;
                        if (coof == 1)
                            for (int d = 0; d < dim; ++d)
                                cur[d] = color[d];
                        else
                        {
                            T curalfha = has_alpha ? cur[alpha_id] : 0;
                            T colalfha = has_alpha ? color[alpha_id] : 0;
                            for (int d = 0; d < dim; ++d)
                            {
                                if (!has_alpha || d == alpha_id)
                                    cur[d] = cur[d] + (color[d] - cur[d]) * coof;
                                else if (has_alpha && curalfha != ZERO)
                                    cur[d] = (cur[d] * curalfha * (1 - coof) + color[d] * colalfha * coof) / (curalfha * (1 - coof) + colalfha * coof);
                                else
                                    cur[d] = color[d];
                            }
                        }
                    }
                }
            }
        }
    }

    delete[] fiil_coof;
}

// =====================================================================
// Render (dispatch по формату)
// =====================================================================
void Render(void* target, uint32_t out_height, uint32_t out_width, e_target_out_type target_type, const vector_t& vector, uint8_t samples, int quality, uint64_t line_size, int alpha_id)
{
    double tolerance = (quality >= 0) ? 0.25 / (1.0 + quality * 0.1) : 0.25 * (1.0 - quality * 0.1);
    switch (target_type)
    {
#if DBG
        case e_target_out_type::RGBA_8:
            Render_imp(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
#else
        case e_target_out_type::RED_8:
            Render_imp<uint8_t, 1>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RED_16:
            Render_imp<uint16_t, 1>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RED_F:
            Render_imp<float, 1>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RED_D:
            Render_imp<double, 1>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RG_8:
            Render_imp<uint8_t, 2>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RG_16:
            Render_imp<uint16_t, 2>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RG_F:
            Render_imp<float, 2>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RG_D:
            Render_imp<double, 2>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGB_8:
            Render_imp<uint8_t, 3>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGB_16:
            Render_imp<uint16_t, 3>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGB_F:
            Render_imp<float, 3>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGB_D:
            Render_imp<double, 3>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGBA_8:
            Render_imp<uint8_t, 4>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGBA_16:
            Render_imp<uint16_t, 4>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGBA_F:
            Render_imp<float, 4>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
        case e_target_out_type::RGBA_D:
            Render_imp<double, 4>(target, out_height, out_width, vector, samples, tolerance, line_size, alpha_id);
            break;
#endif
        default:
            break;
    }
}

#undef DBG

// ---- Font loading (сохранено из старого файла) ----

inline bool _read(const uint8_t* base, size_t total_size, size_t& off, void* dst, size_t n)
{
    if (off + n > total_size)
        return false;
    memcpy(dst, base + off, n);
    off += n;
    return true;
}

static bool LoadCounterBinary(counter_t& cnt,
                              const uint8_t* base,
                              size_t total_size,
                              size_t& off)
{
    counter_t_data hdr{};
    if (!_read(base, total_size, off, &hdr, sizeof(counter_t_data)))
        return false;

    static_cast<counter_t_data&>(cnt) = hdr;

    if (hdr.metadata_size)
    {
        cnt.metadata = new uint8_t[hdr.metadata_size];
        if (!_read(base, total_size, off, cnt.metadata, hdr.metadata_size))
            return false;
        cnt.metadata_allocated = hdr.metadata_size;
    }
    if (hdr.properties_size)
    {
        cnt.properties = new uint8_t[hdr.properties_size];
        if (!_read(base, total_size, off, cnt.properties, hdr.properties_size))
            return false;
        cnt.properties_allocated = hdr.properties_size;
    }
    if (hdr.elements_size)
    {
        cnt.elements = new e_elememt_type[hdr.elements_size];
        if (!_read(base, total_size, off, cnt.elements,
                   (size_t)hdr.elements_size * sizeof(e_elememt_type)))
            return false;
        cnt.elements_allocated = hdr.elements_size;
    }
    if (hdr.data_size)
    {
        cnt.data = new double[hdr.data_size];
        if (!_read(base, total_size, off, cnt.data,
                   (size_t)hdr.data_size * sizeof(double)))
            return false;
        cnt.data_allocated = hdr.data_size;
    }
    return true;
}

bool LoadGlyfBinary(glyf_t& g,
                    const uint8_t* base,
                    size_t total_size,
                    size_t& off)
{
    glyf_t_data hdr{};
    if (!_read(base, total_size, off, &hdr, sizeof(glyf_t_data)))
        return false;

    if (off + hdr.glyf_size > total_size)
        return false;

    g.Codepoint     = hdr.Codepoint;
    g.advance_width = hdr.advance_width;
    g.lsb           = hdr.lsb;
    g.flags         = hdr.flags;

    // Синтезируем vector_t на один contour с единичной матрицей.
    g.glyf.f_type = 0x564C5045;
    g.glyf.format = 0;
    g.glyf.ts     = 0;
    g.glyf.size   = hdr.glyf_size;
    g.glyf.type   = 0;
    g.glyf.width  = 0;
    g.glyf.heigt  = 0;
    g.glyf.t_xx = 1; g.glyf.t_xy = 0; g.glyf.t_x = 0;
    g.glyf.t_yx = 0; g.glyf.t_yy = 1; g.glyf.t_y = 0;
    g.glyf.metadata_size   = 0;
    g.glyf.properties_size = 0;
    g.glyf.counters_size   = 1;

    g.glyf.counters           = new counter_t[1];
    g.glyf.counters_allocated = 1;

    if (!LoadCounterBinary(g.glyf.counters[0], base, total_size, off))
        return false;

    return true;
}


bool LoadFontBinary(font_t& font, const void* data, size_t size)
{
    const uint8_t* base = (const uint8_t*)data;
    size_t off = 0;
    if (!_read(base, size, off, static_cast<font_t_data*>(&font), sizeof(font_t_data)))
        return false;
    if (font.f_type != 0x57464E30)
        return false;
    if (font.size > size)
        return false;

    if (font.metadata_size)
    {
        font.metadata = new uint8_t[font.metadata_size];
        if (!_read(base, size, off, font.metadata, font.metadata_size))
            return false;
        font.metadata_allocated = font.metadata_size;
    }
    if (font.properties_size)
    {
        font.properties = new uint8_t[font.properties_size];
        if (!_read(base, size, off, font.properties, font.properties_size))
            return false;
        font.properties_allocated = font.properties_size;
    }
    if (font.glyfs_size)
    {
        font.glyfs = new glyf_t[font.glyfs_size];
        font.glyfs_allocated = font.glyfs_size;
        for (uint32_t i = 0; i < font.glyfs_size; ++i)
            if (!LoadGlyfBinary(font.glyfs[i], base, size, off))
                return false;
    }
    return true;
}

void FillRRect(unsigned int* data, unsigned int color, int x, int y, int w, int h, int r, int line_size, unsigned char v, int sub)
{
    float delta = 1.0 / float(sub);
    float delta2 = delta * delta;
    if (r * 2 > w)
        r = w / 2;
    unsigned char cr = (color >> 0) & 0xff;
    unsigned char cg = (color >> 8) & 0xff;
    unsigned char cb = (color >> 16) & 0xff;
    unsigned char ca = (color >> 24) & 0xff;
    unsigned int ocolor;
    unsigned char ocr;
    unsigned char ocg;
    unsigned char ocb;
    unsigned char oca;
    float r2 = r * r;

    if (1)
    {
#define GC(X)                    \
    ocolor = X;                  \
    ocr = (ocolor >> 0) & 0xff;  \
    ocg = (ocolor >> 8) & 0xff;  \
    ocb = (ocolor >> 16) & 0xff; \
    oca = (ocolor >> 24) & 0xff;

        /*
        if(!has_alpha || d == alpha_id)
        {
            cur[d] = cur[d] + (color[d] - cur[d]) * coof;
        }
        else if(has_alpha && curalfha != ZERO)
        {
            cur[d] = (cur[d] * curalfha * (1 - coof) + color[d] * colalfha * coof) / (curalfha * (1 - coof) + colalfha * coof);
        }
        */

#define FL      \
    GC(row[j]); \
    row[j] = ((unsigned char)((ocr * oca + (cr * ca - ocr * oca) * k) / (oca + (ca - oca) * k)) << 0) | ((unsigned char)((ocg * oca + (cg * ca - ocg * oca) * k) / (oca + (ca - oca) * k)) << 8) | ((unsigned char)((ocb * oca + (cb * ca - ocb * oca) * k) / (oca + (ca - oca) * k)) << 16) | ((unsigned char)(oca + (ca - oca) * k) << 24);

#define FR              \
    GC(row[w - j - 1]); \
    row[w - j - 1] = ((unsigned char)((ocr * oca + (cr * ca - ocr * oca) * k) / (oca + (ca - oca) * k)) << 0) | ((unsigned char)((ocg * oca + (cg * ca - ocg * oca) * k) / (oca + (ca - oca) * k)) << 8) | ((unsigned char)((ocb * oca + (cb * ca - ocb * oca) * k) / (oca + (ca - oca) * k)) << 16) | ((unsigned char)(oca + (ca - oca) * k) << 24);


#define STEP_1                            \
    int cx = r;                           \
    int cy = r;                           \
    int ci = i;                           \
    int v1 = (i + 1 - cy) * (i + 1 - cy); \
    int v2 = (i - cy) * (i - cy);         \
    int s1 = r - sqrtf(r2 - v1) - 1;      \
    s1 = (s1 > 0) * s1;                   \
    int s2 = r - sqrtf(r2 - v2) + 1;      \
    int e1 = w - s2;                      \
    int e2 = w - s1;

#define STEP_2                                        \
    float k = 0;                                      \
    float dy = 0;                                     \
    float dx = 0;                                     \
    for (int t1 = 0; t1 < sub; ++t1)                  \
        for (int t2 = 0; t2 < sub; ++t2)              \
        {                                             \
            dx = j + delta * 0.5 + t1 * delta - cx;   \
            dy = i + delta * 0.5 + t2 * delta - cy;   \
            k += delta2 * ((dx * dx + dy * dy) < r2); \
        }                                             \
    float ik = 1.0 - k;
    }

    int i = 0;
    int j = 0;
    // lt, rt, rb, lb
    if ((v & 0b1100) == 0b1100)
    {
        for (i = 0; i < r; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            STEP_1;
            for (j = s1; j < s2; j++)
            {
                STEP_2;
                FL;
                FR;
            }
            for (j = s2; j < e1; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b1100) == 0b0000)
    {
        for (i = 0; i < r; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            for (j = 0; j < w; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b1100) == 0b1000)
    {
        for (i = 0; i < r; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            STEP_1;
            for (j = s1; j < s2; j++)
            {
                STEP_2;
                FL;
            }
            for (j = s2; j < w; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b1100) == 0b0100)
    {
        for (i = 0; i < r; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            STEP_1;
            for (j = s1; j < s2; j++)
            {
                STEP_2;
                FR;
            }
            for (j = 0; j < e1; j++)
            {
                row[j] = color;
            }
        }
    }

    for (; i < h - r; ++i)
    {
        unsigned int* row = data + ((i + y) * line_size) + x;
        for (j = 0; j < w; j++)
        {
            row[j] = color;
        }
    }

    if (1)
    {
#undef STEP_1
#define STEP_1                            \
    int cx = r;                           \
    int cy = h - r;                       \
    int ci = h - i - 1;                   \
    int v1 = (i - cy) * (i - cy);         \
    int v2 = (i + 1 - cy) * (i + 1 - cy); \
    int s1 = r - sqrtf(r2 - v1) - 1;      \
    s1 = (s1 > 0) * s1;                   \
    int s2 = r - sqrtf(r2 - v2) + 1;      \
    int e1 = w - s2;                      \
    int e2 = w - s1;
    }

    if ((v & 0b11) == 0b11)
    {
        for (; i < h; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            STEP_1;
            for (j = s1; j < s2; j++)
            {
                STEP_2;
                FL;
                FR;
            }
            for (j = s2; j < e1; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b11) == 0b00)
    {
        for (i = 0; i < h; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            for (j = 0; j < w; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b11) == 0b01)
    {
        for (; i < h; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            STEP_1;
            for (j = s1; j < s2; j++)
            {
                STEP_2;
                FL;
            }
            for (j = s2; j < w; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b11) == 0b10)
    {
        for (; i < h; ++i)
        {
            unsigned int* row = data + ((i + y) * line_size) + x;
            STEP_1(r, h - r);
            for (j = s1; j < s2; j++)
            {
                STEP_2;
                FR;
            }
            for (j = 0; j < e1; j++)
            {
                row[j] = color;
            }
        }
    }

#undef GC
#undef FL
#undef FR
#undef STEP_1
#undef STEP_2_IMP
#undef STEP_2
}

void PaintRRect(unsigned int* data, unsigned int color, int _x, int _y, int w, int h, int r, int in, int out, int line_size, unsigned char v, int sub)
{
    float delta = 1.0 / float(sub);
    float delta2 = delta * delta;

    if (-in == out)
        return;

    if (-in > out)
    {
        float t = -in;
        in = -out;
        out = t;
    }

    if (r * 2 > w)
        r = w / 2;

    int ro = r + out;
    if (ro < 0)
        return;
    int xo = _x - out;
    int yo = _y - out;
    int wo = w + out * 2;
    int ho = h + out * 2;

    int ri = r - in;
    if (ri < 0)
    {
        ri = 0;
        in = r;
    }
    int xi = _x + in;
    int yi = _y + in;
    int wi = w - in * 2;
    int hi = h - in * 2;

    unsigned char cr = (color >> 0) & 0xff;
    unsigned char cg = (color >> 8) & 0xff;
    unsigned char cb = (color >> 16) & 0xff;
    unsigned char ca = (color >> 24) & 0xff;
    unsigned int ocolor;
    unsigned char ocr;
    unsigned char ocg;
    unsigned char ocb;
    unsigned char oca;

    float r2 = r * r;
    float ri2 = ri * ri;
    float ro2 = ro * ro;

    if (1)
    {
#define GC(X)                    \
    ocolor = X;                  \
    ocr = (ocolor >> 0) & 0xff;  \
    ocg = (ocolor >> 8) & 0xff;  \
    ocb = (ocolor >> 16) & 0xff; \
    oca = (ocolor >> 24) & 0xff;

#define FL      \
    GC(row[j]); \
    row[j] = ((unsigned char)((ocr * oca + (cr * ca - ocr * oca) * k) / (oca + (ca - oca) * k)) << 0) | ((unsigned char)((ocg * oca + (cg * ca - ocg * oca) * k) / (oca + (ca - oca) * k)) << 8) | ((unsigned char)((ocb * oca + (cb * ca - ocb * oca) * k) / (oca + (ca - oca) * k)) << 16) | ((unsigned char)(oca + (ca - oca) * k) << 24);

#define FR               \
    GC(row[wo - j - 1]); \
    row[wo - j - 1] = ((unsigned char)((ocr * oca + (cr * ca - ocr * oca) * k) / (oca + (ca - oca) * k)) << 0) | ((unsigned char)((ocg * oca + (cg * ca - ocg * oca) * k) / (oca + (ca - oca) * k)) << 8) | ((unsigned char)((ocb * oca + (cb * ca - ocb * oca) * k) / (oca + (ca - oca) * k)) << 16) | ((unsigned char)(oca + (ca - oca) * k) << 24);
#define STEP_11                           \
    int cx = ro;                          \
    int cy = ro;                          \
    int v1 = (i + 1 - cy) * (i + 1 - cy); \
    int v2 = (i - cy) * (i - cy);         \
    int s1 = ro - sqrtf(ro2 - v1);        \
    int s2 = ro - sqrtf(ro2 - v2) + 1;    \
    int e1 = wo - s2;                     \
    int e2 = wo - s1;

#define STEP_12                           \
    int cx = ro;                          \
    int cy = ro;                          \
    int v1 = (i + 1 - cy) * (i + 1 - cy); \
    int v2 = (i - cy) * (i - cy);         \
    int s1 = ro - sqrtf(ro2 - v1);        \
    int s2 = ro - sqrtf(ri2 - v2) + 1;    \
    int e1 = wo - s2;                     \
    int e2 = wo - s1;

#define STEP_21                                        \
    float k = 0;                                       \
    float dy = 0;                                      \
    float dx = 0;                                      \
    for (int t1 = 0; t1 < sub; ++t1)                   \
        for (int t2 = 0; t2 < sub; ++t2)               \
        {                                              \
            dx = j + delta * 0.5 + t1 * delta - cx;    \
            dy = i + delta * 0.5 + t2 * delta - cy;    \
            k += delta2 * ((dx * dx + dy * dy) < ro2); \
        }                                              \
    float ik = 1.0 - k;

#define STEP_22                                                                         \
    float k = 0;                                                                        \
    float dy = 0;                                                                       \
    float dx = 0;                                                                       \
    for (int t1 = 0; t1 < sub; ++t1)                                                    \
        for (int t2 = 0; t2 < sub; ++t2)                                                \
        {                                                                               \
            dx = j + delta * 0.5 + t1 * delta - cx;                                     \
            dy = i + delta * 0.5 + t2 * delta - cy;                                     \
            k += delta2 * (((dx * dx + dy * dy) < ro2) && ((dx * dx + dy * dy) > ri2)); \
        }                                                                               \
    float ik = 1.0 - k;
    }

    int i = 0;
    int j = 0;
    // lt, rt, rb, lb
    if ((v & 0b1100) == 0b1100)
    {
        for (i = 0; i < ro - ri; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_11;
            for (j = s1; j < s2; j++)
            {
                STEP_21;
                FL;
                FR;
            }
            for (j = s2; j < e1; j++)
            {
                row[j] = color;
            }
        }
        for (; i < ro; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_12;
            for (j = s1; j < s2; j++)
            {
                STEP_22;
                FL;
                FR;
            }
        }
    }
    else if ((v & 0b1100) == 0b0000)
    {
        for (i = 0; i < ro; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            for (j = 0; j < wo; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b1100) == 0b1000)
    {
        for (i = 0; i < ro - ri; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_11;
            for (j = s1; j < s2; j++)
            {
                STEP_21;
                FL;
            }
            for (j = s2; j < wo; j++)
            {
                row[j] = color;
            }
        }
        for (; i < ro; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_12;
            for (j = s1; j < s2; j++)
            {
                STEP_22;
                FL;
            }
            for (j = wo - (ro - ri); j < wo; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b1100) == 0b0100)
    {
        for (i = 0; i < ro - ri; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_11;
            for (j = s1; j < s2; j++)
            {
                STEP_21;
                FR;
            }
            for (j = 0; j < e1; j++)
            {
                row[j] = color;
            }
        }
        for (; i < ro; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_12;
            for (j = s1; j < s2; j++)
            {
                STEP_22;
                FR;
            }
            for (j = 0; j < ro - ri; j++)
            {
                row[j] = color;
            }
        }
    }

    for (; i < ho - ro; ++i)
    {
        unsigned int* row = data + ((i + yo) * line_size) + xo;
        for (j = 0; j < ro - ri; j++)
        {
            row[j] = color;
        }
        for (j = wo - (ro - ri); j < wo; j++)
        {
            row[j] = color;
        }
    }

    if (1)
    {
#undef STEP_11
#undef STEP_12

#define STEP_11                           \
    int cx = ro;                          \
    int cy = ho - ro;                     \
    int v2 = (i + 1 - cy) * (i + 1 - cy); \
    int v1 = (i - cy) * (i - cy);         \
    int s1 = ro - sqrtf(ro2 - v1);        \
    int s2 = ro - sqrtf(ro2 - v2) + 1;    \
    int e1 = wo - s2;                     \
    int e2 = wo - s1;

#define STEP_12                           \
    int cx = ro;                          \
    int cy = ho - ro;                     \
    int v2 = (i + 1 - cy) * (i + 1 - cy); \
    int v1 = (i - cy) * (i - cy);         \
    int s1 = ro - sqrtf(ro2 - v1);        \
    int s2 = ro - sqrtf(ri2 - v2) + 1;    \
    int e1 = wo - s2;                     \
    int e2 = wo - s1;
    }

    if ((v & 0b11) == 0b11)
    {
        for (; i < ho - ro + ri; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_12;
            for (j = s1; j < s2; j++)
            {
                STEP_22;
                FL;
                FR;
            }
        }
        for (; i < ho; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_11;
            for (j = s1; j < s2; j++)
            {
                STEP_21;
                FL;
                FR;
            }
            for (j = s2; j < e1; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b11) == 0b00)
    {
        for (i = 0; i < ho; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            for (j = 0; j < wo; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b11) == 0b01)
    {
        for (; i < ho - ro + ri; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_12;
            for (j = s1; j < s2; j++)
            {
                STEP_22;
                FL;
            }
            for (j = wo - (ro - ri); j < wo; j++)
            {
                row[j] = color;
            }
        }
        for (; i < ho; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_11;
            for (j = s1; j < s2; j++)
            {
                STEP_21;
                FL;
            }
            for (j = s2; j < wo; j++)
            {
                row[j] = color;
            }
        }
    }
    else if ((v & 0b11) == 0b10)
    {
        for (; i < ho - ro + ri; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_12;
            for (j = s1; j < s2; j++)
            {
                STEP_22;
                FR;
            }
            for (j = 0; j < ro - ri; j++)
            {
                row[j] = color;
            }
        }
        for (; i < ho; ++i)
        {
            unsigned int* row = data + ((i + yo) * line_size) + xo;
            STEP_11;
            for (j = s1; j < s2; j++)
            {
                STEP_21;
                FR;
            }
            for (j = 0; j < e1; j++)
            {
                row[j] = color;
            }
        }
    }
#undef GC
#undef FL
#undef FR
#undef STEP_11
#undef STEP_12
#undef STEP_21
#undef STEP_22
}

} // namespace vecp

// =====================================================================
// VecpImage
// =====================================================================
struct VecpImage::VecpImageImp
{
    std::vector<subpath_t> subpaths;
    vecp::vector_t vec;
};

static void DeepCopyVec(vecp::vector_t& dst, const vecp::vector_t& src)
{
    dst.f_type = src.f_type;
    dst.format = src.format;
    dst.ts = src.ts;
    dst.size = src.size;
    dst.type = src.type;
    dst.width = src.width;
    dst.heigt = src.heigt;
    dst.t_xx = src.t_xx;
    dst.t_xy = src.t_xy;
    dst.t_x = src.t_x;
    dst.t_yx = src.t_yx;
    dst.t_yy = src.t_yy;
    dst.t_y = src.t_y;
    dst.metadata_size = src.metadata_size;
    dst.properties_size = src.properties_size;
    dst.counters_size = src.counters_size;
    dst.metadata_allocated = src.metadata_allocated;
    dst.properties_allocated = src.properties_allocated;
    dst.counters_allocated = src.counters_allocated;

    if (src.metadata_allocated)
    {
        dst.metadata = new uint8_t[src.metadata_allocated];
        memcpy(dst.metadata, src.metadata, src.metadata_allocated);
    }
    if (src.properties_allocated)
    {
        dst.properties = new uint8_t[src.properties_allocated];
        memcpy(dst.properties, src.properties, src.properties_allocated);
    }
    if (src.counters_allocated)
    {
        dst.counters = new vecp::counter_t[src.counters_allocated]();
        for (uint32_t i = 0; i < src.counters_allocated; ++i)
        {
            auto& dc = dst.counters[i];
            const auto& sc = src.counters[i];
            dc = sc;
            dc.elements = nullptr;
            dc.data = nullptr;
            dc.metadata = nullptr;
            dc.properties = nullptr;
            if (sc.elements_allocated)
            {
                dc.elements = new vecp::e_elememt_type[sc.elements_allocated];
                memcpy(dc.elements, sc.elements, sc.elements_allocated * sizeof(*sc.elements));
            }
            if (sc.data_allocated)
            {
                dc.data = new double[sc.data_allocated];
                memcpy(dc.data, sc.data, sc.data_allocated * sizeof(*sc.data));
            }
            if (sc.metadata_allocated)
            {
                dc.metadata = new uint8_t[sc.metadata_allocated];
                memcpy(dc.metadata, sc.metadata, sc.metadata_allocated);
            }
            if (sc.properties_allocated)
            {
                dc.properties = new uint8_t[sc.properties_allocated];
                memcpy(dc.properties, sc.properties, sc.properties_allocated);
            }
        }
    }
}

VecpImage::VecpImage() : imp(new VecpImageImp())
{
}

VecpImage::VecpImage(const vecp::vector_t& vec) : imp(new VecpImageImp())
{
    DeepCopyVec(imp->vec, vec);
}

VecpImage::VecpImage(const VecpImage& o) : imp(new VecpImageImp())
{
    DeepCopyVec(imp->vec, o.imp->vec);
}

VecpImage::VecpImage(VecpImage&& o) noexcept : imp(o.imp)
{
    o.imp = nullptr;
}

VecpImage& VecpImage::operator=(const VecpImage& o)
{
    if (this == &o)
        return *this;
    delete imp;
    imp = new VecpImageImp();
    DeepCopyVec(imp->vec, o.imp->vec);
    return *this;
}

VecpImage& VecpImage::operator=(VecpImage&& o) noexcept
{
    if (this == &o)
        return *this;
    delete imp;
    imp = o.imp;
    o.imp = nullptr;
    return *this;
}

VecpImage::~VecpImage()
{
    delete imp;
}

uint32_t VecpImage::GetCounterCount() const
{
    return imp->vec.counters_size;
}

void VecpImage::realloc_counters(uint32_t count)
{
    _begin_;
    while (imp->vec.counters_allocated < count + 1)
    {
        if (!imp->vec.counters)
        {
            imp->vec.counters = new vecp::counter_t[__max(count + 1, 16)];
            imp->vec.counters_allocated = __max(count + 1, 16);
        }
        else
        {
            auto old_c = imp->vec.counters;

            uint32_t old_size = imp->vec.counters_allocated;
            uint32_t new_size = imp->vec.counters_allocated + sqrt(imp->vec.counters_allocated);

            imp->vec.counters = new vecp::counter_t[new_size];

            memcpy(imp->vec.counters, old_c, old_size * sizeof(vecp::counter_t));
            imp->vec.counters_allocated = new_size;
            delete[] old_c;
        }
    }
}

void VecpImage::realloc_counter_elements(uint32_t counter, uint32_t count)
{
    _begin_;
    realloc_counters(counter);
    auto& sel = imp->vec.counters[counter];
    while (sel.elements_allocated < count + 1)
    {
        if (!sel.elements)
        {
            sel.elements = new vecp::e_elememt_type[16];
            sel.elements_allocated = 16;
        }
        else
        {
            auto old_e = sel.elements;

            auto old_size = sel.elements_allocated;
            size_t new_size = sel.elements_allocated + sqrt(sel.elements_allocated);

            sel.elements = new vecp::e_elememt_type[new_size];

            memcpy(sel.elements, old_e, old_size * sizeof(vecp::e_elememt_type));
            sel.elements_allocated = new_size;
            delete[] old_e;
        }
    }
}

void VecpImage::realloc_counter_data(uint32_t counter, uint32_t count)
{
    _begin_;
    realloc_counters(counter);
    auto& sel = imp->vec.counters[counter];
    while (sel.data_allocated < count + 1)
    {
        if (!sel.data)
        {
            sel.data = new double[16];
            sel.data_allocated = 16;
        }
        else
        {
            auto old_d = sel.data;

            uint32_t old_size = sel.data_allocated;
            uint32_t new_size = sel.data_allocated + sqrt(sel.data_allocated);

            sel.data = new double[new_size];

            memcpy(sel.data, old_d, old_size * sizeof(double));
            sel.data_allocated = new_size;
            delete[] old_d;
        }
    }
}

vecp::counter_t* VecpImage::getc(uint32_t counter)
{
    _begin_;
    realloc_counters(counter);
    return imp->vec.counters + counter;
}

void VecpImage::Set_background_color(uint32_t counter, double r, double g, double b, double a)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.background_color;
        col[0] = (uint8_t)(r * 0xff);
        col[1] = (uint8_t)(g * 0xff);
        col[2] = (uint8_t)(b * 0xff);
        col[3] = (uint8_t)(a * 0xff);
    }
}

void VecpImage::Set_background_color(uint32_t counter, double* colors)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.background_color;
        col[0] = (uint8_t)(colors[0] * 0xff);
        col[1] = (uint8_t)(colors[1] * 0xff);
        col[2] = (uint8_t)(colors[2] * 0xff);
        col[3] = (uint8_t)(colors[3] * 0xff);
    }
}

void VecpImage::Set_background_color(uint32_t counter, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.background_color;
        col[0] = r;
        col[1] = g;
        col[2] = b;
        col[3] = a;
    }
}

void VecpImage::Set_background_color(uint32_t counter, uint8_t* colors)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.background_color;
        col[0] = colors[0];
        col[1] = colors[1];
        col[2] = colors[2];
        col[3] = colors[3];
    }
}

void VecpImage::Set_border_color(uint32_t counter, double r, double g, double b, double a)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.border_color;
        col[0] = (uint8_t)(r * 0xff);
        col[1] = (uint8_t)(g * 0xff);
        col[2] = (uint8_t)(b * 0xff);
        col[3] = (uint8_t)(a * 0xff);
    }
}

void VecpImage::Set_border_color(uint32_t counter, double* colors)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.border_color;
        col[0] = (uint8_t)(colors[0] * 0xff);
        col[1] = (uint8_t)(colors[1] * 0xff);
        col[2] = (uint8_t)(colors[2] * 0xff);
        col[3] = (uint8_t)(colors[3] * 0xff);
    }
}

void VecpImage::Set_border_color(uint32_t counter, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.border_color;
        col[0] = r;
        col[1] = g;
        col[2] = b;
        col[3] = a;
    }
}

void VecpImage::Set_border_color(uint32_t counter, uint8_t* colors)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        uint8_t* col = (uint8_t*)&cnt.border_color;
        col[0] = colors[0];
        col[1] = colors[1];
        col[2] = colors[2];
        col[3] = colors[3];
    }
}

void VecpImage::Set_transp(uint32_t counter, double transp)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        cnt.transparent = transp;
    }
}

void VecpImage::Set_border_transp(uint32_t counter, double t)
{
    if (getc(counter))
        imp->vec.counters[counter].border_transparent = t;
}

void VecpImage::Set_inner_border(uint32_t counter, double size)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        cnt.border_inner = size;
    }
}

void VecpImage::Set_outer_border(uint32_t counter, double size)
{
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        cnt.border_outer = size;
    }
}

void VecpImage::Render(void* target, uint32_t out_height, uint32_t out_width, vecp::e_target_out_type target_type, uint8_t samples, int quality, uint64_t line_size, int alpha_id)
{
    vecp::Render(target, out_height, out_width, target_type, imp->vec, samples, quality, line_size, alpha_id);
}

void VecpImage::Clear_elements()
{
    _begin_;
    if (imp->vec.counters && imp->vec.counters_allocated)
    {
        for (auto c = imp->vec.counters; c < imp->vec.counters + imp->vec.counters_allocated; ++c)
        {
            if (c)
            {
                if (c->elements)
                    delete[] c->elements;

                if (c->data)
                    delete[] c->data;

                if (c->metadata)
                    delete[] c->metadata;

                if (c->properties)
                    delete[] c->properties;
            }

            *c = vecp::counter_t();
        }

        delete[] imp->vec.counters;
    }

    if (imp->vec.metadata)
        delete[] imp->vec.metadata;

    if (imp->vec.properties)
        delete[] imp->vec.properties;
    imp->vec = vecp::vector_t();
    _end_;
}

void VecpImage::Clear_elements(uint32_t counter)
{
    _begin_;
    if (imp->vec.counters && imp->vec.counters_allocated)
    {
        if (getc(counter))
        {
            auto c = imp->vec.counters + counter;
            if (c)
            {
                if (c->elements)
                    delete[] c->elements;

                if (c->data)
                    delete[] c->data;
            }

            *c = vecp::counter_t();
        }
    }
}

void VecpImage::Set_offset(uint32_t counter, double x, double y)
{
    _begin_;
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        cnt.t_x = x;
        cnt.t_y = y;
    }
}

void VecpImage::Set_offset(double x, double y)
{
    _begin_;
    imp->vec.t_x = x;
    imp->vec.t_y = y;
}

void VecpImage::Set_scale(uint32_t counter, double x, double y)
{
    _begin_;
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];

        const double& offset_w = cnt.t_x;
        const double& offset_h = cnt.t_y;

        const double brot = atan2(cnt.t_xy, cnt.t_xx);

        const double scale_factor_w = sqrt(cnt.t_xx * cnt.t_xx + cnt.t_xy * cnt.t_xy);
        const double scale_factor_h = sqrt(cnt.t_yx * cnt.t_yx + cnt.t_yy * cnt.t_yy);

        cnt.t_xx = cnt.t_xx / scale_factor_h * x;
        cnt.t_xy = cnt.t_xy / scale_factor_h * x;
        cnt.t_yx = cnt.t_yx / scale_factor_w * y;
        cnt.t_yy = cnt.t_yy / scale_factor_w * y;
    }
}

void VecpImage::Set_scale(double x, double y)
{
    _begin_;
    const double& offset_w = imp->vec.t_x;
    const double& offset_h = imp->vec.t_y;

    const double brot = atan2(imp->vec.t_xy, imp->vec.t_xx);

    const double scale_factor_w = sqrt(imp->vec.t_xx * imp->vec.t_xx + imp->vec.t_xy * imp->vec.t_xy);
    const double scale_factor_h = sqrt(imp->vec.t_yx * imp->vec.t_yx + imp->vec.t_yy * imp->vec.t_yy);

    imp->vec.t_xx = imp->vec.t_xx / scale_factor_w * x;
    imp->vec.t_xy = imp->vec.t_xy / scale_factor_w * x;
    imp->vec.t_yx = imp->vec.t_yx / scale_factor_h * y;
    imp->vec.t_yy = imp->vec.t_yy / scale_factor_h * y;
}

void VecpImage::Set_rot(uint32_t counter, double a)
{
    _begin_;
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];


        const double& offset_w = cnt.t_x;
        const double& offset_h = cnt.t_y;

        const double brot = atan2(cnt.t_xy, cnt.t_xx);

        const double scale_factor_w = sqrt(cnt.t_xx * cnt.t_xx + cnt.t_xy * cnt.t_xy);
        const double scale_factor_h = sqrt(cnt.t_yx * cnt.t_yx + cnt.t_yy * cnt.t_yy);

        cnt.t_xx = cos(a) * scale_factor_w;
        cnt.t_xy = sin(a) * scale_factor_w;

        cnt.t_yy = cos(a) * scale_factor_h;
        cnt.t_yx = -sin(a) * scale_factor_h;
    }
}

void VecpImage::Set_rot(double a)
{
    _begin_;
    const double& offset_w = imp->vec.t_x;
    const double& offset_h = imp->vec.t_y;

    const double brot = atan2(imp->vec.t_xy, imp->vec.t_xx);

    const double scale_factor_w = sqrt(imp->vec.t_xx * imp->vec.t_xx + imp->vec.t_xy * imp->vec.t_xy);
    const double scale_factor_h = sqrt(imp->vec.t_yx * imp->vec.t_yx + imp->vec.t_yy * imp->vec.t_yy);

    imp->vec.t_xx = cos(a) * scale_factor_w;
    imp->vec.t_xy = sin(a) * scale_factor_w;

    imp->vec.t_yy = cos(a) * scale_factor_h;
    imp->vec.t_yx = -sin(a) * scale_factor_h;
}

void VecpImage::Add_element(uint32_t counter, vecp::e_elememt_type type, const double* data)
{
    _begin_;
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        while (counter >= imp->vec.counters_size)
            imp->vec.counters_size = counter + 1;

        uint32_t data_size = __get_offset(type);

        // printf("im.Add_element(%u, (vecp::e_elememt_type)(%u), {", counter, (int)type);
        // for(int o = 0; o < data_size; o++)
        //     printf("%f, ", data[o]);
        // printf("});\n");

        realloc_counter_elements(counter, cnt.elements_size + 1);
        realloc_counter_data(counter, cnt.data_size + data_size);

        fflush(stdout);

        cnt.elements[cnt.elements_size] = type;
        ++cnt.elements_size;

        memcpy(&cnt.data[cnt.data_size], data, data_size * sizeof(double));
        cnt.data_size += data_size;
    }
}

void VecpImage::Add_elements(uint32_t counter, const vecp::e_elememt_type* types, uint32_t count, const double* data)
{
    _begin_;
    if (getc(counter))
    {
        auto& cnt = imp->vec.counters[counter];
        while (counter >= imp->vec.counters_size)
            imp->vec.counters_size = counter + 1;
        uint32_t offset = 0;
        for (uint32_t i = 0; i < count; i++)
        {
            const vecp::e_elememt_type& type = types[i];
            uint32_t data_size = __get_offset(type);

            realloc_counter_elements(counter, cnt.elements_size + 1);
            realloc_counter_data(counter, cnt.data_size + data_size);

            cnt.elements[cnt.elements_size] = type;
            ++cnt.elements_size;

            memcpy(&cnt.data[cnt.data_size], data + offset, data_size * sizeof(double));
            cnt.data_size += data_size;
            offset += data_size;
        }
    }
}

void VecpImage::Load(const void* data, const size_t size)
{
    Clear_elements();
    vecp::LoadBinary(&imp->vec, data, size);
}

size_t VecpImage::To(void* data, size_t size)
{
    return vecp::ToBinary(&imp->vec, data, size);
}

VecpGlyf* VecpFont::GetGlyph(const uint32_t& codepoint) const
{
    auto f = codepoint_map.find(codepoint);
    return f != codepoint_map.end() ? const_cast<VecpGlyf*>(&f->second) : nullptr;
}

void VecpFont::Load(const void* data, const size_t size)
{
    if (!vecp::LoadFontBinary(font, data, size))
        return;

    for (uint32_t i = 0; i < font.glyfs_size; ++i)
    {
        auto& g = font.glyfs[i];

        VecpGlyf vg;
        vg.Codepoint = g.Codepoint;
        vg.advance_width = g.advance_width;
        vg.lsb = g.lsb;
        vg.flags = g.flags;
        vg.glyf = VecpImage(g.glyf);

        bb_t bb = GetVecBB(vg.glyf.imp->vec);
        for (auto& sp : vg.glyf.imp->subpaths)
        {
            bb.upd(sp.bb.min_x, sp.bb.min_y);
            bb.upd(sp.bb.max_x, sp.bb.max_y);
        }
        vg.bx0 = bb.min_x;
        vg.bx1 = bb.max_x;
        vg.by0 = bb.min_y;
        vg.by1 = bb.max_y;

        codepoint_map.emplace(vg.Codepoint, std::move(vg));
    }
}

size_t VecpFont::To(void* data, size_t size)
{
    return 0;
}
