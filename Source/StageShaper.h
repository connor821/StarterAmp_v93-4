#pragma once
// Stage and power-tube transfer curves derived from tube-circuit simulation (see StageCurves.h, generated with TubeNet.h).
// Each curve is normalised to unit slope at zero, so it drops in where the plain tanh used to be.
#include "StageCurves.h"
#include <cmath>

namespace StageShaper
{
    inline float lut (const float* y, float u)
    {
        float p = (u - StageCurves::kU0) / StageCurves::kDu;
        if (p < 0.0f) p = 0.0f;
        if (p > (float) (StageCurves::kN - 1) - 0.001f) p = (float) (StageCurves::kN - 1) - 0.001f;
        const int i = (int) p;
        const float f = p - (float) i;
        return y[i] + f * (y[i + 1] - y[i]);
    }

    inline const float* preCurve (int tube, int arch)
    {
        using namespace StageCurves;
        static const float* const t[3][5] = {
            { pre_0_0, pre_0_1, pre_0_2, pre_0_3, pre_0_4 },
            { pre_1_0, pre_1_1, pre_1_2, pre_1_3, pre_1_4 },
            { pre_2_0, pre_2_1, pre_2_2, pre_2_3, pre_2_4 } };
        return t[tube < 0 ? 0 : tube > 2 ? 2 : tube][arch < 0 ? 0 : arch > 4 ? 4 : arch];
    }
    inline const float* pwrCurve (int tube, bool hot)
    {
        using namespace StageCurves;
        static const float* const m[5] = { pwr_0_med, pwr_1_med, pwr_2_med, pwr_3_med, pwr_4_med };
        static const float* const h[5] = { pwr_0_hot, pwr_1_hot, pwr_2_hot, pwr_3_hot, pwr_4_hot };
        return (hot ? h : m)[tube < 0 ? 0 : tube > 4 ? 4 : tube];
    }

    // What each amp model's stages are built like. Preamp: which of five stage types (0 low-gain blackface-style,
    // 1 cold class-A, 2 crunch with a large cathode resistor, 3 hot, 4 hot cascade stage) feeds each gain stage.
    struct Plan
    {
        bool active = false;
        const float* pre[3] = { nullptr, nullptr, nullptr };
        const float* pwrMed = nullptr;
        const float* pwrHot = nullptr;
        float trimDb = 0.0f;
    };

    inline Plan planFor (int model, int preTube, int pwrSel)
    {
        // model: 0 clean, 1 chime, 2 hot-rod, 3 sat lead, 4 modern, 5 acoustic, 6 classic rock, 7 boutique, 8 tight metal,
        // 9 low-watt, 10 glass clean, 11 creamy, 12 doom, 13 precision, 14 digital, 15 MV rock
        static const signed char arch[16][3] = {
            { 0, 0, 0 }, { 1, 1, 1 }, { 2, 2, 3 }, { 2, 3, 3 }, { 3, 3, 3 }, { 0, 0, 0 }, { 2, 2, 2 }, { 1, 3, 3 },
            { 3, 3, 4 }, { 1, 1, 1 }, { 0, 0, 0 }, { 2, 3, 3 }, { 2, 2, 3 }, { 3, 3, 4 }, { 3, 3, 3 }, { 2, 2, 3 } };
        static const signed char defPwr[16] = { 1, 3, 0, 0, 1, 0, 0, 1, 1, 2, 1, 0, 4, 1, 0, 0 };
        Plan p;
        if (model < 0 || model > 15 || model == 5 || model == 10 || model == 14) return p;   // acoustic, solid-state and digital amps keep their own shaping
        for (int i = 0; i < 3; ++i) p.pre[i] = preCurve (preTube, arch[model][i]);
        const int pt = pwrSel <= 0 ? defPwr[model] : pwrSel - 1;
        p.pwrMed = pwrCurve (pt, false);
        p.pwrHot = pwrCurve (pt, true);
        // level match to the classic shaping (measured with a guitar-like test signal at three gain settings)
        static const float trim[16] = { -0.6f, 1.2f, 3.0f, 2.7f, 3.0f, 0.0f, 2.5f, 0.5f, 1.8f, 0.6f, 0.0f, 2.7f, 4.3f, 1.7f, 0.0f, 2.8f };
        p.trimDb = trim[model];
        p.active = true;
        return p;
    }

    // A preamp stage with grid-current "blocking": when the signal drives the grid positive the coupling capacitor charges
    // and pulls the operating point down, then it leaks back over tens of milliseconds.
    struct StageState
    {
        float shift = 0.0f;
        inline float process (const float* curve, float u, float attack, float release)
        {
            // v94: a smooth knee, half the charging rate, a shallower cap (0.5, was 2.0) and a 3x faster leak (about 15 ms, was 45 ms).
            // The old settings made the operating point wander enough to modulate the level of held notes (the "sputter") and to put
            // extra low-frequency thump on the output. Measured: held-note level wobble on a high E 0.88 dB -> 0.40 dB (Classic 0.34 dB).
            const float v = u + shift;
            const float d = v - 0.9f;
            const float over = d > 1.8f ? d : 0.15f * std::log1p (std::exp (d / 0.15f));
            shift -= 0.5f * attack * over;
            shift += 3.0f * release * (0.0f - shift);
            if (shift < -0.5f) shift = -0.5f;
            return lut (curve, v);
        }
    };

    // One-sample power curve (the same as powerStage below), for the feedback loop
    inline float powerSample (float s, float drive, float deadZone, const float* med, const float* hot, float hotAmt)
    {
        const float d = 1.0f + 5.0f * drive * drive;
        if (deadZone > 0.0f)
            s -= deadZone * std::tanh (s / deadZone);
        if (drive > 0.0f)
        {
            const float u = d * s;
            const float a = lut (med, u);
            s = (hotAmt > 0.0f ? a + hotAmt * (lut (hot, u) - a) : a) / d * std::sqrt (d);
        }
        return s;
    }

    // Power stage: P(d s) / d, with the same make-up the plain version used.
    inline void powerStage (float* y, int m, float drive, float deadZone, const float* med, const float* hot, float hotAmt)
    {
        const float d = 1.0f + 5.0f * drive * drive;
        const float comp = std::sqrt (d);
        for (int i = 0; i < m; ++i)
        {
            float s = y[i];
            if (deadZone > 0.0f)
                s -= deadZone * std::tanh (s / deadZone);
            if (drive > 0.0f)
            {
                const float u = d * s;
                const float a = lut (med, u);
                s = (hotAmt > 0.0f ? a + hotAmt * (lut (hot, u) - a) : a) / d * comp;
            }
            y[i] = s;
        }
    }
}
