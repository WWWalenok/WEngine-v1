#pragma once
#include "grid.h"
#include <vector>

void apply_grid_hydrology(Grids& g, const SimParams& p);
void apply_thermal_erosion(Grids& g, const SimParams& p, float repose_angle);
void apply_slope_diffusion(Grids& g, const SimParams& p, int iterations);