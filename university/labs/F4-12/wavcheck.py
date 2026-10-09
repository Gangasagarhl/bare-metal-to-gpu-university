#!/usr/bin/env python3
"""wavcheck.py - DR302 F4-12: measures the WAV file QEMU's "wav" audio backend recorded.
Prints the header, the length, where the sound starts and stops, and for each channel the
strongest frequency (FFT with a Hann window, peak refined by parabolic interpolation) and
the level. With --expect L,R (Hz) it exits 1 when a channel is more than 1 % off, and it
also rebuilds the kernel's samples exactly as f412_main.cc computes them (same table,
same phase accumulator) and compares the recording with them sample by sample.
Usage: wavcheck.py out.wav [--expect 440,660]"""
import math, sys, wave
import numpy as np

def kernel_samples(f_left, f_right, rate=44100, frames=353280 // 4):
    """The PCM f412_main.cc's fill() writes: a 1024-entry table rounded from a 12-term Taylor
    series, a 32-bit phase accumulator, the top 10 bits as index, halved (truncating)."""
    def series(v):
        term = s = v
        for n in range(1, 12):
            term *= -v * v / ((2 * n) * (2 * n + 1))
            s += term
        return s
    tab = []
    for i in range(1024):
        v = 2 * math.pi * i / 1024
        v = v - 2 * math.pi if v > math.pi else v
        y = series(v) * 32767.0
        tab.append(int(y + 0.5) if y >= 0 else -int(-y + 0.5))
    tab = np.array(tab)
    n = np.arange(frames, dtype=np.uint64)
    out = []
    for f in (f_left, f_right):
        ph = (n * np.uint64((f << 32) // rate)) & np.uint64(0xFFFFFFFF)
        out.append(np.trunc(tab[(ph >> np.uint64(22)).astype(np.int64)] / 2).astype(np.int64))
    return np.stack(out, 1)

def main():
    path = sys.argv[1]
    expect = None
    if len(sys.argv) > 3 and sys.argv[2] == "--expect":
        expect = [float(x) for x in sys.argv[3].split(",")]
    with wave.open(path, "rb") as w:
        rate, ch, width, n = w.getframerate(), w.getnchannels(), w.getsampwidth(), w.getnframes()
        raw = w.readframes(n)
    print("wav: %d Hz, %d channel(s), %d-bit, %d frames = %.3f s" % (rate, ch, 8 * width, n, n / rate))
    if width != 2 or n == 0:
        print("wav: unexpected sample size or empty file"); return 1
    x = np.frombuffer(raw, dtype="<i2").reshape(-1, ch).astype(np.float64) / 32768.0
    loud = np.nonzero(np.abs(x).max(axis=1) > 0.01)[0]
    if len(loud) == 0:
        print("wav: silence only"); return 1
    a, b = loud[0], loud[-1]
    print("wav: sound from %.3f s to %.3f s (%.3f s of sound)" % (a / rate, b / rate, (b - a + 1) / rate))
    seg = x[a:b + 1]
    bad = 0
    for c in range(ch):
        s = seg[:, c] * np.hanning(len(seg))
        spec = np.abs(np.fft.rfft(s))
        k = int(np.argmax(spec[1:])) + 1
        if 1 <= k < len(spec) - 1:                      # parabolic interpolation of the peak
            y0, y1, y2 = np.log(spec[k - 1] + 1e-12), np.log(spec[k] + 1e-12), np.log(spec[k + 1] + 1e-12)
            k = k + 0.5 * (y0 - y2) / (y0 - 2 * y1 + y2)
        f = k * rate / len(s)
        rms = np.sqrt(np.mean(seg[:, c] ** 2))
        peak = np.abs(seg[:, c]).max()
        line = "wav: channel %d: strongest frequency %.1f Hz, RMS %.3f (%.1f dBFS), peak %.3f" % (
            c, f, rms, 20 * np.log10(rms + 1e-12), peak)
        if expect and c < len(expect):
            off = (f - expect[c]) / expect[c] * 100
            line += ", expected %.0f Hz: %+.1f %%" % (expect[c], off)
            if abs(off) > 1.0:
                bad += 1
                line += " WRONG"
        print(line)
    if expect and rate == 44100 and ch == 2:
        src = kernel_samples(int(expect[0]), int(expect[1]))
        rec = np.frombuffer(raw, dtype="<i2").reshape(-1, ch).astype(np.int64)
        best = None
        for off in range(0, 2000):                      # where does the source start in the file?
            if off + 4096 > len(rec):
                break
            d = int(np.abs(rec[off:off + 4096] - src[:4096]).max())
            if best is None or d < best[1]:
                best = (off, d)
        off = best[0]
        m = min(len(rec) - off, len(src))
        diff = np.abs(rec[off:off + m] - src[:m])
        same = int(np.count_nonzero(diff.max(axis=1) == 0))
        print("samples: recording from frame %d vs the kernel's PCM: %d of %d frames identical (largest difference %d); "
              "%d source frames not in the file" % (off, same, m, int(diff.max()), len(src) - m))
        if same != m:
            bad += 1
    if expect:
        print("wavcheck: %s" % ("PASS" if bad == 0 else "FAIL (%d check(s) failed)" % bad))
    return 1 if bad else 0

sys.exit(main())
