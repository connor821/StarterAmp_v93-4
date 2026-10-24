#pragma once
// v0.2: IR reverb support. Two sources for the convolution reverb:
//  - a loaded WAV / AIFF file (any impulse response the player owns), shaped by Decay (a cap on the tail), Stretch and Reverse;
//  - eight generated impulse responses of our own (no sample data of anyone's: made from noise, band-wise decay times and a few
//    other numbers when they are needed). Their character came from measuring several thousand commercial and free reverb
//    impulses (decay time per band, tail shape, stereo width, brightness, how many were reversed or gated), and picking the unusual
//    kinds that the algorithmic reverbs here do not make.
// Everything here runs on the message thread, never the audio thread.
#include <cmath>
#include <vector>

namespace IrReverb
{
    struct Gen
    {
        const char* name;
        float lowRt, midRt, highRt;    // decay time of each band as a multiple of the Decay knob (60 dB)
        float attackMs;                // the onset swells in over about this long (a build rather than a hit)
        bool  reverse;                 // the whole response is played backwards (the tail swells up to the end)
        float gate;                    // > 0: flat plateau of this fraction of Decay, then a quick cut (gated slab)
        float modes;                   // > 0: level of a set of ringing metal-plate style modes mixed in
        float corr;                    // left / right similarity of the noise (1 = mono, 0 = fully independent)
        float hpHz, lpHz;              // fixed filters on the whole response (0 = none)
        float taps;                    // level of sparse early reflections
        float lenMul;                  // overall length as a multiple of the longest band decay
    };

    inline const Gen* gens()
    {
        static const Gen g[8] = {
            // name              lowRt midRt hiRt attack rev   gate  modes corr  hp    lp     taps  len
            { "Dark Infinite",    1.25f, 1.0f, 0.22f, 90.0f, false, 0.0f, 0.0f, 0.05f,   0.0f, 3200.0f, 0.0f, 1.1f },
            { "Reverse Swell",    1.0f,  1.0f, 0.70f, 0.0f,  true,  0.0f, 0.0f, 0.25f,  60.0f, 9000.0f, 0.0f, 1.0f },
            { "Inverted Bloom",   0.45f, 1.0f, 1.55f, 140.0f, false, 0.0f, 0.0f, 0.10f, 120.0f, 0.0f,    0.0f, 1.1f },
            { "Bass Cathedral",   3.0f,  1.0f, 0.55f, 40.0f, false, 0.0f, 0.0f, 0.10f,   0.0f, 6500.0f, 0.6f, 1.0f },
            { "Gated Slab",       1.0f,  1.0f, 0.9f,  0.0f,  false, 0.35f, 0.0f, 0.30f, 100.0f, 0.0f,    0.0f, 1.0f },
            { "Glass Air",        0.30f, 0.85f, 1.0f, 25.0f, false, 0.0f, 0.0f, 0.05f, 350.0f, 0.0f,    0.4f, 1.0f },
            { "Wide Void",        1.0f,  1.0f, 0.60f, 260.0f, false, 0.0f, 0.0f, 0.0f,   80.0f, 7500.0f, 0.0f, 1.1f },
            { "Metal Tank",       0.8f,  1.0f, 0.75f, 5.0f,  false, 0.0f, 0.55f, 0.30f, 200.0f, 8500.0f, 0.0f, 1.0f },
        };
        return g;
    }

    // A fixed random source so a preset sounds the same every time it is built
    struct Rng
    {
        unsigned s;
        explicit Rng (unsigned seed) : s (seed * 2654435761u + 12345u) {}
        float next() { s = s * 1664525u + 1013904223u; return (float) (s >> 8) * (1.0f / 8388608.0f) - 1.0f; }   // -1 .. 1
        float gauss() { return 0.5f * (next() + next() + next()); }
    };

    inline void normalise (juce::AudioBuffer<float>& b, float gain)
    {
        double e = 0.0;
        const int n = b.getNumSamples(), nc = b.getNumChannels();
        for (int c = 0; c < nc; ++c)
        {
            const float* x = b.getReadPointer (c);
            for (int i = 0; i < n; ++i) e += (double) x[i] * x[i];
        }
        e /= (double) std::max (1, nc);
        if (e <= 1.0e-18) return;
        b.applyGain ((float) (gain / std::sqrt (e)));
    }

    // preset 0..7, rtMid = the Decay knob in seconds
    inline void generate (int preset, float rtMid, double sr, juce::AudioBuffer<float>& out)
    {
        const Gen& g = gens()[juce::jlimit (0, 7, preset)];
        const float rtMax = rtMid * std::max (g.lowRt, std::max (1.0f, g.highRt));
        const int len = juce::jlimit (2048, (int) (sr * 30.0), (int) (rtMax * g.lenMul * (float) sr));
        out.setSize (2, len);
        out.clear();
        const float srf = (float) sr;
        const float twoPi = juce::MathConstants<float>::twoPi;
        const float aLow  = 1.0f - std::exp (-twoPi * 250.0f / srf);
        const float aMid  = 1.0f - std::exp (-twoPi * 2000.0f / srf);
        const float aLp   = g.lpHz > 0.0f ? 1.0f - std::exp (-twoPi * g.lpHz / srf) : 1.0f;
        const float aHp   = g.hpHz > 0.0f ? 1.0f - std::exp (-twoPi * g.hpHz / srf) : 0.0f;
        const float rtL = std::max (0.05f, rtMid * g.lowRt), rtM = std::max (0.05f, rtMid), rtH = std::max (0.05f, rtMid * g.highRt);
        const float kL = -6.9078f / (rtL * srf), kM = -6.9078f / (rtM * srf), kH = -6.9078f / (rtH * srf);   // ln(10^-3) per sample
        const float tauA = std::max (0.0f, g.attackMs * 0.001f * srf) / 3.0f;
        const int gateN = g.gate > 0.0f ? (int) (juce::jlimit (0.12f, 1.4f, g.gate * rtMid) * srf) : 0;
        const int gateFall = (int) (0.03f * srf);

        Rng shared (101u + (unsigned) preset * 17u);
        std::vector<float> common ((size_t) len);
        for (auto& v : common) v = shared.gauss();

        for (int ch = 0; ch < 2; ++ch)
        {
            Rng own (7u + (unsigned) ch * 31u + (unsigned) preset * 977u);
            float* y = out.getWritePointer (ch);
            float lo1 = 0, lo2 = 0, mid1 = 0, lp = 0, hp = 0;
            const float cs = std::sqrt (std::max (0.0f, 1.0f - g.corr * g.corr));
            for (int i = 0; i < len; ++i)
            {
                const float n = g.corr * common[(size_t) i] + cs * own.gauss();
                lo1 += aLow * (n - lo1);   lo2 += aLow * (lo1 - lo2);          // below about 250 Hz
                mid1 += aMid * (n - mid1);                                    // below about 2 kHz
                const float low = lo2, mid = mid1 - lo1, high = n - mid1;
                float env[3];
                if (gateN > 0)
                {
                    const float e = i < gateN ? 1.0f : (i < gateN + gateFall ? 1.0f - (float) (i - gateN) / (float) gateFall : 0.0f);
                    env[0] = env[1] = env[2] = e;
                }
                else
                {
                    env[0] = std::exp (kL * (float) i);  env[1] = std::exp (kM * (float) i);  env[2] = std::exp (kH * (float) i);
                }
                float s = 3.0f * low * env[0] + 2.0f * mid * env[1] + high * env[2];
                if (tauA > 0.0f) s *= 1.0f - std::exp (-(float) i / tauA);
                if (g.lpHz > 0.0f) { lp += aLp * (s - lp); s = lp; }
                if (g.hpHz > 0.0f) { hp += aHp * (s - hp); s -= hp; }
                y[i] = s;
            }
            if (g.taps > 0.0f)
            {
                Rng tr (555u + (unsigned) ch * 13u + (unsigned) preset);
                for (int k = 0; k < 18; ++k)
                {
                    const int pos = (int) ((0.004f + 0.075f * (0.5f + 0.5f * tr.next())) * srf);
                    if (pos < len) y[pos] += g.taps * (0.6f + 0.4f * tr.next()) * (tr.next() < 0.0f ? -1.0f : 1.0f) * std::exp (-3.0f * (float) pos / (0.08f * srf));
                }
            }
            if (g.modes > 0.0f)
            {
                Rng mr (31u + (unsigned) ch * 7u);
                for (int k = 0; k < 16; ++k)
                {
                    const float f = 280.0f * std::pow (1.34f, (float) k * 0.62f) * (1.0f + 0.04f * mr.next());
                    if (f > 0.45f * srf) break;
                    const float ph = twoPi * (0.5f + 0.5f * mr.next());
                    const float km = -6.9078f / (std::max (0.05f, rtMid * (0.7f + 0.5f * (0.5f + 0.5f * mr.next()))) * srf);
                    const float amp = g.modes * 0.22f * (1.0f - 0.03f * (float) k);
                    for (int i = 0; i < len; ++i)
                    {
                        const float e = std::exp (km * (float) i);
                        if (e < 1.0e-4f) break;
                        y[i] += amp * e * std::sin (twoPi * f * (float) i / srf + ph);
                    }
                }
            }
        }
        if (g.reverse)
            for (int ch = 0; ch < 2; ++ch)
            {
                float* y = out.getWritePointer (ch);
                std::reverse (y, y + len);
                const int fade = (int) (0.004f * srf);
                for (int i = 0; i < fade && i < len; ++i) { y[len - 1 - i] *= (float) i / (float) fade; }
            }
        normalise (out, 0.5f);
    }

    // Reads a file into a stereo buffer at the file's own rate (up to 40 s). Returns false with a message when it cannot.
    inline bool readFile (const juce::File& file, juce::AudioBuffer<float>& out, double& fileRate, juce::String& message)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
        if (r == nullptr) { message = "That file could not be read as audio."; return false; }
        fileRate = r->sampleRate;
        const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (fileRate * 40.0));
        if (len < 64) { message = "That file is too short to be an impulse response."; return false; }
        const int nch = (int) std::min<unsigned> (r->numChannels, 2u);
        juce::AudioBuffer<float> tmp (nch, len);
        r->read (&tmp, 0, len, 0, true, nch > 1);
        out.setSize (2, len);
        out.copyFrom (0, 0, tmp, 0, 0, len);
        out.copyFrom (1, 0, tmp, nch > 1 ? 1 : 0, 0, len);
        return true;
    }

    // Takes the raw file buffer (or a generated one with fileRate = sr) and applies Stretch, Reverse and the Decay cap
    // (cap > 0: the tail fades out over its last 40 % at this length). The result is at the host rate and has unit energy.
    inline void shape (const juce::AudioBuffer<float>& in, double fileRate, double hostRate, float stretch, bool reverse,
                       float capSeconds, juce::AudioBuffer<float>& out)
    {
        const double step = (fileRate / hostRate) / (double) juce::jlimit (0.25f, 4.0f, stretch);   // input samples per output sample
        const int inLen = in.getNumSamples();
        int outLen = (int) ((double) (inLen - 1) / step);
        if (capSeconds > 0.0f) outLen = std::min (outLen, (int) (capSeconds * hostRate));
        outLen = std::max (64, std::min (outLen, (int) (hostRate * 40.0)));
        out.setSize (2, outLen);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* x = in.getReadPointer (ch);
            float* y = out.getWritePointer (ch);
            for (int i = 0; i < outLen; ++i)
            {
                const double p = (double) i * step;
                const int k = (int) p;
                const float f = (float) (p - (double) k);
                y[i] = k + 1 < inLen ? x[k] + f * (x[k + 1] - x[k]) : x[inLen - 1];
            }
            if (capSeconds > 0.0f)
            {
                const int fadeStart = (int) (0.6 * (double) outLen);
                for (int i = fadeStart; i < outLen; ++i)
                {
                    const float t = (float) (i - fadeStart) / (float) (outLen - fadeStart);
                    y[i] *= 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * t);
                }
            }
            if (reverse) std::reverse (y, y + outLen);
        }
        normalise (out, 0.5f);
    }
}
