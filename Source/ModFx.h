#pragma once
// Extra modulation effects (v51), one multi-type slot (the plugin has two of them):
//   0 Flanger           : a short swept delay with feedback (positive: ringing, negative: hollow)
//   1 Through-zero      : the dry signal is delayed too, so the sweep passes through zero: the deep, jet-like null
//   2 Vibrato           : a swept delay with no dry signal, pure pitch wobble
//   3 Uni-V             : four swept all-pass stages with a lamp-like lag on the sweep (a throbbing phase-chorus-vibrato)
//   4 Rotary            : a two-speed rotating horn and drum with speed inertia (stereo if the signal is stereo)
//   5 Auto-pan          : pans between the sides (a plain tremolo if the signal is mono)
//   6 Harmonic tremolo  : lows and highs pulse in opposite directions
// All hand-built; the names describe the kind of sound, not any particular unit.
#include <algorithm>
#include <cmath>
#include <vector>

class ModFx
{
public:
    struct Params
    {
        int   type = 0, shape = 0;     // shape: 0 sine, 1 triangle, 2 soft square
        float rateHz = 0.5f, depth = 0.5f, feedback = 0.0f, manual = 0.5f, toneHz = 9000.0f, mix = 0.5f;
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        size = 1;
        while (size < (int) (0.03 * sr) + 8) size <<= 1;
        for (auto& c : ch) c.buf.assign ((size_t) size, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& c : ch)
        {
            std::fill (c.buf.begin(), c.buf.end(), 0.0f);
            c.w = 0;  c.fb = 0.0f;  c.tone = 0.0f;
            for (auto& v : c.ap) v = 0.0f;
            for (auto& v : c.xo) v = 0.0f;
            c.hornEnv = c.drumEnv = 0.0f;
        }
        phase = 0.0f;  lamp = 0.0f;
        hornRate = drumRate = 0.0f;
        hornPhase = drumPhase = 0.0f;
    }

    // l (and r, or nullptr for a mono signal), processed in place
    void process (float* l, float* r, int n, const Params& p)
    {
        const bool stereo = r != nullptr;
        for (int i = 0; i < n; ++i)
        {
            float inL = l[i], inR = stereo ? r[i] : l[i];
            phase += p.rateHz / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            float oL = inL, oR = inR;
            switch (p.type)
            {
                case 0: case 1: flangeStep (p, inL, inR, oL, oR, p.type == 1); break;
                case 2:         vibratoStep (p, inL, inR, oL, oR); break;
                case 3:         uniVStep (p, inL, inR, oL, oR); break;
                case 4:         rotaryStep (p, inL, inR, oL, oR, stereo); break;
                case 5:         panStep (p, inL, inR, oL, oR, stereo); break;
                default:        harmonicStep (p, inL, inR, oL, oR); break;
            }
            l[i] = oL;
            if (stereo) r[i] = oR;
        }
    }

    int latencySamples (const Params& p) const { return p.type == 1 ? (int) (0.004f * sr) : 0; }

private:
    struct Chan
    {
        std::vector<float> buf;
        int   w = 0;
        float fb = 0.0f, tone = 0.0f, ap[4] {}, xo[8] {}, hornEnv = 0.0f, drumEnv = 0.0f;
    };

    static float lfo (float ph, int shape)
    {
        ph -= std::floor (ph);
        const float s = std::sin (6.2831853f * ph);
        if (shape == 1) return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
        if (shape == 2) return std::tanh (4.0f * s) * 1.0007f;
        return s;
    }

    float readDelay (const Chan& c, float d) const        // cubic interpolation, d in samples
    {
        d = std::max (1.5f, std::min (d, (float) size - 4.0f));
        float rp = (float) c.w - d;
        while (rp < 0.0f) rp += (float) size;
        const int i1 = (int) rp;
        const float t = rp - (float) i1;
        const int m = size - 1;
        const float y0 = c.buf[(size_t) ((i1 - 1) & m)], y1 = c.buf[(size_t) (i1 & m)], y2 = c.buf[(size_t) ((i1 + 1) & m)], y3 = c.buf[(size_t) ((i1 + 2) & m)];
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    static float toneCoef (float hz, float sr) { return 1.0f - std::exp (-6.2831853f * std::min (hz, 0.45f * sr) / sr); }

    // ---- flanger and through-zero flanger
    void flangeStep (const Params& p, float inL, float inR, float& oL, float& oR, bool tz)
    {
        const float tc = toneCoef (p.toneHz, sr);
        const float fbk = std::max (-0.92f, std::min (0.92f, p.feedback));
        const float lfoV[2] = { lfo (phase, p.shape), lfo (phase + 0.25f, p.shape) };      // the right side is a quarter cycle later
        const float inV[2] = { inL, inR };
        float out[2];
        for (int k = 0; k < 2; ++k)
        {
            Chan& c = ch[k];
            float d;
            if (! tz)
            {
                const float centre = 0.0003f * std::pow (26.0f, p.manual) * sr;       // 0.3 ms to 7.8 ms
                d = centre * (1.0f + 0.9f * p.depth * lfoV[k]);
            }
            else
            {
                const float D = 0.004f * sr;                                           // the dry signal's delay
                const float pos = p.manual + p.depth * std::min (p.manual, 1.0f - p.manual) * lfoV[k];
                d = 2.0f * D * std::max (0.0f, std::min (1.0f, pos));
            }
            c.buf[(size_t) c.w] = inV[k] + c.fb * fbk;
            float wet = readDelay (c, d);
            c.tone += tc * (wet - c.tone);
            wet = c.tone;
            c.fb = std::tanh (wet);
            if (tz)
            {
                const float dry = readDelay (c, 0.004f * sr);
                out[k] = dry * (1.0f - 0.5f * p.mix) - wet * (0.5f * p.mix);          // subtracting: silence at the crossing, like the real thing
            }
            else
                out[k] = (inV[k] + p.mix * wet) / (1.0f + 0.5f * p.mix);
            c.w = (c.w + 1) & (size - 1);
        }
        oL = out[0];  oR = out[1];
    }

    // ---- vibrato
    void vibratoStep (const Params& p, float inL, float inR, float& oL, float& oR)
    {
        const float tc = toneCoef (p.toneHz, sr);
        const float inV[2] = { inL, inR };
        float out[2];
        for (int k = 0; k < 2; ++k)
        {
            Chan& c = ch[k];
            const float v = lfo (phase + 0.125f * (float) k, p.shape);
            const float d = 0.0045f * sr * (1.0f + 0.85f * p.depth * v);
            c.buf[(size_t) c.w] = inV[k];
            float wet = readDelay (c, d);
            c.tone += tc * (wet - c.tone);
            out[k] = inV[k] + p.mix * (c.tone - inV[k]);
            c.w = (c.w + 1) & (size - 1);
        }
        oL = out[0];  oR = out[1];
    }

    // ---- Uni-V: four all-pass stages swept by a lamp-like lagged LFO
    void uniVStep (const Params& p, float inL, float inR, float& oL, float& oR)
    {
        const float raw = 0.5f + 0.5f * lfo (phase, p.shape);
        const float target = raw * raw * (3.0f - 2.0f * raw);                          // brighter in the middle of the swing
        lamp += (target > lamp ? 1.0f - std::exp (-1.0f / (0.030f * sr)) : 1.0f - std::exp (-1.0f / (0.060f * sr))) * (target - lamp);
        static const float base[4] = { 350.0f, 900.0f, 2200.0f, 5200.0f };
        const float sweep = std::pow (2.0f, p.depth * 3.2f * (lamp - 0.5f) * 2.0f);
        const float tc = toneCoef (p.toneHz, sr);
        const float fbk = std::max (-0.85f, std::min (0.85f, p.feedback));
        const float inV[2] = { inL, inR };
        float out[2];
        for (int k = 0; k < 2; ++k)
        {
            Chan& c = ch[k];
            float x = inV[k] + c.fb * fbk;
            for (int s = 0; s < 4; ++s)
            {
                const float f = std::min (0.45f * sr, base[s] * sweep * (1.0f + 0.12f * (float) k));
                const float t = std::tan (3.14159265f * f / sr);
                const float a = (t - 1.0f) / (t + 1.0f);
                const float y = -a * x + c.xo[s] ;
                c.xo[s] = x + a * y;                                                  // first-order all-pass
                x = y;
            }
            c.tone += tc * (x - c.tone);
            c.fb = std::tanh (c.tone);
            out[k] = inV[k] + p.mix * (c.tone - inV[k]);        // half way = the phase-chorus notches, all the way = the vibrato
        }
        oL = out[0];  oR = out[1];
    }

    // ---- rotary speaker: a crossover, then a horn (highs) and a drum (lows) with their own speeds and inertia
    void rotaryStep (const Params& p, float inL, float inR, float& oL, float& oR, bool stereo)
    {
        const float in = stereo ? 0.5f * (inL + inR) : inL;
        // speed inertia: the horn follows quickly (about 0.8 s), the drum slowly (about 2.5 s)
        hornRate += (p.rateHz - hornRate) * (1.0f - std::exp (-1.0f / (0.8f * sr)));
        drumRate += (0.85f * p.rateHz - drumRate) * (1.0f - std::exp (-1.0f / (2.5f * sr)));
        hornPhase += hornRate / sr;  if (hornPhase >= 1.0f) hornPhase -= 1.0f;
        drumPhase += drumRate / sr;  if (drumPhase >= 1.0f) drumPhase -= 1.0f;
        // 800 Hz Linkwitz-Riley-style split (two one-pole lows)
        const float k = 1.0f - std::exp (-6.2831853f * 800.0f / sr);
        Chan& c = ch[0];
        c.xo[0] += k * (in - c.xo[0]);
        c.xo[1] += k * (c.xo[0] - c.xo[1]);
        const float lowB = c.xo[1];
        const float highB = in - lowB;
        // the horn: amplitude + doppler (a small moving delay) + a pan between two microphones
        const float hs = std::sin (6.2831853f * hornPhase), hc = std::cos (6.2831853f * hornPhase);
        const float ds = std::sin (6.2831853f * drumPhase), dc = std::cos (6.2831853f * drumPhase);
        c.buf[(size_t) c.w] = highB;
        const float dopp = 0.00035f * sr * (1.0f + 0.9f * p.depth * hs);
        const float horn = readDelay (c, dopp);
        c.w = (c.w + 1) & (size - 1);
        const float balHorn = 2.0f * p.manual, balDrum = 2.0f * (1.0f - p.manual);
        const float hAmp = 1.0f - 0.22f * p.depth * (1.0f - hc);
        const float dAmp = 1.0f - 0.35f * p.depth * (1.0f - dc) * 0.5f;
        const float hornL = horn * hAmp * (1.0f + 0.6f * p.depth * hs), hornR = horn * hAmp * (1.0f - 0.6f * p.depth * hs);
        const float drumL = lowB * dAmp * (1.0f + 0.3f * p.depth * ds), drumR = lowB * dAmp * (1.0f - 0.3f * p.depth * ds);
        const float tc = toneCoef (p.toneHz, sr);
        float wl = balHorn * hornL * 0.7f + balDrum * drumL * 0.7f;
        float wr = balHorn * hornR * 0.7f + balDrum * drumR * 0.7f;
        c.tone += tc * (wl - c.tone);   wl = c.tone;
        ch[1].tone += tc * (wr - ch[1].tone);   wr = ch[1].tone;
        if (! stereo) wl = 0.5f * (wl + wr);
        oL = inL + p.mix * (wl - inL);
        oR = stereo ? inR + p.mix * (wr - inR) : oL;
    }

    // ---- auto-pan
    void panStep (const Params& p, float inL, float inR, float& oL, float& oR, bool stereo)
    {
        const float v = lfo (phase, p.shape);
        if (stereo)
        {
            const float th = 0.78539816f * (1.0f + p.depth * v);
            const float gl = std::cos (th) * 1.4142f, gr = std::sin (th) * 1.4142f;
            const float m = 0.5f * (inL + inR);
            oL = inL + p.mix * (m * gl - inL);
            oR = inR + p.mix * (m * gr - inR);
        }
        else
        {
            const float g = 1.0f - p.depth * 0.5f * (1.0f - v);
            oL = inL + p.mix * (inL * g - inL);
            oR = oL;
        }
    }

    // ---- harmonic tremolo: split at 700 Hz, the two bands pulse in opposite directions
    void harmonicStep (const Params& p, float inL, float inR, float& oL, float& oR)
    {
        const float v = lfo (phase, p.shape);
        const float k = 1.0f - std::exp (-6.2831853f * 700.0f / sr);
        const float inV[2] = { inL, inR };
        float out[2];
        for (int c = 0; c < 2; ++c)
        {
            Chan& s = ch[c];
            s.xo[0] += k * (inV[c] - s.xo[0]);
            s.xo[1] += k * (s.xo[0] - s.xo[1]);
            const float lo = s.xo[1], hi = inV[c] - lo;
            const float gLo = 1.0f - p.depth * 0.5f * (1.0f + v), gHi = 1.0f - p.depth * 0.5f * (1.0f - v);
            out[c] = inV[c] + p.mix * (lo * gLo + hi * gHi - inV[c]);
        }
        oL = out[0];  oR = out[1];
    }

    float sr = 48000.0f, phase = 0.0f, lamp = 0.0f, hornRate = 0.0f, drumRate = 0.0f, hornPhase = 0.0f, drumPhase = 0.0f;
    int size = 1;
    Chan ch[2];
};
