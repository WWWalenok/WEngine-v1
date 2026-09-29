#pragma once
#include <vector>
#include <cstdint>
#include "params.h"

struct Grids
{
    std::vector<float> height;
    std::vector<uint8_t> plate;
    std::vector<float> plate_vx, plate_vy;
    std::vector<float> plate_omega;
    std::vector<float> plate_pole_x, plate_pole_y;
    std::vector<float> plate_thickness;
    std::vector<float> tectonic_up;
    std::vector<float> lat;
    std::vector<float> f_cor;
    std::vector<float> base_pressure;
    std::vector<float> wind_u, wind_v;
    std::vector<float> moisture;
    std::vector<float> flow_dir_x, flow_dir_y; // текущее направление потока в клетке (единичный вектор)
    std::vector<float> momentum_x, momentum_y; // поле импульса потока
    std::vector<float> momentum_track_x, momentum_track_y; // накопление за шаг
    std::vector<float> discharge; // нормированный расход (0..1)
    std::vector<float> discharge_track; // накопление объёма
    std::vector<float> precip;
    std::vector<float> flow_acc;
    std::vector<int> order;
    std::vector<int> downstream;
    std::vector<float> water_depth; // поверхностная вода, метры
    std::vector<float> water_track; // буфер для накопления при стоке

    Grids(int W, int H)
      : height(W * H, 0),
        plate(W * H, 0),
        plate_vx(),
        plate_vy(),
        plate_omega(),
        plate_pole_x(),
        plate_pole_y(),
        plate_thickness(),
        tectonic_up(W * H, 0),
        lat(H, 0),
        f_cor(H, 0),
        base_pressure(H, 0),
        wind_u(W * H, 0),
        wind_v(W * H, 0),
        moisture(W * H, 0),
        flow_dir_x(W * H, 0),
        flow_dir_y(W * H, 0),
        momentum_x(W * H, 0),
        momentum_y(W * H, 0),
        momentum_track_x(W * H, 0),
        momentum_track_y(W * H, 0),
        discharge(W * H, 0),
        discharge_track(W * H, 0),
        precip(W * H, 0),
        flow_acc(W * H, 0),
        order(W * H),
        downstream(W * H),
        water_depth(W * H, 0),
        water_track(W * H, 0)
    {
    }
};

inline int idx(int x, int y, int W)
{
    return y * W + x;
}