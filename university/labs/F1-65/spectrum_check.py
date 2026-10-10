# F1-65: independent cross-check of Listing 3 with numpy's FFT.
# Reads the same jumpy_imu.csv and prints the four largest peaks of each column.
import sys

import numpy as np

data = np.loadtxt(sys.argv[1], delimiter=",", skiprows=1)
t, columns = data[:, 0], {"motors off": data[:, 1], "motors on": data[:, 2]}
fs = 1.0 / (t[1] - t[0])
n = len(t)
freqs = np.fft.rfftfreq(n, d=1.0 / fs)
print(f"numpy {np.__version__}: {n} samples, fs = {fs:.1f} Hz")
for name, x in columns.items():
    amp = 2.0 * np.abs(np.fft.rfft(x - x.mean())) / n
    amp[0] = 0.0
    amp = amp[: n // 2]
    top = np.argsort(amp)[::-1][:4]
    print(f"== {name}")
    for k in top:
        print(f"  {freqs[k]:6.1f} Hz  amplitude {amp[k]:.4f} g")
