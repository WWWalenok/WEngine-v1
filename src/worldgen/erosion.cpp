#include "erosion.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <omp.h>

// Быстрая нормализация через reciprocal sqrt (точность достаточна)
static inline float fast_rsqrt(float x) {
    // fallback к стандартному, если не подключать аппаратные интринсики
    return 1.0f / std::sqrt(x);
}

// Крутейший спуск с инерцией (оптимизировано)
static int steepest_descent_inertial(int i, const Grids& g, const SimParams& p, float inertia_factor) {
    const int W = p.W, H = p.H;
    int x = i % W, y = i / W;
    float myh = g.height[i];

    float prev_dx = g.flow_dir_x[i];
    float prev_dy = g.flow_dir_y[i];
    float prev_len_sq = prev_dx*prev_dx + prev_dy*prev_dy;
    float prev_inv_len = (prev_len_sq > 1e-12f) ? fast_rsqrt(prev_len_sq) : 0.0f;
    float pdx = prev_dx * prev_inv_len; // нормализованное предыдущее направление
    float pdy = prev_dy * prev_inv_len;

    int best_j = -1;
    float best_score = -1e10f;

    // 8 соседей, развёрнуто вручную для скорости
    auto try_neighbor = [&](int dx, int dy, float dist) {
        int nx = (x + dx + W) % W;
        int ny = y + dy;
        if (ny < 0 || ny >= H) return;
        int j = ny * W + nx;
        float diff = myh - g.height[j];
        if (diff <= 0) return;
        float slope = diff / dist;
        // скалярное произведение с пред-нормализованным направлением
        float alignment = (pdx * dx + pdy * dy) / dist; // потому что (dx/dist, dy/dist) – единичное направление
        float score = slope + inertia_factor * alignment * slope;
        if (score > best_score) {
            best_score = score;
            best_j = j;
        }
    };

    try_neighbor(1, 0, 1.0f);
    try_neighbor(-1, 0, 1.0f);
    try_neighbor(0, 1, 1.0f);
    try_neighbor(0, -1, 1.0f);
    try_neighbor(1, 1, 1.41421356f);
    try_neighbor(-1, 1, 1.41421356f);
    try_neighbor(1, -1, 1.41421356f);
    try_neighbor(-1, -1, 1.41421356f);

    return best_j;
}
void update_water(Grids& g, const SimParams& p);
// Основная функция
void apply_grid_hydrology(Grids& g, const SimParams& p) {
    const int W = p.W, H = p.H;
    const int N = W * H;
    std::mt19937 rng((size_t)(&g) ^ time(0));

    // Используем предвыделенные массивы
    std::vector<float>& precip = g.precip;
    std::vector<float>& flow_acc = g.flow_acc;
    std::vector<float>& mom_track_x = g.momentum_track_x;
    std::vector<float>& mom_track_y = g.momentum_track_y;
    std::vector<int>& order = g.order;
    std::vector<int>& downstream = g.downstream;

    // 1. Осадки: случайные с отсечением положительных (логика без изменений)
    std::uniform_real_distribution<float> rain_dist(-2000.0f, 10.0f);
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        float v = rain_dist(rng);
        precip[i] = (v > 0.0f) ? v : v * -1e-4;   // убрали умножение на 1.0f
        
    }

    // Параметры (константы в стеке)
    const float inertia = 0.3f * 1e3f;
    const float K = 2.0f;

    // 2. Сортировка подсчётом (та же, но order уже выделен)
    #pragma omp parallel
    {
        float local_min = std::numeric_limits<float>::max();
        float local_max = std::numeric_limits<float>::lowest();
        #pragma omp for nowait
        for (int i = 0; i < N; ++i) {
            float h = g.height[i];
            if (h < local_min) local_min = h;
            if (h > local_max) local_max = h;
        }
        #pragma omp critical
        {
            static float global_min = local_min, global_max = local_max;
            // на самом деле можно один раз вычислить min/max вне цикла, но так тоже быстро
        }
    }

    auto [hmin, hmax] = std::minmax_element(g.height.begin(), g.height.end());
    const int bins = 1024;
    std::vector<int> count(bins, 0);   // маленький массив на стеке
    float scale = (bins - 1) / (*hmax - *hmin + 1e-10f);

    for (int i = 0; i < N; ++i) {
        int bin = static_cast<int>((g.height[i] - *hmin) * scale);
        count[bin]++;
    }
    for (int i = 1; i < bins; ++i) count[i] += count[i-1];
    for (int i = N - 1; i >= 0; --i) {
        int bin = static_cast<int>((g.height[i] - *hmin) * scale);
        order[--count[bin]] = i;
    }
    std::reverse(order.begin(), order.end());

    // 3. Направления стока (параллельно)
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        int j = steepest_descent_inertial(i, g, p, inertia);
        downstream[i] = j;
        if (j != -1) {
            int y = i / W, x = i % W;
            int jy = j / W, jx = j % W;
            float dx = (float)(jx - x);
            float dy = (float)(jy - y);
            float inv_len = fast_rsqrt(dx*dx + dy*dy + 1e-12f);
            g.flow_dir_x[i] = dx * inv_len;
            g.flow_dir_y[i] = dy * inv_len;
        } else {
            g.flow_dir_x[i] = 0.0f;
            g.flow_dir_y[i] = 0.0f;
        }
    }

    // 4. Накопление расхода (последовательное, без изменений)
    std::fill(flow_acc.begin(), flow_acc.end(), 0.0f);
    std::fill(mom_track_x.begin(), mom_track_x.end(), 0.0f);
    std::fill(mom_track_y.begin(), mom_track_y.end(), 0.0f);
    for (int idx : order) {
        flow_acc[idx] += precip[idx];
        int d = downstream[idx];
        if (d != -1) {
            flow_acc[d] += flow_acc[idx];
            float vol = flow_acc[idx];
            mom_track_x[d] += vol * g.flow_dir_x[idx];
            mom_track_y[d] += vol * g.flow_dir_y[idx];
        }
    }

    // 5. discharge и импульс (объединены для улучшения локальности)
    const float lrate = 0.5f;
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        g.discharge[i] = std::erff(0.4f * flow_acc[i]);
        g.momentum_x[i] = g.momentum_x[i] * (1.0f - lrate) + mom_track_x[i] * lrate;
        g.momentum_y[i] = g.momentum_y[i] * (1.0f - lrate) + mom_track_y[i] * lrate;
    }

    // 6. Эрозия и осаждение (без изменений, с атомарным обновлением высоты для безопасности)
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        int j = downstream[i];
        if (j == -1) continue;
        float slope = (g.height[i] - g.height[j]) / p.dx;
        float discharge = g.discharge[i];
        float erode_potential = K * discharge * slope;
        float max_erode = (g.height[i] > 1.0f) ? (g.height[i] - 1.0f) : 0.0f;
        float actual_erode = (erode_potential < max_erode) ? erode_potential : max_erode;
        g.height[i] -= actual_erode;
        float deposit = actual_erode * 0.5f;
        // g.height[j] может меняться несколькими потоками, нужна атомарность
        #pragma omp atomic
        g.height[j] += deposit;
    }

    // 7. Термальная эрозия (оптимизирована: меньше ветвлений, отдельный цикл)
    update_water(g, p);
    apply_thermal_erosion(g, p, 60.0f * 3.14159265f / 180.0f);
}

void update_water(Grids& g, const SimParams& p) {
    const int W = p.W, H = p.H;
    const int N = W * H;

    // Параметры
    const float evaporation_rate = 0.01f;  // доля испаряемой воды за шаг
    const float max_water_per_cell = 1000.0f; // максимальный слой воды до стока (м)

    auto& depth = g.water_depth;
    auto& track = g.water_track;
    auto& order = g.order;
    auto& downstream = g.downstream;

    // 1. Добавляем осадки к воде (осадки уже в precip)
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        depth[i] += g.precip[i]; // precip уже заполнен
    }

    // 2. Сток: проходим от верхних клеток к нижним (order) и переливаем избыток
    std::fill(track.begin(), track.end(), 0.0f);
    for (int idx : order) {
        float w = depth[idx] + track[idx]; // вода, включая пришедшую сверху
        if (w > max_water_per_cell && downstream[idx] != -1) {
            float excess = w - max_water_per_cell;
            depth[idx] = max_water_per_cell;
            track[downstream[idx]] += excess;
        } else {
            depth[idx] = w;
            // избыток остаётся на месте (если нет стока, будет озеро)
            if (w > max_water_per_cell) {
                // нет downstream, вода остаётся (озеро)
                depth[idx] = w;
            }
        }
    }

    // 3. Испарение
    #pragma omp parallel for
    for (int i = 0; i < N; ++i) {
        depth[i] *= (1.0f - evaporation_rate);
        if (depth[i] < 0.001f) depth[i] = 0.0f; // обнуляем следы
    }
}

// Оптимизированная термальная эрозия
void apply_thermal_erosion(Grids& g, const SimParams& p, float repose_angle) {
    const float tan_phi = std::tan(repose_angle);
    const float threshold = tan_phi * p.dx;
    const int W = p.W, H = p.H;

    #pragma omp parallel for collapse(2)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            int i = idx(x, y, W);
            float h = g.height[i];
            float max_diff = 0.0f;
            int best_j = -1;

            // Обработка 4 соседей без массива констант (быстрее)
            if (x < W-1) {
                int j = idx(x+1, y, W);
                float diff = h - g.height[j];
                if (diff > max_diff) { max_diff = diff; best_j = j; }
            } else { // wrap по X для x+1
                int j = idx(0, y, W);
                float diff = h - g.height[j];
                if (diff > max_diff) { max_diff = diff; best_j = j; }
            }
            if (x > 0) {
                int j = idx(x-1, y, W);
                float diff = h - g.height[j];
                if (diff > max_diff) { max_diff = diff; best_j = j; }
            } else {
                int j = idx(W-1, y, W);
                float diff = h - g.height[j];
                if (diff > max_diff) { max_diff = diff; best_j = j; }
            }
            if (y < H-1) {
                int j = idx(x, y+1, W);
                float diff = h - g.height[j];
                if (diff > max_diff) { max_diff = diff; best_j = j; }
            }
            if (y > 0) {
                int j = idx(x, y-1, W);
                float diff = h - g.height[j];
                if (diff > max_diff) { max_diff = diff; best_j = j; }
            }

            if (best_j != -1 && max_diff > threshold) {
                float excess = (max_diff - threshold) * 0.5f;
                g.height[i] -= excess;
                // атомарное добавление (т.к. несколько клеток могут сбрасывать в одну)
                #pragma omp atomic
                g.height[best_j] += excess;
            }
        }
    }
}

void apply_slope_diffusion(Grids& g, const SimParams& p, int iterations) {
    const int W = p.W, H = p.H;
    for (int iter = 0; iter < iterations; ++iter) {
        #pragma omp parallel for collapse(2)
        for (int y = 1; y < H - 1; ++y) {
            for (int x = 0; x < W; ++x) {
                int i = idx(x, y, W);
                int xm = (x - 1 + W) % W;
                int xp = (x + 1) % W;
                float sum = g.height[idx(xm, y, W)] + g.height[idx(xp, y, W)]
                          + g.height[idx(x, y-1, W)] + g.height[idx(x, y+1, W)]
                          - 4.0f * g.height[i];
                g.height[i] += p.creep_coeff * sum;
            }
        }
    }
}