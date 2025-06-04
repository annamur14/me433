import matplotlib.pyplot as plt
import numpy as np

data = np.loadtxt("sigD.csv", delimiter=",")
t = data[:, 0]
y = data[:, 1]

dt = t[1] - t[0]
Fs = 1.0 / dt

n = len(y)           
k = np.arange(n)
T = n / Fs
frq = k / T           
frq = frq[: n // 2]  

Y = np.fft.fft(y) / n
Y = Y[: n // 2]       

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))

ax1.plot(t, y, 'b')
ax1.set_xlabel('Time (s)')
ax1.set_ylabel('Amplitude')

ax2.loglog(frq, np.abs(Y), 'b')
ax2.set_xlabel('Freq (Hz)')
ax2.set_ylabel('|Y(freq)|')

plt.tight_layout()
plt.show()
