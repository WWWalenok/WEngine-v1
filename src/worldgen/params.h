#pragma once
#include <cstdint>

struct SimParams {
    int W, H;
    float dx, dy;
    float dt;
    int windUpdateInterval;
    float K_fluvial, m, n;
    float K_wind, wind_threshold;
    float creep_coeff;
    float rho_air;
    float H_scale;
    float omega;
    float total_time;
};