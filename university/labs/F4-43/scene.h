// scene.h - DR405 F4-43: the pictures the lab driver shows. Pure integer code without any
// library, so the same file is compiled into the freestanding kernel and into the host
// program that renders the reference images (refimg.cc).
#pragma once
#include <stdint.h>

namespace scene {
struct Fb { uint32_t* px; int w, h; };            // pixel value 0x00RRGGBB; memory: B, G, R, X
struct Win { int x, y, w, h; uint32_t body, title; };
struct Box { int x0, y0, x1, y1; };               // half-open: x0 <= x < x1

inline void fill(Fb& f, int x, int y, int w, int h, uint32_t c)
{
    const int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    const int x1 = x + w > f.w ? f.w : x + w, y1 = y + h > f.h ? f.h : y + h;
    for (int j = y0; j < y1; ++j)
        for (int i = x0; i < x1; ++i) f.px[j * f.w + i] = c;
}

inline void background(Fb& f)                      // 32-pixel tiles in two blues
{
    for (int j = 0; j < f.h; ++j)
        for (int i = 0; i < f.w; ++i) f.px[j * f.w + i] = (((i >> 5) + (j >> 5)) & 1) ? 0x203a70u : 0x284890u;
}

inline void window(Fb& f, const Win& w)            // 2-pixel frame, 20-pixel title bar, body
{
    fill(f, w.x, w.y, w.w, w.h, 0x101010u);
    fill(f, w.x + 2, w.y + 2, w.w - 4, 18, w.title);
    fill(f, w.x + 2, w.y + 22, w.w - 4, w.h - 24, w.body);
}

// The whole compositor: background, then the windows from bottom (index 0) to top.
inline void compose(Fb& f, const Win* const* order, int n)
{
    background(f);
    for (int k = 0; k < n; ++k) window(f, *order[k]);
}

inline Box bounds(const Win& w) { return Box{w.x, w.y, w.x + w.w, w.y + w.h}; }
inline Box unite(Box a, Box b)
{
    return Box{a.x0 < b.x0 ? a.x0 : b.x0, a.y0 < b.y0 ? a.y0 : b.y0, a.x1 > b.x1 ? a.x1 : b.x1,
               a.y1 > b.y1 ? a.y1 : b.y1};
}
inline Box clip(Box b, const Fb& f)
{
    return Box{b.x0 < 0 ? 0 : b.x0, b.y0 < 0 ? 0 : b.y0, b.x1 > f.w ? f.w : b.x1, b.y1 > f.h ? f.h : b.y1};
}

// The two windows of the lab and their positions in frames 1 and 2.
inline Win win_a() { return Win{40, 40, 300, 200, 0xe8e8e0u, 0x2f7f3fu}; }
inline Win win_b(int frame)
{
    return frame == 1 ? Win{200, 150, 320, 220, 0xf0e0c0u, 0x9f3f2fu} : Win{280, 210, 320, 220, 0xf0e0c0u, 0x9f3f2fu};
}

// Frame 1: A below B. Frame 2: B moved, and A raised on top (a click on A).
inline void frame(Fb& f, int n)
{
    const Win a = win_a(), b = win_b(n);
    const Win* o1[2] = {&a, &b};
    const Win* o2[2] = {&b, &a};
    compose(f, n == 1 ? o1 : o2, 2);
}

// The region that changes between frame 1 and frame 2 (what the driver re-sends).
inline Box damage_1_to_2() { return unite(unite(bounds(win_b(1)), bounds(win_b(2))), bounds(win_a())); }

// A 64 x 64 cursor image, B8G8R8A8: a white arrow with a black outline, transparent elsewhere.
inline void cursor(uint32_t* px)
{
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            uint32_t c = 0;                        // alpha 0: transparent
            if (y < 24 && x <= y / 2 + 1) c = (x == 0 || x == y / 2 + 1 || y == 23) ? 0xff000000u : 0xffffffffu;
            px[y * 64 + x] = c;
        }
}
}  // namespace scene
