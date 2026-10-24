#pragma once
// Circuit models (v77). Shared building blocks for the "Circuit" modes of the Screamer and Tight overdrives and the Wall fuzz.
// They follow the published schematics of the kinds of pedal named in MODELED_GEAR.md: the component values are the
// ones on those schematics, and the maths is the circuit's own (an op-amp stage with diodes in its feedback loop, RC
// poles, a passive tone network). Nothing here is a copy of anybody's code.
//
// Units: 1.0 in the plugin is treated as 1 volt inside these models.
#include <algorithm>
#include <cmath>

namespace Circuit
{
    // ---- pots -------------------------------------------------------------------------------------------------------
    // Fraction of the full resistance of a log ("audio") taper pot at knob position k (0..1): about 9% at the middle.
    inline float audioTaper (float k)
    {
        k = std::min (1.0f, std::max (0.0f, k));
        return (std::pow (10.0f, 2.0f * k) - 1.0f) / 99.0f;
    }

    // ---- analog polynomial -> digital biquad (bilinear transform, no pre-warp) -----------------------------------------
    //   H(s) = (n2 s^2 + n1 s + n0) / (d2 s^2 + d1 s + d0)
    struct BiquadCoeffs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

    inline BiquadCoeffs bilinear (double fs, double n2, double n1, double n0, double d2, double d1, double d0)
    {
        const double c = 2.0 * fs, c2 = c * c;
        const double B0 = n2 * c2 + n1 * c + n0,  B1 = 2.0 * (n0 - n2 * c2),  B2 = n2 * c2 - n1 * c + n0;
        const double A0 = d2 * c2 + d1 * c + d0,  A1 = 2.0 * (d0 - d2 * c2),  A2 = d2 * c2 - d1 * c + d0;
        BiquadCoeffs o;
        o.b0 = B0 / A0;  o.b1 = B1 / A0;  o.b2 = B2 / A0;  o.a1 = A1 / A0;  o.a2 = A2 / A0;
        return o;
    }

    // A small transposed direct-form II biquad (for code that runs sample by sample).
    struct Bq
    {
        BiquadCoeffs c;
        double z1 = 0, z2 = 0;
        void set (const BiquadCoeffs& k) { c = k; }
        void reset() { z1 = z2 = 0; }
        inline float run (float x)
        {
            const double y = c.b0 * x + z1;
            z1 = c.b1 * x - c.a1 * y + z2;
            z2 = c.b2 * x - c.a2 * y;
            return (float) y;
        }
    };

    // First-order low-pass / high-pass at fc (Hz), unity passband.
    inline BiquadCoeffs lowPass1 (double fs, double fc)  { const double t = 1.0 / (2.0 * 3.14159265358979 * fc); return bilinear (fs, 0, 0, 1, 0, t, 1); }
    inline BiquadCoeffs highPass1 (double fs, double fc) { const double t = 1.0 / (2.0 * 3.14159265358979 * fc); return bilinear (fs, 0, t, 0, 0, t, 1); }

    // ---- diodes -------------------------------------------------------------------------------------------------------
    struct DiodeSpec { float isat, nVt; };           // saturation current (A), emission coefficient x thermal voltage (V)
    inline DiodeSpec diodeSilicon()   { return { 2.52e-9f, 0.0453f }; }     // 1N4148 / 1N914
    inline DiodeSpec diodeLed()       { return { 1.4e-13f, 0.0750f }; }     // red LED, about 1.7 V at 1 mA
    inline DiodeSpec diodeGermanium() { return { 2.6e-7f,  0.0340f }; }     // about 0.28 V at 1 mA
    inline DiodeSpec diodeSiliconPair() { return { 2.52e-9f, 0.0906f }; }   // two silicon diodes in series

    // The clipper of an op-amp (or transistor) stage with an antiparallel diode pair across its feedback resistor Rf:
    //     y + 2 Is Rf sinh (y / nVt) = x      (x is what the stage would put out with no diodes, y what it really does)
    // Solved exactly (Newton), with first-order antiderivative anti-aliasing so it can run at the normal sample rate.
    class DiodeLimiter
    {
    public:
        void setup (float rfOhms, const DiodeSpec& pos, const DiodeSpec& neg)
        {
            side[0].a = 2.0f * pos.isat * rfOhms;  side[0].b = pos.nVt;
            side[1].a = 2.0f * neg.isat * rfOhms;  side[1].b = neg.nVt;
        }
        void reset() { xPrev = 0.0f; fPrev = 0.0f; yPrev = 0.0f; }

        // Memoryless curve (no anti-aliasing).
        float curve (float x) const
        {
            const bool neg = x < 0.0f;
            const float y = solve (side[neg ? 1 : 0], std::fabs (x));
            return neg ? -y : y;
        }

        float process (float x)
        {
            const bool neg = x < 0.0f;
            const Side& s = side[neg ? 1 : 0];
            const float ax = std::fabs (x);
            const float y  = solve (s, ax);
            const float f  = antiderivative (s, ax, y);
            float out;
            const float dx = x - xPrev;
            if (std::fabs (dx) > 1.0e-4f)
                out = (f - fPrev) / dx;
            else
                out = curve (0.5f * (x + xPrev));
            xPrev = x;  fPrev = f;
            return out;
        }

    private:
        struct Side { float a = 1.0e-3f, b = 0.0453f; };
        Side side[2];
        float xPrev = 0.0f, fPrev = 0.0f, yPrev = 0.0f;

        // y such that y + a sinh (y/b) = x, for x >= 0
        static float solve (const Side& s, float x)
        {
            if (x <= 0.0f) return 0.0f;
            float y = std::min (x, s.b * std::asinh (x / s.a));      // at or above the root, so Newton comes down monotonically
            for (int i = 0; i < 4; ++i)
            {
                const float e = std::exp (std::min (y / s.b, 60.0f)), ei = 1.0f / e;
                const float f  = y + s.a * 0.5f * (e - ei) - x;
                const float df = 1.0f + (s.a / s.b) * 0.5f * (e + ei);
                y -= f / df;
                y = std::min (x, std::max (0.0f, y));
            }
            return y;
        }
        // F(x) = integral of y dx from 0 (x >= 0, for the side's own curve; the negative half mirrors it)
        static float antiderivative (const Side& s, float x, float y)
        {
            const float e = std::exp (std::min (y / s.b, 60.0f)), ei = 1.0f / e;
            const float sh = 0.5f * (e - ei), ch = 0.5f * (e + ei);
            return 0.5f * y * y + s.a * y * sh - s.a * s.b * ch + s.a * s.b;
        }
    };

    // Op-amp output limit (rails at +/-R volts around the bias point), soft-knee tanh, with anti-aliasing.
    class RailLimiter
    {
    public:
        void setRail (float r) { rail = r; }
        void reset() { xPrev = 0.0; fPrev = 0.0; }
        float curve (float x) const { return rail * std::tanh (x / rail); }
        // v0.2: the antiderivative and the divided difference are done in double precision. In float, R^2 ln cosh(x/R) loses its
        // small-signal accuracy (R^2 is about 20 and the three terms nearly cancel), and dividing that error by a small step
        // turned into hiss and fizz on quiet, slow signals (the Clean Boost, Op-Amp Distortion, Stack Drive and Blues Driver).
        float process (float x)
        {
            const double xd = (double) x, r = (double) rail;
            // (no upper clamp on u: a clamp at 30 made the antiderivative flat above 30 rails = 135 V, and the second stage of the Stack Drive
            //  hands this limiter several hundred volts at full gain, where the divided difference then collapsed to nothing. exp (-2u) just underflows to 0.)
            const double u = std::fabs (xd) / r;
            const double f = r * r * (u + std::log1p (std::exp (-2.0 * u)) - 0.69314718055994531);   // R^2 ln cosh (x/R)
            float out;
            const double dx = xd - xPrev;
            if (std::fabs (dx) > 1.0e-6) out = (float) ((f - fPrev) / dx);
            else                         out = curve (0.5f * (x + (float) xPrev));
            xPrev = xd;  fPrev = f;
            return out;
        }
    private:
        float rail = 4.5f;
        double xPrev = 0.0, fPrev = 0.0;
    };

    // ---- the Screamer / Tight overdrive gain stage: coefficients ---------------------------------------------------
    // A non-inverting op-amp stage: out = in + Vf, where Vf = limiter (in * Zf / Z1) and
    //   Z1 = R1 + 1/(s C1)            (the "attack" / bass corner)
    //   Zf = (Rfix + Rdrive) with C4 across it (the top-end pole), the diodes across both.
    // The linear part of Zf / Z1 is a band-pass with unity peak gain times Rf / R1:
    //     (s / wp) / ((1 + s / wp) (1 + s / wc))         wp = 1 / (R1 C1), wc = 1 / (Rf C4)
    // The op-amp's gain-bandwidth adds one more pole at GBW / (1 + Rf / R1).
    struct GainStage
    {
        float peakGain = 1.0f;       // Rf / R1
        BiquadCoeffs bandPass;       // HP pole x LP pole, unity peak
        BiquadCoeffs gbwAndEdge;     // GBW pole x optional edge shelf
        float rf = 0.0f;
    };

    inline GainStage designGainStage (double fs, float r1, float c1, float rf, float c4, float gbwHz, float edgeDb)
    {
        GainStage g;
        g.rf = rf;
        g.peakGain = rf / r1;
        const double tp = (double) r1 * c1;                    // 1 / wp
        const double tc = (double) rf * c4;                    // 1 / wc
        // (s tp) / ((1 + s tp)(1 + s tc))
        g.bandPass = bilinear (fs, 0, tp, 0, tp * tc, tp + tc, 1.0);
        const double fgbw = std::min ((double) gbwHz / (1.0 + g.peakGain), 0.45 * fs);
        const double tg = 1.0 / (2.0 * 3.14159265358979 * fgbw);
        if (std::fabs (edgeDb) < 0.01f)
            g.gbwAndEdge = bilinear (fs, 0, 0, 1, 0, tg, 1);
        else
        {
            // first-order high shelf of edgeDb around 3 kHz, in front of the GBW pole
            const double ge = std::pow (10.0, edgeDb / 20.0), w0 = 2.0 * 3.14159265358979 * 3000.0;
            const double tz = 1.0 / (w0 / std::sqrt (ge)), tpp = 1.0 / (w0 * std::sqrt (ge));
            g.gbwAndEdge = bilinear (fs, 0, tz, 1, tpp * tg, tpp + tg, 1);
        }
        return g;
    }

    // The tone / bright stage that follows: a passive low-pass (R1 C1) then an op-amp stage with gain
    //     1 + Rfb / (R2 + Rpot + 1 / (s C2))
    // = (1 + s C2 (R2 + Rpot + Rfb)) / (1 + s C2 (R2 + Rpot)).      Returned as one biquad.
    inline BiquadCoeffs designToneStage (double fs, float rLp, float cLp, float rFb, float r2, float c2, float rPot)
    {
        const double tl = (double) rLp * cLp;
        const double tz = (double) c2 * (r2 + rPot + rFb), tp = (double) c2 * (r2 + rPot);
        return bilinear (fs, 0, tz, 1.0, tp * tl, tp + tl, 1.0);
    }

    // ---- the fuzz's passive tone network (exact nodal solution, trapezoidal capacitors) ----------------------------------
    // Source Vs through Rs into node A.  LP branch: R8 (A to B) and C8 (B to ground).  HP branch: C9 (A to C) and R5 (C to ground).
    // The tone pot Rp lies between B and C; its wiper W (fraction t from B) feeds the load RL.
    class MuffToneNetwork
    {
    public:
        void prepare (double sampleRate) { fs = sampleRate; gc8 = 2.0f * c8 * (float) fs; gc9 = 2.0f * c9 * (float) fs; reset(); setTone (0.5f); }
        void reset() { ih8 = ih9 = 0.0f; }
        // Component values: source resistance, LP-branch resistor and capacitor, HP-branch capacitor and resistor, tone pot, load. Call before prepare.
        void setValues (float rs_, float r8_, float c8_, float c9_, float r5_, float rp_, float rl_)
        { rs = rs_; r8 = r8_; c8 = c8_; c9 = c9_; r5 = r5_; rp = rp_; rl = rl_; }
        void setTone (float t)
        {
            t = std::min (0.999f, std::max (0.001f, t));
            const float rbw = t * rp, rwc = (1.0f - t) * rp;
            const float gS = 1.0f / rs, gAB = 1.0f / r8, gCg = 1.0f / r5, gBW = 1.0f / rbw, gWC = 1.0f / rwc, gL = 1.0f / rl;
            // nodes: 0 A, 1 B, 2 C, 3 W
            float Y[4][8] = {};
            auto add = [&] (int i, int j, float g) { Y[i][i] += g; if (j >= 0) { Y[j][j] += g; Y[i][j] -= g; Y[j][i] -= g; } };
            add (0, -1, gS);  add (0, 1, gAB);  add (1, -1, gc8);  add (0, 2, gc9);  add (2, -1, gCg);  add (1, 3, gBW);  add (3, 2, gWC);  add (3, -1, gL);
            gSrc = gS;
            for (int i = 0; i < 4; ++i) Y[i][4 + i] = 1.0f;       // Gauss-Jordan inverse
            for (int c = 0; c < 4; ++c)
            {
                int p = c;  for (int r = c + 1; r < 4; ++r) if (std::fabs (Y[r][c]) > std::fabs (Y[p][c])) p = r;
                for (int k = 0; k < 8; ++k) std::swap (Y[c][k], Y[p][k]);
                const float d = Y[c][c];  for (int k = 0; k < 8; ++k) Y[c][k] /= d;
                for (int r = 0; r < 4; ++r) if (r != c) { const float f = Y[r][c]; for (int k = 0; k < 8; ++k) Y[r][k] -= f * Y[c][k]; }
            }
            for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) inv[i][j] = Y[i][4 + j];
        }
        float process (float vs)
        {
            const float b0 = gSrc * vs + ih9, b1 = ih8, b2 = -ih9;
            const float vA = inv[0][0] * b0 + inv[0][1] * b1 + inv[0][2] * b2;
            const float vB = inv[1][0] * b0 + inv[1][1] * b1 + inv[1][2] * b2;
            const float vC = inv[2][0] * b0 + inv[2][1] * b1 + inv[2][2] * b2;
            const float vW = inv[3][0] * b0 + inv[3][1] * b1 + inv[3][2] * b2;
            ih8 = 2.0f * gc8 * vB - ih8;
            ih9 = 2.0f * gc9 * (vA - vC) - ih9;
            return vW;
        }
    private:
        double fs = 48000.0;
        float rs = 10.0e3f, r8 = 39.0e3f, c8 = 10.0e-9f, c9 = 4.0e-9f, r5 = 22.0e3f, rp = 100.0e3f, rl = 78.0e3f;
        float gc8 = 0, gc9 = 0, gSrc = 0, ih8 = 0, ih9 = 0;
        float inv[4][4] {};
    };

    // ---- the Wall fuzz, circuit version: input booster, two diode-limited gain stages, passive tone, output stage ----------
    class MuffCircuit
    {
    public:
        struct Params { float sustain = 0.6f, tone = 0.5f; int diode = 0; bool opAmp = false, tonePass = false; int variant = 0; };

        // Variants: 0 = the first schematic (1 uF coupling caps, 470 pF feedback caps); 1 and 2 = the later builds: 0.1 uF coupling,
        // 0.047 uF in series with the diodes, 390 ohm emitter resistors, 12k collectors, a 20k tone resistor, and 500 pF (1) or
        // 470 pF (2) feedback capacitors. Gains and poles for 1 and 2 come from a transistor small-signal model calibrated so that
        // it reproduces the published figures of variant 0 (16.7, 23.4 and 25.2 dB; 1.2 kHz, 1.78 kHz and 1.17 kHz).
        struct Var
        {
            float boost, boostLpHz, boostHp1Hz, boostHp2Hz, g1, g2, lp1Hz, lp2Hz, outGain, capSeries, rOut;
            float r8;
            // v95: tone-network and Sustain-pot values (the defaults are the ones the first three variants always used)
            float trs = 0.0f, tc8 = 10.0e-9f, tc9 = 4.0e-9f, tr5 = 22.0e3f, trp = 100.0e3f, trl = 78.0e3f;
            float rPot = 100.0e3f, rBot = 1.0e3f, rLoad = 18.9e3f, rb = 8.9e3f;
            float hpAHz = 55.0f, hpBHz = 94.0f;      // v0.1: the corners ahead of the first and second clipper (the same for variants 0 to 3)
        };
        static Var variantData (int v)
        {
            if (v == 1) return { 6.70f, 1130.0f, 38.0f, 16.0f, 11.74f, 11.63f, 2016.0f, 1638.0f, 3.70f, 47.0e-9f, 11.7e3f, 20.0e3f };
            if (v == 2) return { 6.70f, 1200.0f, 38.0f, 16.0f, 11.74f, 11.63f, 2151.0f, 1748.0f, 3.70f, 47.0e-9f, 11.7e3f, 20.0e3f };
            // v95, variant 3: the modern Muff-type build from the Elk Device board (BOM values). The booster is the same as variant 2.
            // Clipping stages: 82k / 390k bases, 560 pF feedback, collectors 12k and 18k, emitters 150R and 100R, so more gain than variant 2;
            // gains from (Rc in parallel with the next resistor) / (Re + 79 ohm), the 79 ohm calibrated on variant 2; the last one is capped at 45.
            // The feedback pole (390k x 560 pF) is the same time constant as variant 2's (470k x 470 pF), so I kept variant 2's poles, the
            // second one lowered a little for the higher gain. Tone network: 39k / 10 nF low-pass branch, 33 nF / 39k high-pass branch, 50k pot.
            if (v == 3) return { 6.70f, 1200.0f, 38.0f, 16.0f, 21.3f, 45.0f, 2151.0f, 1500.0f, 3.70f, 47.0e-9f, 11.7e3f, 39.0e3f,
                                 18.0e3f, 10.0e-9f, 33.0e-9f, 39.0e3f, 50.0e3f, 42.0e3f,
                                 50.0e3f, 820.0f, 12.5e3f, 4.3e3f };
            // v0.1, variant 4: the early large-enclosure build with the "47" parts (schematic traced from nine units, sent by you).
            // Same booster and two clipping stages as variant 0 (12k collectors, 100 ohm emitters, 470k / 470 pF feedback, 1N914 diodes), so I kept
            // variant 0's gains and poles. What differs: coupling caps 0.47 uF in front of and after the booster and after the Sustain pot, 0.047 uF
            // between the two clippers, a 33k / 12 nF low-pass branch and a 33k shunt on the tone network (4 nF high-pass branch), and 12k at the
            // tone source. The two diode-loop capacitors read 0.47 uF and 0.047 uF; the model has one value, so I used their geometric mean (0.15 uF).
            // The input corners scale with variant 0's by the ratio of the capacitors (1 uF to 0.47 uF).
            if (v == 4) return { 6.80f, 1200.0f, 8.5f, 2.1f, 14.8f, 18.2f, 1780.0f, 1170.0f, 4.5f, 0.15e-6f, 9.8e3f, 33.0e3f,
                                 12.0e3f, 12.0e-9f, 4.0e-9f, 33.0e3f, 100.0e3f, 78.0e3f,
                                 100.0e3f, 1.0e3f, 18.9e3f, 8.9e3f, 55.0f, 138.0f };      // second clipper: 0.047 uF with 7.5k and the 100k bias resistor, about 138 Hz
            return { 6.80f, 1200.0f, 4.0f, 1.0f, 14.8f, 18.2f, 1780.0f, 1170.0f, 4.5f, 1.0e-6f, 9.8e3f, 39.0e3f };
        }

        void prepare (double sampleRate)
        {
            fs = sampleRate;
            curVariant = -1;
            applyVariant (par.variant);
            reset();
            lastTone = -1.0f;
        }
        void applyVariant (int v)
        {
            if (v == curVariant) return;
            curVariant = v;
            var = variantData (v);
            boosterLp.set (lowPass1 (fs, var.boostLpHz));
            hp1.set (highPass1 (fs, var.hpAHz));   lp1.set (lowPass1 (fs, var.lp1Hz));
            hp2.set (highPass1 (fs, var.hpBHz));   lp2.set (lowPass1 (fs, var.lp2Hz));
            inHp.set (highPass1 (fs, var.boostHp1Hz));
            inHp2.set (highPass1 (fs, var.boostHp2Hz));
            outHp.set (highPass1 (fs, 10.0));
            tone.setValues (var.trs > 0.0f ? var.trs : 10.0e3f * (v == 0 ? 1.0f : 1.2f), var.r8, var.tc8, var.tc9, var.tr5, var.trp, var.trl);
            tone.prepare (fs);
            lastTone = -1.0f;
        }
        void reset()
        {
            boosterLp.reset(); hp1.reset(); lp1.reset(); hp2.reset(); lp2.reset(); inHp.reset(); inHp2.reset(); outHp.reset();
            tone.reset(); vc1 = vc2 = 0.0f;
        }
        void configure (const Params& p)
        {
            par = p;
            applyVariant (p.variant);
            if (std::fabs (p.tone - lastTone) > 1.0e-4f) { tone.setTone (p.tone); lastTone = p.tone; }
            DiodeSpec d = p.diode == 1 ? diodeLed() : p.diode == 2 ? diodeGermanium() : diodeSilicon();
            // Rf of both clipping stages is 470k.  The diode pair also has C6 / C7 (1 uF) in series with it.
            const float rfEff = 470.0e3f + 1.0f / ((float) fs * var.capSeries);
            lim[0] = { d.isat, d.nVt, rfEff };
            sustainGain = sustainLaw (p.sustain, var.rOut, var.rPot, var.rBot, var.rLoad, var.rb);
        }
        float process (float x)
        {
            // input booster: a common-emitter stage, gain 16.7 dB, a pole at 1.2 kHz, and a collector that runs out of room
            // sooner on the way up (quiescent about 7 V of the 9 V supply) than on the way down.
            float v = inHp2.run (inHp.run (x));
            v = boosterLp.run (-var.boost * v);
            v = v > 0.0f ? 1.8f * std::tanh (v / 1.8f) : 6.8f * std::tanh (v / 6.8f);

            // Sustain: the pot (loaded by the stage after it) sets how much reaches the first clipper
            float s1 = lp1.run (hp1.run (v)) * (var.g1 * sustainGain);
            s1 = stage (s1, vc1);
            // the second clipper: fixed gain 25 dB
            float s2 = lp2.run (hp2.run (s1)) * -var.g2;
            s2 = stage (s2, vc2);

            float out;
            if (par.tonePass) out = 0.25f * s2;
            else              out = tone.process (s2);
            return var.outGain * outHp.run (out);                 // the output stage: 13 dB, flat
        }

        // How much of the booster's signal gets into the first clipper as the Sustain pot (100k linear, with a 1k at its bottom end)
        // goes from 0 to 1, loaded by the stage after it. 1.0 at the top of the travel.
        static float sustainLaw (float p, float rOut = 9.8e3f, float rPot = 100.0e3f, float r23 = 1.0e3f, float rLoad = 18.9e3f, float rb = 8.9e3f)
        {
            p = std::min (1.0f, std::max (0.0f, p));
            auto level = [=] (float q)
            {
                const float rup = (1.0f - q) * rPot + 1.0e-3f, rlow = q * rPot + r23;
                const float rw = rlow * rLoad / (rlow + rLoad);
                const float vw = rw / (rOut + rup + rw);
                return vw * rb / rLoad;
            };
            return level (p) / level (1.0f);
        }

    private:
        double fs = 48000.0;
        Params par;
        Bq boosterLp, hp1, lp1, hp2, lp2, inHp, inHp2, outHp;
        int curVariant = -1;  Var var = variantData (0);
        MuffToneNetwork tone;
        float lastTone = -1.0f, sustainGain = 1.0f;
        float vc1 = 0.0f, vc2 = 0.0f;                     // the voltages across the 1 uF capacitors in the diode loops
        struct Lim { float isat, nVt, rf; } lim[1] {};

        // One diode-limited stage with the 1 uF capacitor in series with the diode pair (a slow DC shift when the clipping is lopsided).
        //   y = vd + vc,   x - vc = vd + (Rf + T/C) * 2 Is sinh (vd / nVt),   vc += (T/C) * 2 Is sinh (vd / nVt)
        float stage (float xin, float& vc) const
        {
            float x = xin;
            if (par.opAmp) x = 4.5f * std::tanh (x / 4.5f);
            const float a = 2.0f * lim[0].isat * lim[0].rf, b = lim[0].nVt;
            const float target = x - vc;
            const float ax = std::fabs (target);
            float y = std::min (ax, b * std::asinh (ax / a));
            for (int i = 0; i < 4; ++i)
            {
                const float e = std::exp (std::min (y / b, 60.0f)), ei = 1.0f / e;
                const float f  = y + a * 0.5f * (e - ei) - ax;
                const float df = 1.0f + (a / b) * 0.5f * (e + ei);
                y -= f / df;
                y = std::min (ax, std::max (0.0f, y));
            }
            const float vd = target < 0.0f ? -y : y;
            const float id = 2.0f * lim[0].isat * std::sinh (std::min (std::fabs (vd) / b, 60.0f)) * (vd < 0.0f ? -1.0f : 1.0f);
            vc += id / ((float) fs * var.capSeries);              // T / C per sample
            vc *= 1.0f - 1.0f / (100.0e3f * var.capSeries * (float) fs);                // the cap's DC leaks away slowly (about 0.1 s, the base resistor)
            return vd + vc;
        }
    };

    // ---- the op-amp fuzz: two op-amp gain stages, a Sallen-Key low-pass with the sustain pot on its output, a 741 clipper with
    //      three diodes each way, a Muff-style tone network (with a bypass) --------------------------------------------------
    //   IC1.1: inverting, gain -R4/R1 = -5.9, with a 19 Hz input high-pass (150 nF, 56k).
    //   IC1.2: R5 10k, R6 47k, C3 4.7 nF (from the output to the R5/R6 junction), C4 10 nF.  Non-inverting gain K = 1 + R7 (560k) / R8 (62k) = 10,
    //          a low-pass at about 1 kHz with Q about 1.  The 10k sustain pot is a divider on its output, loaded by the next stage's 8.2k.
    //   IC2.1: inverting, 470k / 8.2k = 57 with 150 pF across 470k, 3 + 3 silicon diodes across the 470k.
    class OpAmpMuffCircuit
    {
    public:
        struct Params { float sustain = 0.6f, tone = 0.5f; int diode = 0; bool tonePass = false; };

        void prepare (double sampleRate)
        {
            fs = sampleRate;
            inHp.set (highPass1 (fs, 1.0 / (2.0 * 3.14159265358979 * 56.0e3 * 150.0e-9)));
            c6Hp.set (highPass1 (fs, 1.0 / (2.0 * 3.14159265358979 * 8.2e3 * 4.7e-6)));
            bypassHp.set (highPass1 (fs, 1.0 / (2.0 * 3.14159265358979 * 94.0e3 * 150.0e-9)));
            gs = designGainStage (fs, 8200.0f, 4.7e-6f, 470.0e3f, 150.0e-12f, 1.0e6f, 0.0f);
            stageBp.set (gs.bandPass);  stageGbw.set (gs.gbwAndEdge);
            tone.setValues (150.0f, 5.6e3f, 120.0e-9f, 100.0e-9f, 1.2e3f, 10.0e3f, 100.0e3f);
            tone.prepare (fs);
            rail1.setRail (3.5f);
            reset();
            lastSus = lastTone = -1.0f;
        }
        void reset()
        {
            inHp.reset(); c6Hp.reset(); bypassHp.reset(); sk.reset(); stageBp.reset(); stageGbw.reset(); tone.reset(); rail1.reset(); diode.reset();
        }
        void configure (const Params& p)
        {
            par = p;
            if (std::fabs (p.tone - lastTone) > 1.0e-4f) { tone.setTone (p.tone); lastTone = p.tone; }
            if (std::fabs (p.sustain - lastSus) > 1.0e-4f)
            {
                lastSus = p.sustain;
                const double t = std::min (1.0, std::max (0.0, (double) p.sustain));
                const double R7 = 560.0e3, R8 = 62.0e3, R9 = 47.0, Rpot = 10.0e3, RL = 8.2e3;
                const double K = 1.0 + R7 / R8;                                  // the pot is not in the feedback: it is a divider on the output
                const double Ra = std::max (1.0, (1.0 - t) * Rpot), Rb = t * Rpot + R9;
                const double rb = Rb * RL / (Rb + RL);                             // below the wiper, in parallel with the next stage's input resistor
                wiperGain = (float) (K * rb / (Ra + rb));
                const double R5 = 10.0e3, R6 = 47.0e3, C3 = 4.7e-9, C4 = 10.0e-9;
                sk.set (bilinear (fs, 0, 0, K, R5 * R6 * C3 * C4, R6 * C4 + R5 * C4 + R5 * C3 * (1.0 - K), 1.0));
            }
            DiodeSpec d = p.diode == 1 ? diodeLed() : p.diode == 2 ? diodeGermanium() : diodeSilicon();
            d.nVt *= 3.0f;                                  // three in series each way
            diode.setup (470.0e3f, d, d);
        }
        float process (float x)
        {
            float v = rail1.process (-5.89f * inHp.run (x));
            v = sk.run (v) * wiperGain;
            v = c6Hp.run (v);
            v = stageBp.run (v);
            v = stageGbw.run (v) * gs.peakGain;
            v = diode.process (v);
            if (par.tonePass) return 0.5f * bypassHp.run (v);
            return tone.process (v);
        }

    private:
        double fs = 48000.0;
        Params par;
        Bq inHp, c6Hp, bypassHp, sk, stageBp, stageGbw;
        GainStage gs;
        MuffToneNetwork tone;
        RailLimiter rail1;
        DiodeLimiter diode;
        float wiperGain = 1.0f, lastSus = -1.0f, lastTone = -1.0f;
    };

    // ---- the Distortion pedal, circuit version (v91) -------------------------------------------------------------------
    // Signal path, with the values from the published analysis of a Japanese compact distortion pedal:
    //   transistor booster (gain 56 = 35 dB, 33 Hz input high-pass, soft and lopsided limit: +3.2 V up, -5.6 V down around its 5.8 V collector),
    //   23 Hz coupling high-pass, op-amp stage with gain 1 + Rdist / 4.7k (Rdist 100k log pot, 0.47 uF in the 4.7k leg = 72 Hz corner at full
    //   gain, 100 pF across the pot), then 2.2k into a 10 nF capacitor with antiparallel diodes to ground (the clipper, 7.2 kHz before it clips),
    //   then the passive tone network: 6.8k + 0.1 uF low-pass branch, 22 nF + 6.8k high-pass branch, 20k pot between them.
    // Not modelled: the booster's own high-frequency roll-off (the analysis gives no figure), the input and output emitter followers, and the loading of the
    // clipper node by the tone network. The op-amp's bandwidth is taken as 1 MHz (the parts list names several different op-amps).
    class Ds1Circuit
    {
    public:
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            boostHp.set (highPass1 (fs, 33.0));
            coupleHp.set (highPass1 (fs, 23.0));
            rail.setRail (4.5f);
            tone.setValues (1.0f, 6800.0f, 0.1e-6f, 22.0e-9f, 6800.0f, 20000.0f, 100000.0f);
            tone.prepare (fs);
            reset();
        }
        void reset() { boostHp.reset(); coupleHp.reset(); bp.reset(); ge.reset(); rail.reset(); tone.reset(); vNode = 0.0f; xPrev = 0.0f; }
        // gain: the Distortion knob (0..1), toneKnob: 0 = dark (low-pass side), 1 = bright
        void setup (float gain, float toneKnob, const DiodeSpec& pos, const DiodeSpec& neg)
        {
            const float rv = 100000.0f * audioTaper (std::min (1.0f, std::max (0.0f, gain)));
            const auto gs = designGainStage (fs, 4700.0f, 0.47e-6f, std::max (rv, 1.0f), 100.0e-12f, 1.0e6f, 0.0f);
            bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = rv / 4700.0f;
            dp = pos;  dn = neg;
            tone.setTone (toneKnob);
        }
        float process (float x)
        {
            float v = 56.0f * boostHp.run (x);
            v = v > 0.0f ? 3.2f * std::tanh (v / 3.2f) : 5.6f * std::tanh (v / 5.6f);
            v = coupleHp.run (v);
            float w = v + ge.run (bp.run (v)) * peak;
            w = rail.process (w);
            // 2.2k into 10 nF with the diode pair: backward Euler, four sub-steps per sample, Newton on the node voltage
            const float r = 2200.0f, c = 10.0e-9f, k = c * 4.0f * (float) fs;
            for (int s = 1; s <= 4; ++s)
            {
                const float vin = xPrev + (w - xPrev) * (float) s * 0.25f;
                float u = vNode;
                for (int it = 0; it < 4; ++it)
                {
                    const float ep = std::exp (std::min (u / dp.nVt, 40.0f)), en = std::exp (std::min (-u / dn.nVt, 40.0f));
                    const float id  = dp.isat * (ep - 1.0f) - dn.isat * (en - 1.0f);
                    const float did = dp.isat * ep / dp.nVt + dn.isat * en / dn.nVt;
                    const float f  = k * (u - vNode) + id + (u - vin) / r;
                    u -= f / (k + did + 1.0f / r);
                }
                vNode = u;
            }
            xPrev = w;
            return tone.process (vNode);
        }

    private:
        double fs = 48000.0;
        Bq boostHp, coupleHp, bp, ge;
        RailLimiter rail;
        MuffToneNetwork tone;
        DiodeSpec dp = diodeSilicon(), dn = diodeSilicon();
        float peak = 1.0f, vNode = 0.0f, xPrev = 0.0f;
    };

    // ---- the Open overdrive as a circuit (b20): a Fulltone OCD-style pedal, read from a published schematic -------------------
    // Stage 1  non-inverting op-amp (TL082, 3 MHz): input 22 nF into 10k and 470k to Vref; leg 2.2k + 100 nF (723 Hz corner); feedback
    //          = gain pot (500k) + 18k, with 220 pF across it. Gain 1 + Rf / 2.2k, up to about 47 dB.
    // Clipper  10k from stage 1 into 10 nF to Vref (1.6 kHz low-pass), then a MOSFET used as a diode (2N7000, gate and drain at Vref,
    //          source at the node: conducts when the node is more than its threshold below Vref) on the negative side, and a
    //          germanium diode in series with a second MOSFET's body diode on the positive side. 10k and 220k (to Vref) follow.
    // Stage 3  non-inverting op-amp: leg 39k + 100 nF (41 Hz), feedback 150k with 220 pF across it (4.8 kHz), gain 4.85.
    // Tone     33k (HP position: 33k in parallel with 22k) into 47 nF in series with the 10k tone pot to ground:
    //          H = (1 + s Rt C) / (1 + s (Rs + Rt) C).
    // Estimated: the exact wiring of the two MOSFETs (the schematic and the written analysis disagree), the 2N7000 threshold (2.1 V,
    // datasheet range 0.8 to 3) and transconductance, the op-amp swing (+/- 4 V), the gain pot's taper (audio). Not modelled: the
    // true/buffered bypass switch, the volume pot's loading, the supply's 9 V to 18 V range.
    class OcdCircuit
    {
    public:
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            inHp.set (highPass1 (fs, 1.0 / (6.2831853 * 480000.0 * 22.0e-9)));
            rail1.setRail (4.0f);  rail3.setRail (4.0f);
            reset();
        }
        void reset() { inHp.reset(); bp1.reset(); ge1.reset(); bp3.reset(); ge3.reset(); toneBq.reset(); rail1.reset(); rail3.reset(); u = 0.0f; xPrev = 0.0f; }
        // gain: 0..1 (the gain pot), tone: 0..1 (the tone pot, 1 = brightest), hp: the HP / LP switch (HP = R14 in parallel with R15)
        void setup (float gain, float tone, bool hp, float vth = 2.1f)
        {
            const float rpot = 500000.0f * audioTaper (std::min (1.0f, std::max (0.0f, gain)));
            const float rf = rpot + 18000.0f;
            const auto g1 = designGainStage (fs, 2200.0f, 100.0e-9f, rf, 220.0e-12f, 3.0e6f, 0.0f);
            bp1.set (g1.bandPass);  ge1.set (g1.gbwAndEdge);  peak1 = g1.peakGain;
            const auto g3 = designGainStage (fs, 39000.0f, 100.0e-9f, 150000.0f, 220.0e-12f, 3.0e6f, 0.0f);
            bp3.set (g3.bandPass);  ge3.set (g3.gbwAndEdge);  peak3 = g3.peakGain;
            const double rs = hp ? 33000.0 * 22000.0 / 55000.0 : 33000.0;
            const double rt = 10000.0 * std::min (1.0f, std::max (0.0f, tone)), c = 47.0e-9;
            toneBq.set (bilinear (fs, 0, rt * c, 1.0, 0, (rs + rt) * c, 1.0));
            this->vth = vth;
        }
        float process (float x)
        {
            float v = inHp.run (x);
            float w = v + ge1.run (bp1.run (v)) * peak1;
            w = rail1.process (w);
            // the clipper node (volts from Vref): 10k in from stage 1, 10 nF to Vref, the MOSFET and diodes, about 230k out.
            // Backward Euler with four sub-steps per sample, Newton on the node voltage.
            const float r = 10000.0f, c = 10.0e-9f, k = c * 4.0f * (float) fs, rl = 230000.0f;
            for (int s = 1; s <= 4; ++s)
            {
                const float vin = xPrev + (w - xPrev) * (float) s * 0.25f;
                float uu = u;
                for (int it = 0; it < 6; ++it)
                {
                    // negative side: the diode-connected MOSFET (smooth square law with a sub-threshold tail)
                    const float vgs = -uu;
                    const float a = (vgs - vth) / (2.0f * mNvt);
                    const float sp = a > 30.0f ? a : std::log1p (std::exp (a));
                    const float sg = a > 30.0f ? 1.0f : 1.0f / (1.0f + std::exp (-a));
                    const float veff = 2.0f * mNvt * sp;
                    const float im  = 0.5f * mKn * veff * veff;           // current into the node
                    const float dim = mKn * veff * sg;                     // d im / d vgs
                    // positive side: germanium diode in series with the body diode, as one diode (nVt and Is of the pair)
                    const float ep = std::exp (std::min (uu / dNvt, 40.0f));
                    const float id  = dIs * (ep - 1.0f);
                    const float did = dIs * ep / dNvt;
                    const float f  = k * (uu - u) + id - im + (uu - vin) / r + uu / rl;
                    const float df = k + did + dim + 1.0f / r + 1.0f / rl;   // d(-im)/duu = +dim
                    uu -= f / df;
                }
                u = uu;
            }
            xPrev = w;
            float y = u + ge3.run (bp3.run (u)) * peak3;                   // stage 3: out = in + feedback part
            y = rail3.process (y);
            return toneBq.run (y);
        }

    private:
        double fs = 48000.0;
        Bq inHp, bp1, ge1, bp3, ge3, toneBq;
        RailLimiter rail1, rail3;
        float peak1 = 1.0f, peak3 = 1.0f, u = 0.0f, xPrev = 0.0f, vth = 2.1f;
        const float mKn = 0.026f, mNvt = 0.05f;                            // 2N7000: about 75 mA at Vgs = 4.5 V
        const float dNvt = 0.0793f, dIs = 1.8e-8f;                         // 1N34A plus a body diode in series, as one diode
    };

    // ---- five more pedals from published analyses (v92) -----------------------------------------------------------------
    // kind 0 Treble boost : one germanium transistor stage, gain = gm x the output pot (80 at full), 5 nF input capacitor into about 11k
    //                       (2.9 kHz corner; the Tone knob moves the capacitor from 5 nF to 47 nF), lopsided limit (+7 V / -1.8 V).
    // kind 1 Clean boost  : op-amp stage, gain (1 + 56k / (R5 + 2.7k)) x 0.955 (1 to 20.6 as the reverse-log pot goes round), 4.7 uF in the
    //                       leg, 47 pF across the 56k, the op-amp's rails (+/-4.5 V) are the only limit.
    // kind 2 Op-amp distortion : gain 1 + 1M / (4.7k + pot) (1.5 to 213), 47 nF in the leg (720 Hz corner at full gain), 741-class bandwidth
    //                       (1 MHz), then 10k into 1 nF with diodes to ground (the clip choice picks them; the stock pedal has germanium).
    // kind 3 Stack drive  : gain stage 1 + Rdist / 2.2k (0 to 33 dB, 100 nF = 723 Hz, 120 pF = 13 kHz), then an inverting stage with 680k and 220 pF
    //                       in its feedback (a 1.06 kHz low-pass) and 10k + the unused part of the same 100k Gain pot at its input (so the one knob sets
    //                       both stages: 15.8 dB at minimum, about 70 dB in all at maximum), 1k into red LEDs to ground, then a tone section and a
    //                       15.3 kHz low-pass. The linear pot and its reach into the second stage (110k at minimum, 10k at maximum) are from a clone
    //                       maker's description; the circuit analysis I used gives only 680k / 10k = x68. The 220 pF is INFERRED: that parts list has a 220 pF
    //                       part (C5) next to the 680k (R4) but does not say where it goes. The tone network's values are on no page I could read as text,
    //                       so the tone section is my own approximation (shelves and a fixed mid scoop near 1.8 kHz).
    // kind 4 Sustain comp : transconductance-amp compressor. Input high-pass 32 Hz, a high-frequency lift of about 3 dB, gain = 1 / (1 + K env),
    //                       env = averaged rectified signal. The analysis gives no ratio, attack or release figure, so the time constants
    //                       (attack 8 to 80 ms, release 400 ms) and the control law are my own estimates.
    class NewPedals
    {
    public:
        void prepare (double sampleRate) { fs = sampleRate; reset(); }
        void reset()
        {
            for (auto* b : { &hp1, &hp2, &bp, &ge, &sh1, &sh2, &sh3, &lpOut, &bp2, &ge2, &tLp, &pk, &lpS2 }) b->reset();
            rail.reset();  rail2.reset();  vNode = xPrev = env = 0.0f;
        }
        void setup (int kind_, float gain, float tone, const DiodeSpec& pos, const DiodeSpec& neg)
        {
            kind = kind_;  dp = pos;  dn = neg;
            const float k = std::min (1.0f, std::max (0.0f, gain)), t = std::min (1.0f, std::max (0.0f, tone));
            rail.setRail (4.5f);  rail2.setRail (4.5f);
            if (kind == 0)
            {
                const double c1 = 5.0e-9 * std::pow (9.4, (double) t);
                hp1.set (highPass1 (fs, 1.0 / (2.0 * 3.14159265358979 * 11000.0 * c1)));
                hp2.set (highPass1 (fs, 16.0));
                gainLin = 80.0f * std::max (0.003f, audioTaper (k));
            }
            else if (kind == 1)
            {
                const float r5 = 500000.0f * audioTaper (1.0f - k);
                const auto gs = designGainStage (fs, r5 + 2700.0f, 4.7e-6f, 56000.0f, 47.0e-12f, 1.0e6f, 0.0f);
                bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = gs.peakGain;
                gainLin = 0.955f;
            }
            else if (kind == 2)
            {
                const float rv = 1.0e6f * audioTaper (1.0f - k);
                const auto gs = designGainStage (fs, 4700.0f + rv, 47.0e-9f, 1.0e6f, 5.0e-12f, 1.0e6f, 0.0f);
                bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = gs.peakGain;
                hp1.set (highPass1 (fs, 23.5));
                rail.setRail (3.5f);
                nodeR = 10000.0f;  nodeC = 1.0e-9f;
            }
            else if (kind == 3)
            {
                // The Gain pot (100k, linear) sits in stage 1's feedback and in series with stage 2's 10k input resistor, so one knob moves both.
                // The real pot is the knob range 0.06 to 0.94; below and above that the law is continued (stage 2 -6 dB at the bottom, stage 1 up to 150k
                // and stage 2 +3 dB at the top), so the real 15.8 dB to 70 dB sits inside a wider 10 dB to 76 dB.
                const float u  = (k - 0.06f) * (1.0f / 0.88f);
                const float rv = 100000.0f * (u <= 1.0f ? std::max (u, 0.0f) : 1.0f + 7.35f * (u - 1.0f));
                float rin2 = 110000.0f - std::min (rv, 100000.0f);
                if (u < 0.0f)               rin2 += 110000.0f * (-u) * (1.0f / 0.068f);
                else if (rv > 100000.0f)    rin2 = 10000.0f / (1.0f + 0.43f * (rv - 100000.0f) * (1.0f / 50000.0f));
                stage2Gain = 680000.0f / rin2;
                const auto gs = designGainStage (fs, 2200.0f, 100.0e-9f, std::max (rv, 1.0f), 120.0e-12f, 3.0e6f, 0.0f);
                bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = gs.peakGain;
                hp1.set (highPass1 (fs, 16.0));
                lpS2.set (lowPass1 (fs, 1.0 / (2.0 * 3.14159265358979 * 680000.0 * 220.0e-12)));      // 220 pF across the 680k: 1.06 kHz
                nodeR = 1000.0f;  nodeC = 100.0e-12f;
                sh1.set (lowShelf (fs, 150.0, std::pow (10.0, (0.5 - t) * 8.0 / 20.0)));
                sh2.set (peak2 (fs, 1800.0, 0.7, std::pow (10.0, -6.0 / 20.0)));
                sh3.set (highShelf (fs, 2500.0, std::pow (10.0, (t - 0.5) * 16.0 / 20.0)));
                lpOut.set (lowPass1 (fs, 15300.0));
            }
            else if (kind == 4)
            {
                // v0.2 Blues Driver: two gain stages, the dual-gang Gain pot (250k) in both, diode clipping between them
                const float rp = 250000.0f * audioTaper (k);                                // the published sweep tops out near 37 dB, which is about 22k + 80k in the feedback
                const auto g1 = designGainStage (fs, 1500.0f, 150.0e-9f, 22000.0f + rp, 47.0e-12f, 2.0e6f, 0.0f);
                bp.set (g1.bandPass);  ge.set (g1.gbwAndEdge);  peak = g1.peakGain;
                const auto g2 = designGainStage (fs, 2200.0f, 1.0e-6f, 33000.0f, 100.0e-12f, 1.0e6f, 0.0f);
                bp2.set (g2.bandPass);  ge2.set (g2.gbwAndEdge);  peak2g = g2.peakGain;
                hp1.set (highPass1 (fs, 12.0));
                rail.setRail (4.5f);
                nodeR = 10000.0f;  nodeC = 2.2e-9f;                                   // estimate: the inter-stage values are not all legible
                tLp.set (lowPass1 (fs, 1000.0 * std::pow (12.0, (double) t)));         // from the published Tone sweep: corner about 1 kHz (min) to 4 kHz (max)
                pk.set (peak2 (fs, 120.0, 1.1, std::pow (10.0, (6.0 - 5.0 * t) / 20.0)));   // the ~120 Hz bump (more with Tone down)
            }
        }
        // For the Boost section: the gain is given in dB and the Treble capacitor as a 0..1 knob (kind 0 only)
        void setupBoost (int kind_, float gainDb, float capKnob)
        {
            kind = kind_;
            rail.setRail (4.5f);
            if (kind == 0)
            {
                const double c1 = 5.0e-9 * std::pow (9.4, (double) std::min (1.0f, std::max (0.0f, capKnob)));
                hp1.set (highPass1 (fs, 1.0 / (2.0 * 3.14159265358979 * 11000.0 * c1)));
                hp2.set (highPass1 (fs, 16.0));
                gainLin = std::min (80.0f, std::pow (10.0f, gainDb / 20.0f));
            }
            else
            {
                const float g = std::min (20.0f, std::max (1.0f, std::pow (10.0f, gainDb / 20.0f) / 0.955f));
                const float r5 = g < 1.001f ? 500000.0f : std::max (0.0f, 56000.0f / (g - 1.0f) - 2700.0f);
                const auto gs = designGainStage (fs, r5 + 2700.0f, 4.7e-6f, 56000.0f, 47.0e-12f, 1.0e6f, 0.0f);
                bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = gs.peakGain;
                gainLin = 0.955f;
            }
        }
        float process (float x)
        {
            float v;
            switch (kind)
            {
                case 0:
                    v = gainLin * hp1.run (x);
                    v = v > 0.0f ? 7.0f * std::tanh (v / 7.0f) : 1.8f * std::tanh (v / 1.8f);
                    return hp2.run (v);
                case 1:
                    v = x + ge.run (bp.run (x)) * peak;
                    return rail.process (v) * gainLin;
                case 2:
                    v = hp1.run (x);
                    v = v + ge.run (bp.run (v)) * peak;
                    v = rail.process (v);
                    return rcNode (v);
                case 3:
                    v = hp1.run (x);
                    v = v + ge.run (bp.run (v)) * peak;
                    v = rail.process (v);
                    v = rail2.process (lpS2.run (-stage2Gain * v));     // inverting stage with its 1.06 kHz feedback low-pass, then its own limiter (see Blues Driver below)
                    v = rcNode (v);
                    v = sh3.run (sh2.run (sh1.run (v)));
                    return lpOut.run (v);
                case 4:
                    v = hp1.run (x);
                    v = v + ge.run (bp.run (v)) * peak;
                    v = rail.process (v);
                    v = rcNode (v * 0.3f) * 0.033f;                      // inter-stage divider before and after the diodes, set so the small-signal total matches the published sweep (+7 to +21 dB)
                    v = v + ge2.run (bp2.run (v)) * peak2g;
                    v = rail2.process (v);                               // its own limiter: RailLimiter keeps the previous input for its anti-aliasing, so two stages cannot share one
                    return pk.run (tLp.run (v));
                default: return x;
            }
        }

    private:
        float peak2g = 1.0f, stage2Gain = 68.0f;
        Bq bp2, ge2, tLp, pk, lpS2;
        int kind = 0;
        double fs = 48000.0;
        Bq hp1, hp2, bp, ge, sh1, sh2, sh3, lpOut;
        RailLimiter rail, rail2;
        DiodeSpec dp = diodeSilicon(), dn = diodeSilicon();
        float gainLin = 1.0f, peak = 1.0f, vNode = 0.0f, xPrev = 0.0f, nodeR = 10000.0f, nodeC = 1.0e-9f;
        float env = 0.0f, compK = 10.0f, atkCoef = 0.01f, relCoef = 0.0001f;

        // R into C with antiparallel diodes to ground: backward Euler, four sub-steps, Newton on the node voltage
        float rcNode (float w)
        {
            const float kc = nodeC * 4.0f * (float) fs;
            for (int s = 1; s <= 4; ++s)
            {
                const float vin = xPrev + (w - xPrev) * (float) s * 0.25f;
                float u = vNode;
                for (int it = 0; it < 4; ++it)
                {
                    const float ep = std::exp (std::min (u / dp.nVt, 40.0f)), en = std::exp (std::min (-u / dn.nVt, 40.0f));
                    const float id  = dp.isat * (ep - 1.0f) - dn.isat * (en - 1.0f);
                    const float did = dp.isat * ep / dp.nVt + dn.isat * en / dn.nVt;
                    const float f  = kc * (u - vNode) + id + (u - vin) / nodeR;
                    u -= f / (kc + did + 1.0f / nodeR);
                }
                vNode = u;
            }
            xPrev = w;
            return vNode;
        }
        static BiquadCoeffs lowShelf (double fs, double f0, double g)  { const double w = 2.0 * 3.14159265358979 * f0; return bilinear (fs, 0, 1, g * w, 0, 1, w); }
        static BiquadCoeffs highShelf (double fs, double f0, double g) { const double w = 2.0 * 3.14159265358979 * f0; return bilinear (fs, 0, g, w, 0, 1, w); }
        static BiquadCoeffs peak2 (double fs, double f0, double q, double g)
        {
            const double w = 2.0 * 3.14159265358979 * f0;
            return bilinear (fs, 1, g * w / q, w * w, 1, w / q, w * w);
        }
    };

    // R into C with antiparallel diodes to ground (backward Euler, four sub-steps per sample, Newton on the node voltage)
    struct RcDiodeNode
    {
        float r = 1000.0f, c = 1.0e-9f, v = 0.0f, xPrev = 0.0f, fs = 48000.0f;
        DiodeSpec dp = diodeGermanium(), dn = diodeGermanium();
        void reset() { v = xPrev = 0.0f; }
        float process (float w)
        {
            const float kc = c * 4.0f * fs;
            for (int s = 1; s <= 4; ++s)
            {
                const float vin = xPrev + (w - xPrev) * (float) s * 0.25f;
                float u = v;
                for (int it = 0; it < 4; ++it)
                {
                    const float ep = std::exp (std::min (u / dp.nVt, 40.0f)), en = std::exp (std::min (-u / dn.nVt, 40.0f));
                    const float id  = dp.isat * (ep - 1.0f) - dn.isat * (en - 1.0f);
                    const float did = dp.isat * ep / dp.nVt + dn.isat * en / dn.nVt;
                    u -= (kc * (u - v) + id + (u - vin) / r) / (kc + did + 1.0f / r);
                }
                v = u;
            }
            xPrev = w;
            return v;
        }
    };

    // ---- the Transparent drive, circuit version (v93): the gain-pot-blend overdrive (Centaur-type) ------------------------------------
    // From the published analysis: a unity input buffer; a gain stage with about 40 dB at most and a hump near 1 kHz; two germanium
    // diodes (a 0.35 V knee) to ground after it; a feed-forward low-pass at 106 Hz that carries the low end around the clipper; a dual-gang
    // Gain pot that fades the clean path out as the drive path comes in; an active high shelf with its pivot at 408 Hz (+18.2 dB / -8 dB).
    // NOT in the analysis (so these are my estimates, to be replaced from measured readings): the gain stage's resistor and capacitor
    // values (kR1, kC1, kC4, kRfMin), the clipper's resistor, the fade law of the dual gang, and the weight of the low-end path.
    struct KlonValues
    {
        float r1 = 1000.0f;            // gain stage: resistor in the leg to ground (sets 100 x at full gain with the 100k pot)
        float c1 = 0.33e-6f;           // its capacitor (482 Hz low corner)                                   -- estimate
        float rfMin = 1500.0f, rfPot = 100000.0f;
        float c4 = 390.0e-12f;         // feedback capacitor (4 kHz with the full pot)                         -- estimate
        float gbw = 3.0e6f;            // the op-amp's gain-bandwidth                                          -- estimate
        float clipR = 1000.0f, clipC = 100.0e-12f;                                                        // -- estimate
        float ffHz = 106.0f;           // feed-forward low-pass (analysis)
        float shelfPivotHz = 408.0f, boostDb = 18.24f, cutDb = -8.0f;   // tone shelf (analysis; v94: replaced by the fitted voicing)
        float driveScale = 1.0f;       // v94: scales the drive path to match the captures' Gain 0 to 5 step
    };
    // v94: fitted to the LC_VODA captures you sent (Volume 5, Tone and Gain at 0, 5 and 10, Grip off and on). What was measured and used:
    //   the small-signal response at -36 dBFS: a high-pass (one pole) and a low-pass (two poles, nearly overdamped) with corners that move with Gain
    //   (high-pass 700 / 475 / 385 Hz at Gain 0 / 5 / 10) and with Tone (low-pass 3.9 / 4.5 kHz at Tone 5 / 10, and a bell cut of up to 12 dB at 2.2 kHz towards Tone 0), within about 1 dB at Tone 5 and 10;
    //   the gain from Gain 0 to Gain 5 (about +7 dB at 1 kHz); and what Grip does to the response (about -3 dB around 300 to 500 Hz, +1 dB above 1 kHz).
    // NOT used, because my fits to the compression curves were poor (4 dB RMS) and several capture files are clipped at full scale: the amount of
    //   compression and the Grip peak limiting. Those stay as the old estimates (germanium clipper, fade of the clean path).
    class KlonCircuit
    {
    public:
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            node.fs = (float) fs;  node.r = vals.clipR;  node.c = vals.clipC;
            node.dp = node.dn = diodeGermanium();
            ffLp.set (lowPass1 (fs, vals.ffHz));
            rail.setRail (4.5f);
            reset();
        }
        void reset() { for (auto* b : { &ffLp, &bp, &ge, &vhp, &vlp, &tcut, &edge, &gripCut, &gripHi }) b->reset(); rail.reset(); node.reset(); }
        // gain, treble, edge: all 0..1
        void setup (float gain, float treble, float edgeAmt, float grip = 0.0f)
        {
            const float k = std::min (1.0f, std::max (0.0f, gain)), t = std::min (1.0f, std::max (0.0f, treble));
            const float rf = vals.rfMin + vals.rfPot * audioTaper (k);
            const auto gs = designGainStage (fs, vals.r1, vals.c1, rf, vals.c4, vals.gbw, 0.0f);
            bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = gs.peakGain * vals.driveScale * (k <= 0.5f ? 0.2f : 0.2f - 0.14f * (2.0f * k - 1.0f));   // 0.2 matches the captures' Gain 0 to 5 step; above that there is no data (compressed), so a guess
            cleanW = 1.0f - 0.6f * k;                       // dual gang: the clean path fades as the drive path rises   -- estimate
            driveW = k;                                     // and the drive path fades in                                -- estimate
            ffW    = k <= 0.5f ? 0.03f + 0.28f * k : 0.17f + 0.86f * (k - 0.5f);   // the low-end feed-forward: 0.17 at Gain 5 from the captures' bass; above that a guess
            auto lerp = [] (double a, double b, double u) { return a + (b - a) * u; };
            const double fh = k < 0.5f ? lerp (700.0, 475.0, 2.0 * k) : lerp (475.0, 385.0, 2.0 * (k - 0.5));
            const double fl = t < 0.5f ? 3900.0 : lerp (3900.0, 4540.0, 2.0 * (t - 0.5));
            const double q  = t < 0.5f ? 0.29 : lerp (0.29, 0.49, 2.0 * (t - 0.5));
            // below Tone 5 the captures show a cut of about 10 to 12 dB across 1.5 to 3 kHz (it comes back above 4 kHz), so: a bell cut at 2.2 kHz
            {
                const double dBc = t < 0.5f ? -12.0 * (1.0 - 2.0 * t) : 0.0, A = std::pow (10.0, dBc / 40.0), w0 = 2.0 * 3.14159265358979 * 2200.0, iq = 1.0 / 0.8;
                tcut.set (bilinear (fs, 1.0, A * iq * w0, w0 * w0, 1.0, iq / A * w0, w0 * w0));
            }
            vhp.set (highPass1 (fs, fh));
            const double w = 2.0 * 3.14159265358979 * fl;
            vlp.set (bilinear (fs, 0, 0, w * w, 1, w / q, w * w));
            // 1 kHz makeup so the voicing itself is level-neutral there
            const double rh = 1000.0 / fh, rl = 1000.0 / fl;
            const double mag = (rh / std::sqrt (1.0 + rh * rh)) / std::sqrt ((1.0 - rl * rl) * (1.0 - rl * rl) + (rl / q) * (rl / q));
            voiceMakeup = (float) (1.0 / mag);
            // Edge (the plugin's own extra control): more treble into the clipper, up to +10 dB above 3 kHz
            const double ge2 = std::pow (10.0, 10.0 * std::min (1.0f, std::max (0.0f, edgeAmt)) / 20.0), w2 = 2.0 * 3.14159265358979 * 3000.0;
            edge.set (bilinear (fs, 0, ge2, w2, 0, 1, w2));
            gripOn = grip > 0.001f;
            if (gripOn)
            {
                // about -3 dB at 330 Hz (Q 0.9), +1 dB shelf above 1.2 kHz
                const double A = std::pow (10.0, -6.0 * grip / 40.0), w0 = 2.0 * 3.14159265358979 * 330.0, al = 1.0 / 0.9;
                gripCut.set (bilinear (fs, 1.0, al * A * w0, w0 * w0, 1.0, al / A * w0, w0 * w0));
                const double gh = std::pow (10.0, 3.0 * grip / 20.0), wh = 2.0 * 3.14159265358979 * 1200.0;
                gripHi.set (bilinear (fs, 0, gh, wh, 0, 1, wh));
            }
        }
        float process (float x)
        {
            float d = edge.run (x);
            float h = tcut.run (vlp.run (vhp.run (d))) * voiceMakeup;
            float dr = h + ge.run (bp.run (h)) * peak;
            dr = rail.process (dr);
            dr = node.process (dr);
            float sum = cleanW * h + ffW * ffLp.run (x) + driveW * dr;
            if (gripOn) sum = gripHi.run (gripCut.run (sum));
            return sum;
        }
        KlonValues vals;

    private:
        double fs = 48000.0;
        Bq ffLp, bp, ge, vhp, vlp, tcut, edge, gripCut, gripHi;
        RailLimiter rail;
        RcDiodeNode node;
        float peak = 1.0f, cleanW = 1.0f, driveW = 0.0f, ffW = 0.0f, voiceMakeup = 1.0f;
        bool gripOn = false;
    };

    // ---- the Dyna compressor (v93), for the Comp section ----------------------------------------------------------------------------
    // From the published analysis: input high-pass near 32 Hz, a transconductance amp whose two inputs are unequal, which lifts the highs
    // by about 1 dB at 300 Hz and 4 dB at 10 kHz (modelled as a 3 dB shelf above 3 kHz), a detector that averages the rectified signal, a
    // gain that falls as that average rises, and an output high-pass (52 to 310 Hz, modelled at 90 Hz). The analysis gives NO ratio, attack,
    // release or detector law, so those are my own: gain = 1 / (1 + K x average), K from the Sens knob; times and mix come from the Comp knobs.
    class DynaComp
    {
    public:
        void prepare (double sampleRate) { fs = sampleRate; hp1.set (highPass1 (fs, 32.0)); hp2.set (highPass1 (fs, 90.0));
                                           sh.set (bilinear (fs, 0, std::pow (10.0, 3.0 / 20.0), 2.0 * 3.14159265358979 * 3000.0, 0, 1, 2.0 * 3.14159265358979 * 3000.0)); reset(); }
        void reset() { hp1.reset(); hp2.reset(); sh.reset(); env = 0.0f; grAvg = 0.0f; }
        // sens 0..1, times in ms, outDb, mix 0..1. Returns the average gain reduction of the block in dB (>= 0).
        float process (float* x, int n, float sens, float atkMs, float relMs, float outDb, float mix)
        {
            const float K = 2.0f + 60.0f * std::min (1.0f, std::max (0.0f, sens));
            const float atk = 1.0f - std::exp (-1.0f / (std::max (0.1f, atkMs) * 0.001f * (float) fs));
            const float rel = 1.0f - std::exp (-1.0f / (std::max (5.0f, relMs) * 0.001f * (float) fs));
            const float out = std::pow (10.0f, outDb / 20.0f) * trim;
            float gsum = 0.0f;
            for (int i = 0; i < n; ++i)
            {
                const float dry = x[i];
                float v = sh.run (hp1.run (dry));
                const float a = std::fabs (v);
                env += (a > env ? atk : rel) * (a - env);
                const float g = 1.0f / (1.0f + K * env);
                gsum += g;
                v = 0.5f * std::tanh (v * g / 0.5f);                  // the transconductance amp runs out of range
                v = hp2.run (v) * out;
                x[i] = dry * (1.0f - mix) + v * mix;
            }
            const float gm = gsum / (float) std::max (1, n);
            return -20.0f * std::log10 (std::max (1.0e-4f, gm));
        }
        float trim = 8.0f;
    private:
        double fs = 48000.0;
        Bq hp1, hp2, sh;
        float env = 0.0f, grAvg = 0.0f;
    };

    // ---- dense linear solver for the small circuit networks below (n <= 16), double precision ---------------------------------------
    inline bool solveDense (int n, double (&A)[16][16], double* b)      // Gaussian elimination with partial pivoting; b becomes x
    {
        for (int c = 0; c < n; ++c)
        {
            int p = c;
            for (int r = c + 1; r < n; ++r) if (std::fabs (A[r][c]) > std::fabs (A[p][c])) p = r;
            if (std::fabs (A[p][c]) < 1.0e-30) return false;
            if (p != c) { for (int k = c; k < n; ++k) std::swap (A[c][k], A[p][k]); std::swap (b[c], b[p]); }
            const double inv = 1.0 / A[c][c];
            for (int r = c + 1; r < n; ++r)
            {
                const double f = A[r][c] * inv;
                if (f == 0.0) continue;
                for (int k = c; k < n; ++k) A[r][k] -= f * A[c][k];
                b[r] -= f * b[c];
            }
        }
        for (int r = n - 1; r >= 0; --r)
        {
            double sum = b[r];
            for (int k = r + 1; k < n; ++k) sum -= A[r][k] * b[k];
            b[r] = sum / A[r][r];
        }
        return true;
    }

    // ---- the HM-2 (v93), from a redrawn schematic ----------------------------------------------------------------------------------------
    // Read from the schematic:
    //   Q1 (JFET follower) -> Q6 (NPN, 470k / 22k shunt feedback, 10k collector, 22 ohm emitter) -> Q7 (PNP, 470k / 22k, 120 ohm emitter),
    //   each with 47 nF into 22k (about 130 Hz) and 10 pF across its 470k (about 34 kHz);
    //   IC1B: a non-inverting stage, Rg = 47k + the part of the 250k DIST pot between its wiper (grounded) and the 47k end, 47 nF in the leg,
    //         220k with 10 pF across it, and in the feedback loop one 1N4148 one way and two in series the other way;
    //   the DIST pot's other half (with 150 ohm and 10 uF) loads Q6's collector, so DIST also sets how hard Q7 is driven;
    //   after IC1B: 1 uF, 10k, a germanium diode pair in series with the signal, 10k, then 1 nF and two 1N4148 to ground (the second clipper);
    //   IC1A buffer, 3.3k, then the LOW / HIGH network: IC3A with two 10k pots from its input to its inverting node, each wiper shunted to ground by
    //   a series resonant branch (a capacitor, 330 ohm and an op-amp "inductor": 1.5 uF / 68 nF / 100k for LOW, 0.1 uF / 4.7 nF / 100k and
    //   0.15 uF / 6.8 nF / 82k for HIGH). The op-amp inductor is exactly 330 ohm + (Rb - 330) in parallel with L = (Rb - 330) x 330 x Cf, which
    //   puts LOW's peak at 87 Hz (Q 3.7) and HIGH's at 0.96 and 1.28 kHz, +/-20 dB, as the controls show.
    // NOT on the schematic (my estimates): the transistor stages' gains (-18 and -14; their bias points are not given), their output
    //   limits, the op-amps' own bandwidth and output swing (3.2 V either way), and which way the two diodes in the feedback loop point.
    class Hm2Tone
    {
    public:
        void prepare (double sampleRate) { fs = sampleRate; reset(); last[0] = last[1] = last[2] = -1.0f; setBoost (0.5f, 0.5f, 0.5f); }
        void reset() { for (double& v : ieq) v = 0.0; for (double& v : il) v = 0.0; for (double& v : vlPrev) v = 0.0; }
        // lowBoost, highBoost, midBoost: 0 = full cut, 0.5 = flat, 1 = full boost.
        // v94: the single "hi color" pot is split into two (the N.E.W. build's SPLIT option): MIDS carries the 0.15 uF / 6.8 nF / 82k branch (1.28 kHz),
        // HIGH carries the 0.1 uF / 4.7 nF / 100k branch (0.96 kHz and up). Both pots are 10k and both wipers are summed by the shared op-amp network.
        void setBoost (float lo, float hi, float mid)
        {
            lo = std::min (1.0f, std::max (0.0f, lo));  hi = std::min (1.0f, std::max (0.0f, hi));  mid = std::min (1.0f, std::max (0.0f, mid));
            if (std::fabs (lo - last[0]) < 1.0e-4f && std::fabs (hi - last[1]) < 1.0e-4f && std::fabs (mid - last[2]) < 1.0e-4f) return;
            last[0] = lo;  last[1] = hi;  last[2] = mid;
            const double T = 1.0 / fs;
            double A[16][16] = {};
            auto stampG = [&] (int p, int q, double g) { A[p][p] += g; if (q >= 0) { A[q][q] += g; A[p][q] -= g; A[q][p] -= g; } };
            const double gS = 1.0 / 3300.0;
            A[0][0] += gS;
            auto pot = [&] (double b, int w) { const double r1 = std::max (5.0, 10000.0 * b), r2 = std::max (5.0, 10000.0 * (1.0 - b)); stampG (0, w, 1.0 / r1); stampG (w, 1, 1.0 / r2); };
            pot (lo, 3);  pot (hi, 4);  pot (mid, 12);
            stampG (2, 1, 1.0 / 3300.0);                                       // IC3A feedback
            struct Br { int w; double cs, cf, rb; };
            const Br br[3] = { { 3, 1.5e-6, 68.0e-9, 100.0e3 }, { 4, 0.1e-6, 4.7e-9, 100.0e3 }, { 12, 0.15e-6, 6.8e-9, 82.0e3 } };
            for (int b = 0; b < 3; ++b)
            {
                const int y = 5 + 2 * b, z = 6 + 2 * b;
                const double rp = br[b].rb - 330.0, l = rp * 330.0 * br[b].cf;
                gc[b] = 2.0 * br[b].cs / T;
                stampG (br[b].w, y, gc[b]);                                    // series capacitor (trapezoid)
                stampG (y, z, 1.0 / 330.0);
                stampG (z, -1, 1.0 / rp);
                gl[b] = T / (2.0 * l);
                stampG (z, -1, gl[b]);                                         // the op-amp inductor (trapezoid)
            }
            A[11][0] = 1.0;  A[11][1] = -1.0;                                  // ideal op-amp: V(A) = V(N)
            A[2][11] = 1.0;                                                    // its output current into OUT
            // invert (Gauss-Jordan on a copy)
            double M[16][32] = {};
            for (int i = 0; i < 13; ++i) { for (int j = 0; j < 13; ++j) M[i][j] = A[i][j]; M[i][13 + i] = 1.0; }
            for (int c = 0; c < 13; ++c)
            {
                int p = c; for (int r = c + 1; r < 13; ++r) if (std::fabs (M[r][c]) > std::fabs (M[p][c])) p = r;
                for (int k = 0; k < 26; ++k) std::swap (M[c][k], M[p][k]);
                const double d = M[c][c];  for (int k = 0; k < 26; ++k) M[c][k] /= d;
                for (int r = 0; r < 13; ++r) if (r != c) { const double f = M[r][c]; if (f != 0.0) for (int k = 0; k < 26; ++k) M[r][k] -= f * M[c][k]; }
            }
            for (int i = 0; i < 13; ++i) for (int j = 0; j < 13; ++j) inv[i][j] = M[i][13 + j];
        }
        float process (float vs)
        {
            double b[13] = {};
            b[0] = vs / 3300.0;
            static const int wn[3] = { 3, 4, 12 };
            for (int k = 0; k < 3; ++k)
            {
                b[wn[k]] += ieq[k];  b[5 + 2 * k] -= ieq[k];                   // capacitor history
                b[6 + 2 * k] -= il[k] + gl[k] * vlPrev[k];                     // inductor history (current flowing Z -> ground)
            }
            double x[13];
            for (int i = 0; i < 13; ++i)
            {
                double acc = inv[i][0] * b[0] + inv[i][3] * b[3] + inv[i][4] * b[4] + inv[i][12] * b[12];
                for (int j = 5; j <= 10; ++j) acc += inv[i][j] * b[j];
                x[i] = acc;
            }
            // note: nodes 3, 4 and 12 (the three wipers) appear in b only through the capacitor history
            for (int k = 0; k < 3; ++k)
            {
                const double v = x[wn[k]] - x[5 + 2 * k];
                const double i = gc[k] * v - ieq[k];
                ieq[k] = gc[k] * v + i;
                const double vz = x[6 + 2 * k];
                il[k] = gl[k] * vz + (il[k] + gl[k] * vlPrev[k]);
                vlPrev[k] = vz;
            }
            return (float) x[2];
        }
    private:
        double fs = 48000.0, gc[3] = {}, gl[3] = {}, ieq[3] = {}, il[3] = {}, vlPrev[3] = {};
        double inv[16][16] = {};
        float last[3] = { -1.0f, -1.0f, -1.0f };
    };

    class Hm2Circuit
    {
    public:
        // v94, from the N.E.W. DIY HM-2 schematic (a modded copy of the classic): mid = the split MIDS pot, soft = the 1N4148 feedback diodes in IC1B's loop
        // (SOFT switch), hard = the 1N4148 shunt pair after the germanium pair (HARD switch), blend = the 1M BLEND pot between the distorted signal and a
        // clean copy of the input (IC3A, a non-inverting stage with a gain of 2), pol = the clean copy inverted (not on that board; the Medusa has it).
        struct Params { float dist = 0.6f, low = 0.5f, high = 0.5f, mid = 0.5f, blend = 0.0f; bool soft = true, hard = true, pol = false; };
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            tone.prepare (fs);
            nodeSub = fs < 100000.0 ? 2 : 1;
            reset();  lastDist = -1.0f;
        }
        void reset()
        {
            hp1.reset(); hp2.reset(); lp1.reset(); lp2.reset(); bp.reset(); ge.reset(); lim.reset(); rail.reset(); tone.reset(); acHp.reset();
            vc = 0.0; iSeries = 0.0; xPrev = 0.0f; clipPrev = 0.0f;
        }
        void configure (const Params& p)
        {
            tone.setBoost (p.low, p.high, p.mid);
            soft = p.soft;  hard = p.hard;  blendAmt = std::min (1.0f, std::max (0.0f, p.blend));  cleanSign = p.pol ? -1.0f : 1.0f;
            if (std::fabs (p.dist - lastDist) > 1.0e-4f)
            {
                lastDist = p.dist;
                const float t = std::min (1.0f, std::max (0.0f, p.dist));
                const float rLeft  = 250.0e3f * audioTaper (t);           // wiper to the 150 ohm end: the load on Q6's collector
                const float rRight = 250.0e3f * (1.0f - audioTaper (t));  // wiper to the 47k end: in IC1B's leg
                // stage 1 (Q6): shunt-feedback gain -(Rf/Rs) A / (A + 1 + Rf/Rs + Rf/Rb) with A = gm' x (its collector load, loaded by the DIST pot's other half)
                const float load = 1.0f / (1.0f / 10.0e3f + 1.0f / (150.0f + rLeft) + 1.0f / 22.0e3f + 1.0f / 470.0e3f);
                const float a1 = 13.5e-3f * load;
                g1 = 21.4f * a1 / (a1 + 1.0f + 21.4f + 4.7f);
                const float load2 = 1.0f / (1.0f / 10.0e3f + 1.0f / 470.0e3f);
                const float a2 = 5.6e-3f * load2;
                g2 = 21.4f * a2 / (a2 + 1.0f + 21.4f + 4.7f);
                hp1.set (highPass1 (fs, 133.0));
                hp2.set (highPass1 (fs, 114.0));
                lp1.set (lowPass1 (fs, 34000.0));
                lp2.set (lowPass1 (fs, 34000.0));
                const float r1 = 47000.0f + rRight;
                const auto gs = designGainStage (fs, r1, 47.0e-9f, 220000.0f, 10.0e-12f, 1.0e6f, 0.0f);
                bp.set (gs.bandPass);  ge.set (gs.gbwAndEdge);  peak = gs.peakGain;
                lim.setup (220000.0f, diodeSiliconPair(), diodeSilicon());
                rail.setRail (3.2f);
                acHp.set (highPass1 (fs, 16.0));
            }
        }
        float process (float x)
        {
            // Q1 follower, then Q6 and Q7 (each inverting, 47 nF into 22k, 10 pF across its 470k, running out of collector swing)
            float v = hp1.run (0.95f * x);
            v = lp1.run (-g1 * v);
            v = v > 0.0f ? 4.5f * std::tanh (v / 4.5f) : 3.5f * std::tanh (v / 3.5f);
            v = hp2.run (v);
            v = lp2.run (-g2 * v);
            v = v > 0.0f ? 4.5f * std::tanh (v / 4.5f) : 3.5f * std::tanh (v / 3.5f);
            // IC1B: out = in + Vf, Vf = diode-limited (in x Zf / Zg)
            const float lin = ge.run (bp.run (v)) * peak;
            float w = v + (soft ? lim.process (lin) : lin);
            w = rail.process (w);
            w = acHp.run (w);                                           // the 1 uF into the clipper
            // 10k, the germanium pair in series, 10k, then 1 nF and two 1N4148 to ground
            const float c = clipNode (w);
            const float wet = tone.process (c);
            if (blendAmt <= 0.0005f) return wet;
            return (1.0f - blendAmt) * wet + blendAmt * cleanSign * 2.0f * x;     // IC3A: gain 1 + 10k / 10k
        }
    private:
        double fs = 48000.0;
        Bq hp1, hp2, lp1, lp2, bp, ge, acHp;
        DiodeLimiter lim;
        RailLimiter rail;
        Hm2Tone tone;
        float g1 = 15.0f, g2 = 14.0f, peak = 1.0f, lastDist = -1.0f, xPrev = 0.0f, clipPrev = 0.0f;
        double vc = 0.0, iSeries = 0.0;
        int nodeSub = 1;
        bool soft = true, hard = true;
        float blendAmt = 0.0f, cleanSign = 1.0f;

        // Series germanium pair (i = 2 Is sinh (vd / nVt)) between two 10k resistors into the node with the 1 nF and the shunt silicon pair.
        // Backward Euler on the 1 nF; Newton on (i, v).
        float clipNode (float vin)
        {
            const DiodeSpec ge_ = diodeGermanium(), si = diodeSilicon();
            const double C = 1.0e-9, dt = 1.0 / (fs * nodeSub), Rs = 20000.0;
            for (int s = 1; s <= nodeSub; ++s)
            {
                const double u = (double) xPrev + ((double) vin - (double) xPrev) * (double) s / (double) nodeSub;
                double v = vc, i = iSeries;
                for (int it = 0; it < 5; ++it)
                {
                    const double isg = 2.0 * ge_.isat;
                    const double f1 = Rs * i + ge_.nVt * std::asinh (i / isg) - (u - v);
                    const double ep = std::exp (std::min (v / si.nVt, 40.0)), en = std::exp (std::min (-v / si.nVt, 40.0));
                    const double ish = hard ? si.isat * (ep - en) : 0.0, dish = hard ? si.isat * (ep + en) / si.nVt : 0.0;
                    const double f2 = C * (v - vc) / dt + ish - i;
                    const double a11 = Rs + ge_.nVt / std::sqrt (isg * isg + i * i), a12 = 1.0, a21 = -1.0, a22 = C / dt + dish;
                    const double det = a11 * a22 - a12 * a21;
                    const double di = (f1 * a22 - a12 * f2) / det, dv = (a11 * f2 - a21 * f1) / det;
                    i -= di;  v -= dv;
                }
                vc = v;  iSeries = i;
            }
            xPrev = vin;
            return (float) vc;
        }
    };

    // ---- v95: a general small linear RC network solved node by node (trapezoidal capacitors), for passive tone networks -------
    //   Add resistors and capacitors between nodes (-1 = ground), name the driven node and its source resistance, call setup(),
    //   then run samples through process(). The matrix is inverted once in setup() (call it again when a pot moves).
    class NodalNet
    {
    public:
        static constexpr int kMax = 10;
        void clear() { nEl = 0; n = 0; }
        void addR (int a, int b, double ohms) { add (a, b, 1.0 / std::max (ohms, 0.5), false); }
        void addC (int a, int b, double farads) { add (a, b, farads, true); }
        void setSource (int node, double rs) { src = node; gSrc = 1.0 / rs; }
        void setOutput (int node) { out = node; }
        void setup (double sampleRate)
        {
            fs = sampleRate;
            double Y[kMax][2 * kMax] = {};
            auto stamp = [&] (int a, int b, double g)
            {
                Y[a][a] += g;
                if (b >= 0) { Y[b][b] += g; Y[a][b] -= g; Y[b][a] -= g; }
            };
            for (int i = 0; i < nEl; ++i)
                stamp (el[i].a, el[i].b, el[i].cap ? 2.0 * el[i].v * fs : el[i].v);
            stamp (src, -1, gSrc);
            for (int i = 0; i < n; ++i) Y[i][n + i] = 1.0;
            for (int c = 0; c < n; ++c)
            {
                int p = c;  for (int r = c + 1; r < n; ++r) if (std::fabs (Y[r][c]) > std::fabs (Y[p][c])) p = r;
                for (int k = 0; k < 2 * n; ++k) std::swap (Y[c][k], Y[p][k]);
                const double d = Y[c][c];  for (int k = 0; k < 2 * n; ++k) Y[c][k] /= d;
                for (int r = 0; r < n; ++r) if (r != c) { const double f = Y[r][c]; for (int k = 0; k < 2 * n; ++k) Y[r][k] -= f * Y[c][k]; }
            }
            for (int i = 0; i < n; ++i) for (int j = 0; j < n; ++j) inv[i][j] = Y[i][n + j];
        }
        void reset() { for (int i = 0; i < kMax; ++i) { vPrev[i] = 0.0; } for (auto& e : el) { e.ih = 0.0; e.ic = 0.0; } }
        float process (float vin)
        {
            double b[kMax] = {};
            b[src] += gSrc * (double) vin;
            for (int i = 0; i < nEl; ++i)
                if (el[i].cap)
                {
                    const double gc = 2.0 * el[i].v * fs;
                    const double vp = (el[i].a >= 0 ? vPrev[el[i].a] : 0.0) - (el[i].b >= 0 ? vPrev[el[i].b] : 0.0);
                    el[i].ih = gc * vp + el[i].ic;
                    b[el[i].a] += el[i].ih;
                    if (el[i].b >= 0) b[el[i].b] -= el[i].ih;
                }
            double v[kMax];
            for (int i = 0; i < n; ++i) { double a = 0.0; for (int j = 0; j < n; ++j) a += inv[i][j] * b[j]; v[i] = a; }
            for (int i = 0; i < nEl; ++i)
                if (el[i].cap)
                {
                    const double gc = 2.0 * el[i].v * fs;
                    const double vc = v[el[i].a] - (el[i].b >= 0 ? v[el[i].b] : 0.0);
                    el[i].ic = gc * vc - el[i].ih;
                }
            for (int i = 0; i < n; ++i) vPrev[i] = v[i];
            return (float) v[out];
        }
    private:
        struct El { int a, b; double v; bool cap; double ih, ic; };
        El el[32];  int nEl = 0, n = 0, src = 0, out = 0;
        double gSrc = 1.0e-2, fs = 48000.0, vPrev[kMax] = {}, inv[kMax][kMax] = {};
        void add (int a, int b, double v, bool cap)
        {
            if (nEl >= 32) return;
            el[nEl++] = { a, b, v, cap, 0.0, 0.0 };
            n = std::max (n, std::max (a, b) + 1);
        }
    };

    // A rail-limited stage: a smooth knee (order 4) with its own positive and negative limit.
    inline float railClip (float x, float pos, float neg)
    {
        const float lim = x >= 0.0f ? pos : neg;
        const float u = std::fabs (x) / lim, u2 = u * u;
        return x / std::sqrt (std::sqrt (1.0f + u2 * u2));
    }

    // ---- v95: the cranked power-chip distortion (from the Acapulco Tone schematic) --------------------------------------
    //   J201 buffer -> LM386 with gain 200 (10 uF across pins 1 and 8) -> passive Bass / High network (solved as a circuit) ->
    //   LM386 with gain 20 -> J201 buffer. The Clean blend is the plugin's Mix knob. Output limits of a 9 V LM386 are my estimate
    //   (+3.1 V / -3.8 V around its half-supply point); the chip's 6 MHz gain-bandwidth gives about 30 kHz at gain 200.
    class PowerChipCircuit
    {
    public:
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            inHp.set (highPass1 (fs, 29.0));
            gainHp.set (highPass1 (fs, 11.8));
            chipLp.set (lowPass1 (fs, std::min (30000.0, 0.45 * fs)));
            chip2Lp.set (lowPass1 (fs, std::min (60000.0, 0.45 * fs)));
            outHp.set (highPass1 (fs, 10.0));
            lastBass = lastHigh = -1.0f;
            reset();
        }
        void reset() { inHp.reset(); gainHp.reset(); chipLp.reset(); chip2Lp.reset(); outHp.reset(); net.reset(); }
        // bass, high: 0..1; drive: how hard the guitar hits the chip (linear gain, 1 = unity)
        void configure (float bass, float high, float drive)
        {
            drv = drive;
            if (std::fabs (bass - lastBass) > 1.0e-4f || std::fabs (high - lastHigh) > 1.0e-4f)
            {
                lastBass = bass;  lastHigh = high;
                const double fb = std::pow (std::min (1.0f, std::max (0.0f, bass)), 2.0), fh = std::pow (std::min (1.0f, std::max (0.0f, high)), 2.0);
                net.clear();
                // nodes: 0 driven, 1 S, 2 P1, 3 P3, 4 M, 5 H, 6 L, 7 N, 8 Z
                net.addC (0, 1, 1.0e-6);  net.addR (1, 3, 100.0e3);
                net.addR (2, 4, std::max (fb * 1.0e6, 1.0));  net.addR (4, 3, std::max ((1.0 - fb) * 1.0e6, 1.0));
                net.addR (2, -1, 4.7e3);  net.addC (2, 4, 4.7e-9);  net.addC (3, 4, 1.0e-9);
                net.addC (1, 5, 1.0e-9);
                net.addR (5, 7, std::max ((1.0 - fh) * 1.0e6, 1.0));  net.addR (7, 6, std::max (fh * 1.0e6, 1.0));  net.addC (6, -1, 10.0e-9);
                net.addR (4, 7, 47.0e3);  net.addR (7, 8, 68.0e3);  net.addC (8, -1, 4.7e-9);  net.addR (8, -1, 50.0e3);
                net.setSource (0, 10.0);  net.setOutput (8);
                net.setup (fs);
            }
        }
        float process (float x)
        {
            float v = 0.85f * 1.2f * std::tanh (x * drv / 1.2f);           // the J201 source follower
            v = inHp.run (v);
            v = 20.0f * (v + 9.0f * gainHp.run (v));                       // gain 20 above 12 Hz becoming 200
            v = chipLp.run (v);
            v = railClip (v, 3.1f, 3.8f);
            v -= 0.03f * std::tanh (v / 0.03f);                             // a touch of crossover
            v = net.process (v);
            v = 20.0f * chip2Lp.run (v);                                    // the second chip, gain 20
            v = railClip (v, 3.1f, 3.8f);
            return outHp.run (v);
        }
    private:
        double fs = 48000.0;
        Bq inHp, gainHp, chipLp, chip2Lp, outHp;
        NodalNet net;
        float lastBass = -1.0f, lastHigh = -1.0f, drv = 1.0f;
    };

    // ---- v95: the octave + RAT-type + boost pedal (from the Life Device schematic) ----------------------------------------
    //   JFET buffer -> octave (a gain stage, then a germanium full-wave rectifier; the Octave pot mixes it in) -> op-amp distortion
    //   stage (47 ohm / 2.2 uF in parallel with 560 ohm / 4.7 uF in its feedback leg, the Distortion pot as the feedback resistor,
    //   about 1 MHz of gain-bandwidth) -> diode clipper through 1k -> Filter (1k + pot, 3.3 nF) -> JFET buffer -> Booster.
    //   The octave stage's gain and rectifier levels and the booster's gain are my estimates from the BOM; the rest is read.
    class LifeCircuit
    {
    public:
        struct Params { float dist = 0.5f, filter = 0.5f, octave = 0.0f, boost = 0.0f; int diode = 0; };
        void prepare (double sampleRate)
        {
            fs = sampleRate;
            hp1.set (highPass1 (fs, 1.0 / (6.2831853 * 47.0 * 2.2e-6)));
            hp2.set (highPass1 (fs, 1.0 / (6.2831853 * 560.0 * 4.7e-6)));
            ocDc.set (lowPass1 (fs, 5.0));
            dcOut.set (highPass1 (fs, 10.0));
            outHp.set (highPass1 (fs, 7.0));
            lastFilter = -1.0f;  lastDist = -1.0f;  lastDiode = -1;
            reset();
        }
        void reset() { hp1.reset(); hp2.reset(); ocDc.reset(); dcOut.reset(); outHp.reset(); gbw.reset(); rfLp.reset(); filt.reset(); lim.reset(); }
        void configure (const Params& p)
        {
            par = p;
            if (std::fabs (p.dist - lastDist) > 1.0e-4f)
            {
                lastDist = p.dist;
                rf = 100.0e3f * audioTaper (p.dist) + 100.0f;
                const float gHf = 1.0f + rf / 43.4f;                              // 47R in parallel with 560R at high frequencies
                gbw.set (lowPass1 (fs, std::min (0.45 * fs, 1.0e6 / (double) gHf)));
                rfLp.set (lowPass1 (fs, std::min (0.45 * fs, 1.0 / (6.2831853 * (double) rf * 100.0e-12))));
            }
            if (std::fabs (p.filter - lastFilter) > 1.0e-4f)
            {
                lastFilter = p.filter;
                const double rp = 100.0e3 * (double) audioTaper (1.0f - p.filter);
                filt.set (lowPass1 (fs, std::min (0.45 * fs, 1.0 / (6.2831853 * (1000.0 + rp) * 3.3e-9))));
            }
            if (p.diode != lastDiode)
            {
                lastDiode = p.diode;
                const DiodeSpec d = p.diode == 1 ? diodeLed() : p.diode == 2 ? diodeGermanium() : diodeSilicon();
                lim.setup (1000.0f, d, d);
            }
        }
        float process (float x)
        {
            float v = 0.9f * 1.5f * std::tanh (x / 1.5f);                         // JFET buffer
            // octave: gain stage, then full-wave rectification with germanium drops, mean removed
            float oct = 0.0f;
            if (par.octave > 0.001f)
            {
                const float u = railClip (8.9f * v, 2.2f, 4.5f);
                const float a = std::fabs (u) - 0.18f;
                const float r = 0.5f * (a + std::sqrt (a * a + 0.0025f));
                oct = 0.3f * (r - ocDc.run (r));
            }
            const float in = v + par.octave * oct;
            // op-amp stage: gain 1 + Rf * (branch admittances), each branch a high-pass
            float y = in + rf * (hp1.run (in) / 47.0f + hp2.run (in) / 560.0f);
            y = rfLp.run (gbw.run (y));
            y = railClip (y, 3.0f, 3.0f);
            y = dcOut.run (y);
            y = lim.process (y);
            y = filt.run (y);
            y = 0.9f * outHp.run (y);
            if (par.boost > 0.001f)
                y = railClip (y * (1.0f + 9.0f * par.boost * par.boost), 4.0f, 4.0f);
            return y;
        }
    private:
        double fs = 48000.0;
        Params par;
        Bq hp1, hp2, ocDc, dcOut, outHp, gbw, rfLp, filt;
        DiodeLimiter lim;
        float rf = 1000.0f, lastFilter = -1.0f, lastDist = -1.0f;  int lastDiode = -1;
    };
}
