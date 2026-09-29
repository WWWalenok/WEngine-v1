#pragma once
#include "grid.h"
#include <random>

void generate_plates(Grids& g, const SimParams& p,
                     int num_seed_points, int num_plates,
                     unsigned seed = 42,
                     float seed_height_sigma = 500.0f,
                     int perlin_octaves = 4,
                     float perlin_scale = 50.0f,
                     float perlin_amplitude = 800.0f);