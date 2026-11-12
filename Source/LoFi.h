#pragma once
// Lo-Fi Warble (v50): a degraded, wobbly analog-style pedal. A slow pitch warble (vibrato) with random drift and a
// little fast flutter, then worn-out tone: soft clipping and bit/sample-rate crushing ("Dirt"), a darker top end,
// hiss and crackle ("Noise"), and random level dropouts like a worn tape. The wet signal can be mixed with the dry.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <juce_core/juce_core.h>
#include "ModFx.h"

// b120: Bit mangle. Treats each sample as a 16-bit number and corrupts its bits, the way a damaged digital file or a failing converter does.
// Amount 0 is off. Low amounts flip a few of the low bits (a rough, noisy grain that follows the signal); higher amounts flip higher bits too
// (harsh, gated distortion) and past the middle also clear the lowest bits (a bit-depth collapse). The set of flipped bits is re-rolled many times a
// second, so it keeps changing, like rot spreading. The sign bit is only touched at the very top of the range. My own design, not measured on anything.
class BitMangle
{
public:
    void reset() { mask = 0u;  clearBits = 0;  rng = 0x2545F491u;  hold = 0; }

    void process (float* x, int n, float amount, double sampleRate)
    {
        if (amount < 0.001f) { mask = 0u;  return; }
        const float a = std::min (1.0f, amount);
        const int reroll = std::max (8, (int) (sampleRate / (12.0 + 180.0 * (double) a)));          // about 12 to 190 changes a second
        const int topBit = 6 + (int) (8.0f * a) + (a > 0.9f ? 1 : 0);                                // 6 up to 15 (the sign bit) only at the very top
        const int nFlip = 1 + (int) (3.0f * a);
        const int clr = a > 0.5f ? (int) ((a - 0.5f) * 2.0f * 9.0f) : 0;                            // up to 9 low bits cleared
        for (int i = 0; i < n; ++i)
        {
            if (--hold <= 0)
            {
                hold = reroll / 2 + (int) (nextU() % (unsigned) reroll);
                mask = 0u;
                for (int k = 0; k < nFlip; ++k)
                    mask ^= 1u << (nextU() % (unsigned) (topBit + 1));
                clearBits = clr;
            }
            const float v = std::max (-1.0f, std::min (1.0f, x[i]));
            int s = (int) std::lround (v * 32767.0f);
            unsigned u = (unsigned) (s + 32768) & 0xFFFFu;                                          // offset binary, so a flipped bit moves the level, not the sign convention
            u ^= mask;
            u &= ~((1u << clearBits) - 1u);
            const float y = ((float) (int) u - 32768.0f) / 32767.0f;
            x[i] = x[i] + a * (std::max (-1.0f, std::min (1.0f, y)) - x[i]);                        // the amount also fades it in
        }
    }

private:
    unsigned nextU() { rng ^= rng << 13;  rng ^= rng >> 17;  rng ^= rng << 5;  return rng; }
    unsigned mask = 0u, rng = 0x2545F491u;
    int clearBits = 0, hold = 0;
};

class LoFiWarble
{
public:
    struct Params
    {
        int   shape;      // 0 sine, 1 triangle, 2 random (smoothed steps), 3 and up = the Mod FX waves (b43)
        float rate;       // Hz
        float depth;      // 0..1: how far the pitch swings
        float drift;      // 0..1: random wander and flutter
        float tone;       // 0..1: top-end cutoff (dark to bright)
        float dirt;       // 0..1: saturation plus bit and sample-rate crushing
        float noise;      // 0..1: hiss and crackle
        float dropouts;   // 0..1: how often (and how deep) the level dips
        float mix;        // 0..1: dry to wet
        float levelDb;    // output level
        int   voice = 0;  // b62: 0 = the original warble pedal, 1 = Cassette (tape saturation, no bit crushing, head bump, steeper top roll-off, steady hiss); b105: 2 = Vinyl (a slow wow, soft saturation, surface hiss with ticks and pops)
        float breath = 0.0f;     // b120: Cassette only: tape noise reduction that does not quite match (the hiss swells and dips with your playing)
    };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        size = 1;
        while (size < (int) (0.06 * sr) + 8) size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        w = 0;
        phase = 0.0;
        driftLp = driftLp2 = 0.0f;
        flutterPhase = 0.0;
        randTarget = randSmooth = 0.0f;
        heldSample = 0.0f;  holdCounter = 0.0f;
        lp1 = lp2 = 0.0f;  lpIn1 = lpIn2 = 0.0f;  hpState = 0.0f;  cLp1 = bA = bB = 0.0f;
        hissLp = 0.0f;  crackle = 0.0f;  crackleDecay = 0.985f;
        dropRemaining = dropLen = 0;  dropDepth = 0.0f;  nrLpE = nrEnvE = nrLpD = nrEnvD = 0.0f;
        lastLevel = 1.0f;
        rng = 0x9E3779B9u;
    }

    void process (float* x, int n, const Params& p)
    {
        if (buf.empty()) return;
        const float rate = std::max (0.05f, p.rate);
        const bool cass = p.voice == 1;
        const bool vinyl = p.voice == 2;                 // b105: my own design from the general character of a worn record, not measured on one
        const bool soft = cass || vinyl;                  // tape / record saturation instead of bit crushing
        // Peak pitch swing (a ratio): up to about 6% (a bit under a semitone) at full Depth. The delay excursion that
        // makes it is capped, so slow rates swing less, like a real worn transport.
        const float swing = 0.06f * p.depth * p.depth;
        const float excursion = std::min (0.018f, swing / (6.2831853f * std::max (rate, 0.3f)));
        const float driftExc = p.drift * (vinyl ? 0.0008f : cass ? 0.0025f : 0.004f), flutterExc = p.drift * (vinyl ? 0.0004f : cass ? 0.0012f : 0.0003f);      // cassette: more of the fast flutter, less slow wander
        const float baseDelay = 0.022f * (float) sr;
        const float dPhase = rate / (float) sr;

        // tone: a second-order low-pass from 1.5 kHz to 12 kHz
        const float fc = std::min ((cass ? 2000.0f * std::pow (4.5f, p.tone) : vinyl ? 3500.0f * std::pow (4.0f, p.tone) : 1500.0f * std::pow (8.0f, p.tone)), 0.45f * (float) sr);
        const float k = std::tan (3.14159265f * fc / (float) sr);
        const float q = 0.7071f, norm = 1.0f / (1.0f + k / q + k * k);
        const float b0 = k * k * norm, b1 = 2.0f * b0, a1 = 2.0f * (k * k - 1.0f) * norm, a2 = (1.0f - k / q + k * k) * norm;

        const float g = 1.0f + 9.0f * p.dirt * p.dirt;
        const float bias = 0.12f * p.dirt;
        const float biasOut = std::tanh (bias);
        const float levels = std::pow (2.0f, 11.0f - 6.0f * p.dirt);                // 11 down to 5 bits
        const float holdLen = 1.0f + 5.0f * p.dirt * p.dirt;                         // sample-and-hold length
        const float hissGain = 0.025f * p.noise * p.noise;
        const float crackleRate = 12.0f * p.noise * p.noise / (float) sr;            // chance per sample
        const float dropRate = 2.5f * p.dropouts / (float) sr;
        const float level = std::pow (10.0f, p.levelDb / 20.0f);
        const float mix = p.mix;
        const float randCoef = 1.0f - std::exp (-rate * 3.0f / (float) sr);
        const float driftCoef = 1.0f - std::exp (-0.6f * 6.2831853f / (float) sr);
        const float hpCoef = 1.0f - std::exp (-(cass ? 35.0f : vinyl ? 40.0f : 60.0f) * 6.2831853f / (float) sr);
        const float cLp = 1.0f - std::exp (-6.2831853f * fc / (float) sr);                      // cassette: one more pole on the top
        const float bumpA = 1.0f - std::exp (-6.2831853f * 130.0f / (float) sr), bumpB = 1.0f - std::exp (-6.2831853f * 45.0f / (float) sr);    // cassette: the head bump
        const float hissCoef = 1.0f - std::exp (-6000.0f * 6.2831853f / (float) sr);
        // b120: the tape noise reduction (a Dolby-style compander, my own design): before the tape the quiet highs are boosted, after it they are
        // cut back by what the decoder thinks it sees; the two never match exactly, so the hiss and the top end swell and dip with your playing.
        const bool nr = cass && p.breath > 0.001f;
        const float nrSplit = 1.0f - std::exp (-6.2831853f * 2200.0f / (float) sr);
        const float nrAtk = 1.0f - std::exp (-1.0f / (0.0008f * (float) sr)), nrRel = 1.0f - std::exp (-1.0f / (0.060f * (float) sr));
        const float nrG = 1.0f + 3.0f * p.breath;                                        // up to +12 dB on quiet highs
        auto nrGain = [nrG] (float env) { return 1.0f + (nrG - 1.0f) / (1.0f + env * env * 2500.0f); };       // knee around -34 dBFS of high-band level

        for (int i = 0; i < n; ++i)
        {
            const float in = x[i];
            buf[(size_t) w] = in;

            // ---- the warble: a delay that moves
            float lfo;
            if (p.shape == 0) lfo = std::sin (6.2831853f * (float) phase);
            else if (p.shape == 1) lfo = 4.0f * std::fabs ((float) phase - 0.5f) - 1.0f;
            else if (p.shape >= 3) lfo = modLfoWave ((float) phase, p.shape - 3);       // b43: 3 and up = the Mod FX waves
            else lfo = randSmooth;
            phase += dPhase;
            if (phase >= 1.0)
            {
                phase -= 1.0;
                randTarget = nextRandom();                                         // a new step each cycle
            }
            randSmooth += randCoef * (randTarget - randSmooth);
            const float wander = nextRandom();
            driftLp  += driftCoef * (wander - driftLp);
            driftLp2 += driftCoef * (driftLp - driftLp2);
            flutterPhase += (8.0 + 2.5 * (double) driftLp2) / sr;
            if (flutterPhase >= 1.0) flutterPhase -= 1.0;
            const float flutter = std::sin (6.2831853f * (float) flutterPhase);
            const float delay = baseDelay + (excursion * lfo + driftExc * 6.0f * driftLp2 + flutterExc * flutter) * (float) sr;

            float rp = (float) w - delay;
            while (rp < 0.0f) rp += (float) size;
            const int i1 = (int) rp;
            const float f = rp - (float) i1;
            const float ym1 = buf[(size_t) ((i1 - 1) & (size - 1))], y0 = buf[(size_t) (i1 & (size - 1))],
                        y1  = buf[(size_t) ((i1 + 1) & (size - 1))], y2 = buf[(size_t) ((i1 + 2) & (size - 1))];
            float wet = y0 + 0.5f * f * (y1 - ym1 + f * (2.0f * ym1 - 5.0f * y0 + 4.0f * y1 - y2 + f * (3.0f * (y0 - y1) + y2 - ym1)));
            w = (w + 1) & (size - 1);

            if (nr)
            {
                nrLpE += nrSplit * (wet - nrLpE);
                const float hf = wet - nrLpE, ah = std::fabs (hf);
                nrEnvE += (ah > nrEnvE ? nrAtk : nrRel) * (ah - nrEnvE);
                wet = nrLpE + hf * nrGain (nrEnvE);
            }
            // ---- worn-out tone: soft clip, then crush
            if (p.dirt > 0.001f && soft)
            {
                // tape saturation: a soft, slightly asymmetric squash with no quantising
                const float gc = 1.0f + 5.0f * p.dirt * p.dirt;
                wet = (std::tanh (gc * wet + 0.5f * bias) - std::tanh (0.5f * bias)) / gc;
            }
            else if (p.dirt > 0.001f)
            {
                wet = (std::tanh (g * wet + bias) - biasOut) / g;
                holdCounter += 1.0f;
                if (holdCounter >= holdLen) { holdCounter -= holdLen; heldSample = std::round (wet * levels) / levels; }
                wet = heldSample;
            }
            // band limit: a low-pass, and a gentle high-pass for the thin, cheap sound
            const float lpOut = b0 * wet + b1 * lpIn1 + b0 * lpIn2 - a1 * lp1 - a2 * lp2;
            lpIn2 = lpIn1;  lpIn1 = wet;  lp2 = lp1;  lp1 = lpOut;
            wet = lpOut;
            if (cass)
            {
                cLp1 += cLp * (wet - cLp1);  wet = cLp1;
                bA += bumpA * (wet - bA);  bB += bumpB * (wet - bB);
                wet += 0.45f * (bA - bB);
            }
            hpState += hpCoef * (wet - hpState);
            wet -= hpState;

            // ---- noise
            if (p.noise > 0.001f)
            {
                hissLp += hissCoef * (nextRandom() - hissLp);
                wet += hissGain * hissLp * 4.0f;
                if (vinyl)
                {
                    // surface noise: ticks (short, small) with the odd pop (louder, longer); more of both as Noise rises
                    if (std::fabs (nextRandom()) < (4.0f + 60.0f * p.noise * p.noise) / (float) sr)
                    {
                        const bool pop = std::fabs (nextRandom()) < 0.12f;
                        const float r = std::fabs (nextRandom());
                        crackle = (nextRandom() > 0.0f ? 1.0f : -1.0f) * (pop ? 0.25f + 0.5f * r : 0.03f + 0.2f * r * r) * (0.4f + 0.6f * p.noise);
                        crackleDecay = std::exp (-1.0f / ((pop ? 0.0012f : 0.00025f) * (float) sr));
                    }
                    wet += crackle;
                    crackle *= crackleDecay;
                }
                else
                {
                    if (! cass && std::fabs (nextRandom()) < crackleRate * 0.5f) crackle = (nextRandom() > 0.0f ? 1.0f : -1.0f) * (0.05f + 0.25f * std::fabs (nextRandom())) * p.noise;
                    wet += crackle;
                    crackle *= 0.985f;
                }
            }

            // ---- dropouts: smooth dips in level
            if (p.dropouts > 0.001f || dropRemaining > 0)
            {
                if (dropRemaining <= 0 && std::fabs (nextRandom()) < dropRate)
                {
                    dropLen = dropRemaining = (int) ((0.015f + 0.065f * std::fabs (nextRandom())) * (float) sr);
                    dropDepth = 0.35f + 0.6f * p.dropouts;
                }
                if (dropRemaining > 0)
                {
                    const float prog = 1.0f - (float) dropRemaining / (float) std::max (1, dropLen);
                    const float s = std::sin (3.14159265f * prog);
                    wet *= 1.0f - dropDepth * s * s;
                    --dropRemaining;
                }
            }

            if (nr)
            {
                nrLpD += nrSplit * (wet - nrLpD);
                const float hfOut = (wet - nrLpD) / nrGain (nrEnvD);
                const float ao = std::fabs (hfOut);
                nrEnvD += (ao > nrEnvD ? nrAtk : nrRel) * (ao - nrEnvD);
                wet = nrLpD + hfOut;
            }
            x[i] = level * (in + mix * (wet - in));
        }
    }

private:
    float nextRandom()                                   // -1..1
    {
        rng ^= rng << 13;  rng ^= rng >> 17;  rng ^= rng << 5;
        return (float) (int32_t) rng * (1.0f / 2147483648.0f);
    }

    std::vector<float> buf;
    double sr = 48000.0, phase = 0.0, flutterPhase = 0.0;
    int size = 1, w = 0, dropRemaining = 0, dropLen = 0;
    float driftLp = 0, driftLp2 = 0, randTarget = 0, randSmooth = 0, heldSample = 0, holdCounter = 0;
    float cLp1 = 0, bA = 0, bB = 0, nrLpE = 0, nrEnvE = 0, nrLpD = 0, nrEnvD = 0;
    float lp1 = 0, lp2 = 0, lpIn1 = 0, lpIn2 = 0, hpState = 0, hissLp = 0, crackle = 0, crackleDecay = 0.985f, dropDepth = 0, lastLevel = 1.0f;
    uint32_t rng = 0x9E3779B9u;
};
