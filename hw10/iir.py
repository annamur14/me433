import numpy as np
import matplotlib.pyplot as plt
import glob

def load_csv(filename):
    data = np.loadtxt(filename, delimiter=",")
    t = data[:, 0]
    y = data[:, 1]

    dt = t[1] - t[0]
    Fs = 1.0 / dt
    return t, y, Fs

def compute_fft(y, Fs):
    n = len(y)
    Y = np.fft.fft(y) / n
    Y = Y[: n // 2]                  
    k = np.arange(n // 2)
    T = n / Fs
    frq = k / T
    return frq, np.abs(Y)

def iir_filter(y, A, B):
    y_filt = np.zeros_like(y)
    y_filt[0] = y[0]  
    for i in range(1, len(y)):
        y_filt[i] = A * y_filt[i-1] + B * y[i]
    return y_filt

def high_freq_energy(frq, Y_mag, cutoff_frac=0.1):
    nyquist = frq[-1] 
    cutoff = cutoff_frac * nyquist
    mask = frq > cutoff
    return np.sum(Y_mag[mask])

def find_best_A(y, Fs, A_list, cutoff_frac=0.1):
    best_metric = np.inf
    best_A = None
    best_B = None
    best_filtered = None

    for A in A_list:
        B = 1.0 - A
        y_filt = iir_filter(y, A, B)
        frq_filt, Yf_mag = compute_fft(y_filt, Fs)
        metric = high_freq_energy(frq_filt, Yf_mag, cutoff_frac=cutoff_frac)
        if metric < best_metric:
            best_metric = metric
            best_A = A
            best_B = B
            best_filtered = y_filt

    return best_A, best_B, best_filtered, best_metric

def process_and_plot(filename, A_list, cutoff_frac=0.1):

    t, y, Fs = load_csv(filename)

    best_A, best_B, y_filt, metric = find_best_A(y, Fs, A_list, cutoff_frac=cutoff_frac)
    print(f"{filename}: best A = {best_A:.4f}, B = {best_B:.4f}, high‐freq‐energy = {metric:.3e}")

    frq_orig, Y_orig = compute_fft(y, Fs)
    frq_filt, Y_filt = compute_fft(y_filt, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 8))

    ax1.plot(t, y, 'k', label='Original')
    ax1.plot(t, y_filt, 'r', label=f'IIR Filtered (A={best_A:.3f}, B={best_B:.3f})')
    ax1.set_xlabel('Time (s)')
    ax1.set_ylabel('Amplitude')
    ax1.set_title(f"{filename} – Time Domain (A={best_A:.3f}, B={best_B:.3f})")
    ax1.legend()

    ax2.loglog(frq_orig, Y_orig, 'k', label='Original')
    ax2.loglog(frq_filt, Y_filt, 'r', label=f'Filtered (A={best_A:.3f}, B={best_B:.3f})')
    ax2.set_xlabel('Frequency (Hz)')
    ax2.set_ylabel('|Y(f)|')
    ax2.set_title(f"{filename} – FFT: Original vs IIR (A={best_A:.3f}, B={best_B:.3f})")
    ax2.legend()

    plt.tight_layout()
    plt.show()

def main():
    csv_files = glob.glob("*.csv")  
    if not csv_files:
        print("No CSV files found in current directory.")
        return

    A_list = np.linspace(0.90, 0.999, 20)  

    cutoff_frac = 0.10

    for fn in csv_files:
        process_and_plot(fn, A_list, cutoff_frac=cutoff_frac)

if __name__ == "__main__":
    main()
