#pragma once
// Boost (v51): a pre-amp stage with a "tighten" filter, a four-band parametric EQ and a choice of saturation characters.
//
//   input -> [tighten] -> [EQ, if placed before] -> gain -> saturation (2x oversampled) -> [EQ, if placed after] -> level
//
// Saturation modes (all my own hand-tuned designs, in the character of the kind of gear named in the notes, nothing more):
//   Clean    : (v93) an op-amp boost from published component values: gain 0 to 26 dB, 4.5 V rails, no colour
//   Treble   : (v93) a one-transistor treble booster from published component values: 2.9 kHz input corner (Sat moves the input capacitor
//              from 5 nF to 47 nF), gain up to 38 dB, lopsided germanium-style limit
//   (the other modes below are my own hand-tuned designs)
//   EP       : a warm, slightly bass-heavy, gently asymmetric soft clip with a rolled-off top (a small tube-style tape-echo preamp)
//   Console  : transformer-style: the lows saturate earlier than the highs, symmetrical odd harmonics, a little "silk" on top
//   Tube     : an asymmetric triode-style stage (even and odd harmonics), a touch of top-end loss
//   Mid      : a mid-forward boost: bass trimmed, a broad push around 900 Hz, a gentle asymmetric clip
//   FET      : a transparent JFET-style stage: very little top loss, mostly even harmonics, dynamic and touch-sensitive
// Tighten styles: Mid hump (low cut + a mid hump), Low cut only, Focus (low cut + a push around 1.5 kHz).
#include <algorithm>
#include <cmath>
#include <vector>
#include "CircuitModels.h"

class BoostStage
{
public:
    struct Band { float hz, db, q; };
    struct Params
    {
        int   mode = 0;             // 0 Clean, 1 EP, 2 Console, 3 Tube, 4 Treble, 5 Mid, 6 FET
        float gainDb = 0.0f;        // 0..30
        float sat = 0.3f;           // 0..1
        float tight = 0.0f;         // 0..1 (0 = off)
        int   tightStyle = 0;       // 0 Mid hump, 1 Low cut, 2 Focus
        float levelDb = 0.0f;
        bool  eqOn = false;
        int   eqPos = 1;            // 0 = before the saturation, 1 = after
        Band  lo { 100.0f, 0.0f, 0.7f }, m1 { 400.0f, 0.0f, 1.0f }, m2 { 2500.0f, 0.0f, 1.0f }, hi { 6000.0f, 0.0f, 0.7f };
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        pedC.prepare (sampleRate);  pedT.prepare (2.0 * sampleRate);
        circuitKey[0] = circuitKey[1] = -999.0f;  circuitSat[0] = circuitSat[1] = -1.0f;
        reset();
        lastKey = {};
    }

    void reset()
    {
        for (Biquad* b : { &tightHp, &tightPk, &eqLo, &eqM1, &eqM2, &eqHi, &modePre1, &modePre2, &modePost1, &modePost2, &upA, &upB, &dnA, &dnB, &splitLp, &splitLp2 })
            b->clear();
        pedC.reset();  pedT.reset();  circuitKey[0] = circuitKey[1] = -999.0f;
        dcX = dcY = 0.0f;
        lastGain = 1.0f;
        lastLevel = 1.0f;
    }

    // x: mono, processed in place.
    void process (float* x, int n, const Params& p)
    {
        updateCoefficients (p);
        const float gainT = std::pow (10.0f, p.gainDb / 20.0f);
        const float levelT = std::pow (10.0f, p.levelDb / 20.0f);
        const bool tightOn = p.tight > 0.001f;
        const bool saturate = p.mode != 0 && p.sat > 0.001f;
        const bool eqBefore = p.eqOn && p.eqPos == 0;
        const bool eqAfter  = p.eqOn && p.eqPos != 0;

        if (tightOn)
            for (int i = 0; i < n; ++i)
            {
                float v = tightHp.run (x[i]);
                if (p.tightStyle != 1) v = tightPk.run (v);
                x[i] = v;
            }
        if (eqBefore) runEq (x, n);

        if (p.mode == 0 || p.mode == 4)
        {
            // v93: the circuit versions replace the gain and saturation stages
            const bool treble = p.mode == 4;
            const int key = treble ? 1 : 0;
            if (circuitKey[key] != p.gainDb || circuitSat[key] != p.sat)
            {
                (treble ? pedT : pedC).setupBoost (treble ? 0 : 1, p.gainDb, p.sat);
                circuitKey[key] = p.gainDb;  circuitSat[key] = p.sat;
            }
            if (! treble)
                for (int i = 0; i < n; ++i) x[i] = pedC.process (x[i]);
            else
                for (int i = 0; i < n; ++i)
                {
                    float out = 0.0f;
                    for (int ph = 0; ph < 2; ++ph)
                    {
                        const float u = upB.run (upA.run (ph == 0 ? 2.0f * x[i] : 0.0f));
                        const float y = dnB.run (dnA.run (pedT.process (u)));
                        if (ph == 0) out = y;
                    }
                    x[i] = out;
                }
            if (eqAfter) runEq (x, n);
            const float step = (levelT - lastLevel) / (float) n;
            float g = lastLevel;
            for (int i = 0; i < n; ++i) { x[i] *= g; g += step; }
            lastLevel = levelT;
            return;
        }

        // gain (ramped across the block)
        {
            const float step = (gainT - lastGain) / (float) n;
            float g = lastGain;
            for (int i = 0; i < n; ++i) { x[i] *= g; g += step; }
            lastGain = gainT;
        }

        if (saturate)
            saturateBlock (x, n, p);
        else if (p.mode != 0)
            for (int i = 0; i < n; ++i) x[i] = modePre1.run (x[i]);       // keep the voicing filters' state moving

        if (eqAfter) runEq (x, n);

        {
            const float step = (levelT - lastLevel) / (float) n;
            float g = lastLevel;
            for (int i = 0; i < n; ++i) { x[i] *= g; g += step; }
            lastLevel = levelT;
        }
    }

private:
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void clear() { z1 = z2 = 0.0f; }
        void identity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
        float run (float x)
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
        void set (float nb0, float nb1, float nb2, float na0, float na1, float na2)
        {
            const float inv = 1.0f / na0;
            b0 = nb0 * inv; b1 = nb1 * inv; b2 = nb2 * inv; a1 = na1 * inv; a2 = na2 * inv;
        }
        // RBJ cookbook
        void lowPass (float fs, float f, float q)
        {
            f = std::min (f, 0.49f * fs);
            const float w = 6.2831853f * f / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
            set ((1 - c) * 0.5f, 1 - c, (1 - c) * 0.5f, 1 + al, -2 * c, 1 - al);
        }
        void highPass (float fs, float f, float q)
        {
            f = std::min (f, 0.49f * fs);
            const float w = 6.2831853f * f / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
            set ((1 + c) * 0.5f, -(1 + c), (1 + c) * 0.5f, 1 + al, -2 * c, 1 - al);
        }
        void peak (float fs, float f, float q, float db)
        {
            f = std::min (f, 0.49f * fs);
            const float A = std::pow (10.0f, db / 40.0f), w = 6.2831853f * f / fs, c = std::cos (w), al = std::sin (w) / (2.0f * q);
            set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
        }
        void lowShelf (float fs, float f, float db)
        {
            f = std::min (f, 0.49f * fs);
            const float A = std::pow (10.0f, db / 40.0f), w = 6.2831853f * f / fs, c = std::cos (w), s = std::sin (w);
            const float al = s / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * al;
            set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                 (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
        }
        void highShelf (float fs, float f, float db)
        {
            f = std::min (f, 0.49f * fs);
            const float A = std::pow (10.0f, db / 40.0f), w = 6.2831853f * f / fs, c = std::cos (w), s = std::sin (w);
            const float al = s / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * al;
            set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                 (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
        }
    };

    struct Key
    {
        int mode = -1, tightStyle = -1;
        float tight = -1, sat = -1;
        Band b[4] {};
        bool operator== (const Key& o) const
        {
            if (mode != o.mode || tightStyle != o.tightStyle || tight != o.tight || sat != o.sat) return false;
            for (int i = 0; i < 4; ++i)
                if (b[i].hz != o.b[i].hz || b[i].db != o.b[i].db || b[i].q != o.b[i].q) return false;
            return true;
        }
    };

    void updateCoefficients (const Params& p)
    {
        Key k;
        k.mode = p.mode; k.tightStyle = p.tightStyle; k.tight = p.tight; k.sat = p.sat;
        k.b[0] = p.lo; k.b[1] = p.m1; k.b[2] = p.m2; k.b[3] = p.hi;
        if (k == lastKey) return;
        lastKey = k;

        // tighten: the low cut climbs from 40 Hz to 300 Hz
        const float t = std::min (1.0f, std::max (0.0f, p.tight));
        tightHp.highPass (sr, 40.0f + 260.0f * t, 0.75f);
        if (p.tightStyle == 0)       tightPk.peak (sr, 720.0f, 0.8f, 5.0f * t);
        else if (p.tightStyle == 2)  tightPk.peak (sr, 1500.0f, 0.9f, 5.5f * t);

        eqLo.lowShelf (sr, p.lo.hz, p.lo.db);
        eqM1.peak (sr, p.m1.hz, p.m1.q, p.m1.db);
        eqM2.peak (sr, p.m2.hz, p.m2.q, p.m2.db);
        eqHi.highShelf (sr, p.hi.hz, p.hi.db);

        // the voicing around each saturation mode
        const float s = p.sat;
        modePre1.identity();  modePre2.identity();  modePost1.identity();  modePost2.identity();
        switch (p.mode)
        {
            case 1:  modePre1.lowShelf (sr, 110.0f, 3.0f + 3.0f * s);  modePost1.lowPass (sr, 9000.0f, 0.6f); break;      // EP
            case 2:  modePost1.highShelf (sr, 11000.0f, 1.5f + 1.5f * s);                                       break;      // Console
            case 3:  modePre1.highPass (sr, 40.0f, 0.7f);  modePost1.lowPass (sr, 11000.0f, 0.6f);            break;      // Tube
            case 4:  modePre1.highPass (sr, 450.0f + 150.0f * s, 0.7f);  modePre2.peak (sr, 1300.0f, 1.0f, 4.0f);
                     modePost1.lowPass (sr, 8500.0f, 0.6f);                                                   break;      // Treble
            case 5:  modePre1.highPass (sr, 130.0f, 0.7f);  modePre2.peak (sr, 900.0f, 0.8f, 4.0f + 3.0f * s);
                     modePost1.lowPass (sr, 9500.0f, 0.6f);                                                   break;      // Mid
            case 6:  modePre1.highPass (sr, 35.0f, 0.7f);  modePost1.lowPass (sr, 13000.0f, 0.6f);           break;      // FET
            default: break;
        }
        // oversampling filters: 4th-order Butterworth at 0.47 x the base rate, at twice the base rate
        const float fs2 = 2.0f * sr, fc = 0.47f * sr;
        upA.lowPass (fs2, fc, 0.5412f);  upB.lowPass (fs2, fc, 1.3066f);
        dnA.lowPass (fs2, fc, 0.5412f);  dnB.lowPass (fs2, fc, 1.3066f);
        splitLp.lowPass (sr * 2.0f, 150.0f, 0.7071f);  splitLp2.lowPass (sr * 2.0f, 150.0f, 0.7071f);
    }

    void runEq (float* x, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            float v = x[i];
            if (std::abs (lastKey.b[0].db) > 0.05f) v = eqLo.run (v);
            if (std::abs (lastKey.b[1].db) > 0.05f) v = eqM1.run (v);
            if (std::abs (lastKey.b[2].db) > 0.05f) v = eqM2.run (v);
            if (std::abs (lastKey.b[3].db) > 0.05f) v = eqHi.run (v);
            x[i] = v;
        }
    }

    // one sample of the non-linear part
    float shape (int mode, float x, float d, float lowPart)
    {
        switch (mode)
        {
            case 1:        // EP: gentle asymmetry
            {
                const float b = 0.22f, tb = std::tanh (b), norm = 1.0f / (std::sqrt (d) * (1.0f - tb * tb));
                return (std::tanh (d * x + b) - tb) * norm;
            }
            case 2:        // Console: the low band saturates earlier (iron), the highs more gently
            {
                const float hi = x - lowPart;
                const float dl = d * 2.5f, dh = d * 0.7f;
                return std::tanh (dl * lowPart) / std::sqrt (dl) + std::tanh (dh * hi) / std::sqrt (dh);
            }
            case 3:        // Tube: positive side softer, negative side clips harder
            {
                const float b = 0.30f, tb = std::tanh (b), norm = 1.0f / (std::sqrt (d) * (1.0f - tb * tb));
                const float v = d * x + b;
                const float y = v > 0.0f ? std::tanh (v) : std::tanh (1.6f * v) / 1.6f;
                return (y - (b > 0 ? tb : tb)) * norm;
            }
            case 5:        // Mid: mild asymmetry, firmer on the negative side
            {
                const float b = 0.25f, tb = std::tanh (b), norm = 1.0f / (std::sqrt (d) * (1.0f - tb * tb));
                const float v = d * x + b;
                const float y = v > 0.0f ? std::tanh (v) : 0.75f * std::tanh (v / 0.75f);
                return (y - tb) * norm;
            }
            case 6:        // FET: very gentle asymmetry, mostly second harmonic
            {
                const float b = 0.18f, tb = std::tanh (b), norm = 1.0f / (std::sqrt (d) * (1.0f - tb * tb));
                const float v = d * 0.8f * x + b;
                const float y = v > 0.0f ? std::tanh (v) : 0.85f * std::tanh (v / 0.85f);
                return (y - tb) * norm * 1.12f;
            }
            default:       // Treble: germanium-like, more asymmetric
            {
                const float b = 0.45f, tb = std::tanh (b), norm = 1.0f / (std::sqrt (d) * (1.0f - tb * tb));
                const float v = d * x + b;
                const float y = v > 0.0f ? std::tanh (v) : 0.6f * std::tanh (v / 0.6f);
                return (y - tb) * norm;
            }
        }
    }

    void saturateBlock (float* x, int n, const Params& p)
    {
        const float d = 1.0f + 5.0f * p.sat * p.sat + 1.5f * p.sat;      // 1 .. 7.5
        for (int i = 0; i < n; ++i)
        {
            float in = modePre2.run (modePre1.run (x[i]));
            // 2x oversampling: zero-stuff, filter, shape, filter, take every second sample
            float out = 0.0f;
            for (int ph = 0; ph < 2; ++ph)
            {
                const float u = upB.run (upA.run (ph == 0 ? 2.0f * in : 0.0f));
                const float lowPart = splitLp2.run (splitLp.run (u));
                float y = shape (p.mode, u, d, lowPart);
                y = dnB.run (dnA.run (y));
                if (ph == 0) out = y;
            }
            out = modePost2.run (modePost1.run (out));
            // remove the DC the asymmetric shapes make
            const float dc = out - dcX + 0.9995f * dcY;
            dcX = out;  dcY = dc;
            x[i] = dc;
        }
    }

    float sr = 48000.0f;
    Key lastKey;
    Biquad tightHp, tightPk, eqLo, eqM1, eqM2, eqHi, modePre1, modePre2, modePost1, modePost2, upA, upB, dnA, dnB, splitLp, splitLp2;
    Circuit::NewPedals pedC, pedT;
    float circuitKey[2] { -999.0f, -999.0f }, circuitSat[2] { -1.0f, -1.0f };
    float dcX = 0.0f, dcY = 0.0f, lastGain = 1.0f, lastLevel = 1.0f;
};
