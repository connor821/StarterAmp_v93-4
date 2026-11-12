// Stand-alone test for SatMultiband.h.   g++ -std=c++17 -O2 Tests/satmultiband_test.cpp -ISource -o satmultiband_test && ./satmultiband_test
#include "SatMultiband.h"
#include <cstdio>
#include <cstdlib>
#include <random>

using namespace SatLab;

static int fails = 0;
#define CHECK(cond, ...) do { if (cond) std::printf ("  ok    "); else { std::printf ("  FAIL  "); ++fails; } std::printf (__VA_ARGS__); std::printf ("\n"); } while (0)

static const double SR = 48000.0;
static double db (double x) { return 20.0 * std::log10 (std::max (x, 1.0e-12)); }
static double lin (double d) { return std::pow (10.0, d / 20.0); }

static std::vector<float> sine (double hz, double dbfs, double secs)
{
    std::vector<float> v ((size_t) (secs * SR));
    for (size_t i = 0; i < v.size(); ++i) v[i] = (float) (lin (dbfs) * std::sin (2.0 * 3.14159265358979 * hz * (double) i / SR));
    return v;
}
static double rmsDb (const std::vector<float>& v, double fromSec, double toSec)
{
    size_t a = (size_t) (fromSec * SR), b = std::min (v.size(), (size_t) (toSec * SR));
    double s = 0;  for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i];
    return db (std::sqrt (s / (double) (b - a)));
}
// run a mono signal through a fresh instance
static std::vector<float> run (std::vector<float> x, const MultibandParams& p, int block = 512)
{
    Multiband m;  m.prepare (SR, 512);
    for (size_t pos = 0; pos < x.size(); pos += (size_t) block)
        m.process (x.data() + pos, nullptr, (int) std::min ((size_t) block, x.size() - pos), p);
    return x;
}

// only band 2 (800 Hz to 3.5 kHz) sounds, so a gain change in it is not mixed with its neighbours
static void solo (MultibandParams& p) { for (int b = 0; b < 4; ++b) p.band[(size_t) b].mute = b != 2; }

int main()
{
    // ---- 1. the split is transparent: every band off, flat magnitude
    std::printf ("1. split sums flat (all off)\n");
    {
        MultibandParams p;  double worst = 0;
        for (double hz : { 30.0, 60.0, 100.0, 150.0, 220.0, 400.0, 800.0, 1200.0, 2500.0, 3500.0, 5000.0, 9000.0, 15000.0 })
        {
            auto y = run (sine (hz, -12, 0.6), p);
            worst = std::max (worst, std::abs (rmsDb (y, 0.3, 0.6) - rmsDb (sine (hz, -12, 0.6), 0.3, 0.6)));
        }
        CHECK (worst < 0.1, "worst deviation over 30 Hz to 15 kHz: %.3f dB (limit 0.1)", worst);
        MultibandParams q;  q.xo[0] = 90;  q.xo[1] = 400;  q.xo[2] = 6000;  worst = 0;
        for (double hz : { 40.0, 90.0, 250.0, 400.0, 1000.0, 6000.0, 12000.0 })
            worst = std::max (worst, std::abs (rmsDb (run (sine (hz, -12, 0.6), q), 0.3, 0.6) - rmsDb (sine (hz, -12, 0.6), 0.3, 0.6)));
        CHECK (worst < 0.1, "other crossover points: %.3f dB", worst);
    }

    // ---- 2. band isolation
    std::printf ("2. band isolation\n");
    {
        MultibandParams p;
        for (int b = 1; b < 4; ++b) p.band[(size_t) b].mute = true;
        const double lo = rmsDb (run (sine (50, -12, 0.6), p), 0.3, 0.6), hi = rmsDb (run (sine (2000, -12, 0.6), p), 0.3, 0.6);
        const double ref12 = rmsDb (sine (50, -12, 0.6), 0.3, 0.6);
        CHECK (lo > ref12 - 0.3 && hi < ref12 - 40, "band 0 only: 50 Hz %.1f dB, 2 kHz %.1f dB (input %.1f)", lo, hi, ref12);
        MultibandParams q;  for (int b = 0; b < 3; ++b) q.band[(size_t) b].mute = true;
        const double h2 = rmsDb (run (sine (10000, -12, 0.6), q), 0.3, 0.6), l2 = rmsDb (run (sine (300, -12, 0.6), q), 0.3, 0.6);
        CHECK (h2 > ref12 - 0.3 && l2 < ref12 - 40, "band 3 only: 10 kHz %.1f dB, 300 Hz %.1f dB", h2, l2);
    }

    // ---- 3. static compression law
    std::printf ("3. compressor law\n");
    {
        for (double ratio : { 2.0, 4.0, 10.0 })
        {
            MultibandParams p, off;  solo (p);  solo (off);
            auto& d = p.band[2].dyn;  d.on = true;  d.thr_db = -20;  d.ratio = (float) ratio;  d.knee_db = 0;  d.attack_ms = 0.2f;  d.release_ms = 40;  d.detect_ms = 30;
            auto x = sine (1500, -8, 1.0);
            const double lvl = rmsDb (run (x, off), 0.7, 1.0) + 3.0103;                  // the band's own peak level (it is a little under -8 at 1.5 kHz)
            const double diff = rmsDb (run (x, p), 0.7, 1.0) - rmsDb (run (x, off), 0.7, 1.0);
            const double want = -(1.0 - 1.0 / ratio) * (lvl + 20.0);
            CHECK (std::abs (diff - want) < 0.5, "ratio %.0f:1, %.1f dB over: %.2f dB (theory %.2f)", ratio, lvl + 20.0, diff, want);
        }
        // under the threshold nothing happens, the knee is half-way at the threshold
        MultibandParams p, off;  solo (p);  solo (off);  auto& d = p.band[2].dyn;  d.on = true;  d.thr_db = -20;  d.ratio = 4;  d.knee_db = 0;  d.attack_ms = 0.2f;
        auto q = sine (1500, -30, 1.0);
        CHECK (std::abs (rmsDb (run (q, p), 0.7, 1.0) - rmsDb (run (q, off), 0.7, 1.0)) < 0.05, "below the threshold: unchanged");
        // makeup
        d.makeup_db = 6;  d.ratio = 1;
        const double mk = rmsDb (run (q, p), 0.7, 1.0) - rmsDb (run (q, off), 0.7, 1.0);
        CHECK (std::abs (mk - 6.0) < 0.05, "makeup +6 dB: %.3f", mk);
    }

    // ---- 4. attack and release times (wide stage on a DC step, read the gain reduction)
    std::printf ("4. attack and release\n");
    {
        MultibandParams p;  p.wide.on = true;  p.wide.thr_db = -20;  p.wide.ratio = 4;  p.wide.knee_db = 0;  p.wide.attack_ms = 5;  p.wide.release_ms = 80;
        Multiband m;  m.prepare (SR, 512);
        std::vector<float> one (1, 0.0f);
        double final = 0.75 * (db (0.5) + 20.0);                        // theory for DC 0.5
        int tAtk = -1, tRel = -1;
        for (int i = 0; i < (int) (SR * 0.5); ++i)
        {
            one[0] = 0.5f;  m.process (one.data(), nullptr, 1, p);
            if (tAtk < 0 && m.wideGainReductionDb() >= 0.632 * final) tAtk = i;
        }
        const double settled = m.wideGainReductionDb();
        for (int i = 0; i < (int) (SR * 0.5); ++i)
        {
            one[0] = 0.0f;  m.process (one.data(), nullptr, 1, p);
            if (tRel < 0 && m.wideGainReductionDb() <= 0.368 * settled) tRel = i;
        }
        CHECK (std::abs (settled - final) < 0.1, "settled gain reduction %.2f dB (theory %.2f)", settled, final);
        CHECK (std::abs (tAtk / SR * 1000.0 - 5.0) < 0.5, "attack to 63%%: %.2f ms (set 5)", tAtk / SR * 1000.0);
        CHECK (std::abs (tRel / SR * 1000.0 - 80.0) < 4.0, "release to 37%%: %.1f ms (set 80)", tRel / SR * 1000.0);
    }

    // ---- 5. expander
    std::printf ("5. expander\n");
    {
        MultibandParams p, off;  solo (p);  solo (off);  auto& d = p.band[2].dyn;  d.on = true;  d.ratio = 1;  d.exp_thr_db = -40;  d.exp_ratio = 2;  d.exp_range_db = 40;  d.attack_ms = 0.2f;  d.release_ms = 20;  d.detect_ms = 30;
        auto x = sine (1500, -60, 1.0);
        const double want = -(-40.0 - (rmsDb (run (x, off), 0.7, 1.0) + 3.0103));
        const double diff = rmsDb (run (x, p), 0.7, 1.0) - rmsDb (run (x, off), 0.7, 1.0);
        CHECK (std::abs (diff - want) < 0.5, "20 dB below threshold, 2:1 expander: %.2f dB (theory %.2f)", diff, want);
        d.exp_range_db = 12;
        const double diff2 = rmsDb (run (x, p), 0.7, 1.0) - rmsDb (run (x, off), 0.7, 1.0);
        CHECK (std::abs (diff2 + 12.0) < 0.6, "range limited to 12 dB: %.2f dB", diff2);
        auto loud = sine (1500, -20, 1.0);
        CHECK (std::abs (rmsDb (run (loud, p), 0.7, 1.0) - rmsDb (run (loud, off), 0.7, 1.0)) < 0.05, "above the threshold: unchanged");
    }

    // ---- 6. bands are independent
    std::printf ("6. independence\n");
    {
        MultibandParams p, off;  auto& d = p.band[3].dyn;  d.on = true;  d.thr_db = -50;  d.ratio = 20;  d.knee_db = 0;
        auto x = sine (100, -6, 0.6);
        CHECK (std::abs (rmsDb (run (x, p), 0.3, 0.6) - rmsDb (run (x, off), 0.3, 0.6)) < 0.05, "heavy compression of band 3 leaves a 100 Hz tone alone");
        int cnt = 0;  const Preset* prs = presets (cnt);
        MultibandParams s, off2;  s.band[0].sat_on = true;  s.band[0].sat = prs[4].p;
        auto h = sine (6000, -12, 0.6);
        CHECK (std::abs (rmsDb (run (h, s), 0.3, 0.6) - rmsDb (run (h, off2), 0.3, 0.6)) < 0.1, "saturating band 0 leaves a 6 kHz tone alone");
    }

    // ---- 7. robust: loud noise, every stage on, no NaN and bounded
    std::printf ("7. extreme input\n");
    {
        int count = 0;  const Preset* pr = presets (count);
        MultibandParams p;  p.in_db = 24;
        for (int b = 0; b < 4; ++b)
        {
            auto& s = p.band[(size_t) b];  s.sat_on = true;  s.sat = pr[(b + 2) % count].p;  s.sat_mix = b == 1 ? 0.6f : 1.0f;  s.dyn_first = (b % 2) == 0;
            s.dyn.on = true;  s.dyn.thr_db = -30;  s.dyn.ratio = 8;  s.dyn.exp_thr_db = -60;  s.dyn.exp_ratio = 3;
        }
        p.wide.on = true;  p.wide.ratio = 6;
        std::mt19937 g (1);  std::normal_distribution<float> n (0.0f, 1.0f);
        std::vector<float> l (96000), r (96000);
        for (size_t i = 0; i < l.size(); ++i) { l[i] = 5.0f * n (g);  r[i] = (i % 4000 < 2000 ? 8.0f : 0.001f) * n (g); }
        Multiband m;  m.prepare (SR, 512);  m.process (l.data(), r.data(), (int) l.size(), p);
        bool fin = true;  double peak = 0;
        for (size_t i = 0; i < l.size(); ++i) { fin = fin && std::isfinite (l[i]) && std::isfinite (r[i]); peak = std::max ({ peak, (double) std::abs (l[i]), (double) std::abs (r[i]) }); }
        CHECK (fin && peak < 100.0, "finite, peak %.2f", peak);
        // extreme crossover requests are clamped, not crashed
        MultibandParams q;  q.xo[0] = 30000;  q.xo[1] = 5;  q.xo[2] = 0;
        auto y = run (sine (1000, -12, 0.3), q);
        bool f2 = true;  for (float v : y) f2 = f2 && std::isfinite (v);
        CHECK (f2 && std::abs (rmsDb (y, 0.15, 0.3) - rmsDb (sine (1000, -12, 0.3), 0.15, 0.3)) < 0.5, "silly crossover values are clamped, level kept");
    }

    // ---- 8. block size independence (mono and stereo)
    std::printf ("8. block size independence\n");
    {
        MultibandParams p;  int count = 0;  const Preset* pr = presets (count);
        for (int b = 0; b < 4; ++b)
        {
            auto& s = p.band[(size_t) b];  s.sat_on = b != 2;  s.sat = pr[b % count].p;  s.sat_mix = b == 1 ? 0.5f : 1.0f;
            s.dyn.on = true;  s.dyn.thr_db = -24;  s.dyn.ratio = 4;  s.dyn_first = b < 2;
        }
        std::mt19937 g (7);  std::normal_distribution<float> n (0.0f, 0.2f);
        std::vector<float> x (30000), x2 (30000);
        for (size_t i = 0; i < x.size(); ++i) { x[i] = n (g);  x2[i] = n (g); }
        auto ref = run (x, p, 512), one = run (x, p, 1), odd = run (x, p, 77);
        double d1 = 0, d2 = 0;
        for (size_t i = 0; i < x.size(); ++i) { d1 = std::max (d1, (double) std::abs (ref[i] - one[i]));  d2 = std::max (d2, (double) std::abs (ref[i] - odd[i])); }
        CHECK (d1 < 1.0e-6 && d2 < 1.0e-6, "mono: 1-sample and 77-sample blocks match 512 (max diff %.2g, %.2g)", d1, d2);
        auto stereo = [&] (int block)
        {
            auto l = x, r = x2;  Multiband m;  m.prepare (SR, 512);
            for (size_t pos = 0; pos < l.size(); pos += (size_t) block) m.process (l.data() + pos, r.data() + pos, (int) std::min ((size_t) block, l.size() - pos), p);
            l.insert (l.end(), r.begin(), r.end());  return l;
        };
        auto sa = stereo (512), sb = stereo (33);
        double d3 = 0;  for (size_t i = 0; i < sa.size(); ++i) d3 = std::max (d3, (double) std::abs (sa[i] - sb[i]));
        CHECK (d3 < 1.0e-6, "stereo: 33-sample blocks match (max diff %.2g)", d3);
        // the channels are linked: a loud left channel turns the quiet right channel down too
        auto lq = sine (1500, -6, 0.5), rq = sine (1500, -40, 0.5), rq0 = rq;
        MultibandParams c;  c.band[2].dyn.on = true;  c.band[2].dyn.thr_db = -20;  c.band[2].dyn.ratio = 6;  c.band[2].dyn.knee_db = 0;  c.band[2].dyn.attack_ms = 0.2f;
        Multiband m;  m.prepare (SR, 512);  m.process (lq.data(), rq.data(), (int) lq.size(), c);
        CHECK (rmsDb (rq, 0.3, 0.5) < rmsDb (rq0, 0.3, 0.5) - 5.0, "stereo link: quiet channel follows the loud one (%.1f dB)", rmsDb (rq, 0.3, 0.5) - rmsDb (rq0, 0.3, 0.5));
    }

    // ---- 9. order of dynamics and saturation matters, and does what it should
    std::printf ("9. dynamics before / after the saturator\n");
    {
        int count = 0;  const Preset* pr = presets (count);
        MultibandParams a;  auto& s = a.band[2];
        s.sat_on = true;  s.sat = pr[5].p;                                     // hard clip, high gain
        s.dyn.on = true;  s.dyn.thr_db = -30;  s.dyn.ratio = 8;  s.dyn.knee_db = 0;  s.dyn.attack_ms = 0.5f;  s.dyn.release_ms = 100;
        MultibandParams b = a;  b.band[2].dyn_first = false;  a.band[2].dyn_first = true;
        // a burst after a quiet passage: compress first = the clipper sees less level, so less is clipped at the same drive
        std::vector<float> x = sine (1500, -24, 0.4), loud = sine (1500, -6, 0.4);
        x.insert (x.end(), loud.begin(), loud.end());
        auto ya = run (x, a), yb = run (x, b);
        const double la = rmsDb (ya, 0.7, 0.8), lb = rmsDb (yb, 0.7, 0.8);
        CHECK (std::abs (la - lb) > 1.0, "burst level: compress first %.1f dB, clip first %.1f dB", la, lb);
    }

    // ---- 10. a band-split saturator against the Doom Cannon capture (report, not a pass/fail)
    std::printf ("10. Doom Cannon static curve, 1 kHz (measured out) against 'Hard clip, high gain' in band 2 (info)\n");
    {
        int count = 0;  const Preset* pr = presets (count);
        const double ref[][2] { { -59.8, -36.3 }, { -50.2, -27.2 }, { -40.6, -20.9 }, { -29.6, -16.9 }, { -19.9, -14.6 }, { -11.7, -12.6 }, { -7.6, -11.9 }, { -4.8, -11.5 } };
        MultibandParams p;  for (int b = 0; b < 4; ++b) if (b != 2) p.band[(size_t) b].mute = true;
        p.band[2].sat_on = true;  p.band[2].sat = pr[5].p;
        double sq = 0;
        std::printf ("      in dBFS   capture   model   (model measured on the 1 kHz component through band 2, its passband loss removed)\n");
        MultibandParams flat; for (int b = 0; b < 4; ++b) if (b != 2) flat.band[(size_t) b].mute = true;
        for (auto& rr : ref)
        {
            auto x = sine (1000, rr[0], 0.5);
            const double o = rmsDb (run (x, p), 0.25, 0.5), f = rmsDb (run (x, flat), 0.25, 0.5) - rmsDb (x, 0.25, 0.5);
            std::printf ("      %6.1f   %6.1f   %6.1f\n", rr[0], rr[1], o - f);
            sq += (o - f - rr[1]) * (o - f - rr[1]);
        }
        std::printf ("      rms difference %.1f dB (a memoryless curve; the capture also has level-dependent filtering)\n", std::sqrt (sq / 8.0));
        CHECK (std::isfinite (sq), "ran");
    }

    std::printf ("\n%s (%d failure%s)\n", fails == 0 ? "ALL PASSED" : "FAILED", fails, fails == 1 ? "" : "s");
    return fails == 0 ? 0 : 1;
}
