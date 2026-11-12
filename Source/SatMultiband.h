// SatMultiband.h: four-band dynamics and saturation for the stand-alone saturator. No JUCE; needs only SatModel.h.
//
//   input -> input gain -> [wide-band dynamics] -> split into 4 bands (Linkwitz-Riley 4th order, phase-aligned) ->
//        per band:  dynamics (compressor with soft knee + downward expander/gate)  and  saturation (SatLab::Model, any preset)
//                   in either order -> band level / mute -> sum -> output gain
//
//  * The split sums back to the input with a flat magnitude (an all-pass), so with every band's dynamics and saturation off the block is transparent.
//  * Dynamics detect on the band's own signal: instant peak that falls with the detector time (3 ms), linked across the two channels.
//    Attack and release then smooth the gain reduction in dB.
//  * Saturation runs at 2x inside SatLab::Model, which adds a small filter delay to the wet path; a partial Sat Mix therefore blends two slightly
//    different phases (set it to 1 for the cleanest result).
//  * Zero latency.
#pragma once
#include "SatModel.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace SatLab
{
    struct Dynamics
    {
        bool  on = false;
        float thr_db = -24.0f, ratio = 3.0f, knee_db = 6.0f;      // compressor: above the threshold, ratio:1 (1 = off); knee = soft-knee width
        float attack_ms = 10.0f, release_ms = 120.0f;
        float makeup_db = 0.0f;
        float detect_ms = 3.0f;                                    // level detector: instant peak, falls with this time constant (longer = steadier on bass)
        float exp_thr_db = -80.0f, exp_ratio = 1.0f, exp_range_db = 40.0f;   // downward expander / gate: below its threshold the level falls ratio:1 faster, at most range dB
    };

    struct BandSettings
    {
        Dynamics dyn;
        bool   sat_on = false;
        Params sat;                                   // the saturator's own settings (or a preset's)
        float  sat_mix = 1.0f;
        bool   dyn_first = true;                      // dynamics before the saturator (true) or after it
        float  level_db = 0.0f;
        bool   mute = false;
    };

    struct MultibandParams
    {
        static constexpr int kBands = 4;
        float xo[3] { 150.0f, 800.0f, 3500.0f };      // crossover frequencies (Hz)
        std::array<BandSettings, kBands> band;
        Dynamics wide;                                // optional full-band dynamics ahead of the split
        float in_db = 0.0f, out_db = 0.0f;
    };

    // a chain preset: the whole multiband setting, plus where it came from and how well it fits
    struct ChainPreset { const char* name; const char* source; MultibandParams p; };

    class Multiband
    {
    public:
        static constexpr int kBands = MultibandParams::kBands;

        void prepare (double sampleRate, int maxBlock = 512)
        {
            sr = sampleRate;  maxN = std::max (1, maxBlock);
            for (int c = 0; c < 2; ++c) for (int b = 0; b < kBands; ++b) buf[c][b].assign ((size_t) maxN, 0.0f);
            for (int c = 0; c < 2; ++c) for (int b = 0; b < kBands; ++b) sat[c][b].prepare (sampleRate);
            lastXo[0] = lastXo[1] = lastXo[2] = -1.0f;
            reset();
        }
        void reset()
        {
            for (auto& x : split) x.reset();
            for (int c = 0; c < 2; ++c) for (int b = 0; b < kBands; ++b) sat[c][b].reset();
            for (auto& g : gr) g = 0.0f;
            for (auto& e : env) e = 0.0f;
            wideGr = 0.0f;  wideEnv = 0.0f;
        }

        // l and r are processed in place; r may be null for mono. Any block length.
        void process (float* l, float* r, int n, const MultibandParams& p)
        {
            for (int pos = 0; pos < n; pos += maxN)
                run (l + pos, r != nullptr ? r + pos : nullptr, std::min (maxN, n - pos), p);
        }

        float gainReductionDb (int band) const { return gr[(size_t) band]; }     // for meters: positive = reduced (negative = raised by the expander is not shown)
        float wideGainReductionDb() const { return wideGr; }

    private:
        // Linkwitz-Riley 4th order = two identical second-order Butterworth sections
        struct Lr4
        {
            Biquad a, b;
            void design (bool high, double hz, double sr) { a.design (high, hz, sr);  b = a; }
            void reset() { a.reset();  b.reset(); }
            double run (double x) { return b.run (a.run (x)); }
        };
        struct Cross { Lr4 lp, hp; void design (double hz, double sr) { lp.design (false, hz, sr);  hp.design (true, hz, sr); } void reset() { lp.reset();  hp.reset(); } };
        // per channel: three crossovers (c1..c3), plus all-pass copies of c2 and c3 for the lower bands
        struct Split
        {
            Cross c1, c2, c3, ap2a, ap3a, ap3b;
            void design (const float* xo, double sr) { c1.design (xo[0], sr);  c2.design (xo[1], sr);  c3.design (xo[2], sr);  ap2a.design (xo[1], sr);  ap3a.design (xo[2], sr);  ap3b.design (xo[2], sr); }
            void reset() { c1.reset();  c2.reset();  c3.reset();  ap2a.reset();  ap3a.reset();  ap3b.reset(); }
            void run (double x, float* out)
            {
                const double lo1 = c1.lp.run (x), hi1 = c1.hp.run (x);
                const double lo2 = c2.lp.run (hi1), hi2 = c2.hp.run (hi1);
                const double lo3 = c3.lp.run (hi2), hi3 = c3.hp.run (hi2);
                double b0 = lo1;  b0 = ap2a.lp.run (b0) + ap2a.hp.run (b0);  b0 = ap3a.lp.run (b0) + ap3a.hp.run (b0);     // band 0 passes the all-passes of crossovers 2 and 3
                double b1 = lo2;  b1 = ap3b.lp.run (b1) + ap3b.hp.run (b1);                                                    // band 1 passes the all-pass of crossover 3
                out[0] = (float) b0;  out[1] = (float) b1;  out[2] = (float) lo3;  out[3] = (float) hi3;
            }
        };

        static float dbToLin (float db) { return std::pow (10.0f, db / 20.0f); }

        // one dynamics stage over n samples of one or two channels; g is the stage's smoothed gain reduction (dB, positive = turned down)
        void dynamics (float* a, float* b, int n, const Dynamics& d, float& g, float& env)
        {
            const float atk = 1.0f - std::exp (-1.0f / (0.001f * std::max (0.05f, d.attack_ms) * (float) sr));
            const float rel = 1.0f - std::exp (-1.0f / (0.001f * std::max (1.0f, d.release_ms) * (float) sr));
            const float ratio = std::max (1.0f, d.ratio), eratio = std::max (1.0f, d.exp_ratio);
            const float makeup = dbToLin (d.makeup_db);
            const float knee = std::max (0.0f, d.knee_db), half = 0.5f * knee;
            const float dec = std::exp (-1.0f / (0.001f * std::max (0.1f, d.detect_ms) * (float) sr));
            for (int i = 0; i < n; ++i)
            {
                float pk = std::abs (a[i]);
                if (b != nullptr) pk = std::max (pk, std::abs (b[i]));
                env = std::max (pk, env * dec);
                const float lv = 20.0f * std::log10 (env + 1.0e-9f);
                // compressor: gain reduction wanted, dB
                float target = 0.0f;
                const float over = lv - d.thr_db;
                if (ratio > 1.0f)
                {
                    if (knee > 0.0f && over > -half && over < half) { const float t = over + half;  target = (1.0f - 1.0f / ratio) * t * t / (2.0f * knee); }
                    else if (over >= half) target = (1.0f - 1.0f / ratio) * over;
                }
                // expander / gate: extra reduction below its threshold, up to the range
                if (eratio > 1.0f && lv < d.exp_thr_db)
                    target += std::min (std::max (0.0f, d.exp_range_db), (eratio - 1.0f) * (d.exp_thr_db - lv));
                g += (target > g ? atk : rel) * (target - g);
                const float gain = dbToLin (-g) * makeup;
                a[i] *= gain;
                if (b != nullptr) b[i] *= gain;
            }
        }

        void run (float* l, float* r, int n, const MultibandParams& p)
        {
            const int chans = r != nullptr ? 2 : 1;
            // crossover frequencies: sorted, kept apart, below the Nyquist
            float xo[3] { p.xo[0], p.xo[1], p.xo[2] };
            std::sort (xo, xo + 3);
            xo[0] = std::max (20.0f, xo[0]);  xo[1] = std::max (xo[1], xo[0] * 1.25f);  xo[2] = std::max (xo[2], xo[1] * 1.25f);
            xo[2] = std::min (xo[2], (float) (0.45 * sr));  xo[1] = std::min (xo[1], xo[2] / 1.25f);  xo[0] = std::min (xo[0], xo[1] / 1.25f);
            if (xo[0] != lastXo[0] || xo[1] != lastXo[1] || xo[2] != lastXo[2])
            {
                for (int c = 0; c < 2; ++c) split[c].design (xo, sr);
                for (int k = 0; k < 3; ++k) lastXo[k] = xo[k];
            }
            const float gin = dbToLin (p.in_db), gout = dbToLin (p.out_db);
            for (int i = 0; i < n; ++i) { l[i] *= gin;  if (r != nullptr) r[i] *= gin; }
            if (p.wide.on) dynamics (l, r, n, p.wide, wideGr, wideEnv);
            for (int i = 0; i < n; ++i)
                for (int c = 0; c < chans; ++c)
                {
                    float o[kBands];
                    split[c].run ((double) (c == 0 ? l : r)[i], o);
                    for (int b = 0; b < kBands; ++b) buf[c][b][(size_t) i] = o[b];
                }
            for (int i = 0; i < n; ++i) { l[i] = 0.0f;  if (r != nullptr) r[i] = 0.0f; }
            for (int b = 0; b < kBands; ++b)
            {
                const BandSettings& s = p.band[(size_t) b];
                float* x0 = buf[0][b].data();  float* x1 = chans == 2 ? buf[1][b].data() : nullptr;
                if (s.mute) continue;
                auto doSat = [&]()
                {
                    if (! s.sat_on) return;
                    const float mix = std::min (1.0f, std::max (0.0f, s.sat_mix));
                    for (int c = 0; c < chans; ++c)
                    {
                        float* x = c == 0 ? x0 : x1;
                        if (mix >= 0.9999f) { sat[c][b].process (x, n, s.sat);  continue; }
                        std::vector<float>& tmp = scratch;  tmp.assign (x, x + n);
                        sat[c][b].process (tmp.data(), n, s.sat);
                        for (int i = 0; i < n; ++i) x[i] = (1.0f - mix) * x[i] + mix * tmp[(size_t) i];
                    }
                };
                auto doDyn = [&]() { if (s.dyn.on) dynamics (x0, x1, n, s.dyn, gr[(size_t) b], env[(size_t) b]); };
                if (s.dyn_first) { doDyn();  doSat(); } else { doSat();  doDyn(); }
                const float lv = dbToLin (s.level_db);
                for (int i = 0; i < n; ++i) { l[i] += lv * x0[i];  if (r != nullptr) r[i] += lv * x1[i]; }
            }
            for (int i = 0; i < n; ++i) { l[i] *= gout;  if (r != nullptr) r[i] *= gout; }
        }

        double sr = 48000.0;  int maxN = 512;
        float lastXo[3] { -1.0f, -1.0f, -1.0f };
        std::array<Split, 2> split;
        std::array<std::array<Model, kBands>, 2> sat;
        std::array<std::array<std::vector<float>, kBands>, 2> buf;
        std::vector<float> scratch;
        std::array<float, kBands> gr { 0.0f, 0.0f, 0.0f, 0.0f };
        std::array<float, kBands> env { 0.0f, 0.0f, 0.0f, 0.0f };
        float wideGr = 0.0f, wideEnv = 0.0f;
    };
}
