
#include "VECP_element.h"
#include <math.h>
#include <cmath>
#include <float.h>
#include <algorithm>

typedef unsigned int UINT32;
#define ERR_NO_ERROR 0x00000000
#define ERR_NAN 0x00000001
#define ERR_INF 0x00000002
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

// src https://www.programmersought.com/article/35535041057/ 
// src https://www.programmersought.com/article/58865032874/

/*************************************************
Function: is_number
Description: Determine whether the floating point number is nan
Input: floating point number x
Output: None
Return: If the floating-point number x is nan, return 0, otherwise return 1
Author: Marc Pony(marc_pony@163.com)
*************************************************/
int is_number(double x)
{
	return (x == x);
}

/*************************************************
Function: is_finite_number
Description: Determine whether the floating point number is inf
Input: floating point number x
Output: None
Return: If the floating point number x is inf, return 0, otherwise return 1
Author: Marc Pony(marc_pony@163.com)
*************************************************/
int is_finite_number(double x)
{
	return (x >= -FLT_MAX && x <= FLT_MAX);
}

/*************************************************
Function: solve_quadratic_equation
Description: Find all real roots of a quadratic equation (a*x^2 + b*x + c = 0)
Input: The coefficient of the equation p = (c, b, a)
Output: all real roots x of the equation, the number of real roots rootCount
Return: error number
Author: Marc Pony(marc_pony@163.com)
*************************************************/
UINT32 solve_quadratic_equation(double p[], double x[], int* rootCount)
{
	int i;
	double a, b, c, delta, sqrtDelta;
	const double ZERO = FLT_MIN;  // min normalized positive value（1.175494351e-38F）
	const double EPS = FLT_MIN;
	UINT32 errNo = ERR_NO_ERROR;

	*rootCount = 0;
	for (i = 0; i < 3; i++) {
		if (!is_number(p[i])) {
			errNo = ERR_NAN;
			return errNo;
		}
		if (!is_finite_number(p[i])) {
			errNo = ERR_INF;
			return errNo;
		}
	}

	a = p[2];
	b = p[1];
	c = p[0];

	if (fabs(a - 0.0) < EPS) {
		if (fabs(b - 0.0) > EPS) {
			x[0] = -c / b;
			*rootCount = 1;
		}
	}
	else {
		b /= a;
		c /= a;
		a = 1.0;

		delta = b * b - 4.0 * a * c;
		if (delta > ZERO) {
			if (fabs(c - 0.0) < EPS)	//If c = 0, sqrt(b*b-4*a*c) is not equal to |b| due to calculation errors
			{
				x[0] = 0.0;
				x[1] = -b / a;
			}
			else {
				sqrtDelta = sqrt(delta);
				if (b > 0.0) {
					x[0] = (-2.0 * c) / (b + sqrtDelta);	//Avoid subtracting two very close numbers, resulting in loss of precision
					x[1] = (-b - sqrtDelta) / (2.0 * a);
				}
				else {
					x[0] = (-b + sqrtDelta) / (2.0 * a);
					x[1] = (-2.0 * c) / (b - sqrtDelta);	//Avoid subtracting two very close numbers, resulting in loss of precision
				}
			}
			*rootCount = 2;
		}
		else if (fabs(delta - 0.0) < EPS) {
			x[0] = x[1] = -b / (2.0 * a);
			*rootCount = 2;
		}
		else {
			*rootCount = 0;
		}
	}
	return errNo;
}

/*************************************************
Function: solve_cubic_equation
Description: Shengjin formula finds all real roots of one-dimensional cubic equation (a*x^3 + b*x^2 + c*x + d = 0)
A = b * b - 3.0 * a * c;
B = b * c - 9.0 * a * d;
C = c * c - 3.0 * b * d;
(1) When A = B = 0, the equation has a triple real root
(2) When Δ = B^2－4 * A * C> 0, the equation has a real root and a pair of conjugate imaginary roots
(3) When Δ = B^2－4 * A * C = 0, the equation has three real roots, of which there is a double root
(4) When Δ = B^2－4 * A * C <0, the equation has three unequal real roots
Input: The coefficients of the equation p = (d, c, b, a)
Output: all real roots x of the equation, the number of real roots rootCount
Return: error number
Author: Marc Pony(marc_pony@163.com)
*************************************************/
UINT32 solve_cubic_equation(double p[], double x[], int* rootCount)
{
	int i;
	double a, b, c, d, A, B, C, delta;
	double Y1, Y2, Z1, Z2, K, parm[3], roots[2], theta, T;
	const double ZERO = FLT_MIN;  // min normalized positive value（1.175494351e-38F）
	const double EPS = FLT_MIN;
	const double CALCULATE_ERROR = 1.0e-7;
	UINT32 errNo = ERR_NO_ERROR;

	*rootCount = 0;
	for (i = 0; i < 4; i++) {
		if (!is_number(p[i])) {
			errNo = ERR_NAN;
			return errNo;
		}
		if (!is_finite_number(p[i])) {
			errNo = ERR_INF;
			return errNo;
		}
	}

	a = p[3];
	b = p[2];
	c = p[1];
	d = p[0];

	if (fabs(a - 0.0) < EPS) {
		parm[2] = b;
		parm[1] = c;
		parm[0] = d;

		errNo = solve_quadratic_equation(parm, x, rootCount);
	}
	else {
		b /= a;
		c /= a;
		d /= a;
		a = 1.0;

		A = b * b - 3.0 * a * c;
		B = b * c - 9.0 * a * d;
		C = c * c - 3.0 * b * d;

		delta = B * B - 4.0 * A * C;

		if (fabs(A - 0.0) < EPS && fabs(B - 0.0) < EPS) {
			x[0] = x[1] = x[2] = -b / (3.0 * a);
			*rootCount = 3;
			return errNo;
		}

		if (delta > ZERO) {
			parm[2] = 1.0;
			parm[1] = B;
			parm[0] = A * C;

			errNo = solve_quadratic_equation(parm, roots, rootCount);
			if (errNo != ERR_NO_ERROR) {
				return errNo;
			}
			Z1 = roots[0];
			Z2 = roots[1];

			Y1 = A * b + 3.0 * a * Z1;
			Y2 = A * b + 3.0 * a * Z2;

			if (Y1 < 0.0 && Y2 < 0.0)	//The base of the pow function must be non-negative and must be discussed separately
			{
				x[0] = (-b + pow(-Y1, 1.0 / 3.0) + pow(-Y2, 1.0 / 3.0)) / (3.0 * a);
			}
			else if (Y1 < 0.0 && Y2 > 0.0) {
				x[0] = (-b + pow(-Y1, 1.0 / 3.0) - pow(Y2, 1.0 / 3.0)) / (3.0 * a);
			}
			else if (Y1 > 0.0 && Y2 < 0.0) {
				x[0] = (-b - pow(Y1, 1.0 / 3.0) + pow(-Y2, 1.0 / 3.0)) / (3.0 * a);
			}
			else {
				x[0] = (-b - pow(Y1, 1.0 / 3.0) - pow(Y2, 1.0 / 3.0)) / (3.0 * a);
			}
			*rootCount = 1;
		}
		else if (fabs(delta - 0.0) < EPS) {
			if (fabs(A - 0.0) > EPS) {
				K = B / A;
				x[0] = -b / a + K;
				x[1] = -0.5 * K;
				*rootCount = 2;
				//x[1] = x[2] = -0.5 * K;
				//*rootCount = 3;
			}
		}
		else {
			if (A > 0.0) {
				T = (2.0 * A * b - 3.0 * a * B) / (2.0 * pow(A, 3.0 / 2.0));
				if (T > 1.0)	//Due to calculation errors, the value of T may be slightly greater than 1 (such as 1.0000001)
				{
					if (T < 1.0 + CALCULATE_ERROR) {
						T = 1.0;
					}
					else {
						return errNo;
					}
				}
				if (T < -1.0) {
					if (T > -1.0 - CALCULATE_ERROR) {
						T = -1.0;
					}
					else {
						return errNo;
					}
				}
				theta = acos(T);
				x[0] = (-b - 2.0 * sqrt(A) * cos(theta / 3.0)) / (3.0 * a);
				x[1] = (-b + sqrt(A) * (cos(theta / 3.0) + sqrt(3.0) * sin(theta / 3.0))) / (3.0 * a);
				x[2] = (-b + sqrt(A) * (cos(theta / 3.0) - sqrt(3.0) * sin(theta / 3.0))) / (3.0 * a);
				*rootCount = 3;
			}
		}
	}
	return errNo;
}


int loc_Resample_filter(VecpBaseElement* self, double* ts, VecpElement_ptr* out, int max_out_count)
{
	static const auto filter = [] (const void* elem1, const void* elem2) -> int {
		const double& a = *(const double*)elem1;
		const double& b = *(const double*)elem2;
		if (a < b)
			return -1;
		return a > b;
	};
	std::qsort(ts, 16, sizeof(double), filter);
	int count = 1;
	int num_0 = 1;
    constexpr double B1 = 1e-9; 
    constexpr double B2 = 1 - 1e-9; 
	for (int i = 0; i < 16; i++) {
		if (ts[i] <= B1) {
			num_0 = i + 1;
		}
		if (ts[i] > B1 && ts[i] < B2) {

			count++;
		}
		if (ts[i] >= B2)
			break;
	}
	for (int c = 0; c < (MIN(count, max_out_count)); c++) {
		double t0 = ts[num_0 + c - 1];
		double t1 = ts[num_0 + c];
        if(t0 <= B1)
            t0 = 0;
        if(t1 >= B2)
            t1 = 1;

		out[c] = self->Get_sub_curve(t0, t1);
	}
	return count;
}

#define LVL 25
#define EPS1 -0.0000
#define EPS_1 -0.1
#define DT 0.500

double VecpBaseElement::Get_y_by_x(double target_x, double eps)
{
	double y = 0;
	double x = 0;
	double x0 = 0;
	double x1 = 0;
	double t = 0.5;
	double dt = 0.25;
	Get_xy_by_t(0, &x0, &y);
	Get_xy_by_t(1, &x1, &y);
	Get_xy_by_t(t, &x, &y);
	if ((x0 - target_x) * (x1 - target_x) > EPS1) {
		return NAN;
	}
	for (int level = 0; level < LVL && (x - target_x) * (x - target_x) > eps * eps; level++) {
		if ((x - target_x) * (x - x0) > 0) // Направление к точке 0 (t == 0) совпадает с направлением к цели
			t -= dt;
		else
			t += dt;

		dt *= DT;

		Get_xy_by_t(t, &x, &y);
	}
	return y;
}

double VecpBaseElement::Get_x_by_y(double target_y, double eps)
{
	double y = 0;
	double x = 0;
	double y0 = 0;
	double y1 = 0;
	double t = 0.5;
	double dt = 0.25;
	Get_xy_by_t(0, &x, &y0);
	Get_xy_by_t(1, &x, &y1);
	if ((y0 - target_y) * (y1 - target_y) > EPS1) {
		return NAN;
	}
	Get_xy_by_t(t, &x, &y);
	for (int level = 0; level < LVL && (y - target_y) * (y - target_y) > eps * eps; level++) {
		if ((y - target_y) * (y - y0) > 0) // Направление к точке 0 (t == 0) совпадает с направлением к цели
			t -= dt;
		else
			t += dt;

		dt *= DT;

		Get_xy_by_t(t, &x, &y);
	}
	return x;
}


bool VecpBaseElement::Get_xy_by_t_offset(double offset, double t, double *_x, double *_y)
{
	double nx, ny;
    Get_xy_by_t(t, _x, _y);
    Get_nxny_by_t(t, &nx, &ny);
    *_x = *_x + nx * offset;
    *_y = *_y + ny * offset;
    return true;
}

double VecpBaseElement::Get_y_by_x_offset(double offset, double target_x, double eps)
{
	double y = 0;
	double x = 0;
	double x0 = 0;
	double x1 = 0;
	double t = 0.5;
	double dt = 0.25;
	Get_xy_by_t_offset(offset, 0, &x0, &y);
	Get_xy_by_t_offset(offset, 1, &x1, &y);
	Get_xy_by_t_offset(offset, t, &x, &y);
	if ((x0 - target_x) * (x1 - target_x) > EPS1) {
		return NAN;
	}
	for (int level = 0; level < LVL && (x - target_x) * (x - target_x) > eps * eps; level++) {
		if ((x - target_x) * (x - x0) > 0)
			t -= dt;
		else
			t += dt;

		dt *= DT;

		Get_xy_by_t_offset(offset, t, &x, &y);
	}
	return y;
}

double VecpBaseElement::Get_x_by_y_offset(double offset, double target_y, double eps)
{
	double y = 0;
	double x = 0;
	double y0 = 0;
	double y1 = 0;
	double t = 0.5;
	double dt = 0.25;
	Get_xy_by_t_offset(offset, 0, &x, &y0);
	Get_xy_by_t_offset(offset, 1, &x, &y1);
	if ((y0 - target_y) * (y1 - target_y) > EPS1) {
		return NAN;
	}
	Get_xy_by_t_offset(offset, t, &x, &y);
	for (int level = 0; level < LVL && (y - target_y) * (y - target_y) > eps * eps; level++) {
		if ((y - target_y) * (y - y0) > 0)
			t -= dt;
		else
			t += dt;

		dt *= DT;

		Get_xy_by_t_offset(offset, t, &x, &y);
	}
	return x;
}

// Base Bezie4

bool VecpBezie4Element::Get_xys_by_t(double t, double* _x, double* _y)
{
	//E0
	double x01 = _x[0] = x[0] * (1 - t) + x[1] * t;
	double y01 = _y[0] = y[0] * (1 - t) + y[1] * t;
	//E1
	double x12 = _x[1] = x[1] * (1 - t) + x[2] * t;
	double y12 = _y[1] = y[1] * (1 - t) + y[2] * t;
	//E2
	double x23 = _x[2] = x[2] * (1 - t) + x[3] * t;
	double y23 = _y[2] = y[2] * (1 - t) + y[3] * t;
	//E3
	double x34 = _x[3] = x[3] * (1 - t) + x[4] * t;
	double y34 = _y[3] = y[3] * (1 - t) + y[4] * t;

	//------------------
	//V0
	double x012 = _x[4] = x01 * (1 - t) + x12 * t;
	double y012 = _y[4] = y01 * (1 - t) + y12 * t;
	//V1
	double x123 = _x[5] = x12 * (1 - t) + x23 * t;
	double y123 = _y[5] = y12 * (1 - t) + y23 * t;
	//V2
	double x234 = _x[6] = x23 * (1 - t) + x34 * t;
	double y234 = _y[6] = y23 * (1 - t) + y34 * t;

	//------------------
	//K0
	double x0123 = _x[7] = x012 * (1 - t) + x123 * t;
	double y0123 = _y[7] = y012 * (1 - t) + y123 * t;
	//K1
	double x1234 = _x[8] = x123 * (1 - t) + x234 * t;
	double y1234 = _y[8] = y123 * (1 - t) + y234 * t;

	//------------------
	//J0
	double x01234 = _x[9] = x0123 * (1 - t) + x1234 * t;
	double y01234 = _y[9] = y0123 * (1 - t) + y1234 * t;
	return 1;
}

bool VecpBezie4Element::Get_xy_by_t(double t, double* _x, double* _y)
{
	double _t = (1 - t);
	double k0 = _t * _t * _t * _t * 1;
	double k1 = t * _t * _t * _t * 4;
	double k2 = t * t * _t * _t * 6;
	double k3 = t * t * t * _t * 4;
	double k4 = t * t * t * t * 1;

	*_x = x[0] * k0 + x[1] * k1 + x[2] * k2 + x[3] * k3 + x[4] * k4;
	*_y = y[0] * k0 + y[1] * k1 + y[2] * k2 + y[3] * k3 + y[4] * k4;

	return 1;
}

bool VecpBezie4Element::Get_nxny_by_t(double t, double* _x, double* _y)
{
    double _t = (1 - t);
	double k0 = _t * _t * _t * 4;
	double k1 =  t * _t * _t * 12;
	double k2 =  t *  t * _t * 12;
	double k3 =  t *  t *  t * 4;

	*_x = (x[1] - x[0]) * k0 + (x[2] - x[1]) * k1 + (x[3] - x[2]) * k2 + (x[4] - x[3]) * k3;
	*_y = (y[1] - y[0]) * k0 + (y[2] - y[1]) * k1 + (y[3] - y[2]) * k2 + (y[4] - y[3]) * k3;
    
	float l = sqrt(_x[0] * _x[0] + _y[0] * _y[0]);

    if(l == 0)
    {
		if(t == 0)
		{
			*_x = x[2] - x[0];
			*_y = y[2] - y[0];
		}
		else if(t == 1)
		{
			*_x = x[4] - x[2];
			*_y = y[4] - y[2];
		}
        l = sqrt(_x[0] * _x[0] + _y[0] * _y[0]);
		if(l == 0)
		{
			if(t == 0)
			{
				*_x = x[3] - x[0];
				*_y = y[3] - y[0];
			}
			else if(t == 1)
			{
				*_x = x[4] - x[1];
				*_y = y[4] - y[1];
			}
			l = sqrt(_x[0] * _x[0] + _y[0] * _y[0]);
			if(l == 0)
			{
				*_x = x[4] - x[0];
				*_y = y[4] - y[0];
			}
		}
    }
	if(l != 0)
     	l = 1.0 / l;
	double nx = *_y;
	double ny = -*_x;
	*_x = nx * l;
	*_y = ny * l;

	return 1;
}

VecpElement_ptr VecpBezie4Element::Get_curve_pre(double t)
{
	VecpElement_ptr _ret(new VecpBezie4Element(*this));
	auto& ret = *_ret;

	double xs[16];
	double ys[16];

	Get_xys_by_t(t, xs, ys);

	ret.x[1] = xs[0];
	ret.x[2] = xs[4];
	ret.x[3] = xs[7];
	ret.x[4] = xs[9];

	ret.y[1] = ys[0];
	ret.y[2] = ys[4];
	ret.y[3] = ys[7];
	ret.y[4] = ys[9];

	return _ret;
}

VecpElement_ptr VecpBezie4Element::Get_curve_post(double t)
{
	VecpElement_ptr _ret(new VecpBezie4Element(*this));
	auto& ret = *_ret;
	double xs[16];
	double ys[16];

	Get_xys_by_t(t, xs, ys);

	ret.x[0] = xs[9];
	ret.x[1] = xs[8];
	ret.x[2] = xs[6];
	ret.x[3] = xs[3];

	ret.y[0] = ys[9];
	ret.y[1] = ys[8];
	ret.y[2] = ys[6];
	ret.y[3] = ys[3];


	return _ret;
}

VecpElement_ptr VecpBezie4Element::Get_sub_curve(double t1, double t2)
{
	auto to_t2 = Get_curve_pre(t2);

	double tn = t1 / t2;

	auto _ret = to_t2->Get_curve_post(tn);

	delete to_t2;

    if(
        (_ret->x[0] - _ret->x[4]) * (_ret->x[0] - _ret->x[4]) + 
        (_ret->y[0] - _ret->y[4]) * (_ret->y[0] - _ret->y[4]) < EPS_1
    )
    {
        VecpElement_ptr tret(new VecpLineElement({
            _ret->x[0], _ret->y[0], 
            _ret->x[4], _ret->y[4]
        }));
        delete _ret;
        return tret;
    }

	return _ret;
}

int VecpBezie4Element::Resample_element(VecpElement_ptr* out, int max_out_count)
{

	double ts[16];
	for (int i = 0; i < 16; i++)
		ts[i] = 2;

	ts[6] = 0;
	ts[7] = 1;
	{
		double k = 4 * (1 * x[0] - 4 * x[1] + 6 * x[2] - 4 * x[3] + 1 * x[4]);
		double a = -12 * (1 * x[0] - 3 * x[1] + 3 * x[2] - 1 * x[3]);
		double b = 12 * (1 * x[0] - 2 * x[1] + 1 * x[2]);
		double c = -4 * (1 * x[0] - 1 * x[1]);
		double v[4]{
			c,
			b,
			a,
			k,

		};
		int roots = 0;
		solve_cubic_equation(v, &ts[0], &roots);
	}
	{
		double k = 4 * (1 * y[0] - 4 * y[1] + 6 * y[2] - 4 * y[3] + 1 * y[4]);
		double a = -12 * (1 * y[0] - 3 * y[1] + 3 * y[2] - 1 * y[3]);
		double b = 12 * (1 * y[0] - 2 * y[1] + 1 * y[2]);
		double c = -4 * (1 * y[0] - 1 * y[1]); 1;
		double v[4]{
			c,
			b,
			a,
			k,
		};
		int roots = 0;
		solve_cubic_equation(v, &ts[3], &roots);
	}

	return loc_Resample_filter(this, ts, out, max_out_count);
}

// Bezie3

bool VecpBezie3Element::Get_xys_by_t(double t, double* _x, double* _y)
{
	//E0
	double x01 = _x[0] = x[0] * (1 - t) + x[1] * t;
	double y01 = _y[0] = y[0] * (1 - t) + y[1] * t;
	//E1
	double x12 = _x[1] = x[1] * (1 - t) + x[2] * t;
	double y12 = _y[1] = y[1] * (1 - t) + y[2] * t;
	//E2
	double x23 = _x[2] = x[2] * (1 - t) + x[3] * t;
	double y23 = _y[2] = y[2] * (1 - t) + y[3] * t;

	//------------------
	//V0
	double x012 = _x[3] = x01 * (1 - t) + x12 * t;
	double y012 = _y[3] = y01 * (1 - t) + y12 * t;
	//V1
	double x123 = _x[4] = x12 * (1 - t) + x23 * t;
	double y123 = _y[4] = y12 * (1 - t) + y23 * t;

	//------------------
	//K0
	double x0123 = _x[5] = x012 * (1 - t) + x123 * t;
	double y0123 = _y[5] = y012 * (1 - t) + y123 * t;

	return 1;
}

bool VecpBezie3Element::Get_xy_by_t(double t, double* _x, double* _y)
{
	double _t = (1 - t);
	double k0 = _t * _t * _t * 1;
	double k1 = t * _t * _t * 3;
	double k2 = t * t * _t * 3;
	double k3 = t * t * t * 1;

	*_x = x[0] * k0 + x[1] * k1 + x[2] * k2 + x[3] * k3;
	*_y = y[0] * k0 + y[1] * k1 + y[2] * k2 + y[3] * k3;

	return 1;
}

bool VecpBezie3Element::Get_nxny_by_t(double t, double* _x, double* _y)
{
    double _t = (1 - t);
	double k0 = _t * _t * 3;
	double k1 =  t * _t * 6;
	double k2 =  t *  t * 3;

	*_x = (x[1] - x[0]) * k0 + (x[2] - x[1]) * k1 + (x[3] - x[2]) * k2;
	*_y = (y[1] - y[0]) * k0 + (y[2] - y[1]) * k1 + (y[3] - y[2]) * k2;
    
    float l = sqrt(_x[0] * _x[0] + _y[0] * _y[0]);

    if(l == 0 && k1 == 0)
    {
		*_x = x[2] - x[1];
		*_y = y[2] - y[1];
        l = sqrt(_x[0] * _x[0] + _y[0] * _y[0]);
		if(l == 0)
		{
            *_x = x[3] - x[0];
            *_y = y[3] - y[0];
		}
    }
	if(l != 0)
     	l = 1.0 / l;
	double nx = *_y;
	double ny = -*_x;
	*_x = nx * l;
	*_y = ny * l;

	return 1;
}

VecpElement_ptr VecpBezie3Element::Get_curve_pre(double t)
{
	VecpElement_ptr _ret(new VecpBezie3Element(*this));
	auto& ret = *_ret;

	double xs[16];
	double ys[16];
	Get_xys_by_t(t, xs, ys);

	ret.x[1] = xs[0];
	ret.x[2] = xs[3];
	ret.x[3] = xs[5];

	ret.y[1] = ys[0];
	ret.y[2] = ys[3];
	ret.y[3] = ys[5];

	return _ret;
}

VecpElement_ptr VecpBezie3Element::Get_curve_post(double t)
{
	VecpElement_ptr _ret(new VecpBezie3Element(*this));
	auto& ret = *_ret;
	double xs[16];
	double ys[16];

	Get_xys_by_t(t, xs, ys);

	ret.x[0] = xs[5];
	ret.x[1] = xs[4];
	ret.x[2] = xs[2];

	ret.y[0] = ys[5];
	ret.y[1] = ys[4];
	ret.y[2] = ys[2];
	return _ret;
}

VecpElement_ptr VecpBezie3Element::Get_sub_curve(double t1, double t2)
{
	auto to_t2 = Get_curve_pre(t2);

	double tn = t1 / t2;

	auto _ret = to_t2->Get_curve_post(tn);

	delete to_t2;
    
    if(
        (_ret->x[0] - _ret->x[3]) * (_ret->x[0] - _ret->x[3]) + 
        (_ret->y[0] - _ret->y[3]) * (_ret->y[0] - _ret->y[3]) < EPS_1
    )
    {
        VecpElement_ptr tret(new VecpLineElement({
            _ret->x[0], _ret->y[0], 
            _ret->x[3], _ret->y[3]
        }));
        delete _ret;
        return tret;
    }

	return _ret;
}

int VecpBezie3Element::Resample_element(VecpElement_ptr* out, int max_out_count)
{
	double ts[16];
	for (int i = 0; i < 16; i++)
		ts[i] = 2;

	ts[0] = 0;
	ts[5] = 1;
    double a, b, c;

    double sub_sqrt;

    a        = 3 * (-x[0] + 3 * x[1] - 3 * x[2] + x[3]);
    b        = 6 * (x[0] - 2 * x[1] + x[2]);
    c        = 3 * (x[1] - x[0]);
    sub_sqrt = b * b - 4 * a * c;
    if (sub_sqrt >= 0)
    {
        double _sqrt = sqrt(sub_sqrt);
        if (a == 0)
        {
            ts[1] = -c / b;
        }
        else if (_sqrt == 0)
        {
            ts[1] = -b / (2.0 * a);
        }
        else
        {
            ts[1] = -(b + _sqrt) / (2.0 * a);
            ts[2] = -(b - _sqrt) / (2.0 * a);
        }
    }
    a        = 3 * (-y[0] + 3 * y[1] - 3 * y[2] + y[3]);
    b        = 6 * (y[0] - 2 * y[1] + y[2]);
    c        = 3 * (y[1] - y[0]);
    sub_sqrt = b * b - 4 * a * c;
    if (sub_sqrt >= 0)
    {
        double _sqrt = sqrt(sub_sqrt);
        if (a == 0)
        {
            ts[3] = -c / b;
        }
		else if (_sqrt == 0)
        {
            ts[3] = -b / (2.0 * a);
		}
        else
        {
            ts[3] = -(b + _sqrt) / (2.0 * a);
            ts[4] = -(b - _sqrt) / (2.0 * a);
        }
    }
	return loc_Resample_filter(this, ts, out, max_out_count);
}

// Bezie2

bool VecpBezie2Element::Get_xys_by_t(double t, double* _x, double* _y)
{
	//E0
	double x01 = _x[0] = x[0] * (1 - t) + x[1] * t;
	double y01 = _y[0] = y[0] * (1 - t) + y[1] * t;
	//E1
	double x12 = _x[1] = x[1] * (1 - t) + x[2] * t;
	double y12 = _y[1] = y[1] * (1 - t) + y[2] * t;

	//------------------
	//V0
	double x012 = _x[2] = x01 * (1 - t) + x12 * t;
	double y012 = _y[2] = y01 * (1 - t) + y12 * t;

	return 1;
}

bool VecpBezie2Element::Get_xy_by_t(double t, double* _x, double* _y)
{
	double _t = (1 - t);
	double k0 = _t * _t * 1;
	double k1 = t * _t * 2;
	double k2 = t * t * 1;

	*_x = x[0] * k0 + x[1] * k1 + x[2] * k2;
	*_y = y[0] * k0 + y[1] * k1 + y[2] * k2;

	return 1;
}

bool VecpBezie2Element::Get_nxny_by_t(double t, double* _x, double* _y)
{
    double _t = (1 - t);
	double k0 = _t * 2;
	double k1 =  t * 2;

	*_x = (x[1] - x[0]) * k0 + (x[2] - x[1]) * k1;
	*_y = (y[1] - y[0]) * k0 + (y[2] - y[1]) * k1;
    
    float l = 1.0 / sqrt(_x[0] * _x[0] + _y[0] * _y[0]);

	if(l == 0)
	{
		*_x = x[2] - x[0];
        *_y = y[2] - y[0];
	}

	if(l != 0)
     	l = 1.0 / l;
	double nx = *_y;
	double ny = -*_x;
	*_x = nx * l;
	*_y = ny * l;

	return 1;
}

VecpElement_ptr VecpBezie2Element::Get_curve_pre(double t)
{
	VecpElement_ptr _ret(new VecpBezie2Element(*this));
	auto& ret = *_ret;

	double xs[16];
	double ys[16];

	Get_xys_by_t(t, xs, ys);

	ret.x[1] = xs[0];
	ret.x[2] = xs[2];

	ret.y[1] = ys[0];
	ret.y[2] = ys[2];


	return _ret;
}

VecpElement_ptr VecpBezie2Element::Get_curve_post(double t)
{
	VecpElement_ptr _ret(new VecpBezie2Element(*this));
	auto& ret = *_ret;
	double xs[16];
	double ys[16];

	Get_xys_by_t(t, xs, ys);

	ret.x[0] = xs[2];
	ret.x[1] = xs[1];

	ret.y[0] = ys[2];
	ret.y[1] = ys[1];

	return _ret;
}

VecpElement_ptr VecpBezie2Element::Get_sub_curve(double t1, double t2)
{
	auto to_t2 = Get_curve_pre(t2);

	double tn = t1 / t2;

	auto _ret = to_t2->Get_curve_post(tn);

	delete to_t2;

    if(
        (_ret->x[0] - _ret->x[2]) * (_ret->x[0] - _ret->x[2]) + 
        (_ret->y[0] - _ret->y[2]) * (_ret->y[0] - _ret->y[2]) < EPS_1
    )
    {
        VecpElement_ptr tret(new VecpLineElement({
            _ret->x[0], _ret->y[0], 
            _ret->x[2], _ret->y[2]
        }));
        delete _ret;
        return tret;
    }

	return _ret;
}

int VecpBezie2Element::Resample_element(VecpElement_ptr* out, int max_out_count)
{

	double ts[16];
	for (int i = 0; i < 16; i++)
		ts[i] = 2;

	ts[0] = 0;
	double a;
	if ((a = (x[0] - 2 * x[1] + x[2])) != 0)
		ts[1] = (x[0] - x[1]) / a;
	if ((a = (y[0] - 2 * y[1] + y[2])) != 0)
		ts[2] = (y[0] - y[1]) / a;
	ts[3] = 1;

	return loc_Resample_filter(this, ts, out, max_out_count);
}

// Line

bool VecpLineElement::Get_xys_by_t(double t, double* _x, double* _y)
{
	_x[0] = x[0] * (1 - t) + x[1] * t;
	_y[0] = y[0] * (1 - t) + y[1] * t;

	return 1;
}

bool VecpLineElement::Get_xy_by_t(double t, double* _x, double* _y)
{
	*_x = x[0] * (1 - t) + x[1] * t;
	*_y = y[0] * (1 - t) + y[1] * t;

	return 1;
}

bool VecpLineElement::Get_nxny_by_t(double t, double* _x, double* _y)
{
    double _t = (1 - t);
	double k0 = 1;

	*_x = (x[1] - x[0]) * k0;
	*_y = (y[1] - y[0]) * k0;
    double l = sqrt(_x[0] * _x[0] + _y[0] * _y[0]);
    
	if(l != 0)
     	l = 1.0 / l;
	double nx = *_y;
	double ny = -*_x;
	*_x = nx * l;
	*_y = ny * l;

	return 1;
}

VecpElement_ptr VecpLineElement::Get_curve_pre(double t)
{
	VecpElement_ptr _ret(new VecpLineElement(*this));
	auto& ret = *_ret;
	Get_xy_by_t(t, &ret.x[1], &ret.y[1]);
	return _ret;
}

VecpElement_ptr VecpLineElement::Get_curve_post(double t)
{
	VecpElement_ptr _ret(new VecpLineElement(*this));
	auto& ret = *_ret;
	Get_xy_by_t(t, &ret.x[0], &ret.y[0]);
	return _ret;
}

VecpElement_ptr VecpLineElement::Get_sub_curve(double t1, double t2)
{
	VecpElement_ptr _ret(new VecpLineElement(*this));
	auto& ret = *_ret;
	if (t1 > t2)
		std::swap(t1, t2);

	Get_xy_by_t(t1, &ret.x[0], &ret.y[0]);
	Get_xy_by_t(t2, &ret.x[1], &ret.y[1]);

	return _ret;
}

int VecpLineElement::Resample_element(VecpElement_ptr* out, int max_out_count)
{
    if(x[0] == x[1] && y[0] == y[1])
        return 0;
	out[0] = new VecpLineElement(*this);
	return 1;
}

// Arc

/*
double el = sqrt((dir.x * dir.x) + (dir.y * dir.y));
double ex = dir.x / el;
double ey = dir.y / el;

x[0] - cx
y[0] - cy
x[1] - ex
y[1] - ey
x[2] - a
y[2] - b
x[3] - t0
y[3] - t1

VecpArcElement e{
    cx, cy, 
    ex, ey,
    a, b,
    t0, t1
};
*/

bool VecpArcElement::Get_xys_by_t(double t, double* _x, double* _y)
{
    Get_xy_by_t(t, _x, _y);
	return 1;
}

bool VecpArcElement::Get_xy_by_t(double _t, double* _x, double* _y)
{
    double t = x[3] + (y[3] - x[3]) * _t;
    while (t < -1) t += 2;
    while (t > 1) t -= 2;
    double w = (t > 0 ? t : -t);
    double tn = 1 - w * 2;
    double kn = sqrt(1 - tn * tn); // t^2 + k^2 = 1
    kn = (t > 0 ? kn : -kn);
    double nx = +x[1] * x[2];
    double ny = +y[1] * x[2];
    double ox = -y[1] * y[2];
    double oy = +x[1] * y[2];
    *_x = x[0] + tn * nx + kn * ox;
    *_y = y[0] + tn * ny + kn * oy;
    return 1;
}

bool VecpArcElement::Get_nxny_by_t(double t, double* _x, double* _y)
{
	double bx = 0;
    double by = 0;
    double ex = 0;
    double ey = 0;
    Get_xy_by_t(t, _x, _y);
    double l = 0.0;

    double off = 1e-5;

    while(l < 1e-5)
    {
        Get_xy_by_t(t - off, &bx, &by);
        Get_xy_by_t(t + off, &ex, &ey);
        l = (ex - bx) * (ex - bx) + (ey - by) * (ey - by);
        off *= 10;
    }
    
    double k = 1.0 / sqrt(l);
    _x[0] = ((ey - by)) * k;
    _y[0] = ((bx - ex)) * k;
    return true;
}

VecpElement_ptr VecpArcElement::Get_curve_pre(double t)
{
	VecpElement_ptr _ret(new VecpArcElement(*this));
	auto& ret = *_ret;
    ret.y[3] = x[3] + (y[3] - x[3]) * t;
	return _ret;
}

VecpElement_ptr VecpArcElement::Get_curve_post(double t)
{
	VecpElement_ptr _ret(new VecpArcElement(*this));
	auto& ret = *_ret;
    ret.x[3] = x[3] + (y[3] - x[3]) * t;
	return _ret;
}

VecpElement_ptr VecpArcElement::Get_sub_curve(double t1, double t2)
{
	VecpElement_ptr _ret(new VecpArcElement(*this));
	auto& ret = *_ret;
    ret.x[3] = x[3] + (y[3] - x[3]) * t1;
    ret.y[3] = x[3] + (y[3] - x[3]) * t2;
	return _ret;
}

int VecpArcElement::Resample_element(VecpElement_ptr* out, int max_out_count)
{
    double ts[16];
	for (int i = 0; i < 16; i++)
		ts[i] = 2;
    ts[14] = 0;
    ts[15] = 1;
    double nx = +x[1] * x[2];
    double ny = +y[1] * x[2];
    double ox = -y[1] * y[2];
    double oy = +x[1] * y[2];
    if(1)
    {
        if(nx == 0)
        {
            ts[0] = 0.5;
            ts[1] = 1.5;
        }
        else if(ox == 0)
        {
            ts[0] = 0;
            ts[1] = 1;
        }
        else
        {
            double tn = nx / sqrt(nx * nx + ox * ox);
            // tn = 1 - w * 2;
            double w = (ox > 0 ? 1 : -1) * (1 - tn) / 2;
            ts[0] = w;
            ts[1] = ts[0] + 1;
        }
    }

    if(1)
    {
        if(ny == 0)
        {
            ts[2] = 0.5;
            ts[3] = 1.5;
        }
        else if(oy == 0)
        {
            ts[2] = 0;
            ts[3] = 1;
        }
        else
        {
            double tn = ny / sqrt(ny * ny + oy * oy);
            double w = (oy > 0 ? 1 : -1) * (1 - tn) / 2;
            ts[2] = w;
            ts[3] = ts[2] + 1;
        }
    }
    double tk = 1 / (y[3] - x[3]);
    for(int i = 0; i < 4; i++)
    {
        ts[4 + i] = (ts[i] + 2 - x[3]) * tk;
        ts[8 + i] = (ts[i] - 2 - x[3]) * tk;
        ts[i] = (ts[i] - x[3]) * tk;
    }

	return loc_Resample_filter(this, ts, out, max_out_count);
}



