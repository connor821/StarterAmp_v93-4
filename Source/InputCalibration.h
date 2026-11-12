#pragma once
// Input calibration (b94). Two short measurements of what your guitar and interface actually deliver, then a recommended Input level,
// Gate threshold and (if there is mains hum) the De-Noise setting.
//
//   1. Quiet (3 s): you mute the strings and do not play. Measured: the noise floor (RMS, and the peak the gate's own envelope follower reaches),
//      any DC offset, and mains hum (a Hann-windowed narrow-band measurement at 50 and 60 Hz and their harmonics).
//   2. Play (8 s): you play your loudest normal part. Measured: the peak of every 5 ms block, so a pick click or two does not decide the
//      level; the level that 99% of the active blocks stay under is the "hard playing" level. Also the highest sample and whether any sample hit full scale.
//
// The Input level recommendation puts the hard-playing level at kTargetPeakDb (-10 dBFS), which is the peak level the Auto Level tables and the amp
// fits were measured at (a plucked-note test signal with a peak of 0.3, about -10.5 dBFS). The gate threshold goes 6 dB above the loudest noise the
// gate's detector would see after the new Input level is applied.
//
// Real-time safe: no allocation and no locks; the audio thread owns every counter, the message thread only sets requests and reads atomics and,
// once the phase is 'done', the result.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>

class InputCalibrator
{
public:
    static constexpr float  kTargetPeakDb = -10.0f;
    static constexpr double kQuietSettle  = 0.6;       // seconds ignored at the start of the quiet step (a hand settling on the strings)
    static constexpr double kQuietMeasure = 2.4;       // seconds measured
    static constexpr double kPlayTime     = 8.0;
    enum Phase { idle = 0, quiet = 1, play = 2, done = 3 };

    struct Result
    {
        bool  heardPlaying = false;           // enough blocks well above the noise to judge the playing level
        // quiet step
        float noiseRmsDb = -120.0f;           // RMS of the noise (DC removed), dBFS
        float noiseEnvDb = -120.0f;           // the highest the gate's peak envelope reached, dBFS
        float dc = 0.0f;                      // mean value
        bool  hum = false;  int humHz = 0;  float humDb = -120.0f;     // 50 or 60, and the hum's RMS level
        // play step
        float p99Db = -120.0f, medianDb = -120.0f, maxDb = -120.0f;    // hard-playing level, typical level, highest sample
        int   clipped = 0;                    // samples at (or within 0.01 of) full scale
        float snrDb = 0.0f;                   // hard-playing level against the noise envelope
        // advice
        float recInGain = 0.0f;               // dB, within the Input knob's -24..+24
        float recGate = -65.0f;               // dB, within the Gate knob's -90..-30
        int   outputClass = 1;                // 0 quiet, 1 medium, 2 hot, 3 very hot
        bool  gainLimited = false;            // the recommendation wanted more than the knob's range
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        quietSettleN  = (int64_t) (kQuietSettle * sr);
        quietMeasureN = (int64_t) (kQuietMeasure * sr);
        playN         = (int64_t) (kPlayTime * sr);
        blockLen      = std::max (16, (int) std::lround (0.005 * sr));
        envRelease    = (float) std::exp (-1.0 / (0.050 * sr));
        for (int i = 0; i < kHumFreqs; ++i)
            coeff[i] = 2.0 * std::cos (6.283185307179586 * kFreqs[i] / sr);
        phase.store (idle);
        startReq.store (false);
        cancelReq.store (false);
    }

    // message thread
    void start()  { cancelReq.store (false);  startReq.store (true, std::memory_order_release); }
    void cancel() { startReq.store (false);   cancelReq.store (true, std::memory_order_release); }
    int  currentPhase() const { return phase.load (std::memory_order_acquire); }
    int  completedRuns() const { return runs.load (std::memory_order_acquire); }      // counts finished measurements, so a panel can tell a new result from an old one
    bool isActive() const
    {
        const int p = phase.load (std::memory_order_relaxed);
        return p == quiet || p == play || startReq.load (std::memory_order_relaxed) || cancelReq.load (std::memory_order_relaxed);
    }
    float progress() const { return prog.load (std::memory_order_relaxed); }       // 0..1 within the current step
    float liveLevelDb() const { return live.load (std::memory_order_relaxed); }    // the last block's peak, for a meter
    Result result() const { return res; }                                           // read when currentPhase() == done

    // audio thread: the mono input, before the Input level is applied
    void feed (const float* x, int n)
    {
        if (cancelReq.exchange (false)) { phase.store (idle, std::memory_order_release);  return; }
        if (startReq.exchange (false)) beginQuiet();
        const int ph = phase.load (std::memory_order_relaxed);
        if (ph != quiet && ph != play) return;

        for (int i = 0; i < n; ++i)
        {
            float v = x[i];
            if (! (v == v)) v = 0.0f;                                   // b95: NaN and infinite samples must not poison a measurement
            else v = std::max (-64.0f, std::min (64.0f, v));
            if (ph == quiet) { if (feedQuiet (v)) { endQuiet();  feedFrom (x + i + 1, n - i - 1);  return; } }
            else             { if (feedPlay (v))  { endPlay();   return; } }
        }
    }

    // exposed for tests: the advice for a measurement (so the maths can be checked without audio)
    static void advise (Result& r)
    {
        r.outputClass = r.p99Db > -6.0f ? 3 : r.p99Db > -14.0f ? 2 : r.p99Db > -22.0f ? 1 : 0;
        float g = kTargetPeakDb - r.p99Db;
        r.gainLimited = g > 24.0f || g < -24.0f;
        g = std::max (-24.0f, std::min (24.0f, g));
        r.recInGain = std::round (g * 2.0f) / 2.0f;
        const float envAfter = r.noiseEnvDb + r.recInGain;
        float gate = envAfter > -110.0f ? envAfter + 6.0f : -80.0f;
        r.recGate = std::max (-90.0f, std::min (-30.0f, std::round (gate)));
        r.snrDb = r.p99Db - r.noiseEnvDb;
    }

private:
    static constexpr int kHumFreqs = 8;
    static constexpr double kFreqs[kHumFreqs] = { 50.0, 100.0, 150.0, 200.0, 60.0, 120.0, 180.0, 240.0 };
    static constexpr int kBins = 400;                                               // 0.25 dB bins from -100 to 0 dBFS

    void feedFrom (const float* x, int n) { if (n > 0) feed (x, n); }

    void beginQuiet()
    {
        qIdx = 0;  qSumSq = 0.0;  qSum = 0.0;  qEnv = 0.0f;  qEnvMax = 0.0f;
        for (int i = 0; i < kHumFreqs; ++i) { gs1[i] = 0.0;  gs2[i] = 0.0; }
        prog.store (0.0f);
        phase.store (quiet, std::memory_order_release);
    }

    // returns true when the step is over
    bool feedQuiet (float v)
    {
        const float a = std::abs (v);
        qEnv = (a > qEnv) ? a : a + envRelease * (qEnv - a);
        if (qIdx >= quietSettleN)
        {
            const int64_t k = qIdx - quietSettleN;
            qSumSq += (double) v * v;  qSum += v;
            qEnvMax = std::max (qEnvMax, qEnv);
            const double w = 0.5 - 0.5 * std::cos (6.283185307179586 * (double) k / (double) quietMeasureN);       // Hann
            const double wv = w * v;
            for (int i = 0; i < kHumFreqs; ++i)
            {
                const double s0 = wv + coeff[i] * gs1[i] - gs2[i];
                gs2[i] = gs1[i];  gs1[i] = s0;
            }
        }
        ++qIdx;
        if ((qIdx & 1023) == 0) prog.store ((float) std::min (1.0, (double) qIdx / (double) (quietSettleN + quietMeasureN)), std::memory_order_relaxed);
        return qIdx >= quietSettleN + quietMeasureN;
    }

    void endQuiet()
    {
        Result r;
        const double N = (double) quietMeasureN;
        const double mean = qSum / N;
        const double var = std::max (0.0, qSumSq / N - mean * mean);
        r.dc = (float) mean;
        r.noiseRmsDb = var > 1.0e-14 ? (float) (10.0 * std::log10 (var)) : -120.0f;
        r.noiseEnvDb = qEnvMax > 1.0e-6f ? 20.0f * std::log10 (qEnvMax) : -120.0f;
        // hum: Hann-weighted Goertzel power -> sinusoid amplitude (a = 4 sqrt(P) / N, since the window's coherent gain is 0.5), RMS power a^2 / 2
        double fam[2] = { 0.0, 0.0 };
        for (int i = 0; i < kHumFreqs; ++i)
        {
            const double P = gs1[i] * gs1[i] + gs2[i] * gs2[i] - coeff[i] * gs1[i] * gs2[i];
            const double a = 4.0 * std::sqrt (std::max (0.0, P)) / N;
            fam[i / 4] += 0.5 * a * a;                  // 0: the 50 Hz family, 1: the 60 Hz family
        }
        const int best = fam[1] > fam[0] ? 1 : 0;
        const double humPow = fam[best];
        r.humDb = humPow > 1.0e-14 ? (float) (10.0 * std::log10 (humPow)) : -120.0f;
        r.humHz = best == 1 ? 60 : 50;
        r.hum = var > 1.0e-14 && humPow > 0.3 * var && r.humDb > -88.0f;
        if (! r.hum) r.humHz = 0;
        res = r;
        // the play step
        for (auto& b : hist) b = 0;
        pIdx = 0;  pBlockPos = 0;  pBlockPeak = 0.0f;  pClip = 0;  pMax = 0.0f;  pBlocks = 0;
        prog.store (0.0f);
        phase.store (play, std::memory_order_release);
    }

    bool feedPlay (float v)
    {
        const float a = std::abs (v);
        pBlockPeak = std::max (pBlockPeak, a);
        if (a >= 0.99f) ++pClip;
        pMax = std::max (pMax, a);
        ++pIdx;
        if (++pBlockPos >= blockLen)
        {
            const float db = pBlockPeak > 1.0e-5f ? 20.0f * std::log10 (pBlockPeak) : -100.0f;
            live.store (db, std::memory_order_relaxed);
            const int bin = std::max (0, std::min (kBins - 1, (int) ((db + 100.0f) * 4.0f)));
            ++hist[bin];  ++pBlocks;
            pBlockPos = 0;  pBlockPeak = 0.0f;
            prog.store ((float) std::min (1.0, (double) pIdx / (double) playN), std::memory_order_relaxed);
        }
        return pIdx >= playN;
    }

    void endPlay()
    {
        Result r = res;
        // active blocks: well above the noise (12 dB over the gate's envelope, and at least -70 dBFS)
        const float floorDb = std::max (-70.0f, r.noiseEnvDb + 12.0f);
        const int floorBin = std::max (0, std::min (kBins - 1, (int) ((floorDb + 100.0f) * 4.0f)));
        uint32_t active = 0;
        for (int b = floorBin; b < kBins; ++b) active += hist[b];
        r.maxDb = pMax > 1.0e-5f ? 20.0f * std::log10 (pMax) : -100.0f;
        r.clipped = pClip;
        r.heardPlaying = active >= (uint32_t) std::max (20, (int) (0.4 / 0.005));        // at least about 0.4 s of playing
        if (r.heardPlaying)
        {
            auto levelAtFractionFromTop = [&] (double frac)
            {
                const uint32_t want = (uint32_t) std::max (1.0, std::ceil (frac * (double) active));
                uint32_t acc = 0;
                for (int b = kBins - 1; b >= floorBin; --b)
                {
                    acc += hist[b];
                    if (acc >= want) return (float) b / 4.0f - 100.0f + 0.125f;
                }
                return floorDb;
            };
            r.p99Db = levelAtFractionFromTop (0.01);
            r.medianDb = levelAtFractionFromTop (0.5);
            advise (r);
        }
        res = r;
        prog.store (1.0f);
        runs.fetch_add (1, std::memory_order_release);
        phase.store (done, std::memory_order_release);
    }

    double sr = 48000.0;
    int64_t quietSettleN = 0, quietMeasureN = 0, playN = 0;
    int blockLen = 240;
    float envRelease = 0.0f;
    double coeff[kHumFreqs] {};
    std::atomic<int> phase { idle }, runs { 0 };
    std::atomic<bool> startReq { false }, cancelReq { false };
    std::atomic<float> prog { 0.0f }, live { -100.0f };
    Result res;
    // quiet step
    int64_t qIdx = 0;
    double qSumSq = 0.0, qSum = 0.0, gs1[kHumFreqs] {}, gs2[kHumFreqs] {};
    float qEnv = 0.0f, qEnvMax = 0.0f;
    // play step
    int64_t pIdx = 0;
    int pBlockPos = 0, pClip = 0;
    float pBlockPeak = 0.0f, pMax = 0.0f;
    uint32_t hist[kBins] {};
    uint32_t pBlocks = 0;
};
