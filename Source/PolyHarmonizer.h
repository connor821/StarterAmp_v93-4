#pragma once
// Chord (polyphonic) harmonizing, v53. Everything here is my own design.
//
//   ChordDetector   listens to the clean input and works out which notes are sounding. It looks at the
//                   spectrum, scores every possible note by how many of its harmonics are present, picks the
//                   best, removes that note's harmonics, and repeats (up to 6 notes).
//   SpectralShifter moves each note by its own amount. It is a phase-vocoder pitch shifter: the spectrum is
//                   split into regions around each peak, and each region is shifted by the amount for the
//                   note that peak belongs to. That is the part a normal pitch shifter cannot do, since it
//                   moves every note by the same ratio.
//
// A spectral shifter needs a long analysis window, so this mode is slower than the single-note mode
// (about 46 ms for a 6-string guitar, 93 ms for 8 strings). The single-note mode stays the default.
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>

namespace polyh
{
    inline int orderOf (int n) { int o = 0; while ((1 << o) < n) ++o; return o; }
    inline int pow2Below (double v) { int p = 256; while ((double) p * 2.0 <= v) p *= 2; return p; }
    inline double wrapPi (double x) { return x - 6.283185307179586 * std::floor (x / 6.283185307179586 + 0.5); }
    inline float  midiOfHz (float hz) { return 69.0f + 12.0f * std::log2 (std::max (hz, 1.0e-3f) / 440.0f); }
    inline float  hzOfMidi (float m)  { return 440.0f * std::pow (2.0f, (m - 69.0f) / 12.0f); }
    inline float  hannAt (int i, int n) { return 0.5f - 0.5f * std::cos (6.2831853f * (float) i / (float) n); }

    // The stretch of a string's upper harmonics (they sit slightly above whole multiples of the pitch)
    inline float stretch (int h, float B) { return std::sqrt (1.0f + B * (float) (h * h)); }
}

//==============================================================================
class ChordDetector
{
public:
    static constexpr int kMaxNotes = 6;
    struct Note
    {
        float f0 = 0.0f;          // Hz
        int   age = 0;            // frames it has been heard in a row
        int   missed = 0;         // frames since it was last heard
        bool  confirmed() const { return age >= 2; }
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        N = polyh::pow2Below (sampleRate / 10.0);              // 44.1 kHz -> 4096 (10.8 Hz per bin)
        hop = N / 8;
        fft = std::make_unique<juce::dsp::FFT> (polyh::orderOf (N));
        win.assign ((size_t) N, 0.0f);
        for (int i = 0; i < N; ++i) win[(size_t) i] = polyh::hannAt (i, N);
        ring.assign ((size_t) N, 0.0f);
        buf.assign ((size_t) (2 * N), 0.0f);
        mag.assign ((size_t) (N / 2 + 2), 0.0f);
        reset();
    }
    void reset()
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        pos = 0;  sinceHop = 0;  nTracks = 0;
    }

    void feed (const float* x, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            ring[(size_t) pos] = x[i];
            if (++pos >= N) pos = 0;
            if (++sinceHop >= hop) { sinceHop = 0; analyse(); }
        }
    }

    // Notes that have been heard for two frames in a row
    int  numConfirmed() const { int c = 0; for (int i = 0; i < nTracks; ++i) if (tracks[i].confirmed()) ++c; return c; }
    bool confirmedAt (int idx, Note& out) const
    {
        int c = 0;
        for (int i = 0; i < nTracks; ++i)
            if (tracks[i].confirmed()) { if (c++ == idx) { out = tracks[i]; return true; } }
        return false;
    }

private:
    float sr = 44100.0f;
    int   N = 4096, hop = 512, pos = 0, sinceHop = 0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> win, ring, buf, mag;

    // Peaks of the current frame
    static constexpr int kMaxPeaks = 120;
    float pf[kMaxPeaks], pa[kMaxPeaks], pr[kMaxPeaks];       // Hz, amplitude, amplitude left after removing found notes
    int   nPeaks = 0;
    float maxA = 0.0f;

    Note tracks[10];
    int  nTracks = 0;

    static constexpr float kStretchB = 0.0002f, kTolCents = 40.0f;

    // Nearest peak to 'hz' within the tolerance (strongest one if several), or -1
    int findPeak (float hz) const
    {
        const float lo = hz * std::pow (2.0f, -kTolCents / 1200.0f), hi = hz * std::pow (2.0f, kTolCents / 1200.0f);
        int a = 0, b = nPeaks;                                  // peaks are in ascending frequency
        while (a < b) { const int m = (a + b) / 2; if (pf[m] < lo) a = m + 1; else b = m; }
        int best = -1;
        for (int j = a; j < nPeaks && pf[j] <= hi; ++j)
            if (best < 0 || pa[j] > pa[best]) best = j;
        return best;
    }

    struct Score { float sal = 0.0f; int cover = 0; int unique = 0; };
    Score score (float f0) const
    {
        Score s;  bool h1 = false;
        for (int h = 1; h <= 8; ++h)
        {
            const float ft = f0 * (float) h * polyh::stretch (h, kStretchB);
            if (ft > 7500.0f) break;
            const int j = findPeak (ft);
            if (j < 0) continue;
            const float w = h <= 4 ? 1.0f : (h <= 6 ? 0.8f : 0.6f);
            s.sal += w * std::sqrt (pr[j] / maxA);
            if (h <= 6 && pa[j] >= 0.02f * maxA) { ++s.cover; if (pr[j] >= 0.3f * pa[j]) ++s.unique; }
            if (h == 1 && pa[j] >= 0.02f * maxA) h1 = true;
        }
        if (! h1) s.cover = 0;                  // the pitch itself must be there: a chord's partials also look like a note an octave or two below
        return s;
    }

    void cancel (float f0)
    {
        for (int h = 1; h <= 16; ++h)
        {
            const float ft = f0 * (float) h * polyh::stretch (h, kStretchB);
            if (ft > 7500.0f) break;
            const int j = findPeak (ft);
            if (j >= 0) pr[j] *= 0.15f;
        }
    }

    float refine (float f0) const
    {
        double num = 0.0, den = 0.0;
        for (int h = 1; h <= 8; ++h)
        {
            const float ft = f0 * (float) h * polyh::stretch (h, kStretchB);
            if (ft > 7500.0f) break;
            const int j = findPeak (ft);
            if (j < 0) continue;
            num += (double) pa[j] * (double) (pf[j] / ((float) h * polyh::stretch (h, kStretchB)));
            den += (double) pa[j];
        }
        return den > 0.0 ? (float) (num / den) : f0;
    }

    void findPeaks()
    {
        const int K = N / 2;
        const float binHz = sr / (float) N;
        const int kLo = std::max (3, (int) std::ceil (30.0f / binHz)), kHi = std::min (K - 3, (int) (8000.0f / binHz));
        const float toAmp = 4.0f / (float) N;                    // Hann window: a sine of amplitude A reads A*N/4
        float top = 0.0f;
        for (int k = kLo; k < kHi; ++k) top = std::max (top, mag[(size_t) k]);
        maxA = top * toAmp;
        nPeaks = 0;
        if (maxA < 0.0015f) return;                              // silence (about -56 dBFS)
        const float floorMag = std::max (0.01f * top, 0.0007f / toAmp);
        for (int k = kLo; k < kHi && nPeaks < kMaxPeaks; ++k)
        {
            const float m = mag[(size_t) k];
            if (m < floorMag) continue;
            if (! (m > mag[(size_t) k - 1] && m >= mag[(size_t) k + 1] && m > mag[(size_t) k - 2] && m >= mag[(size_t) k + 2])) continue;
            const float a = std::log (mag[(size_t) k - 1] + 1.0e-20f), b = std::log (m), c = std::log (mag[(size_t) k + 1] + 1.0e-20f);
            const float den = a - 2.0f * b + c;
            const float p = std::abs (den) > 1.0e-9f ? std::min (0.5f, std::max (-0.5f, 0.5f * (a - c) / den)) : 0.0f;
            pf[nPeaks] = ((float) k + p) * binHz;
            pa[nPeaks] = std::exp (b - 0.25f * (a - c) * p) * toAmp;
            pr[nPeaks] = pa[nPeaks];
            ++nPeaks;
        }
    }

    void analyse()
    {
        for (int i = 0; i < N; ++i) buf[(size_t) i] = ring[(size_t) ((pos + i) % N)] * win[(size_t) i];
        std::fill (buf.begin() + N, buf.end(), 0.0f);
        fft->performRealOnlyForwardTransform (buf.data(), true);
        for (int k = 0; k <= N / 2; ++k)
            mag[(size_t) k] = std::sqrt (buf[(size_t) (2 * k)] * buf[(size_t) (2 * k)] + buf[(size_t) (2 * k + 1)] * buf[(size_t) (2 * k + 1)]);
        findPeaks();

        float found[kMaxNotes];  int nFound = 0;
        if (nPeaks >= 3)
        {
            float firstSal = 0.0f;
            for (int iter = 0; iter < kMaxNotes; ++iter)
            {
                // Score every note from E1 to E6, keep the ones that could really be a new note
                float sal[64];  float bestSal = 0.0f;  int bestM = -1;
                for (int m = 28; m <= 88; ++m)
                {
                    const Score sc = score (polyh::hzOfMidi ((float) m));
                    bool ok = sc.cover >= 5;
                    if (ok) ok = (iter == 0) ? sc.sal >= 1.5f : (sc.sal >= 0.28f * firstSal && sc.unique >= 2);
                    sal[m - 28] = ok ? sc.sal : 0.0f;
                    if (sal[m - 28] > bestSal) { bestSal = sal[m - 28]; bestM = m; }
                }
                if (bestM < 0) break;
                // Prefer the lowest note that scores nearly as well (an octave-up candidate shares the real note's partials)
                for (int m = 28; m < bestM; ++m)
                    if (sal[m - 28] >= 0.8f * bestSal) { bestM = m; bestSal = sal[m - 28]; break; }
                if (iter == 0) firstSal = bestSal;
                const float f0 = refine (polyh::hzOfMidi ((float) bestM));
                found[nFound++] = f0;
                cancel (f0);
            }
        }

        // Follow the notes from frame to frame
        bool used[10] = {};
        for (int i = 0; i < nFound; ++i)
        {
            int match = -1;
            for (int t = 0; t < nTracks; ++t)
                if (std::abs (1200.0f * std::log2 (found[i] / tracks[t].f0)) < 60.0f) { match = t; break; }
            if (match >= 0)
            {
                tracks[match].f0 += 0.5f * (found[i] - tracks[match].f0);
                tracks[match].age++;  tracks[match].missed = 0;  used[match] = true;
            }
            else if (nTracks < 10)
            {
                tracks[nTracks] = Note();  tracks[nTracks].f0 = found[i];  tracks[nTracks].age = 1;
                used[nTracks] = true;  ++nTracks;
            }
        }
        int w = 0;
        for (int t = 0; t < nTracks; ++t)
        {
            if (! used[t]) ++tracks[t].missed;
            if (tracks[t].missed > 5) continue;                   // gone for about 70 ms
            tracks[w] = tracks[t];  used[w] = used[t];  ++w;
        }
        nTracks = w;
    }
};

//==============================================================================
class SpectralShifter
{
public:
    struct NoteMap { float f0 = 0.0f; float alpha[2] = { 1.0f, 1.0f }; };

    // Set by the engine before each block
    bool  scaleMode = false;
    int   key = 0, scale = 0, deg[2] = { 2, 4 };
    float alphaInterval[2] = { 1.0f, 1.0f };
    int   nNotes = 0;
    NoteMap notes[10];

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        maxN = 2 * polyh::pow2Below (sampleRate * 0.05);          // 44.1 kHz -> up to 4096
        for (int o = 0; o < 2; ++o)
        {
            const int n = (maxN >> 1) << o;
            fftSet[o] = std::make_unique<juce::dsp::FFT> (polyh::orderOf (n));
        }
        setSize (maxN >> 1);
    }

    // n is the shorter (maxN/2) or the longer (maxN) window
    void setSize (int n)
    {
        N = (n >= maxN) ? maxN : (maxN >> 1);
        H = N / 4;  K = N / 2;
        fft = fftSet[N == maxN ? 1 : 0].get();
        win.assign ((size_t) N, 0.0f);
        for (int i = 0; i < N; ++i) win[(size_t) i] = polyh::hannAt (i, N);
        inRing.assign ((size_t) N, 0.0f);
        buf.assign ((size_t) (2 * N), 0.0f);
        spec.assign ((size_t) (2 * N), 0.0f);
        mag.assign ((size_t) (K + 2), 0.0f);
        ph.assign ((size_t) (K + 2), 0.0);
        prevPh.assign ((size_t) (K + 2), 0.0);
        yRe.assign ((size_t) (K + 2), 0.0f);  yIm.assign ((size_t) (K + 2), 0.0f);  yMag.assign ((size_t) (K + 2), 0.0f);
        pk.assign ((size_t) (K + 2), 0);
        for (int v = 0; v < 2; ++v)
        {
            ola[v].assign ((size_t) N, 0.0f);
            for (int s = 0; s < 2; ++s) { stamp[v][s].assign ((size_t) (K + 2), -10); psi[v][s].assign ((size_t) (K + 2), 0.0f); }
        }
        reset();
    }
    int  windowSize() const { return N; }
    int  latency() const { return N; }

    void reset()
    {
        std::fill (inRing.begin(), inRing.end(), 0.0f);
        for (int v = 0; v < 2; ++v)
        {
            std::fill (ola[v].begin(), ola[v].end(), 0.0f);
            for (int s = 0; s < 2; ++s) std::fill (stamp[v][s].begin(), stamp[v][s].end(), -10);
        }
        std::fill (prevPh.begin(), prevPh.end(), 0.0);
        wp = 0;  rp = 0;  hopCount = 0;  frameId = 0;  haveFrame = false;
    }

    void process (float x, float& y0, float& y1)
    {
        inRing[(size_t) wp] = x;
        if (++wp >= N) wp = 0;
        y0 = ola[0][(size_t) rp];  ola[0][(size_t) rp] = 0.0f;
        y1 = ola[1][(size_t) rp];  ola[1][(size_t) rp] = 0.0f;
        if (++rp >= N) rp = 0;
        if (++hopCount >= H) { hopCount = 0; doFrame(); }
    }

private:
    float sr = 44100.0f;
    int   maxN = 4096, N = 2048, H = 512, K = 1024;
    int   wp = 0, rp = 0, hopCount = 0, frameId = 0;
    bool  haveFrame = false;
    std::unique_ptr<juce::dsp::FFT> fftSet[2];
    juce::dsp::FFT* fft = nullptr;
    std::vector<float> win, inRing, buf, spec, mag, yRe, yIm, yMag, ola[2];
    std::vector<double> ph, prevPh;
    std::vector<int> pk;
    std::vector<int>   stamp[2][2];          // [voice][frame parity]: frame number at which a shifted peak sat on this bin
    std::vector<float> psi[2][2];            //                         its synthesis phase

    // The shift (as a frequency ratio) for a spectral peak at 'hz', for voice v
    float alphaFor (int v, float hz) const
    {
        if (! scaleMode) return alphaInterval[v];
        // 1. Does it belong to one of the notes we heard? (a harmonic of its pitch; lowest harmonic number wins)
        int bestNote = -1, bestH = 99;
        for (int i = 0; i < nNotes; ++i)
        {
            const float r = hz / notes[i].f0;
            const int h = (int) std::lround (r);
            if (h < 1 || h > 24 || h >= bestH) continue;
            const float cents = 1200.0f * std::log2 (r / ((float) h * polyh::stretch (h, 0.0003f)));
            if (std::abs (cents) <= 35.0f) { bestNote = i; bestH = h; }
        }
        if (bestNote >= 0) return notes[bestNote].alpha[v];
        // 2. Otherwise follow the pitch of the peak itself (right for the strong low partials of a new note)
        if (hz < 25.0f) return 1.0f;
        const float shift = harmonyShift (polyh::midiOfHz (hz), key, scale, deg[v]);
        return std::pow (2.0f, shift / 12.0f);
    }

    void doFrame()
    {
        const double twoPi = 6.283185307179586;
        for (int i = 0; i < N; ++i) buf[(size_t) i] = inRing[(size_t) ((wp + i) % N)] * win[(size_t) i];
        std::fill (buf.begin() + N, buf.end(), 0.0f);
        fft->performRealOnlyForwardTransform (buf.data(), true);
        std::copy (buf.begin(), buf.end(), spec.begin());
        float top = 0.0f;
        for (int k = 0; k <= K; ++k)
        {
            const float re = spec[(size_t) (2 * k)], im = spec[(size_t) (2 * k + 1)];
            mag[(size_t) k] = std::sqrt (re * re + im * im);
            ph[(size_t) k] = std::atan2 ((double) im, (double) re);
            top = std::max (top, mag[(size_t) k]);
        }
        ++frameId;
        const int cur = frameId & 1, prv = cur ^ 1;

        // Peaks, then the region of bins that belongs to each one (split at the valley between two peaks)
        int np = 0;
        const float floorMag = std::max (top * 1.0e-3f, 1.0e-7f * (float) N);
        for (int k = 2; k < K - 1 && top > 0.0f; ++k)
        {
            const float m = mag[(size_t) k];
            if (m >= floorMag && m > mag[(size_t) k - 1] && m >= mag[(size_t) k + 1] && m > mag[(size_t) k - 2] && m >= mag[(size_t) k + 2])
                pk[(size_t) np++] = k;
        }
        for (int v = 0; v < 2; ++v)
        {
            std::fill (yRe.begin(), yRe.end(), 0.0f);  std::fill (yIm.begin(), yIm.end(), 0.0f);  std::fill (yMag.begin(), yMag.end(), 0.0f);
            for (int j = 0; j < np; ++j)
            {
                const int kp = pk[(size_t) j];
                int lo = 0, hi = K;
                if (j > 0)      { int b = pk[(size_t) j - 1], bm = b; for (int k = b; k <= kp; ++k) if (mag[(size_t) k] < mag[(size_t) bm]) bm = k; lo = bm; }
                if (j + 1 < np) { int b = pk[(size_t) j + 1], bm = kp; for (int k = kp; k <= b; ++k) if (mag[(size_t) k] < mag[(size_t) bm]) bm = k; hi = bm; }
                // Instantaneous frequency of the peak (rad per sample)
                const double expect = twoPi * (double) kp * (double) H / (double) N;
                const double dev = haveFrame ? polyh::wrapPi (ph[(size_t) kp] - prevPh[(size_t) kp] - expect) : 0.0;
                const double omega = twoPi * (double) kp / (double) N + dev / (double) H;
                const float hz = (float) (omega * (double) sr / twoPi);
                const double alpha = (double) alphaFor (v, hz);
                const double omegaT = alpha * omega;
                const int kt = (int) std::lround (omegaT * (double) N / twoPi);
                const int shift = kt - kp;
                // Synthesis phase: continue the track of the shifted peak from the last frame
                double base = ph[(size_t) kp];
                bool have = false;
                for (int d = 0; d <= 2 && ! have; ++d)
                    for (int sgn = -1; sgn <= 1 && ! have; sgn += 2)
                    {
                        const int b = kt + sgn * d;
                        if (b >= 1 && b < K && stamp[v][prv][(size_t) b] == frameId - 1) { base = (double) psi[v][prv][(size_t) b]; have = true; }
                    }
                const double psiNew = polyh::wrapPi (base + (double) H * omegaT);
                if (kt >= 1 && kt < K) { stamp[v][cur][(size_t) kt] = frameId;  psi[v][cur][(size_t) kt] = (float) psiNew; }
                const double theta = psiNew - ph[(size_t) kp];
                const float c = (float) std::cos (theta), s = (float) std::sin (theta);
                for (int k = lo; k <= hi; ++k)
                {
                    const int kk = k + shift;
                    if (kk < 1 || kk >= K) continue;
                    const float re = spec[(size_t) (2 * k)], im = spec[(size_t) (2 * k + 1)];
                    const float zr = re * c - im * s, zi = re * s + im * c;
                    const float zm = zr * zr + zi * zi;
                    if (zm > yMag[(size_t) kk]) { yMag[(size_t) kk] = zm; yRe[(size_t) kk] = zr; yIm[(size_t) kk] = zi; }
                }
            }
            // Back to the time domain, window again, and add into the output
            std::fill (buf.begin(), buf.end(), 0.0f);
            for (int k = 1; k < K; ++k)
            {
                buf[(size_t) (2 * k)] = yRe[(size_t) k];       buf[(size_t) (2 * k + 1)] = yIm[(size_t) k];
                buf[(size_t) (2 * (N - k))] = yRe[(size_t) k]; buf[(size_t) (2 * (N - k) + 1)] = -yIm[(size_t) k];
            }
            fft->performRealOnlyInverseTransform (buf.data());
            const float scaleOut = 1.0f / 1.5f;
            for (int i = 0; i < N; ++i)
                ola[v][(size_t) ((rp + i) % N)] += buf[(size_t) i] * win[(size_t) i] * scaleOut;
        }
        for (int k = 0; k <= K; ++k) prevPh[(size_t) k] = ph[(size_t) k];
        haveFrame = true;
    }
};
