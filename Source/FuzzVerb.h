#pragma once
// Fuzz + reverb (v97): a distortion and a reverb in one fuzz style, with the order switchable (distortion into reverb, or reverb into
// distortion), three reverb programs (Ambient, Octave Down, Reverse with feedback), a Hold (sustain) switch and Mod. Built from the
// published description of a distortion-and-reverb combination pedal (controls and behaviour only); the reverb is my own small
// feedback-delay-network design, the octave-down uses the plugin's own guitar pitch shifter, and nothing is measured.
#include <algorithm>
#include <cmath>
#include <vector>

class FuzzVerb
{
public:
    // shifter is supplied by the owner (the plugin's GuitarShifter) through a callable, so this header does not depend on it
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        static const float ms[8] = { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 61.7f, 71.3f, 79.9f };
        for (int i = 0; i < 8; ++i)
        {
            len[i] = (int) (ms[i] * 0.001f * sr);
            line[i].assign ((size_t) (len[i] + 64), 0.0f);
        }
        static const int ap[2] = { 142, 107 };
        for (int i = 0; i < 2; ++i) { apLen[i] = std::max (8, (int) ((float) ap[i] * sr / 48000.0f)); apBuf[i].assign ((size_t) apLen[i], 0.0f); }
        revBuf.assign (65536, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& l : line) std::fill (l.begin(), l.end(), 0.0f);
        for (auto& a : apBuf) std::fill (a.begin(), a.end(), 0.0f);
        std::fill (revBuf.begin(), revBuf.end(), 0.0f);
        for (int i = 0; i < 8; ++i) { pos[i] = 0; lp[i] = 0.0f; }
        apPos[0] = apPos[1] = 0;  hp = 0.0f;  lfo = 0.0f;  revW = 0;  headPos[0] = 0;  headPos[1] = kSeg / 2;  anchor[0] = anchor[1] = 0;  revFb = 0.0f;
        shiftOut = 0.0f;
    }

    struct Params { float mix = 0.4f, decaySec = 4.0f, mod = 0.3f; int program = 0; bool hold = false; };

    // shift: function (float in) -> float, the octave-down voice of the input (only called for program 1)
    template <typename Shift>
    void process (float* x, int n, const Params& p, Shift&& shift)
    {
        const float decay = p.hold ? 400.0f : std::max(0.3f, p.decaySec);
        const float damp = 0.35f + 0.1f * (float) (p.program == 1);
        const float lpC = 1.0f - std::exp (-6.2831853f * (p.program == 1 ? 2400.0f : 5200.0f) * (1.0f - damp * 0.3f) / sr);
        const float hpC = 1.0f - std::exp (-6.2831853f * 140.0f / sr);
        float g[8];
        for (int i = 0; i < 8; ++i) g[i] = std::pow (10.0f, -3.0f * ((float) len[i] / sr) / decay);
        const float modDepth = p.mod * 6.0f;
        const float lfoInc = 0.45f / sr;
        const float mixAmt = p.mix;
        for (int s = 0; s < n; ++s)
        {
            const float in = x[s];
            hp += hpC * (in - hp);
            float vin = in - hp;
            if (p.program == 1) { shiftOut = shift (vin); vin = 0.30f * vin + 0.95f * shiftOut; }
            if (p.program == 2) vin += 0.55f * revFb;
            // two allpass diffusers
            for (int a = 0; a < 2; ++a)
            {
                float& b = apBuf[a][(size_t) apPos[a]];
                const float d = b;
                const float v = vin - 0.6f * d;
                b = v;
                vin = d + 0.6f * v;
                if (++apPos[a] >= apLen[a]) apPos[a] = 0;
            }
            lfo += lfoInc;  if (lfo >= 1.0f) lfo -= 1.0f;
            float o[8];
            for (int i = 0; i < 8; ++i)
            {
                const float ph = lfo * (0.8f + 0.11f * (float) i) + 0.13f * (float) i;
                const float dly = (float) len[i] - 2.0f - modDepth * (0.5f + 0.5f * std::sin (6.2831853f * (ph - std::floor (ph))));
                float rp = (float) pos[i] - dly;
                const int sz = (int) line[i].size();
                while (rp < 0.0f) rp += (float) sz;
                const int i0 = (int) rp, i1 = (i0 + 1) % sz;
                const float fr = rp - (float) i0;
                o[i] = line[i][(size_t) (i0 % sz)] * (1.0f - fr) + line[i][(size_t) i1] * fr;
            }
            // Hadamard mixing (8 x 8), scaled
            float m[8];
            for (int i = 0; i < 8; ++i) m[i] = o[i];
            for (int step = 1; step < 8; step <<= 1)
                for (int i = 0; i < 8; i += step * 2)
                    for (int j = i; j < i + step; ++j) { const float a = m[j], b = m[j + step]; m[j] = a + b; m[j + step] = a - b; }
            const float sc = 0.35355339f;
            float wet = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                float v = m[i] * sc * g[i] + ((i & 1) ? -vin : vin) * 0.5f;
                lp[i] += lpC * (v - lp[i]);
                line[i][(size_t) pos[i]] = lp[i];
                if (++pos[i] >= (int) line[i].size()) pos[i] = 0;
                wet += ((i & 2) ? -o[i] : o[i]);
            }
            wet *= 0.35f;
            float out = wet;
            if (p.program == 2)
            {
                revBuf[(size_t) revW] = wet;
                revW = (revW + 1) & 65535;
                float r = 0.0f;
                for (int h = 0; h < 2; ++h)
                {
                    if (headPos[h] >= kSeg) { headPos[h] = 0; anchor[h] = revW; }
                    const float ph = (float) headPos[h] / (float) kSeg;
                    const float w = std::sin (3.14159265f * ph), w2 = w * w;
                    r += w2 * revBuf[(size_t) ((anchor[h] - headPos[h]) & 65535)];
                    ++headPos[h];
                }
                revFb = r;
                out = 0.35f * wet + 0.65f * r;
            }
            x[s] = in * (1.0f - 0.45f * mixAmt) + 2.2f * mixAmt * out;
        }
    }

private:
    static constexpr int kSeg = 26000;
    float sr = 48000.0f, hp = 0.0f, lfo = 0.0f, revFb = 0.0f, shiftOut = 0.0f;
    int len[8] {}, pos[8] {}, apLen[2] {}, apPos[2] {}, revW = 0, headPos[2] {}, anchor[2] {};
    float lp[8] {};
    std::vector<float> line[8], apBuf[2], revBuf;
};
