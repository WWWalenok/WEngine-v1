#include "plate_generator.h"
#include <cmath>
#include <algorithm>
#include <vector>
#include <queue>
#include <random>
#include <cstdint>
#include <array>
#include <numeric>

namespace {
    float wrap_dist_x(float x1, float x2, int W) {
        float dx = std::fabs(x1 - x2);
        return std::min(dx, W - dx);
    }
    float wrap_dist2(float x1, float y1, float x2, float y2, int W) {
        float dx = wrap_dist_x(x1, x2, W);
        float dy = y1 - y2;
        return dx * dx + dy * dy;
    }

    class PerlinNoise {
        std::array<int, 512> p;
        float fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
        float lerp(float a, float b, float t) { return a + t * (b - a); }
        float grad(int hash, float x, float y) {
            int h = hash & 15;
            float u = h < 8 ? x : y;
            float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
            return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
        }
    public:
        PerlinNoise(unsigned seed = 0) {
            std::mt19937 rng(seed);
            std::array<int, 256> perm;
            for (int i = 0; i < 256; ++i) perm[i] = i;
            std::shuffle(perm.begin(), perm.end(), rng);
            for (int i = 0; i < 256; ++i) p[i] = perm[i];
            for (int i = 0; i < 256; ++i) p[256 + i] = perm[i];
        }
        float noise(float x, float y) {
            int X = (int)std::floor(x) & 255;
            int Y = (int)std::floor(y) & 255;
            float xf = x - std::floor(x);
            float yf = y - std::floor(y);
            float u = fade(xf);
            float v = fade(yf);
            int a = p[X] + Y;
            int b = p[X + 1] + Y;
            return lerp(lerp(grad(p[a], xf, yf), grad(p[b], xf - 1, yf), u),
                        lerp(grad(p[a + 1], xf, yf - 1), grad(p[b + 1], xf - 1, yf - 1), u), v);
        }
        float octave_noise(float x, float y, int octaves, float persistence = 0.5f) {
            float total = 0, amp = 1, freq = 1, max = 0;
            for (int i = 0; i < octaves; ++i) {
                total += noise(x * freq, y * freq) * amp;
                max += amp;
                amp *= persistence;
                freq *= 2;
            }
            return total / max;
        }
    };
}

void generate_plates(Grids& g, const SimParams& p,
                     int num_seed_points, int num_plates,
                     unsigned seed,
                     float seed_height_sigma,
                     int perlin_octaves, float perlin_scale, float perlin_amplitude) {
    const int W = p.W, H = p.H;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> distX(0.0f, float(W));
    std::uniform_real_distribution<float> distY(0.0f, float(H));

    std::vector<float> pts_x(num_seed_points), pts_y(num_seed_points);
    for (int i = 0; i < num_seed_points; ++i) {
        pts_x[i] = distX(rng);
        pts_y[i] = distY(rng);
    }

    std::vector<int> cell_map(W * H);
    std::vector<float> sum_x, sum_y, count;
    for (int iter = 0; iter < 3; ++iter) {
        #pragma omp parallel for collapse(2)
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int i = idx(x, y, W);
                float best = 1e30f; int best_id = 0;
                for (int j = 0; j < num_seed_points; ++j) {
                    float d2 = wrap_dist2(float(x)+0.5f, float(y)+0.5f, pts_x[j], pts_y[j], W);
                    if (d2 < best) { best = d2; best_id = j; }
                }
                cell_map[i] = best_id;
            }
        }
        sum_x.assign(num_seed_points, 0.0f);
        sum_y.assign(num_seed_points, 0.0f);
        count.assign(num_seed_points, 0.0f);
        for (int i = 0; i < W * H; ++i) {
            int c = cell_map[i];
            int x = i % W, y = i / W;
            sum_x[c] += x + 0.5f;
            sum_y[c] += y + 0.5f;
            count[c] += 1.0f;
        }
        for (int j = 0; j < num_seed_points; ++j) {
            if (count[j] > 0) {
                pts_x[j] = sum_x[j] / count[j];
                pts_y[j] = sum_y[j] / count[j];
            } else {
                pts_x[j] = distX(rng);
                pts_y[j] = distY(rng);
            }
        }
    }

    std::vector<int> plate_seeds;
    {
        std::vector<int> indices(num_seed_points);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        plate_seeds.assign(indices.begin(), indices.begin() + num_plates);
    }

    std::vector<int> plate_map(W * H, -1);
    for (int j = 0; j < num_plates; ++j) {
        int cell_id = plate_seeds[j];
        for (int i = 0; i < W * H; ++i) {
            if (cell_map[i] == cell_id) plate_map[i] = j;
        }
    }

    bool changed = true;
    std::vector<int> next_plate(W * H);
    while (changed) {
        changed = false;
        #pragma omp parallel for collapse(2)
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                int i = idx(x, y, W);
                next_plate[i] = plate_map[i];
                if (plate_map[i] != -1) continue;
                int candidate = -1;
                bool conflict = false;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = (x + dx + W) % W;
                        int ny = y + dy;
                        if (ny < 0 || ny >= H) continue;
                        int ni = idx(nx, ny, W);
                        int np = plate_map[ni];
                        if (np == -1) continue;
                        if (candidate == -1) candidate = np;
                        else if (candidate != np) { conflict = true; break; }
                    }
                    if (conflict) break;
                }
                if (!conflict && candidate != -1) {
                    next_plate[i] = candidate;
                    changed = true;
                }
            }
        }
        plate_map.swap(next_plate);
    }

    for (int i = 0; i < W * H; ++i) {
        if (plate_map[i] == -1) {
            int y = i / W, x = i % W;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = (x + dx + W) % W;
                    int ny = y + dy;
                    if (ny >= 0 && ny < H) {
                        int nv = plate_map[idx(nx, ny, W)];
                        if (nv != -1) { plate_map[i] = nv; break; }
                    }
                }
                if (plate_map[i] != -1) break;
            }
        }
    }

    {
        std::uniform_real_distribution<float> noise(0.0f, 1.0f);
        std::vector<uint8_t> new_plate(W * H);
        for (int i = 0; i < W * H; ++i) new_plate[i] = (uint8_t)plate_map[i];

        for (int i = 0; i < W * H; ++i) {
            int y = i / W, x = i % W;
            uint8_t my = new_plate[i];
            bool boundary = false;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx == 0 && dy == 0) continue;
                    int nx = (x + dx + W) % W;
                    int ny = y + dy;
                    if (ny < 0 || ny >= H) continue;
                    if (new_plate[idx(nx, ny, W)] != my) { boundary = true; break; }
                }
                if (boundary) break;
            }
            if (boundary && noise(rng) < 0.3f) {
                std::vector<uint8_t> candidates;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = (x + dx + W) % W;
                        int ny = y + dy;
                        if (ny < 0 || ny >= H) continue;
                        uint8_t nv = new_plate[idx(nx, ny, W)];
                        if (nv != my) candidates.push_back(nv);
                    }
                }
                if (!candidates.empty()) {
                    std::uniform_int_distribution<int> pick(0, (int)candidates.size()-1);
                    new_plate[i] = candidates[pick(rng)];
                }
            }
        }
        g.plate.assign(new_plate.begin(), new_plate.end());
    }

    g.plate_vx.resize(num_plates);
    g.plate_vy.resize(num_plates);
    g.plate_omega.resize(num_plates, 0.0f);
    g.plate_pole_x.resize(num_plates);
    g.plate_pole_y.resize(num_plates);
    g.plate_thickness.resize(num_plates);
    std::vector<float> plate_base_height(num_plates);

    std::uniform_real_distribution<float> vel_dist(-0.1f, 0.1f);
    std::uniform_real_distribution<float> thick_dist(50000.0f, 150000.0f);
    std::uniform_real_distribution<float> pole_x(0.0f, float(W) * p.dx);
    std::uniform_real_distribution<float> pole_y(0.0f, float(H) * p.dy);
    std::uniform_real_distribution<float> omega_dist(-1e-8f, 1e-8f);
    std::uniform_real_distribution<float> base_h_dist(-1000.0f, 1000.0f);

    for (int i = 0; i < num_plates; ++i) {
        g.plate_vx[i] = vel_dist(rng) / (365.0f * 24.0f * 3600.0f);
        g.plate_vy[i] = vel_dist(rng) / (365.0f * 24.0f * 3600.0f);
        g.plate_omega[i] = omega_dist(rng);
        g.plate_pole_x[i] = pole_x(rng);
        g.plate_pole_y[i] = pole_y(rng);
        g.plate_thickness[i] = thick_dist(rng);
        plate_base_height[i] = base_h_dist(rng);
    }

    std::vector<int> seed_plate(num_seed_points);
    for (int j = 0; j < num_seed_points; ++j) {
        int ix = (int)pts_x[j] % W;
        int iy = (int)pts_y[j];
        iy = std::clamp(iy, 0, H-1);
        seed_plate[j] = g.plate[idx(ix, iy, W)];
    }

    std::vector<float> seed_height(num_seed_points);
    std::normal_distribution<float> normal(0.0f, seed_height_sigma);
    for (int j = 0; j < num_seed_points; ++j) {
        float mean = plate_base_height[seed_plate[j]];
        float val = mean + normal(rng);
        val = std::clamp(val, mean - 3 * seed_height_sigma, mean + 3 * seed_height_sigma);
        seed_height[j] = val;
    }

    g.height.resize(W * H);
    const float epsilon = 0.001f;
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            uint8_t my_plate = g.plate[i];
            float total_w = 0, total_h = 0;
            for (int j = 0; j < num_seed_points; ++j) {
                if (seed_plate[j] != my_plate) continue;
                float dx = wrap_dist_x(float(x)+0.5f, pts_x[j], W);
                float dy = float(y)+0.5f - pts_y[j];
                float dist = std::sqrt(dx*dx + dy*dy) + epsilon;
                float w = 1.0f / (dist * dist);
                total_w += w;
                total_h += w * seed_height[j];
            }
            if (total_w > 0)
                g.height[i] = total_h / total_w;
            else
                g.height[i] = plate_base_height[my_plate];
        }
    }

    PerlinNoise perlin(seed + 1);
    float inv_scale = 1.0f / perlin_scale;
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            float noise_val = perlin.octave_noise(x * inv_scale, y * inv_scale, perlin_octaves);
            g.height[i] += noise_val * perlin_amplitude;
        }
    }
}