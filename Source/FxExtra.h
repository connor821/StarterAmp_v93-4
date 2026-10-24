#pragma once
// b15: four more effect blocks for the Order board. All hand-built; the names describe the kind of sound, not any particular unit.
//   FilterFx       : one filter pedal with six modes (low-pass, high-pass, band-pass, notch, formant (vowel), resonator (tuned combs))
//                    and four ways to move it (manual, LFO, envelope follower, random steps)
//   VolumePanFx    : a volume pedal / utility: level, pan (stereo only), polarity
//   MultibandCompFx: three bands (Linkwitz-Riley crossovers), a compressor in each
//   GateExpFx      : a noise gate / downward expander with hold and a detector high-pass
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

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
        struct Params
        {
            float x1 = 150.0f, x2 = 2500.0f;          // crossover frequencies (Hz)
            float thr[3] { -24.0f, -24.0f, -24.0f };  // threshold per band (dB)
            float ratio = 3.0f, attackMs = 10.0f, releaseMs = 120.0f, outDb = 0.0f, mix = 1.0f;
        };
        void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
        void reset()
        {
            for (int c = 0; c < 2; ++c) { xHi[c].reset(); xLo[c].reset(); xAp[c].reset(); }
            for (int b = 0; b < 3; ++b) grDb[b] = 0.0f;
        }
        void process (float* l, float* r, int n, const Params& p)
        {
            const int chans = r != nullptr ? 2 : 1;
            const float f1 = std::min (p.x1, 0.8f * p.x2);
            for (int c = 0; c < chans; ++c) { xHi[c].set (p.x2, sr);  xLo[c].set (f1, sr);  xAp[c].set (f1, sr); }
            const float atk = 1.0f - std::exp (-1.0f / (0.001f * std::max (0.1f, p.attackMs) * sr));
            const float rel = 1.0f - std::exp (-1.0f / (0.001f * std::max (5.0f, p.releaseMs) * sr));
            const float out = dbToGain (p.outDb);
            const float ratio = std::max (1.0f, p.ratio);
            for (int i = 0; i < n; ++i)
            {
                float band[2][3];
                for (int c = 0; c < chans; ++c)
                {
                    const float x = c == 0 ? l[i] : r[i];
                    float l2, h2;
                    xHi[c].split (x, l2, h2);               // below / above the upper crossover
                    float low, mid;
                    xLo[c].split (l2, low, mid);            // the lower part split again
                    float hl, hh;
                    xAp[c].split (h2, hl, hh);              // the top band goes through the lower crossover's phase turn (all-pass)
                    band[c][0] = low;  band[c][1] = mid;  band[c][2] = hl + hh;
                }
                float sum[2] = { 0.0f, 0.0f };
                for (int b = 0; b < 3; ++b)
                {
                    float pk = std::abs (band[0][b]);
                    if (chans == 2) pk = std::max (pk, std::abs (band[1][b]));
                    const float lvDb = 20.0f * std::log10 (pk + 1.0e-9f);
                    const float over = lvDb - p.thr[b];
                    float target;                             // gain reduction wanted (dB, positive)
                    if (over <= -3.0f) target = 0.0f;
                    else if (over < 3.0f) { const float t = over + 3.0f; target = (1.0f - 1.0f / ratio) * t * t / 12.0f; }   // soft knee, 6 dB wide
                    else target = (1.0f - 1.0f / ratio) * over;
                    grDb[b] += (target > grDb[b] ? atk : rel) * (target - grDb[b]);
                    const float gain = dbToGain (-grDb[b]);
                    for (int c = 0; c < chans; ++c) sum[c] += band[c][b] * gain;
                }
                for (int c = 0; c < chans; ++c)
                {
                    float* d = c == 0 ? l + i : r + i;
                    *d = *d * (1.0f - p.mix) + sum[c] * out * p.mix;
                }
            }
        }
        float grDb[3] { 0.0f, 0.0f, 0.0f };
    private:
        float sr = 48000.0f;
        LR4 xHi[2], xLo[2], xAp[2];
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
}
