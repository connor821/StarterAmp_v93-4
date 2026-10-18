#pragma once
// TubeNet: a small nodal circuit solver for vacuum-tube stages.
//   Elements: resistors, capacitors and inductors (trapezoidal), diodes (grid current), triodes and pentodes (Koren equations,
//   tabulated). Every sample the node voltages are solved by damped Newton iteration (dense, in double precision).
//   Nodes are either unknown, forced (a prescribed voltage that the caller sets, such as a supply or the input), or ground.
#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace Tube
{
    // ---- vacuum-tube tables -------------------------------------------------------------------------------------------
    struct KorenTriode { double mu, ex, kg1, kp, kvb; };
    struct KorenPentode { double mu, ex, kg1, kg2, kp, kvb; };

    inline KorenTriode tube12AX7() { return { 100.0, 1.4, 1060.0, 600.0, 300.0 }; }
    inline KorenTriode tube12AT7() { return { 60.0, 1.35, 460.0, 300.0, 300.0 }; }      // approximate
    inline KorenTriode tube12AY7() { return { 44.0, 1.35, 780.0, 200.0, 300.0 }; }      // approximate
    inline KorenPentode tubeEL34() { return { 11.0, 1.35, 650.0, 4200.0, 60.0, 24.0 }; }
    inline KorenPentode tube6L6()  { return { 8.7, 1.35, 1460.0, 4500.0, 48.0, 12.0 }; }
    inline KorenPentode tubeKT88() { return { 8.0, 1.35, 720.0, 4500.0, 40.0, 20.0 }; }  // approximate
    inline KorenPentode tubeEL84() { return { 19.0, 1.35, 1250.0, 4500.0, 25.0, 25.0 }; } // approximate
    inline KorenPentode tube6V6()  { return { 10.0, 1.35, 1250.0, 4500.0, 35.0, 18.0 }; } // approximate

    // A two-dimensional table of plate current and its two slopes against grid-cathode and plate-cathode voltage.
    class TubeTable
    {
    public:
        double kVgStep = 1.0 / 32.0;
        double kVgMin = -16.0, kVgMax = 3.0;
        void setGridRange (double lo, double hi, double step = 1.0 / 32.0) { kVgMin = lo; kVgMax = hi; kVgStep = step; }
        static constexpr double kVpMax = 520.0, kVpStep = 2.0;

        void buildTriode (const KorenTriode& t)
        {
            build ([t] (double vg, double vp) { return triode (t, vg, vp); });
        }
        // The pentode with a given screen voltage (the screen is treated as a fixed supply per table)
        void buildPentode (const KorenPentode& t, double vg2)
        {
            build ([t, vg2] (double vg, double vp) { return pentode (t, vg, vp, vg2); });
        }
        static double triode (const KorenTriode& t, double vgk, double vpk)
        {
            vpk = std::max (vpk, 0.0);
            const double s = std::sqrt (t.kvb + vpk * vpk);
            const double x = t.kp * (1.0 / t.mu + vgk / s);
            const double e1 = (vpk / t.kp) * (x > 30.0 ? x : std::log1p (std::exp (x)));
            return e1 > 0.0 ? 2.0 * std::pow (e1, t.ex) / t.kg1 : 0.0;
        }
        static double pentode (const KorenPentode& t, double vgk, double vpk, double vg2k)
        {
            vpk = std::max (vpk, 0.0);
            const double x = t.kp * (1.0 / t.mu + vgk / vg2k);
            const double e1 = (vg2k / t.kp) * (x > 30.0 ? x : std::log1p (std::exp (x)));
            return e1 > 0.0 ? 2.0 * std::pow (e1, t.ex) / t.kg1 * std::atan (vpk / t.kvb) : 0.0;
        }

        // ip (A), gm (A/V) and gp (A/V)
        inline void eval (double vgk, double vpk, double& ip, double& gm, double& gp) const
        {
            double fg = (vgk - kVgMin) / kVgStep, fp = vpk / kVpStep;
            double extraG = 0.0;
            if (fg < 0.0) fg = 0.0;
            if (fg > nG - 1.001) { extraG = vgk - (kVgMin + (nG - 1.001) * kVgStep); fg = nG - 1.001; }
            if (fp < 0.0) fp = 0.0;
            if (fp > nP - 1.001) fp = nP - 1.001;
            const int i = (int) fg, j = (int) fp;
            const double a = fg - i, b = fp - j;
            const size_t k00 = (size_t) i * nP + (size_t) j, k01 = k00 + 1, k10 = k00 + nP, k11 = k10 + 1;
            auto bl = [&] (const std::vector<float>& v)
            { return (1 - a) * ((1 - b) * v[k00] + b * v[k01]) + a * ((1 - b) * v[k10] + b * v[k11]); };
            ip = bl (tIp);  gm = bl (tGm);  gp = bl (tGp);
            if (extraG != 0.0) ip += gm * extraG;
            if (vpk < 0.0) { ip += gp * vpk; }
        }

    private:
        template <class F> void build (F f)
        {
            nG = (int) std::lround ((kVgMax - kVgMin) / kVgStep) + 1;
            nP = (int) std::lround (kVpMax / kVpStep) + 1;
            tIp.assign ((size_t) nG * nP, 0.0f);  tGm = tIp;  tGp = tIp;
            const double hg = 1.0e-3, hp = 5.0e-2;
            for (int i = 0; i < nG; ++i)
                for (int j = 0; j < nP; ++j)
                {
                    const double vg = kVgMin + i * kVgStep, vp = j * kVpStep;
                    const size_t k = (size_t) i * nP + (size_t) j;
                    tIp[k] = (float) f (vg, vp);
                    tGm[k] = (float) ((f (vg + hg, vp) - f (vg - hg, vp)) / (2.0 * hg));
                    tGp[k] = (float) ((f (vg, vp + hp) - f (vg, std::max (0.0, vp - hp))) / (vp > 0.0 ? 2.0 * hp : hp));
                }
        }
        int nG = 0, nP = 0;
        std::vector<float> tIp, tGm, tGp;
    };
}

namespace Tube
{
    class Net
    {
    public:
        static constexpr int GND = -1;
        int node()   { return nUnk++; }
        int forced() { forcedVals.push_back (0.0); return -2 - ((int) forcedVals.size() - 1); }

        void R (int a, int b, double ohms)               { els.push_back ({ kR, a, b, -1, 1.0 / ohms, 0.0, nullptr }); }
        void C (int a, int b, double farads)             { els.push_back ({ kC, a, b, -1, farads, 0.0, nullptr }); }
        void L (int a, int b, double henries)            { els.push_back ({ kL, a, b, -1, henries, 0.0, nullptr }); }
        void diode (int a, int b, double isat, double vt){ els.push_back ({ kD, a, b, -1, isat, vt, nullptr }); }
        int  triode (int g, int p, int k, const TubeTable* t) { els.push_back ({ kT, g, p, k, 0.0, 0.0, t }); return (int) els.size() - 1; }
        void setTable (int idx, const TubeTable* t) { els[(size_t) idx].tab = t; }
        int  Rvar (int a, int b, double ohms)            { R (a, b, ohms); return (int) els.size() - 1; }
        void setR (int idx, double ohms)                 { const double g = 1.0 / ohms; if (std::fabs (g - els[(size_t) idx].val) > 1.0e-9 * g) { els[(size_t) idx].val = g; luValid = false; } }
        int  Cvar (int a, int b, double farads)          { C (a, b, farads); return (int) els.size() - 1; }
        void setC (int idx, double farads)               { els[(size_t) idx].val = farads; if (fs > 0) els[(size_t) idx].gc = 2.0 * farads * fs; }

        void setForced (int code, double v) { forcedVals[(size_t) (-2 - code)] = v; }
        double v (int code) const { return volt (code); }
        void setGuess (int node, double v) { if (node >= 0) x[(size_t) node] = v; }
        int numNodes() const { return nUnk; }

        void prepare (double sampleRate)
        {
            fs = sampleRate;
            const int n = nUnk;
            x.assign ((size_t) n, 0.0);  xPrev = x;
            F.assign ((size_t) n, 0.0);
            J.assign ((size_t) n * n, 0.0);  dx.assign ((size_t) n, 0.0);
            for (auto& e : els)
            {
                if (e.type == kC) e.gc = 2.0 * e.val * fs;
                if (e.type == kL) e.gc = 1.0 / (2.0 * e.val * fs);
            }
        }

        bool dcSolve()
        {
            std::vector<double> target = forcedVals;
            bool ok = true;
            dcMode = true;
            for (int step = 1; step <= 12; ++step)
            {
                const double s = (double) step / 12.0;
                for (size_t i = 0; i < target.size(); ++i) forcedVals[i] = target[i] * s;
                ok = newton (60);
            }
            forcedVals = target;
            ok = newton (100) && ok;
            dcMode = false;
            for (auto& e : els)
            {
                if (e.type == kC) { e.s1 = volt (e.a) - volt (e.b);  e.s2 = 0.0; }
                if (e.type == kL) { e.s1 = volt (e.a) - volt (e.b);  e.s2 = e.s1 * 1.0e3; }
            }
            xPrev = x;
            return ok;
        }

        bool solve (int maxIter = 12)
        {
            for (size_t i = 0; i < x.size(); ++i) { const double t = x[i]; x[i] = 2.0 * x[i] - xPrev[i]; xPrev[i] = t; }
            bool ok = newton (maxIter);
            if (! ok) { x = xPrev; ok = newton (40); }
            for (auto& e : els)
            {
                if (e.type == kC) { const double vv = volt (e.a) - volt (e.b); e.s2 = e.gc * vv - (e.gc * e.s1 + e.s2);  e.s1 = vv; }
                else if (e.type == kL) { const double vv = volt (e.a) - volt (e.b); e.s2 = e.gc * vv + (e.s2 + e.gc * e.s1);  e.s1 = vv; }
            }
            return ok;
        }
        void setTolerance (double t) { tolStep = t; }
        double current (int elIdx) const { return elCurrent (els[(size_t) elIdx]); }
        int lastIterations = 0;

    private:
        enum Type { kR, kC, kL, kD, kT };
        struct El { Type type; int a, b, c; double val, aux; const TubeTable* tab; double gc = 0.0, s1 = 0.0, s2 = 0.0; };
        std::vector<El> els;
        std::vector<double> x, xPrev, F, J, dx, forcedVals;
        int nUnk = 0;
        double fs = 0.0;
        bool dcMode = false;

        inline double volt (int code) const
        {
            if (code >= 0) return x[(size_t) code];
            if (code == GND) return 0.0;
            return forcedVals[(size_t) (-2 - code)];
        }
        double elCurrent (const El& e) const
        {
            const double va = volt (e.a), vb = volt (e.b);
            switch (e.type)
            {
                case kR: return (va - vb) * e.val;
                case kC: return dcMode ? 0.0 : e.gc * (va - vb) - (e.gc * e.s1 + e.s2);
                case kL: return dcMode ? (va - vb) * 1.0e3 : e.gc * (va - vb) + (e.s2 + e.gc * e.s1);
                case kD: return diodeI (e, va - vb);
                case kT: { double ip, gm, gp; e.tab->eval (va - volt (e.c), vb - volt (e.c), ip, gm, gp); return ip; }
            }
            return 0.0;
        }
        static double diodeI (const El& e, double vd)
        {
            const double lim = 0.9;
            if (vd > lim) { const double el = std::exp (lim / e.aux); return e.val * (el * (1.0 + (vd - lim) / e.aux) - 1.0); }
            return e.val * (std::exp (std::max (vd / e.aux, -40.0)) - 1.0);
        }

        inline void addF (int node, double i) { if (node >= 0) F[(size_t) node] += i; }
        inline void addJ (int r, int c, double g) { if (r >= 0 && c >= 0) J[(size_t) r * nUnk + (size_t) c] += g; }
        inline void stampTwo (int a, int b, double i, double g)
        {
            addF (a, i); addF (b, -i);
            addJ (a, a, g); addJ (b, b, g); addJ (a, b, -g); addJ (b, a, -g);
        }

        void assemble()
        {
            std::fill (F.begin(), F.end(), 0.0);
            std::fill (J.begin(), J.end(), 0.0);
            for (int i = 0; i < nUnk; ++i) { F[(size_t) i] += 1.0e-9 * x[(size_t) i]; J[(size_t) i * nUnk + (size_t) i] += 1.0e-9; }
            for (const auto& e : els)
            {
                const double va = volt (e.a), vb = volt (e.b);
                switch (e.type)
                {
                    case kR: stampTwo (e.a, e.b, (va - vb) * e.val, e.val); break;
                    case kC: if (! dcMode) stampTwo (e.a, e.b, e.gc * (va - vb) - (e.gc * e.s1 + e.s2), e.gc); break;
                    case kL: if (dcMode) stampTwo (e.a, e.b, (va - vb) * 1.0e3, 1.0e3);
                             else        stampTwo (e.a, e.b, e.gc * (va - vb) + (e.s2 + e.gc * e.s1), e.gc);
                             break;
                    case kD:
                    {
                        const double vd = va - vb, i = diodeI (e, vd);
                        double g;
                        if (vd > 0.9) g = e.val * std::exp (0.9 / e.aux) / e.aux;
                        else          g = e.val * std::exp (std::max (vd / e.aux, -40.0)) / e.aux;
                        stampTwo (e.a, e.b, i, g + 1.0e-12);
                        break;
                    }
                    case kT:
                    {
                        const double vk = volt (e.c);
                        double ip, gm, gp;
                        e.tab->eval (va - vk, vb - vk, ip, gm, gp);
                        addF (e.b, ip);  addF (e.c, -ip);
                        addJ (e.b, e.a, gm);  addJ (e.b, e.b, gp);  addJ (e.b, e.c, -(gm + gp));
                        addJ (e.c, e.a, -gm); addJ (e.c, e.b, -gp); addJ (e.c, e.c, gm + gp);
                        break;
                    }
                }
            }
        }

        std::vector<double> LU, tmpb;  std::vector<int> piv;
        bool luValid = false;
        bool factor()
        {
            const int n = nUnk;
            LU = J;  piv.assign ((size_t) n, 0);
            double* A = LU.data();
            for (int c = 0; c < n; ++c)
            {
                int p = c;  double best = std::fabs (A[c * n + c]);
                for (int r = c + 1; r < n; ++r) { const double a = std::fabs (A[r * n + c]); if (a > best) { best = a; p = r; } }
                if (best < 1.0e-30) { luValid = false; return false; }
                piv[(size_t) c] = p;
                if (p != c) for (int k = 0; k < n; ++k) std::swap (A[c * n + k], A[p * n + k]);
                const double inv = 1.0 / A[c * n + c];
                for (int r = c + 1; r < n; ++r)
                {
                    const double f = A[r * n + c] * inv;
                    A[r * n + c] = f;
                    if (f == 0.0) continue;
                    for (int k = c + 1; k < n; ++k) A[r * n + k] -= f * A[c * n + k];
                }
            }
            luValid = true;
            return true;
        }
        void luSolve()
        {
            const int n = nUnk;
            const double* A = LU.data();
            double* b = dx.data();
            for (int i = 0; i < n; ++i) b[i] = -F[(size_t) i];
            for (int c = 0; c < n; ++c) { const int p = piv[(size_t) c]; if (p != c) std::swap (b[c], b[p]); }
            for (int c = 0; c < n; ++c)
                for (int r = c + 1; r < n; ++r) { const double f = A[r * n + c]; if (f != 0.0) b[r] -= f * b[c]; }
            for (int r = n - 1; r >= 0; --r)
            {
                double sum = b[r];
                for (int k = r + 1; k < n; ++k) sum -= A[r * n + k] * b[k];
                b[r] = sum / A[r * n + r];
            }
        }
        bool linsolve (bool refactor)
        {
            if (refactor || ! luValid) { if (! factor()) return false; }
            luSolve();
            return true;
        }

        double resNorm() const { double m = 0.0; for (double f : F) m = std::max (m, std::fabs (f)); return m; }

        std::vector<double> x0buf;
        bool newton (int maxIter)
        {
            lastIterations = 0;
            if (x0buf.size() != x.size()) x0buf.assign (x.size(), 0.0);
            const size_t n = x.size();
            double prevD = 1.0e30;
            for (int it = 0; it < maxIter; ++it)
            {
                assemble();
                const double r0 = resNorm();
                const bool fresh = dcMode || ! chord || it >= 4 || ! luValid;
                if (! linsolve (fresh)) return false;
                double maxd = 0.0;
                for (size_t i = 0; i < n; ++i) maxd = std::max (maxd, std::fabs (dx[i]));
                lastIterations = it + 1;
                if (maxd < tolStep) { for (size_t i = 0; i < n; ++i) x[i] += dx[i]; if (it >= 2 || maxd > 0.05) luValid = ! chord ? luValid : false; return true; }
                if (it > 0 && maxd > 0.6 * prevD) luValid = false;          // chord iteration stalling: refresh the factors
                prevD = maxd;
                double scale = maxd > 80.0 ? 80.0 / maxd : 1.0;
                if (maxd < 1.5 && ! dcMode)
                {
                    for (size_t i = 0; i < n; ++i) x[i] += scale * dx[i];
                    continue;
                }
                luValid = false;
                for (size_t i = 0; i < n; ++i) x0buf[i] = x[i];
                for (int bt = 0; bt < 6; ++bt)
                {
                    for (size_t i = 0; i < n; ++i) x[i] = x0buf[i] + scale * dx[i];
                    assemble();
                    if (resNorm() <= r0 * 1.0001 || r0 < 1.0e-12) break;
                    scale *= 0.5;
                }
                if (scale * maxd < 2.0e-5) return true;
            }
            return false;
        }
    public:
        double tolStep = 5.0e-6;
        bool chord = false;
        void setChord (bool c) { chord = c; luValid = false; }
    private:
    };
}
