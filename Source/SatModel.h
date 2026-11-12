// SatModel.h: a stand-alone saturator model, made to be lifted out of the guitar plug-in.
// No JUCE, no other plug-in headers: only <cmath>, <algorithm>, <array>, <vector>.
//
// Structure (a Wiener-Hammerstein chain; fitted to measurements of other saturators, see SATURATOR_CAPTURE_FINDINGS.md):
//     input -> pre high-pass -> pre low-pass -> drive -> curve -> post high-pass -> post low-pass -> output level
// The curve is   y = (1 - fold) * ( c(u + bias) - c(bias) ) + fold * sin(u) + fullwave * c(|u|)
// with the soft clipper  c(v) = v / (1 + |v|^hardness)^(1/hardness)   (hardness 1 is a soft knee, 2 the smooth "Transformer" shape, 10 nearly a hard clip),
// "bias" adds even harmonics, "fold" blends in a sine fold (the Mixhead shape), "fullwave" adds an octave-up component.
// The filters are second-order Butterworth. The model runs at twice the host rate (zero-stuffing up, two second-order low-passes at 0.45 x the host rate
// each way), so the harmonics a hard drive makes stay mostly out of the audible band. Memoryless apart from the filters: no dynamics, no level memory.
// Presets: fits to captures. fitGainErr / fitHarmErr are the fit's RMS errors in dB over sine tests at 440 Hz, 1 kHz, 3 kHz and 110 Hz
// (8 levels each, H2 to H5); under about 5 dB is close, over 8 dB is only a rough likeness. Measured, not listened to.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace SatLab
{
    struct Params
    {
        float pre_hp_hz = 20.0f, pre_lp_hz = 20000.0f;     // filters ahead of the curve (20 Hz / 20 kHz: off)
        float drive_db = 0.0f;                              // gain into the curve
        float hardness = 2.0f;                              // 1 to 10
        float bias = 0.0f;                                  // 0 to 1.2: pushes the curve off-centre (even harmonics)
        float fold = 0.0f;                                  // 0 to 1: blends in a sine fold
        float fullwave = 0.0f;                              // 0 to 1: adds a full-wave (octave-up) part
        float post_hp_hz = 20.0f, post_lp_hz = 20000.0f;   // filters after the curve
        float out_db = 0.0f;                                // output level
    };

    struct Preset { const char* name; const char* source; float fitGainErr, fitHarmErr; Params p; };
    inline const Preset* presets (int& count)
    {
        static const Preset list[] =
        {
        { "Sine fold, Mixhead Default", "MB Mixhead, Default", 0.1f, 0.1f, { 20.0f, 20000.0f, 0.00f, 2.000f, 0.000f, 1.000f, 0.000f, 20.0f, 20000.0f, 0.00f } },
        { "Sine fold, Mixhead Drive 5", "MB Mixhead, Drive 5", 0.7f, 2.8f, { 35.2f, 21939.1f, 3.21f, 4.647f, 0.054f, 0.975f, 0.001f, 45.9f, 11616.1f, 1.30f } },
        { "Wavefold", "Dicksmasher The Monkey's Paw", 1.6f, 3.6f, { 33.8f, 8854.9f, 15.35f, 1.000f, 0.000f, 0.441f, 0.000f, 31.5f, 22000.0f, -1.32f } },
        { "Even-harmonic saturator", "Dicksmasher Discharge", 1.7f, 4.4f, { 20.4f, 18917.0f, 3.39f, 2.471f, 0.463f, 0.000f, 0.000f, 89.4f, 9576.0f, -5.29f } },
        { "High-gain saturator", "Dicksmasher To Hunt a Man", 1.2f, 7.1f, { 64.0f, 16951.6f, 21.38f, 1.000f, 0.002f, 0.004f, 0.205f, 121.0f, 21998.7f, -3.87f } },
        { "Hard clip, high gain", "Dicksmasher Doom Cannon (clipper stage only)", 1.8f, 8.5f, { 37.7f, 9844.2f, 38.27f, 10.000f, 0.755f, 0.009f, 0.128f, 89.4f, 14512.7f, -12.98f } },
        };
        count = (int) (sizeof (list) / sizeof (list[0]));
        return list;
    }

    // one second-order section (Butterworth by the bilinear transform); high-pass or low-pass
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void design (bool high, double hz, double sr)
        {
            const double w = std::tan (3.14159265358979323846 * std::min (hz, 0.49 * sr) / sr), k = std::sqrt (2.0), n = 1.0 / (1.0 + k * w + w * w);
            if (high) { b0 = n; b1 = -2 * n; b2 = n; }
            else      { b0 = w * w * n; b1 = 2 * b0; b2 = b0; }
            a1 = 2 * (w * w - 1) * n;  a2 = (1 - k * w + w * w) * n;
        }
        void reset() { z1 = z2 = 0; }
        double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    };

    class Model
    {
    public:
        void prepare (double hostRate)
        {
            sr2 = 2.0 * hostRate;
            for (auto* s : { &upA, &upB, &dnA, &dnB }) s->design (false, 0.45 * hostRate, sr2);
            lastKey = -1.0;  reset();
        }
        void reset() { for (auto* s : { &upA, &upB, &dnA, &dnB, &preHp, &preLp, &postHp, &postLp }) s->reset(); }
        static double clip (double v, double p) { return v / std::pow (1.0 + std::pow (std::abs (v), p), 1.0 / p); }
        void process (float* x, int n, const Params& q)
        {
            const double key = q.pre_hp_hz * 1.0 + q.pre_lp_hz * 3.0 + q.post_hp_hz * 7.0 + q.post_lp_hz * 11.0;
            if (key != lastKey)
            {
                lastKey = key;
                preHp.design (true, q.pre_hp_hz, sr2);   preLp.design (false, q.pre_lp_hz, sr2);
                postHp.design (true, q.post_hp_hz, sr2); postLp.design (false, q.post_lp_hz, sr2);
            }
            const double g = std::pow (10.0, q.drive_db / 20.0), o = std::pow (10.0, q.out_db / 20.0);
            const double p = std::max (1.0f, q.hardness), c0 = clip ((double) q.bias, p);
            const double nyq = 0.5 * sr2;
            const bool uPreHp = q.pre_hp_hz > 25.0f, uPreLp = q.pre_lp_hz < 0.9 * nyq, uPostHp = q.post_hp_hz > 25.0f, uPostLp = q.post_lp_hz < 0.9 * nyq;
            for (int i = 0; i < n; ++i)
            {
                double d = 0.0;
                for (int k = 0; k < 2; ++k)
                {
                    double v = k == 0 ? 2.0 * (double) x[i] : 0.0;                 // zero stuffing; the 2 keeps the level
                    v = upB.run (upA.run (v));
                    if (uPreHp) v = preHp.run (v);
                    if (uPreLp) v = preLp.run (v);
                    const double u = g * v;
                    double y = (1.0 - q.fold) * (clip (u + q.bias, p) - c0) + q.fold * std::sin (u);
                    if (q.fullwave > 0.0f) y += q.fullwave * clip (std::abs (u), p);
                    if (uPostHp) y = postHp.run (y);
                    if (uPostLp) y = postLp.run (y);
                    d = dnB.run (dnA.run (o * y));                                 // both samples go through the down filter; the second is kept
                }
                x[i] = (float) d;
            }
        }
    private:
        double sr2 = 96000.0, lastKey = -1.0;
        Biquad upA, upB, dnA, dnB, preHp, preLp, postHp, postLp;
    };
}
