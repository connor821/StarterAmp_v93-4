#pragma once
// v65: two helpers that make a cab (built-in or a loaded impulse response) behave less like a fixed snapshot.
//   ResonanceTamer: after the cab, finds frequencies that stick out of the local spectrum and turns them down only
//                   while they stick out (a dynamic, subtractive EQ). Zero latency: a bank of band-pass detectors drives
//                   a bank of narrow peaking cuts.
//   MotionDrift   : after the cab, slow random changes in a few broad bands, different for the left and the right
//                   channel and stronger when you play harder, so the sound never repeats exactly (a static IR does).
// Both are my own simple designs from general descriptions of this kind of processing, not copies of any product.
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace cabreal
{
struct Bq
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0.0f; }
    void peak (float sr, float f, float q, float gainDb)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f);
        const float w = 6.2831853f * f / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        const float a0 = 1.0f + al / A;
        b0 = (1.0f + al * A) / a0;  b1 = (-2.0f * c) / a0;  b2 = (1.0f - al * A) / a0;
        a1 = (-2.0f * c) / a0;      a2 = (1.0f - al / A) / a0;
    }
    void bandpass (float sr, float f, float q)
    {
        const float w = 6.2831853f * f / sr, c = std::cos (w), al = std::sin (w) / (2.0f * q);
        const float a0 = 1.0f + al;
        b0 = al / a0;  b1 = 0.0f;  b2 = -al / a0;
        a1 = (-2.0f * c) / a0;  a2 = (1.0f - al) / a0;
    }
};

class ResonanceTamer
{
public:
    static constexpr int N = 16;
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (int i = 0; i < N; ++i)
        {
            freq[i] = 150.0f * std::pow (60.0f, (float) i / (float) (N - 1));     // 150 Hz to 9 kHz
            det[i].bandpass (sr, freq[i], 3.3f);
        }
        envA = 1.0f - std::exp (-1.0f / (0.008f * sr));
        envR = 1.0f - std::exp (-1.0f / (0.06f * sr));
        cutA = 1.0f - std::exp (-(float) kTick / (0.012f * sr));
        cutR = 1.0f - std::exp (-(float) kTick / (0.15f * sr));
        reset();
    }
    void reset()
    {
        for (int i = 0; i < N; ++i) { det[i].reset(); env[i] = 0.0f; cut[i] = 0.0f; pk[0][i].reset(); pk[1][i].reset(); pk[0][i] = Bq(); pk[1][i] = Bq(); active[i] = false; }
        tick = 0;
    }
    // r may be null (mono). depth is 0 to 1. delta plays back what is being removed.
    void process (float* l, float* r, int n, float depth, bool delta)
    {
        for (int s = 0; s < n; ++s)
        {
            const float in0 = l[s], in1 = r != nullptr ? r[s] : 0.0f;
            const float mono = r != nullptr ? 0.5f * (in0 + in1) : in0;
            for (int i = 0; i < N; ++i)
            {
                const float d = det[i].process (mono), p = d * d;
                env[i] += (p > env[i] ? envA : envR) * (p - env[i]);
            }
            if (++tick >= kTick) { tick = 0; update (depth); }

            float o0 = in0, o1 = in1;
            for (int i = 0; i < N; ++i)
                if (active[i])
                {
                    o0 = pk[0][i].process (o0);
                    if (r != nullptr) o1 = pk[1][i].process (o1);
                }
            l[s] = delta ? in0 - o0 : o0;
            if (r != nullptr) r[s] = delta ? in1 - o1 : o1;
        }
    }

private:
    static constexpr int kTick = 32;
    void update (float depth)
    {
        float db[N];
        for (int i = 0; i < N; ++i) db[i] = 10.0f * std::log10 (env[i] + 1.0e-12f);
        const float maxCut = 9.0f * (0.5f + 0.5f * depth);
        for (int i = 0; i < N; ++i)
        {
            float sum = 0.0f, w = 0.0f;
            for (int j = std::max (0, i - 3); j <= std::min (N - 1, i + 3); ++j)
                if (j != i) { const float wj = 4.0f - (float) std::abs (j - i); sum += wj * db[j]; w += wj; }
            const float ref = sum / w;
            const float gate = std::min (1.0f, std::max (0.0f, (db[i] + 75.0f) / 15.0f));      // nothing in near-silence
            const float excess = std::max (0.0f, db[i] - ref - 1.5f);
            const float target = std::min (maxCut, excess * depth * 2.0f) * gate;
            cut[i] += (target > cut[i] ? cutA : cutR) * (target - cut[i]);
            if (cut[i] > 0.05f)
            {
                pk[0][i].b0 = 1.0f;                       // keep the state, change only the coefficients
                Bq tmp;  tmp.peak (sr, freq[i], 3.0f, -cut[i]);
                for (int c = 0; c < 2; ++c) { pk[c][i].b0 = tmp.b0; pk[c][i].b1 = tmp.b1; pk[c][i].b2 = tmp.b2; pk[c][i].a1 = tmp.a1; pk[c][i].a2 = tmp.a2; }
                active[i] = true;
            }
            else if (active[i])
            {
                active[i] = false;  pk[0][i].reset();  pk[1][i].reset();
            }
        }
    }
    float sr = 48000.0f, envA = 0.01f, envR = 0.001f, cutA = 0.1f, cutR = 0.01f;
    float freq[N] {}, env[N] {}, cut[N] {};
    Bq det[N], pk[2][N];
    bool active[N] {};
    int tick = 0;
};

class MotionDrift
{
public:
    static constexpr int B = 6;
    void prepare (double sampleRate) { sr = (float) sampleRate; reset(); }
    void reset()
    {
        for (int c = 0; c < 2; ++c)
            for (int b = 0; b < B; ++b) { x[c][b] = 0.0f; bq[c][b] = Bq(); }
        env = 0.0f;  tick = 0;  rng = 0x9E3779B9u;
    }
    void process (float* l, float* r, int n, float amount, float rate)
    {
        static const float f[B] = { 180.0f, 420.0f, 950.0f, 2200.0f, 4500.0f, 8500.0f };
        const float tau = 0.25f + 3.0f * (1.0f - rate);                     // seconds
        const float k = (float) kTick / (tau * sr);
        const float sd = std::sqrt (2.0f * k) * 1.732f;
        const float eA = 1.0f - std::exp (-1.0f / (0.005f * sr)), eR = 1.0f - std::exp (-1.0f / (0.12f * sr));
        for (int s = 0; s < n; ++s)
        {
            const float a = std::max (std::abs (l[s]), std::abs (r[s]));
            env += (a > env ? eA : eR) * (a - env);
            if (++tick >= kTick)
            {
                tick = 0;
                const float lvl = 0.4f + 0.6f * std::min (1.0f, env * 6.0f);      // harder playing moves it more
                for (int c = 0; c < 2; ++c)
                    for (int b = 0; b < B; ++b)
                    {
                        x[c][b] += -k * x[c][b] + sd * noise();
                        x[c][b] = std::max (-2.5f, std::min (2.5f, x[c][b]));
                        Bq t;  t.peak (sr, f[b], 0.9f, amount * 1.1f * x[c][b] * lvl);
                        bq[c][b].b0 = t.b0; bq[c][b].b1 = t.b1; bq[c][b].b2 = t.b2; bq[c][b].a1 = t.a1; bq[c][b].a2 = t.a2;
                    }
            }
            float yl = l[s], yr = r[s];
            for (int b = 0; b < B; ++b) { yl = bq[0][b].process (yl); yr = bq[1][b].process (yr); }
            l[s] = yl;  r[s] = yr;
        }
    }

private:
    static constexpr int kTick = 32;
    float noise() { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) / 8388608.0f - 1.0f; }
    float sr = 48000.0f, env = 0.0f, x[2][B] {};
    Bq bq[2][B];
    int tick = 0;
    uint32_t rng = 1;
};
}
