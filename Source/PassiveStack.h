#pragma once
// A real passive bass / mid / treble tone stack (see ToneStackTmb.h) running as a third-order filter.
#include "ToneStackTmb.h"

class PassiveStack
{
public:
    // type 0..4 (see ToneStack::valuesFor); treble, bass, mid are pot positions 0..1; call once per block
    void set (int newType, float treble, float bass, float mid, double fs)
    {
        const bool first = ! ready || newType != type || fs != sampleRate;
        type = newType;  sampleRate = fs;
        if (first)
        {
            makeup = referenceMakeup (type, fs);
            ready = true;
            haveTarget = false;
        }
        double nb[7];
        compute (type, treble, bass, mid, fs, makeup, nb);
        for (int i = 0; i < 7; ++i) { prev[i] = haveTarget ? tgt[i] : nb[i]; tgt[i] = nb[i]; }
        haveTarget = true;
    }

    void process (float* x, int n)
    {
        if (! haveTarget || n <= 0) return;
        const double inv = 1.0 / (double) n;
        for (int i = 0; i < n; ++i)
        {
            const double f = (double) (i + 1) * inv;
            const double b0 = prev[0] + f * (tgt[0] - prev[0]), b1 = prev[1] + f * (tgt[1] - prev[1]),
                         b2 = prev[2] + f * (tgt[2] - prev[2]), b3 = prev[3] + f * (tgt[3] - prev[3]),
                         a1 = prev[4] + f * (tgt[4] - prev[4]), a2 = prev[5] + f * (tgt[5] - prev[5]), a3 = prev[6] + f * (tgt[6] - prev[6]);
            const double in = (double) x[i];
            const double y = b0 * in + z1;
            z1 = b1 * in - a1 * y + z2;
            z2 = b2 * in - a2 * y + z3;
            z3 = b3 * in - a3 * y;
            x[i] = std::isfinite (y) ? (float) y : 0.0f;
        }
        for (int i = 0; i < 7; ++i) prev[i] = tgt[i];
        if (! std::isfinite (z1 + z2 + z3) || std::fabs (z1) + std::fabs (z2) + std::fabs (z3) > 1.0e6) reset();
    }

    void reset() { z1 = z2 = z3 = 0.0; }

private:
    static void compute (int type, float treble, float bass, float mid, double fs, double makeup, double* out)
    {
        const auto v = ToneStack::valuesFor (type);
        const double w0 = 2.0 * 3.14159265358979 * 1000.0, K = 2.0 * fs / w0;
        ToneStack::Poly3 N, D;
        ToneStack::polynomials (v, ToneStack::taper (treble, v.trebleAudio), ToneStack::taper (bass, v.bassAudio), (double) mid, w0, N, D);
        const auto d = ToneStack::bilinear (N, D, K);
        for (int j = 0; j < 4; ++j) out[j] = d.b[j] * makeup;
        for (int j = 1; j < 4; ++j) out[3 + j] = d.a[j];
    }
    // Overall gain that puts the average level (100 Hz to 5 kHz) at zero dB with all three controls at the middle
    static double referenceMakeup (int type, double fs)
    {
        double c[7];
        compute (type, 0.5f, 0.5f, 0.5f, fs, 1.0, c);
        ToneStack::Digital d;
        for (int j = 0; j < 4; ++j) d.b[j] = c[j];
        d.a[0] = 1.0;  d.a[1] = c[4];  d.a[2] = c[5];  d.a[3] = c[6];
        double sum = 0.0;  int k = 0;
        for (double f = 100.0; f <= 5000.0; f *= 1.12) { sum += ToneStack::magnitudeDb (d, f, fs); ++k; }
        return std::pow (10.0, -(sum / (double) k) / 20.0);   // magnitudeDb is 20 log10 of the amplitude
    }

    int type = 0;  double sampleRate = 0.0, makeup = 1.0;
    bool ready = false, haveTarget = false;
    double prev[7] = {}, tgt[7] = {};
    double z1 = 0.0, z2 = 0.0, z3 = 0.0;
};

// The 5E3 tweed amp's single Tone control (v93), solved from the schematic: the first stage's plate (100k load in parallel with the tube's
// own resistance) feeds, through 0.1 uF, the 1M volume pot; the 1M tone pot hangs from the signal through 500 pF at its top and to ground through
// 4.7 nF at its bottom, and its wiper joins the volume pot's wiper at the next grid (about 100 pF of Miller capacitance there).
// Tone up = the wiper nearer the 500 pF end = brighter. Third order (three capacitors), solved like ToneStack and run as a cubic IIR.
class TweedTone
{
public:
    void set (float tone, double fs)
    {
        const bool first = ! ready || fs != sampleRate;
        sampleRate = fs;
        if (first) { makeup = referenceMakeup (fs);  ready = true;  haveTarget = false; }
        double nb[7];
        compute (tone, fs, makeup, nb);
        for (int i = 0; i < 7; ++i) { prev[i] = haveTarget ? tgt[i] : nb[i]; tgt[i] = nb[i]; }
        haveTarget = true;
    }
    void process (float* x, int n)
    {
        if (! haveTarget || n <= 0) return;
        const double inv = 1.0 / (double) n;
        for (int i = 0; i < n; ++i)
        {
            const double f = (double) (i + 1) * inv;
            const double b0 = prev[0] + f * (tgt[0] - prev[0]), b1 = prev[1] + f * (tgt[1] - prev[1]),
                         b2 = prev[2] + f * (tgt[2] - prev[2]), b3 = prev[3] + f * (tgt[3] - prev[3]),
                         a1 = prev[4] + f * (tgt[4] - prev[4]), a2 = prev[5] + f * (tgt[5] - prev[5]), a3 = prev[6] + f * (tgt[6] - prev[6]);
            const double in = (double) x[i];
            const double y = b0 * in + z1;
            z1 = b1 * in - a1 * y + z2;
            z2 = b2 * in - a2 * y + z3;
            z3 = b3 * in - a3 * y;
            x[i] = std::isfinite (y) ? (float) y : 0.0f;
        }
        for (int i = 0; i < 7; ++i) prev[i] = tgt[i];
        if (! std::isfinite (z1 + z2 + z3) || std::fabs (z1) + std::fabs (z2) + std::fabs (z3) > 1.0e6) reset();
    }
    void reset() { z1 = z2 = z3 = 0.0; }

    // the network at one scaled real frequency; returns the output (the volume wiper node) and the determinant
    static void solveAt (double tone, double sigma, double w0, double& vout, double& det)
    {
        const double rs = 1.0 / (1.0 / 100.0e3 + 1.0 / 25.0e3);        // 100k plate load with the 12AY7's own resistance (about 25k, estimated)
        const double volPos = 0.6;                                    // the volume pot: a fixed mid-high setting (the plugin's Gain is not this pot)
        const double f = (std::pow (10.0, 2.0 * volPos) - 1.0) / 99.0; // audio taper: the share of the pot below the wiper
        const double eps = 1.0;
        const double rTM = std::max (eps, (1.0 - tone) * 1.0e6), rMB = std::max (eps, tone * 1.0e6);
        const double rVa = std::max (eps, (1.0 - f) * 1.0e6), rVb = std::max (eps, f * 1.0e6);
        const double s = w0 * sigma;
        enum { A, T, B, M, NN };
        double Mx[NN][NN + 1] = {};
        auto stamp = [&] (int a, int b, double y)
        {
            if (a >= 0) Mx[a][a] += y;
            if (b >= 0) Mx[b][b] += y;
            if (a >= 0 && b >= 0) { Mx[a][b] -= y; Mx[b][a] -= y; }
        };
        stamp (A, -1, 1.0 / rs);  Mx[A][NN] += 1.0 / rs;
        stamp (A, T, s * 500.0e-12);
        stamp (T, M, 1.0 / rTM);
        stamp (M, B, 1.0 / rMB);
        stamp (B, -1, s * 4.7e-9);
        stamp (M, -1, s * 100.0e-12);
        stamp (A, M, 1.0 / rVa);
        stamp (M, -1, 1.0 / rVb);
        double d = 1.0;
        for (int c = 0; c < NN; ++c)
        {
            int p = c;
            for (int r = c + 1; r < NN; ++r) if (std::fabs (Mx[r][c]) > std::fabs (Mx[p][c])) p = r;
            if (p != c) { for (int k = 0; k <= NN; ++k) std::swap (Mx[c][k], Mx[p][k]); d = -d; }
            const double piv = Mx[c][c];
            d *= piv;
            for (int r = c + 1; r < NN; ++r) { const double fct = Mx[r][c] / piv; for (int k = c; k <= NN; ++k) Mx[r][k] -= fct * Mx[c][k]; }
        }
        double x[NN];
        for (int r = NN - 1; r >= 0; --r) { double sum = Mx[r][NN]; for (int k = r + 1; k < NN; ++k) sum -= Mx[r][k] * x[k]; x[r] = sum / Mx[r][r]; }
        vout = x[M];  det = d;
    }

private:
    static void polys (double tone, double w0, ToneStack::Poly3& N, ToneStack::Poly3& D)
    {
        const double sig[4] = { 0.0, 0.7, 1.6, 3.0 };
        double Dv[4], Nv[4];
        for (int i = 0; i < 4; ++i) { double vo, de; solveAt (tone, sig[i], w0, vo, de); Dv[i] = de; Nv[i] = vo * de; }
        auto fit = [&] (const double* y, ToneStack::Poly3& P)
        {
            double dd[4] = { y[0], y[1], y[2], y[3] };
            for (int j = 1; j < 4; ++j) for (int i = 3; i >= j; --i) dd[i] = (dd[i] - dd[i - 1]) / (sig[i] - sig[i - j]);
            double cur[4] = { dd[3], 0, 0, 0 };
            for (int k = 2; k >= 0; --k)
            {
                double nxt[4] = { 0, 0, 0, 0 };
                for (int i = 0; i < 3; ++i) { nxt[i + 1] += cur[i]; nxt[i] -= sig[k] * cur[i]; }
                nxt[0] += dd[k];
                for (int i = 0; i < 4; ++i) cur[i] = nxt[i];
            }
            for (int i = 0; i < 4; ++i) P.c[i] = cur[i];
        };
        fit (Nv, N);  fit (Dv, D);
    }
    static void compute (float tone, double fs, double makeup, double* out)
    {
        const double w0 = 2.0 * 3.14159265358979 * 1000.0, K = 2.0 * fs / w0;
        ToneStack::Poly3 N, D;
        polys ((double) std::min (1.0f, std::max (0.0f, tone)), w0, N, D);
        const auto d = ToneStack::bilinear (N, D, K);
        for (int j = 0; j < 4; ++j) out[j] = d.b[j] * makeup;
        for (int j = 1; j < 4; ++j) out[3 + j] = d.a[j];
    }
    static double referenceMakeup (double fs)
    {
        double c[7];
        compute (0.5f, fs, 1.0, c);
        ToneStack::Digital d;
        for (int j = 0; j < 4; ++j) d.b[j] = c[j];
        d.a[0] = 1.0;  d.a[1] = c[4];  d.a[2] = c[5];  d.a[3] = c[6];
        double sum = 0.0;  int k = 0;
        for (double f = 100.0; f <= 5000.0; f *= 1.12) { sum += ToneStack::magnitudeDb (d, f, fs); ++k; }
        return std::pow (10.0, -(sum / (double) k) / 20.0);
    }
    double sampleRate = 0.0, makeup = 1.0;
    bool ready = false, haveTarget = false;
    double prev[7] = {}, tgt[7] = {};
    double z1 = 0.0, z2 = 0.0, z3 = 0.0;
};

