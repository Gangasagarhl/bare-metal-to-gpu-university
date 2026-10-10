# F10-09 cross-check of vib_report.cpp with numpy: vibration level per axis, samples at
# the +-16 g limit, and the two strongest z frequencies, for flightA.csv and flightB.csv.
import sys
import numpy as np

for name in sys.argv[1:]:
    d = np.loadtxt(name, delimiter=",", skiprows=1)
    t, acc = d[:, 0], d[:, 1:]
    fs = (len(t) - 1) / (t[-1] - t[0])
    print(f"{name}: numpy {np.__version__}, fs = {fs:.0f} Hz")
    for k, axis in enumerate("xyz"):
        a = acc[:, k]
        print(f"  {axis}: mean {a.mean():+.3f} g, vibe {a.std():.3f} g, "
              f"at limit {int((np.abs(a) >= 16.0).sum())}")
    z = acc[:, 2] - acc[:, 2].mean()
    amp = 2 * np.abs(np.fft.rfft(z)) / len(z)
    freqs = np.fft.rfftfreq(len(z), 1 / fs)
    amp[0] = 0
    for p in range(2):
        k = int(np.argmax(amp))
        print(f"  z peak {p + 1}: {freqs[k]:.1f} Hz {amp[k]:.3f} g")
        amp[max(0, k - 3):k + 4] = 0
