#pragma once
#include <math.h>
#include <float.h>
#include <initializer_list>


struct point_t
{
    double x;
    double y;

    point_t operator+(const point_t& o)
    {
        return { x + o.x, y + o.y };
    }

    point_t operator-(const point_t& o)
    {
        return { x - o.x, y - o.y };
    }

    point_t operator*(const double& o)
    {
        return { x * o, y * o };
    }

    double operator*(const point_t& o)
    {
        return x * o.x + y * o.y;
    }

    point_t operator/(const double& o)
    {
        return { x / o, y / o };
    }

    double len() { return sqrt(x * x + y * y); }
};

struct transform_matrix
{
    double 
        xx, xy, x,
        yx, yy, y;

    void applay(double* v) const
    {
        applay(v[0], v[1]);
    }

    void applay(point_t& v) const
    {
        applay(v.x, v.y);
    }

    void applay(double& _x, double& _y) const
    {
        double tx = _x;
        _x = xx * tx + xy * _y + x;
        _y = yx * tx + yy * _y + y;
    }

    static transform_matrix mult(const transform_matrix& a, const transform_matrix& b)
    {
        return 
        {
            a.xx * b.xx + a.xy * b.yx, a.xx * b.xy + a.xy * b.yy, a.xx * b.x + a.xy * b.y + a.x,
            a.yx * b.xx + a.yy * b.yx, a.yx * b.xy + a.yy * b.yy, a.yx * b.x + a.yy * b.y + a.y
        };
    }

    transform_matrix invers2() const
    {
        double k = 1 / (xx * yy - xy * yx);
        return {
             yy * k, -xy * k, 0,
            -yx * k,  xx * k, 0
        };
    }
};

struct VecpBaseElement;

typedef VecpBaseElement* VecpElement_ptr;

#define __MIN(a,b)            (((a) < (b)) ? (a) : (b))

#define __INIT__SWG(name, __type)\
name() \
{ \
	for (int i = 0; i < 10; i++) { \
		x[i] = 0; \
		y[i] = 0; \
	} \
	type = __type; \
} \
 \
name(double* xs, double* ys, int s) \
{	 \
	s = __MIN(s, 10); \
	for (int i = 0; i < s; i++) { \
		x[i] = xs[i]; \
		y[i] = ys[i]; \
	} \
	type = __type; \
} \
 \
name(std::initializer_list<double> list) \
{ \
	int s = list.size() / 2; \
	auto val = list.begin(); \
	s = __MIN(s, 10); \
	for (int i = 0; i < s; i++) { \
		x[i] = val[i * 2]; \
		y[i] = val[i * 2 + 1]; \
	} \
	type = __type; \
} \
static VecpElement_ptr get_elem(const VecpElement_ptr& val) \
{\
    VecpElement_ptr _ret(new name(val->x, val->y, 10)); \
    return _ret; \
}\
static const name::Type stype = __type;\
~name(){} \

struct VecpBaseElement
{

	enum Type : int
	{
		LINE,
		BEZIE2,
		BEZIE3,
		BEZIE4,
        ARC,
		NONE
	};

	int type;

	double x[10];
	double y[10];

	__INIT__SWG(VecpBaseElement, NONE)

	virtual bool Get_xys_by_t(double t, double* _x, double* _y) { throw 1; };

	virtual bool Get_xy_by_t(double t, double* _x, double* _y) { throw 1; };

    virtual bool Get_nxny_by_t(double t, double* _x, double* _y) { throw 1; };

    virtual bool Get_xy_by_t_offset(double offset, double t, double* _x, double* _y);

	virtual VecpElement_ptr Get_curve_pre(double t) { throw 1; };

	virtual VecpElement_ptr Get_curve_post(double t) { throw 1; };

	virtual VecpElement_ptr Get_sub_curve(double t1, double t2) { throw 1; };

	virtual int Resample_element(VecpElement_ptr* out, int max_out_count) { throw 1; };

	virtual double Get_x_by_y(double y, double eps = 1e-6);

	virtual double Get_y_by_x(double x, double eps = 1e-6);

	virtual double Get_x_by_y_offset(double offset, double y, double eps = 1e-7);

	virtual double Get_y_by_x_offset(double offset, double x, double eps = 1e-7);
};

struct VecpBezie4Element : public VecpBaseElement
{
	__INIT__SWG(VecpBezie4Element, BEZIE4)

	virtual bool Get_xys_by_t(double t, double* _x, double* _y) override;

	virtual bool Get_xy_by_t(double t, double* _x, double* _y) override;

    virtual bool Get_nxny_by_t(double t, double* _x, double* _y) override;

	virtual VecpElement_ptr Get_curve_pre(double t) override;

	virtual VecpElement_ptr Get_curve_post(double t) override;

	virtual VecpElement_ptr Get_sub_curve(double t1, double t2) override;

	virtual int Resample_element(VecpElement_ptr* out, int max_out_count) override;
};

struct VecpBezie3Element : public VecpBaseElement
{
	__INIT__SWG(VecpBezie3Element, BEZIE3)

	virtual bool Get_xys_by_t(double t, double* _x, double* _y) override;

	virtual bool Get_xy_by_t(double t, double* _x, double* _y) override;

    virtual bool Get_nxny_by_t(double t, double* _x, double* _y) override;

	virtual VecpElement_ptr Get_curve_pre(double t) override;

	virtual VecpElement_ptr Get_curve_post(double t) override;

	virtual VecpElement_ptr Get_sub_curve(double t1, double t2) override;

	virtual int Resample_element(VecpElement_ptr* out, int max_out_count) override;
};

struct VecpBezie2Element : public VecpBaseElement
{
	__INIT__SWG(VecpBezie2Element, BEZIE2)

	virtual bool Get_xys_by_t(double t, double* _x, double* _y) override;

	virtual bool Get_xy_by_t(double t, double* _x, double* _y) override;

    virtual bool Get_nxny_by_t(double t, double* _x, double* _y) override;

	virtual VecpElement_ptr Get_curve_pre(double t) override;

	virtual VecpElement_ptr Get_curve_post(double t) override;

	virtual VecpElement_ptr Get_sub_curve(double t1, double t2) override;

	virtual int Resample_element(VecpElement_ptr* out, int max_out_count) override;
};

struct VecpLineElement : public VecpBaseElement
{
	__INIT__SWG(VecpLineElement, LINE)

	virtual bool Get_xys_by_t(double t, double* _x, double* _y) override;

	virtual bool Get_xy_by_t(double t, double* _x, double* _y) override;

    virtual bool Get_nxny_by_t(double t, double* _x, double* _y) override;

	virtual VecpElement_ptr Get_curve_pre(double t) override;

	virtual VecpElement_ptr Get_curve_post(double t) override;

	virtual VecpElement_ptr Get_sub_curve(double t1, double t2) override;

	virtual int Resample_element(VecpElement_ptr* out, int max_out_count) override;
};

struct VecpArcElement : public VecpBaseElement
{
	__INIT__SWG(VecpArcElement, ARC)

	virtual bool Get_xys_by_t(double t, double* _x, double* _y) override;

	virtual bool Get_xy_by_t(double t, double* _x, double* _y) override;

    virtual bool Get_nxny_by_t(double t, double* _x, double* _y) override;

	virtual VecpElement_ptr Get_curve_pre(double t) override;

	virtual VecpElement_ptr Get_curve_post(double t) override;

	virtual VecpElement_ptr Get_sub_curve(double t1, double t2) override;

	virtual int Resample_element(VecpElement_ptr* out, int max_out_count) override;
private:
    bool inited = false;

};



