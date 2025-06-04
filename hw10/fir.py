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

def design_fir_lowpass(N, cutoff_hz, Fs, window_name="hamming"):
    n = np.arange(N)
    M = (N - 1) / 2.0   

    fc = cutoff_hz
    h_ideal = (2.0 * fc / Fs) * np.sinc((2.0 * fc / Fs) * (n - M))

    w_name = window_name.lower()
    if w_name == "hamming":
        win = np.hamming(N)
    elif w_name == "hann" or w_name == "hanning":
        win = np.hanning(N)
    elif w_name == "blackman":
        win = np.blackman(N)
    elif w_name == "bartlett" or w_name == "triangular":
        win = np.bartlett(N)

    h = h_ideal * win

    h = h / np.sum(h)

    return h

def apply_fir_filter(y, h):
    y_filt = np.convolve(y, h, mode="same")
    return y_filt

def compute_fft(y, Fs):
    n = len(y)
    Y = np.fft.fft(y) / n
    Y = Y[: n // 2]
    k = np.arange(n // 2)
    T = n / Fs
    frq = k / T
    return frq, np.abs(Y)

def plot_time_and_fft(t, y, y_filt, Fs, filename, filter_info):
    frq_orig, Y_orig = compute_fft(y, Fs)
    frq_filt, Y_filt = compute_fft(y_filt, Fs)

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 6))

    ax1.plot(t, y,   'k', label="Original")
    ax1.plot(t, y_filt, 'r', label="Filtered")
    ax1.set_xlabel("Time (s)")
    ax1.set_ylabel("Amplitude")
    ax1.set_title(f"{filename}\n{filter_info}")
    ax1.legend(loc="upper right")

    ax2.loglog(frq_orig, Y_orig,   'k', label="Original FFT")
    ax2.loglog(frq_filt, np.abs(Y_filt), 'r', label="Filtered FFT")
    ax2.set_xlabel("Frequency (Hz)")
    ax2.set_ylabel("|Y(f)|")
    ax2.set_title("FFT: Original vs. Filtered")
    ax2.legend(loc="upper right")

    plt.tight_layout()
    plt.show()

def main():
    csv_files = glob.glob("*.csv")
    if not csv_files:
        print("No CSV files found. Make sure each .csv has two columns: time,signal.")
        return

    fir_configs = [
        {"N":  300, "cutoff":  100, "window": "hamming"},
    ]

    for fn in csv_files:
        t, y, Fs = load_csv(fn)

        for cfg in fir_configs:
            N       = cfg["N"]
            cutoff  = cfg["cutoff"]
            window  = cfg["window"]

            h = design_fir_lowpass(N=N, cutoff_hz=cutoff, Fs=Fs, window_name=window)

            y_fir = apply_fir_filter(y, h)

            filter_info = f"FIR N={N}, window={window}, cutoff={cutoff} Hz"

            plot_time_and_fft(t, y, y_fir, Fs, fn, filter_info)

if __name__ == "__main__":
    main()
