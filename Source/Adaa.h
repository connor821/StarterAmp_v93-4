#pragma once
// b41: first-order antiderivative anti-aliasing (ADAA) for the amp's clipping stages. b45: second order as well (see the end of this comment).
//
// A clipper y = f(u) run sample by sample creates harmonics above the sampling rate that fold back down as inharmonic grit
// (aliasing). Instead of f(u[n]), ADAA returns the average of f over the step from u[n-1] to u[n], computed exactly from
// the antiderivative F: y[n] = (F(u[n]) - F(u[n-1])) / (u[n] - u[n-1]). It acts like a gentle half-sample smoothing of
// the curve and removes most of the folded energy at a given oversampling rate. All of it is double precision because
// the difference of two large F values is what carries the signal.
//
// b45, second order: the average over the step is replaced by a weighted average over the last two steps (a triangle instead of
// a box), computed exactly from the SECOND antiderivative F2: y[n] = 2 * F2[u[n], u[n-1], u[n-2]], the second divided difference.
// It removes roughly another 10 to 20 dB of the folded energy and costs one more table lookup per sample; the group delay goes from
// half a sample to one sample of the rate it runs at. Each history (Prev) has an 'order' (kOrder by default, 2); setting it to 1
// gives the b41 behaviour at that place. Wherever a clipper has no closed-form F2, the same triangle average is done numerically.
#include "StageCurves.h"
#include <cmath>
#include <vector>
#include <utility>

namespace Adaa
{
#ifndef ADAA_ORDER
 #define ADAA_ORDER 2                        // compile with -DADAA_ORDER=1 to get the b41 / b42 first-order anti-aliasing everywhere (for A / B measurements)
#endif
    constexpr int kOrder = ADAA_ORDER;       // the order every new history starts with

    struct Prev
    {
        double u = 0.0, F = 0.0;
        double key = 0.0;                    // b42: identifies the curve F belongs to; a change starts the history afresh (a stale F of another curve would give a spike)
        bool primed = false;
        // b45: second order. u2 is the sample before u; G and G2 are F2 at u and at u2; seen counts the samples since the last reset (0, 1, 2+)
        double u2 = 0.0, G = 0.0, G2 = 0.0;
        int seen = 0;
        int order = kOrder;
        void reset() { primed = false; seen = 0; }
        void setKey (double k) { if (k != key) { key = k; primed = false; seen = 0; } }
    };

    // Below this step size the divided difference is replaced by f at the midpoint (the limit), which avoids 0 / 0.
    constexpr double kSmallStep = 1.0e-5;

    // =================================================================================================================
    // b45: second order
    // =================================================================================================================
    constexpr double kSmallStep2 = 2.0e-3;   // below this spacing between two of the three samples, the divided difference is done from the limit

    // The second antiderivative of a curve whose first antiderivative and value are known on a uniform grid: each cell holds the
    // cubic Hermite interpolant of F1 (it matches F1 and f = F1' at both ends), whose integral is exact. Outside the grid the curve is
    // held at the end slope (a straight F1).
    struct Herm2
    {
        double x0 = 0.0, h = 1.0, inv = 1.0, xEnd = 0.0;
        double z1 = 0.0, z2 = 0.0;           // F1 and F2 at 0: F1c / F2c are the antiderivatives that are 0 at 0 (where the two halves of a curve meet)
        int n = 0;
        std::vector<double> c;               // 5 numbers per node: F1, f, c2, c3 (of the cell that starts here), F2
        template <class Fn, class FInt>
        void build (double xa, double hh, int nn, Fn fnc, FInt fint)
        {
            x0 = xa;  h = hh;  inv = 1.0 / hh;  n = nn;  xEnd = xa + hh * (double) (nn - 1);
            c.assign ((size_t) nn * 5, 0.0);
            for (int i = 0; i < nn; ++i) { c[(size_t) i * 5] = fint (xa + hh * (double) i);  c[(size_t) i * 5 + 1] = fnc (xa + hh * (double) i); }
            double G = 0.0;
            for (int i = 0; i + 1 < nn; ++i)
            {
                const double Fa = c[(size_t) i * 5], fa = c[(size_t) i * 5 + 1], Fb = c[(size_t) (i + 1) * 5], fb = c[(size_t) (i + 1) * 5 + 1];
                const double D = Fb - Fa;
                const double c2 = (3.0 * D / hh - 2.0 * fa - fb) / hh, c3 = (-2.0 * D / hh + fa + fb) / (hh * hh);
                c[(size_t) i * 5 + 2] = c2;  c[(size_t) i * 5 + 3] = c3;  c[(size_t) i * 5 + 4] = G;
                G += hh * (Fa + hh * (0.5 * fa + hh * (c2 / 3.0 + hh * c3 * 0.25)));
            }
            c[(size_t) (nn - 1) * 5 + 4] = G;
            z1 = (xa <= 0.0 && xEnd >= 0.0) ? F1 (0.0) : 0.0;  z2 = (xa <= 0.0 && xEnd >= 0.0) ? F2 (0.0) : 0.0;
        }
        inline double F1c (double u) const { return F1 (u) - z1; }
        inline double F2c (double u) const { return F2 (u) - z2 - z1 * u; }
        inline double F2 (double u) const
        {
            const double p = (u - x0) * inv;
            if (! (p > 0.0))             { const double* k = c.data();                              const double t = u - x0;   return k[4] + t * (k[0] + 0.5 * k[1] * t); }
            if (p >= (double) (n - 1))   { const double* k = c.data() + (size_t) (n - 1) * 5;       const double t = u - xEnd; return k[4] + t * (k[0] + 0.5 * k[1] * t); }
            const int i = (int) p;
            const double* k = c.data() + (size_t) i * 5;
            const double t = u - (x0 + h * (double) i);
            return k[4] + t * (k[0] + t * (0.5 * k[1] + t * (k[2] * (1.0 / 3.0) + t * k[3] * 0.25)));
        }
        inline double F1 (double u) const
        {
            const double p = (u - x0) * inv;
            if (! (p > 0.0))             { const double* k = c.data();                              return k[0] + k[1] * (u - x0); }
            if (p >= (double) (n - 1))   { const double* k = c.data() + (size_t) (n - 1) * 5;       return k[0] + k[1] * (u - xEnd); }
            const int i = (int) p;
            const double* k = c.data() + (size_t) i * 5;
            const double t = u - (x0 + h * (double) i);
            return k[0] + t * (k[1] + t * (k[2] + t * k[3]));
        }
        inline double f (double u) const
        {
            const double p = (u - x0) * inv;
            if (! (p > 0.0))             return c[1];
            if (p >= (double) (n - 1))   return c[(size_t) (n - 1) * 5 + 1];
            const int i = (int) p;
            const double* k = c.data() + (size_t) i * 5;
            const double t = u - (x0 + h * (double) i);
            return k[1] + t * (2.0 * k[2] + 3.0 * k[3] * t);
        }
    };

    // A curve as the three callables the second-order step needs: value, first and second antiderivative.
    template <class Fn, class F1Fn, class F2Fn>
    struct Fam
    {
        Fn fn;  F1Fn f1;  F2Fn f2;
        inline double f  (double u) const { return fn (u); }
        inline double F1 (double u) const { return f1 (u); }
        inline double F2 (double u) const { return f2 (u); }
    };
    template <class Fn, class F1Fn, class F2Fn>
    inline Fam<Fn, F1Fn, F2Fn> makeFam (Fn a, F1Fn b, F2Fn c) { return { a, b, c }; }
    struct HermFam
    {
        const Herm2* H;
        inline double f  (double u) const { return H->f (u); }
        inline double F1 (double u) const { return H->F1 (u); }
        inline double F2 (double u) const { return H->F2 (u); }
    };

    // 2 * F2[a, b, c] (the second divided difference, which is what the second-order step returns). 'Ga', 'Gb', 'Gc' are F2 at the three points.
    // Two of the points can be within kSmallStep2 of each other (it happens at every peak of the waveform: a == c) and then the difference is
    // taken in the limit, from F1 and f at their midpoint; all three close is just f.
    template <class Fm>
    inline double dd2 (double a, double b, double c, double Ga, double Gb, double Gc, const Fm& fm)
    {
        const double dab = std::abs (a - b), dbc = std::abs (b - c), dac = std::abs (a - c);
        const double span = std::max (dab, std::max (dbc, dac));
        if (span < kSmallStep2) return fm.f ((a + b + c) * (1.0 / 3.0));
        // the closest pair
        double p = a, q = b, r = c, Gr = Gc;
        double dmin = dab;
        if (dbc < dmin) { dmin = dbc;  p = b;  q = c;  r = a;  Gr = Ga; }
        if (dac < dmin) { dmin = dac;  p = a;  q = c;  r = b;  Gr = Gb; }
        if (dmin < kSmallStep2)
        {
            const double m = 0.5 * (p + q), dr = m - r;
            return 2.0 * (fm.F1 (m) - (fm.F2 (m) - Gr) / dr) / dr;
        }
        return 2.0 * ((Ga - Gb) / (a - b) - (Gb - Gc) / (b - c)) / (a - c);
    }

    // One sample, second order, for a curve given as a family (f, F1, F2). The first two samples after a reset use the lower orders.
    template <class Fm>
    inline double step2 (Prev& p, double u, const Fm& fm)
    {
        double y;
        const double G0 = fm.F2 (u);
        if (p.seen == 0)       y = fm.f (u);
        else if (p.seen == 1)
        {
            if (std::abs (u - p.u) < kSmallStep) y = fm.f (0.5 * (u + p.u));
            else                                 y = (fm.F1 (u) - fm.F1 (p.u)) / (u - p.u);
        }
        else                   y = dd2 (u, p.u, p.u2, G0, p.G, p.G2, fm);
        p.u2 = p.u;  p.G2 = p.G;  p.u = u;  p.G = G0;
        if (p.seen < 2) ++p.seen;
        p.primed = true;
        return y;
    }

    // A clipper with no second antiderivative at hand: the triangle average itself, by Gauss-Legendre on each side of the middle knot.
    template <class Fn>
    inline double meanStep2 (Prev& p, double u, Fn f)
    {
        double y;
        if (p.seen == 0) y = f (u);
        else if (p.seen == 1)
        {
            // first order (box average) for the second sample
            const double du = u - p.u;
            if (std::abs (du) < kSmallStep) y = f (0.5 * (u + p.u));
            else
            {
                static const double x5[5] = { 0.0, -0.5384693101056831, 0.5384693101056831, -0.9061798459386640, 0.9061798459386640 };
                static const double w5[5] = { 0.5688888888888889, 0.4786286704993665, 0.4786286704993665, 0.2369268850561891, 0.2369268850561891 };
                const double mid = 0.5 * (u + p.u), half = 0.5 * du;
                double acc = 0.0;
                for (int i = 0; i < 5; ++i) acc += w5[i] * f (mid + half * x5[i]);
                y = 0.5 * acc;
            }
        }
        else
        {
            // the weight is the tent on the three samples (u, p.u, p.u2), peak at the middle one in value, area 1
            double s0 = u, s1 = p.u, s2 = p.u2;
            if (s0 > s1) std::swap (s0, s1);
            if (s1 > s2) std::swap (s1, s2);
            if (s0 > s1) std::swap (s0, s1);
            const double span = s2 - s0;
            if (span < kSmallStep2) y = f ((u + p.u + p.u2) * (1.0 / 3.0));
            else
            {
                static const double x5[5] = { 0.0, -0.5384693101056831, 0.5384693101056831, -0.9061798459386640, 0.9061798459386640 };
                static const double w5[5] = { 0.5688888888888889, 0.4786286704993665, 0.4786286704993665, 0.2369268850561891, 0.2369268850561891 };
                const double peak = 2.0 / span;
                double acc = 0.0;
                if (s1 - s0 > 1.0e-12)           // rising side: weight peak * (t - s0) / (s1 - s0)
                {
                    const double mid = 0.5 * (s0 + s1), half = 0.5 * (s1 - s0);
                    for (int i = 0; i < 5; ++i) { const double t = mid + half * x5[i]; acc += w5[i] * half * f (t) * peak * (t - s0) / (s1 - s0); }
                }
                if (s2 - s1 > 1.0e-12)           // falling side
                {
                    const double mid = 0.5 * (s1 + s2), half = 0.5 * (s2 - s1);
                    for (int i = 0; i < 5; ++i) { const double t = mid + half * x5[i]; acc += w5[i] * half * f (t) * peak * (s2 - t) / (s2 - s1); }
                }
                y = acc;
            }
        }
        p.u2 = p.u;  p.u = u;  if (p.seen < 2) ++p.seen;
        p.primed = true;
        return y;
    }

    // ---- tanh: F = log(cosh(u)) -------------------------------------------------------------------------------
    inline double logcosh (double x)
    {
        x = std::abs (x);
        return x + std::log1p (std::exp (-2.0 * x)) - 0.69314718055994531;
    }
    // F2 of tanh (no closed form: the integral of log cosh is a dilogarithm), from the Hermite table. Range +-20 (tanh is 1 to 1e-17 there; beyond it F1 is a straight line).
    inline const Herm2& tanhH()
    {
        static const Herm2 t = [] { Herm2 h;  h.build (-20.0, 1.0 / 64.0, 2561, [] (double u) { return std::tanh (u); }, [] (double u) { return logcosh (u); });  return h; }();
        return t;
    }
    inline double tanhStep (Prev& p, double u)
    {
        if (p.order >= 2) return step2 (p, u, HermFam { &tanhH() });
        const double F = logcosh (u);
        double y;
        if (! p.primed)                      y = std::tanh (u);
        else if (std::abs (u - p.u) < kSmallStep) y = std::tanh (0.5 * (u + p.u));
        else                                 y = (F - p.F) / (u - p.u);
        p.u = u;  p.F = F;  p.primed = true;
        return y;
    }

    // ---- the circuit-derived stage curves (piecewise linear tables), integrated exactly ---------------------------
    struct CurveInt
    {
        const float* y = nullptr;
        std::vector<double> F;               // F[i] = integral of the table from its first node to node i
        std::vector<double> G;               // b45: G[i] = the second antiderivative (integral of F) from the first node to node i
    };

    inline CurveInt makeCurveInt (const float* y)
    {
        CurveInt c;  c.y = y;
        c.F.assign ((size_t) StageCurves::kN, 0.0);
        c.G.assign ((size_t) StageCurves::kN, 0.0);
        const double du = StageCurves::kDu;
        for (int i = 0; i + 1 < StageCurves::kN; ++i)
        {
            c.F[(size_t) i + 1] = c.F[(size_t) i] + 0.5 * du * ((double) y[i] + (double) y[i + 1]);
            c.G[(size_t) i + 1] = c.G[(size_t) i] + du * c.F[(size_t) i] + du * du * (0.5 * (double) y[i] + ((double) y[i + 1] - (double) y[i]) * (1.0 / 6.0));
        }
        return c;
    }

    // F(u) for the table's piecewise-linear curve; outside the table the curve is held flat (as StageShaper::lut does)
    inline double curveF (const CurveInt& c, double u)
    {
        const double u0 = StageCurves::kU0, du = StageCurves::kDu;
        const int N = StageCurves::kN;
        double p = (u - u0) / du;
        if (! (p > 0.0))             return (double) c.y[0] * (u - u0);   // b95: written so a NaN takes this branch instead of indexing the table with (int) NaN
        if (p >= (double) (N - 1))   return c.F[(size_t) (N - 1)] + (double) c.y[N - 1] * (u - (u0 + du * (double) (N - 1)));
        const int i = (int) p;
        const double t = p - (double) i, y0 = c.y[i], y1 = c.y[i + 1];
        return c.F[(size_t) i] + du * (y0 * t + 0.5 * (y1 - y0) * t * t);
    }
    inline double curveY (const CurveInt& c, double u)
    {
        const double u0 = StageCurves::kU0, du = StageCurves::kDu;
        const int N = StageCurves::kN;
        double p = (u - u0) / du;
        if (! (p > 0.0)) return c.y[0];   // b95: NaN-safe
        if (p >= (double) (N - 1)) return c.y[N - 1];
        const int i = (int) p;
        const double t = p - (double) i;
        return c.y[i] + t * ((double) c.y[i + 1] - c.y[i]);
    }
    // the exact second antiderivative of the piecewise-linear table (a piecewise cubic), held flat outside like curveF
    inline double curveF2 (const CurveInt& c, double u)
    {
        const double u0 = StageCurves::kU0, du = StageCurves::kDu;
        const int N = StageCurves::kN;
        const double p = (u - u0) / du;
        if (! (p > 0.0))             { const double t = u - u0;  return 0.5 * (double) c.y[0] * t * t; }   // b95: NaN-safe
        if (p >= (double) (N - 1))   { const double t = u - (u0 + du * (double) (N - 1));  return c.G[(size_t) (N - 1)] + t * (c.F[(size_t) (N - 1)] + 0.5 * (double) c.y[N - 1] * t); }
        const int i = (int) p;
        const double t = (p - (double) i) * du, y0 = c.y[i], y1 = c.y[i + 1];
        return c.G[(size_t) i] + t * (c.F[(size_t) i] + t * (0.5 * y0 + t * (y1 - y0) / (6.0 * du)));
    }
    struct CurveFam
    {
        const CurveInt* c;
        inline double f  (double u) const { return curveY (*c, u); }
        inline double F1 (double u) const { return curveF (*c, u); }
        inline double F2 (double u) const { return curveF2 (*c, u); }
    };
    inline double curveStep (const CurveInt& c, Prev& p, double u)
    {
        if (p.order >= 2) return step2 (p, u, CurveFam { &c });
        const double F = curveF (c, u);
        double y;
        if (! p.primed)                           y = curveY (c, u);
        else if (std::abs (u - p.u) < kSmallStep) y = curveY (c, 0.5 * (u + p.u));
        else                                      y = (F - p.F) / (u - p.u);
        p.u = u;  p.F = F;  p.primed = true;
        return y;
    }

    // ---- the smooth hard clips of PluginProcessor.h: f (u) = u / (1 + u^N)^(1/N), N = 2^M (4: diode / zener / rail stages, 8 and 16: the
    //      germanium Face's two stages): numeric antiderivative (Simpson on a fine table) -----------------------------------------------------
    template <int M>
    inline double sigFn (double u)
    {
        double p = u;
        for (int i = 0; i < M; ++i) p *= p;
        double d = 1.0 + p;
        for (int i = 0; i < M; ++i) d = std::sqrt (d);
        return u / d;
    }
    inline double hardFn (double u) { return sigFn<2> (u); }
    template <int M>
    struct SigInt
    {
        static constexpr double kR = M == 2 ? 64.0 : M == 3 ? 50.0 : 8.0, kH = 1.0 / 128.0;
        std::vector<double> F;               // F at -kR + k * kH
        double at0 = 0.0;                    // F (0): subtract it (see c) where two pieces of one curve meet at 0
        Herm2 H2;                            // b45: the Hermite table for the second antiderivative, on the same nodes
        SigInt()
        {
            const int n = (int) (2.0 * kR / kH) + 1;
            F.assign ((size_t) n, 0.0);
            for (int k = 0; k + 1 < n; ++k)
            {
                const double a = -kR + kH * k, b = a + kH, m = 0.5 * (a + b);
                F[(size_t) k + 1] = F[(size_t) k] + kH / 6.0 * (sigFn<M> (a) + 4.0 * sigFn<M> (m) + sigFn<M> (b));
            }
            at0 = (*this) (0.0);
            H2.build (-kR, kH, n, [] (double u) { return sigFn<M> (u); },
                      [this] (double u) { const int k = (int) std::lround ((u + kR) / kH);  return F[(size_t) k]; });
        }
        double c (double u) const { return (*this) (u) - at0; }       // the antiderivative that is 0 at 0
        double c2 (double u) const { return H2.F2c (u); }              // b45: and the second antiderivative that is 0 at 0 (with slope 0 there)
        double operator() (double u) const
        {
            if (u <= -kR) return F[0] + sigFn<M> (-kR) * (u + kR);
            if (u >= kR)  return F.back() + sigFn<M> (kR) * (u - kR);
            const double q = (u + kR) / kH;
            const int k = (int) q;
            const double a = -kR + kH * k, t = u - a, m = a + 0.5 * t;
            return F[(size_t) k] + t / 6.0 * (sigFn<M> (a) + 4.0 * sigFn<M> (m) + sigFn<M> (u));
        }
    };
    using HardInt = SigInt<2>;
    inline const SigInt<2>& sig4()  { static const SigInt<2> t; return t; }
    inline const SigInt<3>& sig8()  { static const SigInt<3> t; return t; }
    inline const SigInt<4>& sig16() { static const SigInt<4> t; return t; }
    inline void warmTables() { (void) sig4(); (void) sig8(); (void) sig16(); (void) tanhH(); }       // build them on the prepare thread, never on the audio thread

    inline double hardStep (const HardInt& H, Prev& p, double u)
    {
        if (p.order >= 2) return step2 (p, u, HermFam { &H.H2 });
        const double F = H (u);
        double y;
        if (! p.primed)                           y = hardFn (u);
        else if (std::abs (u - p.u) < kSmallStep) y = hardFn (0.5 * (u + p.u));
        else                                      y = (F - p.F) / (u - p.u);
        p.u = u;  p.F = F;  p.primed = true;
        return y;
    }

    // ---- any clipper with a closed-form antiderivative: f (u) and F (u) given as callables ------------------------------------------------------
    template <class Fn, class FInt>
    inline double step (Prev& p, double u, Fn f, FInt F)
    {
        const double Fu = F (u);
        double y;
        if (! p.primed)                           y = f (u);
        else if (std::abs (u - p.u) < kSmallStep) y = f (0.5 * (u + p.u));
        else                                      y = (Fu - p.F) / (u - p.u);
        p.u = u;  p.F = Fu;  p.primed = true;
        return y;
    }

    // b45: the same with the second antiderivative as well (F2' = F, F2'' = f); second order unless the history says first
    template <class Fn, class FInt, class F2Int>
    inline double step (Prev& p, double u, Fn f, FInt F, F2Int F2)
    {
        if (p.order >= 2) return step2 (p, u, makeFam (f, F, F2));
        return step (p, u, f, F);
    }

    // ---- a clipper with no closed-form antiderivative: the same idea with the average of f over the step done by 5-point Gauss-Legendre ----------
    template <class Fn>
    inline double meanStep (Prev& p, double u, Fn f)
    {
        if (p.order >= 2) return meanStep2 (p, u, f);
        double y;
        const double du = u - p.u;
        if (! p.primed)                  y = f (u);
        else if (std::abs (du) < kSmallStep) y = f (0.5 * (u + p.u));
        else
        {
            static const double x[5] = { 0.0, -0.5384693101056831, 0.5384693101056831, -0.9061798459386640, 0.9061798459386640 };
            static const double w[5] = { 0.5688888888888889, 0.4786286704993665, 0.4786286704993665, 0.2369268850561891, 0.2369268850561891 };
            const double mid = 0.5 * (u + p.u), half = 0.5 * du;
            double acc = 0.0;
            for (int i = 0; i < 5; ++i) acc += w[i] * f (mid + half * x[i]);
            y = 0.5 * acc;                 // weights sum to 2
        }
        p.u = u;  p.primed = true;
        return y;
    }

    // ---- the cold-bias dead zone of the power stage: Z(s) = s - dz * tanh(s / dz), F(s) = s^2 / 2 - dz^2 * log(cosh(s / dz)) -------
    inline double deadStep (Prev& p, double s, double dz)
    {
        if (p.order >= 2)
        {
            const double dz2 = dz * dz, dz3 = dz2 * dz;
            return step2 (p, s, makeFam ([dz] (double v) { return v - dz * std::tanh (v / dz); },
                                         [dz2, dz] (double v) { return 0.5 * v * v - dz2 * logcosh (v / dz); },
                                         [dz3, dz] (double v) { return v * v * v * (1.0 / 6.0) - dz3 * tanhH().F2c (v / dz); }));
        }
        const double F = 0.5 * s * s - dz * dz * logcosh (s / dz);
        double y;
        if (! p.primed)                           y = s - dz * std::tanh (s / dz);
        else if (std::abs (s - p.u) < kSmallStep) { const double m = 0.5 * (s + p.u); y = m - dz * std::tanh (m / dz); }
        else                                      y = (F - p.F) / (s - p.u);
        p.u = s;  p.F = F;  p.primed = true;
        return y;
    }
}
