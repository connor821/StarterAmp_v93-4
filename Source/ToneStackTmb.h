#pragma once
// Passive bass / mid / treble tone stacks solved from their component values.
//
//   source -> Rsrc -> X;   X -R4-> S;   X -C1-> T;   C2: S-M;   C3: S-W
//   treble pot R1: T-O (top part) and O-M (bottom part), output at O (the treble wiper)
//   bass pot  R2: M-N;   mid pot R3: N-W (top part) and W-GND (bottom part)
//   load Rload: O-GND
//
// The transfer function is a rational function of order 3 in s. Its numerator and denominator are found by solving the
// circuit at four real frequencies (in a scaled variable) and interpolating; then a bilinear transform gives a
// third-order IIR. Coefficients are rebuilt only when a control moves.
#include <cmath>
#include <algorithm>

namespace ToneStack
{
    struct Values { double R1, R2, R3, R4, C1, C2, C3, Rsrc, Rload; bool bassAudio, trebleAudio; };

    // 0 American combo, 1 British 50 W master-volume lead, 2 British lead 100 W, 3 American lead, 4 chime (class A), 5 and 6 high-gain two-channel stack (680 pF and 500 pF treble caps)
    inline Values valuesFor (int type)
    {
        switch (type)
        {
            case 0:  return { 250e3, 250e3, 10e3, 100e3, 250e-12, 0.1e-6, 0.047e-6, 38e3, 1.0e6, true, true };
            case 1:  return { 250e3, 1.0e6, 25e3, 33e3, 470e-12, 22e-9, 22e-9, 1.0e3, 1.0e6, true, false };
            case 2:  return { 250e3, 1.0e6, 25e3, 47e3, 470e-12, 20e-9, 20e-9, 1.0e3, 1.0e6, true, false };
            case 3:  return { 250e3, 250e3, 10e3, 100e3, 250e-12, 0.1e-6, 0.047e-6, 38e3, 470e3, true, true };
            case 5:  return { 220e3, 1.0e6, 22e3, 47.5e3, 680e-12, 20e-9, 20e-9, 1.0e3, 1.0e6, true, false };   // 220k treble, 22k mid, 680 pF
            case 6:  return { 220e3, 1.0e6, 22e3, 47.5e3, 500e-12, 20e-9, 20e-9, 1.0e3, 1.0e6, true, false };   // same with a 500 pF treble cap
            default: return { 1.0e6, 1.0e6, 10e3, 56e3, 50e-12, 22e-9, 22e-9, 5e3, 1.0e6, true, true };
        }
    }

    inline double taper (double k, bool audio)
    {
        k = std::min (1.0, std::max (0.0, k));
        return audio ? (std::pow (10.0, 2.0 * k) - 1.0) / 99.0 : k;
    }

    struct Poly3 { double c[4] = { 0, 0, 0, 0 }; };

    // Solves the 7-node circuit at the scaled real frequency sigma (s = w0 * sigma). Returns the output voltage and the determinant.
    inline void solveAt (const Values& v, double t, double b, double m, double sigma, double w0, double& vout, double& det)
    {
        const double eps = 1.0;                                  // ohms: keeps pots at their ends from short-circuiting the nodes
        const double r1a = std::max (eps, (1.0 - t) * v.R1), r1b = std::max (eps, t * v.R1);
        const double r2 = std::max (eps, b * v.R2);
        const double r3a = std::max (eps, (1.0 - m) * v.R3), r3b = std::max (eps, m * v.R3);
        const double s = w0 * sigma;
        enum { X, T, S, M, N, W, O, NN };
        double A[NN][NN + 1] = {};
        auto stamp = [&] (int a, int bnode, double y)       // admittance between two nodes (-1 = ground)
        {
            if (a >= 0) A[a][a] += y;
            if (bnode >= 0) A[bnode][bnode] += y;
            if (a >= 0 && bnode >= 0) { A[a][bnode] -= y; A[bnode][a] -= y; }
        };
        stamp (X, -1, 1.0 / v.Rsrc);                         // the source (1 V) behind Rsrc
        A[X][NN] += 1.0 / v.Rsrc;
        stamp (X, S, 1.0 / v.R4);
        stamp (X, T, s * v.C1);
        stamp (S, M, s * v.C2);
        stamp (S, W, s * v.C3);
        stamp (T, O, 1.0 / r1a);
        stamp (O, M, 1.0 / r1b);
        stamp (M, N, 1.0 / r2);
        stamp (N, W, 1.0 / r3a);
        stamp (W, -1, 1.0 / r3b);
        stamp (O, -1, 1.0 / v.Rload);
        // Gaussian elimination with the determinant
        double d = 1.0;
        for (int c = 0; c < NN; ++c)
        {
            int p = c;
            for (int r = c + 1; r < NN; ++r) if (std::fabs (A[r][c]) > std::fabs (A[p][c])) p = r;
            if (p != c) { for (int k = 0; k <= NN; ++k) std::swap (A[c][k], A[p][k]); d = -d; }
            const double piv = A[c][c];
            d *= piv;
            for (int r = c + 1; r < NN; ++r)
            {
                const double f = A[r][c] / piv;
                for (int k = c; k <= NN; ++k) A[r][k] -= f * A[c][k];
            }
        }
        double x[NN];
        for (int r = NN - 1; r >= 0; --r)
        {
            double sum = A[r][NN];
            for (int k = r + 1; k < NN; ++k) sum -= A[r][k] * x[k];
            x[r] = sum / A[r][r];
        }
        vout = x[O];
        det = d;
    }

    // Numerator and denominator in the scaled variable sigma (s = w0 sigma): H = N(sigma) / D(sigma), degree 3.
    inline void polynomials (const Values& v, double t, double b, double m, double w0, Poly3& N, Poly3& D)
    {
        const double sig[4] = { 0.0, 0.7, 1.6, 3.0 };
        double Dv[4], Nv[4];
        for (int i = 0; i < 4; ++i) { double vo, de; solveAt (v, t, b, m, sig[i], w0, vo, de); Dv[i] = de; Nv[i] = vo * de; }
        // Newton interpolation -> power basis
        auto fit = [&] (const double* y, Poly3& P)
        {
            double dd[4] = { y[0], y[1], y[2], y[3] };
            for (int j = 1; j < 4; ++j) for (int i = 3; i >= j; --i) dd[i] = (dd[i] - dd[i - 1]) / (sig[i] - sig[i - j]);
            // p(x) = dd0 + dd1 (x-s0) + dd2 (x-s0)(x-s1) + dd3 (x-s0)(x-s1)(x-s2)
            double c[4] = { dd[3], 0, 0, 0 };                   // Horner in power basis, building up
            double cur[4] = { dd[3], 0, 0, 0 };
            for (int k = 2; k >= 0; --k)
            {
                // cur = cur * (x - sig[k]) + dd[k]
                double nxt[4] = { 0, 0, 0, 0 };
                for (int i = 0; i < 3; ++i) { nxt[i + 1] += cur[i]; nxt[i] -= sig[k] * cur[i]; }
                nxt[0] += dd[k];
                for (int i = 0; i < 4; ++i) cur[i] = nxt[i];
            }
            for (int i = 0; i < 4; ++i) P.c[i] = cur[i]; (void) c;
        };
        fit (Nv, N);  fit (Dv, D);
    }

    struct Digital { double b[4], a[4]; };     // a[0] = 1

    // Bilinear transform of a cubic in sigma, with sigma = K (1 - z^-1) / (1 + z^-1), K = 2 fs / w0
    inline Digital bilinear (const Poly3& N, const Poly3& D, double K)
    {
        auto conv = [&] (const Poly3& P, double* out)
        {
            // sum_i c_i K^i (1-z)^i (1+z)^(3-i), with z standing for z^-1
            static const double basis[4][4] = { { 1, 3, 3, 1 }, { 1, 1, -1, -1 }, { 1, -1, -1, 1 }, { 1, -3, 3, -1 } };
            for (int j = 0; j < 4; ++j) out[j] = 0;
            double Kp = 1.0;
            for (int i = 0; i < 4; ++i) { for (int j = 0; j < 4; ++j) out[j] += P.c[i] * Kp * basis[i][j]; Kp *= K; }
        };
        double nb[4], db[4];
        conv (N, nb);  conv (D, db);
        Digital d;
        const double inv = 1.0 / db[0];
        for (int j = 0; j < 4; ++j) { d.b[j] = nb[j] * inv; d.a[j] = db[j] * inv; }
        return d;
    }

    inline double magnitudeDb (const Digital& d, double f, double fs)
    {
        const double w = 2.0 * 3.14159265358979 * f / fs;
        double nr = 0, ni = 0, dr = 0, di = 0;
        for (int j = 0; j < 4; ++j) { nr += d.b[j] * std::cos (j * w); ni -= d.b[j] * std::sin (j * w); dr += d.a[j] * std::cos (j * w); di -= d.a[j] * std::sin (j * w); }
        return 10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1e-30);
    }
}
