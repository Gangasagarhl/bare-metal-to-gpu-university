// fbconsole.h - F3-23: a text console drawn into a 32-bit-per-pixel framebuffer.
// Header-only and hardware-free: the kernel gives it the framebuffer's address; the host
// checker (fbcheck.cc) gives it an array and replays the same text to build the reference.
#pragma once
#include <cstdint>
#include "font5x7.h"

class FbConsole {
public:
    static constexpr int kCellW = 6;          // 5 pixels of glyph + 1 of space
    static constexpr int kCellH = 10;         // 7 pixels of glyph + 3 of space
    static constexpr uint32_t kBackground = 0x00101820;
    static constexpr uint32_t kText = 0x00d0d0d0;

    void init(uint32_t* pixels, int width, int height, int pitch_pixels)
    {
        px_ = pixels;
        w_ = width;
        h_ = height;
        pitch_ = pitch_pixels;
        cols_ = width / kCellW;
        rows_ = height / kCellH;
        fg_ = kText;
        bg_ = kBackground;
        clear(bg_);
    }
    void clear(uint32_t color)
    {
        bg_ = color;
        for (int y = 0; y < h_; ++y) {
            for (int x = 0; x < w_; ++x) {
                px_[y * pitch_ + x] = color;
            }
        }
        cx_ = cy_ = 0;
    }
    void set_colors(uint32_t fg, uint32_t bg)
    {
        fg_ = fg;
        bg_ = bg;
    }
    void put(char c)
    {
        if (c == '\n') {
            newline();
            return;
        }
        if (cx_ >= cols_) {
            newline();
        }
        draw(c, cx_, cy_);
        ++cx_;
    }
    int cols() const { return cols_; }
    int rows() const { return rows_; }

private:
    void draw(char c, int col, int row)
    {
        uint8_t bits[7];
        font::rows_of(c, bits);
        int x0 = col * kCellW;
        int y0 = row * kCellH;
        for (int y = 0; y < kCellH; ++y) {
            for (int x = 0; x < kCellW; ++x) {
                bool ink = y < 7 && x < 5 && ((bits[y] >> (4 - x)) & 1);
                px_[(y0 + y) * pitch_ + x0 + x] = ink ? fg_ : bg_;
            }
        }
    }
    void newline()
    {
        cx_ = 0;
        if (++cy_ < rows_) {
            return;
        }
        // scroll: move every text line up by one cell height, clear the last line
        for (int y = 0; y < (rows_ - 1) * kCellH; ++y) {
            for (int x = 0; x < w_; ++x) {
                px_[y * pitch_ + x] = px_[(y + kCellH) * pitch_ + x];
            }
        }
        for (int y = (rows_ - 1) * kCellH; y < rows_ * kCellH; ++y) {
            for (int x = 0; x < w_; ++x) {
                px_[y * pitch_ + x] = bg_;
            }
        }
        cy_ = rows_ - 1;
    }

    uint32_t* px_ = nullptr;
    int w_ = 0, h_ = 0, pitch_ = 0, cols_ = 0, rows_ = 0, cx_ = 0, cy_ = 0;
    uint32_t fg_ = 0, bg_ = 0;
};
