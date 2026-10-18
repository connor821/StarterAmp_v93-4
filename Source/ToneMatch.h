#pragma once
// Tone matching (v49): measures the average spectrum of a reference recording and of your own playing
// (as it arrives at the Post EQ), then fits the nine Post EQ faders so that your tone's overall balance moves
// toward the reference. It only matches the broad tonal balance; it does not copy distortion, dynamics or room.
#include "PluginProcessor.h"

namespace ToneMatch
{
    constexpr int kBands = 25;                                        // third-octave bands, 50 Hz to 12.7 kHz
    inline float bandHz (int k) { return 50.0f * std::pow (2.0f, (float) k / 3.0f); }

    // Collects the average tonal shape of what it is fed. Each frame counts equally (it is normalised
    // by its own level), so one loud chord does not outweigh a quiet passage. Silence is skipped.
    struct Analyzer
    {
        static constexpr int order = 13, size = 1 << order;           // 8192-sample frames
        juce::dsp::FFT fft { order };
        std::vector<float> window, work;
        std::array<double, kBands> sum {};
        int frames = 0;

        Analyzer() : window ((size_t) size), work ((size_t) size * 2)
        {
            for (int i = 0; i < size; ++i)
                window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);
        }

        void reset() { sum.fill (0.0); frames = 0; }

        // Returns false if the frame was too quiet to count (below about -60 dBFS).
        bool addFrame (const float* x, double sampleRate)
        {
            double energy = 0.0;
            for (int i = 0; i < size; ++i) energy += (double) x[i] * x[i];
            if (std::sqrt (energy / size) < 0.001) return false;

            std::fill (work.begin(), work.end(), 0.0f);
            for (int i = 0; i < size; ++i) work[(size_t) i] = x[i] * window[(size_t) i];
            fft.performFrequencyOnlyForwardTransform (work.data());

            std::array<double, kBands> p {};
            double total = 0.0;
            const double binHz = sampleRate / size;
            const double edge = std::pow (2.0, 1.0 / 6.0);
            for (int k = 0; k < kBands; ++k)
            {
                const int lo = std::max (1, (int) std::ceil (bandHz (k) / edge / binHz));
                const int hi = std::min (size / 2 - 1, (int) std::floor (bandHz (k) * edge / binHz));
                if (hi < lo) { p[(size_t) k] = -1.0; continue; }       // no bin falls in this band
                double a = 0.0;
                for (int b = lo; b <= hi; ++b) a += (double) work[(size_t) b] * work[(size_t) b];
                p[(size_t) k] = a / (hi - lo + 1);
                total += p[(size_t) k];
            }
            if (total <= 0.0) return false;
            for (int k = 0; k < kBands; ++k)
                sum[(size_t) k] += std::max (0.0, p[(size_t) k]) / total;
            ++frames;
            return true;
        }

        // The average shape in dB with its own mean removed (only the balance matters, not the level).
        std::array<float, kBands> shapeDb() const
        {
            std::array<float, kBands> out {};
            if (frames == 0) return out;
            double mean = 0.0;
            for (int k = 0; k < kBands; ++k)
            {
                out[(size_t) k] = (float) (10.0 * std::log10 (sum[(size_t) k] / frames + 1e-12));
                mean += out[(size_t) k];
            }
            mean /= kBands;
            for (auto& v : out) v -= (float) mean;
            return out;
        }
    };

    // How much each Post EQ fader moves each band when it is raised by 1 dB (in the current EQ mode).
    using Response = std::array<std::array<float, kBands>, kEqBands>;
    inline Response responses (double sampleRate, bool modMode, float qKnob)
    {
        Response r {};
        for (int b = 0; b < kEqBands; ++b)
        {
            auto c = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, std::min (kEqBandHz[b], 0.45f * (float) sampleRate),
                                                                          eqBandQ (b, modMode, qKnob), juce::Decibels::decibelsToGain (1.0f));
            for (int k = 0; k < kBands; ++k)
                r[(size_t) b][(size_t) k] = juce::Decibels::gainToDecibels ((float) c->getMagnitudeForFrequency (bandHz (k), sampleRate));
        }
        return r;
    }

    // Least squares: finds the nine fader gains (dB) whose combined response best follows 'wanted' (reference
    // minus yours, per band). A free overall level is fitted too, so only the shape counts. Edges count less,
    // and a small penalty keeps the faders from fighting each other.
    inline std::array<float, kEqBands> fit (const std::array<float, kBands>& wanted, const Response& resp, float strength)
    {
        constexpr int n = kEqBands + 1;
        double m[n][n + 1] = {};
        for (int k = 0; k < kBands; ++k)
        {
            const float f = bandHz (k);
            const double w = (f < 80.0f || f > 9000.0f) ? 0.4 : 1.0;
            double col[n];
            for (int b = 0; b < kEqBands; ++b) col[b] = resp[(size_t) b][(size_t) k];
            col[kEqBands] = 1.0;
            for (int i = 0; i < n; ++i)
            {
                for (int j = 0; j < n; ++j) m[i][j] += w * col[i] * col[j];
                m[i][n] += w * col[i] * wanted[(size_t) k];
            }
        }
        for (int b = 0; b < kEqBands; ++b) m[b][b] += 0.01;

        for (int i = 0; i < n; ++i)                                    // Gaussian elimination with pivoting
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
        std::array<float, kEqBands> g {};
        for (int b = 0; b < kEqBands; ++b)
            g[(size_t) b] = juce::jlimit (-12.0f, 12.0f, strength * (std::abs (m[b][b]) < 1e-12 ? 0.0f : (float) (m[b][n] / m[b][b])));
        return g;
    }

    // The sum of the faders' responses at each band (what the EQ would do), for drawing and for tests.
    inline std::array<float, kBands> eqCurve (const std::array<float, kEqBands>& g, const Response& resp)
    {
        std::array<float, kBands> out {};
        for (int k = 0; k < kBands; ++k)
            for (int b = 0; b < kEqBands; ++b)
                out[(size_t) k] += g[(size_t) b] * resp[(size_t) b][(size_t) k];
        return out;
    }

    // Reads up to 90 seconds of an audio file (any format JUCE reads) into the analyser.
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

    inline juce::String toString (const std::array<float, kBands>& a)
    {
        juce::StringArray s;
        for (float v : a) s.add (juce::String (v, 2));
        return s.joinIntoString (" ");
    }
    inline bool fromString (const juce::String& text, std::array<float, kBands>& a)
    {
        auto t = juce::StringArray::fromTokens (text, " ", "");
        t.removeEmptyStrings();
        if (t.size() != kBands) return false;
        for (int k = 0; k < kBands; ++k)
        {
            a[(size_t) k] = t[k].getFloatValue();
            if (! std::isfinite (a[(size_t) k])) return false;
        }
        return true;
    }
}
