#pragma once
// Metal pedals (v97): approximations of four pedals known only from their published descriptions (control list and text),
// not from schematics. Each is built from the same blocks as the other circuit models: a pre-gain high-pass (the "Cut" control),
// gain stages with diode clipping, and an active EQ made of shelves and peaks. Everything here is my own estimate of the
// behaviour described; none of the gains, corner frequencies or EQ centres are measured from the real pedals.
//   11 Altar: Muff-type fuzz core, 3 band EQ (+/-18 dB), Comp (clipping diodes LED -> silicon -> germanium), Depth, clean boost.
//   12 Medusa: HM-2 style core with extra gain, Cut, Low / Mid / Grind / Presence, a noise gate.
//   13 Gehenna: two switchable gain stages, Cut, 4 band active EQ.
//   14 Total Distortion Worship: distortion, Cut, 5 band EQ (+/-12 dB) after it, a boost, a noise gate.
#include <algorithm>
#include <cmath>
#include "CircuitModels.h"

namespace Circuit
{
    // RBJ biquads in the same coefficient layout as Circuit::Bq
    inline BiquadCoeffs rbjPeak (double fs, double f, double q, double dB)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * 3.14159265358979 * f / fs, al = std::sin (w) / (2.0 * q), c = std::cos (w);
        const double a0 = 1.0 + al / A;
        BiquadCoeffs o;  o.b0 = (1.0 + al * A) / a0;  o.b1 = -2.0 * c / a0;  o.b2 = (1.0 - al * A) / a0;  o.a1 = -2.0 * c / a0;  o.a2 = (1.0 - al / A) / a0;
        return o;
    }
    inline BiquadCoeffs rbjShelf (double fs, double f, double dB, bool high)
    {
        const double A = std::pow (10.0, dB / 40.0), w = 2.0 * 3.14159265358979 * f / fs, c = std::cos (w), s = std::sin (w);
        const double al = s / 2.0 * std::sqrt (2.0), tA = 2.0 * std::sqrt (A) * al;
        BiquadCoeffs o;
        if (! high)
        {
            const double a0 = (A + 1) + (A - 1) * c + tA;
            o.b0 = A * ((A + 1) - (A - 1) * c + tA) / a0;  o.b1 = 2 * A * ((A - 1) - (A + 1) * c) / a0;  o.b2 = A * ((A + 1) - (A - 1) * c - tA) / a0;
            o.a1 = -2 * ((A - 1) + (A + 1) * c) / a0;       o.a2 = ((A + 1) + (A - 1) * c - tA) / a0;
        }
        else
        {
            const double a0 = (A + 1) - (A - 1) * c + tA;
            o.b0 = A * ((A + 1) + (A - 1) * c + tA) / a0;  o.b1 = -2 * A * ((A - 1) + (A + 1) * c) / a0;  o.b2 = A * ((A + 1) + (A - 1) * c - tA) / a0;
            o.a1 = 2 * ((A - 1) - (A + 1) * c) / a0;        o.a2 = ((A + 1) - (A - 1) * c - tA) / a0;
        }
        return o;
    }

    class MetalPedal
    {
    public:
        struct Params
        {
            int   style = 11;               // 11 Altar, 12 Medusa, 13 Gehenna, 14 TDW
            float gain = 0.5f, cut = 0.3f;  // 0..1
            float eq[5] = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };   // 0.5 = flat
            float depth = 0.5f, comp = 0.5f, boost = 0.0f, gate = 0.0f;
            int   diode = 0;
        };

        void prepare (double sampleRate)
        {
            fs = sampleRate;
            muff.prepare (fs);  hm2.prepare (fs);
            for (auto& b : eqB) b.reset();
            cutHp.reset();  postLp.reset();  clipA.reset();  clipB.reset();  dcHp.reset();
            gateEnv = 0.0f;  gateGain = 1.0f;  boostHp.reset();
            lastKey = -1.0f;
            setupClippers (0);
        }

        void configure (const Params& np)
        {
            p = np;
            const float key = (float) p.style * 1000.0f + p.cut * 100.0f + p.eq[0] * 7 + p.eq[1] * 11 + p.eq[2] * 13 + p.eq[3] * 17 + p.eq[4] * 19 + p.comp * 3 + p.diode;
            if (key != lastKey)
            {
                lastKey = key;
                // "Cut": a pre-gain high-pass, 25 Hz open to about 700 Hz tight (log sweep)
                const double fc = 25.0 * std::pow (28.0, (double) p.cut);
                cutHp.set (highPass1 (fs, fc));
                const float r18 = 18.0f, r12 = 12.0f;
                auto db = [] (float e, float r) { return (e - 0.5f) * 2.0f * r; };
                switch (p.style)
                {
                    case 11:  eqB[0].set (rbjShelf (fs, 110.0, db (p.eq[0], r18), false));
                              eqB[1].set (rbjPeak  (fs, 650.0, 0.7, db (p.eq[2], r18)));
                              eqB[2].set (rbjShelf (fs, 3200.0, db (p.eq[4], r18), true));
                              eqB[3].set (BiquadCoeffs()); eqB[4].set (BiquadCoeffs());
                              setupClippers (p.comp < 0.33f ? 2 : (p.comp < 0.66f ? 0 : 1));   // LED, silicon, germanium
                              break;
                    case 12:  eqB[0].set (rbjShelf (fs, 100.0, db (p.eq[0], 16.0f), false));
                              eqB[1].set (rbjPeak  (fs, 200.0 + 500.0 * p.eq[1], 0.9, db (p.eq[2], 16.0f)));    // Mid, with its frequency on Lo-Mid
                              eqB[2].set (rbjPeak  (fs, 1100.0, 1.4, db (p.eq[3], 16.0f)));                     // Grind
                              eqB[3].set (rbjShelf (fs, 4500.0, db (p.eq[4], 16.0f), true));                    // Presence
                              eqB[4].set (BiquadCoeffs());
                              break;
                    case 13:  eqB[0].set (rbjShelf (fs, 100.0, db (p.eq[0], 15.0f), false));
                              eqB[1].set (rbjPeak  (fs, 400.0, 0.8, db (p.eq[1], 15.0f)));
                              eqB[2].set (rbjPeak  (fs, 1000.0, 0.8, db (p.eq[3], 15.0f)));
                              eqB[3].set (rbjShelf (fs, 3800.0, db (p.eq[4], 15.0f), true));
                              eqB[4].set (BiquadCoeffs());
                              break;
                    default:  eqB[0].set (rbjShelf (fs, 80.0, db (p.eq[0], r12), false));
                              eqB[1].set (rbjPeak  (fs, 250.0, 0.8, db (p.eq[1], r12)));
                              eqB[2].set (rbjPeak  (fs, 800.0, 0.8, db (p.eq[2], r12)));
                              eqB[3].set (rbjPeak  (fs, 2500.0, 0.8, db (p.eq[3], r12)));
                              eqB[4].set (rbjShelf (fs, 6500.0, db (p.eq[4], r12), true));
                              break;
                }
            }
            if (p.style == 11)
            {
                MuffCircuit::Params cp;  cp.sustain = p.gain;  cp.tone = 0.5f;  cp.diode = p.comp < 0.33f ? 1 : (p.comp < 0.66f ? 0 : 2);
                cp.opAmp = false;  cp.tonePass = true;  cp.variant = 1;
                muff.configure (cp);
            }
            if (p.style == 12)
            {
                Hm2Circuit::Params hp;  hp.dist = std::min (1.0f, p.gain);  hp.low = 0.5f;  hp.high = 0.5f;  hp.mid = 0.5f;  hp.blend = 0.0f;  hp.soft = true;  hp.hard = true;
                hm2.configure (hp);
            }
        }

        float process (float x)
        {
            // noise gate (Medusa, TDW): keyed from the input, fast attack, slow release
            if (p.gate > 0.001f && (p.style == 12 || p.style == 14))
            {
                const float a = std::fabs (x);
                gateEnv += (a > gateEnv ? 0.02f : 0.0004f) * (a - gateEnv);
                const float thr = 0.0005f + 0.04f * p.gate * p.gate;
                const float target = gateEnv > thr ? 1.0f : 0.0f;
                gateGain += (target > gateGain ? 0.01f : 0.0015f) * (target - gateGain);
                x *= gateGain;
            }
            float y = cutHp.run (x);
            switch (p.style)
            {
                case 11:
                {
                    // Depth: input saturation / compression / fuzz before the core
                    const float dep = p.depth;
                    y = (1.0f - 0.5f * dep) * y + 0.5f * dep * std::tanh (3.0f * y) * 0.5f;
                    y = 0.8f * muff.process (y);
                    y = eq3 (y);
                    break;
                }
                case 12:
                {
                    const float pre = 1.0f + 5.0f * p.boost;               // the "modern" gain stage: more gain ahead of the classic core
                    y = hm2.process (std::tanh (pre * y));
                    y = eqN (y, 4);
                    y *= 0.8f;
                    break;
                }
                case 13:
                {
                    const float g1 = 3.0f + 90.0f * Circuit::audioTaper (p.gain);
                    const float g2 = 1.0f + 6.0f * p.boost;                  // Gain II
                    y = clipA.process (g1 * y * 0.1f) * 2.0f;
                    y = clipB.process (g2 * y * 3.0f) * 0.9f;
                    y = dcHp.run (y);
                    y = eqN (y, 4);
                    break;
                }
                default:
                {
                    const float g = 4.0f + 160.0f * Circuit::audioTaper (p.gain);
                    y = clipA.process (g * y * 0.05f) * 2.0f;
                    y = clipB.process (1.6f * y) * 0.9f;
                    y = dcHp.run (y);
                    y = eqN (y, 5);
                    if (p.boost > 0.001f)
                        y = std::tanh ((1.0f + 3.0f * p.boost) * y * 0.5f) * 2.0f * (0.7f);
                    break;
                }
            }
            if (p.style == 11 && p.boost > 0.001f)
                y = std::tanh (0.25f * (1.0f + 15.0f * p.boost * p.boost) * y) * 4.0f / (1.0f + 3.0f * p.boost);   // independent clean boost, up to about +24 dB
            return y;
        }

    private:
        float eq3 (float y) { for (int i = 0; i < 3; ++i) y = eqB[i].run (y); return y; }
        float eqN (float y, int n) { for (int i = 0; i < n; ++i) y = eqB[i].run (y); return y; }
        void setupClippers (int kind)
        {
            const DiodeSpec a = p.style == 14 ? diodeLed() : (kind == 1 ? diodeGermanium() : (kind == 2 ? diodeLed() : diodeSilicon()));
            clipA.setup (22000.0f, diodeSilicon(), diodeSilicon());
            clipB.setup (22000.0f, a, p.style == 13 ? diodeSilicon() : a);
            dcHp.set (highPass1 (fs, 20.0));
        }

        double fs = 48000.0;
        Params p;
        MuffCircuit muff;  Hm2Circuit hm2;
        Bq eqB[5], cutHp, postLp, dcHp, boostHp;
        DiodeLimiter clipA, clipB;
        float gateEnv = 0.0f, gateGain = 1.0f, lastKey = -1.0f;
    };
}
