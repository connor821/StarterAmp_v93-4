#pragma once
// Polyphonic tuner (b44): reads every string of a strummed chord at once, against targets you choose.
//
// How it works: the last third of a second of the input is windowed (Hann), zero-padded and Fourier-transformed. Every clear
// peak in the spectrum is located to a fraction of a bin (a parabola through the log magnitudes). Then the strings are taken
// from the lowest to the highest. For each one, the peaks that sit near its harmonics (1x, 2x, 3x ... of the target pitch,
// within 100 cents) are collected, each one gives an offset in cents, and the largest group of peaks that agree on one offset
// is that string. The peaks it uses are claimed, so a higher string cannot reuse them.
//
// The hard case: in standard guitar tuning the B and the high E strings have every harmonic in common with the low E and A
// strings (the low E's third harmonic is a B, its fourth is an E). When such a string has no peak of its own, a second step
// looks for it with a least-squares fit that already knows where the lower strings' harmonics are, so a string a few cents to a
// few tens of cents from one of them is still found. When it is closer than that, its peaks merge with the lower string's and
// cannot be told apart: a string seen cleanly a moment ago keeps being reported (marked "shared") using the merged peak's offset,
// which is good to about 10 cents. Pluck it alone for an exact reading.
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <complex>
#include <memory>
#include <vector>
#include <juce_dsp/juce_dsp.h>

class PolyTuner
{
public:
    static constexpr int kMaxStrings = 8;

    struct Result
    {
        int   count = 0;
        bool  found[kMaxStrings] {};        // a reading for this string (clean or shared)
        bool  shared[kMaxStrings] {};       // approximate: the string sits inside a lower string's overtones (read by a fit, or by the merged peak)
        float cents[kMaxStrings] {};        // how far from the target (positive: sharp)
        float hz[kMaxStrings] {};           // the measured pitch of the string
        float level[kMaxStrings] {};        // strength of the string's peaks, linear
    };

    // Tunings. Notes are MIDI note numbers, lowest string first. The last entry is "Custom" (whatever the string notes are set to).
    struct Preset { const char* name; int count; int notes[kMaxStrings]; };
    static constexpr int kNumPresets = 16;
    static const Preset* presets()
    {
        static const Preset p[kNumPresets] = {
            { "Guitar 6: Standard (E A D G B E)",         6, { 40, 45, 50, 55, 59, 64, 69, 74 } },
            { "Guitar 6: Drop D",                          6, { 38, 45, 50, 55, 59, 64, 69, 74 } },
            { "Guitar 6: Half step down (Eb)",             6, { 39, 44, 49, 54, 58, 63, 69, 74 } },
            { "Guitar 6: Whole step down (D)",             6, { 38, 43, 48, 53, 57, 62, 69, 74 } },
            { "Guitar 6: DADGAD",                          6, { 38, 45, 50, 55, 57, 62, 69, 74 } },
            { "Guitar 6: Open G (D G D G B D)",            6, { 38, 43, 50, 55, 59, 62, 69, 74 } },
            { "Guitar 6: Open D (D A D F# A D)",           6, { 38, 45, 50, 54, 57, 62, 69, 74 } },
            { "Guitar 7: Standard (B E A D G B E)",        7, { 35, 40, 45, 50, 55, 59, 64, 74 } },
            { "Guitar 7: A standard (A D G C F A D)",      7, { 33, 38, 43, 48, 53, 57, 62, 74 } },
            { "Guitar 7: Drop A (A E A D G B E)",          7, { 33, 40, 45, 50, 55, 59, 64, 74 } },
            { "Guitar 8: Standard (F# B E A D G B E)",     8, { 30, 35, 40, 45, 50, 55, 59, 64 } },
            { "Guitar 8: Drop E (E B E A D G B E)",        8, { 28, 35, 40, 45, 50, 55, 59, 64 } },
            { "Bass 4: Standard (E A D G)",                4, { 28, 33, 38, 43, 59, 64, 69, 74 } },
            { "Bass 4: Drop D",                            4, { 26, 33, 38, 43, 59, 64, 69, 74 } },
            { "Bass 5: Standard (B E A D G)",              5, { 23, 28, 33, 38, 43, 64, 69, 74 } },
            { "Custom",                                    6, { 40, 45, 50, 55, 59, 64, 69, 74 } },
        };
        return p;
    }
    static constexpr int kCustomPreset = kNumPresets - 1;

    // The number of input samples one analysis needs, for a sample rate
    static int windowFor (double sampleRate)
    {
        return sampleRate <= 56000.0 ? 16384 : sampleRate <= 112000.0 ? 32768 : 65536;
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        n = windowFor (sampleRate);
        m = 2 * n;
        int order = 0;  while ((1 << order) < m) ++order;
        fft = std::make_unique<juce::dsp::FFT> (order);
        win.resize ((size_t) n);
        for (int i = 0; i < n; ++i)
            win[(size_t) i] = 0.5f - 0.5f * std::cos (6.283185307f * (float) i / (float) n);
        buf.assign ((size_t) m * 2, 0.0f);
        mag.assign ((size_t) m / 2 + 2, 0.0f);
        reset();
    }

    void reset()
    {
        for (int i = 0; i < kMaxStrings; ++i) seenAge[i] = 1000.0;
    }

    int window() const { return n; }

    // x: the latest window() samples, oldest first. midiTargets: the target of every string as a (possibly fractional) MIDI note.
    // refA: the pitch of A4. frameSeconds: time since the last call, for the memory of strings seen before.
    void analyse (const float* x, const float* midiTargets, int count, float refA, float frameSeconds, Result& out)
    {
        out = Result();
        fitLog.clear();
        count = std::min (count, kMaxStrings);
        out.count = count;
        if (fft == nullptr || count <= 0) return;
        for (int i = 0; i < count; ++i) seenAge[i] += frameSeconds;

        // level gate
        double e = 0.0;
        for (int i = 0; i < n; ++i) e += (double) x[i] * x[i];
        const float rms = (float) std::sqrt (e / n);
        if (rms < 0.0015f) return;

        // spectrum
        for (int i = 0; i < n; ++i) buf[(size_t) i] = x[i] * win[(size_t) i];
        std::fill (buf.begin() + n, buf.end(), 0.0f);
        fft->performRealOnlyForwardTransform (buf.data(), true);
        bins = m / 2;
        for (int k = 0; k <= bins; ++k)
        {
            const float re = buf[(size_t) (2 * k)], im = buf[(size_t) (2 * k + 1)];
            mag[(size_t) k] = std::sqrt (re * re + im * im) * (4.0f / (float) n);        // a sinusoid of amplitude a reads about a
        }
        binHz = (float) (sr / m);

        // noise floor: the median magnitude between 40 Hz and 3 kHz
        const int kLo = std::max (12, (int) (40.0f / binHz)), kHi = std::min (bins - 12, (int) (3000.0f / binHz));
        const int kPk = std::max (12, (int) (24.0f / binHz));              // peaks are taken from 24 Hz (a 5-string bass B is 31 Hz)
        if (kHi <= kLo + 8) return;
        scratch.assign (mag.begin() + kLo, mag.begin() + kHi);
        std::nth_element (scratch.begin(), scratch.begin() + (std::ptrdiff_t) scratch.size() / 2, scratch.end());
        const float noise = std::max (scratch[scratch.size() / 2], 1.0e-6f);
        const float thr = std::max (noise * 9.0f, 4.0e-5f);
        noiseAmp = noise;

        // peaks: local maxima above the threshold, refined to a fraction of a bin
        peaks.clear();
        for (int k = kPk; k < kHi; ++k)
        {
            const float a = mag[(size_t) k];
            if (a > thr && a >= mag[(size_t) k - 1] && a > mag[(size_t) k + 1])
            {
                // a window's side lobes are small peaks next to a big one: skip a peak under 6% of the biggest magnitude within 5 raw bins
                float nearMax = 0.0f;
                for (int j = k - 10; j <= k + 10; ++j) nearMax = std::max (nearMax, mag[(size_t) j]);
                if (a < 0.06f * nearMax) continue;
                const float l = std::log (std::max (mag[(size_t) k - 1], 1.0e-9f)), c = std::log (a), r = std::log (std::max (mag[(size_t) k + 1], 1.0e-9f));
                const float den = l - 2.0f * c + r;
                const float d = std::abs (den) > 1.0e-9f ? std::max (-0.5f, std::min (0.5f, 0.5f * (l - r) / den)) : 0.0f;
                Peak p;  p.hz = ((float) k + d) * binHz;  p.amp = std::exp (c - 0.25f * (l - r) * d);  p.claimed = false;
                peaks.push_back (p);
            }
        }

#ifdef POLY_TRACE
        for (const auto& pk : peaks) std::printf ("peak %.2f Hz amp %.5f\n", pk.hz, pk.amp);
#endif
        // the strings, lowest first
        float target[kMaxStrings];  int order[kMaxStrings];
        for (int i = 0; i < count; ++i) { target[i] = refA * std::pow (2.0f, (midiTargets[i] - 69.0f) / 12.0f);  order[i] = i; }
        std::sort (order, order + count, [&] (int a, int b) { return target[a] < target[b]; });
        known.clear();

        for (int oi = 0; oi < count; ++oi)
        {
            const int s = order[oi];
            const float f0 = target[s];
            bool done = false;

            // 1. peaks of its own: the largest group that agree on one offset
            cands.clear();
            bool fundMasked = false;                                                    // its fundamental peak is there but sits in a lower string's lobe
            for (int h = 1; h <= 8; ++h)
            {
                const float fh = f0 * (float) h;
                if (fh > 2800.0f || fh > 0.45f * (float) sr) break;
                const float lo = fh * 0.9439f, hi = fh * 1.0595f;                       // plus and minus 100 cents
                for (int pi = 0; pi < (int) peaks.size(); ++pi)
                {
                    const Peak& p = peaks[(size_t) pi];
                    if (p.hz < lo) continue;
                    if (p.hz > hi) break;
                    if (p.claimed) continue;
                    // a peak within about three and a half bins of one that belongs to a lower string is interference with it (the lumps two or three
                    // close sinusoids make can sit anywhere), not a clean reading
                    bool nearClaimed = false;
                    for (const auto& q : peaks)
                        if (q.claimed && std::abs (q.hz - p.hz) < 3.5f * (float) sr / (float) n) { nearClaimed = true;  break; }
                    if (nearClaimed) { if (h == 1) fundMasked = true;  continue; }
                    const float stiff = 0.5f * 1.0e-4f * (float) (h * h);              // the stiff-string sharpening of the upper harmonics
                    cands.push_back ({ 1200.0f * std::log2 (p.hz / fh) - 1731.2f * stiff, p.amp, h });
                }
            }
#ifdef POLY_TRACE
            std::printf ("string %d f0 %.2f: %d candidates\n", s, f0, (int) cands.size());
            for (const auto& cd_ : cands) std::printf ("    h%d  %+.1f c  amp %.5f\n", cd_.h, cd_.cents, cd_.amp);
#endif
            bool haveH1 = fundMasked;                                                   // some peak at the target pitch (whether or not it agrees with the rest)
            for (const auto& c1 : cands) if (c1.h == 1) haveH1 = true;
            if (! cands.empty())
            {
                float bestW = 0.0f, bestC = 0.0f;
                for (const auto& a : cands)
                {
                    float w = 0.0f, cs = 0.0f;  unsigned hm = 0;
                    for (const auto& b : cands)
                        if (std::abs (b.cents - a.cents) <= 6.0f + 0.4f * (float) b.h)
                        {
                            const float wb = b.amp / (0.6f + 0.4f * (float) b.h);
                            w += wb;  cs += wb * b.cents;  hm |= 1u << b.h;
                        }
                    int nh = 0;  for (unsigned t = hm; t; t &= t - 1) ++nh;
                    // a string's own evidence must include its 1st or 2nd harmonic: a group made only of higher partials is usually a lower string's overtones
                    const bool lowest = (hm & 6u) != 0;
                    // b93: and the fundamental itself. Without a peak at the target pitch, a set of partials only says "something is ringing with
                    // a 55 Hz series" - which is also what the A2 and E2 strings of a guitar still in B standard make (110, 165, 220 and 330 Hz), and
                    // that used to be accepted as A1 "in tune". No fundamental peak now means "not heard" rather than a wrong reading.
                    const bool fund = haveH1;
                    const bool ok = lowest && fund && ((nh >= 2 && w > 3.0f * thr) || ((hm & 2u) && a.amp > thr * 6.0f));        // two harmonics, or a strong fundamental
                    if (ok && w > bestW) { bestW = w;  bestC = cs / w; }
                }
                if (bestW > 0.0f)
                {
                    float w = 0.0f, cs = 0.0f;
                    double sw = 0, sx = 0, sy = 0, sxx = 0, sxy = 0;  int hMin = 99, hMax = 0;
                    for (const auto& b : cands)
                        if (std::abs (b.cents - bestC) <= 6.0f + 0.4f * (float) b.h)
                        {
                            const float wb = b.amp / (0.6f + 0.4f * (float) b.h);
                            w += wb;  cs += wb * b.cents;
                            const double x = (double) b.h * b.h;
                            sw += wb;  sx += wb * x;  sy += wb * b.cents;  sxx += wb * x * x;  sxy += wb * x * b.cents;
                            hMin = std::min (hMin, b.h);  hMax = std::max (hMax, b.h);
                        }
                    float c = cs / w;
                    // b93: a string's own stiffness. The partials sit sharp by 865.6 * B * h^2 cents, and the fixed B used above (1e-4) is wrong for
                    // thick low strings (a 7 string's low A can be 3 to 4 times that), which put a low string's reading 1 to 4 cents off. With three or more
                    // harmonics spread over a range, fit that slope (B limited to 0 .. 6e-4) and read the string at its intercept: the fundamental.
                    if (hMax - hMin >= 3)
                    {
                        const double den = sw * sxx - sx * sx;
                        if (den > 1.0e-12 * sw * sxx)
                        {
                            double slope = (sw * sxy - sx * sy) / den;
                            slope = std::max (-0.0866, std::min (0.4328, slope));
                            const float plain = c;
                            c = (float) ((sy - slope * sx) / sw);
                            c = std::max (plain - 2.5f, std::min (plain + 2.5f, c));                 // never move a reading by more than the stiffness can explain
                        }
                    }
                    setFound (out, s, f0, c, w, false);
                    claim (out.hz[s]);
                    done = true;
                }
            }

            // 2. no peaks of its own: look for it behind the lower strings' overtones with a fit
            if (! done)
            {
                float c = 0.0f, strength = 0.0f;
                if (fitMasked (f0, c, strength))
                {
                    setFound (out, s, f0, c, strength, true);                         // not a clean peak of its own: approximate
                    claim (out.hz[s]);
                    done = true;
                }
            }

            // 3. seen cleanly a moment ago and now hiding inside an overtone of a lower string: report the merged peak's offset
            if (! done && seenAge[s] < 8.0)
            {
                float bestC = 1.0e9f;
                for (int h = 1; h <= 4; ++h)
                {
                    const float fh = f0 * (float) h;
                    for (float kf : known)
                        for (int hk = 1; hk <= 12; ++hk)
                        {
                            const float pk = kf * (float) hk * (1.0f + 0.5f * 1.0e-4f * (float) (hk * hk));
                            if (pk > fh * 0.9885f && pk < fh * 1.0116f)                         // within 20 cents
                            {
                                const float c = 1200.0f * std::log2 (pk / fh);
                                if (std::abs (c) < std::abs (bestC)) bestC = c;
                            }
                        }
                }
                if (std::abs (bestC) <= 20.0f)
                {
                    out.found[s] = true;  out.shared[s] = true;  out.cents[s] = bestC;  out.hz[s] = f0 * std::pow (2.0f, bestC / 1200.0f);  out.level[s] = 0.0f;
                }
            }
            if (out.found[s]) known.push_back (out.hz[s]);
        }
    }

private:
    struct Peak { float hz, amp; bool claimed; };
    struct CandT { float cents, amp; int h; };
    using cd = std::complex<double>;

    void setFound (Result& out, int s, float f0, float c, float level, bool shared)
    {
        out.found[s] = true;  out.shared[s] = shared;  out.cents[s] = c;  out.hz[s] = f0 * std::pow (2.0f, c / 1200.0f);  out.level[s] = level;
        seenAge[s] = 0.0;
    }

    // mark every peak on the harmonics of a found string as used (within 25 cents of a harmonic)
    void claim (float f)
    {
        for (int h = 1; h <= 12; ++h)
        {
            const float fh = f * (float) h * (1.0f + 0.5f * 1.0e-4f * (float) (h * h));
            if (fh > 3000.0f) break;
            for (auto& p : peaks)
                if (p.hz > fh * 0.9857f && p.hz < fh * 1.0145f) p.claimed = true;
        }
    }

    // The spectrum of a Hann-windowed sinusoid at relative frequency f (cycles per sample): the window's own transform.
    cd kernel (double f) const
    {
        const double pi = 3.14159265358979323846;
        auto D = [&] (double g)
        {
            const double sg = std::sin (pi * g);
            if (std::abs (sg) < 1.0e-12) return cd ((double) n, 0.0);
            return std::polar (std::sin (pi * n * g) / sg, -pi * g * (n - 1));
        };
        return 0.5 * D (f) - 0.25 * D (f - 1.0 / n) - 0.25 * D (f + 1.0 / n);
    }

    // Least-squares: the energy of the spectrum around bin kc that a set of sinusoids (relative frequencies) explains.
    double explained (const std::vector<double>& nus, int kc, int W)
    {
        const int d = (int) std::min<size_t> (nus.size(), 6);
        if (d == 0) return 0.0;
        const int nb = 2 * W + 1;
        cols.assign ((size_t) d * (size_t) nb, cd());
        for (int i = 0; i < d; ++i)
            for (int j = 0; j < nb; ++j)
                cols[(size_t) i * nb + (size_t) j] = kernel ((double) (kc - W + j) / m - nus[(size_t) i]);
        cd G[6][6], b[6];
        for (int i = 0; i < d; ++i)
        {
            b[i] = cd();
            for (int j = 0; j < nb; ++j)
            {
                const int k = kc - W + j;
                b[i] += std::conj (cols[(size_t) i * nb + (size_t) j]) * cd ((double) buf[(size_t) (2 * k)], (double) buf[(size_t) (2 * k + 1)]);
            }
            for (int q = 0; q < d; ++q)
            {
                cd sum;
                for (int j = 0; j < nb; ++j) sum += std::conj (cols[(size_t) i * nb + (size_t) j]) * cols[(size_t) q * nb + (size_t) j];
                G[i][q] = sum;
            }
        }
        for (int i = 0; i < d; ++i) G[i][i] += 1.0e-7 * std::abs (G[i][i]);               // a hair of ridge: two sinusoids at one frequency are one
        cd A[6][7];
        for (int i = 0; i < d; ++i) { for (int q = 0; q < d; ++q) A[i][q] = G[i][q];  A[i][d] = b[i]; }
        for (int i = 0; i < d; ++i)
        {
            int piv = i;
            for (int r = i + 1; r < d; ++r) if (std::abs (A[r][i]) > std::abs (A[piv][i])) piv = r;
            if (std::abs (A[piv][i]) < 1.0e-30) return 0.0;
            if (piv != i) for (int q = 0; q <= d; ++q) std::swap (A[i][q], A[piv][q]);
            for (int r = i + 1; r < d; ++r)
            {
                const cd f = A[r][i] / A[i][i];
                for (int q = i; q <= d; ++q) A[r][q] -= f * A[i][q];
            }
        }
        cd c[6];
        for (int i = d - 1; i >= 0; --i)
        {
            cd sum = A[i][d];
            for (int q = i + 1; q < d; ++q) sum -= A[i][q] * c[q];
            c[i] = sum / A[i][i];
        }
        double E = 0.0;
        for (int i = 0; i < d; ++i) E += std::real (std::conj (b[i]) * c[i]);
        return E;
    }

    // Look for a string whose harmonics are covered by the lower strings' harmonics (their positions are in 'known').
    bool fitMasked (float f0, float& centsOut, float& strengthOut)
    {
        constexpr int W = 10;                                            // plus and minus 5 raw bins
        const double fs = sr;
        struct Site { int kc; int h; std::vector<double> contam; double base; };
        std::vector<Site> sites;
        for (int h = 1; h <= 6; ++h)
        {
            const float fh = f0 * (float) h;
            if (fh > 2200.0f || fh > 0.45f * (float) sr) break;
            Site st;  st.kc = (int) std::lround (fh / binHz);  st.h = h;
            if (st.kc - W - 8 < 2 || st.kc + W + 8 >= bins) continue;
            for (float kf : known)                                       // the known sinusoids whose lobes reach this window
                for (int hk = 1; hk <= 14; ++hk)
                {
                    const double pk = (double) kf * hk * (1.0 + 0.5e-4 * hk * hk);
                    if (std::abs (pk / binHz - st.kc) <= W + 6) st.contam.push_back (pk / fs);
                }
            if (st.contam.empty()) continue;                             // nothing hides this harmonic; the first step would have seen it
            if (st.contam.size() > 4) st.contam.resize (4);
            // let each known overtone settle onto its best frequency first, so a small error in the model of the lower strings
            // (stiffness, a little detune) cannot pass for a string of its own
            st.base = explained (st.contam, st.kc, W);
            for (int rep = 0; rep < 2; ++rep)
                for (size_t i = 0; i < st.contam.size(); ++i)
                {
                    const double orig = st.contam[i];
                    double bestV = st.base, bestNu = orig;
                    for (double dc = -8.0; dc <= 8.0; dc += 1.0)
                    {
                        if (dc == 0.0) continue;
                        st.contam[i] = orig * std::pow (2.0, dc / 1200.0);
                        const double v = explained (st.contam, st.kc, W);
                        if (v > bestV) { bestV = v;  bestNu = st.contam[i]; }
                    }
                    st.contam[i] = bestNu;  st.base = bestV;
                }
            sites.push_back (std::move (st));
        }
        if (sites.empty()) return false;

        bool hasLow = false;
        for (auto& st : sites) if (st.h <= 2) hasLow = true;
        if (! hasLow) return false;                                      // its 1st or 2nd harmonic must be the hidden one, otherwise step 1 would have seen it

        double lowPart = 0.0;
        auto score = [&] (double deltaCents)
        {
            const double ratio = std::pow (2.0, deltaCents / 1200.0);
            double sum = 0.0, low = 0.0;
            for (auto& st : sites)
            {
                const double stiff = 0.5e-4 * st.h * st.h;
                std::vector<double> nus = st.contam;
                nus.push_back (f0 * st.h * ratio * (1.0 + stiff) / fs);
                const double g = std::max (0.0, explained (nus, st.kc, W) - st.base);
                sum += g;  if (st.h <= 2) low += g;
            }
            lowPart = low;
            return sum;
        };

        double bestS = 0.0, bestD = 0.0, bestLow = 0.0;
        double coarse[101];  int nc = 0;
        for (double dc = -100.0; dc <= 100.0; dc += 2.0)
        {
            const double sc = score (dc);
            coarse[nc++] = sc;
            if (sc > bestS) { bestS = sc;  bestD = dc;  bestLow = lowPart; }
        }
        // how clear is the answer: the best score somewhere else (further than 12 cents away) against the best one
        double other = 0.0;
        for (int i = 0; i < nc; ++i)
            if (std::abs ((-100.0 + 2.0 * i) - bestD) > 12.0) other = std::max (other, coarse[i]);
        const double ambiguity = bestS > 0.0 ? other / bestS : 1.0;
        for (double dc = bestD - 2.0; dc <= bestD + 2.0; dc += 0.25)
        {
            const double sc = score (dc);
            if (sc > bestS) { bestS = sc;  bestD = dc;  bestLow = lowPart; }
        }
        double baseSum = 0.0, baseLow = 0.0;  for (auto& st : sites) { baseSum += st.base;  if (st.h <= 2) baseLow += st.base; }
        lastLow = bestLow;  lastBaseLow = baseLow;
        // evidence: well above the noise, and a real share of what the lower strings already explain there
        const double noiseE = (double) noiseAmp * noiseAmp * (double) n * (double) n / 16.0;       // |X|^2 of a noise-level bin
        lastScore = bestS;  lastBase = baseSum;  lastNoiseE = noiseE;
        fitLog.push_back ({ f0, bestS / noiseE, bestLow / noiseE, bestS / std::max (baseSum, 1e-30), ambiguity, bestD });
        if (bestLow < 1.0e4 * noiseE || bestS < 0.005 * baseSum || ambiguity > 0.8) return false;
        centsOut = (float) bestD;
        strengthOut = (float) std::sqrt (bestS) * 4.0f / (float) n;
        return true;
    }

    double sr = 48000.0;
    int n = 16384, m = 32768, bins = 16384;
    float binHz = 1.0f, noiseAmp = 1.0e-6f;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> win, buf, mag, scratch;
    std::vector<Peak> peaks;
    std::vector<CandT> cands;
    std::vector<cd> cols;
    std::vector<float> known;                 // pitches of the strings found so far in this analysis
    double seenAge[kMaxStrings] {};
public:
    struct FitRec { float f0; double sNoise, lowNoise, sBase, lowBase, d; };
    std::vector<FitRec> fitLog;                                // for tests
    double lastScore = 0, lastBase = 0, lastNoiseE = 0, lastLow = 0, lastBaseLow = 0;       // for tests
};

// b111: hold-off for the polyphonic tuner's readings. A string read within about 0.4 s of its pluck is 1 to 6 cents sharp on the low strings
// (the pitch settles after the attack); 0.7 s after, it is within 1 cent. The gate watches each string's level: when it jumps to well above its
// recent average (a pluck), readings of that string are not trusted for holdSeconds. One gate per string; the tuner view asks it every frame.
struct PolySettleGate
{
    static constexpr int kN = PolyTuner::kMaxStrings;
    float holdSeconds = 0.6f, jump = 2.5f;
    float avg[kN] {}, left[kN] {};

    void reset() { for (int i = 0; i < kN; ++i) avg[i] = left[i] = 0.0f; }

    // level: the string's peak strength this frame (0 when not heard); dt: seconds since the last frame. Returns true when the reading may be shown.
    bool update (int i, float level, float dt)
    {
        if (level > jump * avg[i] + 1.0e-5f && left[i] <= 0.0f)
            left[i] = holdSeconds;                                          // a pluck (or the first time this string is heard)
        else if (left[i] > 0.0f)
            left[i] = std::max (0.0f, left[i] - dt);
        avg[i] += (level - avg[i]) * std::min (1.0f, dt / 0.5f);            // about 0.5 s memory; after the check, so the pluck itself is not averaged in first
        if (level <= 0.0f) { avg[i] *= 0.5f; }
        return left[i] <= 0.0f;
    }
};
