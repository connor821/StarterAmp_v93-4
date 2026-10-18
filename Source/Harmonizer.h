#pragma once
// Harmonizer (v52): two pitch-shifted voices that follow the note you play.
//   Interval mode : each voice is a fixed number of semitones away
//   Scale mode    : each voice sits a number of scale steps away (a third above, a fifth below...),
//                   so the harmony stays in the chosen key
// Scale mode needs to know which note is being played, so a small pitch tracker (YIN) listens to the clean input.
// Everything here is my own design; the voices use the same delay-line shifter as the octaver.
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <cstdint>

class PitchTracker
{
public:
    void prepare (double sampleRate)
    {
        decim = std::max (1, (int) std::floor (sampleRate / 12000.0));
        fs = (float) (sampleRate / decim);
        minLag = std::max (4, (int) (fs / 1000.0f));
        maxLag = std::min (kHist - kWin - 2, (int) (fs / 38.0f));
        hop = std::max (32, (int) (fs * 0.010f));
        reset();
    }
    void reset()
    {
        std::fill (hist, hist + kHist, 0.0f);
        pos = 0; acc = 0.0f; accN = 0; sinceHop = 0;
        midi = 0.0f; valid = false; candidate = 0.0f; candCount = 0; silentHops = 0;
    }
    void feed (const float* x, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            acc += x[i];
            if (++accN >= decim)
            {
                hist[pos] = acc / (float) decim;
                pos = (pos + 1) % kHist;
                acc = 0.0f;  accN = 0;
                if (++sinceHop >= hop) { sinceHop = 0; analyse(); }
            }
        }
    }
    bool  hasNote() const { return valid; }
    float noteMidi() const { return midi; }      // fractional MIDI note number of the last confident detection

private:
    static constexpr int kHist = 1024, kWin = 512;
    float hist[kHist] = {};
    float work[kHist] = {}, d[kHist] = {};
    int   decim = 4, minLag = 12, maxLag = 300, hop = 120, pos = 0, accN = 0, sinceHop = 0, candCount = 0, silentHops = 0;
    float fs = 12000.0f, acc = 0.0f, midi = 0.0f, candidate = 0.0f;
    bool  valid = false;

    void analyse()
    {
        for (int i = 0; i < kHist; ++i)
            work[i] = hist[(pos + i) % kHist];                   // oldest first
        const int start = kHist - kWin - maxLag - 1;             // use the newest stretch
        const float* w = work + start;
        float energy = 0.0f;
        for (int j = 0; j < kWin; ++j) energy += w[maxLag + 1 + j] * w[maxLag + 1 + j];
        if (energy / (float) kWin < 1.0e-6f)                      // silence: forget the note after a while
        {
            if (++silentHops > 40) { valid = false; candCount = 0; }
            return;
        }
        silentHops = 0;

        // YIN difference function over the newest window, lags 1..maxLag
        const float* a = work + kHist - kWin;
        d[0] = 1.0f;
        float running = 0.0f;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            const float* b = work + kHist - kWin - tau;
            float sum = 0.0f;
            for (int j = 0; j < kWin; ++j) { const float e = a[j] - b[j]; sum += e * e; }
            running += sum;
            d[tau] = running > 0.0f ? sum * (float) tau / running : 1.0f;
        }
        int best = -1;
        for (int tau = minLag; tau < maxLag; ++tau)
            if (d[tau] < 0.15f)
            {
                while (tau + 1 < maxLag && d[tau + 1] < d[tau]) ++tau;
                best = tau;  break;
            }
        if (best < 0) return;

        float t = (float) best;
        if (best > 1 && best < maxLag)                             // parabolic refinement
        {
            const float s0 = d[best - 1], s1 = d[best], s2 = d[best + 1];
            const float den = s0 - 2.0f * s1 + s2;
            if (std::abs (den) > 1.0e-9f) t += 0.5f * (s0 - s2) / den;
        }
        const float hz = fs / t;
        const float m = 69.0f + 12.0f * std::log2 (hz / 440.0f);
        if (m < 20.0f || m > 100.0f) return;

        // Accept a new note only once two readings in a row agree (rejects stray octave errors)
        if (valid && std::abs (m - midi) < 0.7f) { midi += 0.3f * (m - midi); candCount = 0; return; }
        if (candCount > 0 && std::abs (m - candidate) < 0.7f) ++candCount; else { candidate = m; candCount = 1; }
        if (candCount >= 2) { midi = m; valid = true; candCount = 0; }
    }
};

// A plain delay of a fixed number of samples (keeps the unshifted signal in step with the shifted voices)
class FixedDelay
{
public:
    void prepare (int maxSamples) { buf.assign ((size_t) std::max (8, maxSamples), 0.0f); pos = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
    float process (float x, int d)
    {
        const int n = (int) buf.size();
        d = std::min (std::max (d, 0), n - 1);
        buf[(size_t) pos] = x;
        int r = pos - d;  if (r < 0) r += n;
        const float y = buf[(size_t) r];
        if (++pos >= n) pos = 0;
        return y;
    }
private:
    std::vector<float> buf;
    int pos = 0;
};

struct HarmScale { int n; int steps[7]; };
inline const HarmScale& harmScale (int idx)
{
    static const HarmScale scales[7] = {
        { 7, { 0, 2, 4, 5, 7, 9, 11 } },     // major
        { 7, { 0, 2, 3, 5, 7, 8, 10 } },     // natural minor
        { 7, { 0, 2, 3, 5, 7, 9, 10 } },     // dorian
        { 7, { 0, 2, 4, 5, 7, 9, 10 } },     // mixolydian
        { 7, { 0, 2, 3, 5, 7, 8, 11 } },     // harmonic minor
        { 5, { 0, 2, 4, 7, 9, 0, 0 } },      // major pentatonic
        { 5, { 0, 3, 5, 7, 10, 0, 0 } } };   // minor pentatonic
    return scales[std::min (std::max (idx, 0), 6)];
}

// How far to move a played note so that it lands 'degree' scale steps away (0 = same note).
inline float harmonyShift (float playedMidi, int key, int scaleIdx, int degree)
{
    const HarmScale& sc = harmScale (scaleIdx);
    const int note = (int) std::lround (playedMidi);
    const int rel = note - key;
    const int oct = (int) std::floor ((double) rel / 12.0);
    const int pc = rel - oct * 12;
    int idx = 0, bestDist = 99;                                   // nearest scale tone (a tie goes down)
    for (int i = 0; i < sc.n; ++i)
    {
        const int dist = std::abs (sc.steps[i] - pc);
        if (dist < bestDist) { bestDist = dist; idx = i; }
    }
    const int snapped = oct * 12 + sc.steps[idx];
    const int j = idx + degree;
    const int jo = (int) std::floor ((double) j / sc.n);
    const int ji = j - jo * sc.n;
    const int target = (oct + jo) * 12 + sc.steps[ji];
    return (float) (target - snapped);
}

#include "PolyHarmonizer.h"

struct HarmSettings
{
    bool  scaleMode = false;
    bool  poly = false;                       // v53: follow chords (spectral shifter) instead of one note
    int   key = 0, scale = 0;
    float semisA = 4.0f, semisB = 7.0f;       // interval mode
    int   degA = 2, degB = 4;                 // scale mode (2 = a third, 4 = a fifth, 7 = an octave; negative = below)
    float lvlA = 0.6f, lvlB = 0.5f, panA = -0.5f, panB = 0.5f, dry = 1.0f, tone = 0.7f, glideMs = 25.0f;
    float noteOffset = 0.0f;                  // semitones the tracked note has been moved by earlier effects
};

class HarmonizerEngine
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        v1.prepare (sampleRate);  v2.prepare (sampleRate);  tracker.prepare (sampleRate);
        det.prepare (sampleRate);  ss.prepare (sampleRate);  polyOn = false;
        dl.prepare ((int) (0.12 * sampleRate));  dr.prepare ((int) (0.12 * sampleRate));
        reset();
    }
    void reset() { v1.reset(); v2.reset(); dl.reset(); dr.reset(); lp1 = lp2 = 0.0f; sA = sB = 0.0f; first = true; det.reset(); ss.reset(); heard.store (0); }
    void setRange (float lowHz, float windowMs) { v1.setLowHz (lowHz); v2.setLowHz (lowHz); v1.setWindowMs (windowMs); v2.setWindowMs (windowMs); }
    int  latencySamples() const { return polyOn ? ss.latency() : v1.latencySamples(); }
    // v53 chords: 'longWindow' picks the 2x longer analysis window (8-string guitars, or the High quality setting)
    void setPoly (bool on, bool longWindow)
    {
        polyOn = on;
        if (on) ss.setSize (longWindow ? 1 << 20 : 0);
        lp1 = lp2 = 0.0f;  dl.reset();  dr.reset();  det.reset();
    }
    bool isPoly() const { return polyOn; }
    void feedChords (const float* x, int n) { det.feed (x, n); }
    // Notes being heard, as a bit mask (bit i = MIDI note 24 + i); for the display
    uint64_t heardNotes() const { return heard.load(); }
    PitchTracker& getTracker() { return tracker; }

    // L is required; R may be null (mono). Adds the harmony to L (and R) in place.
    void process (float* L, float* R, juce::AudioBuffer<float>& scratch, int n, const HarmSettings& s)
    {
        if (polyOn) { processPoly (L, R, scratch, n, s); return; }
        scratch.setSize (4, n, false, false, true);
        float* w1 = scratch.getWritePointer (0);
        float* w2 = scratch.getWritePointer (1);
        float* dry = scratch.getWritePointer (2);
        for (int i = 0; i < n; ++i) w1[i] = w2[i] = R ? 0.5f * (L[i] + R[i]) : L[i];

        float tA = s.semisA, tB = s.semisB;
        if (s.scaleMode)
        {
            if (tracker.hasNote())
            {
                const float played = tracker.noteMidi() + s.noteOffset;
                tA = harmonyShift (played, s.key, s.scale, s.degA);
                tB = harmonyShift (played, s.key, s.scale, s.degB);
            }
            else { tA = lastA; tB = lastB; }
            lastA = tA;  lastB = tB;
        }
        if (s.scaleMode && tracker.hasNote()) heard.store (maskOf (tracker.noteMidi()));
        else if (! s.scaleMode) heard.store (0);
        if (first) { sA = tA; sB = tB; first = false; }
        const float g = 1.0f - std::exp (-(float) n / (0.001f * std::max (1.0f, s.glideMs) * sr));
        sA += g * (tA - sA);  sB += g * (tB - sB);

        v1.process (w1, dry, n, sA);
        v2.process (w2, dry, n, sB);

        const float fc = std::min (0.45f * sr, 1200.0f * std::pow (10.0f, 1.4f * s.tone));
        const float k = 1.0f - std::exp (-6.2831853f * fc / sr);
        const float pa = (s.panA + 1.0f) * 0.78539816f, pb = (s.panB + 1.0f) * 0.78539816f;
        const float aL = 1.4142136f * std::cos (pa), aR = 1.4142136f * std::sin (pa);
        const float bL = 1.4142136f * std::cos (pb), bR = 1.4142136f * std::sin (pb);
        const int lat = latencySamples();
        for (int i = 0; i < n; ++i)
        {
            lp1 += k * (w1[i] - lp1);
            lp2 += k * (w2[i] - lp2);
            const float h1 = s.lvlA * lp1, h2 = s.lvlB * lp2;
            if (R)
            {
                const float yl = dl.process (L[i], lat), yr = dr.process (R[i], lat);
                L[i] = s.dry * yl + h1 * aL + h2 * bL;
                R[i] = s.dry * yr + h1 * aR + h2 * bR;
            }
            else
                L[i] = s.dry * dry[i] + h1 + h2;
        }
    }

private:
    static uint64_t maskOf (float midi)
    {
        const int m = (int) std::lround (midi) - 24;
        return (m >= 0 && m < 64) ? (uint64_t (1) << m) : 0;
    }

    // Chord mode: the spectral shifter moves each heard note by its own amount
    void processPoly (float* L, float* R, juce::AudioBuffer<float>& scratch, int n, const HarmSettings& s)
    {
        scratch.setSize (4, n, false, false, true);
        float* w1 = scratch.getWritePointer (0);
        float* w2 = scratch.getWritePointer (1);
        ss.scaleMode = s.scaleMode;  ss.key = s.key;  ss.scale = s.scale;
        ss.deg[0] = s.degA;  ss.deg[1] = s.degB;
        ss.alphaInterval[0] = std::pow (2.0f, s.semisA / 12.0f);
        ss.alphaInterval[1] = std::pow (2.0f, s.semisB / 12.0f);
        ss.nNotes = 0;
        uint64_t mask = 0;
        if (s.scaleMode)
        {
            ChordDetector::Note nt;
            for (int i = 0; ss.nNotes < 10 && det.confirmedAt (i, nt); ++i)
            {
                mask |= maskOf (polyh::midiOfHz (nt.f0));
                const float hz = nt.f0 * std::pow (2.0f, s.noteOffset / 12.0f);        // where earlier effects have moved the note
                const float midi = polyh::midiOfHz (hz);
                auto& m = ss.notes[ss.nNotes++];
                m.f0 = hz;
                m.alpha[0] = std::pow (2.0f, harmonyShift (midi, s.key, s.scale, s.degA) / 12.0f);
                m.alpha[1] = std::pow (2.0f, harmonyShift (midi, s.key, s.scale, s.degB) / 12.0f);
            }
        }
        heard.store (mask);

        for (int i = 0; i < n; ++i)
            ss.process (R ? 0.5f * (L[i] + R[i]) : L[i], w1[i], w2[i]);

        const float fc = std::min (0.45f * sr, 1200.0f * std::pow (10.0f, 1.4f * s.tone));
        const float k = 1.0f - std::exp (-6.2831853f * fc / sr);
        const float pa = (s.panA + 1.0f) * 0.78539816f, pb = (s.panB + 1.0f) * 0.78539816f;
        const float aL = 1.4142136f * std::cos (pa), aR = 1.4142136f * std::sin (pa);
        const float bL = 1.4142136f * std::cos (pb), bR = 1.4142136f * std::sin (pb);
        const int lat = ss.latency();
        for (int i = 0; i < n; ++i)
        {
            lp1 += k * (w1[i] - lp1);
            lp2 += k * (w2[i] - lp2);
            const float h1 = s.lvlA * lp1, h2 = s.lvlB * lp2;
            if (R)
            {
                const float yl = dl.process (L[i], lat), yr = dr.process (R[i], lat);
                L[i] = s.dry * yl + h1 * aL + h2 * bL;
                R[i] = s.dry * yr + h1 * aR + h2 * bR;
            }
            else
                L[i] = s.dry * dl.process (L[i], lat) + h1 + h2;
        }
    }

    ChordDetector det;
    SpectralShifter ss;
    bool polyOn = false;
    std::atomic<uint64_t> heard { 0 };
    Transposer v1, v2;
    PitchTracker tracker;
    FixedDelay dl, dr;
    float sr = 44100.0f, lp1 = 0.0f, lp2 = 0.0f, sA = 0.0f, sB = 0.0f, lastA = 0.0f, lastB = 0.0f;
    bool first = true;
};
