import numpy as np
import matplotlib.pyplot as plt

data = np.loadtxt("sigD.csv", delimiter=",")
t    = data[:, 0]
y    = data[:, 1]
Fs   = 1.0 / (t[1] - t[0])   

X = 200  

filtered = []
for i in range(len(y) - X + 1):
    window_avg = np.mean(y[i : i + X])
    filtered.append(window_avg)
filtered = np.array(filtered)

t_f = t[X - 1 :]

n   = len(y)
k   = np.arange(n)
T   = n / Fs
frq = k / T
frq = frq[: n // 2]

Y = np.fft.fft(y) / n
Y = Y[: n // 2]

n_f   = len(filtered)
k_f   = np.arange(n_f)
T_f   = n_f / Fs
frq_f = k_f / T_f
frq_f = frq_f[: n_f // 2]

Y_f = np.fft.fft(filtered) / n_f
Y_f = Y_f[: n_f // 2]

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 8))

ax1.plot(t, y, 'k', label='Original')
ax1.plot(t_f, filtered, 'r', label=f'MA Filtered (X={X})')
ax1.set_xlabel('Time (s)')
ax1.set_ylabel('Amplitude')
ax1.set_title(f'Time‐Domain: Moving Average Filter (X = {X})')
ax1.legend()

ax2.loglog(frq,   np.abs(Y),   'k', label='Original')
ax2.loglog(frq_f, np.abs(Y_f), 'r', label=f'Filtered (X={X})')
ax2.set_xlabel('Frequency (Hz)')
ax2.set_ylabel('|Y(f)|')
ax2.set_title(f'FFT: Original vs Filtered (X = {X})')
ax2.legend()

plt.tight_layout()
plt.show()
