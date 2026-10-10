// astar_small.cpp - F9-54 worked example: A* by hand on a 7 x 5 grid, 4-connected,
// every step costs 1, heuristic = Manhattan distance. Prints every expansion.
// Ties in f are broken by the larger g (prefer nodes closer to the goal), then by order.
#include <array>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int main()
{
    const std::array<std::string, 5> rows = {   // row 0 is the top; '#' = wall
        ".......",
        "...#...",
        "S..#..G",
        "...#...",
        ".......",
    };
    const int w = 7;
    const int h = 5;
    const int sx = 0, sy = 2, gx = 6, gy = 2;
    auto heur = [&](int x, int y) { return std::abs(x - gx) + std::abs(y - gy); };
    std::vector<int> g(w * h, 1000);
    std::vector<int> parent(w * h, -1);
    std::vector<char> closed(w * h, 0);
    std::vector<int> open = {sy * w + sx};
    g[sy * w + sx] = 0;
    int step = 0;
    while (!open.empty()) {
        std::size_t best = 0;                     // pick the open node with the smallest f
        for (std::size_t k = 1; k < open.size(); ++k) {
            const int a = open[k], b = open[best];
            const int fa = g[a] + heur(a % w, a / w), fb = g[b] + heur(b % w, b / w);
            if (fa < fb || (fa == fb && g[a] > g[b])) { best = k; }
        }
        const int cur = open[best];
        open.erase(open.begin() + static_cast<long>(best));
        closed[cur] = 1;
        const int x = cur % w, y = cur / w;
        std::printf("step %2d: expand (%d,%d)  g=%d h=%d f=%d  open after:", ++step, x, y, g[cur],
                    heur(x, y), g[cur] + heur(x, y));
        if (x == gx && y == gy) { std::printf(" (goal reached)\n"); break; }
        const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
        for (int d = 0; d < 4; ++d) {
            const int nx = x + dx[d], ny = y + dy[d];
            if (nx < 0 || ny < 0 || nx >= w || ny >= h || rows[ny][nx] == '#') { continue; }
            const int n = ny * w + nx;
            if (closed[n] || g[cur] + 1 >= g[n]) { continue; }
            if (g[n] == 1000) { open.push_back(n); }
            g[n] = g[cur] + 1;
            parent[n] = cur;
        }
        for (int n : open) { std::printf(" (%d,%d)f=%d", n % w, n / w, g[n] + heur(n % w, n / w)); }
        std::printf("\n");
    }
    std::vector<std::string> out(rows.begin(), rows.end());
    int len = 0;
    for (int k = parent[gy * w + gx]; k != -1 && k != sy * w + sx; k = parent[k]) {
        out[k / w][k % w] = '*';
        ++len;
    }
    std::printf("path length %d steps; expanded %d of %d free cells\n", len + 1, step, w * h - 3);
    for (const std::string& r : out) { std::printf("  %s\n", r.c_str()); }
    return 0;
}
