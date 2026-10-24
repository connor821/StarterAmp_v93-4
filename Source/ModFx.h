#pragma once
// Extra modulation effects (v51), one multi-type slot (the plugin has two of them):
//   0 Flanger           : a short swept delay with feedback (positive: ringing, negative: hollow)
//   1 Through-zero      : the dry signal is delayed too, so the sweep passes through zero: the deep, jet-like null
//   2 Vibrato           : a swept delay with no dry signal, pure pitch wobble
//   3 Uni-V             : four swept all-pass stages with a lamp-like lag on the sweep (a throbbing phase-chorus-vibrato)
//   4 Rotary            : a two-speed rotating horn and drum with speed inertia (stereo if the signal is stereo)
//   5 Auto-pan          : pans between the sides (a plain tremolo if the signal is mono)
//   6 Harmonic tremolo  : lows and highs pulse in opposite directions
//   7 Wobble            : handled in the processor (a loosely held tremolo-arm vibrato)
//   8 Tremolo           : plain amplitude tremolo (shape 7 = optical, a lagging lamp)
//   9 Shifted flanger   : a flanger whose feedback is frequency-shifted a few Hz each trip, so the sweep spirals up or down
//  11 Stutter           : each cycle (Rate or tempo Note) the last part of it loops the slice just played (Depth = how much of the cycle, Manual = repeats)
//  10 Pitch modulation  : a pitch-shifted voice (Manual = interval, up to an octave either way) whose pitch an LFO swings (Depth); Feedback spirals it
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
        float shiftHz = 7.0f;          // type 9: how far the feedback is shifted on each trip round the loop (Hz, + up, - down)
        float fidelity = 1.0f;         // v97: 1 = clean; lower rolls off lows and highs and adds a little saturation (worn tape / cassette)
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        size = 1;
        while (size < (int) (0.03 * sr) + 8) size <<= 1;
        for (auto& c : ch) c.buf.assign ((size_t) size, 0.0f);
        stSize = (int) sr;
        for (auto& c : ch) c.st.assign ((size_t) stSize, 0.0f);
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
            c.hb = Chan::Hilbert();
            c.hornEnv = c.drumEnv = 0.0f;
            c.pph = 0.0f;
            std::fill (c.st.begin(), c.st.end(), 0.0f);
            c.stW = 0;  c.stRead = 0;  c.stStart = 0;  c.stLen = 64;  c.stGate = 0.0f;
        }
        stWasIn = false;
        phase = 0.0f;  lamp = 0.0f;  fLp[0] = fLp[1] = fHp[0] = fHp[1] = 0.0f;
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
                case 8:         tremStep (p, inL, inR, oL, oR); break;
                case 9:         flangeStep (p, inL, inR, oL, oR, false, true); break;
                case 10:        pitchStep (p, inL, inR, oL, oR); break;
                case 11:        stutterStep (p, inL, inR, oL, oR); break;
                default:        harmonicStep (p, inL, inR, oL, oR); break;
            }
            if (p.fidelity < 0.999f)
            {
                const float f = p.fidelity;
                const float lpC = toneCoef (900.0f + 9000.0f * f * f, sr), hpC = toneCoef (35.0f + 260.0f * (1.0f - f), sr);
                const float drive = 1.0f + 2.5f * (1.0f - f);
                for (int k = 0; k < (stereo ? 2 : 1); ++k)
                {
                    float x = k == 0 ? oL : oR;
                    fLp[k] += lpC * (x - fLp[k]);
                    fHp[k] += hpC * (fLp[k] - fHp[k]);
                    x = std::tanh (drive * (fLp[k] - fHp[k])) / drive;
                    (k == 0 ? oL : oR) = x;
                }
            }
            l[i] = oL;
            if (stereo) r[i] = oR;
        }
    }

    int latencySamples (const Params& p) const { return p.type == 1 ? (int) (0.004f * sr) : 0; }

private:
    float fLp[2] {}, fHp[2] {};
    struct Chan
    {
        std::vector<float> buf, st;
        int   w = 0, stW = 0, stRead = 0, stStart = 0, stLen = 64;
        float stGate = 0.0f;
        float pph = 0.0f, fb = 0.0f, tone = 0.0f, ap[4] {}, xo[8] {}, hornEnv = 0.0f, drumEnv = 0.0f;
        struct Hilbert { float x1[8] {}, x2[8] {}, y1[8] {}, y2[8] {}, d = 0.0f; } hb;
    };

    // Single-sideband frequency shifter (two chains of four all-passes 90 degrees apart, then a quadrature oscillator)
    float shiftSample (Chan& c, float x, float shiftHz)
    {
        static const float a[8] = { 0.6923878f, 0.9360654f, 0.9882295f, 0.9987488f, 0.4021921f, 0.8561711f, 0.9722910f, 0.9952885f };
        float v[2] = { x, c.hb.d };
        c.hb.d = x;
        float outv[2];
        for (int path = 0; path < 2; ++path)
        {
            float t = v[path];
            for (int k = 0; k < 4; ++k)
            {
                const int j = path * 4 + k;
                const float a2 = a[j] * a[j];
                const float y = a2 * (t + c.hb.y2[j]) - c.hb.x2[j];
                c.hb.x2[j] = c.hb.x1[j];  c.hb.x1[j] = t;
                c.hb.y2[j] = c.hb.y1[j];  c.hb.y1[j] = y;
                t = y;
            }
            outv[path] = t;
        }
        const float ang = 6.2831853f * shiftPhase;
        return outv[0] * std::cos (ang) - outv[1] * std::sin (ang);
    }

    static float lfo (float ph, int shape)
    {
        ph -= std::floor (ph);
        const float s = std::sin (6.2831853f * ph);
        if (shape == 1) return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
        if (shape == 2) return std::tanh (4.0f * s) * 1.0007f;
        // v97: ramp up, ramp down, square (short soft edges), and a slow wandering random made from three unrelated sines
        if (shape == 3) return ph < 0.97f ? -1.0f + 2.0f * ph / 0.97f : 1.0f - 2.0f * (ph - 0.97f) / 0.03f;
        if (shape == 4) return ph < 0.03f ? 1.0f - 2.0f * ph / 0.03f : -1.0f + 2.0f * (ph - 0.03f) / 0.97f;
        if (shape == 5) return std::tanh (12.0f * s) * 1.0000001f;
        if (shape >= 6) return 0.55f * s + 0.3f * std::sin (6.2831853f * (ph * 2.618f + 0.3f)) + 0.2f * std::sin (6.2831853f * (ph * 4.236f + 0.7f));
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
    void flangeStep (const Params& p, float inL, float inR, float& oL, float& oR, bool tz, bool shifted = false)
    {
        if (shifted) { shiftPhase += p.shiftHz / sr; shiftPhase -= std::floor (shiftPhase); }
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
            c.buf[(size_t) c.w] = inV[k] + (shifted ? shiftSample (c, c.fb, p.shiftHz) : c.fb) * fbk;
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

    // ---- pitch modulation: a two-tap crossfading pitch shifter. Manual picks the interval (whole semitones, centre = unison),
    //      the LFO swings the pitch around it by up to +-12 semitones (Depth, squared so small settings are a gentle detune),
    //      Feedback sends the shifted voice back in so repeats climb or fall.
    void pitchStep (const Params& p, float inL, float inR, float& oL, float& oR)
    {
        const float tc = toneCoef (p.toneHz, sr);
        const float fbk = std::max (-0.85f, std::min (0.85f, p.feedback));
        const float centre = std::round ((p.manual - 0.5f) * 24.0f);
        const float swing = p.depth * p.depth * 12.0f;
        const float win = std::min (0.0295f * sr, (float) size - 8.0f);
        const float inV[2] = { inL, inR };
        float out[2];
        for (int k = 0; k < 2; ++k)
        {
            Chan& c = ch[k];
            const float semis = centre + swing * lfo (phase + 0.25f * (float) k, p.shape);
            const float ratio = std::pow (2.0f, semis / 12.0f);
            c.buf[(size_t) c.w] = inV[k] + c.fb * fbk;
            float wet = 0.0f;
            for (int tap = 0; tap < 2; ++tap)
            {
                float q = c.pph + 0.5f * (float) tap;
                if (q >= 1.0f) q -= 1.0f;
                wet += (0.5f - 0.5f * std::cos (6.2831853f * q)) * readDelay (c, win * (1.0f - q) + 2.0f);
            }
            c.pph += (ratio - 1.0f) / win;
            c.pph -= std::floor (c.pph);
            c.tone += tc * (wet - c.tone);
            c.fb = std::tanh (c.tone);
            out[k] = inV[k] + p.mix * (c.tone - inV[k]);
            c.w = (c.w + 1) & (size - 1);
        }
        oL = out[0];  oR = out[1];
    }

    // ---- stutter: the cycle (Rate, or a tempo Note) is split into a live part and a stuttered tail (Depth = the tail's share).
    //      At the start of the tail the last stretch of sound is captured and looped (Manual = how many repeats fit in the tail,
    //      1 to 8), with short fades at the edges. Mix blends the loop with the live signal.
    void stutterStep (const Params& p, float inL, float inR, float& oL, float& oR)
    {
        const float cycleStart = 1.0f - std::max (0.05f, std::min (1.0f, p.depth));
        const bool  inTail = phase >= cycleStart;
        const bool  enter = inTail && ! stWasIn;
        stWasIn = inTail;
        const int   repeats = 1 + (int) std::round (std::max (0.0f, std::min (1.0f, p.manual)) * 7.0f);
        const float cycleSamples = sr / std::max (0.02f, p.rateHz);
        const int   len = std::max (64, std::min (stSize - 8, (int) (std::max (0.05f, p.depth) * cycleSamples / (float) repeats)));
        const float gCoef = 1.0f - std::exp (-1.0f / (0.002f * sr));
        const float inV[2] = { inL, inR };
        float out[2];
        for (int k = 0; k < 2; ++k)
        {
            Chan& c = ch[k];
            if (enter)
            {
                c.stLen = len;
                c.stStart = (c.stW - len + stSize) % stSize;
                c.stRead = 0;
            }
            if (! inTail) { c.st[(size_t) c.stW] = inV[k]; c.stW = (c.stW + 1) % stSize; }       // recording stops while the tail loops
            c.stGate += gCoef * ((inTail ? 1.0f : 0.0f) - c.stGate);
            float loop = 0.0f;
            if (c.stGate > 1.0e-3f)
            {
                const float edge = 0.0015f * sr;
                const float fade = std::min (1.0f, std::min ((float) c.stRead / edge, (float) (c.stLen - c.stRead) / edge));
                loop = c.st[(size_t) ((c.stStart + c.stRead) % stSize)] * fade;
                if (++c.stRead >= c.stLen) c.stRead = 0;
            }
            const float wet = inV[k] * (1.0f - c.stGate) + loop * c.stGate;
            out[k] = inV[k] + p.mix * (wet - inV[k]);
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

    // ---- tremolo (shape 7 = optical: the lamp lags the control, so the dips round off and the bright part lingers)
    void tremStep (const Params& p, float inL, float inR, float& oL, float& oR)
    {
        float v;
        if (p.shape >= 7)
        {
            const float raw = 0.5f + 0.5f * std::sin (6.2831853f * phase);
            lamp += (raw > lamp ? 1.0f - std::exp (-1.0f / (0.012f * sr)) : 1.0f - std::exp (-1.0f / (0.045f * sr))) * (raw - lamp);
            v = 2.0f * lamp * lamp - 1.0f;
        }
        else
            v = lfo (phase, p.shape);
        const float g = 1.0f - p.depth * 0.5f * (1.0f - v);
        oL = inL + p.mix * (inL * g - inL);
        oR = inR + p.mix * (inR * g - inR);
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

    float sr = 48000.0f, shiftPhase = 0.0f, phase = 0.0f, lamp = 0.0f, hornRate = 0.0f, drumRate = 0.0f, hornPhase = 0.0f, drumPhase = 0.0f;
    int size = 1, stSize = 1;
    bool stWasIn = false;
    Chan ch[2];
};
