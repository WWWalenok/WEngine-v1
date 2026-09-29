#include "climate.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

void init_latitude(Grids& g, const SimParams& p) {
    const int H = p.H;
    for (int y = 0; y < H; ++y) {
        float frac = y / float(H - 0.5);
        float colat = frac * 3.141592653589793f;
        g.lat[y] = 3.141592653589793f / 2.0f - colat;
        g.f_cor[y] = 2.0f * p.omega * std::sin(g.lat[y]);
        float lat_deg = g.lat[y] * 180.0f / 3.141592653589793f;
        float a = std::abs(lat_deg);
        float p_base = 101325.0f;
        
        g.base_pressure[y] = p_base;
    }
}

void update_wind(Grids& g, const SimParams& p) {
    const int W = p.W, H = p.H;

    // Расчёт давления (с учётом рельефа)
    std::vector<float> pressure(W * H);
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            pressure[i] = g.base_pressure[y] * (p.H_scale - g.height[i] / p.H_scale);
        }
    }

    std::vector<float> new_u(W * H), new_v(W * H);
    std::vector<float> tmp_u(W*H), tmp_v(W*H);

    const float tau = 10000.0f;   // характерное время трения (сек)
    const float wind_dt = 60.0f;  // шаг интегрирования внутри подшага
    const int sub_steps = 2;     // количество подитераций на одно обновление

    for (int sub = 0; sub < sub_steps; ++sub) {
        #pragma omp parallel for collapse(2)
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int i = idx(x, y, W);
                int xp = (x + 1) % W;
                int xm = (x - 1 + W) % W;
                int yp = std::min(y + 1, H - 1);
                int ym = std::max(y - 1, 0);

                float dp_dx = (pressure[idx(xp, y, W)] - pressure[idx(xm, y, W)]) / (2.0f * p.dx);
                float dp_dy = (pressure[idx(x, yp, W)] - pressure[idx(x, ym, W)]) / (2.0f * p.dy);

                float f = g.f_cor[y];
                float u = g.wind_u[i];
                float v = g.wind_v[i];

                // Уравнения движения с трением (стационарное приближение с шагом по времени)
                float du = -dp_dx / p.rho_air + f * v - u / tau;
                float dv = -dp_dy / p.rho_air - f * u - v / tau;

                new_u[i] = u + du * wind_dt;
                new_v[i] = v + dv * wind_dt;
            }
        }
    }
    // Небольшое сглаживание (диффузия) для подавления точечных выбросов
    const int smooth_iters = 1;
    const float blend = 0.005f;
    for (int iter = 0; iter < smooth_iters; ++iter) {
        #pragma omp parallel for collapse(2)
        for (int y = 1; y < H - 1; ++y) {
            for (int x = 0; x < W; ++x) {
                int i = idx(x, y, W);
                int xm = (x - 1 + W) % W;
                int xp = (x + 1) % W;
                float lap_u = new_u[idx(xm,y,W)] + new_u[idx(xp,y,W)]
                            + new_u[idx(x,y-1,W)] + new_u[idx(x,y+1,W)] - 4.0f*new_u[i];
                float lap_v = new_v[idx(xm,y,W)] + new_v[idx(xp,y,W)]
                            + new_v[idx(x,y-1,W)] + new_v[idx(x,y+1,W)] - 4.0f*new_v[i];
                tmp_u[i] = new_u[i] + blend * lap_u;
                tmp_v[i] = new_v[i] + blend * lap_v;
            }
        }
        new_u.swap(tmp_u);
        new_v.swap(tmp_v);
    }

    // Смешивание с предыдущим полем (инерция)
    const float inertia = 0.7f;
    #pragma omp parallel for
    for (int i = 0; i < W * H; ++i) {
        g.wind_u[i] = g.wind_u[i] * (1.0f - inertia) + new_u[i] * inertia;
        g.wind_v[i] = g.wind_v[i] * (1.0f - inertia) + new_v[i] * inertia;
    }
}

static inline float bilinear_sample(const std::vector<float>& field, int W, int H, float x, float y) {
    int x0 = (int)std::floor(x); int x1 = x0 + 1;
    int y0 = (int)std::floor(y); int y1 = y0 + 1;
    float fx = x - x0; float fy = y - y0;
    auto sample = [&](int px, int py) -> float {
        px = (px % W + W) % W;
        py = std::clamp(py, 0, H - 1);
        return field[py * W + px];
    };
    return (1-fx)*(1-fy)*sample(x0,y0) + fx*(1-fy)*sample(x1,y0) +
           (1-fx)*fy*sample(x0,y1) + fx*fy*sample(x1,y1);
}

void advect_moisture(Grids& g, const SimParams& p) {
    const int W = p.W, H = p.H;
    std::vector<float> new_moist(W * H);
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            float u = g.wind_u[i];
            float v = g.wind_v[i];
            float src_x = x - u * p.dt / p.dx;
            float src_y = y - v * p.dt / p.dy;
            new_moist[i] = bilinear_sample(g.moisture, W, H, src_x, src_y);
        }
    }
    #pragma omp parallel for
    for (int i = 0; i < W * H; ++i) {
        if (g.height[i] <= 0.0f) new_moist[i] = 1.0f;
    }
    g.moisture.swap(new_moist);
}

void compute_precipitation(Grids& g, const SimParams& p, std::vector<float>& precip) {
    const int W = p.W, H = p.H;
    precip.assign(W * H, 0.0f);
    const float k = 0.01f;
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            int xp = (x + 1) % W;
            int xm = (x - 1 + W) % W;
            int yp = std::min(y + 1, H - 1);
            int ym = std::max(y - 1, 0);
            float grad_x = (g.height[idx(xp, y, W)] - g.height[idx(xm, y, W)]) / (2.0f * p.dx);
            float grad_y = (g.height[idx(x, yp, W)] - g.height[idx(x, ym, W)]) / (2.0f * p.dy);
            float lift = g.wind_u[i] * grad_x + g.wind_v[i] * grad_y;
            if (lift > 0.0f) {
                float ppt = k * g.moisture[i] * lift;
                precip[i] = ppt;
                g.moisture[i] -= ppt * p.dt;
                if (g.moisture[i] < 0.0f) g.moisture[i] = 0.0f;
            }
        }
    }
}