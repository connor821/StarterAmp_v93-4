#pragma once
// Harmonizer (v52; four voices, MIDI mode, melodic minor and the arpeggiator from b115): pitch-shifted voices that follow the note you play.
//   Interval mode : each voice is a fixed number of semitones away
//   Scale mode    : each voice sits a number of scale steps away (a third above, a fifth below...),
//                   so the harmony stays in the chosen key
//   MIDI mode     : the voices sing the MIDI notes you hold (lowest note on voice A, then B, C, D); your note moves to each of them
//   Arp           : one voice steps through the chord (the voices that are on) in time with the tempo
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
    static const HarmScale scales[8] = {
        { 7, { 0, 2, 4, 5, 7, 9, 11 } },     // major
        { 7, { 0, 2, 3, 5, 7, 8, 10 } },     // natural minor
        { 7, { 0, 2, 3, 5, 7, 9, 10 } },     // dorian
        { 7, { 0, 2, 4, 5, 7, 9, 10 } },     // mixolydian
        { 7, { 0, 2, 3, 5, 7, 8, 11 } },     // harmonic minor
        { 5, { 0, 2, 4, 7, 9, 0, 0 } },      // major pentatonic
        { 5, { 0, 3, 5, 7, 10, 0, 0 } },     // minor pentatonic
        { 7, { 0, 2, 3, 5, 7, 9, 11 } } };   // melodic minor (the rising form), b115
    return scales[std::min (std::max (idx, 0), 7)];
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
    static constexpr int kV = 4;
    int   mode = 1;                           // 0 Interval, 1 Scale, 2 MIDI
    bool  poly = false;                       // v53: follow chords (spectral shifter) instead of one note
    int   key = 0, scale = 0;
    float semis[kV] = { 4.0f, 7.0f, 12.0f, -12.0f };       // interval mode
    int   deg[kV]   = { 2, 4, 7, -7 };                      // scale mode (2 = a third, 4 = a fifth, 7 = an octave; negative = below)
    float lvl[kV]   = { 0.6f, 0.5f, 0.0f, 0.0f };
    float pan[kV]   = { -0.5f, 0.5f, -0.2f, 0.2f };
    float dry = 1.0f, tone = 0.7f, glideMs = 25.0f;
    float noteOffset = 0.0f;                  // semitones the tracked note has been moved by earlier effects
    int   midiNotes[kV] = { -1, -1, -1, -1 };  int nMidi = 0;      // b115: the MIDI notes being held, lowest first
    // b115: arpeggiator. One voice steps through the voices that are on; its level is arpLevel and its pan follows the voice it is playing
    bool  arp = false;  int arpPattern = 0;   // 0 Up, 1 Down, 2 Up-down, 3 Random
    float arpBeats = 0.25f, arpGate = 0.6f, arpLevel = 0.7f;
    double bpm = 120.0;

    bool voiceValid (int v) const { return mode != 2 || v < nMidi; }
    bool voiceOn (int v) const { return lvl[v] > 0.0f && voiceValid (v); }
    // semitones to move a played note for voice v
    float shiftFor (int v, float playedMidi) const
    {
        if (mode == 0) return semis[v];
        if (mode == 1) return harmonyShift (playedMidi, key, scale, deg[v]);
        return (float) midiNotes[v] - (float) std::lround (playedMidi);      // MIDI: to the held note; bends and vibrato of your note still come through
    }
};

class HarmonizerEngine
{
public:
    static constexpr int kV = HarmSettings::kV;
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& t : vc) t.prepare (sampleRate);
        tracker.prepare (sampleRate);
        det.prepare (sampleRate);  ss.setVoices (kV);  ss.prepare (sampleRate);  polyOn = false;
        dl.prepare ((int) (0.12 * sampleRate));  dr.prepare ((int) (0.12 * sampleRate));
        reset();
    }
    void reset()
    {
        for (auto& t : vc) t.reset();
        dl.reset(); dr.reset();
        for (int v = 0; v < kV; ++v) { lp[v] = 0.0f; sm[v] = 0.0f; last[v] = 0.0f; wasOn[v] = false; }
        first = true; det.reset(); ss.reset(); heard.store (0);
        arpPos = 0;  arpStep = 0;  arpCur = -1;  arpRnd = 12345u;  gateEnv = 0.0f;
    }
    void setRange (float lowHz, float windowMs) { for (auto& t : vc) { t.setLowHz (lowHz); t.setWindowMs (windowMs); } }
    int  latencySamples() const { return polyOn ? ss.latency() : vc[0].latencySamples(); }
    // v53 chords: 'longWindow' picks the 2x longer analysis window (8-string guitars, or the High quality setting)
    void setPoly (bool on, bool longWindow)
    {
        polyOn = on;
        if (on) ss.setSize (longWindow ? 1 << 20 : 0);
        for (auto& l : lp) l = 0.0f;
        dl.reset();  dr.reset();  det.reset();
    }
    bool isPoly() const { return polyOn; }
    void feedChords (const float* x, int n) { det.feed (x, n); }
    // Notes being heard, as a bit mask (bit i = MIDI note 24 + i); for the display
    uint64_t heardNotes() const { return heard.load(); }
    PitchTracker& getTracker() { return tracker; }
    int arpVoiceNow() const { return arpCur; }                  // for tests and the display

    // L is required; R may be null (mono). Adds the harmony to L (and R) in place.
    void process (float* L, float* R, juce::AudioBuffer<float>& scratch, int n, const HarmSettings& s)
    {
        if (! s.arp) { arpCur = -1;  segment (L, R, scratch, n, s, -1, 0, 1.0e9f);  return; }
        int members[kV], nm = 0;
        for (int v = 0; v < kV; ++v) if (s.voiceOn (v)) members[nm++] = v;
        const double stepD = std::max (64.0, 60.0 / std::max (20.0, s.bpm) * (double) std::max (0.03f, s.arpBeats) * (double) sr);
        const int stepSamples = (int) stepD;
        const float gateSamples = std::max (16.0f, std::min (1.0f, std::max (0.05f, s.arpGate)) * (float) stepSamples);
        int off = 0;
        while (off < n)
        {
            if (arpPos >= stepSamples) { arpPos = 0;  ++arpStep;  arpCur = -2; }          // a new step: choose again
            if (arpCur == -2 || arpCur == -1) arpCur = nm > 0 ? choose (members, nm, s.arpPattern) : -3;
            const int len = std::min (n - off, stepSamples - arpPos);
            const int sel = (arpCur >= 0 && s.voiceOn (arpCur)) ? arpCur : -1;
            segment (L + off, R ? R + off : nullptr, scratch, len, s, sel, arpPos, gateSamples);
            arpPos += len;  off += len;
        }
    }

private:
    int choose (const int* m, int nm, int pattern)
    {
        int idx = 0;
        const int st = arpStep;
        if (pattern == 0) idx = st % nm;
        else if (pattern == 1) idx = nm - 1 - (st % nm);
        else if (pattern == 2) { const int cyc = std::max (1, 2 * nm - 2);  const int k = st % cyc;  idx = k < nm ? k : cyc - k; }
        else { arpRnd = arpRnd * 1664525u + 1013904223u;  idx = (int) ((arpRnd >> 16) % (unsigned) nm); }
        return m[idx];
    }

    static uint64_t maskOf (float midi)
    {
        const int m = (int) std::lround (midi) - 24;
        return (m >= 0 && m < 64) ? (uint64_t (1) << m) : 0;
    }

    // One stretch of audio with the arp step (sel = the voice being played, -1 = rest, or no arp when gateSamples is huge) fixed
    void segment (float* L, float* R, juce::AudioBuffer<float>& scratch, int n, const HarmSettings& s, int sel, int stepPos, float gateSamples)
    {
        const bool arp = s.arp;
        float lvlV[kV], panV[kV];  bool on[kV];
        for (int v = 0; v < kV; ++v)
        {
            on[v] = arp ? (v == 0 && sel >= 0) : s.voiceOn (v);
            lvlV[v] = on[v] ? (arp ? s.arpLevel : s.lvl[v]) : 0.0f;
            panV[v] = arp ? (sel >= 0 ? s.pan[sel] : 0.0f) : s.pan[v];
        }
        if (polyOn) { poly (L, R, scratch, n, s, sel, on, lvlV, panV, stepPos, gateSamples); return; }

        scratch.setSize (kV + 2, n, false, false, true);
        float* w[kV];  for (int v = 0; v < kV; ++v) w[v] = scratch.getWritePointer (v);
        float* dry = scratch.getWritePointer (kV);
        for (int i = 0; i < n; ++i) w[0][i] = R ? 0.5f * (L[i] + R[i]) : L[i];
        for (int v = 1; v < kV; ++v) if (on[v]) std::copy (w[0], w[0] + n, w[v]);

        float tgt[kV];
        for (int v = 0; v < kV; ++v) tgt[v] = last[v];
        if (s.mode == 0)
        {
            for (int v = 0; v < kV; ++v) tgt[v] = s.semis[v];
            if (arp && sel >= 0) tgt[0] = s.semis[sel];
        }
        else if (tracker.hasNote())
        {
            const float played = tracker.noteMidi() + s.noteOffset;
            for (int v = 0; v < kV; ++v) if (s.voiceValid (v)) tgt[v] = s.shiftFor (v, played);
            if (arp && sel >= 0) tgt[0] = s.shiftFor (sel, played);
        }
        for (int v = 0; v < kV; ++v) last[v] = tgt[v];
        if (s.mode != 0 && tracker.hasNote()) heard.store (maskOf (tracker.noteMidi()));
        else if (s.mode == 0) heard.store (0);
        if (first) { for (int v = 0; v < kV; ++v) sm[v] = tgt[v]; first = false; }
        const float glide = arp ? std::min (s.glideMs, 6.0f) : s.glideMs;
        const float g = 1.0f - std::exp (-(float) n / (0.001f * std::max (1.0f, glide) * sr));
        for (int v = 0; v < kV; ++v)
        {
            if (! on[v]) { wasOn[v] = false; continue; }
            if (! wasOn[v]) { vc[v].reset();  sm[v] = tgt[v];  wasOn[v] = true; }
            sm[v] += g * (tgt[v] - sm[v]);
            vc[v].process (w[v], dry, n, sm[v]);
        }
        bool anyOn = false;  for (int v = 0; v < kV; ++v) anyOn = anyOn || on[v];
        mix (L, R, w, dry, on, lvlV, panV, n, s, latencySamples(), stepPos, gateSamples, anyOn);
    }

    // The low-pass, level, pan, arp gate and the dry sound (delayed to match the voices)
    void mix (float* L, float* R, float** w, const float* dry, const bool* on, const float* lvlV, const float* panV, int n, const HarmSettings& s, int lat, int stepPos, float gateSamples, bool monoDry)
    {
        const float fc = std::min (0.45f * sr, 1200.0f * std::pow (10.0f, 1.4f * s.tone));
        const float k = 1.0f - std::exp (-6.2831853f * fc / sr);
        float gL[kV], gR[kV];
        for (int v = 0; v < kV; ++v) { const float p = (panV[v] + 1.0f) * 0.78539816f;  gL[v] = 1.4142136f * std::cos (p);  gR[v] = 1.4142136f * std::sin (p); }
        const float kg = 1.0f - std::exp (-1.0f / (0.003f * sr));
        for (int i = 0; i < n; ++i)
        {
            gateEnv += kg * ((s.arp ? ((float) (stepPos + i) < gateSamples ? 1.0f : 0.0f) : 1.0f) - gateEnv);
            float hl = 0.0f, hr = 0.0f, hm = 0.0f;
            for (int v = 0; v < kV; ++v)
            {
                if (! on[v]) continue;
                lp[v] += k * (w[v][i] - lp[v]);
                const float h = lvlV[v] * lp[v] * (s.arp ? gateEnv : 1.0f);
                hl += h * gL[v];  hr += h * gR[v];  hm += h;
            }
            if (R)
            {
                const float yl = dl.process (L[i], lat), yr = dr.process (R[i], lat);
                L[i] = s.dry * yl + hl;
                R[i] = s.dry * yr + hr;
            }
            else
            {
                const float d = dl.process (L[i], lat);              // always fed, so it is ready when a voice comes on
                L[i] = s.dry * (monoDry ? dry[i] : d) + hm;
            }
        }
    }

    // Chord mode: the spectral shifter moves each heard note by its own amount
    void poly (float* L, float* R, juce::AudioBuffer<float>& scratch, int n, const HarmSettings& s, int sel, const bool* on, const float* lvlV, const float* panV, int stepPos, float gateSamples)
    {
        scratch.setSize (kV + 2, n, false, false, true);
        float* w[kV];  for (int v = 0; v < kV; ++v) w[v] = scratch.getWritePointer (v);
        ss.scaleMode = s.mode == 1;  ss.midiMode = s.mode == 2;  ss.key = s.key;  ss.scale = s.scale;
        for (int v = 0; v < kV; ++v)
        {
            ss.on[v] = on[v];
            const int src = (s.arp && v == 0 && sel >= 0) ? sel : v;            // with the arp, voice 0 plays the chosen voice's part
            ss.deg[v] = s.deg[src];
            ss.alphaInterval[v] = std::pow (2.0f, s.semis[src] / 12.0f);
        }
        ss.nNotes = 0;
        uint64_t mask = 0;
        if (s.mode != 0)
        {
            ChordDetector::Note nt;
            for (int i = 0; ss.nNotes < 10 && det.confirmedAt (i, nt); ++i)
            {
                mask |= maskOf (polyh::midiOfHz (nt.f0));
                const float hz = nt.f0 * std::pow (2.0f, s.noteOffset / 12.0f);        // where earlier effects have moved the note
                const float midi = polyh::midiOfHz (hz);
                auto& m = ss.notes[ss.nNotes++];
                m.f0 = hz;
                for (int v = 0; v < kV; ++v)
                {
                    const int src = (s.arp && v == 0 && sel >= 0) ? sel : v;
                    m.alpha[v] = (on[v] && s.voiceValid (src)) ? std::pow (2.0f, s.shiftFor (src, midi) / 12.0f) : 1.0f;
                }
            }
        }
        heard.store (mask);

        float y[kV];
        for (int i = 0; i < n; ++i)
        {
            ss.process4 (R ? 0.5f * (L[i] + R[i]) : L[i], y);
            for (int v = 0; v < kV; ++v) w[v][i] = y[v];
        }
        mix (L, R, w, nullptr, on, lvlV, panV, n, s, ss.latency(), stepPos, gateSamples, false);
    }

    ChordDetector det;
    SpectralShifter ss;
    bool polyOn = false;
    std::atomic<uint64_t> heard { 0 };
    Transposer vc[kV];
    PitchTracker tracker;
    FixedDelay dl, dr;
    float sr = 44100.0f, lp[kV] = {}, sm[kV] = {}, last[kV] = {}, gateEnv = 0.0f;
    bool first = true, wasOn[kV] = {};
    int arpPos = 0, arpStep = 0, arpCur = -1;
    unsigned arpRnd = 12345u;
};
