#pragma once
// b15: four more effect blocks for the Order board. All hand-built; the names describe the kind of sound, not any particular unit.
//   FilterFx       : one filter pedal with six modes (low-pass, high-pass, band-pass, notch, formant (vowel), resonator (tuned combs))
//                    and four ways to move it (manual, LFO, envelope follower, random steps)
//   VolumePanFx    : a volume pedal / utility: level, pan (stereo only), polarity
//   MultibandCompFx: four bands (Linkwitz-Riley crossovers), a compressor in each
//   GateExpFx      : a noise gate / downward expander with hold and a detector high-pass
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include "ModFx.h"

namespace FxExtra
{
    // ---- small helpers
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void lowpass (float f, float sr, float q = 0.70710678f)
        {
            const float K = std::tan (3.14159265f * std::min (f, 0.45f * sr) / sr), norm = 1.0f / (1.0f + K / q + K * K);
            b0 = K * K * norm;  b1 = 2.0f * b0;  b2 = b0;  a1 = 2.0f * (K * K - 1.0f) * norm;  a2 = (1.0f - K / q + K * K) * norm;
        }
        void highpass (float f, float sr, float q = 0.70710678f)
        {
            const float K = std::tan (3.14159265f * std::min (f, 0.45f * sr) / sr), norm = 1.0f / (1.0f + K / q + K * K);
            b0 = norm;  b1 = -2.0f * norm;  b2 = norm;  a1 = 2.0f * (K * K - 1.0f) * norm;  a2 = (1.0f - K / q + K * K) * norm;
        }
        // b54: second-order all-pass (Q 0.7071 is the sum of the two halves of a Linkwitz-Riley split: low-pass + high-pass of the same frequency)
        void allpass (float f, float sr, float q = 0.70710678f)
        {
            const float w0 = 6.2831853f * std::min (f, 0.45f * sr) / sr, al = std::sin (w0) / (2.0f * q), cw = std::cos (w0), n = 1.0f / (1.0f + al);
            b0 = (1.0f - al) * n;  b1 = -2.0f * cw * n;  b2 = (1.0f + al) * n;  a1 = -2.0f * cw * n;  a2 = (1.0f - al) * n;
        }
        inline float process (float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void reset() { z1 = z2 = 0.0f; }
    };

    // Linkwitz-Riley 4th order split (two cascaded Butterworth stages): lows + highs add back to the input's magnitude
    struct LR4
    {
        Biquad lp[2], hp[2];
        void set (float f, float sr)
        {
            for (auto& b : lp) b.lowpass (f, sr);
            for (auto& b : hp) b.highpass (f, sr);
        }
        inline void split (float x, float& lo, float& hi)
        {
            lo = lp[1].process (lp[0].process (x));
            hi = hp[1].process (hp[0].process (x));
        }
        void reset() { for (auto& b : lp) b.reset();  for (auto& b : hp) b.reset(); }
    };

    inline float dbToGain (float db) { return std::pow (10.0f, 0.05f * db); }

    // b55: the four-band splitter shared by the transient shaper, the compressor and the saturator. Three crossovers; the middle one splits the
    // signal in two, each half goes through the all-pass of the OTHER half's crossover (so the phase turn is the same everywhere and the four
    // bands add back flat), then each half is split again.
    struct Split4
    {
        static constexpr int kBands = 4;
        LR4 mid, lo, hi;
        Biquad apLo, apHi;
        float lastXo[3] = { -1.0f, -1.0f, -1.0f };
        // The crossovers after keeping them in order, at least a quarter of an octave apart, and under the Nyquist limit
        static void ordered (const float in[3], float sampleRate, float out[3])
        {
            const float top = 0.42f * sampleRate;
            const float f3 = juce::jlimit (60.0f, top, in[2]);
            const float f2 = juce::jlimit (40.0f, f3 / 1.25f, in[1]);
            const float f1 = juce::jlimit (20.0f, f2 / 1.25f, in[0]);
            out[0] = f1;  out[1] = f2;  out[2] = f3;
        }
        void reset()
        {
            mid.reset();  lo.reset();  hi.reset();  apLo.reset();  apHi.reset();
            lastXo[0] = lastXo[1] = lastXo[2] = -1.0f;
        }
        void setCrossovers (const float xo[3], float sr)       // xo: already ordered
        {
            if (xo[0] == lastXo[0] && xo[1] == lastXo[1] && xo[2] == lastXo[2]) return;
            mid.set (xo[1], sr);
            apLo.allpass (xo[2], sr);  apHi.allpass (xo[0], sr);
            lo.set (xo[0], sr);  hi.set (xo[2], sr);
            lastXo[0] = xo[0];  lastXo[1] = xo[1];  lastXo[2] = xo[2];
        }
        inline void split (float x, float b[kBands])
        {
            float l, h;
            mid.split (x, l, h);
            l = apLo.process (l);  h = apHi.process (h);
            lo.split (l, b[0], b[1]);
            hi.split (h, b[2], b[3]);
        }
    };

    // =====================================================================================================================
    class FilterFx
    {
    public:
        struct Params
        {
            int   mode = 0;          // 0 low-pass, 1 high-pass, 2 band-pass, 3 notch, 4 formant, 5 resonator
            float freqHz = 800.0f;   // centre / cutoff; formant: shifts the vowel's formants; resonator: the pitch
            float res = 0.4f;        // 0..1
            int   motion = 0;        // 0 manual, 1 LFO, 2 envelope, 3 random steps
            float rateHz = 1.0f;
            float depth = 0.5f;      // -1..1 (the size of the sweep; negative flips it)
            float vowel = 0.0f;      // 0..1 across A E I O U (formant mode)
            float mix = 1.0f;
        };

        void prepare (double sampleRate)
        {
            sr = (float) sampleRate;
            for (auto& c : comb) for (auto& b : c) { b.assign (8192, 0.0f); }
            reset();
        }
        void reset()
        {
            for (int c = 0; c < 2; ++c)
            {
                lps[c] = bps[c] = 0.0f;  ic1[c] = ic2[c] = 0.0f;  for (int i = 0; i < 3; ++i) { f1[c][i] = f2[c][i] = 0.0f; }
                for (int i = 0; i < 3; ++i) { std::fill (comb[c][i].begin(), comb[c][i].end(), 0.0f); combLp[c][i] = 0.0f; }
                combW[c] = 0;
            }
            lfoPhase = 0.0f;  env = 0.0f;  stepCount = 0;  stepTarget = stepNow = 0.0f;  smoothFreq = -1.0f;  smoothVowel = -1.0f;  rng = 22222u;
        }

        void process (float* l, float* r, int n, const Params& p)
        {
            const int chans = r != nullptr ? 2 : 1;
            const float rate = std::max (0.02f, p.rateHz);
            for (int i = 0; i < n; ++i)
            {
                float* xs[2] = { l + i, r != nullptr ? r + i : nullptr };
                const float in0 = chans == 2 ? 0.5f * (*xs[0] + *xs[1]) : *xs[0];

                // motion signal m: -1..1 (envelope: 0..1)
                float m = 0.0f;
                if (p.motion == 1)
                {
                    lfoPhase += rate / sr;  if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
                    m = std::sin (6.2831853f * lfoPhase);
                }
                else if (p.motion == 2)
                {
                    const float a = std::abs (in0) * 6.0f;
                    env += (a > env ? 1.0f - std::exp (-1.0f / (0.004f * sr)) : 1.0f - std::exp (-1.0f / ((0.03f + 0.25f / (0.5f + rate)) * sr))) * (a - env);
                    m = std::min (1.0f, env);
                }
                else if (p.motion == 3)
                {
                    if (--stepCount <= 0)
                    {
                        stepCount = std::max (1, (int) (sr / rate));
                        rng = rng * 1664525u + 1013904223u;
                        stepTarget = (float) ((rng >> 8) & 0xffff) / 32768.0f - 1.0f;
                    }
                    stepNow += (1.0f - std::exp (-1.0f / (0.012f * sr))) * (stepTarget - stepNow);
                    m = stepNow;
                }

                // control-rate targets, smoothed so sweeps never click
                const float fTarget = juce::jlimit (30.0f, 0.45f * sr, p.freqHz * std::exp2 (3.0f * p.depth * m));
                const float vTarget = juce::jlimit (0.0f, 1.0f, p.vowel + 0.5f * p.depth * m);
                if (smoothFreq < 0.0f) { smoothFreq = fTarget;  smoothVowel = vTarget; }
                const float sc = 1.0f - std::exp (-1.0f / (0.003f * sr));
                smoothFreq += sc * (fTarget - smoothFreq);
                smoothVowel += sc * (vTarget - smoothVowel);
                if ((i & 7) == 0) updateCoeffs (p);

                for (int c = 0; c < chans; ++c)
                {
                    const float x = *xs[c];
                    float y = 0.0f;
                    switch (p.mode)
                    {
                        case 0: case 1: case 2: case 3:
                        {
                            const float hp = (x - (k + g) * ic1[c] - ic2[c]) * a1;   // TPT state-variable filter
                            const float bp = g * hp + ic1[c];
                            const float lp = g * bp + ic2[c];
                            ic1[c] = g * hp + bp;
                            ic2[c] = g * bp + lp;
                            y = p.mode == 0 ? lp * ptrim : p.mode == 1 ? hp * ptrim : p.mode == 2 ? k * bp * 1.0f : lp + hp;
                            break;
                        }
                        case 4:
                        {
                            for (int b = 0; b < 3; ++b)
                            {
                                const float hp = (x - (fk[b] + fg[b]) * f1[c][b] - f2[c][b]) * fa[b];
                                const float bp = fg[b] * hp + f1[c][b];
                                const float lp = fg[b] * bp + f2[c][b];
                                f1[c][b] = fg[b] * hp + bp;
                                f2[c][b] = fg[b] * bp + lp;
                                y += fAmp[b] * fk[b] * bp;
                            }
                            y *= 1.8f;
                            break;
                        }
                        default:
                        {
                            const float fb = std::min (0.97f, 0.2f + 0.77f * p.res);
                            for (int b = 0; b < 3; ++b)
                            {
                                auto& buf = comb[c][b];
                                const float rp = (float) combW[c] - combD[b];
                                const int i0 = (int) std::floor (rp);
                                const float fr = rp - (float) i0;
                                const float d0 = buf[(size_t) (i0 & 8191)], d1 = buf[(size_t) ((i0 + 1) & 8191)];
                                float d = d0 + fr * (d1 - d0);
                                combLp[c][b] += 0.45f * (d - combLp[c][b]);        // damping in the loop
                                d = combLp[c][b];
                                const float v = x + fb * d;
                                buf[(size_t) combW[c]] = v;
                                y += combAmp[b] * v;
                            }
                            combW[c] = (combW[c] + 1) & 8191;
                            y *= 0.30f * (1.0f - 0.5f * fb);
                            break;
                        }
                    }
                    *xs[c] = x * (1.0f - p.mix) + y * p.mix;
                }
            }
        }

    private:
        void updateCoeffs (const Params& p)
        {
            const float q = 0.6f + 19.4f * p.res * p.res;
            if (p.mode <= 3)
            {
                g = std::tan (3.14159265f * smoothFreq / sr);
                k = 1.0f / q;
                a1 = 1.0f / (1.0f + g * (g + k));
                ptrim = 1.0f / std::sqrt (std::max (1.0f, 0.5f * q));
            }
            else if (p.mode == 4)
            {
                // vowels A E I O U: first three formants (Hz) and how loud each is
                static const float F[5][3] = { { 800.0f, 1150.0f, 2900.0f }, { 400.0f, 1600.0f, 2700.0f }, { 270.0f, 2300.0f, 3000.0f },
                                               { 450.0f, 800.0f, 2830.0f }, { 325.0f, 700.0f, 2700.0f } };
                static const float A[3] = { 1.0f, 0.7f, 0.4f };
                const float pos = smoothVowel * 4.0f;
                const int i0 = std::min(3, (int) pos);
                const float fr = pos - (float) i0;
                const float shift = std::pow (juce::jlimit (0.5f, 2.0f, p.freqHz / 800.0f), 0.8f);
                const float Q = 4.0f + 14.0f * p.res;
                for (int b = 0; b < 3; ++b)
                {
                    const float f = std::exp (std::log (F[i0][b]) * (1.0f - fr) + std::log (F[i0 + 1][b]) * fr) * shift;
                    fg[b] = std::tan (3.14159265f * std::min (f, 0.45f * sr) / sr);
                    fk[b] = 1.0f / Q;
                    fa[b] = 1.0f / (1.0f + fg[b] * (fg[b] + fk[b]));
                    fAmp[b] = A[b];
                }
            }
            else
            {
                static const float ratio[3] = { 1.0f, 1.5f, 2.0f };
                static const float amp[3] = { 1.0f, 0.6f, 0.5f };
                for (int b = 0; b < 3; ++b)
                {
                    combD[b] = juce::jlimit (8.0f, 8000.0f, sr / (smoothFreq * ratio[b]));
                    combAmp[b] = amp[b];
                }
            }
        }

        float sr = 48000.0f;
        float g = 0.1f, k = 1.0f, a1 = 1.0f, ptrim = 1.0f;
        float ic1[2] { 0.0f, 0.0f }, ic2[2] { 0.0f, 0.0f };
        float fg[3] { 0.1f, 0.1f, 0.1f }, fk[3] { 0.2f, 0.2f, 0.2f }, fa[3] { 1.0f, 1.0f, 1.0f }, fAmp[3] { 1.0f, 0.7f, 0.4f };
        float f1[2][3] {}, f2[2][3] {};
        float lps[2] {}, bps[2] {};
        std::vector<float> comb[2][3];
        float combLp[2][3] {};  int combW[2] { 0, 0 };
        float combD[3] { 100.0f, 150.0f, 200.0f }, combAmp[3] { 1.0f, 0.6f, 0.5f };
        float lfoPhase = 0.0f, env = 0.0f, stepTarget = 0.0f, stepNow = 0.0f, smoothFreq = -1.0f, smoothVowel = -1.0f;
        int stepCount = 0;
        uint32_t rng = 22222u;
    };

    // =====================================================================================================================
    class VolumePanFx
    {
    public:
        struct Params { float levelDb = 0.0f; float pan = 0.0f; bool flip = false; };
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset() { gl = gr = -1.0f; }
        void process (float* l, float* r, int n, const Params& p)
        {
            const float lin = p.levelDb <= -59.5f ? 0.0f : dbToGain (p.levelDb);
            const float pol = p.flip ? -1.0f : 1.0f;
            float tl = lin * pol, tr = lin * pol;
            if (r != nullptr)
            {
                const float a = (juce::jlimit (-1.0f, 1.0f, p.pan) + 1.0f) * 0.78539816f;     // 0..pi/2
                tl *= std::cos (a) * 1.41421356f;
                tr *= std::sin (a) * 1.41421356f;
            }
            if (gl < 0.0f && gr < 0.0f) { gl = tl;  gr = tr; }
            const float s = 1.0f - std::exp (-1.0f / (0.010f * sr));
            for (int i = 0; i < n; ++i)
            {
                gl += s * (tl - gl);  gr += s * (tr - gr);
                l[i] *= gl;
                if (r != nullptr) r[i] *= gr;
            }
        }
    private:
        float sr = 48000.0f, gl = -1.0f, gr = -1.0f;
    };

    // =====================================================================================================================
    class MultibandCompFx
    {
    public:
        static constexpr int kBands = 4;
        struct Params
        {
            float xo[3] { 150.0f, 800.0f, 3500.0f };                 // crossover frequencies (Hz)
            float thr[kBands]     { -24.0f, -24.0f, -24.0f, -24.0f };  // per band: threshold (dB), ratio, attack and release (ms), makeup gain (dB)
            float ratio[kBands]   { 3.0f, 3.0f, 3.0f, 3.0f };
            float attackMs[kBands]  { 10.0f, 10.0f, 10.0f, 10.0f };
            float releaseMs[kBands] { 120.0f, 120.0f, 120.0f, 120.0f };
            float gainDb[kBands]  { 0.0f, 0.0f, 0.0f, 0.0f };
            float outDb = 0.0f, mix = 1.0f;
        };
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset()
        {
            for (auto& c : cx) c.reset();
            for (int b = 0; b < kBands; ++b) grDb[b] = 0.0f;
        }
        void process (float* l, float* r, int n, const Params& p)
        {
            const int chans = r != nullptr ? 2 : 1;
            float xo[3];
            Split4::ordered (p.xo, sr, xo);
            for (auto& c : cx) c.setCrossovers (xo, sr);
            float atk[kBands], rel[kBands], ratio[kBands], gain[kBands];
            for (int b = 0; b < kBands; ++b)
            {
                atk[b] = 1.0f - std::exp (-1.0f / (0.001f * std::max (0.1f, p.attackMs[b]) * sr));
                rel[b] = 1.0f - std::exp (-1.0f / (0.001f * std::max (5.0f, p.releaseMs[b]) * sr));
                ratio[b] = std::max (1.0f, p.ratio[b]);
                gain[b] = dbToGain (p.gainDb[b] + p.outDb);
            }
            const float mix = juce::jlimit (0.0f, 1.0f, p.mix);
            for (int i = 0; i < n; ++i)
            {
                float band[2][kBands];
                for (int c = 0; c < chans; ++c) cx[c].split (c == 0 ? l[i] : r[i], band[c]);
                // Each band is blended with ITSELF (not with the unsplit input), so Mix never meets a phase-shifted copy of the dry signal
                float sum[2] = { 0.0f, 0.0f };
                for (int b = 0; b < kBands; ++b)
                {
                    float pk = std::abs (band[0][b]);
                    if (chans == 2) pk = std::max (pk, std::abs (band[1][b]));
                    const float lvDb = 20.0f * std::log10 (pk + 1.0e-9f);
                    const float over = lvDb - p.thr[b];
                    float target;                             // gain reduction wanted (dB, positive)
                    if (over <= -3.0f) target = 0.0f;
                    else if (over < 3.0f) { const float t = over + 3.0f; target = (1.0f - 1.0f / ratio[b]) * t * t / 12.0f; }   // soft knee, 6 dB wide
                    else target = (1.0f - 1.0f / ratio[b]) * over;
                    grDb[b] += (target > grDb[b] ? atk[b] : rel[b]) * (target - grDb[b]);
                    const float g = 1.0f + mix * (dbToGain (-grDb[b]) * gain[b] - 1.0f);
                    for (int c = 0; c < chans; ++c) sum[c] += band[c][b] * g;
                }
                for (int c = 0; c < chans; ++c) (c == 0 ? l : r)[i] = sum[c];
            }
        }
        float grDb[kBands] { 0.0f, 0.0f, 0.0f, 0.0f };
    private:
        float sr = 48000.0f;
        Split4 cx[2];
    };

    // =====================================================================================================================
    // b55: four-band saturator. The same splitter as the compressor and the transient shaper; each band has its own Drive and Level.
    // Each band runs at twice the sample rate (a four-coefficient all-pass pair halfband filter, designed for under a sample of
    // delay) with first-order ADAA on top, so the harmonics a hard drive makes stay out of the audible band. A band's level at a typical guitar
    // peak (-12 dBFS) stays where it was whatever its Drive, so Drive changes the tone more than the loudness; Level trims it from there.
    // b63: every band can have its own curve (14 types: the five original curves, three tube/transistor shapes, two iron/tape types whose
    // saturation depends on frequency, three effect-like shapes and a very clean line transformer), its own Mix, a Tone tilt ahead of the curve and a Dynamics amount (drive
    // follows the playing level); the whole block has a Low Cut and High Cut on its output and an Auto level. With every new control at its
    // default the block sounds exactly as it did in b62.
    // b64: the one list of saturation curves, used by the Multiband Saturator, the Delay, the Reverb wet drive and the Compressor saturation menus
    inline juce::StringArray uniSatNames()
    {
        return { "Tape", "Tube", "Diode", "Transformer", "Hard clip", "Triode", "Pentode", "Germanium", "Iron (core)", "Tape + (bump)", "Foldback", "Rectify", "Bit crush", "Line Xfmr", "FET stage", "Opto tube stage" };
    }
    class MultibandSatFx
    {
    public:
        static constexpr int kBands = 4;
        // 0 Tape (tanh), 1 Tube (tanh, off-centre: even harmonics), 2 Diode (soft knee), 3 Transformer (smooth round-off), 4 Hard clip,
        // 5 Triode (asymmetric: even + odd), 6 Pentode (exponential knee: odd, edgy), 7 Germanium (soft on one side, long on the other),
        // 8 Iron (a transformer core: saturates more at low frequencies), 9 Tape + (head bump, and the highs squash first),
        // 10 Foldback (the peaks fold over), 11 Rectify (half-wave: octave-up), 12 Bit crush (coarse steps),
        // 13 Line Xfmr (an input transformer run well inside its headroom: the Tape curve 24 dB lower than Drive says, plus a tiny offset, so it adds only a
        //    whisper of third and second harmonic that grows with level; Drive 24 dB is the point where it starts to bite)
        //    b64 (shared with the Delay, Reverb wet drive and Compressor saturation menus): 14 FET stage (a symmetric tanh with a tighter knee and a lower ceiling: odd harmonics,
        //    the compressor's FET saturation), 15 Opto tube stage (a soft asymmetric tube curve with a long, gentle knee: even harmonics, the compressor's opto saturation)
        static constexpr int kTypes = 16;
        struct Params
        {
            float xo[3] { 150.0f, 800.0f, 3500.0f };
            float driveDb[kBands] { 12.0f, 12.0f, 12.0f, 12.0f };
            float levelDb[kBands] { 0.0f, 0.0f, 0.0f, 0.0f };
            int   type = 0;
            int   bandType[kBands] { -1, -1, -1, -1 };       // -1: the band uses the global type
            float bandMix[kBands] { 1.0f, 1.0f, 1.0f, 1.0f };        // times the global Mix
            float tone[kBands] { 0.0f, 0.0f, 0.0f, 0.0f };           // -1 to 1: a tilt ahead of the curve (about +/-6 dB at the band's edges, flat at its centre)
            float dyn[kBands] { 0.0f, 0.0f, 0.0f, 0.0f };            // -1 to 1: drive follows the band's level (+: louder drives harder, -: softer)
            float bias = 0.0f;                 // 0 to 1: pushes the curve off-centre (even harmonics) on top of the type's own
            float outDb = 0.0f, mix = 1.0f;
            float loCutHz = 20.0f, hiCutHz = 20000.0f;               // on the block's output; 20 Hz and 20 kHz are off
            bool  autoLevel = false;
        };
        bool adaa = true;                      // (tests switch this off to measure what the ADAA adds)
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset()
        {
            for (int c = 0; c < 2; ++c)
            {
                cx[c].reset();
                for (int b = 0; b < kBands; ++b) st[c][b] = Chan();
                loCut[c] = Bq();  hiCut[c] = Bq();
            }
            for (int b = 0; b < kBands; ++b) lastKey[b] = -1;
            eIn = eOut = 0.0;  alGain = 1.0;
        }
        // value, slope and antiderivative of each curve (all pass through 0 with slope 1, except Rectify, whose slope there is 1/2)
        static inline double logCosh (double u) { const double a = std::abs (u); return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453; }
        static constexpr double kTriA = 0.8, kTriB = 1.4, kGeC = 0.5, kQ = 32.0;
        struct Table
        {
            static constexpr int N = 4801;
            static constexpr double lim = 12.0, du = 2.0 * lim / (N - 1);
            double f[N], F[N];
            static double g (double u) { return std::tanh (0.5 * u + 0.5 * (std::sqrt (u * u + 0.09) - 0.3)); }
            Table()
            {
                for (int i = 0; i < N; ++i) f[i] = g (-lim + i * du);
                const int mid = (N - 1) / 2;
                F[mid] = 0.0;
                for (int i = mid + 1; i < N; ++i) F[i] = F[i - 1] + 0.5 * du * (f[i] + f[i - 1]);
                for (int i = mid - 1; i >= 0; --i) F[i] = F[i + 1] - 0.5 * du * (f[i] + f[i + 1]);
            }
            double fAt (double u) const
            {
                if (u <= -lim) return f[0];
                if (u >= lim) return f[N - 1];
                const double x = (u + lim) / du;  const int i = std::min (N - 2, (int) x);  const double t = x - i;
                return f[i] + t * (f[i + 1] - f[i]);
            }
            double FAt (double u) const
            {
                if (u <= -lim) return F[0] + f[0] * (u + lim);
                if (u >= lim) return F[N - 1] + f[N - 1] * (u - lim);
                const double x = (u + lim) / du;  const int i = std::min (N - 2, (int) x);  const double t = x - i;
                return F[i] + t * (F[i + 1] - F[i]);
            }
        };
        static const Table& table() { static const Table t; return t; }
        static inline double fcurve (int t, double u)
        {
            switch (t)
            {
                case 2:  return u / (1.0 + std::abs (u));
                case 3:  return u / std::sqrt (1.0 + u * u);
                case 4:  return u < -1.0 ? -1.0 : u > 1.0 ? 1.0 : u;
                case 5:  return u >= 0.0 ? kTriA * std::tanh (u / kTriA) : kTriB * std::tanh (u / kTriB);
                case 6:  return u >= 0.0 ? 1.0 - std::exp (-u) : std::exp (u) - 1.0;
                case 7:  return u >= 0.0 ? u / (1.0 + u) : u / (1.0 - kGeC * u);
                case 10: return std::sin (u);
                case 11: return table().fAt (u);
                case 12: { const double v = u < -1.0 ? -1.0 : u > 1.0 ? 1.0 : u;  return std::round (v * kQ) / kQ; }
                case 14: return std::tanh (1.4 * u) / 1.4;
                case 15: return 2.0 * std::tanh (0.5 * u);
                default: return std::tanh (u);
            }
        }
        static inline double dcurve (int t, double u)
        {
            switch (t)
            {
                case 2:  { const double d = 1.0 + std::abs (u); return 1.0 / (d * d); }
                case 3:  { const double q = 1.0 + u * u;  return 1.0 / (q * std::sqrt (q)); }
                case 4:  return std::abs (u) < 1.0 ? 1.0 : 0.0;
                case 5:  { const double th = std::tanh (u >= 0.0 ? u / kTriA : u / kTriB);  return 1.0 - th * th; }
                case 6:  return std::exp (-std::abs (u));
                case 7:  { const double d = u >= 0.0 ? 1.0 + u : 1.0 - kGeC * u;  return 1.0 / (d * d); }
                case 10: return std::cos (u);
                case 11: { const double h = Table::du;  return (table().fAt (u + h) - table().fAt (u - h)) / (2.0 * h); }
                case 12: return 1.0;
                case 14: { const double th = std::tanh (1.4 * u); return 1.0 - th * th; }
                case 15: { const double th = std::tanh (0.5 * u); return 1.0 - th * th; }
                default: { const double th = std::tanh (u); return 1.0 - th * th; }
            }
        }
        static inline double icurve (int t, double u)
        {
            switch (t)
            {
                case 2:  { const double a = std::abs (u);  return a - std::log1p (a); }
                case 3:  return std::sqrt (1.0 + u * u);
                case 4:  { const double a = std::abs (u);  return a <= 1.0 ? 0.5 * u * u : a - 0.5; }
                case 5:  return u >= 0.0 ? kTriA * kTriA * logCosh (u / kTriA) : kTriB * kTriB * logCosh (u / kTriB);
                case 6:  { const double a = std::abs (u);  return a - 1.0 + std::exp (-a); }
                case 7:  { if (u >= 0.0) return u - std::log1p (u);  const double a = -u;  return a / kGeC - std::log1p (kGeC * a) / (kGeC * kGeC); }
                case 10: return 1.0 - std::cos (u);
                case 11: return table().FAt (u);
                case 14: return logCosh (1.4 * u) / 1.96;
                case 15: return 4.0 * logCosh (0.5 * u);
                default: return logCosh (u);
            }
        }
        // 8 (Iron) and 9 (Tape +) use the Tape curve; their frequency dependence is in the filters around it
        static inline int curveOf (int type) { return (type == 8 || type == 9) ? 0 : type; }
        // The offset the type and Bias put on the curve; Hard clip keeps it small enough that the slope there is 1
        static inline double offsetFor (int type, float bias) { return ((type == 1 || type == 15) ? 0.35 : 0.0) + 0.5 * (double) juce::jlimit (0.0f, 1.0f, bias) * (type == 4 ? 0.9 : 1.0); }
        // What a band does to a level of 0.25 (-12 dBFS) at Drive g, relative to Drive 0 dB: divided out, so Drive does not change the loudness there
        static inline double refGain (int type, double off, double g)
        {
            const double r = 0.25;
            const double s1 = 0.5 * (fcurve (type, r + off) - fcurve (type, -r + off));
            const double sg = 0.5 * (fcurve (type, g * r + off) - fcurve (type, -g * r + off));
            return s1 / std::max (1.0e-6, std::abs (sg));
        }
        void process (float* l, float* r, int n, const Params& p)
        {
            const int chans = r != nullptr ? 2 : 1;
            double eInBlock = 0.0;
            if (p.autoLevel)
                for (int c = 0; c < chans; ++c) { const float* x = c == 0 ? l : r;  for (int i = 0; i < n; ++i) eInBlock += (double) x[i] * x[i]; }

            float xo[3];
            Split4::ordered (p.xo, sr, xo);
            for (auto& c : cx) c.setCrossovers (xo, sr);
            const double dcCoef = 1.0 - std::exp (-6.283185307 * 6.0 / (2.0 * (double) sr));
            const double mixG = juce::jlimit (0.0f, 1.0f, p.mix);
            const bool useAdaa = adaa;
            const double tau = 6.283185307 / (double) sr;
            const double centre[kBands] { 0.5 * xo[0], std::sqrt ((double) xo[0] * xo[1]), std::sqrt ((double) xo[1] * xo[2]), std::min (16000.0, 1.5 * (double) xo[2]) };

            struct Cfg
            {
                int type, ct;  double off, f0, d0, k, g, lv, mix;  bool blockDc, adaa;
                int emph;  double emA, emAlpha;                  // 0 none, 1 low (Iron), 2 high (Tape +)
                bool bump;
                double toneG, toneAlpha;  bool tone;
                double dyn;  bool useDyn;
            } B[kBands];
            for (int b = 0; b < kBands; ++b)
            {
                Cfg& q = B[b];
                q.type = juce::jlimit (0, kTypes - 1, p.bandType[b] >= 0 ? p.bandType[b] : p.type);
                q.ct = curveOf (q.type);
                q.off = offsetFor (q.ct, p.bias) + (q.type == 13 ? 0.004 : 0.0);
                const int key = q.type * 1000 + (int) std::lround (q.off * 1000.0);
                if (key != lastKey[b]) { for (auto& ch : st) ch[b].primed = false;  lastKey[b] = key; }
                q.f0 = fcurve (q.ct, q.off);
                q.d0 = std::max (1.0e-3, dcurve (q.ct, q.off));
                q.blockDc = q.off > 1.0e-6 || q.ct == 5 || q.ct == 7 || q.ct == 11;
                q.adaa = useAdaa && q.ct != 12;
                q.g = (double) dbToGain (p.driveDb[b] - (q.type == 13 ? 24.0f : 0.0f));
                q.k = (q.ct == 10 ? refGain (0, 0.0, q.g) : refGain (q.ct, q.off, q.g)) / q.d0;          // Foldback has no steady level to divide out: it borrows the Tape curve's
                q.lv = (double) dbToGain (p.levelDb[b] + p.outDb);
                q.mix = juce::jlimit (0.0, 1.0, mixG * (double) juce::jlimit (0.0f, 1.0f, p.bandMix[b]));
                q.emph = q.type == 8 ? 1 : q.type == 9 ? 2 : 0;
                q.emA = q.type == 8 ? 2.0 : 3.0;
                q.emAlpha = 1.0 - std::exp (-tau * (q.type == 8 ? 110.0 : 2500.0));
                q.bump = q.type == 9;
                q.tone = std::abs (p.tone[b]) > 0.002f;
                q.toneG = std::pow (10.0, 6.0 * (double) juce::jlimit (-1.0f, 1.0f, p.tone[b]) / 20.0);
                q.toneAlpha = 1.0 - std::exp (-tau * centre[b]);
                q.dyn = 0.5 * (double) juce::jlimit (-1.0f, 1.0f, p.dyn[b]);
                q.useDyn = std::abs (p.dyn[b]) > 0.002f;
            }
            const double bumpAlpha = 1.0 - std::exp (-tau * 75.0);
            const double envRel = 1.0 - std::exp (-1.0 / (0.06 * (double) sr));

            auto shape = [&] (Chan& a, const Cfg& q, double gEff, double s) -> double          // one sample at the double rate
            {
                const double u = gEff * s + q.off;
                double y;
                if (! q.adaa) y = fcurve (q.ct, u);
                else
                {
                    // ADAA on the part of the curve that is not a straight line (r = f(u) - u, with antiderivative F - u^2/2): the straight part
                    // is passed exactly, so small signals keep their highs and their timing, and only the bend gets the half-sample smoothing
                    const double R = icurve (q.ct, u) - 0.5 * u * u;
                    if (! a.primed) { y = fcurve (q.ct, u);  a.primed = true; }
                    else
                    {
                        const double du = u - a.u;
                        const double mid = 0.5 * (u + a.u);
                        y = u + (std::abs (du) < 1.0e-5 ? fcurve (q.ct, mid) - mid : (R - a.F) / du);
                    }
                    a.u = u;  a.F = R;
                }
                double o = (y - q.f0) * q.k;
                if (q.blockDc) { a.dc += dcCoef * (o - a.dc);  o -= a.dc; }
                return (1.0 - q.mix) * s + q.mix * q.lv * o;                    // blended at the double rate, so the dry part takes the same filters
            };
            for (int i = 0; i < n; ++i)
            {
                float band[2][kBands];
                for (int c = 0; c < chans; ++c) cx[c].split (c == 0 ? l[i] : r[i], band[c]);
                for (int c = 0; c < chans; ++c)
                {
                    double sum = 0.0;
                    for (int b = 0; b < kBands; ++b)
                    {
                        Chan& a = st[c][b];
                        const Cfg& q = B[b];
                        double x = (double) band[c][b];
                        if (q.tone)                                                // tilt ahead of the curve
                        {
                            a.toneZ += q.toneAlpha * (x - a.toneZ);
                            x = a.toneZ / q.toneG + (x - a.toneZ) * q.toneG;          // lows down and highs up (or the reverse) around the band's centre
                        }
                        double gEff = q.g;
                        if (q.useDyn)                                              // drive follows the level
                        {
                            const double ax = std::abs (x);
                            a.env = ax > a.env ? ax : a.env + envRel * (ax - a.env);
                            gEff *= std::pow (juce::jlimit (0.05, 8.0, a.env * 4.0), q.dyn);
                        }
                        if (q.emph == 1)       { a.preZ += q.emAlpha * (x - a.preZ);  x += q.emA * a.preZ; }                    // low boost: the core saturates at the lows first
                        else if (q.emph == 2)  { a.preZ += q.emAlpha * (x - a.preZ);  x = (1.0 + q.emA) * x - q.emA * a.preZ; }  // high boost: the highs squash first
                        const double se = Ap::run (a.up0, kC0, x), so = Ap::run (a.up1, kC1, x);          // the two phases of the doubled signal
                        const double ve = shape (a, q, gEff, se), vo = shape (a, q, gEff, so);
                        double y = 0.5 * (Ap::run (a.dn0, kC0, ve) + Ap::run (a.dn1, kC1, a.voPrev));
                        a.voPrev = vo;
                        if (q.emph == 1)       // the exact inverse of the boost above, so the band is flat where it is not saturating
                        {
                            const double xr = (y - q.emA * (1.0 - q.emAlpha) * a.postZ) / (1.0 + q.emA * q.emAlpha);
                            a.postZ += q.emAlpha * (xr - a.postZ);  y = xr;
                        }
                        else if (q.emph == 2)
                        {
                            const double xr = (y + q.emA * (1.0 - q.emAlpha) * a.postZ) / (1.0 + q.emA - q.emA * q.emAlpha);
                            a.postZ += q.emAlpha * (xr - a.postZ);  y = xr;
                        }
                        if (q.bump) { a.bumpZ += bumpAlpha * (y - a.bumpZ);  y += 0.35 * a.bumpZ; }
                        sum += y;
                    }
                    (c == 0 ? l : r)[i] = (float) sum;
                }
            }

            // Low Cut and High Cut on the output, then Auto level
            const bool lo = p.loCutHz > 21.0f, hi = p.hiCutHz < 19500.0f;
            if (lo) loCut[0].set (false, juce::jlimit (20.0f, 1000.0f, p.loCutHz), sr);
            if (hi) hiCut[0].set (true, juce::jlimit (1000.0f, 0.45f * sr, p.hiCutHz), sr);
            if (lo || hi)
                for (int c = 0; c < chans; ++c)
                {
                    float* x = c == 0 ? l : r;
                    if (lo) loCut[c].copyCoefs (loCut[0]);
                    if (hi) hiCut[c].copyCoefs (hiCut[0]);
                    for (int i = 0; i < n; ++i)
                    {
                        double v = x[i];
                        if (lo) v = loCut[c].run (v);
                        if (hi) v = hiCut[c].run (v);
                        x[i] = (float) v;
                    }
                }
            if (p.autoLevel)
            {
                double eOutBlock = 0.0;
                for (int c = 0; c < chans; ++c) { const float* x = c == 0 ? l : r;  for (int i = 0; i < n; ++i) eOutBlock += (double) x[i] * x[i]; }
                const double ca = 1.0 - std::exp (-(double) n / (0.4 * (double) sr));
                eIn += ca * (eInBlock - eIn);  eOut += ca * (eOutBlock - eOut);
                const double target = eIn > 1.0e-9 && eOut > 1.0e-12 ? juce::jlimit (0.04, 16.0, std::sqrt (eIn / eOut)) : alGain;
                const double g0 = alGain, g1 = alGain + 0.5 * (target - alGain);
                for (int i = 0; i < n; ++i)
                {
                    const double gg = g0 + (g1 - g0) * ((double) (i + 1) / (double) n);
                    l[i] = (float) (l[i] * gg);  if (chans == 2) r[i] = (float) (r[i] * gg);
                }
                alGain = g1;
            }
            else { eIn = eOut = 0.0;  alGain = 1.0; }
        }
    private:
        // halfband filter as two all-pass chains (two sections each); coefficients from a minimax search, 65 dB down from 28 kHz at a 96 kHz rate
        static constexpr double kC0[2] = { 0.577019203646, 0.089551237167 }, kC1[2] = { 0.310572181672, 0.850529686886 };
        struct Ap
        {
            double x1[2] = { 0, 0 }, y1[2] = { 0, 0 };
            static inline double run (Ap& a, const double (&c)[2], double x)
            {
                for (int s = 0; s < 2; ++s) { const double y = c[s] * (x - a.y1[s]) + a.x1[s];  a.x1[s] = x;  a.y1[s] = y;  x = y; }
                return x;
            }
        };
        struct Chan { Ap up0, up1, dn0, dn1; double voPrev = 0.0, u = 0.0, F = 0.0, dc = 0.0; bool primed = false;
                      double toneZ = 0.0, env = 0.0, preZ = 0.0, postZ = 0.0, bumpZ = 0.0; };
        struct Bq        // second-order Butterworth (RBJ), direct form II transposed
        {
            double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
            void set (bool lowpass, float f, float sr)
            {
                const double w = 6.283185307179586 * (double) f / (double) sr, cw = std::cos (w), al = std::sin (w) / (2.0 * 0.70710678);
                const double a0 = 1.0 + al;
                if (lowpass) { b0 = 0.5 * (1.0 - cw) / a0;  b1 = (1.0 - cw) / a0;  b2 = b0; }
                else         { b0 = 0.5 * (1.0 + cw) / a0;  b1 = -(1.0 + cw) / a0;  b2 = b0; }
                a1 = -2.0 * cw / a0;  a2 = (1.0 - al) / a0;
            }
            void copyCoefs (const Bq& o) { b0 = o.b0;  b1 = o.b1;  b2 = o.b2;  a1 = o.a1;  a2 = o.a2; }
            double run (double x) { const double y = b0 * x + z1;  z1 = b1 * x - a1 * y + z2;  z2 = b2 * x - a2 * y;  return y; }
        };
        float sr = 48000.0f;
        int lastKey[kBands] { -1, -1, -1, -1 };
        Split4 cx[2];
        Chan st[2][kBands];
        Bq loCut[2], hiCut[2];
        double eIn = 0.0, eOut = 0.0, alGain = 1.0;
    };

    // =====================================================================================================================
    class GateExpFx
    {
    public:
        struct Params
        {
            float threshDb = -50.0f, rangeDb = -60.0f;       // range: how far it closes (negative dB)
            float attackMs = 2.0f, holdMs = 60.0f, releaseMs = 150.0f;
            int   mode = 0;                                  // 0 gate, 1 expander (soft, 2:1)
            float detHpHz = 80.0f;                           // detector high-pass: keeps rumble from holding it open
        };
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset() { det = 0.0f; hpState = 0.0f; hold = 0; open = false; grDb = 0.0f; }
        void process (float* l, float* r, int n, const Params& p, const float* key = nullptr)    // b34: key = a separate signal to listen to (or null = listen to this one)
        {
            const float hpC = std::exp (-6.2831853f * std::max (10.0f, p.detHpHz) / sr);
            const float dAtk = 1.0f - std::exp (-1.0f / (0.0005f * sr)), dRel = 1.0f - std::exp (-1.0f / (0.02f * sr));
            const float atk = 1.0f - std::exp (-1.0f / (0.001f * std::max (0.05f, p.attackMs) * sr));
            const float rel = 1.0f - std::exp (-1.0f / (0.001f * std::max (5.0f, p.releaseMs) * sr));
            const int holdSamples = (int) (0.001f * p.holdMs * sr);
            const float range = std::max (-90.0f, std::min (0.0f, p.rangeDb));
            for (int i = 0; i < n; ++i)
            {
                float x = key != nullptr ? key[i] : (r != nullptr ? 0.5f * (l[i] + r[i]) : l[i]);
                hpState = hpC * hpState + (1.0f - hpC) * x;      // one-pole high-pass for the detector
                x -= hpState;
                const float a = std::abs (x);
                det += (a > det ? dAtk : dRel) * (a - det);
                const float lvDb = 20.0f * std::log10 (det + 1.0e-9f);
                float target;                                    // gain reduction wanted (dB, positive)
                if (p.mode == 0)
                {
                    if (lvDb > p.threshDb) { open = true;  hold = holdSamples; }
                    else if (lvDb < p.threshDb - 4.0f)           // 4 dB of hysteresis: no chatter around the threshold
                    {
                        if (hold > 0) --hold; else open = false;
                    }
                    target = open ? 0.0f : -range;
                }
                else
                {
                    if (lvDb > p.threshDb) { hold = holdSamples;  target = 0.0f; }
                    else if (hold > 0) { --hold;  target = 0.0f; }
                    else target = std::min (-range, (p.threshDb - lvDb) * 1.0f);   // 2:1 below the threshold
                }
                grDb += (target > grDb ? rel : atk) * (target - grDb);    // closing is the release, opening the attack
                const float gain = dbToGain (-grDb);
                l[i] *= gain;
                if (r != nullptr) r[i] *= gain;
            }
        }
        float grDb = 0.0f;
    private:
        float sr = 48000.0f, det = 0.0f, hpState = 0.0f;
        int hold = 0;
        bool open = false;
    };

    // =====================================================================================================================
    // b53: transient shaper. Two envelope followers on the (high-passed) signal, a fast one and a slow one. When the fast one runs ahead of
    // the slow one by more than a couple of dB the signal is at the start of a note (the attack region), otherwise it is in the body or the
    // tail (the sustain region). Attack and Sustain are dB amounts for the two regions (-12 to +12 dB), blended smoothly. The decision uses
    // the ratio of the two followers, so it does not depend on how loud you play. No look-ahead, so no latency.
    class TransientFx
    {
    public:
        struct Params
        {
            float attack = 0.0f, sustain = 0.0f;            // -1 to +1 (x 12 dB)
            float body = 0.0f;                               // b56: -1 to +1 (x 12 dB): the stretch just after the attack, while the note is still near its peak level
            float speed = 0.5f;                              // 0 slow (long attack region) to 1 fast
            float focusHz = 100.0f;                          // detector high-pass: low notes do not ripple the followers
            float outDb = 0.0f, mix = 1.0f;
            float timeScale = 1.0f;                          // b54: slows every detector time constant (the low bands of the multiband shaper use more than 1)
        };
        struct Coefs { float fAtk, fRel, sAtk, sRel, dC, gC, hpC, aDb, sDb, bDb, pDec; };
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset() { hp = 0.0f; fast = slow = 1.0e-6f; peak = 1.0e-6f; dSm = 0.0f; gSm = 0.0f; grDb = 0.0f; }
        Coefs makeCoefs (const Params& p) const
        {
            const float ts = std::max (1.0f, p.timeScale);
            const float tauSlow = ts * 10.0f * std::pow (8.0f, 1.0f - juce::jlimit (0.0f, 1.0f, p.speed));      // ms: 10 (fast) to 80 (slow)
            auto coef = [this] (float ms) { return 1.0f - std::exp (-1.0f / (0.001f * ms * sr)); };
            Coefs c;
            c.fAtk = coef (0.3f);  c.fRel = coef (std::max (8.0f * ts, 0.35f * tauSlow));
            c.sAtk = coef (tauSlow);  c.sRel = coef (4.0f * tauSlow);
            c.dC = coef (3.0f);  c.gC = coef (1.0f);
            c.hpC = std::exp (-6.2831853f * juce::jlimit (20.0f, 1000.0f, p.focusHz) / sr);
            c.aDb = 12.0f * juce::jlimit (-1.0f, 1.0f, p.attack);  c.sDb = 12.0f * juce::jlimit (-1.0f, 1.0f, p.sustain);
            c.bDb = 12.0f * juce::jlimit (-1.0f, 1.0f, p.body);
            c.pDec = std::exp (-1.0f / (0.001f * 1000.0f * ts * sr));       // the note's peak level (kept by the Body stage) fades slowly, over about a second
            return c;
        }
        // One sample of the detector: x is the (mono) signal the shaper listens to; returns the linear gain to apply (before Output and Mix).
        inline float stepGain (float x, const Coefs& c)
        {
            hp = c.hpC * hp + (1.0f - c.hpC) * x;
            x -= hp;
            const float a = std::abs (x) + 1.0e-9f;
            fast += (a > fast ? c.fAtk : c.fRel) * (a - fast);
            slow += (a > slow ? c.sAtk : c.sRel) * (a - slow);
            const float dDb = 20.0f * std::log10 (fast / slow);
            dSm += c.dC * (dDb - dSm);
            float w = juce::jlimit (0.0f, 1.0f, (dSm - 2.0f) / 6.0f);       // 0 below a 2 dB lead, 1 at 8 dB: the attack region
            w = w * w * (3.0f - 2.0f * w);
            const float lvl = 20.0f * std::log10 (slow + 1.0e-9f);              // below about -75 dBFS a boost would only lift noise
            const float quiet = (c.sDb > 0.0f || c.bDb > 0.0f) ? juce::jlimit (0.0f, 1.0f, (lvl + 75.0f) / 15.0f) : 1.0f;
            // b56 Body: how far the slow follower has fallen from the note's recent peak. Within about 1 dB of it (the first part of the note) the
            // Body setting applies in full on top of Sustain; by 8 dB down (the tail) it no longer does. It never touches the attack region.
            float bw = 0.0f;
            if (c.bDb != 0.0f)
            {
                peak = std::max (slow, peak * c.pDec);
                const float downDb = 20.0f * std::log10 (peak / (slow + 1.0e-12f));
                bw = 1.0f - juce::jlimit (0.0f, 1.0f, (downDb - 1.0f) / 7.0f);
                bw = bw * bw * (3.0f - 2.0f * bw);
            }
            const float target = c.aDb * w + (c.sDb + c.bDb * bw) * (1.0f - w) * quiet;
            gSm += c.gC * (target - gSm);
            grDb = gSm;
            return dbToGain (gSm);
        }
        void process (float* l, float* r, int n, const Params& p)
        {
            const Coefs c = makeCoefs (p);
            const float out = dbToGain (p.outDb), mix = juce::jlimit (0.0f, 1.0f, p.mix);
            for (int i = 0; i < n; ++i)
            {
                const float x = r != nullptr ? 0.5f * (l[i] + r[i]) : l[i];
                const float g = stepGain (x, c);
                const float wetGain = g * out;
                const float gain = 1.0f + mix * (wetGain - 1.0f);
                l[i] *= gain;
                if (r != nullptr) r[i] *= gain;
            }
        }
        float grDb = 0.0f;                                   // the gain being applied (dB), for a readout
    private:
        float sr = 48000.0f, hp = 0.0f, fast = 1.0e-6f, slow = 1.0e-6f, peak = 1.0e-6f, dSm = 0.0f, gSm = 0.0f;
    };

    // =====================================================================================================================
    // b54: the transient shaper in four bands. Three Linkwitz-Riley crossovers split the signal into four bands; each band has its own
    // detector (the same one as above, listening to that band alone) and its own Attack and Sustain, and the bands are added back together
    // (out = sum of band_k x gain_k). The split is a tree: the middle crossover first, then each half is split again, and each half also goes
    // through the all-pass of the other half's crossover, so that with every band at 0 dB the sum is flat (a pure phase change, no level change).
    // Lower bands use slower detectors (their waves are longer).
    class MultiTransientFx
    {
    public:
        static constexpr int kBands = 4;
        struct Params
        {
            float attack[kBands] = { 0, 0, 0, 0 }, sustain[kBands] = { 0, 0, 0, 0 }, body[kBands] = { 0, 0, 0, 0 };     // -1 to +1 (x 12 dB), per band
            float xo[3] = { 150.0f, 800.0f, 3500.0f };                                      // crossover frequencies (Hz)
            float speed = 0.5f, outDb = 0.0f, mix = 1.0f;
        };
        void prepare (double sampleRate)
        {
            sr = (float) sampleRate;
            for (auto& b : band) b.prepare (sampleRate);
            reset();
        }
        void reset()
        {
            for (auto& b : band) b.reset();
            for (auto& ch : cx) ch.reset();
        }
        static void orderedCrossovers (const float in[3], float sampleRate, float out[3]) { Split4::ordered (in, sampleRate, out); }
        void process (float* l, float* r, int n, const Params& p)
        {
            const int chans = r != nullptr ? 2 : 1;
            float xo[3];
            orderedCrossovers (p.xo, sr, xo);
            for (auto& c : cx) c.setCrossovers (xo, sr);
            // lower edges of the bands, for the detector speed: waves below about 150 Hz need longer windows
            const float edge[kBands] = { 0.6f * xo[0], xo[0], xo[1], xo[2] };
            TransientFx::Coefs co[kBands];
            for (int k = 0; k < kBands; ++k)
            {
                TransientFx::Params tp;
                tp.attack = p.attack[k];  tp.sustain = p.sustain[k];  tp.body = p.body[k];  tp.speed = p.speed;  tp.focusHz = 20.0f;
                tp.timeScale = juce::jlimit (1.0f, 4.0f, 150.0f / std::max (30.0f, edge[k]));
                co[k] = band[k].makeCoefs (tp);
            }
            const float out = dbToGain (p.outDb), mix = juce::jlimit (0.0f, 1.0f, p.mix);
            for (int i = 0; i < n; ++i)
            {
                float bs[2][kBands];
                for (int c = 0; c < chans; ++c)
                {
                    cx[c].split ((c == 0 ? l : r)[i], bs[c]);
                }
                float g[kBands];                                                        // each band's gain with Output and Mix folded in
                for (int k = 0; k < kBands; ++k)
                {
                    const float det = chans == 2 ? 0.5f * (bs[0][k] + bs[1][k]) : bs[0][k];
                    g[k] = 1.0f + mix * (band[k].stepGain (det, co[k]) * out - 1.0f);
                }
                for (int c = 0; c < chans; ++c)
                    (c == 0 ? l : r)[i] = g[0] * bs[c][0] + g[1] * bs[c][1] + g[2] * bs[c][2] + g[3] * bs[c][3];
            }
        }
        float bandGrDb (int k) const { return band[juce::jlimit (0, kBands - 1, k)].grDb; }
    private:
        float sr = 48000.0f;
        TransientFx band[kBands];
        Split4 cx[2];
    };

    // =====================================================================================================================
    // b29: step slicer. An eight-step gate pattern that repeats; each step lasts one cycle of the rate. The steps fade in and out
    // over a couple of milliseconds so they do not click.
    class SlicerFx
    {
    public:
        struct Params { int pattern = 0; float rateHz = 4.0f; float depth = 1.0f; float smoothMs = 3.0f; float mix = 1.0f; };
        static constexpr int kPatterns = 8;
        static unsigned patternBits (int i)
        {
            static const unsigned bits[kPatterns] = { 0xAA, 0xCC, 0x92, 0xEE, 0xA6, 0xF0, 0x88, 0xDA };    // bit 7 is step 1
            return bits[juce::jlimit (0, kPatterns - 1, i)];
        }
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset() { phase = 0.0f; gain = 1.0f; }
        void process (float* l, float* r, int n, const Params& p)
        {
            const unsigned bits = patternBits (p.pattern);
            const float rate = juce::jlimit (0.05f, 40.0f, p.rateHz);
            const float coef = 1.0f - std::exp (-1.0f / (std::max (0.2f, p.smoothMs) * 0.001f * sr));
            const float depth = juce::jlimit (0.0f, 1.0f, p.depth), mix = juce::jlimit (0.0f, 1.0f, p.mix);
            for (int i = 0; i < n; ++i)
            {
                const int step = (int) (phase * 8.0f) & 7;
                const bool on = ((bits >> (7 - step)) & 1u) != 0;
                const float target = on ? 1.0f : 1.0f - depth;
                gain += coef * (target - gain);
                const float g = 1.0f - mix + mix * gain;
                l[i] *= g;
                if (r != nullptr) r[i] *= g;
                phase += rate / (8.0f * sr);              // one pass through the pattern = eight steps = eight cycles of the rate
                if (phase >= 1.0f) phase -= 1.0f;
            }
        }
    private:
        float sr = 48000.0f, phase = 0.0f, gain = 1.0f;
    };

    // =====================================================================================================================
    // b32: spring reverb tank. Two springs of different length, each a delay line in a feedback loop. A spring is dispersive:
    // low frequencies travel slower than high ones, so a pluck arrives as a falling chirp (the "drip" and "boing"). That is built
    // with a long cascade of first-order all-pass sections (coefficient -0.7) inside each loop. The loop loses level and top
    // end on every trip, and the pickup at the far end only hears the middle of the spectrum. Own design, not matched to a unit.
    class SpringTank
    {
    public:
        void prepare (double sampleRate)
        {
            sr = (float) sampleRate;
            sections = juce::jlimit (40, 200, (int) std::lround (100.0 * sampleRate / 48000.0));
            int n = 1;  while ((double) n < 0.12 * sampleRate) n <<= 1;
            mask = n - 1;
            for (int k = 0; k < 2; ++k)
            {
                buf[k].assign ((size_t) n, 0.0f);
                z[k].assign ((size_t) 200, 0.0f);
                delayLen[k] = std::max (16, (int) ((k == 0 ? 0.0335 : 0.0437) * sampleRate) - (int) (0.006 * sampleRate));   // the all-pass chain adds about 6 ms
            }
            setParams (2.0f, 0.5f);
            reset();
        }
        void reset()
        {
            for (int k = 0; k < 2; ++k) { std::fill (buf[k].begin(), buf[k].end(), 0.0f);  std::fill (z[k].begin(), z[k].end(), 0.0f);  lpFb[k] = 0.0f; }
            w = 0;  inLp = outHp = outLp = 0.0f;
        }
        // decaySeconds: time for the tail to fall by 60 dB. damping 0..1: how quickly the top end is lost on each trip.
        void setParams (float decaySeconds, float damping)
        {
            const float decay = juce::jlimit (0.2f, 12.0f, decaySeconds);
            for (int k = 0; k < 2; ++k)
            {
                const float loopTime = (float) (delayLen[k] + (int) (0.006f * sr)) / sr;
                g[k] = juce::jlimit (0.0f, 0.985f, std::pow (10.0f, -3.0f * loopTime / decay));
            }
            const float hz = 6500.0f - 4200.0f * juce::jlimit (0.0f, 1.0f, damping);
            lpC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * hz / sr);
            inC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 5000.0f / sr);
            hpC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 120.0f / sr);
            outC = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 4800.0f / sr);
        }
        void process (float in, float& l, float& r)
        {
            inLp += inC * (in - inLp);                        // the driver and the spring's ends only pass the middle of the spectrum
            float tap[2];
            for (int k = 0; k < 2; ++k)
            {
                float x = 0.35f * inLp + g[k] * lpFb[k];
                float* zs = z[k].data();
                constexpr float a = -0.7f;
                for (int j = 0; j < sections; ++j)             // dispersion: low frequencies are delayed more than high ones
                {
                    const float y = a * x + zs[j];
                    zs[j] = x - a * y;
                    x = y;
                }
                buf[k][(size_t) (w & mask)] = x;
                const float d = buf[k][(size_t) ((w - delayLen[k]) & mask)];
                lpFb[k] += lpC * (d - lpFb[k]);
                tap[k] = d;
            }
            ++w;
            float a0 = tap[0] + 0.45f * tap[1], a1 = tap[1] + 0.45f * tap[0];
            const float m = 0.5f * (a0 + a1);
            outHp += hpC * (m - outHp);                        // the pickup does not hear the lows
            outLp += outC * (m - outLp - 0.0f);
            const float band = outLp - outHp;
            const float side = 0.5f * (a0 - a1);
            l = (band + side) * outGain;
            r = (band - side) * outGain;
        }
        float outGain = 1.6f;
    private:
        float sr = 48000.0f, g[2] { 0.9f, 0.9f }, lpC = 0.5f, inC = 0.5f, hpC = 0.01f, outC = 0.5f;
        float lpFb[2] { 0.0f, 0.0f }, inLp = 0.0f, outHp = 0.0f, outLp = 0.0f;
        int sections = 100, mask = 0, w = 0, delayLen[2] { 1000, 1300 };
        std::vector<float> buf[2], z[2];
    };

    // =====================================================================================================================
    // b33: the electronics around a spring tank. The tank (SpringTank, or the old Spring type) is the same; what changes is how the
    // send is shaped before it and how the return is filtered after it. Each style is read off one amp's reverb circuit
    // (see CIRCUITS.md): the return high-pass is the coupling capacitor into the Reverb pot, the rest is the pickup and gain stages.
    //   0 Generic (nothing), 1 Twin (530 Hz, pickup sizzle), 2 AC30 (1.6 kHz), 3 Chieftain (two stages near 400 Hz, send high-pass),
    //   4 Full (almost no high-pass), 5 Clean (solid-state current drive: flat, and the driver does not clip), 6 Tilt (treble-tilted op-amp).
    class SpringReturn
    {
    public:
        void prepare (double sampleRate) { fs = (float) sampleRate; cur = -1; setStyle (0); }
        void reset()
        {
            for (auto& c : hpS) for (auto& v : c) v = 0.0f;
            for (auto& c : bq) for (auto& b : c) b.reset();
            sendHpS = 0.0f;  sendBq.reset();
        }
        bool linearDriver() const { return cur == 5; }
        void setStyle (int s)
        {
            s = std::max (0, std::min (6, s));
            if (s == cur) return;
            cur = s;  reset();
            nHp = 0;  nBq = 0;  sendKind = 0;  trim = 1.0f;
            auto hp = [this] (float f) { hpC[nHp++] = 1.0f - std::exp (-6.28318531f * f / fs); };
            switch (s)
            {
                case 1: hp (530.0f);  peak (0, 4000.0f, 1.2f, 4.0f);  trim = 1.0f;  break;
                case 2: hp (1600.0f);  trim = 1.41f;  break;        // +3 dB: the high-pass takes out most of the energy, measured -4 dB
                case 3: hp (329.0f);  hp (226.0f);  sendKind = 1;  sendC = 1.0f - std::exp (-6.28318531f * 284.0f / fs);  trim = 1.12f;  break;
                case 4: hp (55.0f);  trim = 1.0f;  break;
                case 5: hp (160.0f);  trim = 1.0f;  break;
                case 6: hp (362.0f);  shelf (0, false, 800.0f, -8.0f);  shelf (1, true, 7000.0f, -8.0f);  sendKind = 2;  sendBq.shelf (fs, 1500.0f, 12.0f, true);  trim = 0.56f;  break;      // -5 dB: the lifted send and the tilt measured +3 to +7 dB
                default: break;
            }
        }
        int style() const { return cur; }
        inline float send (float x)
        {
            if (sendKind == 1) { sendHpS += sendC * (x - sendHpS);  return x - sendHpS; }
            if (sendKind == 2) return sendBq.process (x);
            return x;
        }
        inline void ret (float& l, float& r)
        {
            if (cur <= 0) return;
            float* xs[2] = { &l, &r };
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = *xs[ch];
                for (int k = 0; k < nHp; ++k) { hpS[ch][k] += hpC[k] * (x - hpS[ch][k]);  x -= hpS[ch][k]; }
                for (int k = 0; k < nBq; ++k) x = bq[ch][k].process (x);
                *xs[ch] = x * trim;
            }
        }
        float trim = 1.0f;
        void setTrim (float t) { trim = t; }
    private:
        struct Bq
        {
            float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
            void reset() { z1 = z2 = 0.0f; }
            inline float process (float x) { const float y = b0 * x + z1;  z1 = b1 * x - a1 * y + z2;  z2 = b2 * x - a2 * y;  return y; }
            void set (float B0, float B1, float B2, float A0, float A1, float A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
            void peak (float fs, float f, float q, float db)
            {
                const float A = std::pow (10.0f, db / 40.0f), w = 6.28318531f * f / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
                set (1.0f + al * A, -2.0f * c, 1.0f - al * A, 1.0f + al / A, -2.0f * c, 1.0f - al / A);
            }
            void shelf (float fs, float f, float db, bool high)
            {
                const float A = std::pow (10.0f, db / 40.0f), w = 6.28318531f * f / fs, c = std::cos (w), al = std::sin (w) * 0.70710678f, t = 2.0f * std::sqrt (A) * al;
                if (high) set (A * ((A + 1) + (A - 1) * c + t), -2.0f * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
                               (A + 1) - (A - 1) * c + t, 2.0f * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
                else      set (A * ((A + 1) - (A - 1) * c + t), 2.0f * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
                               (A + 1) + (A - 1) * c + t, -2.0f * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
            }
        };
        void peak (int k, float f, float q, float db) { for (int ch = 0; ch < 2; ++ch) bq[ch][k].peak (fs, f, q, db);  nBq = std::max (nBq, k + 1); }
        void shelf (int k, bool high, float f, float db) { for (int ch = 0; ch < 2; ++ch) bq[ch][k].shelf (fs, f, db, high);  nBq = std::max (nBq, k + 1); }
        float fs = 48000.0f, hpC[2] { 0, 0 }, hpS[2][2] {}, sendC = 0.0f, sendHpS = 0.0f;
        Bq bq[2][2], sendBq;
        int cur = -1, nHp = 0, nBq = 0, sendKind = 0;
    };

    //==============================================================================
    // b58: the Comp style "Opto Sustain": an optical-cell sustain pedal of the squeeze-and-swell kind.
    // From the published descriptions only (one control sets the threshold, the ratio is as high as it goes, a very fast fixed attack and a release of
    // about half a second so a note dips when it is hit and then rises as it dies away), plus a blend control, a layer of hard-clipped grit, a
    // treble cut / boost and an output level. Every number below is my own reading of those descriptions, not a measurement.
    struct OptoSustainFx
    {
        struct Params { float sustain = 0.5f, grit = 0.0f, treble = 0.0f, blend = 1.0f, volumeDb = 0.0f; };

        void prepare (double sr) { fs = (float) sr;  reset(); }
        void reset()
        {
            sq = 0.0f;  grA = grB = mem = 0.0f;  gritHp = gritLp = shelfLp = 0.0f;  uPrev = 0.0f;
            makeupSm = -1000.0f;  volSm = blendSm = gritSm = trebSm = -1000.0f;
        }
        // Mono, in place. Returns the gain reduction in dB (the squeeze, not counting the makeup), for the meter.
        float process (float* x, int n, const Params& p)
        {
            const float sus = juce::jlimit (0.0f, 1.0f, p.sustain);
            const float thr = -14.0f - 40.0f * sus;                       // the Sustain knob lowers the threshold
            const float makeupT = 36.0f * sus;                            // and the level it holds the note at stays about the same (about -16 dB)
            const float slope = 1.0f - 1.0f / 40.0f;                      // "as high as it goes"
            const float knee = 6.0f;
            const float levelC = 1.0f - std::exp (-1.0f / (0.004f * fs));
            const float atkC = std::exp (-1.0f / (0.002f * fs));          // fixed, very fast
            const float relA = std::exp (-1.0f / (0.090f * fs));          // first half of the release
            const float memC = 1.0f - std::exp (-1.0f / (1.2f * fs));
            const float smC = 1.0f - std::exp (-1.0f / (0.020f * fs));
            const float tc = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 2500.0f / fs);       // treble shelf corner
            const float hc = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 150.0f / fs);        // the grit layer has no lows
            const float lc = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 6000.0f / fs);       // and its fizz is rolled off
            const float gritT = juce::jlimit (0.0f, 1.0f, p.grit), blendT = juce::jlimit (0.0f, 1.0f, p.blend);
            const float volT = std::pow (10.0f, juce::jlimit (-24.0f, 24.0f, p.volumeDb) / 20.0f);
            const float trebT = std::pow (10.0f, 12.0f * juce::jlimit (-1.0f, 1.0f, p.treble) / 20.0f);
            if (makeupSm < -999.0f) { makeupSm = makeupT;  volSm = volT;  blendSm = blendT;  gritSm = gritT;  trebSm = trebT; }
            float lastGr = 0.0f;

            for (int i = 0; i < n; ++i)
            {
                makeupSm += smC * (makeupT - makeupSm);  volSm += smC * (volT - volSm);  blendSm += smC * (blendT - blendSm);
                gritSm += smC * (gritT - gritSm);  trebSm += smC * (trebT - trebSm);

                const float in = x[i];
                sq += levelC * (in * in - sq);
                const float levelDb = 3.0103f * std::log2 (sq + 1.0e-9f);
                const float over = levelDb - thr;
                float target = 0.0f;
                if (2.0f * over < -knee)                target = 0.0f;
                else if (2.0f * std::abs (over) <= knee) target = slope * (over + 0.5f * knee) * (over + 0.5f * knee) / (2.0f * knee);
                else if (over > 0.0f)                   target = slope * over;

                // two-stage release (a fast half and a slow half), the slow half longer the longer and deeper it has been squeezing
                mem += memC * ((grA + grB) - mem);
                const float held = juce::jlimit (0.0f, 1.0f, mem / 12.0f);
                const float relB = std::exp (-1.0f / ((0.45f + 0.5f * held) * fs));
                const float tgt = 0.5f * target;
                grA = tgt + (tgt > grA ? atkC : relA) * (grA - tgt);
                grB = tgt + (tgt > grB ? atkC : relB) * (grB - tgt);
                const float gr = grA + grB;
                lastGr = gr;

                float y = in * std::exp2 ((makeupSm - gr) * 0.166096f);
                y = 2.0f * std::tanh (0.5f * y);                      // a soft ceiling for the first few milliseconds of a note after silence (the gain is still at its highest)

                // grit: the squeezed signal, high-passed, driven hard into a hard clipper (first-order anti-aliased), then low-passed
                if (gritSm > 0.002f)
                {
                    gritHp += hc * (y - gritHp);
                    const float u = (y - gritHp) * (6.0f + 60.0f * gritSm);
                    float c;
                    const float du = u - uPrev;
                    auto F = [] (float v) { const float a = std::abs (v); return a <= 1.0f ? 0.5f * v * v : a - 0.5f; };
                    if (std::abs (du) > 1.0e-4f) c = (F (u) - F (uPrev)) / du;
                    else                         c = juce::jlimit (-1.0f, 1.0f, 0.5f * (u + uPrev));
                    uPrev = u;
                    gritLp += lc * (c - gritLp);
                    y += gritSm * 0.30f * gritLp;
                }
                else uPrev = 0.0f;

                // treble: a shelf around 2.5 kHz, cut or boost up to 12 dB
                shelfLp += tc * (y - shelfLp);
                y = shelfLp + trebSm * (y - shelfLp);

                x[i] = (in * (1.0f - blendSm) + y * blendSm) * volSm;
            }
            return lastGr;
        }

    private:
        float fs = 48000.0f, sq = 0.0f, grA = 0.0f, grB = 0.0f, mem = 0.0f, gritHp = 0.0f, gritLp = 0.0f, shelfLp = 0.0f, uPrev = 0.0f;
        float makeupSm = -1000.0f, volSm = 1.0f, blendSm = 1.0f, gritSm = 0.0f, trebSm = 1.0f;
    };

    //==============================================================================
    // b62: the Comp style "Crush Limiter": a podium-microphone limiter of the late 1960s, the kind that was pushed past its design on purpose.
    // From the published description and specification of a plug-in reissue only (attack about 1.3 ms; release about 1.7 s, half of that on Fast,
    // and about 22 s when the control circuit is driven into saturation; a limiter, no ratio or threshold knob; Crush sends more signal to the
    // gain-reduction circuit; Crunch sets the gain after the limiter and so how hard the output stage is driven; Darkness is a low-pass filter after
    // the distortion; Mix; a Slow / Fast release switch). The numbers for the knee, the detector and the output-stage curve are my own reading.
    // Crush lowers the level at which limiting starts and the makeup follows it, so Crush changes how much is squeezed, not how loud it is: quiet
    // notes and the tail of every note are lifted by up to 36 dB (that is the sound, and also the noise).
    struct CrushLimiterFx
    {
        struct Params { float crush = 0.5f, crunch = 0.0f, darkness = 0.0f, mix = 1.0f, volumeDb = 0.0f; bool fast = false; };

        void prepare (double sr) { fs = (float) sr;  reset(); }
        void reset()
        {
            env = 0.0f;  gr = 0.0f;  lp1 = lp2 = 0.0f;  hpY = 0.0f;  zPrev = 0.0f;
            crushSm = crunchSm = darkSm = mixSm = volSm = -1000.0f;
        }
        // Mono, in place. Returns the gain reduction in dB for the meter.
        float process (float* x, int n, const Params& p)
        {
            const float crushT = juce::jlimit (0.0f, 1.0f, p.crush), crunchT = juce::jlimit (0.0f, 1.0f, p.crunch);
            const float darkT = juce::jlimit (0.0f, 1.0f, p.darkness), mixT = juce::jlimit (0.0f, 1.0f, p.mix);
            const float volT = std::pow (10.0f, juce::jlimit (-24.0f, 24.0f, p.volumeDb) / 20.0f);
            if (crushSm < -999.0f) { crushSm = crushT;  crunchSm = crunchT;  darkSm = darkT;  mixSm = mixT;  volSm = volT; }
            const float smC = 1.0f - std::exp (-1.0f / (0.020f * fs));
            const float atkC = std::exp (-1.0f / (0.0013f * fs));                         // about 1.3 ms
            const float envRel = std::exp (-1.0f / (0.008f * fs));                        // the detector lets go in about 8 ms
            const float hpC = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 20.0f / fs);
            const float ceilDb = -12.0f, kneeDb = 3.0f;                                   // the level the limiter holds (at the output, before Crunch)
            const float fcT = std::min (0.45f * fs, 20000.0f * std::pow (0.06f, darkT));
            const float lc = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * fcT / fs);       // Darkness: set once per block
            float lastGr = 0.0f;

            for (int i = 0; i < n; ++i)
            {
                crushSm += smC * (crushT - crushSm);  crunchSm += smC * (crunchT - crunchSm);  darkSm += smC * (darkT - darkSm);
                mixSm += smC * (mixT - mixSm);  volSm += smC * (volT - volSm);

                const float crushDb = 36.0f * crushSm;                                    // signal sent to the gain-reduction circuit: 0 to +36 dB
                const float in = x[i];
                const float a = std::abs (in) * std::exp2 (crushDb * 0.166096f);          // what the detector sees
                env = a > env ? a : a + envRel * (env - a);
                const float lvDb = 6.0206f * std::log2 (env + 1.0e-9f);
                const float over = lvDb - ceilDb;                                         // dB over the limit (infinite ratio, 3 dB soft knee)
                float target = 0.0f;
                if (2.0f * over > kneeDb)           target = over;
                else if (2.0f * over > -kneeDb)     target = (over + 0.5f * kneeDb) * (over + 0.5f * kneeDb) / (2.0f * kneeDb);

                // release: about 1.7 s (0.85 s on Fast); driving Crush hard saturates the control circuit and the release stretches (about 13 times at the top)
                const float sat = juce::jlimit (0.0f, 1.0f, (crushSm - 0.55f) / 0.45f);
                const float relS = (p.fast ? 0.85f : 1.7f) * (1.0f + 12.0f * sat * sat);
                const float relC = std::exp (-1.0f / (relS * fs));
                gr = target + (target > gr ? atkC : relC) * (gr - target);
                lastGr = gr;

                // gain: +crush into the limiter and the same amount back out, minus the reduction. Small signals come up, loud ones sit at the ceiling.
                float y = in * std::exp2 ((crushDb - gr) * 0.166096f);
                y = 1.2f * std::tanh (y * 0.8333333f);                                    // a safety clip: the detector needs about a millisecond to catch a hard hit at high Crush
                // output stage: Crunch is gain after the limiter (0 to +24 dB) into an asymmetric soft clipper (first-order anti-aliased), then a partial level trim
                const float crunchDb = 24.0f * crunchSm;
                if (crunchSm > 0.002f)
                {
                    const float g = std::exp2 (crunchDb * 0.166096f), bias = 0.10f;
                    const float z = y * g * 4.0f + bias;                                  // the ceiling (-12 dB, 0.25) sits at about the knee of the curve
                    auto F = [] (float v) { const float av = std::abs (v); return av + std::log1p (std::exp (-2.0f * av)) - 0.6931472f; };   // log(cosh v)
                    const float dz = z - zPrev;
                    const float t = std::abs (dz) > 1.0e-4f ? (F (z) - F (zPrev)) / dz : std::tanh (0.5f * (z + zPrev));
                    zPrev = z;
                    const float trim = std::exp2 (-0.55f * crunchDb * 0.166096f);
                    const float c = (t - std::tanh (bias)) * 0.25f * trim;
                    const float w = juce::jlimit (0.0f, 1.0f, crunchSm * 8.0f);          // fade the stage in over the first bit of the knob so there is no step
                    y = y * (1.0f - w) + c * w;
                }
                else zPrev = 4.0f * y + 0.10f;
                // the output stage blocks DC
                hpY += hpC * (y - hpY);  y -= hpY;
                // Darkness: two one-pole low-passes after the distortion, from 20 kHz (0) down to about 1.2 kHz (1)
                lp1 += lc * (y - lp1);  lp2 += lc * (lp1 - lp2);
                y = darkSm > 0.002f ? lp2 : y;
                x[i] = (in * (1.0f - mixSm) + y * mixSm) * volSm;
            }
            return lastGr;
        }

    private:
        float fs = 48000.0f, env = 0.0f, gr = 0.0f, lp1 = 0.0f, lp2 = 0.0f, hpY = 0.0f, zPrev = 0.0f;
        float crushSm = -1000.0f, crunchSm = 0.0f, darkSm = 0.0f, mixSm = 1.0f, volSm = 1.0f;
    };

    // ---- b59: Program EQ. A passive-style program equaliser: a low shelf that boosts and a wider low shelf that attenuates at the same chosen frequency
    // (using both gives the tight, lifted low end with a dip above it), a high peak boost with a broad-to-sharp bandwidth, and a wide high attenuating shelf.
    // The shapes follow the published curve families by eye (gradual shelves, the attenuation wider than the boost), not a circuit solve.
    // Drive is an optional iron-and-tube warmth stage (asymmetric soft clip, a little more on the lows); at 0 it is bypassed exactly.
    // Everything at 0 dB with Drive 0 passes the signal untouched.
    class ProgramEqFx
    {
    public:
        struct Params
        {
            float lowBoostDb = 0.0f, lowCutDb = 0.0f, lowHz = 60.0f;       // low boost 0 to 18, low attenuate 0 to 24
            float highBoostDb = 0.0f, highHz = 8000.0f, bandwidth = 0.5f;  // high boost 0 to 24, bandwidth 0 (broad) to 1 (sharp)
            float highCutDb = 0.0f, highCutHz = 10000.0f;                  // high attenuate 0 to 24
            float drive = 0.0f, levelDb = 0.0f;                            // warmth 0 to 1, output level in dB
        };

        void prepare (double sampleRate) { sr = (float) sampleRate; for (auto& b : bq) b.reset(); last = {};  haveLast = false;  lowSat[0] = lowSat[1] = 0.0f;  lvl = 1.0f; }

        // l and r are processed in place (r may be null for a mono signal)
        void process (float* l, float* r, int n, const Params& p)
        {
            if (! haveLast || differs (p, last)) { design (p); last = p;  haveLast = true; }
            const bool on[4] = { p.lowBoostDb > 0.01f, p.lowCutDb > 0.01f, p.highBoostDb > 0.01f, p.highCutDb > 0.01f };
            const float targetLvl = std::pow (10.0f, p.levelDb / 20.0f);
            const float drive = std::min (1.0f, std::max (0.0f, p.drive));
            const float k = 1.0f + 3.0f * drive, bias = 0.12f * drive;
            const float lowC = 1.0f - std::exp (-6.2831853f * 140.0f / sr);
            for (int ch = 0; ch < (r != nullptr ? 2 : 1); ++ch)
            {
                float* x = ch == 0 ? l : r;
                for (int i = 0; i < n; ++i)
                {
                    float y = x[i];
                    for (int b = 0; b < 4; ++b) if (on[b]) y = bq[b].run (ch, y);
                    if (drive > 0.0005f)
                    {
                        // pre-gain so a guitar-level signal reaches the bend; the lows are pushed a little harder (the iron saturates first on bass)
                        const float pre = 2.0f;
                        lowSat[ch] += lowC * (y - lowSat[ch]);
                        const float z = (y + 0.5f * drive * lowSat[ch]) * pre;
                        const float t = std::tanh (k * z + bias) - std::tanh (bias);
                        const float slope = k * (1.0f - std::tanh (bias) * std::tanh (bias));
                        y = t / (slope * pre);
                    }
                    else lowSat[ch] = 0.0f;
                    lvl += 0.002f * (targetLvl - lvl);
                    x[i] = y * lvl;
                }
            }
        }

        // magnitude response in dB (for tests and the curve display), ignoring Drive and Level
        float responseDb (float hz, const Params& p) const
        {
            Coef c[4];  designInto (p, c);
            float tot = 0.0f;
            const bool on[4] = { p.lowBoostDb > 0.01f, p.lowCutDb > 0.01f, p.highBoostDb > 0.01f, p.highCutDb > 0.01f };
            for (int b = 0; b < 4; ++b) if (on[b]) tot += mag (c[b], hz);
            return tot;
        }

    private:
        struct Coef { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
        struct Bq : Coef
        {
            float z1[2] = { 0, 0 }, z2[2] = { 0, 0 };
            float run (int ch, float x) { const float y = b0 * x + z1[ch]; z1[ch] = b1 * x - a1 * y + z2[ch]; z2[ch] = b2 * x - a2 * y; return y; }
            void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
        };
        static bool differs (const Params& a, const Params& b)
        {
            return a.lowBoostDb != b.lowBoostDb || a.lowCutDb != b.lowCutDb || a.lowHz != b.lowHz || a.highBoostDb != b.highBoostDb || a.highHz != b.highHz
                || a.bandwidth != b.bandwidth || a.highCutDb != b.highCutDb || a.highCutHz != b.highCutHz;
        }
        // RBJ shelf and peak (kind 0 low shelf, 1 peak, 2 high shelf)
        Coef make (int kind, float fc, float dB, float q) const
        {
            const float A = std::pow (10.0f, dB / 40.0f), w = 6.2831853f * std::min (fc, 0.45f * sr) / sr, c = std::cos (w), s = std::sin (w);
            float n0, n1, n2, d0, d1, d2;
            if (kind == 1)
            {
                const float al = s / (2.0f * q);
                n0 = 1.0f + al * A;  n1 = -2.0f * c;  n2 = 1.0f - al * A;  d0 = 1.0f + al / A;  d1 = -2.0f * c;  d2 = 1.0f - al / A;
            }
            else
            {
                const float al = s / (2.0f * q), t = 2.0f * std::sqrt (A) * al;
                if (kind == 0) { n0 = A * ((A + 1) - (A - 1) * c + t);  n1 = 2 * A * ((A - 1) - (A + 1) * c);  n2 = A * ((A + 1) - (A - 1) * c - t);
                                 d0 = (A + 1) + (A - 1) * c + t;  d1 = -2 * ((A - 1) + (A + 1) * c);  d2 = (A + 1) + (A - 1) * c - t; }
                else           { n0 = A * ((A + 1) + (A - 1) * c + t);  n1 = -2 * A * ((A - 1) + (A + 1) * c);  n2 = A * ((A + 1) + (A - 1) * c - t);
                                 d0 = (A + 1) - (A - 1) * c + t;  d1 = 2 * ((A - 1) - (A + 1) * c);  d2 = (A + 1) - (A - 1) * c - t; }
            }
            Coef o;  o.b0 = n0 / d0;  o.b1 = n1 / d0;  o.b2 = n2 / d0;  o.a1 = d1 / d0;  o.a2 = d2 / d0;  return o;
        }
        void designInto (const Params& p, Coef* c) const
        {
            const float lf = std::max (15.0f, p.lowHz);
            c[0] = make (0, lf, p.lowBoostDb, 0.75f);                                                 // boost: a shelf with a little resonance at its corner
            c[1] = make (0, lf * 2.0f, -p.lowCutDb, 0.4f);                                             // attenuate: wider and smooth, so it reaches further up
            c[2] = make (1, p.highHz, p.highBoostDb, 0.4f * std::pow (10.0f, std::min (1.0f, std::max (0.0f, p.bandwidth))));   // Q 0.4 (broad) to 4 (sharp)
            c[3] = make (2, p.highCutHz * 0.6f, -p.highCutDb, 0.5f);                                   // wide: starts well below the dial
        }
        void design (const Params& p) { Coef c[4]; designInto (p, c); for (int b = 0; b < 4; ++b) static_cast<Coef&> (bq[b]) = c[b]; }
        float mag (const Coef& c, float hz) const
        {
            const float w = 6.2831853f * hz / sr, cw = std::cos (w), sw = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
            const float nr = c.b0 + c.b1 * cw + c.b2 * c2, ni = -(c.b1 * sw + c.b2 * s2), dr = 1.0f + c.a1 * cw + c.a2 * c2, di = -(c.a1 * sw + c.a2 * s2);
            return 10.0f * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1.0e-20f);
        }

        float sr = 48000.0f, lvl = 1.0f, lowSat[2] = { 0, 0 };
        Bq bq[4];
        Params last;  bool haveLast = false;
    };

    // ======================================================================== b62
    // Quadrature pair: two all-pass chains whose outputs stay 90 degrees apart over the audio range (a Hilbert transformer, about +/-1 degree from 100 Hz up).
    // The one-sample delay goes on the chain with the first four coefficients (put on the other chain the error is 15 degrees at 1 kHz and 45 at 3 kHz).
    struct QuadPair
    {
        float x1[8] {}, x2[8] {}, y1[8] {}, y2[8] {}, d = 0.0f;
        void reset() { *this = QuadPair(); }
        inline void process (float x, float& a, float& b)
        {
            static const float k[8] = { 0.6923878f, 0.9360654f, 0.9882295f, 0.9987488f, 0.4021921f, 0.8561711f, 0.9722910f, 0.9952885f };
            float v[2] = { d, x };
            d = x;
            float o[2];
            for (int path = 0; path < 2; ++path)
            {
                float t = v[path];
                for (int q = 0; q < 4; ++q)
                {
                    const int j = path * 4 + q;
                    const float a2 = k[j] * k[j];
                    const float y = a2 * (t + y2[j]) - x2[j];
                    x2[j] = x1[j];  x1[j] = t;
                    y2[j] = y1[j];  y1[j] = y;
                    t = y;
                }
                o[path] = t;
            }
            a = o[0];  b = o[1];
        }
    };

    // ---- Frequency shifter (b62): every partial moves by the same number of Hz, so harmonics stop being harmonic (clangorous, metallic, barber-pole).
    //      Modes: 0 single-sideband shift (Feedback gives shifted repeats that spiral), 1 ring modulator (carrier at the Shift frequency),
    //             2 spread (the left side shifted up, the right side down; mono: the sum of both).
    class FreqShiftFx
    {
    public:
        struct Params { int mode = 0; float shiftHz = 7.0f; float lfoHz = 0.5f; float depth = 0.0f; int shape = 0; float feedback = 0.0f; float delayMs = 10.0f; float toneHz = 12000.0f; float mix = 0.5f; };

        void prepare (double sampleRate)
        {
            sr = (float) sampleRate;
            dsize = (int) (0.15f * sr) + 8;
            for (auto& c : ch) { c.buf.assign ((size_t) dsize, 0.0f); }
            reset();
        }
        void reset()
        {
            for (auto& c : ch) { std::fill (c.buf.begin(), c.buf.end(), 0.0f); c.w = 0; c.hb.reset(); c.lp = 0.0f; c.dc = 0.0f; }
            ph = 0.0; lph = 0.0; dly = -1.0f;
        }

        void process (float* l, float* r, int n, const Params& p)
        {
            const bool st = r != nullptr;
            const float tc = 1.0f - std::exp (-6.2831853f * std::min (p.toneHz, 0.45f * sr) / sr);
            const float fb = std::max (-0.92f, std::min (0.92f, p.feedback));
            const float dTarget = std::max (1.0f, std::min (0.12f * sr, 0.001f * p.delayMs * sr));
            if (dly < 0.0f) dly = dTarget;
            const float dk = 1.0f - std::exp (-1.0f / (0.03f * sr));
            for (int i = 0; i < n; ++i)
            {
                lph += p.lfoHz / sr;  lph -= std::floor (lph);
                const float lfo = p.depth > 0.0f ? modLfoWave ((float) lph, p.shape) : 0.0f;
                const float sh = std::max (-3000.0f, std::min (3000.0f, p.shiftHz * std::exp2 (2.0f * p.depth * lfo)));
                ph += (double) sh / (double) sr;  ph -= std::floor (ph);
                const float cs = std::cos (6.2831853f * (float) ph), sn = std::sin (6.2831853f * (float) ph);
                dly += dk * (dTarget - dly);

                float inV[2] = { l[i], st ? r[i] : l[i] }, wet[2];
                const int nch = st ? 2 : 1;
                for (int k = 0; k < nch; ++k)
                {
                    Chan& c = ch[k];
                    float x = inV[k];
                    if (p.mode == 1)
                    {
                        wet[k] = 1.41f * x * sn;                                  // ring modulator
                    }
                    else
                    {
                        float fbv = 0.0f;
                        if (std::abs (fb) > 0.001f)
                        {
                            float rp = (float) c.w - dly;  if (rp < 0.0f) rp += (float) dsize;
                            const int i0 = (int) rp, i1 = (i0 + 1) % dsize;
                            const float f = rp - (float) i0;
                            fbv = c.buf[(size_t) i0] * (1.0f - f) + c.buf[(size_t) i1] * f;
                        }
                        float a, b;
                        const float xin = x + fb * fbv;
                        c.hb.process (std::abs (fb) > 0.001f ? std::tanh (xin) : xin, a, b);
                        // the sign of the sine term sets which sideband survives: with this pair of all-pass chains a plus moves everything up for a positive Shift (checked by the b62 test)
                        float up = a * cs + b * sn;
                        float w = up;
                        if (p.mode == 2)
                        {
                            const float down = a * cs - b * sn;
                            w = st ? (k == 0 ? up : down) : 0.7071f * (up + down);     // spread: left up and right down; mono: both together
                        }
                        wet[k] = w;
                        c.buf[(size_t) c.w] = up;  c.w = (c.w + 1) % dsize;
                    }
                }
                for (int k = 0; k < nch; ++k)
                {
                    Chan& c = ch[k];
                    c.lp += tc * (wet[k] - c.lp);
                    c.dc += 0.0005f * (c.lp - c.dc);                              // keeps a stray DC offset out of the feedback
                    const float y = c.lp - c.dc;
                    const float o = inV[k] * (1.0f - p.mix) + y * p.mix;
                    (k == 0 ? l[i] : r[i]) = o;
                }
            }
        }

    private:
        struct Chan { std::vector<float> buf; int w = 0; QuadPair hb; float lp = 0.0f, dc = 0.0f; };
        Chan ch[2];
        int dsize = 1024;
        float sr = 48000.0f, dly = -1.0f;
        double ph = 0.0, lph = 0.0;
    };

    // ---- Vocoder (b62): the spectrum of one signal (the modulator: a voice, or your guitar) shapes another (the carrier: a synth, noise, a drone, your guitar).
    //      N band-pass pairs (analysis and synthesis) with envelope followers between them. The carrier bank can be shifted up or down (formant shift).
    class VocoderFx
    {
    public:
        static constexpr int kMaxBands = 32;
        struct Params { int bands = 16; float topHz = 9000.0f; float sharp = 0.5f; float response = 0.5f; float formant = 0.0f; float sibilance = 0.4f; float levelDb = 0.0f; };

        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); lastBands = -1; }
        void reset()
        {
            for (int b = 0; b < kMaxBands; ++b) { for (int k = 0; k < 2; ++k) { mod[b][k] = Biquad(); car[b][k] = Biquad(); } env[b] = 0.0f;  carEnv[b] = 0.0f; }
            sibHp = Biquad();  sibEnv = 0.0f;  noiseState = 22222u;  lastBands = -1;
        }

        // mod and car: mono, n samples; out: the vocoded signal (not mixed)
        void process (const float* modIn, const float* carIn, float* out, int n, const Params& p)
        {
            const int nb = std::max (4, std::min (kMaxBands, p.bands));
            const float top = std::max (2000.0f, std::min (0.42f * sr, p.topHz));
            if (nb != lastBands || std::abs (top - lastTop) > 1.0f || std::abs (p.sharp - lastSharp) > 0.002f || std::abs (p.formant - lastFormant) > 0.001f)
            {
                design (nb, top, p.sharp, p.formant);
                lastBands = nb;  lastTop = top;  lastSharp = p.sharp;  lastFormant = p.formant;
            }
            // the envelope speed: slow (about 12 Hz, smooth and mushy) to fast (about 70 Hz, crisp consonants)
            const float fcEnv = 12.0f * std::pow (70.0f / 12.0f, std::max (0.0f, std::min (1.0f, p.response)));
            const float kR = 1.0f - std::exp (-6.2831853f * fcEnv / sr), kA = 1.0f - std::exp (-6.2831853f * 3.0f * fcEnv / sr);
            const float carA = 1.0f - std::exp (-1.0f / (0.001f * sr)), carR = 1.0f - std::exp (-1.0f / (0.06f * sr));
            const float gain = std::pow (10.0f, p.levelDb / 20.0f) * 1.2f;
            sibHp.highpass (5200.0f, sr);
            for (int i = 0; i < n; ++i)
            {
                const float m = modIn[i], c = carIn[i];
                float sum = 0.0f;
                for (int b = 0; b < nb; ++b)
                {
                    float e = mod[b][1].process (mod[b][0].process (m));
                    const float a = std::abs (e);
                    env[b] += (a > env[b] ? kA : kR) * (a - env[b]);
                    const float cv = car[b][1].process (car[b][0].process (c));
                    // the carrier band is levelled first (fast up, slow down), so the output follows the voice, not the carrier's own loudness
                    const float ca = std::abs (cv);
                    carEnv[b] += (ca > carEnv[b] ? carA : carR) * (ca - carEnv[b]);
                    sum += cv * env[b] * bandGain[b] / (carEnv[b] + 0.003f);
                }
                // consonants: noise at the level of the modulator's top end
                noiseState = noiseState * 1664525u + 1013904223u;
                const float nz = (float) (int) noiseState * (1.0f / 2147483648.0f);
                const float hf = std::abs (sibHp.process (m));
                sibEnv += (hf > sibEnv ? kA : kR) * (hf - sibEnv);
                sum += p.sibilance * 2.0f * sibEnv * nz;
                out[i] = gain * sum;
            }
        }

    private:
        // two cascaded band-passes per band (4th order), analysis at the band centre and synthesis at the centre moved by the formant shift
        void design (int nb, float top, float sharp, float formant)
        {
            const float lo = 110.0f, octs = std::log2 (top / lo), bw = octs / (float) nb;
            const float q = 1.0f / (2.0f * std::sinh (0.5f * 0.6931472f * bw * 1.15f)) * (0.7f + 0.9f * sharp);
            const float shiftF = std::exp2 (formant);
            for (int b = 0; b < nb; ++b)
            {
                const float fc = lo * std::exp2 (bw * ((float) b + 0.5f));
                for (int k = 0; k < 2; ++k) mod[b][k].bandpass (fc, sr, q);
                const float fcc = std::min (0.45f * sr, fc * shiftF);
                for (int k = 0; k < 2; ++k) car[b][k].bandpass (fcc, sr, q);
                bandGain[b] = 1.0f + 0.9f * (float) b / (float) nb;           // a little tilt up: the high bands carry the consonants
            }
            for (int b = nb; b < kMaxBands; ++b) bandGain[b] = 0.0f;
        }

        struct Biquad
        {
            float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
            void bandpass (float f, float sr, float q)
            {
                const float w = 6.2831853f * std::min (f, 0.45f * sr) / sr, al = std::sin (w) / (2.0f * q), cw = std::cos (w), n = 1.0f / (1.0f + al);
                b0 = al * n;  b1 = 0.0f;  b2 = -al * n;  a1 = -2.0f * cw * n;  a2 = (1.0f - al) * n;       // constant 0 dB peak
            }
            void highpass (float f, float sr)
            {
                const float K = std::tan (3.14159265f * std::min (f, 0.45f * sr) / sr), q = 0.70710678f, norm = 1.0f / (1.0f + K / q + K * K);
                b0 = norm;  b1 = -2.0f * norm;  b2 = norm;  a1 = 2.0f * (K * K - 1.0f) * norm;  a2 = (1.0f - K / q + K * K) * norm;
            }
            inline float process (float x)
            {
                const float y = b0 * x + z1;
                z1 = b1 * x - a1 * y + z2;
                z2 = b2 * x - a2 * y;
                return y;
            }
        };

        Biquad mod[kMaxBands][2], car[kMaxBands][2], sibHp;
        float env[kMaxBands] {}, carEnv[kMaxBands] {}, bandGain[kMaxBands] {};
        float sibEnv = 0.0f, sr = 48000.0f, lastTop = 0.0f, lastSharp = -1.0f, lastFormant = 9.0f;
        int lastBands = -1;
        uint32_t noiseState = 22222u;
    };

    // ---- the vocoder's own carriers: white noise and a detuned saw-and-square drone (band-limited with a polyBLEP edge)
    class DroneOsc
    {
    public:
        void reset() { for (auto& p : ph) p = 0.0f; }
        // note: MIDI note number
        float next (float midiNote, float sr)
        {
            static const float det[3] = { -7.0f, 0.0f, 7.0f };
            float s = 0.0f;
            for (int k = 0; k < 3; ++k)
            {
                const float f = 440.0f * std::exp2 ((midiNote - 69.0f + det[k] * 0.01f) / 12.0f), dt = f / sr;
                ph[k] += dt;  if (ph[k] >= 1.0f) ph[k] -= 1.0f;
                s += (2.0f * ph[k] - 1.0f) - blep (ph[k], dt);
            }
            const float fs = 440.0f * std::exp2 ((midiNote - 12.0f - 69.0f) / 12.0f), dts = fs / sr;
            ph[3] += dts;  if (ph[3] >= 1.0f) ph[3] -= 1.0f;
            s += 0.8f * ((ph[3] < 0.5f ? 1.0f : -1.0f) + blep (ph[3], dts) - blep (std::fmod (ph[3] + 0.5f, 1.0f), dts));
            return s * 0.3f;
        }
    private:
        static float blep (float t, float dt)
        {
            if (t < dt) { t /= dt;  return t + t - t * t - 1.0f; }
            if (t > 1.0f - dt) { t = (t - 1.0f) / dt;  return t * t + t + t + 1.0f; }
            return 0.0f;
        }
        float ph[4] {};
    };
}
