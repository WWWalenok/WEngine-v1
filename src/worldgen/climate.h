#pragma once
#include "grid.h"

void init_latitude(Grids& g, const SimParams& p);
void update_wind(Grids& g, const SimParams& p);
void advect_moisture(Grids& g, const SimParams& p);
void compute_precipitation(Grids& g, const SimParams& p, std::vector<float>& precip);