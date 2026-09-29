#include "tectonics.h"
#include <cmath>
#include <algorithm>
#include <omp.h>

void precompute_tectonics(Grids& g, const SimParams& p) {
    const int W = p.W, H = p.H;
    std::fill(g.tectonic_up.begin(), g.tectonic_up.end(), 0.0f);
    const int dx8[8] = {1,1,0,-1,-1,-1,0,1};
    const int dy8[8] = {0,1,1,1,0,-1,-1,-1};
    const float ndist[8] = {1.0f, 1.41421356f, 1.0f, 1.41421356f,
                            1.0f, 1.41421356f, 1.0f, 1.41421356f};

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            uint8_t pid = g.plate[i];
            float vx_me = g.plate_vx[pid] - (y * p.dy - g.plate_pole_y[pid]) * g.plate_omega[pid];
            float vy_me = g.plate_vy[pid] + (x * p.dx - g.plate_pole_x[pid]) * g.plate_omega[pid];
            float thick_me = g.plate_thickness[pid];

            float sum_up = 0.0f;
            int contacts = 0;

            for (int k = 0; k < 8; ++k) {
                int nx = (x + dx8[k] + W) % W;
                int ny = y + dy8[k];
                if (ny < 0 || ny >= H) continue;
                int ni = idx(nx, ny, W);
                uint8_t npid = g.plate[ni];
                if (npid == pid) continue;

                float vx_n = g.plate_vx[npid] - (ny * p.dy - g.plate_pole_y[npid]) * g.plate_omega[npid];
                float vy_n = g.plate_vy[npid] + (nx * p.dx - g.plate_pole_x[npid]) * g.plate_omega[npid];
                float thick_n = g.plate_thickness[npid];

                float dvx = vx_n - vx_me;
                float dvy = vy_n - vy_me;
                float len = ndist[k];
                float nx_dir = dx8[k] / len;
                float ny_dir = dy8[k] / len;
                float dot = dvx * nx_dir + dvy * ny_dir;

                if (dot > 1e-9f) {
                    float thick_factor = (thick_me - thick_n) / (thick_me + thick_n + 1e-10f);
                    sum_up += dot * thick_factor * 0.5f;
                } else if (dot < -1e-9f) {
                    sum_up += dot * 0.5f;
                }
                contacts++;
            }
            if (contacts > 0)
                g.tectonic_up[i] = sum_up / contacts;
        }
    }

    const int dif_iter = 20;
    std::vector<float> tmp(W * H);
    for (int iter = 0; iter < dif_iter; ++iter) {
        #pragma omp parallel for collapse(2)
        for (int y = 1; y < H - 1; ++y) {
            for (int x = 0; x < W; ++x) {
                int i = idx(x, y, W);
                uint8_t pid = g.plate[i];
                int xm = (x - 1 + W) % W;
                int xp = (x + 1) % W;
                float sum = 0.0f;
                int cnt = 0;

                auto add = [&](int xi, int yi) {
                    int ni = idx(xi, yi, W);
                    if (g.plate[ni] == pid) {
                        sum += g.tectonic_up[ni];
                        cnt++;
                    }
                };
                add(xm, y); add(xp, y);
                add(x, y-1); add(x, y+1);
                if (cnt > 0)
                    tmp[i] = (g.tectonic_up[i] + sum) / (cnt + 1.0f);
                else
                    tmp[i] = g.tectonic_up[i];
            }
        }
        #pragma omp parallel for
        for (int i = 0; i < W * H; ++i) {
            int y = i / W;
            if (y > 0 && y < H - 1)
                g.tectonic_up[i] = tmp[i];
        }
    }

    for (auto& v : g.tectonic_up) v *= 1e-9f;
}