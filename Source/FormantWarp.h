#pragma once
// b97: two helpers for the pitch pedal.
//
// FormantWarp: moves the spectral envelope (the "formants", the broad hills in the spectrum that make a sound big or small) up or down
//   on its own, without moving the pitch. A shifted note normally brings its tone with it: an octave up sounds small and bright, an octave
//   down sounds huge and dull. This undoes that (Keep natural), or adds a move of its own (Formant knob).
//   How: 1024-sample frames every 256 samples, windowed with a Hann window. For each frame the spectral envelope is estimated by cepstral
//   smoothing (the log spectrum, low quefrencies only), then each bin is scaled by envelope(f / ratio) / envelope(f), limited to +-20 dB, so
//   the harmonics stay where they are and only their loudness pattern slides. Overlap-added with a second Hann window. Delay: kLatency samples.
//   This is my own design from the textbook method, NOT measured from any unit.
//
// DryLowSplit: a 4th-order Linkwitz-Riley crossover (two Butterworth biquads each side), so the lows can stay dry while the highs are shifted
//   and the two add back flat when nothing is shifted.
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <vector>
#include <juce_dsp/juce_dsp.h>

// b99: the frame size is a template argument. FormantWarp (1024, 21 ms) is the pitch pedal's; FormantWarpFast (256, 5 ms) is Transpose's.
template <int NN, int LQ = 28, int GM10 = 23>
class FormantWarpT
{
public:
    static constexpr int N = NN, H = NN / 4, kBins = NN / 2 + 1;
    static constexpr int kLatency = N;                                  // the output is read N samples behind the newest input: every frame that touches it is finished

    FormantWarpT() : fft (NN == 1024 ? 10 : NN == 512 ? 9 : NN == 256 ? 8 : 7)
    {
        for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (6.28318530718f * (float) i / (float) N);
        reset();
    }

    void prepare (double sampleRate) { liftQ = std::max (12, std::min (N / 2 - 16, (int) std::lround ((double) LQ * sampleRate / 48000.0)));  reset(); }

    void reset()
    {
        std::fill (inBuf.begin(), inBuf.end(), 0.0f);
        std::fill (outAcc.begin(), outAcc.end(), 0.0f);
        wp = 0;  hopCount = 0;  cur = 0.0f;  target = 0.0f;
    }

    // The envelope move in semitones (positive = formants up). Takes effect smoothly.
    void setSemis (float s) { target = std::max (-24.0f, std::min (24.0f, s)); }

    float process (float x)
    {
        inBuf[(size_t) wp] = x;
        // the sample leaving now: N samples behind the write position, ready because every frame covering it has been added
        const int rp = wp;                                              // (wp - N) mod N == wp
        const float y = outAcc[(size_t) rp];
        outAcc[(size_t) rp] = 0.0f;
        if (++wp >= N) wp = 0;
        if (++hopCount >= H) { hopCount = 0;  frame(); }
        return y;
    }

private:
    void frame()
    {
        cur += 0.35f * (target - cur);                                  // glide the move from frame to frame (about 4 frames to settle)
        // 1. the latest N samples, oldest first, windowed
        for (int i = 0; i < N; ++i)
            data[(size_t) i] = inBuf[(size_t) ((wp + i) % N)] * win[(size_t) i];
        std::fill (data.begin() + N, data.end(), 0.0f);
        fft.performRealOnlyForwardTransform (data.data(), true);

        const float c = cur;
        if (std::fabs (c) > 0.005f)
        {
            // 2. log magnitude, mirrored into a real, even sequence
            for (int k = 0; k < kBins; ++k)
            {
                const float re = data[(size_t) (2 * k)], im = data[(size_t) (2 * k + 1)];
                lm[(size_t) k] = std::log (std::sqrt (re * re + im * im) + 1.0e-4f);
            }
            for (int n = 0; n < N; ++n)
                cx[(size_t) n] = std::complex<float> (lm[(size_t) (n <= N / 2 ? n : N - n)], 0.0f);
            // 3. cepstrum, keep the low quefrencies (the smooth envelope), back to the log spectrum
            fft.perform (cx.data(), cy.data(), false);
            for (int q = 0; q < N; ++q)
            {
                const int qq = q <= N / 2 ? q : N - q;
                const float lift = qq <= liftQ ? 1.0f : (qq < liftQ + 12 ? 0.5f + 0.5f * std::cos (3.14159265f * (float) (qq - liftQ) / 12.0f) : 0.0f);
                cy[(size_t) q] *= lift / (float) N;
            }
            fft.perform (cy.data(), cx.data(), false);
            for (int k = 0; k < kBins; ++k) env[(size_t) k] = cx[(size_t) k].real();
            // 4. slide the envelope: bin k takes the envelope that was at k / ratio
            const float inv = std::exp2 (-c / 12.0f);
            for (int k = 0; k < kBins; ++k)
            {
                const float src = std::min ((float) (kBins - 1), (float) k * inv);
                const int i0 = (int) src;  const int i1 = std::min (kBins - 1, i0 + 1);
                const float e2 = env[(size_t) i0] + (src - (float) i0) * (env[(size_t) i1] - env[(size_t) i0]);
                const float g = std::exp (std::max (-gmaxLog, std::min (gmaxLog, e2 - env[(size_t) k])));
                data[(size_t) (2 * k)] *= g;
                data[(size_t) (2 * k + 1)] *= g;
            }
        }
        // 5. back to the time domain, window again, and add (1.5 is the sum of the squared Hann window at a quarter-frame hop)
        fft.performRealOnlyInverseTransform (data.data());
        for (int i = 0; i < N; ++i)
            outAcc[(size_t) ((wp + i) % N)] += data[(size_t) i] * win[(size_t) i] * (1.0f / 1.5f);
    }

    static constexpr float gmaxLog = (float) GM10 / 10.0f;               // largest boost or cut per bin in natural-log units (2.3 = about 20 dB, 1.4 = about 12 dB)
    int liftQ = LQ;                                                     // quefrency limit in samples: about 0.6 ms (scaled with the sample rate), so only the few broad hills of the envelope are kept, not the harmonics
    juce::dsp::FFT fft;
    std::array<float, N> win {};
    std::array<float, N> inBuf {}, outAcc {};
    std::array<float, 2 * N> data {};
    std::array<float, kBins> lm {}, env {};
    std::array<std::complex<float>, N> cx {}, cy {};
    int wp = 0, hopCount = 0;
    float cur = 0.0f, target = 0.0f;
};

using FormantWarp = FormantWarpT<1024>;
using FormantWarpFast = FormantWarpT<256, 28, 14>;      // Transpose: 256-sample frames (5.3 ms), gentler limit (about 12 dB) and the same 28-sample lifter as the pedal; on the G2 captures at -12, +7 and +12 it scored the same as 512-sample frames
using FormantWarpQuick = FormantWarpT<128, 28, 14>;   // b117: Transpose in Fast / Low Latency mode: 128-sample frames, 2.7 ms of delay instead of 5.3 (same lifter and limit; the envelope is a little blurrier on low notes)

// ---------------------------------------------------------------------------------------------------------------------
class DryLowSplit
{
public:
    void prepare (double sampleRate) { sr = sampleRate;  hz = -1.0f;  reset(); }
    void reset() { for (auto& b : hpS) b = {};  for (auto& b : lpS) b = {}; }
    void setHz (float f)
    {
        f = std::max (30.0f, std::min (600.0f, f));
        if (std::fabs (f - hz) < 0.25f) return;
        hz = f;
        const double w = 6.283185307179586 * (double) f / sr, cs = std::cos (w), sn = std::sin (w), al = sn / (2.0 * 0.70710678);
        const double a0 = 1.0 + al;
        a1 = (float) (-2.0 * cs / a0);  a2 = (float) ((1.0 - al) / a0);
        lb0 = (float) ((1.0 - cs) * 0.5 / a0);  lb1 = (float) ((1.0 - cs) / a0);  lb2 = lb0;
        hb0 = (float) ((1.0 + cs) * 0.5 / a0);  hb1 = (float) (-(1.0 + cs) / a0);  hb2 = hb0;
    }
    float hp (float x) { return run (hpS[1], hb0, hb1, hb2, run (hpS[0], hb0, hb1, hb2, x)); }
    float lp (float x) { return run (lpS[1], lb0, lb1, lb2, run (lpS[0], lb0, lb1, lb2, x)); }

private:
    struct S { float z1 = 0.0f, z2 = 0.0f; };
    float run (S& s, float b0, float b1, float b2, float x) const
    {
        const float y = b0 * x + s.z1;
        s.z1 = b1 * x - a1 * y + s.z2;
        s.z2 = b2 * x - a2 * y;
        return y;
    }
    S hpS[2], lpS[2];
    float a1 = 0.0f, a2 = 0.0f, lb0 = 1.0f, lb1 = 0.0f, lb2 = 0.0f, hb0 = 1.0f, hb1 = 0.0f, hb2 = 0.0f, hz = -1.0f;
    double sr = 48000.0;
};
