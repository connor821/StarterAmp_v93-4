#pragma once
// b34: DI Match. Reshapes the DI (the guitar as it arrives) so one guitar can sound like another, before the amp.
// It sits at the end of the input stage, after the Transpose. Three parts:
//   1. Spectral match: a gentle 13-band correction (zero latency) that moves the average tone of "your pickups" toward a target.
//      "Your pickups" is either a generic pickup type, or Measured (you play for ten seconds and the plug-in learns your average tone).
//      The target is a generic pickup type, pink noise (a smooth, neutral balance), or a DI recording you load (a reference).
//   2. Level match: nudges the level so the amp sees the loudness of the target (hotter pickups push an amp harder).
//   3. Active circuit: a small preamp like the one in an active pickup, with gain and a little edge, for adding output to a passive guitar.
// The generic pickup types are my own simple resonance models (a coil with a resonant peak), not measurements of any pickup.
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

namespace DiMatch
{
    constexpr int kBands = 25;                                   // third-octave bands, 50 Hz to 12.7 kHz (as in the tone matcher)
    constexpr int kFilters = 13;                                 // low shelf, eleven bells, high shelf
    using Shape = std::array<float, kBands>;
    inline float bandHz (int k) { return 50.0f * std::pow (2.0f, (float) k / 3.0f); }

    // ---- analysis ---------------------------------------------------------------------------------------------------------
    // Average tonal shape (each frame normalised by its own level, silence skipped) plus the average loudness of the frames counted.
    struct Analyzer
    {
        static constexpr int order = 13, size = 1 << order;      // 8192-sample frames
        juce::dsp::FFT fft { order };
        std::vector<float> window, work;
        std::array<double, kBands> sum {};
        double meanSquare = 0.0;
        int frames = 0;

        Analyzer() : window ((size_t) size), work ((size_t) size * 2)
        {
            for (int i = 0; i < size; ++i)
                window[(size_t) i] = 0.5f - 0.5f * std::cos (6.28318531f * (float) i / (float) size);
        }
        void reset() { sum.fill (0.0); meanSquare = 0.0; frames = 0; }

        // false if the frame was too quiet to count (below about -60 dBFS)
        bool addFrame (const float* x, double sampleRate)
        {
            double energy = 0.0;
            for (int i = 0; i < size; ++i) energy += (double) x[i] * x[i];
            const double ms = energy / size;
            if (std::sqrt (ms) < 0.001) return false;
            std::fill (work.begin(), work.end(), 0.0f);
            for (int i = 0; i < size; ++i) work[(size_t) i] = x[i] * window[(size_t) i];
            fft.performFrequencyOnlyForwardTransform (work.data());
            std::array<double, kBands> p {};
            double total = 0.0;
            const double binHz = sampleRate / size, edge = std::pow (2.0, 1.0 / 6.0);
            for (int k = 0; k < kBands; ++k)
            {
                const int lo = std::max (1, (int) std::ceil (bandHz (k) / edge / binHz));
                const int hi = std::min (size / 2 - 1, (int) std::floor (bandHz (k) * edge / binHz));
                if (hi < lo) { p[(size_t) k] = -1.0; continue; }
                double a = 0.0;
                for (int b = lo; b <= hi; ++b) a += (double) work[(size_t) b] * work[(size_t) b];
                p[(size_t) k] = a / (hi - lo + 1);
                total += p[(size_t) k];
            }
            if (total <= 0.0) return false;
            for (int k = 0; k < kBands; ++k) sum[(size_t) k] += std::max (0.0, p[(size_t) k]) / total;
            meanSquare += ms;
            ++frames;
            return true;
        }
        // dB per band with its own mean removed (only the balance matters)
        Shape shapeDb() const
        {
            Shape out {};
            if (frames == 0) return out;
            double mean = 0.0;
            for (int k = 0; k < kBands; ++k) { out[(size_t) k] = (float) (10.0 * std::log10 (sum[(size_t) k] / frames + 1e-12)); mean += out[(size_t) k]; }
            mean /= kBands;
            for (auto& v : out) v -= (float) mean;
            return out;
        }
        float levelDb() const { return frames > 0 ? (float) (10.0 * std::log10 (meanSquare / frames + 1e-12)) : -60.0f; }
    };

    // Up to 90 seconds of an audio file into an analyser (any format JUCE reads).
    inline bool analyseFile (const juce::File& file, Analyzer& an, juce::String& error)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples < Analyzer::size) { error = "That file could not be read (or it is too short)."; return false; }
        const juce::int64 total = std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (90.0 * reader->sampleRate));
        juce::AudioBuffer<float> buf ((int) reader->numChannels, Analyzer::size);
        std::vector<float> mono ((size_t) Analyzer::size);
        an.reset();
        for (juce::int64 pos = 0; pos + Analyzer::size <= total; pos += Analyzer::size / 2)
        {
            reader->read (&buf, 0, Analyzer::size, pos, true, true);
            std::fill (mono.begin(), mono.end(), 0.0f);
            for (int c = 0; c < buf.getNumChannels(); ++c)
                for (int i = 0; i < Analyzer::size; ++i) mono[(size_t) i] += buf.getSample (c, i) / (float) buf.getNumChannels();
            an.addFrame (mono.data(), reader->sampleRate);
        }
        if (an.frames < 4) { error = "That file is too quiet or too short to measure."; return false; }
        return true;
    }

    inline juce::String toString (const Shape& a)
    {
        juce::StringArray s;
        for (float v : a) s.add (juce::String (v, 2));
        return s.joinIntoString (" ");
    }
    inline bool fromString (const juce::String& text, Shape& a)
    {
        auto t = juce::StringArray::fromTokens (text, " ", "");
        t.removeEmptyStrings();
        if (t.size() != kBands) return false;
        for (int k = 0; k < kBands; ++k) { a[(size_t) k] = t[k].getFloatValue(); if (! std::isfinite (a[(size_t) k])) return false; }
        return true;
    }

    // ---- generic pickup types and pink noise ------------------------------------------------------------------------------
    // 0 Single coil, 1 Humbucker, 2 P90, 3 Active. A pickup is a coil and a cable: a resonant low-pass. These are the rough resonance
    // frequency and sharpness of each family, a little body in the lows, and a typical relative output level (dB).
    struct PickupInfo { float f0, q, bodyDb, levelDb; };
    inline PickupInfo pickupInfo (int type)
    {
        static const PickupInfo t[4] = { { 5500.0f, 2.0f, -1.0f, -4.5f }, { 2800.0f, 1.7f, 1.5f, 0.0f },
                                         { 3800.0f, 1.8f, 0.8f, -2.0f }, { 14000.0f, 0.7f, 0.0f, 6.0f } };
        return t[std::max (0, std::min (3, type))];
    }
    inline Shape pickupShape (int type)
    {
        const PickupInfo p = pickupInfo (type);
        Shape s {};
        double mean = 0.0;
        for (int k = 0; k < kBands; ++k)
        {
            const double f = bandHz (k), r = f / p.f0;
            const double mag = 1.0 / std::sqrt ((1.0 - r * r) * (1.0 - r * r) + (r / p.q) * (r / p.q));
            const double body = p.bodyDb / (1.0 + (f / 250.0) * (f / 250.0));
            s[(size_t) k] = (float) (20.0 * std::log10 (mag) + body);
            mean += s[(size_t) k];
        }
        mean /= kBands;
        for (auto& v : s) v -= (float) mean;
        return s;
    }
    // Pink noise has equal energy per octave, which is 3 dB per octave less per bin (the analyser's units): one dB per band step.
    inline Shape pinkShape()
    {
        Shape s {};
        double mean = 0.0;
        for (int k = 0; k < kBands; ++k) { s[(size_t) k] = -1.0f * (float) k; mean += s[(size_t) k]; }
        mean /= kBands;
        for (auto& v : s) v -= (float) mean;
        return s;
    }
    // What a transposed signal's shape looks like: everything moves with the notes, pickup resonance included. 'wet' blends
    // the shifted shape with the unshifted one (power sum), like the Transpose Mix.
    inline Shape warp (const Shape& s, float semis, float wet)
    {
        if (std::abs (semis) < 0.01f || wet <= 0.0f) return s;
        const float step = semis / 12.0f * 3.0f;                 // bands (third octaves) the shape moves up
        Shape o {};
        for (int k = 0; k < kBands; ++k)
        {
            const float src = std::max (0.0f, std::min ((float) (kBands - 1), (float) k - step));
            const int i0 = (int) std::floor (src), i1 = std::min (kBands - 1, i0 + 1);
            const float fr = src - (float) i0;
            const float shifted = s[(size_t) i0] + fr * (s[(size_t) i1] - s[(size_t) i0]);
            const float pw = (1.0f - wet) * std::pow (10.0f, 0.1f * s[(size_t) k]) + wet * std::pow (10.0f, 0.1f * shifted);
            o[(size_t) k] = 10.0f * std::log10 (std::max (pw, 1e-9f));
        }
        return o;
    }

    // ---- the correction filter --------------------------------------------------------------------------------------------
    struct Bq
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void reset() { z1 = z2 = 0.0f; }
        inline float process (float x) { const float y = b0 * x + z1;  z1 = b1 * x - a1 * y + z2;  z2 = b2 * x - a2 * y;  return y; }
        void set (float B0, float B1, float B2, float A0, float A1, float A2) { b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0; }
        // kind 0 low shelf, 1 bell, 2 high shelf
        void design (int kind, float fs, float f, float q, float db)
        {
            const float A = std::pow (10.0f, db / 40.0f), w = 6.28318531f * std::min (f, 0.45f * fs) / fs, c = std::cos (w), s = std::sin (w);
            if (kind == 1)
            {
                const float al = s / (2.0f * q);
                set (1.0f + al * A, -2.0f * c, 1.0f - al * A, 1.0f + al / A, -2.0f * c, 1.0f - al / A);
                return;
            }
            const float al = s * 0.70710678f, t = 2.0f * std::sqrt (A) * al;
            if (kind == 2) set (A * ((A + 1) + (A - 1) * c + t), -2.0f * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
                                (A + 1) - (A - 1) * c + t, 2.0f * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
            else           set (A * ((A + 1) - (A - 1) * c + t), 2.0f * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
                                (A + 1) + (A - 1) * c + t, -2.0f * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
        }
        float magDb (float fs, float f) const
        {
            const float w = 6.28318531f * f / fs;
            const std::complex<float> z1_ = std::polar (1.0f, -w), z2_ = std::polar (1.0f, -2.0f * w);
            const std::complex<float> n = b0 + b1 * z1_ + b2 * z2_, d = 1.0f + a1 * z1_ + a2 * z2_;
            return 20.0f * std::log10 (std::max (1e-9f, std::abs (n) / std::max (1e-9f, std::abs (d))));
        }
    };

    inline constexpr float kFilterHz[kFilters] = { 70.0f, 110.0f, 170.0f, 260.0f, 400.0f, 600.0f, 900.0f, 1350.0f, 2000.0f, 3000.0f, 4500.0f, 6700.0f, 10000.0f };
    inline constexpr int   kFilterKind[kFilters] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2 };

    struct Settings
    {
        bool  on = false;
        int   source = 2, target = 0;          // source: 0 Measured, 1..4 pickup types; target: 0..3 pickup types, 4 Pink noise, 5 Loaded DI
        float amount = 1.0f, level = 1.0f;     // 0..1
        bool  active = false;  float gain = 0.3f;
        float semis = 0.0f, tpWet = 0.0f;      // the Transpose in front (tpWet is 0 when it is off)
        bool operator== (const Settings& o) const
        {
            return on == o.on && source == o.source && target == o.target && amount == o.amount && level == o.level
                && active == o.active && gain == o.gain && semis == o.semis && tpWet == o.tpWet;
        }
    };
    // what update() tells the editor
    enum Status { ok = 0, needLearn = 1, needReference = 2, badCombo = 3 };

    class Engine
    {
    public:
        void prepare (double sampleRate)
        {
            fs = (float) sampleRate;
            // how much a one-dB filter moves each band
            for (int b = 0; b < kFilters; ++b)
            {
                Bq q;  q.design (kFilterKind[b], fs, kFilterHz[b], 1.2f, 1.0f);
                for (int k = 0; k < kBands; ++k) resp[b][k] = q.magDb (fs, std::min (bandHz (k), 0.45f * fs));
            }
            reset();
            gainDb.fill (0.0f);  levelTrimTarget = levelTrim = 1.0f;
            haveLast = false;
        }
        void reset() { for (auto& b : bq) b.reset(); dcX = dcY = 0.0f; }

        // Recomputes the correction. Cheap enough to call every block; it only works when something changed.
        // Returns the status for the editor.
        int update (const Settings& s, const Shape* src, float srcLvlDb, const Shape* ref, float refLvlDb, uint32_t dataVersion)
        {
            if (haveLast && s == last && dataVersion == lastVersion) return status;
            last = s;  lastVersion = dataVersion;  haveLast = true;
            gainDb.fill (0.0f);  levelTrimTarget = 1.0f;  levelFix = 0.0f;
            Shape S {}, T {};
            float lvlDelta = 0.0f;
            status = ok;
            const bool measured = s.source == 0;
            if (s.target >= 4 && ! measured)                          status = badCombo;      // pink noise and a loaded DI need your measured tone
            else if (s.target < 4 && measured)                        status = badCombo;      // a pickup type needs to know your pickups
            else if (measured && src == nullptr)                      status = needLearn;
            else if (s.target == 5 && ref == nullptr)                 status = needReference;
            if (status == ok)
            {
                S = measured ? *src : pickupShape (s.source - 1);
                S = warp (S, s.semis, s.tpWet);
                if (s.target < 4)        { T = pickupShape (s.target);  lvlDelta = pickupInfo (s.target).levelDb - pickupInfo (s.source - 1).levelDb; }
                else if (s.target == 4)  { T = pinkShape();             lvlDelta = 0.0f; }
                else                     { T = *ref;                    lvlDelta = refLvlDb - srcLvlDb; }
                // wanted correction per band; only the shape counts (the mean is a level)
                Shape D {};
                double mean = 0.0;
                for (int k = 0; k < kBands; ++k) { D[(size_t) k] = T[(size_t) k] - S[(size_t) k]; mean += D[(size_t) k]; }
                mean /= kBands;
                for (auto& v : D) v = std::max (-15.0f, std::min (15.0f, (float) (v - mean))) * s.amount;
                fit (D);
                levelTrimTarget = std::pow (10.0f, (std::max (-18.0f, std::min (18.0f, lvlDelta * s.level)) + levelFix) / 20.0f);
            }
            // design the filters
            for (int b = 0; b < kFilters; ++b) bq[b].design (kFilterKind[b], fs, kFilterHz[b], 1.2f, gainDb[(size_t) b]);
            return status;
        }

        // The correction currently asked for, per band (dB), for the editor's drawing and for tests.
        Shape curve() const
        {
            Shape c {};
            for (int k = 0; k < kBands; ++k) for (int b = 0; b < kFilters; ++b) c[(size_t) k] += gainDb[(size_t) b] * resp[b][k];
            return c;
        }
        int lastStatus() const { return status; }

        void process (float* x, int n, bool corrOn, bool active, float gain01)
        {
            if (active)
            {
                const float g = std::pow (10.0f, 12.0f * std::max (0.0f, std::min (1.0f, gain01)) / 20.0f);
                constexpr float c = 1.0f, bias = 0.08f, t0 = 0.07982f;      // t0 = tanh (bias): no DC from the asymmetry at rest
                for (int i = 0; i < n; ++i)
                {
                    const float y = c * (std::tanh (g * x[i] / c + bias) - t0);
                    dcY = y - dcX + 0.995f * dcY;  dcX = y;                  // the preamp's coupling cap: a gentle low cut near 8 Hz
                    x[i] = dcY;
                }
            }
            if (corrOn)
            {
                for (int i = 0; i < n; ++i)
                {
                    float v = x[i];
                    for (auto& b : bq) v = b.process (v);
                    levelTrim += 0.001f * (levelTrimTarget - levelTrim);
                    x[i] = v * levelTrim;
                }
            }
            else
                levelTrim = levelTrimTarget = 1.0f;
        }

    private:
        // Least squares: the filter gains whose combined response follows D (a free level is fitted too, and removed).
        void fit (const Shape& D)
        {
            constexpr int n = kFilters + 1;
            double m[n][n + 1] = {};
            for (int k = 0; k < kBands; ++k)
            {
                const double f = bandHz (k), w = (f < 80.0 || f > 9000.0) ? 0.5 : 1.0;
                double col[n];
                for (int b = 0; b < kFilters; ++b) col[b] = resp[b][k];
                col[kFilters] = 1.0;
                for (int i = 0; i < n; ++i)
                {
                    for (int j = 0; j < n; ++j) m[i][j] += w * col[i] * col[j];
                    m[i][n] += w * col[i] * D[(size_t) k];
                }
            }
            for (int b = 0; b < kFilters; ++b) m[b][b] += 0.05;           // keeps neighbouring filters from fighting
            for (int i = 0; i < n; ++i)
            {
                int piv = i;
                for (int r = i + 1; r < n; ++r) if (std::abs (m[r][i]) > std::abs (m[piv][i])) piv = r;
                for (int c = 0; c <= n; ++c) std::swap (m[i][c], m[piv][c]);
                if (std::abs (m[i][i]) < 1e-12) continue;
                for (int r = 0; r < n; ++r)
                {
                    if (r == i) continue;
                    const double q = m[r][i] / m[i][i];
                    for (int c = i; c <= n; ++c) m[r][c] -= q * m[i][c];
                }
            }
            for (int b = 0; b < kFilters; ++b)
                gainDb[(size_t) b] = std::max (-12.0f, std::min (12.0f, std::abs (m[b][b]) < 1e-12 ? 0.0f : (float) (m[b][n] / m[b][b])));
            // the filters' average level stays where it was: the level is the Level control's job
            double avg = 0.0;
            for (int k = 0; k < kBands; ++k) { double c = 0; for (int b = 0; b < kFilters; ++b) c += gainDb[(size_t) b] * resp[b][k]; avg += c; }
            avg /= kBands;
            levelFix = (float) -avg;
        }
        float fs = 48000.0f, resp[kFilters][kBands] {};
        std::array<float, kFilters> gainDb {};
        Bq bq[kFilters];
        float levelTrim = 1.0f, levelTrimTarget = 1.0f, levelFix = 0.0f, dcX = 0.0f, dcY = 0.0f;   // levelFix: dB that keeps the filters' average level at 0
        Settings last;  uint32_t lastVersion = 0;  bool haveLast = false;
        int status = ok;
    };
}
