// Stand-alone check of SatModel.h: g++ -std=c++17 -O2 Tests/satmodel_test.cpp -ISource -o satmodel_test && ./satmodel_test
// Prints, for each preset, the gain and H2..H5 of a 1 kHz sine at six levels (the same quantities the fits used).
#include "SatModel.h"
#include <cstdio>
#include <vector>
int main()
{
    int n = 0;  const SatLab::Preset* pr = SatLab::presets (n);
    const double SR = 48000.0;
    int bad = 0;
    for (int k = 0; k < n; ++k)
    {
        std::printf ("%s  (%s; fit errors %.1f / %.1f dB)\n   level:  gain   H2   H3   H4   H5\n", pr[k].name, pr[k].source, pr[k].fitGainErr, pr[k].fitHarmErr);
        for (int lvl : { -50, -30, -18, -12, -8, -4 })
        {
            SatLab::Model m;  m.prepare (SR);
            const double A = std::pow (10.0, (lvl + 3.0) / 20.0), f = 1000.0;
            std::vector<float> x ((size_t) (1.0 * SR));
            for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (A * std::sin (2 * 3.14159265358979 * f * (double) i / SR));
            m.process (x.data(), (int) x.size(), pr[k].p);
            const size_t a = (size_t) (0.25 * SR), len = 24000;     // whole periods (48 samples each)
            auto c = [&] (int h) { double s = 0, q = 0; for (size_t i = 0; i < len; ++i) { const double t = (double) (a + i) / SR; s += x[a + i] * std::cos (2 * 3.14159265358979 * h * f * t); q += x[a + i] * std::sin (2 * 3.14159265358979 * h * f * t); } return 2.0 * std::hypot (s, q) / (double) len; };
            const double f1 = c (1);
            std::printf ("   %4d: %+6.1f", lvl, 20 * std::log10 (std::max (f1, 1e-12) / A));
            for (int h = 2; h <= 5; ++h) std::printf (" %5.0f", 20 * std::log10 (std::max (c (h), 1e-12) / std::max (f1, 1e-12)));
            std::printf ("\n");
            for (float v : x) if (! std::isfinite (v) || std::abs (v) > 20.0f) ++bad;
        }
    }
    std::printf (bad ? "FAIL: non-finite or huge output\n" : "all outputs finite and bounded\n");
    return bad ? 1 : 0;
}
