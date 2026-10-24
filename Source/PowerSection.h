#pragma once
// Power section, supply and output transformer behaviour derived from circuit values (v83).
// Nothing here runs a circuit. The responses were solved offline (see MODELED_GEAR.md) and stored as small tables.
#include <cmath>
#include <algorithm>
#include "PowerSectionTable.h"

namespace PowerSection
{
    struct Params
    {
        int   family = -1;          // row of PowerSectionTable (-1 = no power-section model for this amp)
        int   feel = 1;             // 0 loose, 1 normal, 2 tight (feedback amount)
        bool  filtersOn = false;    // use the table filters in place of the presence shelf
        bool  supplyOn = false;     // two-stage supply droop and transformer saturation
        float s1 = 0.0f, s2 = 0.0f; // droop depth: fast (reservoir) and slow (filter stages)
        float tau1 = 0.02f, tau2 = 0.3f;  // seconds
        float otSat = 0.0f;         // output transformer low-end saturation amount
        float fbLg = 0.0f;          // loop gain of the global negative feedback (0 = none): reduces distortion until the stage runs out of swing
    };

    // Which table row each amp model uses (-1 = none: acoustic, solid-state and digital models)
    inline int familyForModel (int m)
    {
        static const int fam[34] = { 0, 3, 1, 2, 2, -1, 1, 4, 2, 3, -1, 2, 3, 2, -1, 1, 4, 2, 2, 2, 2, 2, -1, 1, 2, 1, 2, 2, 3, -1, 0, 0, -1, 3 };   // v0.1: 21 Hi-Wat Headroom (heavy feedback, so the tight family), 22 Console DI (no power section)   // v95: 16 to 20 are the five new amps (estimates, see MODELED_GEAR.md)   // v90: Marque Dream moved from the light-feedback family to the British-lead one (Mark-series feedback network read, see MODELED_GEAR.md)
        return (m >= 0 && m < 34) ? fam[m] : -1;
    }

    // Presence knob 0..1 interpolated between the five table steps
    inline void gainsFor (int fam, int feel, float presence, float* g)
    {
        const float x = std::min (3.9999f, std::max (0.0f, presence) * 4.0f);
        const int i = (int) x;
        const float t = x - (float) i;
        fam = std::max (0, std::min (PowerSectionTable::kFamilies - 1, fam));
        feel = std::max (0, std::min (2, feel));
        for (int k = 0; k < 4; ++k)
        {
            const float a = PowerSectionTable::gains[fam][feel][i][k];
            const float b = PowerSectionTable::gains[fam][feel][i + 1][k];
            g[k] = a + t * (b - a);
        }
    }

    // Loop gain (amplifier gain times feedback fraction) of the global feedback, by family and feel (loose, normal, tight),
    // from the same values the response table was solved with (open-loop gain times Rk / (Rk + Rfb / feel))
    inline float loopGainFor (int fam, int feel)
    {
        static const float lg[5][3] = { { 1.35f, 2.83f, 4.68f }, { 0.79f, 1.71f, 2.96f }, { 1.83f, 3.65f, 6.06f }, { 0.0f, 0.0f, 0.0f }, { 0.29f, 0.63f, 1.11f } };
        return lg[std::max (0, std::min (4, fam))][std::max (0, std::min (2, feel))];
    }

    // The output stage inside its feedback loop: y = F(g (x - beta y)). F is the one-sample power-stage curve with small-signal gain
    // 'comp'; g and beta are set so the closed-loop small-signal gain stays comp. Small signals pass as before, the distortion of
    // the stage is reduced by the loop gain, and when the stage runs out of swing the loop cannot help and it clips hard.
    // Solved per sample by Newton's method (three steps, starting from the last output).
    template <typename F>
    inline void feedbackStage (float* y, int m, float lg, float comp, float& last, F f)
    {
        const float g = 1.0f + lg, beta = lg / (comp * g);
        float yy = last;
        for (int i = 0; i < m; ++i)
        {
            const float x = y[i];
            for (int it = 0; it < 3; ++it)
            {
                const float u = g * (x - beta * yy);
                const float fu = f (u);
                const float e = 1.0e-3f;
                const float dF = (f (u + e) - f (u - e)) * (0.5f / e);
                const float h = yy - fu;
                const float hp = std::max (1.0f, 1.0f + g * beta * dF);
                yy -= h / hp;
            }
            y[i] = yy;
        }
        last = yy;
    }

    struct Chain
    {
        float b0[4], b1[4], b2[4], a1[4], a2[4];
        float z1[2][4] = {}, z2[2][4] = {};
        bool  active = false;

        void reset() { for (auto& c : z1) for (float& v : c) v = 0.0f; for (auto& c : z2) for (float& v : c) v = 0.0f; }

        // Same four basis filters the table was fitted with (RBJ): peak 100 Hz Q1.4, peak 1.2 kHz Q0.8, peak 3.5 kHz Q1, high shelf 7 kHz Q0.7
        void design (double sr, const float* gDb)
        {
            static const double f0[4] = { 100.0, 1200.0, 3500.0, 7000.0 }, Q[4] = { 1.4, 0.8, 1.0, 0.7 };
            for (int k = 0; k < 4; ++k)
            {
                const double A = std::pow (10.0, (double) gDb[k] / 40.0);
                const double w0 = 6.283185307179586 * std::min (f0[k], 0.45 * sr) / sr, al = std::sin (w0) / (2.0 * Q[k]), c = std::cos (w0);
                double b[3], a[3];
                if (k < 3) { b[0] = 1 + al * A; b[1] = -2 * c; b[2] = 1 - al * A; a[0] = 1 + al / A; a[1] = -2 * c; a[2] = 1 - al / A; }
                else
                {
                    const double sq = 2.0 * std::sqrt (A) * al;
                    b[0] = A * ((A + 1) + (A - 1) * c + sq);  b[1] = -2 * A * ((A - 1) + (A + 1) * c);  b[2] = A * ((A + 1) + (A - 1) * c - sq);
                    a[0] = (A + 1) - (A - 1) * c + sq;        a[1] = 2 * ((A - 1) - (A + 1) * c);      a[2] = (A + 1) - (A - 1) * c - sq;
                }
                b0[k] = (float) (b[0] / a[0]);  b1[k] = (float) (b[1] / a[0]);  b2[k] = (float) (b[2] / a[0]);
                a1[k] = (float) (a[1] / a[0]);  a2[k] = (float) (a[2] / a[0]);
            }
        }

        void process (float* x, int n, int ch)
        {
            ch = std::min (ch, 1);
            for (int k = 0; k < 4; ++k)
            {
                float s1 = z1[ch][k], s2 = z2[ch][k];
                const float B0 = b0[k], B1 = b1[k], B2 = b2[k], A1 = a1[k], A2 = a2[k];
                for (int i = 0; i < n; ++i)
                {
                    const float in = x[i];
                    const float out = B0 * in + s1;
                    s1 = B1 * in - A1 * out + s2;
                    s2 = B2 * in - A2 * out;
                    x[i] = out;
                }
                z1[ch][k] = s1;  z2[ch][k] = s2;
            }
        }
    };

    struct Supply
    {
        float e1[2] = {}, e2[2] = {}, lo[2] = {};
        void reset() { for (int c = 0; c < 2; ++c) e1[c] = e2[c] = lo[c] = 0.0f; }

        // Two time constants: the reservoir drops at once and recovers in tens of milliseconds, the filter stages
        // follow more slowly (a few hundred milliseconds for the biggest dropping resistor and capacitor).
        void droop (float* x, int n, int ch, const Params& p, double sr)
        {
            ch = std::min (ch, 1);
            const float r1 = std::exp (-1.0f / (p.tau1 * (float) sr));
            const float r2 = std::exp (-1.0f / (p.tau2 * (float) sr));
            const float atk2 = 1.0f - std::exp (-1.0f / (0.15f * p.tau2 * (float) sr));
            float a1 = e1[ch], a2 = e2[ch];
            for (int i = 0; i < n; ++i)
            {
                const float a = std::abs (x[i]);
                a1 = (a > a1) ? a : a + r1 * (a1 - a);
                a2 = (a > a2) ? a2 + atk2 * (a - a2) : a + r2 * (a2 - a);
                x[i] *= 1.0f / (1.0f + p.s1 * a1 + p.s2 * a2);
            }
            e1[ch] = a1;  e2[ch] = a2;
        }

        // Output transformer: the low end saturates first (flux), so loud bass notes are rounded off and compressed
        void transformer (float* x, int n, int ch, const Params& p, double sr)
        {
            if (p.otSat <= 0.0f) return;
            ch = std::min (ch, 1);
            const float c = 1.0f - std::exp (-6.283185307f * 110.0f / (float) sr);
            float l = lo[ch];
            for (int i = 0; i < n; ++i)
            {
                l += c * (x[i] - l);
                const float sat = std::tanh (1.3f * l) / 1.3f;
                x[i] += p.otSat * (sat - l);
            }
            lo[ch] = l;
        }
    };

    // Per-model supply and transformer estimates. The first four numbers are the reading of each amp's supply (B+ under load,
    // peak current of the output stage, source resistance of transformer and rectifier, dropping resistor times filter capacitor
    // in seconds); depth = 5 * Ipk * Rs / B+ for the fast stage. They are estimates: see MODELED_GEAR.md.
    struct SupplyInfo { float bplus, ipk, rs, tauSlow, otSat; };
    inline bool supplyFor (int model, SupplyInfo& s)
    {
        switch (model)
        {
            case 0:  s = { 480.0f, 0.45f,  60.0f, 0.044f, 0.15f }; return true;     // American combo, silicon rectifier, 2.2k x 20 uF
            case 1:  s = { 330.0f, 0.22f, 150.0f, 0.70f,  0.35f }; return true;     // chime combo, bridge rectifier, 22k x 32 uF
            case 6:  s = { 480.0f, 0.50f,  80.0f, 0.50f,  0.25f }; return true;     // 100 W 1959-style: four EL34s, silicon bridge, 10k x 50 uF dropping stage (read, 1970 schematic); B+ and peak current estimated
            case 2: case 15:
                     s = { 420.0f, 0.30f, 100.0f, 0.50f,  0.25f }; return true;     // 50 W British: 10k x 50 uF
            case 3:  s = { 495.0f, 0.40f,  90.0f, 0.40f,  0.15f }; return true;     // 100 W lead (SLO-100): 497 V plates, 495 V screens, 2 x 200 uF reservoir, choke, then 10k x 40 uF (read); peak current and source resistance estimated
            case 4:  s = { 460.0f, 0.50f,  80.0f, 0.081f, 0.15f }; return true;     // 100 W dual-rectifier amp: 2 x 220 uF reservoir, choke, 2.7k x 30 uF (read); source resistance between silicon and tube rectifier is my estimate
            case 8: case 13:
                     s = { 480.0f, 0.50f,  80.0f, 0.20f,  0.15f }; return true;     // 120 W modern (dropping network estimated)
            case 7:  s = { 436.0f, 0.45f, 110.0f, 0.044f, 0.25f }; return true;     // v0.2 Bumble Lead, read from the '70s Dumble-style power sheet: six-diode supply, 100 uF pair, 80 ohm choke (plus the transformer: about 110 ohm in all), 436 V on the output stage's rail through 2.2k with 20 uF; peak current and transformer saturation are estimates
            case 9:  s = { 335.0f, 0.12f, 250.0f, 0.075f, 0.40f }; return true;    // tweed combo (5E3): 5Y3 rectifier, 345 V at the plates, 320 V screens through 4.7k x 16 uF (read); the rectifier and transformer resistance and peak current are estimates
            case 11: s = { 448.0f, 0.35f,  60.0f, 0.185f, 0.20f }; return true;     // Mark-series: silicon bridge, 220 uF reservoir, choke, 5.6k x 30 uF (read); peak current and transformer saturation are my estimates
            case 12: s = { 500.0f, 0.50f, 110.0f, 0.60f,  0.30f }; return true;
            case 16: s = { 470.0f, 0.50f,  85.0f, 0.30f,  0.18f }; return true;     // v95 Harmonic Haze (4 x EL34 or 6L6, solid-state rectifier; the schematic shows a B1 rail but no values; all numbers estimated)
            case 17: s = { 470.0f, 0.50f,  85.0f, 0.35f,  0.20f }; return true;     // v95 Diesel Fuel (4 x KT77, 100 W; service manual gives bias only; estimated)
            case 18: s = { 545.0f, 0.60f,  70.0f, 0.25f,  0.15f }; return true;     // v95 Wreck Stack (read: 545 V plates, 335 V screens, 2 x 220 uF reservoirs, 150k and 330k dropping resistors; peak current and source resistance estimated)
            case 19: s = { 450.0f, 0.50f,  80.0f, 0.20f,  0.15f }; return true;     // v95 Power Brawl (100 W, 4 x 6L6GC; b18: plate supply read from the schematic, 440 V on / 460 V standby printed, 470R screen resistors, 220 uF reservoir; peak current and source resistance still estimated)
            case 20: s = { 480.0f, 0.55f,  85.0f, 0.30f,  0.20f }; return true;     // v95 Cutaway (100 to 140 W, 4 x EL34 as shipped; estimated)
            case 21: s = { 460.0f, 0.55f, 60.0f, 0.05f, 0.10f }; return true;     // v0.1 Hi-Wat Headroom (read from the 100 W British head schematic: bridge of four 1N5408 into 2 x 220 uF in series, 100 ohm to the screens, 1k x 50 uF to the preamp; the 460 V, peak current and source resistance are my estimates, and the transformer is assumed large)
            case 23: s = { 480.0f, 0.50f,  80.0f, 0.50f,  0.25f }; return true;     // v0.2 Plexi-style 100 W: the same supply reading as the 1959-style amp (silicon bridge, 10k x 50 uF dropping stage); estimated
            case 24: s = { 470.0f, 0.55f,  90.0f, 0.50f,  0.25f }; return true;     // v0.2 Matamp-style 120 W: eight-diode bridge, 100 uF pair, choke, 33k and 33k dropping stages with 16 uF (read from the schematics); voltages are not printed, so B+ and peak current are estimated
            case 27: s = { 448.0f, 0.35f,  60.0f, 0.185f, 0.20f }; return true;     // v0.2 Marque Five: same supply as the Mark-series amp (silicon bridge, 220 uF, choke, 5.6k x 30 uF)
            case 26: s = { 480.0f, 0.55f,  80.0f, 0.20f,  0.15f }; return true;     // v0.2 Hi / Lo input amp: 100 W, four EL34s, silicon rectifier; estimated (no supply data), the stiff modern 100 W figures
            case 28: s = { 350.0f, 0.24f, 140.0f, 0.16f, 0.30f }; return true;     // v0.2 Dual Thirty (read from the two-channel 30 W EL84 combo's supply sheet: rectifier tube, 33 uF reservoirs with a choke, 4.7k and 22k dropping stages with 22 and 33 uF; 350 V at the reservoir, 308 to 338 V on the plates, measured on the sheet. Peak current, source resistance and transformer saturation are estimates)
            case 30: s = { 420.0f, 0.20f, 110.0f, 0.12f, 0.30f }; return true;     // v0.2 Cedar Twelve: small blackface-style combo, two 6V6s, silicon rectifier; peak current and source resistance estimated (a small supply droops sooner than the big amp's)
            case 31: s = { 420.0f, 0.20f, 110.0f, 0.12f, 0.30f }; return true;     // v0.2 Triple Blend part one (the small combo's supply)
            case 33: s = { 330.0f, 0.18f, 120.0f, 0.15f, 0.30f }; return true;     // b14 Brit Eighteen: 18 W EL84 combo, rectifier tube; estimated (no schematic read)
            default: return false;
        }
    }
}
