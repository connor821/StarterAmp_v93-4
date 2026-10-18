#include "PluginProcessor.h"
#include "FactoryPresets.h"
#include "PluginEditor.h"

#include <complex>
#include <cstdint>

namespace
{
    float dbToGain (float db) { return juce::Decibels::decibelsToGain (db); }

    // How the knobs map to internal gains. Tweak these to taste.
    // style 0 = transparent (clean path + gain path), 1 = tight (almost all gain path)
    // 'style' here is the internal code: 0 Transparent, 1 Tight, 2 Open (high range), 3 Open (low range).
    // v92: fixed output trims of the five new pedals (Treble Boost, Clean Boost, Op-Amp Distortion, Stack Drive, Sustain Comp), set from measurements
    static const float odPedTrim[2] = { 0.45f, 2.0f };
    static const float odKlonTrim = 1.6f;      // v94: re-matched after the capture-fitted voicing (about 4 dB quieter at the same settings before this)
    float odGainPathLevel (float knob, int style)
    {
        if (style >= 13) return 1.0f;                            // v77 circuit modes: the real gain comes from the circuit's own resistor ratio
        if (style == 12) return 0.3f + 26.0f * knob * knob;      // v64 Tube Driver
        if (style >= 8) return 0.7f + 45.0f * knob * knob;       // v58 Distortion: everything goes through the gain stage
        if (style >= 4) return 0.25f + 28.0f * knob * knob;      // v58 Screamer
        return style == 1 ? 0.5f + 40.0f * knob * knob
             : style == 2 ? 0.35f + 30.0f * knob * knob
             : style == 3 ? 0.2f + 14.0f * knob * knob
                          : 0.2f + 25.0f * knob * knob;
    }
    float odCleanLevel (float knob, int style)
    {
        if (style >= 19) return 0.0f;                            // v91: the Distortion circuit has no clean path
        if (style >= 13) return 1.0f;
        if (style == 12) return 0.12f + 0.78f * (1.0f - knob);   // v64 Tube Driver: a lot of the clean signal stays at low Drive
        if (style >= 8) return 0.12f * (1.0f - knob);
        if (style >= 4) return 0.85f - 0.45f * knob;
        return style == 1 ? 0.25f * (1.0f - knob)
             : style == 2 ? 0.30f * (1.0f - knob)
             : style == 3 ? 0.6f * (1.0f - 0.6f * knob)
                          : 1.0f - 0.5f * knob;
    }

    // The clipper: an asinh curve. The Open style bends the curve off-centre, so the two halves of the wave clip
    // differently (even harmonics, the way a diode-to-ground clipper behaves). Styles 0 and 1 are unchanged.
    inline float odClipK (int code)      { return code == 1 ? 6.0f : code == 2 ? 3.4f : code == 3 ? 2.6f : 4.0f; }
    inline float odClipOffset (int code) { return code == 2 ? 0.35f : code == 3 ? 0.22f : 0.0f; }
    inline float odClipFn (float k, float a, float s)
    {
        return (std::asinh (k * s + a) - (a != 0.0f ? std::asinh (a) : 0.0f)) / k;
    }

    // v58: the clipper for any overdrive code. Codes 0 to 3 are the old asinh curve (unchanged). Codes 4 to 7 are the
    // Screamer (soft knee) and 8 to 11 the Distortion (hard knee), each with a clip choice: Silicon, LED, Germanium,
    // Asymmetric. A lower 'k' means the bend starts later (more headroom, louder).
    struct OdClip
    {
        float k = 4.0f, a = 0.0f, off = 0.0f;
        bool hard = false;
        inline float operator() (float s) const
        {
            if (! hard) return odClipFn (k, a, s);
            const float u = k * s + a, u2 = u * u;
            return (u / std::sqrt (std::sqrt (1.0f + u2 * u2)) - off) / k;
        }
    };
    inline OdClip odClipFor (int code)
    {
        OdClip c;
        if (code < 4) { c.k = odClipK (code); c.a = odClipOffset (code); return c; }
        if (code >= 13) { c.k = 0.08f; return c; }                  // v77 circuit modes: the clipping already happened (diodes or rails), this is just a safety bend
        if (code == 12) { c.k = 2.8f; c.a = 0.20f; return c; }      // v64 Tube Driver: a soft, slightly asymmetric tube-style curve with plenty of headroom
        static const float kSoft[4] = { 4.0f, 2.2f, 6.5f, 3.4f },  aSoft[4] = { 0.0f, 0.0f, 0.0f, 0.30f };
        static const float kHard[4] = { 5.0f, 2.6f, 7.5f, 5.0f },  aHard[4] = { 0.0f, 0.0f, 0.0f, 0.30f };
        const int ci = code & 3;
        c.hard = code >= 8;
        c.k = c.hard ? kHard[ci] : kSoft[ci];
        c.a = c.hard ? aHard[ci] : aSoft[ci];
        if (c.hard && c.a != 0.0f) c.off = c.a / std::sqrt (std::sqrt (1.0f + c.a * c.a * c.a * c.a));
        return c;
    }
    // The internal overdrive code: 0 Transparent, 1 Tight, 2/3 Open (high/low range), 4..7 Screamer, 8..11 Distortion (+ clip choice)
    // v77: the circuit modes exist for Tight (code 13) and Screamer (codes 14..18, one per clip choice incl. "None")
    inline bool odCircuitActive (int rawStyle, bool circuitParam) { return rawStyle >= 6 || (circuitParam && (rawStyle == 0 || rawStyle == 1 || rawStyle == 3 || rawStyle == 4)); }   // v94: Transparent, Tight, Screamer, Distortion are hand-tuned unless CIRCUIT MODE is on   // v93: all but Open and Tube Driver
    inline int odCodeOf (int rawStyle, bool rangeHigh, int clip, bool circuit = false)
    {
        if (rawStyle >= 6) return 14 + rawStyle;                        // 20 Op-Amp Distortion, 21 Stack Drive
        if (circuit && rawStyle == 0) return 22;                        // v93: Transparent circuit (Klon-type)
        if (circuit && rawStyle == 1) return 13;
        if (circuit && rawStyle == 3) return 14 + juce::jlimit (0, 4, clip);
        if (circuit && rawStyle == 4) return 19;                        // v91: Distortion circuit (one code; the Clipping menu picks the diodes)
        if (rawStyle == 2) return rangeHigh ? 2 : 3;
        if (rawStyle == 5) return 12;                                   // v64: Tube Driver
        if (rawStyle >= 3) return (rawStyle == 3 ? 4 : 8) + juce::jlimit (0, 3, clip);
        return rawStyle;                                                // v94: hand-tuned Transparent (0) and Tight (1) are back
    }
    // The Screamer's voicing switch changes how much gain it has (Classic 1, Mild 0.7, Open 1.3)
    inline float odVoiceScale (int rawStyle, int voice)
    {
        static const float sc[3] = { 1.0f, 0.7f, 1.3f };
        return rawStyle == 3 ? sc[juce::jlimit (0, 2, voice)] : 1.0f;
    }

    // ---------------------------------------------------------------------
    // Amp voicings. Each amp model is just a different set of numbers for the
    // same machinery: how many gain stages, how tight each stage is, how much
    // gain, how asymmetric the clipping is, how much "sag", and where the tone
    // controls sit. These are hand-tuned approximations of each amp's general
    // character, not circuit models. Edit the numbers and listen.
    // ---------------------------------------------------------------------
    struct AmpVoicing
    {
        int   stages;                 // 2 or 3 gain stages
        float hp[3];                  // high-pass before each stage (Hz): higher = tighter
        float gainMin[3], gainMax[3]; // each stage's gain at Amp Gain = 0 and = 1
        float bias[3];                // clipping asymmetry per stage (more = more even harmonics)
        float voiceHz, voiceDb, voiceQ;                    // boost/cut before the first stage
        float sag;                    // power-amp compression (0 = none)
        float bassHz,    bassOffsetDb;                     // tone stack: bass shelf
        float lowMidHz,  lowMidQ,  lowMidOffsetDb;         //             low-mid peak
        float highMidHz, highMidQ, highMidOffsetDb;        //             high-mid peak
        float trebleHz,  trebleOffsetDb;                   //             treble shelf
        float presenceHz, presenceMaxDb;                   // presence shelf: where it sits, and its
                                                           // boost (dB) with the knob fully up
        float cabLowpassHz;           // speaker roll-off used when no IR is loaded
        float trimDb;                 // level match between the models (see README: "Matching the amp volumes")
        float gainExtend = 1.0f;      // extra gain over the top of the Amp Gain knob (see stageGain); 1 = none
        float pwrAdd = 0.0f, pwrMul = 1.0f; // power tube type (v49): Power Drive is used as clamp (drive * pwrMul + pwrAdd, 0, 1)
        float stageLpHz = 0.0f;       // first-order low-pass ahead of gain stages 2 and 3 (0 = none): like the interstage roll-off
                                      // of a real amp, it keeps the fizz and the aliasing down in the high-gain amps
        PowerSection::Params ps;      // v83: circuit-derived power section, supply and transformer (off unless chosen)
        float brightCapPf = 0.0f;     // v83: bright-switch capacitor across the volume pot (0 = the classic shelf)
        float brightPotOhm = 1.0e6f;
    };

    // model: 0 Clean, 1 Vintage Chime, 2 British Hot-Rod, 3 Saturated Lead, 4 Modern High-Gain.
    // The models are ordered from least to most gain (see README: "Amp models").
    // 'clarity' is the Voicing switch. Every amp has its own alternate voicing (see the table below).

    // 'semitonesDown' only affects the Saturated Lead (3) and Modern High-Gain (4) amps. A guitar
    // tuned lower puts more energy under 100 Hz, which makes a high-gain amp flabby and mushy, so
    // for these two amps the voicing is tightened as the tuning drops (x = octaves below E standard):
    //   - every high-pass between the gain stages moves up (tighter, less flub going into the clipping)
    //   - the first stage gets a little less gain (low notes hit it harder)
    //   - less power-amp sag (low notes sag it more)
    //   - the bass and low-mid settings give a little less around 100 to 400 Hz
    //   - the pre-gain mid push goes up a little in frequency and level, to keep notes defined
    // These are my own hand-tuned numbers, by the usual rules of thumb for low-tuned high-gain playing.
    // v60: 'voicing' is the Voicing switch, now with three positions (0, 1 and 2). Positions 0 and 1 are the two voicings
    // the switch has always had. Position 2 is a new third voicing for every electric amp (the 'extra' table below).
    AmpVoicing getVoicingThree (int model, int voicing, float semitonesDown)
    {
        const bool clarity = (voicing == 1);
        static const AmpVoicing voicings[] =
        {
            // 0: Clean: lots of headroom, scooped mids, sparkly top.
            { 2, { 35.0f, 60.0f, 60.0f },  { 1.0f, 1.0f, 1.0f },  { 3.0f, 2.5f, 1.0f },  { 0.05f, 0.05f, 0.0f },
              2800.0f, 2.0f, 0.7f,   0.25f,
              100.0f, 0.0f,
              400.0f, 0.8f, -2.0f,   1500.0f, 0.8f, -1.0f,
              4000.0f, 2.0f,
              5500.0f, 8.0f,
              8000.0f,   9.0f },

            // 1: Vintage chime: two stages, moderate gain, strong
            //    asymmetry (Class A character), upper-mid "honk" and chime, medium sag.
            //    (A real amp of this type has a "Cut" control instead of Presence, so Presence
            //    only adds a small amount of air here.)
            { 2, { 90.0f, 130.0f, 100.0f },  { 1.5f, 1.5f, 1.0f },  { 9.0f, 8.0f, 1.0f },  { 0.20f, 0.18f, 0.0f },
              1800.0f, 3.0f, 0.8f,   0.45f,
              110.0f, -2.0f,
              500.0f, 0.8f, -1.0f,   2000.0f, 0.9f, 3.0f,
              4000.0f, 2.0f,
              6000.0f, 4.0f,
              7000.0f,   1.5f },

            // 2: Hot-rodded British: tight, bright, lots of upper-mid bite.
            { 3, { 120.0f, 160.0f, 110.0f }, { 3.0f, 3.0f, 1.5f }, { 18.0f, 14.0f, 5.0f }, { 0.10f, 0.08f, 0.0f },
              1300.0f, 4.0f, 0.8f,   0.20f,
              110.0f, 0.0f,
              500.0f, 0.8f, 1.0f,    1800.0f, 0.9f, 2.0f,
              3500.0f, 1.0f,
              4500.0f, 9.0f,
              6200.0f,  -6.8f },

            // 3: Saturated lead: a smooth, thick, singing high-gain
            //    sound with a forward upper-mid hump, tight but full lows, and a lot of
            //    compression. A touch less gain than the Modern High-Gain amp, but more than
            //    the British amp.
            { 3, { 95.0f, 135.0f, 100.0f }, { 3.2f, 3.2f, 2.0f },  { 19.0f, 14.0f, 5.5f }, { 0.12f, 0.10f, 0.04f },
              2000.0f, 3.5f, 0.8f,   0.50f,
              100.0f, 1.0f,
              400.0f, 0.8f, -2.0f,   1600.0f, 0.8f, 2.5f,
              3600.0f, 1.5f,
              4500.0f, 9.0f,
              5600.0f,  -3.8f },

            // 4: Modern high-gain: big lows, scooped mids, heavy sag. (v45: tighter lows, less sag, and less
            //    push around 3 to 5 kHz and above, after measuring it loose, harsh and fizzy next to the others.)
            { 3, { 95.0f, 130.0f, 100.0f }, { 3.0f, 3.0f, 2.0f },  { 20.0f, 16.0f, 6.0f }, { 0.15f, 0.12f, 0.05f },
              2400.0f, 1.0f, 0.7f,   0.50f,
              100.0f, 0.5f,
              400.0f, 0.7f, -4.0f,   1200.0f, 0.7f, -2.5f,
              3500.0f, -1.0f,
              4000.0f, 6.5f,
              5200.0f,  -1.4f },

            // 5: Modern high-gain with the Voicing switch on ("clarity"): the same
            //    amp, but the mid scoop is filled in, the lows are tighter, there's
            //    less sag, and the pre-gain push moves down into the mids.
            { 3, { 95.0f, 130.0f, 100.0f }, { 3.0f, 3.0f, 2.0f },  { 20.0f, 16.0f, 6.0f }, { 0.15f, 0.12f, 0.05f },
              1800.0f, 3.0f, 0.8f,   0.45f,
              100.0f, 1.5f,
              400.0f, 0.7f, -1.0f,   1400.0f, 0.8f, 1.5f,
              3500.0f, 1.5f,
              4000.0f, 10.0f,
              5800.0f,  -4.65f },

            // ---- Voicing switch ON, one alternate voicing for each amp (6 to 9; Modern High-Gain uses 5 above) ----

            // 6: Clean amp, voicing on = "edge push": a small old amp turned up. Much more gain
            //    and asymmetry, a mid push around 800 Hz, heavy sag, darker top, a thicker low mid.
            { 2, { 35.0f, 60.0f, 60.0f },  { 1.6f, 1.4f, 1.0f },  { 6.0f, 5.0f, 1.0f },  { 0.18f, 0.16f, 0.0f },
              800.0f, 2.5f, 0.8f,   0.65f,
              110.0f, 0.5f,
              350.0f, 0.8f, 1.5f,    1500.0f, 0.8f, 0.0f,
              3500.0f, -3.0f,
              4500.0f, 5.0f,
              6000.0f,   7.5f },

            // 7: Vintage Chime, voicing on = "treble boost": a treble booster in front of
            //    the amp. More gain, a strong push around 2.6 kHz, thinner lows, a rising top.
            { 2, { 130.0f, 170.0f, 100.0f },  { 2.5f, 2.0f, 1.0f },  { 14.0f, 11.0f, 1.2f },  { 0.22f, 0.20f, 0.0f },
              2600.0f, 5.0f, 0.9f,   0.50f,
              130.0f, -3.0f,
              600.0f, 0.8f, 0.0f,    2400.0f, 0.9f, 3.5f,
              4500.0f, 1.5f,
              6500.0f, 5.0f,
              7500.0f,   0.0f },

            // 8: British Hot-Rod amp, voicing on = "heavy": a modern compact heavy-rhythm voicing.
            //    More gain and compression, a deep but controlled low end (first stage lets the low
            //    fundamentals through, the later stages clamp them), a slightly scooped low mid, a
            //    forward upper-mid for chunk and definition, and a tamed, less fizzy top.
            { 3, { 95.0f, 175.0f, 125.0f }, { 3.6f, 3.4f, 2.0f }, { 23.0f, 17.0f, 6.0f }, { 0.12f, 0.10f, 0.03f },
              1000.0f, 3.5f, 0.8f,   0.50f,
              95.0f, 3.5f,
              420.0f, 0.8f, -1.5f,   2200.0f, 0.9f, 2.5f,
              3800.0f, 0.0f,
              4800.0f, 8.0f,
              5600.0f,  -7.3f },

            // 9: Saturated Lead, voicing on = "lead" (a modern shred-lead sound): more gain
            //    and more compression for sustain, a strong singing push around 1.1 to 1.4 kHz, a slightly
            //    lighter low end, a smoother top.
            { 3, { 95.0f, 135.0f, 100.0f }, { 3.6f, 3.4f, 2.2f }, { 22.0f, 16.0f, 6.0f }, { 0.14f, 0.12f, 0.05f },
              1100.0f, 4.5f, 0.9f,   0.55f,
              110.0f, -1.0f,
              500.0f, 0.8f, -0.5f,   1400.0f, 0.8f, 3.5f,
              3200.0f, -0.5f,
              4200.0f, 7.0f,
              5200.0f,  -5.4f },

            // 10: Classic rock crunch (the classic British hard-rock family): two stages, medium gain that breaks up
            //     around the middle of the knob and gets thick and saggy at the top, a forward upper mid, and a
            //     bright, open top. The tone stack leans on the mids more than the high-gain amps.
            { 2, { 80.0f, 110.0f, 90.0f },  { 2.0f, 1.8f, 1.0f },  { 13.0f, 10.0f, 1.0f },  { 0.14f, 0.12f, 0.0f },
              1500.0f, 2.5f, 0.8f,   0.55f,
              100.0f, 0.5f,
              450.0f, 0.8f, 0.5f,    1800.0f, 0.9f, 2.5f,
              3800.0f, 1.0f,
              5000.0f, 8.0f,
              5800.0f,  -1.0f },

            // 11: Classic rock, voicing on = "pushed": the same amp turned up with both channels linked. A third
            //     stage, more gain, a thicker mid push around 1.1 kHz, more sag, and a darker, smoother top.
            { 3, { 90.0f, 120.0f, 100.0f }, { 2.4f, 2.2f, 1.5f },  { 16.0f, 12.0f, 3.0f },  { 0.16f, 0.14f, 0.04f },
              1100.0f, 4.0f, 0.8f,   0.70f,
              105.0f, 1.5f,
              420.0f, 0.8f, 0.5f,    1600.0f, 0.9f, 3.0f,
              3400.0f, 0.0f,
              4800.0f, 6.0f,
              5400.0f,  -2.0f },

            // ---- v54: two more amps (12 to 15). Each has a voicing for the switch off and one for it on. ----

            // 12: Boutique Lead, Jazz voicing (switch off): a lot of headroom and a round, warm, smooth clean. The mids
            //     bloom a little around 700 Hz before the first stage, the top is rolled off, and it only starts to
            //     break up (softly, with even harmonics) near the top of the Gain knob.
            { 2, { 45.0f, 70.0f, 60.0f },   { 1.0f, 1.1f, 1.0f },  { 6.5f, 5.0f, 1.0f },  { 0.12f, 0.10f, 0.0f },
              700.0f, 2.0f, 0.7f,   0.35f,
              105.0f, 1.0f,
              420.0f, 0.8f, 1.0f,    1700.0f, 0.9f, -0.5f,
              3800.0f, -1.5f,
              5000.0f, 5.0f,
              6500.0f,   2.8f },

            // 13: Boutique Lead, Rock voicing (switch on): the same amp pushed. A third stage, much more gain, a thick
            //     singing mid push around 900 Hz, a firm low mid, a smooth top (a low-pass between the stages keeps the
            //     fizz off), and more sag. Long, compressed, vocal sustain rather than a tight crunch.
            { 3, { 70.0f, 110.0f, 90.0f },  { 1.6f, 1.8f, 1.0f },  { 11.0f, 9.0f, 2.4f }, { 0.16f, 0.14f, 0.04f },
              900.0f, 4.0f, 0.9f,   0.60f,
              110.0f, -0.5f,
              450.0f, 0.8f, 1.5f,    1500.0f, 0.9f, 2.5f,
              3400.0f, -3.0f,
              4500.0f, 5.0f,
              5800.0f,  -1.4f },

            // 14: Tight Metal, Raw voicing (switch off): a tight, bright, aggressive three-stage high-gain amp. Tight
            //     lows, a cut in the boxy low mids, a hard upper-mid edge and a lot of presence. Raw and a bit fizzy.
            { 3, { 115.0f, 155.0f, 110.0f }, { 3.2f, 3.0f, 1.8f }, { 21.0f, 16.0f, 6.0f }, { 0.12f, 0.10f, 0.04f },
              1200.0f, 4.0f, 0.8f,   0.25f,
              100.0f, 0.5f,
              400.0f, 0.8f, -2.5f,   2000.0f, 0.9f, 2.0f,
              3600.0f, 1.0f,
              4500.0f, 9.0f,
              6000.0f,  -4.6f },

            // 15: Tight Metal, Modern voicing (switch on): more gain and compression, a tighter and deeper low end, a
            //     deeper cut in the low mids, a smoother top (less fizz) and a more focused upper-mid.
            { 3, { 135.0f, 185.0f, 130.0f }, { 3.8f, 3.5f, 2.4f }, { 25.0f, 18.0f, 7.0f }, { 0.12f, 0.10f, 0.04f },
              1000.0f, 3.5f, 0.8f,   0.45f,
              90.0f, 2.0f,
              380.0f, 0.8f, -3.0f,   2300.0f, 0.9f, 2.5f,
              3600.0f, -0.5f,
              4600.0f, 8.0f,
              5200.0f,  -3.5f },

            // ---- v55: four more amps (parameter values 9 to 12), two voicings each (entries 16 to 23). ----

            // 16: Low-Watt Combo, voicing off = "Soft": a small, low-power, two-stage amp. Early, soft breakup with a lot of
            //     even-harmonic asymmetry, heavy sag (the supply gives way under a hard pick), a honky low-mid and upper-mid,
            //     loose lows and a rolled-off top. Cleans up a lot when you play softly.
            { 2, { 40.0f, 65.0f, 55.0f },   { 1.0f, 1.0f, 1.0f },  { 7.5f, 6.0f, 1.0f },  { 0.22f, 0.18f, 0.0f },
              900.0f, 3.0f, 0.8f,   0.60f,
              110.0f, 1.0f,
              450.0f, 0.8f, 1.5f,    1600.0f, 0.9f, 1.0f,
              3400.0f, -0.5f,
              4800.0f, 5.0f,
              6200.0f,   3.8f },

            // 17: Low-Watt Combo, voicing on = "Cranked": the same amp with the volume wide open. More gain, a thick, spitty
            //     mid-forward crunch, much more sag and bloom, and a raw, a bit fizzy edge on the top.
            { 2, { 45.0f, 75.0f, 60.0f },   { 1.4f, 1.4f, 1.0f },  { 10.0f, 8.0f, 1.0f },  { 0.26f, 0.22f, 0.0f },
              1000.0f, 4.0f, 0.8f,   0.80f,
              105.0f, 1.0f,
              450.0f, 0.8f, 1.5f,    1500.0f, 0.9f, 2.0f,
              3200.0f, -1.0f,
              4500.0f, 5.0f,
              5800.0f,   1.9f },

            // 18: Glass Clean, voicing off = "Glass": a stiff, solid-state style clean. Huge headroom, no sag, symmetric
            //     (odd-harmonic) clipping that only appears at the very top of the knob, a slightly scooped low mid and a
            //     bright, glassy, wide-open top.
            { 2, { 30.0f, 50.0f, 50.0f },   { 0.8f, 0.8f, 1.0f },  { 2.0f, 1.6f, 1.0f },  { 0.0f, 0.0f, 0.0f },
              3200.0f, 3.0f, 0.7f,   0.0f,
              100.0f, 0.0f,
              400.0f, 0.8f, -1.0f,   2500.0f, 0.8f, 0.5f,
              4500.0f, 4.5f,
              6000.0f, 9.0f,
              9500.0f,  13.1f },

            // 19: Glass Clean, voicing on = "Warm": the same clean, rounder and darker (jazz and warm-ambient tones), with
            //     a fuller low mid, a softer top and a touch of gentle compression and drive near the top of the knob.
            { 2, { 35.0f, 55.0f, 50.0f },   { 1.0f, 1.0f, 1.0f },  { 3.5f, 2.5f, 1.0f },  { 0.03f, 0.02f, 0.0f },
              1800.0f, 2.0f, 0.7f,   0.10f,
              105.0f, 1.5f,
              450.0f, 0.8f, 0.5f,    2200.0f, 0.8f, -0.5f,
              3800.0f, -1.5f,
              5000.0f, 4.0f,
              6800.0f,   6.8f },

            // 20: Creamy Lead, voicing off = "Rhythm": a tight, punchy three-stage amp for palm-muted chunk. Very tight lows,
            //     a scooped low mid, a forward upper mid and a clear, crisp top. Less fizz than the Tight Metal amp.
            { 3, { 120.0f, 165.0f, 115.0f }, { 3.0f, 2.8f, 1.8f }, { 20.0f, 15.0f, 5.5f }, { 0.10f, 0.08f, 0.03f },
              1100.0f, 3.0f, 0.8f,   0.35f,
              105.0f, 1.0f,
              420.0f, 0.8f, -3.5f,   2100.0f, 0.9f, 2.0f,
              3600.0f, -0.5f,
              4600.0f, 6.0f,
              5600.0f,  -3.7f },

            // 21: Creamy Lead, voicing on = "Lead": a creamy, vocal, very sustained lead. More gain, a big mid hump around
            //     1.4 kHz, a fuller low end, more sag, and a smooth, rounded top.
            { 3, { 95.0f, 135.0f, 100.0f }, { 3.6f, 3.3f, 2.2f }, { 23.0f, 17.0f, 6.5f }, { 0.14f, 0.12f, 0.05f },
              1400.0f, 5.0f, 0.9f,   0.50f,
              115.0f, 0.5f,
              480.0f, 0.8f, 0.5f,    1500.0f, 0.9f, 3.5f,
              3300.0f, -1.5f,
              4300.0f, 6.0f,
              5200.0f,  -5.4f },

            // 22: Doom Stack, voicing off = "Stoner": a thick, loose, fuzzy, heavily saturated amp. Huge low end with
            //     almost no tightening, a fat low mid, a dark top, lots of asymmetry (fuzz-like even harmonics) and heavy sag.
            { 3, { 40.0f, 60.0f, 55.0f },   { 2.4f, 2.2f, 1.6f },  { 16.0f, 13.0f, 5.5f }, { 0.25f, 0.20f, 0.08f },
              500.0f, 3.0f, 0.8f,   0.75f,
              95.0f, 3.0f,
              350.0f, 0.7f, 2.0f,    1300.0f, 0.8f, -1.0f,
              3000.0f, -4.0f,
              3800.0f, 3.0f,
              4600.0f,  -4.1f },

            // 23: Doom Stack, voicing on = "Wall": a huge, deep, loud wall of low end with a lot of power-amp headroom and
            //     sag. Less preamp gain, a very full bass and low mid, a dark, smooth top, and a rumbling, slow feel.
            { 3, { 32.0f, 50.0f, 45.0f },   { 1.8f, 1.6f, 1.2f },  { 10.0f, 8.0f, 3.5f },  { 0.12f, 0.10f, 0.04f },
              300.0f, 3.0f, 0.7f,   0.90f,
              80.0f, 4.0f,
              300.0f, 0.7f, 2.0f,    1000.0f, 0.8f, -1.5f,
              2800.0f, -4.5f,
              3200.0f, 2.0f,
              3800.0f,  -2.2f },
        };


        // ---- v60: the third voicing of each electric amp (position 2 of the Voicing switch). Order: Clean, Vintage Chime,
        //      Hot-Rod, Saturated Lead, Modern High-Gain, Classic Rock, Boutique Lead, Tight Metal, Low-Watt Combo,
        //      Glass Clean, Creamy Lead, Doom Stack. Hand-tuned approximations of each style's general character.
        static const AmpVoicing extraV[12] =
        {
            // Clean, "Boutique": a boutique-style clean. Fuller mids, tighter lows, more compression and a touch-sensitive chime
            //     that breaks up gently.
            { 2, { 45.0f, 70.0f, 60.0f },   { 1.2f, 1.2f, 1.0f },  { 4.5f, 4.0f, 1.0f },  { 0.12f, 0.10f, 0.0f },
              1600.0f, 2.5f, 0.8f,   0.35f,
              105.0f, 1.0f,
              450.0f, 0.8f, 1.0f,    2200.0f, 0.9f, 1.5f,
              4200.0f, 0.5f,
              5500.0f, 5.0f,
              7500.0f,   4.1f },
            // Vintage Chime, "Normal": the Normal channel without the top boost. Darker, rounder and thicker, with a more
            //     asymmetric feel and no sparkle.
            { 2, { 70.0f, 100.0f, 90.0f },  { 1.3f, 1.3f, 1.0f },  { 7.0f, 6.0f, 1.0f },  { 0.24f, 0.22f, 0.0f },
              1200.0f, 2.0f, 0.8f,   0.55f,
              100.0f, 1.5f,
              450.0f, 0.8f, 1.5f,    1800.0f, 0.9f, 0.5f,
              3600.0f, -3.0f,
              5000.0f, 2.0f,
              6000.0f,   2.6f },
            // Hot-Rod, "Brown": a thicker, saggier, mid-rich, vintage-hot-rod crunch: less scooped and looser than the other two.
            { 3, { 95.0f, 140.0f, 110.0f }, { 2.6f, 2.6f, 1.5f },  { 16.0f, 12.0f, 4.5f }, { 0.16f, 0.14f, 0.05f },
              900.0f, 3.5f, 0.8f,   0.65f,
              100.0f, 1.5f,
              450.0f, 0.8f, 2.0f,    1700.0f, 0.9f, 1.5f,
              3400.0f, -1.5f,
              4500.0f, 5.0f,
              5600.0f,   -4.3f },
            // Saturated Lead, "Crunch": the low-gain crunch channel. Open and dynamic, with the amp's thick mids but far
            //     less compression.
            { 2, { 85.0f, 120.0f, 100.0f }, { 2.0f, 2.0f, 1.0f },  { 10.0f, 8.0f, 1.0f },  { 0.14f, 0.12f, 0.0f },
              1500.0f, 3.0f, 0.8f,   0.50f,
              100.0f, 1.0f,
              400.0f, 0.8f, -1.0f,   1600.0f, 0.8f, 2.0f,
              3600.0f, 0.5f,
              4500.0f, 6.0f,
              5800.0f,   -0.2f },
            // Modern High-Gain, "Vintage": spongy tube-rectifier sag, looser and fatter lows, a softer attack and a rounder top.
            { 3, { 70.0f, 105.0f, 85.0f },  { 2.6f, 2.6f, 1.8f },  { 16.0f, 12.0f, 5.0f }, { 0.20f, 0.16f, 0.07f },
              1500.0f, 2.5f, 0.8f,   0.85f,
              95.0f, 3.0f,
              400.0f, 0.7f, -1.0f,   1300.0f, 0.7f, -1.0f,
              3300.0f, -2.5f,
              4000.0f, 4.0f,
              5000.0f,   -1.9f },
            // Classic Rock, "Jumped": both channels linked, the old way of getting a fatter, hotter sound from a vintage
            //     two-channel head. Fuller low mids, more gain and thickness, more sag, and a softer, rounder top than Crunch.
            { 2, { 80.0f, 115.0f, 95.0f },  { 2.4f, 2.2f, 1.0f },  { 12.0f, 10.0f, 1.0f }, { 0.20f, 0.18f, 0.0f },
              1100.0f, 3.5f, 0.8f,   0.55f,
              105.0f, 1.5f,
              450.0f, 0.8f, 1.0f,    1800.0f, 0.9f, 2.0f,
              3600.0f, -0.5f,
              4800.0f, 6.0f,
              6200.0f,   -1.9f },
            // Boutique Lead, "Lead": the boosted lead mode. More compression and sustain, a vocal mid hump, a smooth top.
            { 3, { 60.0f, 95.0f, 80.0f },   { 1.9f, 2.0f, 1.2f },  { 13.0f, 10.5f, 3.0f }, { 0.17f, 0.15f, 0.05f },
              1000.0f, 5.0f, 0.9f,   0.55f,
              110.0f, -0.5f,
              450.0f, 0.8f, 1.0f,    1400.0f, 0.9f, 3.5f,
              3200.0f, -3.0f,
              4500.0f, 4.0f,
              5600.0f,   -3.1f },
            // Tight Metal, "Crunch": a low-gain, palm-mute-friendly rhythm channel with the same tight lows.
            { 2, { 110.0f, 150.0f, 110.0f }, { 2.0f, 1.8f, 1.0f }, { 9.0f, 7.0f, 1.0f },  { 0.12f, 0.10f, 0.0f },
              1300.0f, 3.0f, 0.8f,   0.30f,
              100.0f, 0.0f,
              400.0f, 0.8f, -2.0f,   2000.0f, 0.9f, 2.5f,
              3600.0f, 1.0f,
              4500.0f, 7.0f,
              5800.0f,   0.2f },
            // Low-Watt Combo, "Four-ten": a 4x10-style voicing. More headroom, bigger lows, a slightly scooped low mid, a brighter top and
            //     less sag (v76: retuned so it no longer sounds like the Soft voicing).
            { 2, { 52.0f, 80.0f, 65.0f },   { 1.05f, 1.05f, 1.0f }, { 4.5f, 4.0f, 1.0f },  { 0.09f, 0.08f, 0.0f },
              550.0f, 0.5f, 0.8f,    0.38f,
              110.0f, 3.2f,
              450.0f, 0.8f, -1.0f,   2000.0f, 0.9f, 2.5f,
              3600.0f, 0.5f,
              5000.0f, 4.5f,
              7100.0f,   3.2f },
            // Glass Clean, "Funk": a tight, percussive, scooped solid-state voicing with a fast, stiff low end and a little pop.
            { 2, { 55.0f, 80.0f, 70.0f },   { 0.9f, 0.9f, 1.0f },  { 2.2f, 1.8f, 1.0f },  { 0.0f, 0.0f, 0.0f },
              3000.0f, 3.0f, 0.7f,   0.0f,
              100.0f, -1.0f,
              400.0f, 0.8f, -2.5f,   2500.0f, 0.8f, 1.5f,
              4500.0f, 4.0f,
              6000.0f, 9.0f,
              9500.0f,   11.9f },
            // Creamy Lead, "Vintage": an early-Mark-style sound. Lower gain, thick creamy mids and a smaller, sweeter low end.
            { 2, { 80.0f, 115.0f, 95.0f },  { 2.0f, 2.0f, 1.0f },  { 10.0f, 8.0f, 1.0f },  { 0.16f, 0.14f, 0.0f },
              1100.0f, 4.0f, 0.9f,   0.55f,
              110.0f, 1.0f,
              450.0f, 0.8f, 1.0f,    1500.0f, 0.9f, 2.5f,
              3200.0f, -1.5f,
              4300.0f, 5.0f,
              5400.0f,   -0.9f },
            // Doom Stack, "Bass stack": a big bass-amp-style stack. Massive low mids, a cleaner and tighter top, huge headroom,
            //     and a grind that only appears when you push it.
            { 2, { 35.0f, 55.0f, 50.0f },   { 1.3f, 1.2f, 1.0f },  { 7.0f, 5.5f, 1.0f },  { 0.10f, 0.08f, 0.0f },
              350.0f, 3.0f, 0.7f,   0.80f,
              90.0f, 3.5f,
              300.0f, 0.7f, 2.5f,    1000.0f, 0.8f, -1.0f,
              2800.0f, -3.5f,
              3400.0f, 2.0f,
              4400.0f,   1.2f },
        };

        // ---- v61: two new amps (parameter values 13 and 14), three voicings each (entries 0..2 = model 13, 3..5 = model 14).
        //      13 Precision High-Gain: a modern boutique high-gain amp: very tight, articulate and clear, a firm low end, a
        //         focused upper mid and a smooth, detailed top. Rhythm = Tight, Lead = Lead, Crunch = Crunch.
        //      14 Digital Extreme: a modern digital-style amp for extreme styles: brutally tight, very high gain, little sag
        //         and clean, hard, symmetrical clipping. Thall = Thall, Industrial = Industrial, Cavern = Cavern.
        //      Hand-tuned approximations of the general character of these kinds of amp.
        static const AmpVoicing hiV[6] =
        {
            // 13, "Tight": the main high-gain channel. Tight lows, a scooped low mid, a forward 1.8 kHz bite and a firm
            //     but smooth top. Very articulate when palm muted, with enough sag to bloom on held notes.
            { 3, { 120.0f, 165.0f, 115.0f }, { 3.4f, 3.2f, 2.1f }, { 23.0f, 17.0f, 6.0f }, { 0.13f, 0.11f, 0.04f },
              1200.0f, 3.5f, 0.8f,   0.40f,
              95.0f, 1.5f,
              400.0f, 0.8f, -2.5f,   1800.0f, 0.9f, 2.5f,
              3600.0f, 0.0f,
              4600.0f, 7.0f,
              5800.0f,   -4.15f },
            // 13, "Lead": the boosted lead mode. More gain than Tight, a focused upper-mid push near 1.9 kHz, less sag and a
            //     more open top so single notes stay defined (v76: retuned so it no longer sounds like the Creamy Lead amp).
            { 3, { 120.0f, 165.0f, 118.0f }, { 3.7f, 3.45f, 2.4f }, { 24.0f, 17.5f, 6.2f }, { 0.11f, 0.09f, 0.035f },
              1750.0f, 4.5f, 0.95f,  0.40f,
              100.0f, 0.5f,
              415.0f, 0.8f, -1.8f,   1900.0f, 0.95f, 3.8f,
              3550.0f, -0.25f,
              4650.0f, 6.0f,
              6100.0f,   -4.80f },
            // 13, "Crunch": an open, dynamic, lower-gain channel. Cleans up with the guitar's volume knob, with a tight
            //     enough low end for fast rhythm playing and a little more room in the lows.
            { 2, { 90.0f, 130.0f, 100.0f }, { 2.4f, 2.2f, 1.0f }, { 13.0f, 10.0f, 1.0f }, { 0.12f, 0.10f, 0.0f },
              1500.0f, 3.0f, 0.8f,   0.35f,
              100.0f, 1.0f,
              420.0f, 0.8f, -1.0f,   1900.0f, 0.9f, 3.0f,
              3800.0f, 0.5f,
              4800.0f, 7.0f,
              6200.0f,   -2.20f },
            // 14, "Thall": ultra-tight, very high gain with almost no sag. A firm sub-low, a deep cut at 350 Hz, hard
            //     symmetrical clipping and a sharp 2.4 kHz edge. Chugs stay pinned and clear at low tunings.
            { 3, { 175.0f, 235.0f, 165.0f }, { 4.4f, 4.0f, 2.8f }, { 30.0f, 22.0f, 8.5f }, { 0.04f, 0.03f, 0.0f },
              900.0f, 2.5f, 0.8f,   0.10f,
              80.0f, 3.5f,
              350.0f, 0.8f, -4.5f,   2400.0f, 1.0f, 3.0f,
              3800.0f, -0.5f,
              5000.0f, 7.0f,
              5200.0f,   -5.30f },
            // 14, "Industrial": a harsh, gritty, wide-open voicing. Fuzzier clipping with some even harmonics, strong upper
            //     mids, more top-end fizz and a mid-forward low mid, for aggressive, abrasive riffs and sound-design leads.
            { 3, { 110.0f, 160.0f, 120.0f }, { 4.2f, 4.0f, 3.0f }, { 28.0f, 22.0f, 9.0f }, { 0.22f, 0.18f, 0.10f },
              1800.0f, 4.0f, 0.8f,   0.20f,
              100.0f, 1.0f,
              500.0f, 0.7f, 0.5f,    2000.0f, 0.8f, 5.0f,
              3400.0f, 3.5f,
              4800.0f, 10.0f,
              7500.0f,   -7.80f },
            // 14, "Cavern": a huge, slow, downtuned wall. Loose-ish giant lows, a dark smooth top, a lot of sag and
            //     headroom behind the clipping. Built for 7- and 8-string and drop-tuned doom and slam.
            { 3, { 70.0f, 100.0f, 80.0f },   { 3.0f, 3.0f, 2.2f }, { 22.0f, 18.0f, 7.0f }, { 0.12f, 0.10f, 0.04f },
              400.0f, 3.0f, 0.7f,   0.70f,
              70.0f, 4.0f,
              300.0f, 0.7f, 1.5f,    1000.0f, 0.8f, -1.5f,
              2800.0f, -3.0f,
              3400.0f, 3.0f,
              4400.0f,   -4.60f },
        };
        static const float hiExt[6] = { 2.6f, 2.8f, 1.6f, 3.2f, 3.0f, 2.2f };
        static const float hiLp[6]  = { 6000.0f, 6600.0f, 0.0f, 5400.0f, 9500.0f, 4600.0f };


        // ---- v62: the Master-Volume Rock amp (parameter value 15), split out of the Classic Rock amp, three voicings.
        //      Stock = the 80s master-volume voicing that was the Classic Rock amp's third position in v60 and v61 (unchanged).
        //      Modded = a modified version: an extra cascaded gain stage, tighter, smoother, more compressed and more saturated.
        //      Cranked = the master volume wide open: the power stage does the clipping, so it is looser, saggier and louder.
        static const AmpVoicing mvV[3] =
        {
            { 3, { 105.0f, 145.0f, 115.0f }, { 2.8f, 2.6f, 1.6f }, { 18.0f, 13.0f, 4.0f }, { 0.12f, 0.10f, 0.03f },
              1800.0f, 3.5f, 0.9f,   0.35f,
              105.0f, 0.0f,
              500.0f, 0.8f, -0.5f,   2000.0f, 0.9f, 3.5f,
              3800.0f, 1.0f,
              5000.0f, 8.0f,
              6200.0f,   -5.0f },
            { 3, { 120.0f, 170.0f, 120.0f }, { 3.4f, 3.2f, 2.2f }, { 22.0f, 16.0f, 5.5f }, { 0.14f, 0.12f, 0.04f },
              1500.0f, 4.0f, 0.9f,   0.30f,
              100.0f, 0.5f,
              450.0f, 0.8f, -1.5f,   1900.0f, 0.9f, 3.0f,
              3500.0f, 0.5f,
              4800.0f, 7.0f,
              5800.0f,   -5.6f },
            { 2, { 85.0f, 120.0f, 100.0f }, { 2.2f, 2.0f, 1.0f },  { 12.0f, 9.0f, 1.0f },  { 0.18f, 0.15f, 0.0f },
              1400.0f, 3.0f, 0.8f,   0.70f,
              100.0f, 1.5f,
              450.0f, 0.8f, 0.5f,    1800.0f, 0.9f, 2.5f,
              3600.0f, -0.5f,
              4600.0f, 6.0f,
              5800.0f,   -1.0f },
        };
        static const float mvExt[3] = { 2.2f, 2.4f, 1.6f };
        static const float mvLp[3]  = { 6000.0f, 6000.0f, 0.0f };

        static const float extraExt[12] = { 1.5f, 1.5f, 1.8f, 1.8f, 2.0f, 2.0f, 2.0f, 1.8f, 1.4f, 1.0f, 1.6f, 1.4f };
        static const float extraLp[12]  = { 0.0f, 0.0f, 5400.0f, 0.0f, 5400.0f, 0.0f, 4800.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        static const int   extraOf[13]  = { 0, 1, 2, 3, 4, -1, 5, 6, 7, 8, 9, 10, 11 };   // model value -> entry (5 = Acoustic has none)
        const int exIdx = (voicing >= 2 && model >= 0 && model <= 12) ? extraOf[model] : -1;

        const int  modelIn = model;          // the amp model value, before the lookups below change 'model'
        const bool classic = (model == 6);   // the Classic Rock amp (parameter value 6)
        const bool boutique = (model == 7), tightMetal = (model == 8);     // v54: parameter values 7 and 8
        const bool newAmp = (model >= 9 && model <= 12);                    // v55: Low-Watt Combo, Glass Clean, Creamy Lead, Doom Stack
        const int  newIdx = newAmp ? 16 + (model - 9) * 2 + (clarity ? 1 : 0) : -1;
        if (model >= 5 && ! boutique && ! tightMetal && ! newAmp) model = 0;     // Acoustic: neutral tone stack (the Clean amp's)
        static const int voicedIndex[5] = { 6, 7, 8, 9, 5 };   // the entry used when the Voicing switch is on
        model = juce::jlimit (0, 8, model);
        AmpVoicing v = newAmp ? voicings[newIdx]
                     : (boutique || tightMetal) ? voicings[(boutique ? 12 : 14) + (clarity ? 1 : 0)]
                     : classic ? voicings[clarity ? 11 : 10] : voicings[clarity ? voicedIndex[juce::jlimit (0, 4, model)] : juce::jlimit (0, 4, model)];

        if (exIdx >= 0)
            v = extraV[exIdx];
        const bool hiAmp = (modelIn == 13 || modelIn == 14);                // v61: Precision High-Gain, Digital Extreme
        const int  hiIdx = hiAmp ? (modelIn - 13) * 3 + juce::jlimit (0, 2, voicing) : -1;
        if (hiAmp)
            v = hiV[hiIdx];
        const bool mvAmp = (modelIn == 15);                                  // v62: Master-Volume Rock
        const int  mvIdx = mvAmp ? juce::jlimit (0, 2, voicing) : -1;
        if (mvAmp)
        {
            v = mvV[mvIdx];
            if (mvIdx == 2) v.pwrAdd = 0.30f;                                // Cranked: the power stage is already working
        }

        // Extra gain over the top of the knob (from v39), more on the higher-gain amps. Nothing changes below
        // about 55% of the knob, so older settings keep their sound; the last part of the travel adds up to this
        // much more gain to the first stage (less to the later ones), which is the extra saturation, sustain
        // and compression for modern metal and djent rhythm tones.
        static const float extendFor[5] = { 1.4f, 1.6f, 2.2f, 2.4f, 2.8f };
        if (mvAmp)            { v.gainExtend = mvExt[mvIdx];     v.stageLpHz = mvLp[mvIdx]; }
        else if (hiAmp)       { v.gainExtend = hiExt[hiIdx];     v.stageLpHz = hiLp[hiIdx]; }
        else if (exIdx >= 0)  { v.gainExtend = extraExt[exIdx];  v.stageLpHz = extraLp[exIdx]; }
        else if (boutique)    { v.gainExtend = clarity ? 1.7f : 1.3f;  v.stageLpHz = clarity ? 5200.0f : 0.0f; }
        else if (tightMetal)  { v.gainExtend = clarity ? 2.8f : 2.6f;  v.stageLpHz = clarity ? 5200.0f : 7000.0f; }
        else if (newAmp)
        {
            static const float ext[8] = { 1.2f, 1.6f, 1.0f, 1.0f, 2.4f, 3.0f, 2.0f, 1.3f };
            static const float lp[8]  = { 0.0f, 0.0f, 0.0f, 0.0f, 6500.0f, 5200.0f, 4800.0f, 4200.0f };
            v.gainExtend = ext[newIdx - 16];  v.stageLpHz = lp[newIdx - 16];
        }
        else
        {
            v.gainExtend = classic ? 1.9f : extendFor[model];
            v.stageLpHz  = (! classic && model >= 2 && model <= 4) ? 6000.0f : 0.0f;   // the three high-gain amps (both voicings)
        }

        if ((! classic && ! boutique && ! newAmp && (model == 3 || model == 4)) || tightMetal || (newAmp && modelIn == 11) || hiAmp)   // (v60: this used to test 'model', which had been changed to 8 above, so Focus did nothing on the Creamy Lead)
        {
            if (semitonesDown > 0.01f)
            {
                const float x = semitonesDown / 12.0f;
                const float tighten = std::pow (2.0f, (modelIn == 14 ? 0.4f : 0.8f) * x);   // v61: the Digital Extreme amp is already very tight, so Focus tightens it half as much
                for (float& hz : v.hp)
                    hz *= tighten;
                v.gainMin[0] *= dbToGain (-2.5f * x);
                v.gainMax[0] *= dbToGain (-2.5f * x);
                v.sag *= 1.0f - 0.4f * x;
                v.bassOffsetDb   -= 3.0f * x;
                v.lowMidOffsetDb -= 3.0f * x;
                v.voiceHz *= 1.0f + 0.5f * x;
                v.voiceDb += 1.5f * x;
            }
        }
        return v;
    }

    // ---- v63: a fourth voicing for every electric amp (Voicing menu position 4). Each one is one of the amp's three voicings with
    //      a set of changes on top (gain, tightness, sag, tone offsets, top-end limit). The trims are measured so that
    //      at Gain 0.5 each is about as loud as the amp's first voicing.
    struct FourthDelta
    {
        int   base;            // which of the amp's own voicings (0 to 2) it starts from
        float gain;            // multiplies the first two stages' gain
        float hp;              // multiplies the high-passes between stages (above 1 = tighter)
        float sag;             // multiplies the power-amp sag
        float bass, lowMid, highMid, treble, presence;   // dB offsets (presence is the most the Presence knob adds)
        float voiceDb, voiceHz;                          // pre-gain mid push: dB added, frequency multiplier
        float ext;             // added to the extra top-of-knob gain
        float lpHz;            // if above 0, the limit on the top end between stages (Hz)
        float bias;            // added to the bias of the first two stages
    };
    inline AmpVoicing getVoicing (int model, int voicing, float semitonesDown)
    {
        if (voicing < 3)
            return getVoicingThree (model, voicing, semitonesDown);

        static const FourthDelta d[16] =
        {
            { 0, 0.8f, 0.8f, 0.5f,   1.0f, -1.5f,  0.5f,  1.5f,  2.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  0 Clean: Studio (hi-fi, more headroom)
            { 0, 0.8f, 1.0f, 0.6f,   0.0f, -1.5f,  0.0f,  2.0f,  2.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  1 Vintage Chime: Jangle
            { 0, 1.0f, 1.0f, 1.0f,  -1.0f,  1.5f,  2.5f, -1.0f,  0.0f,   1.5f, 0.8f,  0.0f,    0.0f, 0.0f },   //  2 Hot-Rod: Mid push
            { 1, 1.25f, 1.0f, 1.3f,  0.0f,  0.0f,  0.0f, -1.5f, -1.0f,   0.0f, 1.0f,  0.4f, 5200.0f, 0.0f },   //  3 Saturated Lead: Sustain
            { 1, 1.1f, 1.25f, 1.0f,  1.0f, -1.0f,  1.5f,  0.5f,  0.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  4 Modern High-Gain: Chunk
            { 0, 1.0f, 1.0f, 1.0f,   0.0f,  0.0f,  0.0f,  0.0f,  0.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  5 (Acoustic: none)
            { 0, 1.25f, 0.9f, 1.2f, -0.5f,  0.0f,  0.0f,  1.0f,  1.5f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  6 Classic Rock: Bright
            { 0, 1.2f, 1.0f, 1.2f,   0.0f,  0.0f,  1.5f, -0.5f,  0.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  7 Boutique Lead: Blues
            { 1, 1.15f, 1.15f, 1.0f, 0.0f, -2.0f,  2.0f,  1.5f,  0.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   //  8 Tight Metal: Buzz
            { 1, 1.3f, 1.0f, 1.3f,   0.5f,  0.0f,  0.0f,  0.0f, -1.0f,   0.0f, 1.0f,  0.0f, 4800.0f, 0.0f },   //  9 Low-Watt Combo: Gritty
            { 0, 1.0f, 1.0f, 0.5f,   0.0f, -2.0f,  1.0f,  3.0f,  3.0f,   0.0f, 1.3f,  0.0f,    0.0f, 0.0f },   // 10 Glass Clean: Shimmer
            { 1, 1.15f, 1.0f, 1.2f,  0.0f,  1.0f, -0.5f, -1.0f,  0.0f,   0.0f, 1.0f,  0.0f, 5000.0f, 0.0f },   // 11 Creamy Lead: Singing
            { 0, 1.4f, 1.0f, 1.3f,   0.0f,  1.0f, -2.0f, -2.0f,  0.0f,   0.0f, 1.0f,  0.0f, 4200.0f, 0.05f },  // 12 Doom Stack: Fuzz
            { 2, 1.25f, 1.1f, 0.7f,  1.0f,  0.5f,  0.5f,  2.0f,  2.0f,   0.0f, 1.0f,  0.3f,    0.0f, 0.02f },  // 13 Precision: Polished (v76: now built on Crunch, so it no longer duplicates Tight)
            { 0, 1.2f, 0.85f, 1.0f,  0.0f,  2.0f,  1.0f,  0.0f,  0.0f,   0.0f, 1.0f,  0.0f,    0.0f, 0.0f },   // 14 Digital Extreme: Lead
            { 0, 1.35f, 1.2f, 0.9f,  0.0f,  1.0f,  2.0f,  0.0f,  0.0f,   0.0f, 1.0f,  0.3f,    0.0f, 0.0f },   // 15 Master-Volume Rock: Boosted
        };
        static const float trim[16] = { 3.4f, 2.0f, -1.1f, 1.7f, 1.0f, 0.0f, -0.8f, -1.7f, 1.1f, -0.9f, 0.9f, 0.2f, 0.3f, -3.4f, -1.1f, -1.1f };   // measured (see the Auto Level table below)
        const int m = juce::jlimit (0, 15, model);
        const auto& q = d[m];
        AmpVoicing v = getVoicingThree (model, q.base, semitonesDown);
        for (int i = 0; i < 2; ++i)
        {
            v.gainMin[i] *= q.gain;  v.gainMax[i] *= q.gain;
            v.bias[i] += q.bias;
        }
        for (float& hz : v.hp)
            hz *= q.hp;
        v.sag *= q.sag;
        v.bassOffsetDb += q.bass;  v.lowMidOffsetDb += q.lowMid;  v.highMidOffsetDb += q.highMid;
        v.trebleOffsetDb += q.treble;  v.presenceMaxDb = std::max (0.0f, v.presenceMaxDb + q.presence);
        v.voiceDb += q.voiceDb;  v.voiceHz *= q.voiceHz;
        v.gainExtend += q.ext;
        if (q.lpHz > 0.0f)
            v.stageLpHz = v.stageLpHz > 0.0f ? std::min (v.stageLpHz, q.lpHz) : q.lpHz;
        v.trimDb += trim[m];
        return v;
    }

    // The "Modern Metal" controls, applied on top of the amp's voicing for the British Hot-Rod (2),
    // Saturated Lead (3) and Modern High-Gain (4) amps only:
    //   Tight     : raises every high-pass between the gain stages (up to about 2.8x), so palm mutes stay clear
    //   Mid Hz    : slides the pre-gain mid push (0.4x to 2.5x of the amp's own frequency; 0.5 = unchanged)
    //   Mid Boost : adds up to 8 dB to that push
    //   Sag       : scales the power-amp compression (0.5 = unchanged, 0 = none, 1 = double)
    //   Lead      : more gain in the first stages, a little more push and sag, and a level trim to match
    struct MetalSettings { float tight, midHz, midBoost, sag; bool lead; };
    inline bool hasMetalControls (int ampModelValue) { return (ampModelValue >= 2 && ampModelValue <= 4) || ampModelValue == 8 || ampModelValue == 11 || ampModelValue == 13 || ampModelValue == 14; }
    // Amp B's menu has no Acoustic entry, so its positions differ from the parameter values from Classic Rock on:
    // 0 to 4 are the same, 5 = Classic Rock (6), 6 = Boutique Lead (7), 7 = Tight Metal (8), 8 to 11 = the four v55 amps (9 to 12).
    inline int ampBModelValue (int menuIndex) { return menuIndex <= 4 ? juce::jmax (0, menuIndex) : juce::jmin (menuIndex, 14) + 1; }

    AmpVoicing metalVoicing (int model, int voicing, float semitonesDown, const MetalSettings& m)
    {
        AmpVoicing v = getVoicing (model, voicing, semitonesDown);
        if ((model < 2 || model > 4) && model != 8 && model != 11 && model != 13 && model != 14)         // v54/v55/v61: the Tight Metal (8) and Creamy Lead (11) amps have them too
            return v;

        const float t = std::pow (2.0f, 1.5f * m.tight);
        for (float& hz : v.hp)
            hz *= t;
        v.voiceHz = juce::jlimit (150.0f, 6000.0f, v.voiceHz * std::pow (2.5f, (m.midHz - 0.5f) * 2.0f));
        v.voiceDb += 8.0f * m.midBoost;
        v.sag *= 2.0f * m.sag;
        if (m.lead)
        {
            v.gainMin[0] *= 1.4f;  v.gainMax[0] *= 1.4f;
            v.gainMax[1] *= 1.2f;
            v.voiceDb += 2.0f;
            v.sag *= 1.3f;
            v.trimDb -= 2.5f;
        }
        return v;
    }

    // Stage gain for a given Amp Gain knob position (smooth, geometric curve).
    float stageGain (const AmpVoicing& v, int stage, float knob)
    {
        const float base = v.gainMin[stage] * std::pow (v.gainMax[stage] / v.gainMin[stage], knob);
        const float t = juce::jlimit (0.0f, 1.0f, (knob - 0.55f) / 0.45f);
        const float eased = t * t * (3.0f - 2.0f * t);                       // smooth start at 55%
        static const float share[3] = { 1.0f, 0.8f, 0.4f };                 // how much of the extension each stage gets
        return base * std::pow (v.gainExtend, eased * share[stage]);
    }

    // Amp realism controls (v41), applied on top of the amp's voicing:
    //   Bias    : above 0 = hotter (more asymmetric clipping, more even harmonics); below 0 = colder (crossover
    //             distortion in the power stage, a gritty "sputter" on low-level notes; see powerStage)
    //   Sag     : scales the power-amp compression of any amp (0.5 = as voiced)
    //   Tube    : the preamp tube type: high-gain, medium or low-gain (lower gain, softer)
    //   Rectifier: tube = full sag, silicon = tighter and firmer
    struct Realism { float bias, sagAmt; int tube, rect; int pwr = 0; float tubeTrimDb = 0.0f;
                     int feel = 1, watt = 0, stack = 0, cls = 0; bool deep = false;
                     int derived = 0, model = -1; };   // v83: 0 classic, 1 power section, 2 + supply and transformer, 3 + component-value voicing   // v59 amp-wide switches (all neutral by default)

    // Power tube type (v49): 0 As voiced (no change), 1 EL34, 2 6L6, 3 6V6, 4 EL84, 5 KT88. Hand-tuned
    // approximations of the general character of each tube, not measurements of any amp.
    struct PowerTube { float add, mul, sag, bass, lowMid, highMid, treble, presence, trimDb; };
    inline const PowerTube& powerTubeFor (int i)
    {
        static const PowerTube t[6] = {
            { 0.00f, 1.0f, 1.00f,  0.0f,  0.0f,  0.0f,  0.0f,  0.0f, 0.0f },   // as voiced
            { 0.10f, 1.0f, 1.00f,  0.0f,  0.0f,  1.5f,  0.0f,  1.0f, 0.5f },   // EL34: mid-forward, crunchy, medium sag
            { 0.00f, 0.7f, 0.80f,  1.0f,  0.0f, -1.0f,  0.5f,  0.0f, -0.3f },   // 6L6: tight, clean headroom, scooped
            { 0.25f, 1.0f, 1.40f,  0.0f,  0.5f,  0.0f, -1.0f, -2.0f, 0.8f },   // 6V6: soft, early breakup, lots of sag
            { 0.20f, 1.0f, 1.20f, -0.5f, -0.5f,  2.0f,  1.0f,  1.5f, 0.45f },   // EL84: chimey, bright, compressed
            { 0.00f, 0.5f, 0.65f,  1.5f,  0.0f,  0.0f,  0.0f,  0.0f, -0.8f } }; // KT88: big, tight, lots of headroom
        return t[juce::jlimit (0, 5, i)];
    }
    inline float effectivePower (float drive, const AmpVoicing& v)
    {
        return juce::jlimit (0.0f, 1.0f, drive * v.pwrMul + v.pwrAdd);
    }

    // v83: loudness compensation of the power-section responses (measured, see MODELED_GEAR.md) and the calibration of the
    // supply model against the classic sag (measured: average gain reduction over the amps at the default settings).
    // level: 1 power section, 2 supply and transformer, 3 component voicing (each is the extra trim of that level)
    inline float derivedTrimDb (int model, int level)
    {
        //                              0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15
        static const float l1[16] = { -1.20f,-1.60f,-1.30f,-1.70f,-1.80f,0.00f,-1.30f,-1.70f,-1.70f,-2.10f,0.00f,-0.50f,-2.60f,-1.60f,0.00f,-1.40f };
        static const float l2[16] = { 0.30f,0.80f,1.20f,0.10f,0.40f,0.00f,-0.40f,0.50f,1.40f,-0.30f,0.00f,0.60f,0.10f,0.80f,0.00f,0.70f };
        static const float l3[16] = { 0.00f,0.00f,-0.70f,0.00f,0.20f,0.00f,0.10f,0.00f,0.00f,1.90f,0.00f,-1.60f,0.00f,0.00f,0.00f,-1.00f };
        const int m = juce::jlimit (0, 15, model);
        return level == 1 ? l1[m] : level == 2 ? l2[m] : l3[m];
    }
    constexpr float kSupplyCal = 1.0f;

    // v83: preamp numbers worked out from component values, for the amps whose schematics I have read with confidence.
    //  - Cathode bypass: a resistor Rk with a capacitor Ck (zero 1/(2 pi Rk Ck)) in front of a stage with plate resistance
    //    ri/(mu+1) lifts the gain above its pole (Rk parallel that, with Ck); I use the geometric mean of the two as the corner.
    //  - Coupling: capacitor into the grid leak resistor, a first-order high-pass at 1/(2 pi R C).
    //  - Bright cap across the volume pot: a shelf from the zero at 1/(2 pi Rtop C) up to the pole, with a boost of 1/wiper.
    inline float bypassCornerHz (float rk, float ck, float riOverMuPlus1)
    {
        const float fz = 1.0f / (6.2831853f * rk * ck);
        const float rp = rk * riOverMuPlus1 / (rk + riOverMuPlus1);
        const float fp = 1.0f / (6.2831853f * rp * ck);
        return std::sqrt (fz * fp);
    }
    inline float couplingCornerHz (float r, float c) { return 1.0f / (6.2831853f * r * c); }

    // Bright switch: classic is a fixed shelf at 2.8 kHz that fades as the gain goes up. With a bright capacitor value (v83) the
    // shelf comes from the cap across the top of the volume pot: boost 1/wiper (audio taper, wiper = knob squared) from
    // the zero 1/(2 pi Rtop C) up to the pole where it flattens out.
    inline void derivedBright (const AmpVoicing& v, float gainKnob, float& hz, float& db)
    {
        if (v.brightCapPf <= 0.0f) return;
        const float w = juce::jlimit (0.02f, 0.98f, gainKnob * gainKnob);
        const float fz = 1.0f / (6.2831853f * (1.0f - w) * v.brightPotOhm * v.brightCapPf * 1.0e-12f);
        const float fp = fz / w;
        hz = juce::jlimit (300.0f, 9000.0f, std::sqrt (fz * fp));
        db = juce::jmin (14.0f, -20.0f * std::log10 (w) * 0.5f);      // half the full boost, as the shelf is centred in its transition
    }

    void applyComponentVoicing (AmpVoicing& v, int model)
    {
        if (model == 0)            // American combo: 1.5k cathodes with 25 uF, 0.02 uF into 1M volume pots
        {
            v.hp[0] = bypassCornerHz (1500.0f, 25.0e-6f, 1600.0f);
            v.hp[1] = couplingCornerHz (1.0e6f, 0.02e-6f);
            v.brightCapPf = 120.0f;  v.brightPotOhm = 1.0e6f;
        }
        else if (model == 4)       // Modern High-Gain, read from a 1990s dual-channel high-gain schematic: 1.8k / 1 uF cathodes on every stage
        {
            v.hp[0] = bypassCornerHz (1800.0f, 1.0e-6f, 1600.0f);
        }
        else if (model == 11)      // Creamy Lead, read from a Mark-series rhythm channel: first stage 1.6k / 0.47 uF cathode, 120 pF bright cap across a 1M pot
        {
            v.hp[0] = bypassCornerHz (1600.0f, 0.47e-6f, 1600.0f);
            v.brightCapPf = 120.0f;  v.brightPotOhm = 1.0e6f;
        }
        else if (model == 3)       // Saturated Lead, read from an SLO-100 lead channel: V1b and V2a both 1.8k / 1 uF cathodes (220k and 100k plates), 0.02 uF couplings
        {
            v.hp[0] = bypassCornerHz (1800.0f, 1.0e-6f, 1600.0f);
            v.hp[1] = bypassCornerHz (1800.0f, 1.0e-6f, 1600.0f);
        }
        else if (model == 9)       // Low-Watt Combo, read from a tweed Deluxe (5E3): V1A 820 ohm / 25 uF (12AY7), V2A 1.5k / 25 uF, 0.1 uF and 0.022 uF couplings into 1M (below 10 Hz)
        {
            v.hp[0] = bypassCornerHz (820.0f, 25.0e-6f, 570.0f);
            v.hp[1] = bypassCornerHz (1500.0f, 25.0e-6f, 650.0f);
            v.hp[2] = couplingCornerHz (220.0e3f, 0.1e-6f);
            for (float& hz : v.hp) hz = juce::jmax (hz, 25.0f);     // below this the float filters at 4x oversampling are not well conditioned
            v.bassOffsetDb = v.lowMidOffsetDb = v.highMidOffsetDb = v.trebleOffsetDb = 0.0f;   // the tweed tone network (TweedTone) supplies the shape
        }
        else if (model == 2 || model == 6 || model == 15)   // British 50 W: 2.7k / 0.68 uF first stage, 0.022 uF into 470k
        {
            v.hp[0] = bypassCornerHz (2700.0f, 0.68e-6f, 1600.0f);
            v.hp[1] = couplingCornerHz (470.0e3f, 0.022e-6f);
            v.hp[2] = couplingCornerHz (470.0e3f, 0.022e-6f);
        }
    }

    void applyRealism (AmpVoicing& v, const Realism& r)
    {
        const float hot = std::max (0.0f, r.bias);
        for (int i = 0; i < 3; ++i)
            v.bias[i] = v.bias[i] * (1.0f + 1.5f * hot) + 0.05f * hot * (i < 2 ? 1.0f : 0.5f);
        static const float tubeScale[3] = { 1.0f, 0.72f, 0.5f };
        const float ts = tubeScale[juce::jlimit (0, 2, r.tube)];
        v.gainMin[0] *= ts;  v.gainMax[0] *= ts;
        v.gainMin[1] *= ts;  v.gainMax[1] *= ts;
        v.gainMin[2] *= std::sqrt (ts);  v.gainMax[2] *= std::sqrt (ts);
        v.sag *= 2.0f * r.sagAmt;
        v.trimDb += r.tubeTrimDb;
        if (r.pwr > 0)
        {
            const auto& t = powerTubeFor (r.pwr);
            v.pwrAdd = t.add;  v.pwrMul = t.mul;  v.sag *= t.sag;
            // v85: with the derived power section the tube's tone offsets would count the same thing twice
            const bool derivedPower = r.derived >= 1 && PowerSection::familyForModel (r.model) >= 0;
            if (! derivedPower)
            {
                v.bassOffsetDb += t.bass;  v.lowMidOffsetDb += t.lowMid;  v.highMidOffsetDb += t.highMid;
                v.trebleOffsetDb += t.treble;  v.presenceMaxDb += t.presence;
            }
            v.trimDb += t.trimDb;
        }

        // ---- v59 amp-wide switches. Each is a set of changes to the voicing, and each has a small level
        //      compensation (measured as the average over twelve amps at three gain settings) so the switch changes the feel and tone, not the loudness.
        // Feel (power-amp feedback): Loose = less negative feedback (more sag, looser and fuller lows, less
        // presence effect); Tight = more feedback (stiffer, faster, tighter lows, more presence effect).
        const bool dPow = r.derived >= 1 && PowerSection::familyForModel (r.model) >= 0;     // v83: the feedback is modelled, not nudged
        if (r.feel == 0)
        {
            v.sag *= 1.35f;
            if (! dPow) { v.pwrAdd += 0.08f; v.bassOffsetDb += 1.0f;  v.presenceMaxDb = std::max (0.0f, v.presenceMaxDb - 2.0f); }
        }
        else if (r.feel == 2)
        {
            v.sag *= 0.6f;
            if (! dPow)
            {
                v.bassOffsetDb -= 0.8f;  v.presenceMaxDb += 1.5f;
                for (int i = 0; i < 3; ++i) v.hp[i] *= 1.12f;
            }
        }
        // Tone stack: Neutral takes most of the amp's own baked-in tone shape out (the passive stacks are solved elsewhere).
        if (r.stack == 1)
        {
            v.bassOffsetDb *= 0.3f;  v.lowMidOffsetDb *= 0.3f;  v.highMidOffsetDb *= 0.3f;  v.trebleOffsetDb *= 0.3f;
        }
        // Deep: a low-end boost into the first stage and the tone stack
        if (r.deep) { v.bassOffsetDb += 2.5f;  v.hp[0] *= 0.85f; }
        {
            static const float feelTrim[3]  = { 1.1f, 0.0f, -0.6f };
            v.trimDb += (dPow ? 0.0f : feelTrim[juce::jlimit (0, 2, r.feel)]) + (r.deep ? -0.5f : 0.0f);
        }

        // ---- v83: circuit-derived options (see MODELED_GEAR.md). All off at 0.
        if (r.derived >= 1)
        {
            const int fam = PowerSection::familyForModel (r.model);
            if (fam >= 0)
            {
                v.ps.family = fam;  v.ps.feel = juce::jlimit (0, 2, r.feel);  v.ps.filtersOn = true;
                v.ps.fbLg = PowerSection::loopGainFor (fam, v.ps.feel);
                v.presenceMaxDb = 0.0f;                                  // the presence shelf is replaced by the feedback network
                v.trimDb += derivedTrimDb (r.model, 1);
            }
        }
        PowerSection::SupplyInfo si;
        if (r.derived >= 2 && PowerSection::supplyFor (r.model, si))
        {
            const float rs = si.rs;
            const float depth = kSupplyCal * 5.0f * si.ipk * rs / si.bplus * 2.0f * r.sagAmt;
            v.ps.supplyOn = true;
            v.ps.s1 = depth;
            v.ps.s2 = 0.45f * depth;
            v.ps.tau1 = juce::jlimit (0.012f, 0.05f, 0.02f * rs / 100.0f);
            v.ps.tau2 = si.tauSlow;
            v.ps.otSat = si.otSat;
            v.sag = 0.0f;                                               // the classic single-time-constant droop is replaced
            v.trimDb += derivedTrimDb (r.model, 2);
        }
        if (r.derived >= 3) { applyComponentVoicing (v, r.model);  v.trimDb += derivedTrimDb (r.model, 3); }
    }

    //==========================================================================
    // "Auto level" helpers (v43). All are static tables made by measurement, so they add no processing.
    //==========================================================================

    // Amp level at the output of the amp (before the cab) for a plucked test signal, in dB, by amp
    // (Clean, Vintage Chime, British Hot-Rod, Saturated Lead, Modern High-Gain, Classic Rock), voicing
    // (off / on) and Amp Gain (0, 0.1 ... 1.0), with the trims above in place. Measured with the overdrive off.
    // Auto level adds (target - this value), so every amp lands at the same loudness at every setting of the
    // Gain knob. If you change a trim or a voicing, measure this table again.
    inline float autoAmpLevelDb (int model, int voicing, float gain)
    {
        static const float tbl[4][15][11] = {
        {
            { -32.06f, -30.44f, -28.84f, -27.28f, -25.75f, -24.27f, -22.68f, -20.38f, -17.86f, -15.67f, -14.21f },
            { -34.62f, -32.26f, -30.03f, -27.92f, -25.95f, -24.16f, -22.47f, -20.47f, -18.74f, -17.55f, -16.91f },
            { -30.79f, -28.78f, -27.15f, -25.91f, -25.01f, -24.39f, -23.95f, -23.62f, -23.33f, -22.95f, -22.69f },
            { -28.25f, -26.77f, -25.61f, -24.72f, -24.06f, -23.55f, -23.08f, -22.49f, -22.10f, -21.86f, -21.79f },
            { -28.35f, -26.59f, -25.25f, -24.29f, -23.63f, -23.19f, -22.86f, -22.53f, -22.17f, -21.79f, -21.60f },
            { -32.96f, -30.78f, -28.74f, -26.85f, -25.16f, -23.72f, -22.39f, -20.85f, -19.63f, -18.90f, -18.56f },
            { -36.32f, -33.65f, -31.10f, -28.67f, -26.40f, -24.26f, -22.20f, -19.84f, -17.71f, -16.18f, -15.28f },
            { -29.05f, -27.23f, -25.82f, -24.77f, -24.06f, -23.58f, -23.22f, -22.83f, -22.38f, -22.01f, -21.80f },
            { -36.55f, -33.69f, -31.00f, -28.46f, -26.11f, -23.92f, -21.83f, -19.65f, -17.79f, -16.43f, -15.59f },
            { -31.10f, -29.64f, -28.18f, -26.72f, -25.27f, -23.81f, -22.34f, -20.82f, -19.31f, -17.88f, -16.57f },
            { -29.91f, -28.02f, -26.51f, -25.35f, -24.54f, -23.98f, -23.57f, -23.18f, -22.81f, -22.45f, -22.21f },
            { -30.97f, -28.79f, -27.00f, -25.62f, -24.64f, -23.95f, -23.43f, -22.93f, -22.61f, -22.43f, -22.31f },
            { -28.40f, -26.89f, -25.74f, -24.89f, -24.32f, -23.93f, -23.64f, -23.32f, -22.93f, -22.56f, -22.33f },
            { -27.00f, -25.91f, -25.12f, -24.54f, -24.15f, -23.89f, -23.68f, -23.39f, -22.95f, -22.55f, -22.32f },
            { -30.64f, -28.67f, -27.06f, -25.78f, -24.85f, -24.17f, -23.65f, -23.13f, -22.74f, -22.38f, -22.16f }
        },
        {
            { -27.18f, -25.48f, -23.85f, -22.29f, -20.79f, -19.36f, -17.90f, -16.11f, -14.48f, -13.31f, -12.66f },
            { -31.47f, -29.44f, -27.57f, -25.90f, -24.43f, -23.13f, -21.92f, -20.48f, -19.23f, -18.41f, -17.96f },
            { -31.75f, -30.23f, -29.04f, -28.18f, -27.61f, -27.26f, -27.04f, -26.82f, -26.48f, -26.14f, -25.97f },
            { -28.47f, -27.22f, -26.28f, -25.62f, -25.17f, -24.85f, -24.57f, -24.20f, -23.95f, -23.87f, -23.74f },
            { -29.09f, -27.36f, -26.03f, -25.06f, -24.36f, -23.84f, -23.38f, -22.83f, -22.49f, -22.20f, -22.09f },
            { -30.34f, -28.37f, -26.65f, -25.21f, -24.08f, -23.24f, -22.60f, -21.96f, -21.51f, -21.24f, -21.08f },
            { -34.35f, -31.67f, -29.28f, -27.17f, -25.38f, -23.88f, -22.59f, -21.33f, -20.59f, -20.21f, -20.00f },
            { -27.65f, -26.39f, -25.43f, -24.73f, -24.24f, -23.89f, -23.59f, -23.26f, -22.94f, -22.62f, -22.42f },
            { -34.19f, -31.81f, -29.60f, -27.54f, -25.63f, -23.89f, -22.29f, -20.50f, -19.04f, -18.07f, -17.59f },
            { -32.94f, -31.07f, -29.23f, -27.41f, -25.64f, -23.91f, -22.22f, -20.52f, -18.89f, -17.40f, -16.07f },
            { -27.72f, -26.42f, -25.46f, -24.76f, -24.30f, -23.96f, -23.69f, -23.38f, -23.03f, -22.70f, -22.53f },
            { -32.99f, -30.61f, -28.50f, -26.68f, -25.16f, -23.96f, -23.01f, -22.17f, -21.57f, -21.22f, -21.06f },
            { -27.19f, -26.02f, -25.20f, -24.62f, -24.24f, -23.95f, -23.71f, -23.43f, -23.12f, -22.82f, -22.67f },
            { -26.49f, -25.51f, -24.84f, -24.39f, -24.10f, -23.87f, -23.64f, -23.21f, -22.77f, -22.54f, -22.43f },
            { -28.47f, -27.03f, -25.93f, -25.12f, -24.57f, -24.18f, -23.87f, -23.54f, -23.18f, -22.86f, -22.67f }
        },
        {
            { -33.20f, -31.24f, -29.35f, -27.51f, -25.77f, -24.09f, -22.34f, -19.90f, -17.44f, -15.60f, -14.57f },
            { -34.71f, -32.36f, -30.12f, -27.99f, -26.00f, -24.13f, -22.30f, -20.12f, -18.20f, -16.90f, -16.22f },
            { -30.58f, -28.70f, -27.13f, -25.85f, -24.88f, -24.15f, -23.60f, -23.07f, -22.70f, -22.44f, -22.27f },
            { -32.22f, -30.36f, -28.63f, -26.99f, -25.50f, -24.17f, -22.87f, -21.20f, -19.83f, -18.99f, -18.61f },
            { -29.96f, -28.18f, -26.71f, -25.54f, -24.70f, -24.09f, -23.61f, -23.13f, -22.78f, -22.53f, -22.35f },
            { -31.56f, -29.77f, -28.11f, -26.60f, -25.30f, -24.15f, -23.04f, -21.65f, -20.56f, -19.96f, -19.71f },
            { -33.15f, -30.76f, -28.65f, -26.82f, -25.31f, -24.12f, -23.15f, -22.24f, -21.73f, -21.46f, -21.28f },
            { -32.67f, -30.73f, -28.91f, -27.19f, -25.61f, -24.16f, -22.70f, -20.75f, -19.08f, -18.05f, -17.57f },
            { -34.89f, -32.48f, -30.20f, -28.03f, -26.00f, -24.09f, -22.19f, -19.88f, -17.75f, -16.25f, -15.42f },
            { -31.23f, -29.80f, -28.37f, -26.94f, -25.53f, -24.11f, -22.69f, -21.22f, -19.77f, -18.40f, -17.15f },
            { -31.81f, -30.02f, -28.35f, -26.78f, -25.37f, -24.09f, -22.88f, -21.39f, -20.14f, -19.33f, -18.94f },
            { -33.78f, -31.60f, -29.56f, -27.63f, -25.84f, -24.16f, -22.51f, -20.64f, -19.08f, -18.07f, -17.54f },
            { -31.95f, -30.02f, -28.24f, -26.60f, -25.18f, -23.92f, -22.77f, -21.43f, -20.37f, -19.71f, -19.39f },
            { -27.68f, -26.29f, -25.31f, -24.63f, -24.20f, -23.91f, -23.67f, -23.37f, -23.10f, -22.88f, -22.75f },
            { -31.32f, -29.60f, -28.01f, -26.56f, -25.30f, -24.20f, -23.16f, -21.94f, -20.91f, -20.26f, -19.95f }
        },
        {
            { -32.61f, -30.92f, -29.23f, -27.56f, -25.91f, -24.29f, -22.53f, -19.93f, -17.02f, -14.46f, -12.72f },
            { -36.21f, -33.60f, -31.07f, -28.65f, -26.35f, -24.17f, -22.03f, -19.40f, -17.05f, -15.45f, -14.58f },
            { -30.52f, -28.66f, -27.13f, -25.93f, -25.05f, -24.41f, -23.97f, -23.63f, -23.34f, -22.94f, -22.68f },
            { -26.26f, -25.37f, -24.72f, -24.24f, -23.89f, -23.58f, -23.26f, -22.79f, -22.38f, -22.15f, -22.10f },
            { -27.83f, -26.25f, -25.05f, -24.18f, -23.58f, -23.19f, -22.88f, -22.55f, -22.08f, -21.62f, -21.41f },
            { -31.46f, -29.57f, -27.79f, -26.21f, -24.86f, -23.73f, -22.74f, -21.62f, -20.78f, -20.24f, -19.96f },
            { -35.24f, -32.74f, -30.39f, -28.17f, -26.12f, -24.22f, -22.39f, -20.37f, -18.66f, -17.47f, -16.81f },
            { -26.87f, -25.73f, -24.87f, -24.25f, -23.84f, -23.54f, -23.28f, -22.86f, -22.43f, -22.14f, -21.98f },
            { -32.48f, -30.44f, -28.54f, -26.75f, -25.20f, -23.90f, -22.73f, -21.44f, -20.35f, -19.67f, -19.36f },
            { -31.10f, -29.64f, -28.18f, -26.71f, -25.26f, -23.80f, -22.34f, -20.81f, -19.30f, -17.87f, -16.55f },
            { -26.96f, -25.89f, -25.13f, -24.59f, -24.23f, -23.96f, -23.73f, -23.43f, -23.04f, -22.72f, -22.57f },
            { -28.69f, -27.08f, -25.88f, -25.01f, -24.42f, -24.00f, -23.67f, -23.33f, -23.08f, -22.87f, -22.75f },
            { -29.07f, -27.37f, -26.05f, -25.08f, -24.43f, -23.98f, -23.63f, -23.22f, -22.84f, -22.53f, -22.34f },
            { -25.99f, -25.22f, -24.69f, -24.32f, -24.07f, -23.85f, -23.63f, -23.26f, -22.84f, -22.51f, -22.35f },
            { -28.69f, -27.22f, -26.09f, -25.24f, -24.64f, -24.20f, -23.84f, -23.39f, -22.94f, -22.55f, -22.32f }
        }
        };
        constexpr float target = -23.5f;
        const int mi = model >= 9 ? juce::jmin (model, 15) - 1 : model == 8 ? 7 : model == 7 ? 6 : model == 6 ? 5 : juce::jlimit (0, 4, model);
        const float x = juce::jlimit (0.0f, 1.0f, gain) * 10.0f;
        const int   i = juce::jmin ((int) x, 9);
        const float f = x - (float) i;
        const float* r = tbl[juce::jlimit (0, 3, voicing)][mi];
        return target - (r[i] + f * (r[i + 1] - r[i]));
    }

    // Compressor makeup gain (dB) that restores the level for the plucked test signal, by threshold
    // (-36 to 0 dB in steps of 6) and ratio setting (1:1, 2:1, 4:1, 10:1, 20:1, Limit; between them by index).
    inline float autoCompMakeupDb (float threshDb, int ratioIdx, float mix)
    {
        static const int   idxAt[6] = { 0, 1, 3, 5, 7, 8 };
        static const float tbl[6][7] = {
            { 0.86f, 0.86f, 0.86f, 0.86f, 0.86f, 0.86f, 0.86f },
            { 7.29f, 4.65f, 2.40f, 1.02f, 0.86f, 0.86f, 0.86f },
            { 10.45f, 6.49f, 3.15f, 1.09f, 0.86f, 0.86f, 0.86f },
            { 12.32f, 7.57f, 3.59f, 1.14f, 0.86f, 0.86f, 0.86f },
            { 12.94f, 7.92f, 3.74f, 1.15f, 0.86f, 0.86f, 0.86f },
            { 13.43f, 8.17f, 3.80f, 0.98f, 0.86f, 0.86f, 0.86f } };
        const float tx = juce::jlimit (0.0f, 6.0f, (threshDb + 36.0f) / 6.0f);
        const int   ti = juce::jmin ((int) tx, 5);
        const float tf = tx - (float) ti;
        auto at = [&] (int row) { return tbl[row][ti] + tf * (tbl[row][ti + 1] - tbl[row][ti]); };
        const int ri = juce::jlimit (0, 8, ratioIdx);
        int k = 0;
        while (k < 4 && ri > idxAt[k + 1]) ++k;
        const float rf = (float) (ri - idxAt[k]) / (float) (idxAt[k + 1] - idxAt[k]);
        const float m = at (k) + rf * (at (k + 1) - at (k));
        return m * juce::jlimit (0.0f, 1.0f, mix);
    }

    // Overdrive: how much louder (dB) the guitar gets with the overdrive on at Volume -6 dB than with it off,
    // by style (Transparent, Tight, Open high range, Open low range, then v58 Screamer and Distortion with each of the four clip choices) and Drive (0, 0.1 ... 1.0). Measured
    // through the Clean amp at Gain 0 (nearly linear), so it is the overdrive's own loudness change, not
    // what a high-gain amp does with it. Auto level takes it away again.
    inline float autoOdDeltaDb (int style, bool rangeHigh, float drive, int clip = 0, bool circuit = false)
    {
        static const float tbl[23][11] = {
            { -5.30f, -4.67f, -2.15f, 1.03f, 3.83f, 6.08f, 7.88f, 9.33f, 10.52f, 11.51f, 12.38f },
            { -11.07f, -7.54f, -2.14f, 1.72f, 4.37f, 6.25f, 7.67f, 8.76f, 9.63f, 10.35f, 10.94f },
            { -12.53f, -9.68f, -4.02f, 0.70f, 4.16f, 6.68f, 8.55f, 10.00f, 11.17f, 12.12f, 12.91f },
            { -8.34f, -7.44f, -4.61f, -1.29f, 1.71f, 4.20f, 6.23f, 7.88f, 9.21f, 10.31f, 11.21f },
                        { -7.21f, -7.17f, -5.33f, -2.06f, 1.01f, 3.44f, 5.34f, 6.85f, 8.07f, 9.08f, 9.93f },
            { -6.99f, -6.84f, -4.66f, -1.13f, 2.11f, 4.69f, 6.70f, 8.29f, 9.59f, 10.66f, 11.56f },
            { -7.57f, -7.66f, -6.18f, -3.12f, -0.19f, 2.12f, 3.90f, 5.31f, 6.45f, 7.39f, 8.18f },
            { -7.69f, -7.68f, -5.85f, -2.33f, 1.05f, 3.68f, 5.68f, 7.24f, 8.50f, 9.53f, 10.40f },
            { -9.15f, -5.74f, -0.87f, 2.04f, 3.56f, 4.37f, 4.85f, 5.16f, 5.37f, 5.54f, 5.66f },
            { -9.06f, -5.42f, 0.32f, 4.36f, 6.75f, 8.14f, 8.98f, 9.50f, 9.84f, 10.08f, 10.26f },
            { -9.42f, -6.32f, -2.26f, -0.06f, 1.02f, 1.59f, 1.93f, 2.17f, 2.34f, 2.46f, 2.56f },
            { -9.66f, -6.40f, -1.37f, 1.92f, 3.61f, 4.48f, 4.94f, 5.24f, 5.44f, 5.59f, 5.71f },
            { -5.62f, -4.86f, -1.80f, 1.80f, 4.85f, 7.21f, 9.02f, 10.43f, 11.56f, 12.47f, 13.22f },   // v64 Tube Driver (measured)
        { 0.43f, 7.03f, 9.41f, 10.73f, 11.96f, 12.97f, 13.77f, 14.41f, 14.90f, 15.20f, 15.24f },   // v77 circuit: Tight
        { -5.69f, -4.78f, -2.01f, 1.49f, 1.75f, 2.20f, 2.88f, 3.76f, 4.66f, 5.39f, 5.87f },   // Screamer circuit, Silicon
        { -5.69f, -4.77f, -1.61f, 4.43f, 4.89f, 5.65f, 6.81f, 8.36f, 10.28f, 12.26f, 13.86f },   // LED
        { -5.70f, -5.01f, -4.04f, -3.17f, -3.12f, -3.04f, -2.91f, -2.77f, -2.63f, -2.50f, -2.38f },   // Germanium
        { -5.69f, -4.78f, -2.01f, 1.61f, 1.93f, 2.51f, 3.50f, 4.93f, 6.54f, 7.87f, 8.68f },   // Asymmetric
        { -5.71f, -4.81f, -1.67f, 4.48f, 4.99f, 5.86f, 7.21f, 9.09f, 11.34f, 13.70f, 15.88f },   // None (rails)
        { 5.55f, 5.67f, 5.84f, 6.08f, 6.37f, 6.71f, 7.06f, 7.39f, 7.73f, 8.01f, 8.22f },   // v91 Distortion circuit, Silicon (measured)
        { -7.33f, -5.47f, -3.40f, -1.30f, 0.61f, 2.19f, 3.40f, 4.28f, 4.88f, 5.68f, 6.95f },   // v92 Op-Amp Distortion (measured)
        { -2.42f, -2.30f, -2.15f, -1.78f, -1.08f, 0.16f, 1.95f, 3.43f, 3.79f, 8.83f, 7.18f },   // v92 Stack Drive (measured)
        { -5.08f, -4.74f, -4.35f, -3.81f, -2.93f, -1.49f, 0.46f, 2.60f, 4.68f, 6.53f, 8.10f },   // v93 Transparent (Klon-type circuit, measured)
        };
        const int row = circuit && style == 0 ? 22 : style >= 6 ? 14 + style : odCircuitActive (style, circuit) ? (style == 1 ? 13 : style == 4 ? 19 : 14 + juce::jlimit (0, 4, clip)) : style == 5 ? 12 : style == 2 ? (rangeHigh ? 2 : 3) : style >= 3 ? (style == 3 ? 4 : 8) + juce::jlimit (0, 3, clip) : juce::jlimit (0, 1, style);
        const float x = juce::jlimit (0.0f, 1.0f, drive) * 10.0f;
        const int   i = juce::jmin ((int) x, 9);
        return tbl[row][i] + (x - (float) i) * (tbl[row][i + 1] - tbl[row][i]);
    }

    // The overdrive's Volume in dB as the processor should use it (with Auto level on, the knob is a trim
    // around the matched level, so the same knob position gives "same loudness as overdrive off" at -6 dB).
    inline float odVolumeDbEff (juce::AudioProcessorValueTreeState& s, const char* pre = "od")
    {
        auto p = [&s, pre] (const char* id) { return s.getRawParameterValue (juce::String (pre) + id)->load(); };
        const float vol = p ("Volume");
        if (p ("Auto") < 0.5f) return vol;
        return vol - autoOdDeltaDb ((int) p ("Style"), p ("Range") > 0.5f, p ("Gain"), (int) p ("Clip"), s.getRawParameterValue ("odCircuit")->load() > 0.5f);
    }

    // The voicing for Amp A (isB false) or Amp B, from the parameters.
    AmpVoicing ampVoicingFor (juce::AudioProcessorValueTreeState& s, bool isB)
    {
        auto p = [&s] (const char* id) { return s.getRawParameterValue (id)->load(); };
        AmpVoicing v;
        Realism r;
        if (! isB)
        {
            v = metalVoicing ((int) p ("ampModel"), (int) p ("ampVoicing"), p ("ampTune"),
                              MetalSettings { p ("mtTight"), p ("mtMidHz"), p ("mtMid"), 0.5f, p ("mtLead") > 0.5f });
            r = Realism { p ("ampBias"), p ("ampSagAmt"), (int) p ("ampTube"), 0, (int) p ("ampPwrTube") };
            r.feel = (int) p ("ampFeel");  r.stack = (int) p ("ampStack");
            r.deep = p ("ampDeep") > 0.5f;
            r.derived = (int) p ("ampMode") == 1 ? 3 : 0;  r.model = (int) p ("ampModel");
        }
        else
        {
            v = metalVoicing (ampBModelValue ((int) p ("ampBModel")), (int) p ("ampBVoicing"), p ("ampTune"),
                              MetalSettings { p ("ampBTight"), 0.5f, 0.0f, 0.5f, false });
            // v52: Amp B has no output ceiling like Amp A's, so a lower-gain preamp tube used to make it 5 to 10 dB quieter.
            // The tube choice is meant to change the feel (softer, rounder), not the loudness, so it is levelled here.
            const int bt = (int) p ("ampBTube");
            r = Realism { p ("ampBBias"), p ("ampBSagAmt"), bt, 0, (int) p ("ampBPwrTube"), bt == 1 ? 4.3f : bt == 2 ? 9.2f : 0.0f };
            r.feel = (int) p ("ampBFeel");  r.stack = (int) p ("ampBStack");
            r.deep = p ("ampBDeep") > 0.5f;
            r.derived = (int) p ("ampBMode") == 1 ? 3 : 0;  r.model = ampBModelValue ((int) p ("ampBModel"));
        }
        applyRealism (v, r);
        return v;
    }

    // Power stage (after the preamp, sag and before presence), run at the oversampled rate:
    //   drive 0..1 : tube power-amp saturation/compression (unity gain for small signals)
    //   deadZone   : crossover distortion of a cold-biased output stage
    inline float powerSample (float s, float drive, float deadZone)
    {
        const float d = 1.0f + 5.0f * drive * drive;
        if (deadZone > 0.0f)
            s -= deadZone * fastTanh (s / deadZone);
        if (drive > 0.0f)
            s = fastTanh (d * s) / d * std::sqrt (d);
        return s;
    }

    inline void powerStage (float* y, int m, float drive, float deadZone)
    {
        const float d = 1.0f + 5.0f * drive * drive;
        const float comp = std::pow (d, 0.5f);
        for (int i = 0; i < m; ++i)
        {
            float s = y[i];
            if (deadZone > 0.0f)
                s -= deadZone * fastTanh (s / deadZone);
            if (drive > 0.0f)
                s = fastTanh (d * s) / d * comp;
            y[i] = s;
        }
    }

    // Built-in cab models, used when no IR is loaded. Each is a hand-tuned
    // approximation of the general character of that kind of cab: a low
    // resonance, a presence bump and a roll-off. They are not measurements.
    struct CabModel
    {
        float hpHz;
        float lowHz, lowQ, lowDb;     // bass resonance
        float midHz, midQ, midDb;     // upper-mid "bark"
        float lpHz;                   // top-end roll-off
    };

    // type: 1 = 1x12 open back, 2 = 2x12 combo, 3 = 4x12 closed back, 4 = 1x12 bright
    // (type 0, "Amp default", uses the amp model's own low-pass instead.)
    const CabModel& getCabModel (int type)
    {
        static const CabModel models[] =
        {
            {  90.0f, 110.0f, 1.0f, 2.0f,  2400.0f, 1.0f, 3.0f,  5800.0f },   // 1: 1x12 open back
            {  80.0f, 100.0f, 0.9f, 3.0f,  2000.0f, 0.9f, 2.0f,  6500.0f },   // 2: 2x12 combo
            {  70.0f,  85.0f, 1.0f, 4.0f,  2800.0f, 0.9f, 2.0f,  5000.0f },   // 3: 4x12 closed back
            { 100.0f, 120.0f, 1.0f, 1.0f,  3200.0f, 0.8f, 4.0f,  7500.0f },   // 4: 1x12 bright
        };
        return models[juce::jlimit (1, 4, type) - 1];
    }

    // Constant-power pan: pan -1 = hard left, 0 = centre, +1 = hard right.
    // Scaled so that the centre has gain 1 on both sides (like the old mono cab).
    void panGains (float pan, float& left, float& right)
    {
        const float angle = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        left  = std::sqrt (2.0f) * std::cos (angle);
        right = std::sqrt (2.0f) * std::sin (angle);
    }
}

//==============================================================================

//==============================================================================
// Cab builder: synthesises a cabinet impulse response from a speaker, a mic, the
// mic's position, distance and angle, a cabinet type and some room.
//
// IMPORTANT: this is a model, not a measurement. Each speaker and mic is a hand-
// made frequency response (a high-pass, a few peaks and dips, a low-pass) based
// on the general character these parts are known for, tuned by ear. They will
// not match a particular recording of the real thing. The response is turned
// into a minimum-phase impulse response, which is how a real speaker behaves
// to a good approximation, and loaded into the same convolution as a file IR.
//==============================================================================
namespace cabbuilder
{
    struct Bump { float hz, db, oct; };   // a peak (db > 0) or dip (db < 0); 'oct' = width in octaves (one sigma)

    struct SpeakerModel
    {
        const char* name;
        float hpHz, hpOrder;                // low end roll-off
        float resHz, resDb, resOct;         // the low resonance "thump"
        Bump bumps[4];                      // upper-mid character
        float lpHz, lpOrder;                // high end roll-off
        float ripple, seed;                 // small irregularities (cone breakup), in dB
    };

    struct MicModel
    {
        const char* name;
        float hpHz, hpOrder;
        Bump bumps[3];
        float lpHz, lpOrder;
        float proximity;                    // how much the lows rise when close (0 to 1)
        float offAxis;                      // how much the highs fall when angled away (0 to 1)
    };

    const SpeakerModel speakers[] =
    {
        { "Modern Tight",      70.0f, 2.0f,  78.0f, 2.5f, 0.35f, { { 1600.0f,  3.0f, 0.50f }, { 3300.0f,  5.0f, 0.35f }, {  800.0f, -2.0f, 0.60f }, { 0.0f, 0.0f, 1.0f } }, 5500.0f, 4.0f, 0.8f, 0.3f },
        { "Vintage Crunch",    80.0f, 2.0f, 100.0f, 4.0f, 0.40f, { { 2200.0f,  3.5f, 0.50f }, { 1000.0f,  1.5f, 0.60f }, { 4000.0f, -1.0f, 0.40f }, { 0.0f, 0.0f, 1.0f } }, 4800.0f, 3.0f, 0.7f, 1.1f },
        { "Rock Bite",         85.0f, 2.0f, 110.0f, 1.0f, 0.40f, { { 2500.0f,  4.0f, 0.40f }, { 4500.0f,  3.0f, 0.40f }, { 1200.0f, -1.5f, 0.50f }, { 0.0f, 0.0f, 1.0f } }, 6000.0f, 4.0f, 0.8f, 2.0f },
        { "Smooth Vintage",    75.0f, 2.0f,  90.0f, 3.0f, 0.40f, { { 1800.0f,  2.5f, 0.60f }, { 3500.0f,  2.0f, 0.50f }, {  600.0f,  1.0f, 0.60f }, { 0.0f, 0.0f, 1.0f } }, 5200.0f, 3.0f, 0.6f, 2.9f },
        { "Chimey Alnico",     90.0f, 2.0f, 110.0f, 2.0f, 0.40f, { { 2000.0f,  3.0f, 0.50f }, { 3000.0f,  3.5f, 0.35f }, {  500.0f, -1.0f, 0.70f }, { 0.0f, 0.0f, 1.0f } }, 5800.0f, 3.0f, 0.6f, 3.7f },
        { "Vintage American",  85.0f, 2.0f,  95.0f, 2.0f, 0.40f, { { 3000.0f,  3.0f, 0.50f }, { 5500.0f,  3.0f, 0.35f }, {  700.0f, -2.0f, 0.60f }, { 0.0f, 0.0f, 1.0f } }, 7500.0f, 3.0f, 0.7f, 4.6f },
        { "Hi-Fi Full",        65.0f, 2.0f,  70.0f, 1.5f, 0.40f, { { 2500.0f,  2.0f, 0.60f }, { 4500.0f,  1.0f, 0.50f }, {  900.0f, -1.0f, 0.70f }, { 0.0f, 0.0f, 1.0f } }, 6500.0f, 3.0f, 0.5f, 5.5f },
        // v39: modern metal and djent oriented (and useful beyond)
        { "Modern Scoop",      58.0f, 2.0f,  74.0f, 3.0f, 0.38f, { { 5000.0f,  3.5f, 0.35f }, {  800.0f, -3.0f, 0.60f }, { 3000.0f, -1.0f, 0.50f }, {  180.0f, 1.0f, 0.60f } }, 6500.0f, 4.0f, 0.7f, 6.3f },
        { "Tight Ceramic",     82.0f, 2.0f,  86.0f, 1.5f, 0.30f, { { 2800.0f,  4.5f, 0.35f }, { 4800.0f,  3.0f, 0.30f }, {  450.0f, -2.5f, 0.50f }, { 0.0f, 0.0f, 1.0f } }, 5800.0f, 4.0f, 0.8f, 7.1f },
        { "Dark Thump",        50.0f, 2.0f,  68.0f, 3.5f, 0.40f, { { 1500.0f,  2.0f, 0.60f }, {  180.0f,  1.5f, 0.70f }, { 3500.0f, -1.5f, 0.40f }, {  900.0f, -1.0f, 0.60f } }, 4300.0f, 4.0f, 0.6f, 7.9f },
        { "Hi-Def Light Cone", 72.0f, 2.0f,  92.0f, 1.0f, 0.40f, { { 3500.0f,  3.0f, 0.50f }, { 7000.0f,  2.5f, 0.40f }, {  700.0f, -1.0f, 0.60f }, { 0.0f, 0.0f, 1.0f } }, 8500.0f, 3.0f, 0.8f, 8.8f },
        { "Smooth Lead",       75.0f, 2.0f, 100.0f, 2.0f, 0.40f, { { 1400.0f,  3.0f, 0.50f }, { 2500.0f,  2.0f, 0.40f }, { 5500.0f, -3.0f, 0.30f }, {  300.0f, 0.5f, 0.70f } }, 4800.0f, 5.0f, 0.5f, 9.6f },
        { "Mid Forward",       85.0f, 2.0f, 105.0f, 1.0f, 0.40f, { { 1800.0f,  5.0f, 0.40f }, { 3200.0f,  2.5f, 0.40f }, {  500.0f, -2.0f, 0.50f }, { 0.0f, 0.0f, 1.0f } }, 5500.0f, 4.0f, 0.8f, 10.4f },
        // v67/v69: fitted to the smoothed frequency response of mixed 4x12 recordings (see MODELED_GEAR.md). Seven were fitted; five
        // that landed within about 1 dB of Studio Standard or Studio Smooth Tight were dropped in v69 (Tight and Bite cover them). They
        // assume the 4x12 Closed cabinet, the Flat mic and Position 0.3; other settings stack on top of that.
        { "Studio Standard", 81.0f, 2.0f, 248.0f, 7.2f, 0.80f, { { 1400.0f, -1.7f, 1.20f }, { 727.0f, 2.3f, 0.25f }, { 2274.0f, 4.8f, 0.20f }, { 3689.0f, 2.3f, 0.20f } }, 5684.0f, 8.3f, 0.0f, 0.0f },
        { "Studio Smooth Tight", 111.0f, 2.0f, 254.0f, 5.8f, 0.80f, { { 1400.0f, -1.2f, 1.20f }, { 727.0f, 1.6f, 0.25f }, { 2276.0f, 4.0f, 0.20f }, { 3689.0f, 1.1f, 0.20f } }, 5672.0f, 8.3f, 0.0f, 0.0f },
        // v68: fitted to the smoothed responses of four more sets of mixed cab recordings (heavy 4x12s, rhythm 4x12s from three
        // studios, and small cabs). Same assumptions as the Studio speakers above.
        { "Heavy Bright", 99.0f, 2.0f, 97.0f, 4.5f, 0.41f, { { 350.0f, 7.8f, 0.82f }, { 631.0f, -4.2f, 0.48f }, { 2877.0f, 6.5f, 0.31f }, { 5960.0f, 10.0f, 0.20f } }, 5123.0f, 9.0f, 0.0f, 0.0f },
        { "Heavy Tight", 126.0f, 2.0f, 232.0f, 6.3f, 1.10f, { { 1020.0f, -2.0f, 0.25f }, { 563.0f, -2.7f, 0.20f }, { 2277.0f, 3.6f, 0.20f }, { 3450.0f, 4.7f, 0.36f } }, 5718.0f, 11.8f, 0.0f, 0.0f },
        { "Studio Hollow", 91.0f, 2.0f, 241.0f, 7.9f, 0.72f, { { 672.0f, -5.3f, 0.31f }, { 691.0f, 10.0f, 0.20f }, { 3000.0f, -8.0f, 0.73f }, { 3000.0f, 9.4f, 0.50f } }, 5607.0f, 5.7f, 0.0f, 0.0f },
        { "Rhythm Dark", 37.0f, 2.0f, 310.0f, 5.4f, 0.61f, { { 1365.0f, -4.0f, 1.20f }, { 708.0f, 5.7f, 0.20f }, { 1800.0f, 5.4f, 0.42f }, { 5752.0f, 10.0f, 0.37f } }, 3880.0f, 6.5f, 0.0f, 0.0f },
        { "Rhythm Smooth", 40.0f, 2.0f, 250.0f, 12.0f, 0.56f, { { 350.0f, -9.0f, 1.20f }, { 596.0f, 10.0f, 0.56f }, { 2134.0f, 3.8f, 0.20f }, { 3000.0f, -3.4f, 0.80f } }, 4360.0f, 6.2f, 0.0f, 0.0f },
        { "Compact Scooped", 90.0f, 2.0f, 220.0f, 10.9f, 0.43f, { { 350.0f, -6.3f, 1.20f }, { 1254.0f, 6.2f, 0.20f }, { 1800.0f, 2.7f, 0.80f }, { 3373.0f, -3.9f, 0.37f } }, 5166.0f, 6.7f, 0.0f, 0.0f },
        { "Compact Balanced", 83.0f, 2.0f, 205.0f, 9.0f, 0.52f, { { 350.0f, -10.0f, 1.20f }, { 500.0f, 8.0f, 0.96f }, { 2164.0f, 3.4f, 0.29f }, { 3873.0f, -1.6f, 0.20f } }, 5329.0f, 7.0f, 0.0f, 0.0f },
        { "Compact Warm", 30.0f, 2.0f, 205.0f, 5.5f, 0.32f, { { 712.0f, -4.2f, 1.20f }, { 1162.0f, 8.6f, 0.35f }, { 2996.0f, 4.6f, 0.34f }, { 3449.0f, -8.0f, 0.21f } }, 4937.0f, 5.1f, 0.0f, 0.0f },
        { "Compact Dark", 60.0f, 2.0f, 226.0f, 9.2f, 0.61f, { { 350.0f, -10.0f, 1.20f }, { 744.0f, 5.9f, 0.84f }, { 2338.0f, 6.7f, 0.27f }, { 3399.0f, -4.7f, 0.42f } }, 5163.0f, 9.3f, 0.0f, 0.0f },
        { "Compact Round Low", 100.0f, 2.0f, 76.0f, 7.9f, 0.42f, { { 352.0f, -4.2f, 0.25f }, { 889.0f, -3.3f, 0.20f }, { 1800.0f, 2.7f, 0.44f }, { 3934.0f, -8.0f, 0.35f } }, 5352.0f, 12.0f, 0.0f, 0.0f },
        { "Compact Open Air", 100.0f, 2.0f, 320.0f, 3.6f, 0.92f, { { 350.0f, -5.5f, 1.20f }, { 1700.0f, 4.6f, 0.52f }, { 3000.0f, -2.2f, 0.20f }, { 6057.0f, -8.0f, 0.20f } }, 4800.0f, 3.4f, 0.0f, 0.0f },
        // v69: nine more voicings, each kept only if it was at least 2.4 dB (rms) away from every speaker above. Studio ... and
        // Heavy ... come from more mixed cab recordings. Classic ... come from close-mic cab recordings in older rock and metal
        // styles, so they include some close-mic brightness. Same assumptions as above (4x12 Closed cabinet, Flat mic, Position 0.3).
        { "Studio Dark", 102.0f, 2.0f, 298.0f, 11.4f, 0.68f, { { 350.0f, -10.0f, 1.20f }, { 695.0f, 6.9f, 0.20f }, { 1800.0f, 6.1f, 0.80f }, { 3000.0f, -6.0f, 0.80f } }, 4068.0f, 3.6f, 0.0f, 0.0f },
        { "Studio Thick Dark", 140.0f, 2.0f, 113.0f, 12.0f, 0.46f, { { 451.0f, -6.0f, 0.44f }, { 500.0f, 3.2f, 0.89f }, { 2799.0f, 2.2f, 0.27f }, { 5532.0f, -8.0f, 0.20f } }, 4467.0f, 6.1f, 0.0f, 0.0f },
        { "Heavy Hi-Lo", 95.0f, 2.0f, 116.0f, 10.0f, 1.08f, { { 511.0f, -3.2f, 0.44f }, { 1366.0f, 2.1f, 0.20f }, { 2161.0f, 4.3f, 0.20f }, { 3902.0f, 7.1f, 0.46f } }, 6276.0f, 12.0f, 0.0f, 0.0f },
        { "Studio Warm Mid", 101.0f, 2.0f, 202.0f, 9.3f, 0.24f, { { 374.0f, 6.1f, 0.59f }, { 1661.0f, -3.6f, 0.59f }, { 2999.0f, 3.6f, 0.80f }, { 6035.0f, -8.0f, 0.20f } }, 4759.0f, 7.1f, 0.0f, 0.0f },
        { "Studio Thick Bright", 140.0f, 2.0f, 116.0f, 12.0f, 0.43f, { { 350.0f, 5.6f, 0.91f }, { 500.0f, -3.3f, 0.77f }, { 2255.0f, 1.9f, 0.20f }, { 4446.0f, 10.0f, 0.26f } }, 4173.0f, 3.6f, 0.0f, 0.0f },
        { "Classic Dark", 140.0f, 2.0f, 242.0f, 5.1f, 0.20f, { { 508.0f, 10.0f, 0.58f }, { 1680.0f, -7.4f, 0.20f }, { 2605.0f, -8.0f, 0.74f }, { 3355.0f, -8.0f, 0.33f } }, 3812.0f, 5.0f, 0.0f, 0.0f },
        { "Classic Big Low", 30.0f, 2.0f, 92.0f, 12.0f, 1.10f, { { 350.0f, 10.0f, 1.20f }, { 658.0f, -9.1f, 0.72f }, { 2254.0f, 9.7f, 0.23f }, { 4072.0f, 10.0f, 0.60f } }, 6140.0f, 5.8f, 0.0f, 0.0f },
        { "Classic Mid Focus", 140.0f, 2.0f, 288.0f, 12.0f, 0.45f, { { 350.0f, -10.0f, 1.20f }, { 511.0f, 10.0f, 0.56f }, { 1800.0f, 1.7f, 0.67f }, { 5434.0f, 4.9f, 0.20f } }, 3500.0f, 3.7f, 0.0f, 0.0f },
        { "Classic Punch", 118.0f, 2.0f, 101.0f, 10.6f, 0.90f, { { 1400.0f, -7.6f, 1.20f }, { 967.0f, 10.0f, 0.51f }, { 2148.0f, 7.8f, 0.20f }, { 5681.0f, -8.0f, 0.80f } }, 5627.0f, 10.9f, 0.0f, 0.0f },
        // v71: one more voicing, from five recordings of one older-style cab that agree within 1 dB (so it is the
        // cab, not one mic placement). At least 2.4 dB (rms) from every speaker above. Same assumptions as above.
        { "Classic Warm Presence", 50.1f, 2.0f, 252.2f, 9.4f, 0.89f, { { 971.3f, -7.4f, 0.25f }, { 1628.0f, -10.0f, 0.25f }, { 1800.0f, 7.2f, 0.80f }, { 4075.2f, -3.1f, 0.20f } }, 4257.8f, 4.3f, 0.0f, 0.0f },
        // v71: two raw single-mic voicings from centre-of-cap recordings of two other cabs (no mix EQ): far from
        // everything above (4 dB or more) and fitted to within 2.4 dB. They are bold; Tilt and Position tame them.
        { "Raw Big Low", 54.5f, 2.0f, 155.6f, 12.0f, 1.10f, { { 350.0f, 10.0f, 1.00f }, { 815.6f, -6.5f, 0.69f }, { 2254.4f, 3.5f, 0.20f }, { 4031.0f, 5.5f, 0.20f } }, 4863.4f, 6.8f, 0.0f, 0.0f },
        { "Raw Bright Cap", 30.0f, 2.0f, 104.1f, 12.0f, 1.10f, { { 355.2f, 10.0f, 0.51f }, { 1368.7f, -7.9f, 0.35f }, { 3000.0f, 10.0f, 0.80f }, { 4762.2f, 10.0f, 0.31f } }, 5908.5f, 6.0f, 0.0f, 0.0f },
    };

    const MicModel mics[] =
    {
        { "Dynamic Bright",     80.0f, 2.0f, { { 5500.0f, 5.0f, 0.45f }, { 3000.0f,  1.5f, 0.60f }, {    0.0f, 0.0f, 1.0f } }, 12000.0f, 2.0f, 0.6f, 0.9f },
        { "Dynamic Full",       50.0f, 2.0f, { { 4500.0f, 3.5f, 0.55f }, {  150.0f,  1.5f, 0.70f }, { 1000.0f, -1.0f, 0.50f } }, 14000.0f, 2.0f, 0.7f, 0.7f },
        { "Dynamic Presence",   70.0f, 2.0f, { { 4500.0f, 3.0f, 0.50f }, {  800.0f, -1.5f, 0.60f }, {    0.0f, 0.0f, 1.0f } }, 13000.0f, 2.0f, 0.5f, 0.6f },
        { "Ribbon Smooth",      40.0f, 2.0f, { {  200.0f, 1.5f, 0.80f }, { 1200.0f,  1.0f, 0.80f }, {    0.0f, 0.0f, 1.0f } },  9000.0f, 3.0f, 1.0f, 0.5f },
        { "Condenser Detailed", 30.0f, 2.0f, { { 8000.0f, 3.0f, 0.60f }, { 3500.0f,  1.5f, 0.60f }, {    0.0f, 0.0f, 1.0f } }, 18000.0f, 2.0f, 0.3f, 0.4f },
        { "Flat (no mic)",       20.0f, 1.0f, { {    0.0f, 0.0f, 1.0f }, {    0.0f,  0.0f, 1.0f }, {    0.0f, 0.0f, 1.0f } }, 22000.0f, 1.0f, 0.0f, 0.0f },
        // v39: modern metal and djent oriented (and useful beyond)
        { "Dynamic Aggressive", 100.0f, 2.0f, { { 3600.0f, 4.0f, 0.40f }, { 7000.0f,  3.0f, 0.35f }, {  700.0f, -1.0f, 0.60f } }, 11000.0f, 2.0f, 0.5f, 0.9f },
        { "Dynamic Punch",       45.0f, 2.0f, { {  120.0f, 2.0f, 0.60f }, { 3500.0f,  3.0f, 0.45f }, {  800.0f, -2.0f, 0.55f } }, 13000.0f, 2.0f, 0.85f, 0.7f },
        { "Dynamic Scoop",       70.0f, 2.0f, { {  450.0f, -2.5f, 0.50f }, { 3000.0f, 2.5f, 0.50f }, { 6000.0f,  2.5f, 0.40f } }, 12500.0f, 2.0f, 0.6f, 0.8f },
        { "Ribbon Dark",         35.0f, 2.0f, { {  180.0f, 2.5f, 0.80f }, { 3000.0f,  1.0f, 0.60f }, {    0.0f, 0.0f, 1.0f } },  7000.0f, 3.0f, 1.0f, 0.6f },
        { "Condenser Warm",      30.0f, 2.0f, { {  250.0f, 1.5f, 0.80f }, { 4500.0f,  1.5f, 0.50f }, { 9000.0f,  2.0f, 0.50f } }, 16000.0f, 2.0f, 0.35f, 0.4f },
        { "Blend (dynamic + ribbon)", 55.0f, 2.0f, { { 200.0f, 2.0f, 0.70f }, { 3800.0f, 3.0f, 0.50f }, { 1500.0f, -1.0f, 0.60f } }, 10500.0f, 2.0f, 0.8f, 0.65f },
        // v71: seven mics fitted to measured recordings. Each is the average difference from a plain dynamic mic over
        // twelve cabs, applied on top of the speaker voicings (which already contain a dynamic mic). The Flat mic is
        // the plain dynamic reference.
        { "Ribbon Open", 30.0f, 2.0f, { { 100.0f, 4.4f, 0.84f }, { 1281.0f, -2.5f, 0.24f }, { 7628.0f, 5.4f, 0.20f } }, 5527.0f, 2.7f, 1.00f, 0.50f },
        { "Condenser Small", 30.0f, 2.0f, { { 189.0f, -1.4f, 0.33f }, { 2281.0f, -1.2f, 0.53f }, { 5224.0f, -3.8f, 0.45f } }, 18000.0f, 6.0f, 0.30f, 0.40f },
        { "Condenser Large", 30.0f, 2.0f, { { 100.0f, 5.0f, 1.11f }, { 1219.0f, -1.8f, 0.21f }, { 7739.0f, 8.0f, 0.20f } }, 6332.0f, 1.9f, 0.35f, 0.40f },
        { "Dynamic Large", 30.0f, 2.0f, { { 100.0f, 3.1f, 0.25f }, { 2563.0f, 1.0f, 0.34f }, { 4202.0f, 3.7f, 0.20f } }, 15372.0f, 1.5f, 0.70f, 0.80f },
        { "Dynamic Long Throw", 30.0f, 2.0f, { { 121.0f, 4.5f, 1.00f }, { 3045.0f, 5.1f, 0.37f }, { 7257.0f, 8.0f, 0.25f } }, 9533.0f, 6.0f, 0.90f, 0.70f },
        { "Dynamic Hyper", 30.0f, 2.0f, { { 100.0f, 3.3f, 0.66f }, { 2718.0f, 3.8f, 0.45f }, { 7868.0f, 8.0f, 0.20f } }, 11207.0f, 1.5f, 0.60f, 0.90f },
        { "Dynamic Wide", 33.0f, 2.0f, { { 120.0f, 3.4f, 0.85f }, { 2905.0f, 3.8f, 0.38f }, { 8358.0f, 8.0f, 0.71f } }, 5490.0f, 1.5f, 0.50f, 0.80f }
    };

    constexpr int kNumSpeakers = (int) (sizeof (speakers) / sizeof (speakers[0]));
    constexpr int kNumMics     = (int) (sizeof (mics) / sizeof (mics[0]));

    inline juce::StringArray speakerNames() { juce::StringArray a; for (auto& s : speakers) a.add (s.name); return a; }
    inline juce::StringArray micNames()     { juce::StringArray a; for (auto& m : mics)     a.add (m.name); return a; }
    inline juce::StringArray cabinetNames()
    {
        return juce::StringArray { "1x12 Open", "2x12 Open", "2x12 Closed", "4x12 Closed",
                                   "4x12 Straight", "4x12 Angled", "4x12 Open Back",
                                   "Oversized 4x12 Straight (ported)", "Oversized 4x12 Angled (ported)",
                                   // v69
                                   "1x12 Closed", "1x10 Open", "2x10 Open", "4x10 Closed", "1x15 Closed", "1x8 Small",
                                   "Oversized 2x12 (ported)" };
    }

    struct Params
    {
        int speaker = 0, mic = 0, cabinet = 3;
        float position = 0.3f;     // 0 = centre of the dust cap, 1 = edge of the cone
        float distanceIn = 1.0f;   // inches from the grille
        float angleDeg = 0.0f;     // 0 = pointing straight at the cone
        float room = 0.0f;         // 0 to 1
        float tight = 0.0f;        // v67: -1 (fuller lows) to +1 (tighter lows)
        float bite = 0.0f;         // v67: -1 (smoother top) to +1 (more upper-mid bite)
        float box = 0.0f;          // v67: 0 to 1, how much of the cabinet's narrow resonance structure is added
        int roomType = 0;          // v68: 0 = the original dark ambience, 1 to 3 = modelled rooms
        bool measured = false;     // v71: position and proximity follow measured recordings (off = the earlier behaviour)
    };

    inline double smooth01 (double x) { x = std::min (1.0, std::max (0.0, x)); return x * x * (3.0 - 2.0 * x); }

    inline double bumpDb (double f, const Bump& b)
    {
        if (b.hz <= 0.0f || b.db == 0.0f)
            return 0.0;
        const double x = std::log2 (f / (double) b.hz) / (double) b.oct;
        return (double) b.db * std::exp (-0.5 * x * x);
    }

    inline double highpassDb (double f, double fc, double n) { return -10.0 * std::log10 (1.0 + std::pow (fc / f, 2.0 * n)); }
    inline double lowpassDb  (double f, double fc, double n) { return -10.0 * std::log10 (1.0 + std::pow (f / fc, 2.0 * n)); }
    inline double lowShelfDb (double f, double fc, double db) { const double r = f / fc; return db / (1.0 + r * r); }

    // v67: the cabinet's narrow resonances and cancellations (peaks and notches that are only a few percent wide), found in
    // the spectra of seven mixed 4x12 recordings, rounded and with the deepest notches limited. 'Box' scales their depth.
    // The frequencies move with the cabinet: smaller boxes ring higher, the oversized ones lower.
    const Bump boxModes[] =
    {
        { 614.0f, -6.0f, 0.030f },
        { 644.0f, 2.6f, 0.100f },
        { 773.0f, -1.3f, 0.058f },
        { 974.0f, -3.2f, 0.054f },
        { 1001.0f, 3.1f, 0.063f },
        { 1137.0f, -6.0f, 0.046f },
        { 1180.0f, 1.8f, 0.030f },
        { 1250.0f, 3.9f, 0.030f },
        { 1530.0f, -2.1f, 0.090f },
        { 1725.0f, 1.6f, 0.054f },
        { 1885.0f, -3.1f, 0.051f },
        { 2030.0f, -2.1f, 0.054f },
        { 2380.0f, 2.1f, 0.078f },
        { 2575.0f, -3.0f, 0.041f },
        { 2745.0f, 1.6f, 0.063f },
        { 2985.0f, -2.3f, 0.068f },
        { 3390.0f, 2.0f, 0.046f },
        { 3570.0f, -1.8f, 0.034f },
        { 4420.0f, -1.4f, 0.034f }
    };

    inline double boxScale (int cabinet)
    {
        switch (cabinet)
        {
            case 0:  return 1.30;   // 1x12 open
            case 1:  return 1.18;   // 2x12 open
            case 2:  return 1.08;   // 2x12 closed
            case 6:  return 1.10;   // 4x12 open back
            case 9:  return 1.25;   // 1x12 closed
            case 10: return 1.45;   // 1x10 open
            case 11: return 1.30;   // 2x10 open
            case 12: return 1.12;   // 4x10 closed
            case 13: return 0.95;   // 1x15 closed
            case 14: return 1.70;   // 1x8 small
            case 15: return 0.90;   // oversized 2x12
            case 7:
            case 8:  return 0.88;   // oversized
            default: return 1.0;    // the 4x12 closed and straight/angled
        }
    }

    // The whole chain's magnitude response, in dB, at one frequency.
    inline double responseDb (const Params& p, double f)
    {
        const auto& s = speakers[juce::jlimit (0, kNumSpeakers - 1, p.speaker)];
        const auto& m = mics[juce::jlimit (0, kNumMics - 1, p.mic)];
        double db = 0.0;

        // Speaker
        db += highpassDb (f, (double) s.hpHz, (double) s.hpOrder);
        db += bumpDb (f, Bump { s.resHz, s.resDb, s.resOct });
        for (auto& b : s.bumps)
            db += bumpDb (f, b);
        db += lowpassDb (f, (double) s.lpHz, (double) s.lpOrder);
        const double lf = std::log (f);
        db += (double) s.ripple * (std::sin (11.0 * lf + (double) s.seed) + 0.6 * std::sin (23.0 * lf + 2.0 * (double) s.seed));

        // Where the mic points: the centre of the cap is bright, the edge is dark and warm
        // (v71: recordings of five-step position sweeps show about 40% of the old swing, so the measured setting scales it)
        db += (0.3 - (double) p.position) * (p.measured ? 5.8 : 14.0) * smooth01 (std::log2 (f / 1500.0) / 2.0);
        db += (double) p.position * 2.5 / (1.0 + (f / 250.0) * (f / 250.0));
        db += bumpDb (f, Bump { 2800.0f, -2.5f * (float) smooth01 (((double) p.position - 0.5) * 2.0), 0.5f });

        // The mic
        db += highpassDb (f, (double) m.hpHz, (double) m.hpOrder);
        for (auto& b : m.bumps)
            db += bumpDb (f, b);
        db += lowpassDb (f, (double) m.lpHz, (double) m.lpOrder);

        // Distance: close = more lows (proximity effect); far = a little less top
        db += lowShelfDb (f, 180.0, (double) m.proximity * (p.measured ? 3.5 : 10.0) / (1.0 + (double) p.distanceIn / 1.2));
        db -= ((double) p.distanceIn / 12.0) * 4.0 * smooth01 (std::log2 (f / 3000.0) / 1.7);

        // Angle: pointing away from the cone loses the highs
        db -= (double) m.offAxis * ((double) p.angleDeg / 60.0) * 9.0 * smooth01 (std::log2 (f / 2000.0) / 2.0);

        // The cabinet
        switch (p.cabinet)
        {
            case 0:  db += highpassDb (f, 105.0, 2.0) + bumpDb (f, Bump { 220.0f, 1.5f, 0.60f }); break;   // 1x12 open back
            case 1:  db += highpassDb (f,  90.0, 2.0) + bumpDb (f, Bump { 170.0f, 1.5f, 0.60f }); break;   // 2x12 open back
            case 2:  db += highpassDb (f,  70.0, 2.0) + bumpDb (f, Bump { 110.0f, 3.0f, 0.45f }); break;   // 2x12 closed
            case 4:  db += highpassDb (f,  65.0, 2.0) + bumpDb (f, Bump { 100.0f, 4.0f, 0.45f })                // 4x12 straight: the
                                                      + bumpDb (f, Bump { 400.0f, -1.5f, 0.50f })                // front is square to the mic,
                                                      + bumpDb (f, Bump { 2300.0f, 1.4f, 0.50f }); break;        // so more upper-mid focus
            case 5:  db += highpassDb (f,  65.0, 2.0) + bumpDb (f, Bump { 100.0f, 4.0f, 0.45f })                // 4x12 angled: the top speakers
                                                      + bumpDb (f, Bump { 400.0f, -1.5f, 0.50f })                // aim upward, so a mic on one
                                                      + bumpDb (f, Bump { 2300.0f, -0.6f, 0.50f })               // speaker hears a little less
                                                      + bumpDb (f, Bump { 150.0f, 0.8f, 0.60f })                 // forward focus, a little more
                                                      + bumpDb (f, Bump { 6000.0f, 0.8f, 0.50f }); break;        // floor / air
            case 6:  db += highpassDb (f,  95.0, 2.0) + bumpDb (f, Bump { 190.0f, 1.8f, 0.60f })                // 4x12 open back: airy, loose lows
                                                      + bumpDb (f, Bump { 1100.0f, 1.0f, 0.70f }); break;
            // Oversized (large internal volume, ported): the box resonance moves down, the low end extends and
            // stays tight (a ported cab rolls off steeply below its tuning), and the boxy low mids are cleaned up.
            case 7:  db += highpassDb (f,  46.0, 3.0) + bumpDb (f, Bump { 62.0f, 3.5f, 0.35f })
                                                      + bumpDb (f, Bump { 110.0f, 1.0f, 0.45f })
                                                      + bumpDb (f, Bump { 300.0f, -3.0f, 0.55f })
                                                      + bumpDb (f, Bump { 2300.0f, 1.2f, 0.50f }); break;        // oversized, straight
            case 8:  db += highpassDb (f,  46.0, 3.0) + bumpDb (f, Bump { 62.0f, 3.5f, 0.35f })
                                                      + bumpDb (f, Bump { 110.0f, 1.0f, 0.45f })
                                                      + bumpDb (f, Bump { 300.0f, -3.0f, 0.55f })
                                                      + bumpDb (f, Bump { 2300.0f, -0.6f, 0.50f })
                                                      + bumpDb (f, Bump { 6000.0f, 0.8f, 0.50f }); break;        // oversized, angled
            // v69: smaller and different boxes. Hand-built from what the sizes do (a smaller box resonates higher and
            // loses bass sooner; a closed back adds a low thump), not from measurements.
            case 9:  db += highpassDb (f,  85.0, 2.0) + bumpDb (f, Bump { 135.0f, 3.0f, 0.50f })
                                                      + bumpDb (f, Bump { 2300.0f, 1.0f, 0.50f }); break;        // 1x12 closed
            case 10: db += highpassDb (f, 125.0, 2.0) + bumpDb (f, Bump { 250.0f, 1.5f, 0.60f })
                                                      + bumpDb (f, Bump { 2800.0f, 1.2f, 0.50f }); break;        // 1x10 open
            case 11: db += highpassDb (f, 105.0, 2.0) + bumpDb (f, Bump { 205.0f, 1.8f, 0.60f })
                                                      + bumpDb (f, Bump { 1500.0f, 0.8f, 0.60f }); break;        // 2x10 open
            case 12: db += highpassDb (f,  80.0, 2.0) + bumpDb (f, Bump { 120.0f, 3.5f, 0.45f })
                                                      + bumpDb (f, Bump { 450.0f, -1.0f, 0.50f })
                                                      + bumpDb (f, Bump { 2600.0f, 1.0f, 0.50f }); break;        // 4x10 closed: punchy, mid-forward
            case 13: db += highpassDb (f,  55.0, 2.0) + bumpDb (f, Bump { 80.0f, 3.0f, 0.50f })
                                                      + bumpDb (f, Bump { 300.0f, -1.5f, 0.60f })
                                                      + bumpDb (f, Bump { 4500.0f, -2.0f, 0.60f }); break;       // 1x15 closed: deep, darker
            case 14: db += highpassDb (f, 180.0, 3.0) + bumpDb (f, Bump { 350.0f, 2.0f, 0.50f })
                                                      + bumpDb (f, Bump { 2500.0f, 1.5f, 0.50f }); break;        // 1x8 small: narrow and thin
            case 15: db += highpassDb (f,  50.0, 3.0) + bumpDb (f, Bump { 68.0f, 3.0f, 0.35f })
                                                      + bumpDb (f, Bump { 110.0f, 1.0f, 0.45f })
                                                      + bumpDb (f, Bump { 300.0f, -2.5f, 0.55f })
                                                      + bumpDb (f, Bump { 2300.0f, 0.8f, 0.50f }); break;        // oversized 2x12 (ported)
            default: db += highpassDb (f,  65.0, 2.0) + bumpDb (f, Bump { 100.0f, 4.0f, 0.45f })
                                                      + bumpDb (f, Bump { 400.0f, -1.5f, 0.50f }); break;  // 4x12 closed
        }

        // v67 controls. All are zero by default, so earlier sessions build exactly the same IR as before.
        if (p.tight != 0.0f)   // lows: + = tighter (less below ~150 Hz), - = fuller
            db -= 5.0 * (double) p.tight / (1.0 + std::pow (f / 120.0, 2.5));
        if (p.bite != 0.0f)    // upper-mid bite: the 2.4 kHz and 3.8 kHz region
            db += (double) p.bite * (3.0 * bumpDb (f, Bump { 2400.0f, 1.0f, 0.35f }) + 2.5 * bumpDb (f, Bump { 3800.0f, 1.0f, 0.40f }));
        if (p.box > 0.0f)
        {
            const double sc = boxScale (p.cabinet);
            for (auto& b : boxModes)
                db += (double) p.box * bumpDb (f, Bump { (float) (b.hz * sc), b.db, b.oct });
        }

        return std::max (db, -80.0);
    }

    // A plain radix-2 FFT (used only to build IRs, never on the audio thread).
    inline void fft (std::vector<std::complex<double>>& a, bool inverse)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
        {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap (a[i], a[j]);
        }
        for (size_t len = 2; len <= n; len <<= 1)
        {
            const double ang = 2.0 * 3.14159265358979323846 / (double) len * (inverse ? 1.0 : -1.0);
            const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (size_t i = 0; i < n; i += len)
            {
                std::complex<double> w (1.0, 0.0);
                for (size_t k = 0; k < len / 2; ++k)
                {
                    const std::complex<double> u = a[i + k], v = a[i + k + len / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
        if (inverse)
            for (auto& x : a)
                x /= (double) n;
    }

    // v68: three rooms, from the decay and tone of room-microphone impulse responses (two studio rooms and a living room).
    // Each has three bands (low, mid, high) with their own decay time (60 dB, seconds) and level (dB), and 'ratio' is how
    // much energy the room has compared with the direct sound when the Room knob is at 1.
    struct RoomModel { float t60Lo, t60Mid, t60Hi, dbLo, dbMid, dbHi, ratio, preMs, lpHz; };
    const RoomModel rooms[] =
    {
        { 0.26f, 0.20f, 0.15f, 7.0f, 0.0f,  -7.0f, 0.90f, 0.0f, 5200.0f },   // studio room
        { 0.30f, 0.22f, 0.13f, 5.0f, 0.0f,  -8.0f, 0.70f, 1.0f, 5000.0f },   // large studio room
        { 0.52f, 0.34f, 0.30f, 5.0f, 0.0f, -12.0f, 0.80f, 2.0f, 4200.0f },   // living room
    };

    // Builds the IR (mono, 48 kHz, 4096 samples; 16384 when a modelled room is on). Minimum phase, via the cepstrum:
    // log magnitude -> inverse FFT -> fold onto positive time -> FFT -> exp -> inverse FFT.
    inline std::vector<float> buildIR (const Params& p)
    {
        constexpr int N = 8192;
        const int outLen = p.roomType > 0 && p.room > 0.001f ? 16384 : 4096;
        constexpr double rate = 48000.0, ln10 = 2.302585092994046;

        std::vector<std::complex<double>> a ((size_t) N);
        for (int k = 0; k <= N / 2; ++k)
        {
            const double f = std::max (0.5 * rate / N, (double) k * rate / N);
            const double logMag = responseDb (p, f) / 20.0 * ln10;
            a[(size_t) k] = logMag;
            if (k > 0 && k < N / 2)
                a[(size_t) (N - k)] = logMag;
        }

        fft (a, true);   // real cepstrum

        std::vector<std::complex<double>> c ((size_t) N, std::complex<double> (0.0, 0.0));
        c[0] = a[0].real();
        for (int n = 1; n < N / 2; ++n)
            c[(size_t) n] = 2.0 * a[(size_t) n].real();
        c[(size_t) (N / 2)] = a[(size_t) (N / 2)].real();

        fft (c, false);
        for (auto& x : c)
            x = std::exp (x);
        fft (c, true);

        // Time of flight from the mic distance (about 3.55 samples per inch at 48 kHz)
        const int delay = (int) std::lround ((double) p.distanceIn * 3.554);
        std::vector<float> h ((size_t) outLen, 0.0f);
        for (int n = 0; n + delay < outLen; ++n)
            h[(size_t) (n + delay)] = (float) c[(size_t) n].real();

        // v68: a modelled room: three noise bands with their own decay, shaped by a low-pass
        if (p.room > 0.001f && p.roomType > 0)
        {
            const auto& rm = rooms[juce::jlimit (1, 3, p.roomType) - 1];
            double mainEnergy = 0.0;
            for (float v : h)
                mainEnergy += (double) v * (double) v;

            const int start = delay + 140 + (int) std::lround (rm.preMs * 48.0f);
            const double pi = 3.14159265358979323846, sec = 1.0 / rate;
            auto coef = [&] (double hz) { return 1.0 - std::exp (-2.0 * pi * hz * sec); };
            const double c300 = coef (300.0), c2k = coef (2000.0), cTop = coef ((double) rm.lpHz);
            const double gLo = std::pow (10.0, rm.dbLo / 20.0), gMid = std::pow (10.0, rm.dbMid / 20.0), gHi = std::pow (10.0, rm.dbHi / 20.0);
            std::vector<double> tail ((size_t) outLen, 0.0);
            uint32_t seed = 24680u;
            double a1 = 0.0, a2 = 0.0, b1 = 0.0, t1 = 0.0, t2 = 0.0, tailEnergy = 0.0;
            for (int n = start; n < outLen; ++n)
            {
                seed = seed * 1664525u + 1013904223u;
                const double r = (double) ((seed >> 8) & 0xFFFFu) / 32768.0 - 1.0;
                a1 += c300 * (r - a1);  a2 += c300 * (a1 - a2);                 // below about 300 Hz
                b1 += c2k * (r - b1);                                           // below about 2 kHz
                const double lo = a2, mid = b1 - a2, hi = r - b1;
                const double t = (double) (n - start) * sec;
                const double y = gLo * lo * std::exp (-6.9078 * t / (double) rm.t60Lo)
                               + gMid * mid * std::exp (-6.9078 * t / (double) rm.t60Mid)
                               + gHi * hi * std::exp (-6.9078 * t / (double) rm.t60Hi);
                t1 += cTop * (y - t1);  t2 += cTop * (t1 - t2);                 // top roll-off
                tail[(size_t) n] = t2;
                tailEnergy += t2 * t2;
            }
            if (tailEnergy > 1.0e-12)
            {
                const double scale = std::sqrt (mainEnergy * (double) rm.ratio * (double) p.room * (double) p.room / tailEnergy);
                for (int n = start; n < outLen; ++n)
                    h[(size_t) n] += (float) (scale * tail[(size_t) n]);
            }
        }
        // A little room: a short, dark, decaying noise tail after the direct sound
        else if (p.room > 0.001f)
        {
            double mainEnergy = 0.0;
            for (float v : h)
                mainEnergy += (double) v * (double) v;

            std::vector<double> tail ((size_t) outLen, 0.0);
            const int start = delay + 140;
            const double tau = 0.03 * rate;
            uint32_t seed = 12345u;
            double lp = 0.0, tailEnergy = 0.0;
            for (int n = start; n < outLen; ++n)
            {
                seed = seed * 1664525u + 1013904223u;
                const double r = (double) ((seed >> 8) & 0xFFFFu) / 32768.0 - 1.0;
                lp += 0.35 * (r - lp);
                tail[(size_t) n] = lp * std::exp (-(double) (n - start) / tau);
                tailEnergy += tail[(size_t) n] * tail[(size_t) n];
            }
            if (tailEnergy > 1.0e-12)
            {
                const double scale = std::sqrt (mainEnergy * std::pow (0.35 * (double) p.room, 2.0) / tailEnergy);
                for (int n = start; n < outLen; ++n)
                    h[(size_t) n] += (float) (scale * tail[(size_t) n]);
            }
        }

        // Fade out the last quarter so the IR ends cleanly
        const int fadeStart = outLen * 3 / 4;
        for (int n = fadeStart; n < outLen; ++n)
            h[(size_t) n] *= 0.5f * (1.0f + std::cos (3.14159265f * (float) (n - fadeStart) / (float) (outLen - fadeStart)));

        return h;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
StarterAmpProcessor::createParameterLayout()
{
    using F  = juce::AudioParameterFloat;
    using ID = juce::ParameterID;
    using R  = juce::NormalisableRange<float>;

    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<F> (ID { "inGain", 1 },   "Input (dB)",   R (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "gate", 1 },     "Gate (dB)",    R (-90.0f, -30.0f, 0.1f), -65.0f));

    // Pickup de-noise (in front of the gate)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "deNoiseOn", 1 }, "Pickup De-Noise", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "deNoiseHz", 1 }, "Hum Frequency",
                juce::StringArray { "60 Hz (US)", "50 Hz (EU)" }, 0));
    layout.add (std::make_unique<F> (ID { "deNoiseHum", 1 },  "Hum Reduction",  R (0.0f, 1.0f, 0.001f), 0.7f));
    layout.add (std::make_unique<F> (ID { "deNoiseHiss", 1 }, "Hiss Reduction", R (0.0f, 1.0f, 0.001f), 0.4f));

    // Noise pedal (pitch-shifting noise generator)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseOct", 1 },  "Noise: Octave Up", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseOct2", 1 }, "Noise: 2 Octaves Up", false));
    layout.add (std::make_unique<F> (ID { "noiseRise", 1 },  "Noise Rise",  R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "noiseSpeed", 1 }, "Noise Speed", R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "noisePanic", 1 }, "Noise Panic", R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "noiseChaos", 1 }, "Noise Chaos", R (0.0f, 1.0f, 0.001f), 0.3f));
    // Clash layer (dissonant pitch voices), plus latch/momentary mode per switch
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseGross", 1 }, "Noise: Clash", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "noiseGrossScene", 1 }, "Clash Scene",
                juce::StringArray { "Bright (tritone)", "Dark (minor 2nd)", "Full (all three)" }, 0));
    layout.add (std::make_unique<F> (ID { "noiseNuke", 1 },   "Clash Drive",  R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "noiseSicken", 1 }, "Clash Blend",  R (0.0f, 1.0f, 0.001f), 0.5f));
    // Ring modulator layer
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseRing", 1 }, "Noise: Ring Mod", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ringMode", 1 }, "Ring Mode",
                juce::StringArray { "Ring", "AM" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ringWave", 1 }, "Ring Wave",
                juce::StringArray { "Sine", "Square", "Triangle" }, 0));
    layout.add (std::make_unique<F> (ID { "ringFreq", 1 },   "Ring Freq (Hz)", R (1.0f, 5000.0f, 0.01f, 0.3f), 330.0f));
    layout.add (std::make_unique<F> (ID { "ringMix", 1 },    "Ring Mix",       R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<F> (ID { "ringWobble", 1 }, "Ring Wobble",    R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseRingLatch", 1 }, "Noise: Ring Mod Latch", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseOctLatch", 1 },   "Noise: Octave Latch", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseOct2Latch", 1 },  "Noise: 2 Octaves Latch", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseGrossLatch", 1 }, "Noise: Clash Latch", false));

    // Fuzz pedal (four styles: wall, tight, gated, buzzsaw)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzOn", 1 }, "Fuzz On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzzStyle", 1 }, "Fuzz Style",
                juce::StringArray { "Wall (sustained)", "Tight (op-amp)", "Gated (velcro)", "Buzzsaw (scooped)",
                                    "Face (round, germanium or silicon)", "Bender (raspy, 2 or 3 stage)", "Factory (gated, squealing)",
                                    "Nuke (wave-shaper)", "Crusher (bits and rate)" }, 0));
    layout.add (std::make_unique<F> (ID { "fuzzGain", 1 },  "Fuzz Gain",          R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<F> (ID { "fuzzTone", 1 },  "Fuzz Tone",          R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzzLevel", 1 }, "Fuzz Level (dB)",    R (-24.0f, 6.0f, 0.1f), -9.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzAuto", 1 }, "Fuzz Auto Level", false));
    layout.add (std::make_unique<F> (ID { "fuzzMix", 1 }, "Fuzz Mix", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "fuzzGate", 1 },  "Fuzz Gate (Gated style)", R (0.0f, 1.0f, 0.001f), 0.2f));
    layout.add (std::make_unique<F> (ID { "fuzzStab", 1 },  "Fuzz Stability (Gated style)", R (0.0f, 1.0f, 0.001f), 0.8f));
    // v56: the Face, Bender and Factory styles, the Wall options, and options for every style
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzGe", 1 }, "Fuzz Germanium (off = silicon)", true));
    layout.add (std::make_unique<F> (ID { "fuzzStarve", 1 }, "Fuzz Starve (battery)", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "fuzzInput", 1 },  "Fuzz Input (guitar volume)", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "fuzzComp", 1 },   "Fuzz Comp (Factory style)", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzzBenderModel", 1 }, "Fuzz Bender Model",
                juce::StringArray { "Early (2 stage)", "Classic (3 stage)", "Late (3 stage, tone sweep)" }, 1));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzBuffer", 1 }, "Fuzz Buffer In Front", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzzMuffVer", 1 }, "Fuzz Wall Version",
                juce::StringArray { "Standard", "Smooth (deep scoop)", "Gritty (thick)", "Mid-forward (bite)", "Heavy (fuzzy lows)", "Circuit (from the schematic)", "Circuit (variant B, 500 pF)", "Circuit (variant C, 470 pF)", "Circuit (op-amp build)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzzMuffDiode", 1 }, "Fuzz Wall Diodes",
                juce::StringArray { "Silicon", "LED (open)", "Germanium (soft)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzMuffOpAmp", 1 }, "Fuzz Wall Op-Amp Mode", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzMuffTonePass", 1 }, "Fuzz Wall Tone Bypass", false));
    layout.add (std::make_unique<F> (ID { "fuzzMuffMids", 1 }, "Fuzz Wall Mids", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzzDrift", 1 },  "Fuzz Temperature Drift", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "fuzzHiss", 1 },   "Fuzz Hiss", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzzOctave", 1 }, "Fuzz Octave Up", false));
    layout.add (std::make_unique<F> (ID { "fuzzShape", 1 },  "Fuzz Shape (Nuke style)",   R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzzCrush", 1 },  "Fuzz Crush (Nuke style)",   R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "fuzzBits", 1 },   "Fuzz Bits (Crusher style)", R (1.0f, 16.0f, 1.0f), 8.0f));
    layout.add (std::make_unique<F> (ID { "fuzzRate", 1 },   "Fuzz Rate (Crusher style)", R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "buzzLow", 1 },   "Buzzsaw Low",        R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<F> (ID { "buzzHigh", 1 },  "Buzzsaw High",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "buzzMid", 1 },   "Buzzsaw Mid",        R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "buzzSoft", 1 }, "Buzzsaw Soft Clip", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "buzzHard", 1 }, "Buzzsaw Hard Clip", true));
    layout.add (std::make_unique<F> (ID { "buzzBlend", 1 }, "Buzzsaw Blend",      R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "buzzPol", 1 },  "Buzzsaw Blend Invert", false));
    // Fuzz 2 (v93): a full copy of the fuzz block, for stacking (series) or running beside the first (parallel)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2On", 1 }, "Fuzz 2 On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzz2Style", 1 }, "Fuzz 2 Style",
                juce::StringArray { "Wall (sustained)", "Tight (op-amp)", "Gated (velcro)", "Buzzsaw (scooped)",
                                    "Face (round, germanium or silicon)", "Bender (raspy, 2 or 3 stage)", "Factory (gated, squealing)",
                                    "Nuke (wave-shaper)", "Crusher (bits and rate)" }, 0));
    layout.add (std::make_unique<F> (ID { "fuzz2Gain", 1 },  "Fuzz 2 Gain",          R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<F> (ID { "fuzz2Tone", 1 },  "Fuzz 2 Tone",          R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzz2Level", 1 }, "Fuzz 2 Level (dB)",    R (-24.0f, 6.0f, 0.1f), -9.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2Auto", 1 }, "Fuzz 2 Auto Level", false));
    layout.add (std::make_unique<F> (ID { "fuzz2Mix", 1 }, "Fuzz 2 Mix", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "fuzz2Gate", 1 },  "Fuzz 2 Gate (Gated style)", R (0.0f, 1.0f, 0.001f), 0.2f));
    layout.add (std::make_unique<F> (ID { "fuzz2Stab", 1 },  "Fuzz 2 Stability (Gated style)", R (0.0f, 1.0f, 0.001f), 0.8f));
    // v56: the Face, Bender and Factory styles, the Wall options, and options for every style
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2Ge", 1 }, "Fuzz 2 Germanium (off = silicon)", true));
    layout.add (std::make_unique<F> (ID { "fuzz2Starve", 1 }, "Fuzz 2 Starve (battery)", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "fuzz2Input", 1 },  "Fuzz 2 Input (guitar volume)", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "fuzz2Comp", 1 },   "Fuzz 2 Comp (Factory style)", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzz2BenderModel", 1 }, "Fuzz 2 Bender Model",
                juce::StringArray { "Early (2 stage)", "Classic (3 stage)", "Late (3 stage, tone sweep)" }, 1));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2Buffer", 1 }, "Fuzz 2 Buffer In Front", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzz2MuffVer", 1 }, "Fuzz 2 Wall Version",
                juce::StringArray { "Standard", "Smooth (deep scoop)", "Gritty (thick)", "Mid-forward (bite)", "Heavy (fuzzy lows)", "Circuit (from the schematic)", "Circuit (variant B, 500 pF)", "Circuit (variant C, 470 pF)", "Circuit (op-amp build)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzz2MuffDiode", 1 }, "Fuzz 2 Wall Diodes",
                juce::StringArray { "Silicon", "LED (open)", "Germanium (soft)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2MuffOpAmp", 1 }, "Fuzz 2 Wall Op-Amp Mode", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2MuffTonePass", 1 }, "Fuzz 2 Wall Tone Bypass", false));
    layout.add (std::make_unique<F> (ID { "fuzz2MuffMids", 1 }, "Fuzz 2 Wall Mids", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzz2Drift", 1 },  "Fuzz 2 Temperature Drift", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "fuzz2Hiss", 1 },   "Fuzz 2 Hiss", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2Octave", 1 }, "Fuzz 2 Octave Up", false));
    layout.add (std::make_unique<F> (ID { "fuzz2Shape", 1 },  "Fuzz 2 Shape (Nuke style)",   R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzz2Crush", 1 },  "Fuzz 2 Crush (Nuke style)",   R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "fuzz2Bits", 1 },   "Fuzz 2 Bits (Crusher style)", R (1.0f, 16.0f, 1.0f), 8.0f));
    layout.add (std::make_unique<F> (ID { "fuzz2Rate", 1 },   "Fuzz 2 Rate (Crusher style)", R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "fuzz2BuzzLow", 1 },   "Fuzz 2 Buzzsaw Low",        R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<F> (ID { "fuzz2BuzzHigh", 1 },  "Fuzz 2 Buzzsaw High",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "fuzz2BuzzMid", 1 },   "Fuzz 2 Buzzsaw Mid",        R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2BuzzSoft", 1 }, "Fuzz 2 Buzzsaw Soft Clip", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2BuzzHard", 1 }, "Fuzz 2 Buzzsaw Hard Clip", true));
    layout.add (std::make_unique<F> (ID { "fuzz2BuzzBlend", 1 }, "Fuzz 2 Buzzsaw Blend",      R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "fuzz2BuzzPol", 1 },  "Fuzz 2 Buzzsaw Blend Invert", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "fuzz2Route", 1 }, "Fuzz 2 Route",
                juce::StringArray { "Series (after Fuzz 1)", "Parallel (beside Fuzz 1)" }, 0));


    // Compressor, before the amp
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "compOn", 1 }, "Comp On", false));
    layout.add (std::make_unique<F> (ID { "compThresh", 1 }, "Comp Threshold (dB)", R (-40.0f, 0.0f, 0.1f), -18.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "compRatio", 1 }, "Comp Ratio",
                juce::StringArray { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "10:1 Soft", "20:1", "Limit" }, 4));
    layout.add (std::make_unique<F> (ID { "compAttack", 1 },  "Comp Attack (ms)",  R (0.05f, 50.0f, 0.01f, 0.4f), 5.0f));
    layout.add (std::make_unique<F> (ID { "compRelease", 1 }, "Comp Release (ms)", R (50.0f, 3500.0f, 1.0f, 0.4f), 250.0f));
    layout.add (std::make_unique<F> (ID { "compOutput", 1 },  "Comp Output (dB)",  R (-12.0f, 18.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "compAuto", 1 }, "Comp Auto Level", false));
    layout.add (std::make_unique<F> (ID { "compMix", 1 },     "Comp Mix",          R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "compBloom", 1 },   "Comp Bloom",        R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "compSmack", 1 },   "Comp Smack (parallel)", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "compClarity", 1 }, "Comp Clarity (tilt)", R (-1.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "compDetHp", 1 }, "Comp Detector HP",
                juce::StringArray { "Off", "80 Hz", "160 Hz", "320 Hz" }, 1));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "compSat", 1 }, "Comp Saturation",
                juce::StringArray { "Off", "Tube-like", "Tape-like", "Both", "Transformer", "FET stage", "Opto tube stage", "Mu iron + tube", "Diode" }, 1));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "compStyle", 1 }, "Comp Style",
                juce::StringArray { "Standard", "FET", "Opto", "Variable-mu", "Sustainer", "Bus", "Dyna (OTA)" }, 0));

    // Overdrive
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "odOn", 1 }, "OD On", true));
    layout.add (std::make_unique<F> (ID { "odGain", 1 },   "OD Gain",      R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "odTone", 1 },   "OD Tone",      R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "odEdge", 1 },   "OD Edge",      R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "odStyle", 1 }, "OD Style",
                juce::StringArray { "Transparent", "Tight", "Open", "Screamer", "Distortion", "Tube Driver", "Op-Amp Distortion", "Stack Drive" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "odRange", 1 }, "OD Range (Open style, on = high)", true));
    layout.add (std::make_unique<F> (ID { "odAttack", 1 }, "OD Attack (Tight style, 0 to 5 = positions 1 to 6)", R (0.0f, 5.0f, 1.0f), 2.0f));
    layout.add (std::make_unique<F> (ID { "odGate", 1 },   "OD Gate (Precision)", R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "odVolume", 1 }, "OD Volume (dB)", R (-24.0f, 12.0f, 0.1f), -6.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "odAuto", 1 }, "OD Auto Level", false));
    layout.add (std::make_unique<F> (ID { "odMix", 1 },    "OD Mix",          R (0.0f, 1.0f, 0.001f), 1.0f));      // v94
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "odGrip", 1 }, "OD Grip (Transparent)", false));
    // v58: the Screamer and Distortion styles
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "odClip", 1 }, "OD Clipping (Screamer, Distortion)",
                juce::StringArray { "Silicon (classic)", "LED (open)", "Germanium (soft)", "Asymmetric", "None (op-amp rails)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "odVoice", 1 }, "OD Voicing (Screamer)",
                juce::StringArray { "Classic (mid hump)", "Mild (lean)", "Open (bright)" }, 0));
    layout.add (std::make_unique<F> (ID { "odBass", 1 },    "OD Bass (Screamer)",     R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "odMid", 1 },     "OD Mid (Screamer)",      R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "odMidFreq", 1 }, "OD Mid Freq (Screamer)", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "odMidMod", 1 }, "OD Mids (Distortion)",
                juce::StringArray { "Scoop (stock)", "Flat", "Boost" }, 0));

    // Drive 2 (v93): a full copy of the drive block, for stacking (series) or running beside the first (parallel)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "od2On", 1 }, "Drive 2 On", false));
    layout.add (std::make_unique<F> (ID { "od2Gain", 1 },   "Drive 2 Gain",      R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "od2Tone", 1 },   "Drive 2 Tone",      R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "od2Edge", 1 },   "Drive 2 Edge",      R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "od2Style", 1 }, "Drive 2 Style",
                juce::StringArray { "Transparent", "Tight", "Open", "Screamer", "Distortion", "Tube Driver", "Op-Amp Distortion", "Stack Drive" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "od2Range", 1 }, "Drive 2 Range (Open style, on = high)", true));
    layout.add (std::make_unique<F> (ID { "od2Attack", 1 }, "Drive 2 Attack (Tight style, 0 to 5 = positions 1 to 6)", R (0.0f, 5.0f, 1.0f), 2.0f));
    layout.add (std::make_unique<F> (ID { "od2Gate", 1 },   "Drive 2 Gate (Precision)", R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "od2Volume", 1 }, "Drive 2 Volume (dB)", R (-24.0f, 12.0f, 0.1f), -6.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "od2Auto", 1 }, "Drive 2 Auto Level", false));
    layout.add (std::make_unique<F> (ID { "od2Mix", 1 },    "Drive 2 Mix",     R (0.0f, 1.0f, 0.001f), 1.0f));      // v94
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "od2Grip", 1 }, "Drive 2 Grip (Transparent)", false));
    // v58: the Screamer and Distortion styles
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "od2Clip", 1 }, "Drive 2 Clipping (Screamer, Distortion)",
                juce::StringArray { "Silicon (classic)", "LED (open)", "Germanium (soft)", "Asymmetric", "None (op-amp rails)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "od2Voice", 1 }, "Drive 2 Voicing (Screamer)",
                juce::StringArray { "Classic (mid hump)", "Mild (lean)", "Open (bright)" }, 0));
    layout.add (std::make_unique<F> (ID { "od2Bass", 1 },    "Drive 2 Bass (Screamer)",     R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "od2Mid", 1 },     "Drive 2 Mid (Screamer)",      R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "od2MidFreq", 1 }, "Drive 2 Mid Freq (Screamer)", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "od2MidMod", 1 }, "Drive 2 Mids (Distortion)",
                juce::StringArray { "Scoop (stock)", "Flat", "Boost" }, 0));

    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "od2Route", 1 }, "Drive 2 Route",
                juce::StringArray { "Series (after Drive 1)", "Parallel (beside Drive 1)" }, 0));

    // Amp
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampModel", 1 }, "Amp Model",
                juce::StringArray { "Clean (sparkly)", "Vintage Chime (class-A)",
                                    "British Hot-Rod (tight)",
                                    "Saturated Lead (smooth)",
                                    "Modern High-Gain (scooped)",
                                    "Acoustic (piezo to acoustic)",
                                    "Classic Rock (crunch)",
                                    "Boutique Lead (smooth, singing)",
                                    "Tight Metal (raw to modern)",
                                    "Low-Watt Combo (spongy breakup)",
                                    "Glass Clean (solid-state)",
                                    "Creamy Lead (tight, mid-hump)",
                                    "Doom Stack (thick, loose)",
                                    "Precision High-Gain (tight, modern)",
                                    "Digital Extreme (brutal, digital)",
                                    "Master-Volume Rock (80s hard rock)" }, 2));
    // Amp realism (v41): the same seven controls for Amp A and Amp B
    {
        for (const char* pre : { "amp", "ampB" })
        {
            const juce::String p (pre);
            const juce::String n = p == "amp" ? "Amp " : "Amp B ";
            layout.add (std::make_unique<juce::AudioParameterBool> (ID { p + "Bright", 1 }, n + "Bright Switch", false));
            layout.add (std::make_unique<F> (ID { p + "Bias", 1 },   n + "Bias (cold to hot)", R (-1.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { p + "Power", 1 },  n + "Power Amp Drive", R (0.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { p + "Reso", 1 },   n + "Resonance (low-end damping)", R (0.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { p + "SagAmt", 1 }, n + "Sag", R (0.0f, 1.0f, 0.001f), 0.5f));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "Tube", 1 }, n + "Preamp Tube",
                        juce::StringArray { "12AX7 (high gain)", "12AT7 (medium)", "12AY7 (low gain)" }, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "Rect", 1 }, n + "Rectifier",
                        juce::StringArray { "Tube (full sag)", "Silicon (tight)" }, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "PwrTube", 1 }, n + "Power Tube",
                        juce::StringArray { "As voiced", "EL34", "6L6", "6V6", "EL84", "KT88" }, 0));
            // v59: amp-wide switches (feedback feel, power scaling, tone stack, class, deep)
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "Feel", 1 }, n + "Feel (feedback)",
                        juce::StringArray { "Loose feel (low feedback)", "Normal feel", "Tight feel (high feedback)" }, 1));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "Watt", 1 }, n + "Power Scaling",
                        juce::StringArray { "Full power", "Half power", "Low power" }, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "Stack", 1 }, n + "Tone Stack",
                        juce::StringArray { "Stack as voiced", "Neutral (flat)",
                                    "Passive stack: American combo", "Passive stack: British 50 W", "Passive stack: British lead", "Passive stack: American lead", "Passive stack: chime (class A)",
                                    "Passive stack: high-gain, 680 pF", "Passive stack: high-gain, 500 pF" }, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { p + "Class", 1 }, n + "Output Class",
                        juce::StringArray { "Class as voiced", "Class A (rich, compressed)", "Class AB (tight, punchy)" }, 0));
            layout.add (std::make_unique<juce::AudioParameterBool> (ID { p + "Deep", 1 }, n + "Deep Switch", false));
        }
    }
    // Parallel Amp B (v40): a second electric amp beside the first. It shares the overdrive pedal and
    // Amp Focus with Amp A; everything else is its own. Each cab block picks Amp A, Amp B or both.
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "ampBOn", 1 }, "Amp B On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampBModel", 1 }, "Amp B Model",
                juce::StringArray { "Clean (sparkly)", "Vintage Chime (class-A)", "British Hot-Rod (tight)",
                                    "Saturated Lead (smooth)", "Modern High-Gain (scooped)", "Classic Rock (crunch)",
                                    "Boutique Lead (smooth, singing)", "Tight Metal (raw to modern)",
                                    "Low-Watt Combo (spongy breakup)", "Glass Clean (solid-state)",
                                    "Creamy Lead (tight, mid-hump)", "Doom Stack (thick, loose)",
                                    "Precision High-Gain (tight, modern)", "Digital Extreme (brutal, digital)",
                                    "Master-Volume Rock (80s hard rock)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampBVoicing", 1 }, "Amp B Voicing Switch",
                juce::StringArray { "Voicing 1", "Voicing 2", "Voicing 3", "Voicing 4" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "ampBUseOd", 1 }, "Amp B Uses Overdrive", true));
    layout.add (std::make_unique<F> (ID { "ampBGain", 1 },     "Amp B Gain",     R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "ampBTight", 1 },    "Amp B Tight",    R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "ampBBass", 1 },     "Amp B Bass (dB)",     R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "ampBLowMid", 1 },   "Amp B Low Mid (dB)",  R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "ampBHighMid", 1 },  "Amp B High Mid (dB)", R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "ampBTreble", 1 },   "Amp B Treble (dB)",   R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "ampBPresence", 1 }, "Amp B Presence",      R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "ampALevel", 1 },    "Amp A Level (dB)",    R (-24.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "ampBLevel", 1 },    "Amp B Level (dB)",    R (-24.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "ampAutoLevel", 1 },  "Amp A Auto Level", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "ampBAutoLevel", 1 }, "Amp B Auto Level", false));
    // Acoustic / piezo model (used when Amp Model = Acoustic)
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "acBody", 1 }, "Acoustic Body",
                juce::StringArray { "Body: Dreadnought", "Body: Jumbo", "Body: Parlor / OM", "Body: Nylon classical" }, 0));
    layout.add (std::make_unique<F> (ID { "acBodyAmt", 1 }, "Acoustic Body Amount",  R (0.0f, 1.0f, 0.001f), 0.8f));
    layout.add (std::make_unique<F> (ID { "acFix", 1 },     "Acoustic Piezo Fix",    R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<F> (ID { "acPreamp", 1 },  "Acoustic Preamp Warmth", R (0.0f, 1.0f, 0.001f), 0.2f));
    layout.add (std::make_unique<F> (ID { "acSqueeze", 1 }, "Acoustic Squeeze",      R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "acSparkle", 1 }, "Acoustic Sparkle (dB)", R (-6.0f, 6.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "acWarmth", 1 },  "Acoustic Warmth (dB)",  R (-6.0f, 6.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "acRoom", 1 },    "Acoustic Room",         R (0.0f, 1.0f, 0.001f), 0.25f));
    layout.add (std::make_unique<F> (ID { "acLevel", 1 },   "Acoustic Level (dB)",   R (-12.0f, 12.0f, 0.1f), 0.0f));
    // Modern Metal controls (British Hot-Rod, Saturated Lead and Modern High-Gain amps)
    layout.add (std::make_unique<F> (ID { "mtTight", 1 },  "Metal Tight",     R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "mtDepth", 1 },  "Metal Depth",     R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "mtMidHz", 1 },  "Metal Mid Freq",  R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "mtMid", 1 },    "Metal Mid Boost", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "mtFizz", 1 },   "Metal Fizz Tamer", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "mtSag", 1 },    "Metal Sag",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "mtChug", 1 },   "Metal Chug Gate", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "mtLead", 1 }, "Metal Lead", false));
    // Pitch pedal and octaver (v41)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "wowOn", 1 }, "Pitch Pedal On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "wowMode", 1 }, "Pitch Pedal Mode",
                juce::StringArray { "Octave up", "Two octaves up", "Octave down", "Two octaves down", "Dive bomb", "Bend (-12 to +12)", "Custom (heel and toe)" }, 0));
    layout.add (std::make_unique<F> (ID { "wowPos", 1 },   "Pitch Pedal Position (heel to toe)", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "wowHeel", 1 },  "Pitch Pedal Heel (semitones)", R (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "wowToe", 1 },   "Pitch Pedal Toe (semitones)",  R (-24.0f, 24.0f, 0.1f), 12.0f));
    layout.add (std::make_unique<F> (ID { "wowGlide", 1 }, "Pitch Pedal Glide (ms)", R (0.0f, 500.0f, 1.0f, 0.5f), 40.0f));
    layout.add (std::make_unique<F> (ID { "wowMix", 1 },   "Pitch Pedal Mix", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "octOn", 1 }, "Octaver On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "octA", 1 }, "Octave 1",
                juce::StringArray { "2 octaves down", "1 octave down", "1 octave up", "2 octaves up" }, 1));
    layout.add (std::make_unique<F> (ID { "octALvl", 1 }, "Octave 1 Level", R (0.0f, 1.0f, 0.001f), 0.6f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "octB", 1 }, "Octave 2",
                juce::StringArray { "2 octaves down", "1 octave down", "1 octave up", "2 octaves up" }, 2));
    layout.add (std::make_unique<F> (ID { "octBLvl", 1 }, "Octave 2 Level", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "octDry", 1 },  "Octaver Dry Level", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "octTone", 1 }, "Octaver Tone", R (0.0f, 1.0f, 0.001f), 0.5f));
    // Transpose / formant, at the start of the chain
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "tpOn", 1 }, "Transpose On", false));
    layout.add (std::make_unique<F> (ID { "tpSemis", 1 },   "Transpose (semitones)", R (-24.0f, 24.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<F> (ID { "tpFine", 1 },    "Transpose Fine (cents)", R (-50.0f, 50.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<F> (ID { "tpFormant", 1 }, "Formant Shift (semitones)", R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "tpLock", 1 }, "Formant Lock", true));
    layout.add (std::make_unique<F> (ID { "tpMix", 1 },     "Transpose Mix", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "tpQuality", 1 }, "Transpose Mode",
                juce::StringArray { "Fast (~14 ms delay)", "Smooth (~27 ms delay)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampQuality", 1 }, "Amp Quality",
                juce::StringArray { "Best (4x, more CPU)", "Balanced (2x)", "Lowest latency (no oversampling)" }, 1));
    // Global mode and the optional final limiter (v33)
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "perfMode", 1 }, "Mode",
                juce::StringArray { "Normal", "Low Latency", "High Quality" }, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "limOn", 1 }, "Limiter On", false));
    layout.add (std::make_unique<F> (ID { "limCeil", 1 }, "Limiter Ceiling (dB)", R (-12.0f, 0.0f, 0.1f), -1.0f));
    layout.add (std::make_unique<F> (ID { "limRel", 1 },  "Limiter Release (ms)", R (10.0f, 1000.0f, 1.0f, 0.4f), 150.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "outMono", 1 }, "Mono Output", false));
    layout.add (std::make_unique<F> (ID { "ampTune", 1 }, "Amp Focus (low-tuning tightening)", R (0.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampVoicing", 1 },
                "Voicing Switch (alternate voicings for the chosen amp)", juce::StringArray { "Voicing 1", "Voicing 2", "Voicing 3", "Voicing 4" }, 0));
    layout.add (std::make_unique<F> (ID { "ampGain", 1 },  "Amp Gain",     R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "bass", 1 },     "Bass (dB)",    R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "lowMid", 1 },   "Low Mid (dB)",  R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "highMid", 1 },  "High Mid (dB)", R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "treble", 1 },   "Treble (dB)",  R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "presence", 1 }, "Presence",     R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "cabOn", 1 }, "Cab On", true));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabType", 1 }, "Built-in Cab",
                juce::StringArray { "Amp default", "1x12 Open", "2x12 Combo", "4x12 Closed", "1x12 Bright" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabRoomType", 1 }, "Cab Room Type",
                juce::StringArray { "Original (dark)", "Studio room", "Large studio room", "Living room" }, 0));
    layout.add (std::make_unique<F> (ID { "cabReso", 1 },     "Cab Resonance (dB)", R (-6.0f, 9.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "cabResoHz", 1 },   "Cab Resonance Freq (Hz)", R (50.0f, 200.0f, 1.0f), 100.0f));
    layout.add (std::make_unique<F> (ID { "cabAir", 1 },      "Cab Air (dB)",       R (-6.0f, 6.0f, 0.1f), 0.0f));
    // Cab blocks A to D. Each is a built-in model or an IR file, with its own level, pan, delay,
    // polarity and filters. Only block A is on by default. Cabinet and room are shared.
    {
        static const int   defSpeaker[kCabBlocks] = { 0, 0, 1, 3 };
        static const int   defMic[kCabBlocks]     = { 0, 3, 1, 2 };
        static const float defPos[kCabBlocks]     = { 0.3f, 0.55f, 0.4f, 0.5f };
        static const float defDist[kCabBlocks]    = { 1.0f, 3.0f, 2.0f, 4.0f };

        for (int blk = 0; blk < kCabBlocks; ++blk)
        {
            const juce::String s = cabLetter (blk);
            const juce::String n = "IR " + s + " ";
            layout.add (std::make_unique<juce::AudioParameterBool> (ID { "cabOn" + s, 1 }, n + "On", blk == 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabSrc" + s, 1 }, n + "Source",
                        juce::StringArray { "Built-in model", "IR file" }, 0));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabSpk" + s, 1 }, n + "Speaker", cabbuilder::speakerNames(), defSpeaker[blk]));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabCbn" + s, 1 }, n + "Cabinet", cabbuilder::cabinetNames(), 3));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabMic" + s, 1 }, n + "Mic", cabbuilder::micNames(), defMic[blk]));
            layout.add (std::make_unique<F> (ID { "cabPos" + s, 1 },  n + "Mic Position", R (0.0f, 1.0f, 0.001f), defPos[blk]));
            layout.add (std::make_unique<F> (ID { "cabDist" + s, 1 }, n + "Mic Distance (in)", R (0.0f, 12.0f, 0.1f), defDist[blk]));
            layout.add (std::make_unique<F> (ID { "cabAng" + s, 1 },  n + "Mic Angle", R (0.0f, 60.0f, 0.5f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabLevel" + s, 1 }, n + "Level (dB)", R (-24.0f, 12.0f, 0.1f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabPan" + s, 1 },   n + "Pan", R (-1.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabDelay" + s, 1 }, n + "Delay (ms)", R (0.0f, 10.0f, 0.01f), 0.0f));
            layout.add (std::make_unique<juce::AudioParameterBool> (ID { "cabPol" + s, 1 }, n + "Invert", false));
            layout.add (std::make_unique<F> (ID { "cabLowCut" + s, 1 },  n + "Low Cut (Hz)",  R (20.0f, 500.0f, 1.0f, 0.4f), 20.0f));
            layout.add (std::make_unique<F> (ID { "cabHighCut" + s, 1 }, n + "High Cut (Hz)", R (2000.0f, 20000.0f, 10.0f, 0.4f), 20000.0f));
            layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabAmp" + s, 1 }, n + "Amp Source",
                        juce::StringArray { "Amp A", "Amp B", "Amp A + B" }, 2));
            static const char* eqNames[5] = { "EQ Low Shelf (dB)", "EQ Low Mid (dB)", "EQ Mid (dB)", "EQ High Mid (dB)", "EQ High Shelf (dB)" };
            for (int k = 0; k < 5; ++k)
                layout.add (std::make_unique<F> (ID { "cabEq" + juce::String (k + 1) + s, 1 }, n + eqNames[k], R (-12.0f, 12.0f, 0.1f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabRso" + s, 1 }, n + "Resonance (dB)", R (-6.0f, 9.0f, 0.1f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabPhs" + s, 1 }, n + "Phase", R (0.0f, 1.0f, 0.001f), 0.0f));
            // v67: built-in cab macros (all neutral at their defaults)
            layout.add (std::make_unique<F> (ID { "cabTight" + s, 1 }, n + "Tight", R (-1.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabBite" + s, 1 },  n + "Bite",  R (-1.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabBox" + s, 1 },   n + "Box",   R (0.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { "cabTilt" + s, 1 },  n + "Tilt (dB)", R (-12.0f, 12.0f, 0.1f), 0.0f));   // v70: bright to dark
        }
    }
    layout.add (std::make_unique<F> (ID { "cabRoom", 1 }, "Cab Room", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "cabDyn", 1 }, "Speaker Dynamics", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "cabDynMode", 1 }, "Speaker Dynamics Mode",
                juce::StringArray { "Tight (modern)", "Vintage (spongy)", "Real: tight", "Real: vintage" }, 0));
    layout.add (std::make_unique<F> (ID { "cabTame", 1 }, "Cab Resonance Tamer", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "cabTameDelta", 1 }, "Cab Resonance Tamer: Listen to Cuts", false));
    layout.add (std::make_unique<F> (ID { "cabMotion", 1 }, "Cab Motion", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "cabMotionRate", 1 }, "Cab Motion Rate", R (0.0f, 1.0f, 0.001f), 0.4f));

    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "cabAutoAlign", 1 }, "Auto Align IRs", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "cabMeasured", 1 }, "Cab Realistic Response", true));   // v71
    layout.add (std::make_unique<F> (ID { "cabSpread", 1 }, "Cab Stereo Spread", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "cabLowCut", 1 }, "Cab Low Cut (Hz)",  R (20.0f, 500.0f, 1.0f, 0.4f), 20.0f));
    layout.add (std::make_unique<F> (ID { "cabHighCut", 1 }, "Cab High Cut (Hz)", R (2000.0f, 20000.0f, 10.0f, 0.4f), 20000.0f));
    layout.add (std::make_unique<F> (ID { "cabTilt", 1 }, "Cab Tilt (dB)", R (-6.0f, 6.0f, 0.1f), 0.0f));

    // Delays: two completely separate sets of controls (Mix at 0 = off).
    // The pre-amp delay's repeats go through the overdrive and preamp.
    layout.add (std::make_unique<F> (ID { "preDelayMix", 1 },       "Pre-Amp Delay Mix",         R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "preDelayTime", 1 },      "Pre-Amp Delay Time (ms)",   R (20.0f, 1000.0f, 1.0f), 250.0f));
    layout.add (std::make_unique<F> (ID { "preDelayFeedback", 1 },  "Pre-Amp Delay Feedback",    R (0.0f, 0.9f, 0.001f), 0.30f));
    layout.add (std::make_unique<F> (ID { "preDelayLofi", 1 },      "Pre-Amp Delay Lo-Fi",       R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "preDelayWarble", 1 },    "Pre-Amp Delay Tape Warble", R (0.0f, 1.0f, 0.001f), 0.0f));

    layout.add (std::make_unique<F> (ID { "postDelayMix", 1 },      "Post-Amp Delay Mix",         R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "postDelayTime", 1 },     "Post-Amp Delay Time (ms)",   R (20.0f, 1000.0f, 1.0f), 380.0f));
    layout.add (std::make_unique<F> (ID { "postDelayFeedback", 1 }, "Post-Amp Delay Feedback",    R (0.0f, 0.9f, 0.001f), 0.35f));
    layout.add (std::make_unique<F> (ID { "postDelayLofi", 1 },     "Post-Amp Delay Lo-Fi",       R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "postDelayWarble", 1 },   "Post-Amp Delay Tape Warble", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "postDelayStereo", 1 }, "Post-Amp Delay Stereo",
                juce::StringArray { "Stereo (same time)", "Ping-Pong (left first)", "Ping-Pong (right first)", "Dual (right offset)" }, 0));
    layout.add (std::make_unique<F> (ID { "postDelayOffset", 1 },   "Post-Amp Delay Right Offset", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "postDelaySpread", 1 },   "Post-Amp Delay Spread", R (0.0f, 1.0f, 0.001f), 1.0f));

    // Reverb (Mix at 0 = off)
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "reverbType", 1 }, "Reverb Type",
                juce::StringArray { "Classic", "Room", "Hall", "Plate", "Ambient", "Cathedral", "Spring", "Soft Plate", "Tumble", "Abyss" }, 0));
    layout.add (std::make_unique<F> (ID { "reverbMix", 1 },     "Reverb Mix",      R (0.0f, 1.0f, 0.001f), 0.0f));
    // Decay time in seconds (this was a 0 to 1 "Size" knob before v31; old sessions are converted on load)
    layout.add (std::make_unique<F> (ID { "reverbSize", 1 },    "Reverb Decay (s)", R (0.1f, 30.0f, 0.01f, 0.4f), 2.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "shimmerOn", 1 }, "Shimmer", false));
    layout.add (std::make_unique<F> (ID { "shimmerAmount", 1 }, "Shimmer Amount",  R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "shimmerPitch", 1 }, "Shimmer Pitch",
                juce::StringArray { "+12 (octave)", "+7 (fifth)", "+24 (two octaves)", "-12 (octave down)",
                                    "+12 & +7 (octave + fifth)", "+12 & -12 (up and down)", "+5 (fourth)", "+19 (octave + fifth up)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "shimmerMode", 1 }, "Shimmer Mode",
                juce::StringArray { "Feedback (blooming)", "Input (no build-up)", "Both" }, 0));
    layout.add (std::make_unique<F> (ID { "shimmerDetune", 1 }, "Shimmer Detune (cents)", R (0.0f, 50.0f, 0.1f), 7.0f));
    // Input source, tempo (shared by both delays), and the delays' note values
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "inputMode", 1 }, "Input",
                juce::StringArray { "Input: Left", "Input: Right", "Input: Mono (L + R)" }, 0));
    layout.add (std::make_unique<F> (ID { "tempoBpm", 1 }, "Tempo (BPM)", R (40.0f, 300.0f, 0.1f), 120.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "tempoHost", 1 }, "Use Host Tempo", false));
    layout.add (std::make_unique<F> (ID { "tunerRef", 1 }, "Tuner Reference (A, Hz)", R (427.0f, 453.0f, 1.0f), 440.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "metroOn", 1 }, "Metronome On", false));
    layout.add (std::make_unique<juce::AudioParameterInt> (ID { "metroBeats", 1 }, "Metronome Beats per Bar", 1, 12, 4));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "metroSub", 1 }, "Metronome Subdivision",
                                                               juce::StringArray { "None", "8ths", "Triplets", "16ths" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "metroSound", 1 }, "Metronome Sound",
                                                               juce::StringArray { "Click", "Wood", "Beep" }, 0));
    layout.add (std::make_unique<F> (ID { "metroVol", 1 }, "Metronome Volume (dB)", R (-40.0f, 0.0f, 0.1f), -14.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "metroAccent", 1 }, "Metronome Accent First Beat", true));
    {
        const juce::StringArray notes { "Free (ms)", "1/2", "Dotted 1/4", "1/4", "1/4 triplet", "Dotted 1/8",
                                        "1/8", "1/8 triplet", "Dotted 1/16", "1/16", "1/16 triplet" };
        const juce::StringArray delayModes { "Digital", "Analog", "Tape", "Lo-Fi" };
        layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "preDelayMode", 1 },  "Pre-Amp Delay Mode", delayModes, 1));
        layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "postDelayMode", 1 }, "Post-Amp Delay Mode", delayModes, 1));
        layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "preDelayNote", 1 },  "Pre-Amp Delay Note", notes, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "postDelayNote", 1 }, "Post-Amp Delay Note", notes, 0));
        // Tempo sync for the rest of the time-based settings (Free = use the knob)
        auto notesFor = [&notes] (const juce::String& prefix, const juce::String& free)
        {
            juce::StringArray out;
            for (int i = 0; i < notes.size(); ++i)
                out.add (prefix + (i == 0 ? free : notes[i]));
            return out;
        };
        auto addNote = [&layout, &notesFor] (const char* id, const char* name, const char* prefix, const char* free)
        {
            layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, notesFor (prefix, free), 0));
        };
        addNote ("wahNote",      "Wah LFO Note",         "",        "Free (Hz)");
        addNote ("phaserNote",   "Phaser Note",          "",        "Free (Hz)");
        addNote ("wobNote",      "Wobble Note",          "",        "Free (Hz)");
        addNote ("swellNote",    "Swell Fade-In Note",   "Fade: ",  "Free (ms)");
        addNote ("granSizeNote", "Grain Size Note",      "Size: ",  "Free (ms)");
        addNote ("granDensNote", "Grain Rate Note",      "Rate: ",  "Free (/s)");
        addNote ("granPosNote",  "Grain Position Note",  "Pos: ",   "Free (ms)");
        addNote ("reverbPreNote","Reverb Pre-Delay Note","Pre-dly: ","Free (ms)");
    }

    // On/off switches for the effects that did not have one
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "noiseOn", 1 },     "Noise Pedal On", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "preDelayOn", 1 },  "Pre-Amp Delay On", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "postDelayOn", 1 }, "Post-Amp Delay On", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "reverbOn", 1 },    "Reverb On", true));

    // Phaser and chorus, both before the amp
    // Stereo tools: doubler and widener
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "dblOn", 1 }, "Doubler On", false));
    layout.add (std::make_unique<F> (ID { "dblTime", 1 },   "Doubler Time (ms)", R (5.0f, 60.0f, 0.1f), 22.0f));
    layout.add (std::make_unique<F> (ID { "dblVar", 1 },    "Doubler Variation", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "dblSpread", 1 }, "Doubler Spread",    R (0.0f, 1.0f, 0.001f), 0.9f));
    layout.add (std::make_unique<F> (ID { "dblMix", 1 },    "Doubler Mix",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "widOn", 1 }, "Widener On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "widMode", 1 }, "Widener Mode",
                juce::StringArray { "Wide: Classic", "Wide: Desktop", "Wide: Rack" }, 0));
    layout.add (std::make_unique<F> (ID { "widWow", 1 },    "Widener Wide",      R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "widSpace", 1 },  "Widener Space",     R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "widBass", 1 },   "Widener Bass Lift", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "widBassHz", 1 }, "Widener Bass Lift Freq (Hz)", R (50.0f, 200.0f, 1.0f), 100.0f));
    layout.add (std::make_unique<F> (ID { "widMono", 1 },   "Widener Mono Bass (Hz)", R (20.0f, 300.0f, 1.0f, 0.6f), 120.0f));
    layout.add (std::make_unique<F> (ID { "widMix", 1 },    "Widener Mix",       R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "widLevel", 1 },  "Widener Level (dB)", R (-12.0f, 6.0f, 0.1f), 0.0f));

    // Textures: tremolo, wobble, swell and granular cloud (after the Post EQ)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "tremOn", 1 }, "Tremolo On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "tremMode", 1 }, "Tremolo Mode",
                juce::StringArray { "Tremolo (both sides)", "Stereo (sides alternate)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "tremShape", 1 }, "Tremolo Shape",
                juce::StringArray { "Sine", "Triangle", "Square (soft)", "Optical (bulb and photocell)" }, 0));
    layout.add (std::make_unique<F> (ID { "tremRate", 1 },  "Tremolo Rate (Hz)", R (0.2f, 15.0f, 0.01f, 0.5f), 4.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "tremNote", 1 }, "Tremolo Note",
                juce::StringArray { "Free (Hz)", "1/2", "Dotted 1/4", "1/4", "1/4 triplet", "Dotted 1/8",
                                    "1/8", "1/8 triplet", "Dotted 1/16", "1/16", "1/16 triplet" }, 0));
    layout.add (std::make_unique<F> (ID { "tremDepth", 1 }, "Tremolo Depth",  R (0.0f, 1.0f, 0.001f), 0.5f));

    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "wobOn", 1 }, "Wobble On", false));
    layout.add (std::make_unique<F> (ID { "wobRate", 1 },  "Wobble Rate (Hz)", R (0.1f, 8.0f, 0.01f, 0.5f), 0.8f));
    layout.add (std::make_unique<F> (ID { "wobDepth", 1 }, "Wobble Depth",     R (0.0f, 1.0f, 0.001f), 0.4f));
    layout.add (std::make_unique<F> (ID { "wobDrift", 1 }, "Wobble Drift",     R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "wobMix", 1 },   "Wobble Mix",       R (0.0f, 1.0f, 0.001f), 1.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "swellOn", 1 }, "Swell On", false));
    layout.add (std::make_unique<F> (ID { "swellAttack", 1 }, "Swell Fade-In (ms)", R (50.0f, 4000.0f, 1.0f, 0.5f), 800.0f));
    layout.add (std::make_unique<F> (ID { "swellSens", 1 },   "Swell Sensitivity",  R (0.0f, 1.0f, 0.001f), 0.5f));

    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "granOn", 1 }, "Granular On", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "granFreeze", 1 }, "Granular Freeze", false));
    layout.add (std::make_unique<F> (ID { "granSize", 1 },      "Grain Size (ms)",    R (10.0f, 600.0f, 1.0f, 0.5f), 120.0f));
    layout.add (std::make_unique<F> (ID { "granDensity", 1 },   "Grain Density (/s)", R (1.0f, 80.0f, 0.1f, 0.6f), 16.0f));
    layout.add (std::make_unique<F> (ID { "granPos", 1 },       "Grain Position (ms back)", R (20.0f, 3000.0f, 1.0f, 0.5f), 400.0f));
    layout.add (std::make_unique<F> (ID { "granSpray", 1 },     "Grain Spray",        R (0.0f, 1.0f, 0.001f), 0.3f));
    layout.add (std::make_unique<F> (ID { "granPitch", 1 },     "Grain Pitch (semitones)", R (-24.0f, 24.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<F> (ID { "granPitchRand", 1 }, "Grain Pitch Random (semitones)", R (0.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "granReverse", 1 },   "Grain Reverse",      R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "granWidth", 1 },     "Grain Width",        R (0.0f, 1.0f, 0.001f), 0.8f));
    layout.add (std::make_unique<F> (ID { "granFeedback", 1 },  "Grain Feedback",     R (0.0f, 0.9f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "granMix", 1 },       "Grain Mix",          R (0.0f, 1.0f, 0.001f), 0.4f));

    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "wahOn", 1 }, "Wah On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "wahMode", 1 }, "Wah Mode",
                juce::StringArray { "Manual (Position)", "Auto: Envelope", "Auto: LFO" }, 0));
    layout.add (std::make_unique<F> (ID { "wahPos", 1 },   "Wah Position",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "wahHeel", 1 },  "Wah Heel (Hz)",      R (200.0f, 900.0f, 1.0f, 0.5f), 400.0f));
    layout.add (std::make_unique<F> (ID { "wahToe", 1 },   "Wah Toe (Hz)",       R (1200.0f, 4000.0f, 1.0f, 0.5f), 2200.0f));
    layout.add (std::make_unique<F> (ID { "wahQ", 1 },     "Wah Resonance (Q)",  R (1.0f, 12.0f, 0.01f, 0.6f), 5.0f));
    layout.add (std::make_unique<F> (ID { "wahMix", 1 },   "Wah Mix",            R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "wahSens", 1 },  "Wah Sensitivity",    R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "wahDecay", 1 }, "Wah Decay (ms)",     R (20.0f, 600.0f, 1.0f, 0.5f), 150.0f));
    layout.add (std::make_unique<F> (ID { "wahRate", 1 },  "Wah LFO Rate (Hz)",  R (0.1f, 10.0f, 0.01f, 0.5f), 1.0f));
    layout.add (std::make_unique<F> (ID { "wahDepth", 1 }, "Wah LFO Depth",      R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "phaserOn", 1 }, "Phaser On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "phaserStages", 1 }, "Phaser Stages",
                juce::StringArray { "2 stages", "4 stages (classic)", "6 stages", "8 stages", "10 stages", "12 stages" }, 1));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "phaserShape", 1 }, "Phaser LFO Shape",
                juce::StringArray { "Sine", "Triangle", "Random" }, 0));
    layout.add (std::make_unique<F> (ID { "phaserRate", 1 },     "Phaser Rate (Hz)", R (0.05f, 10.0f, 0.01f, 0.4f), 0.6f));
    layout.add (std::make_unique<F> (ID { "phaserDepth", 1 },    "Phaser Depth",     R (0.0f, 1.0f, 0.001f), 0.7f));
    layout.add (std::make_unique<F> (ID { "phaserCenter", 1 },   "Phaser Centre (Hz)", R (80.0f, 3000.0f, 1.0f, 0.4f), 600.0f));
    layout.add (std::make_unique<F> (ID { "phaserFeedback", 1 }, "Phaser Feedback",  R (-0.9f, 0.9f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "phaserMix", 1 },      "Phaser Mix",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "phaserEnv", 1 },      "Phaser Envelope",  R (0.0f, 1.0f, 0.001f), 0.0f));

    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "chorusOn", 1 }, "Chorus On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "chorusMode", 1 }, "Chorus Mode",
                juce::StringArray { "I", "II", "I + II", "Dimension", "Tri-chorus", "Ensemble", "Detune" }, 0));
    layout.add (std::make_unique<F> (ID { "chorusMix", 1 },   "Chorus Mix",   R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "chorusRate", 1 },  "Chorus Rate (x)", R (0.25f, 4.0f, 0.01f, 0.5f), 1.0f));
    layout.add (std::make_unique<F> (ID { "chorusDepth", 1 }, "Chorus Depth", R (0.0f, 1.5f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "chorusTone", 1 },  "Chorus Tone (Hz)", R (2000.0f, 14000.0f, 10.0f, 0.5f), 8000.0f));
    layout.add (std::make_unique<F> (ID { "chorusHiss", 1 },  "Chorus Hiss",  R (0.0f, 1.0f, 0.001f), 0.0f));

    layout.add (std::make_unique<F> (ID { "reverbPreDelay", 1 }, "Reverb Pre-Delay (ms)", R (0.0f, 250.0f, 0.1f, 0.5f), 15.0f));
    layout.add (std::make_unique<F> (ID { "reverbDamp", 1 },     "Reverb Damping",   R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "reverbWidth", 1 },    "Reverb Width",     R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "reverbLowCut", 1 },   "Reverb Low Cut (Hz)",  R (20.0f, 1000.0f, 1.0f, 0.4f), 100.0f));
    layout.add (std::make_unique<F> (ID { "reverbHighCut", 1 },  "Reverb High Cut (Hz)", R (1000.0f, 20000.0f, 10.0f, 0.4f), 12000.0f));
    layout.add (std::make_unique<F> (ID { "reverbDuck", 1 },     "Reverb Ducking",   R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "reverbFreeze", 1 }, "Reverb Freeze", false));
    layout.add (std::make_unique<F> (ID { "shimmerTone", 1 },    "Shimmer Tone (Hz)", R (1500.0f, 12000.0f, 10.0f, 0.4f), 4500.0f));

    // Post EQ (9-band graphic EQ, after the cab)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "eqOn", 1 }, "Post EQ On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "eqMode", 1 }, "Post EQ Mode",
                juce::StringArray { "Graphic", "Mod (custom Q)" }, 0));
    layout.add (std::make_unique<F> (ID { "eqQ", 1 },     "Post EQ Q (Mod mode)", R (0.4f, 4.0f, 0.01f), 1.4f));
    layout.add (std::make_unique<F> (ID { "eqLowCut", 1 },  "Post EQ Low Cut (Hz)",  R (20.0f, 500.0f, 1.0f, 0.4f), 20.0f));
    layout.add (std::make_unique<F> (ID { "eqHighCut", 1 }, "Post EQ High Cut (Hz)", R (2000.0f, 20000.0f, 10.0f, 0.4f), 20000.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "eqSlope", 1 }, "Post EQ Cut Slope",
                juce::StringArray { "12 dB/oct", "24 dB/oct" }, 1));
    layout.add (std::make_unique<F> (ID { "eqLevel", 1 }, "Post EQ Level (dB)",   R (-12.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "eqAuto", 1 }, "Post EQ Auto Level", false));
    for (int b = 0; b < kEqBands; ++b)
    {
        const juce::String hz = kEqBandHz[b] >= 1000.0f ? juce::String (kEqBandHz[b] / 1000.0f, kEqBandHz[b] == 1000.0f || kEqBandHz[b] == 2000.0f || kEqBandHz[b] == 4000.0f || kEqBandHz[b] == 8000.0f || kEqBandHz[b] == 16000.0f ? 0 : 1) + " kHz"
                                                         : juce::String ((int) kEqBandHz[b]) + " Hz";
        layout.add (std::make_unique<F> (ID { "eqB" + juce::String (b + 1), 1 }, "Post EQ " + hz + " (dB)",
                                         R (-12.0f, 12.0f, 0.1f), 0.0f));
    }

    // NAM model player (v50): replaces Amp A's preamp and power stage with a neural capture
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "namOn", 1 }, "NAM Model On", false));
    layout.add (std::make_unique<F> (ID { "namIn", 1 },  "NAM Input (dB)",  R (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<F> (ID { "namOut", 1 }, "NAM Output (dB)", R (-24.0f, 24.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "namNorm", 1 }, "NAM Match Loudness", true));

    // Lo-Fi Warble pedal (v50)
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "lfOn", 1 }, "Lo-Fi On", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "lfShape", 1 }, "Lo-Fi Warble Shape",
                juce::StringArray { "Sine", "Triangle", "Random" }, 0));
    layout.add (std::make_unique<F> (ID { "lfRate", 1 },     "Lo-Fi Rate (Hz)",   R (0.1f, 10.0f, 0.01f, 0.4f), 1.6f));
    layout.add (std::make_unique<F> (ID { "lfDepth", 1 },    "Lo-Fi Depth",       R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<F> (ID { "lfDrift", 1 },    "Lo-Fi Drift",       R (0.0f, 1.0f, 0.001f), 0.35f));
    layout.add (std::make_unique<F> (ID { "lfTone", 1 },     "Lo-Fi Tone",        R (0.0f, 1.0f, 0.001f), 0.45f));
    layout.add (std::make_unique<F> (ID { "lfDirt", 1 },     "Lo-Fi Dirt",        R (0.0f, 1.0f, 0.001f), 0.25f));
    layout.add (std::make_unique<F> (ID { "lfNoise", 1 },    "Lo-Fi Noise",       R (0.0f, 1.0f, 0.001f), 0.2f));
    layout.add (std::make_unique<F> (ID { "lfDropouts", 1 }, "Lo-Fi Dropouts",    R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (ID { "lfMix", 1 },      "Lo-Fi Mix",         R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<F> (ID { "lfLevel", 1 },    "Lo-Fi Level (dB)",  R (-18.0f, 12.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<F> (ID { "outGain", 1 },  "Output (dB)",  R (-36.0f, 12.0f, 0.1f), -12.0f));

    // ======== v51 ========
    {
        using Bool = juce::AudioParameterBool;
        using Choice = juce::AudioParameterChoice;
        const juce::StringArray lfoNotes { "Free (Hz)", "1/2", "Dotted 1/4", "1/4", "1/4 triplet", "Dotted 1/8",
                                           "1/8", "1/8 triplet", "Dotted 1/16", "1/16", "1/16 triplet" };

        // Boost: tighten, a four-band parametric EQ and saturation modes
        layout.add (std::make_unique<Bool> (ID { "bsOn", 1 }, "Boost On", false));
        layout.add (std::make_unique<Choice> (ID { "bsMode", 1 }, "Boost Saturation Mode", juce::StringArray { "Clean", "EP", "Console", "Tube", "Treble", "Mid", "FET" }, 0));
        layout.add (std::make_unique<F> (ID { "bsGain", 1 },  "Boost Gain (dB)",   R (0.0f, 30.0f, 0.1f), 8.0f));
        layout.add (std::make_unique<F> (ID { "bsSat", 1 },   "Boost Saturation",  R (0.0f, 1.0f, 0.001f), 0.35f));
        layout.add (std::make_unique<F> (ID { "bsTight", 1 }, "Boost Tighten",     R (0.0f, 1.0f, 0.001f), 0.0f));
        layout.add (std::make_unique<Choice> (ID { "bsTightStyle", 1 }, "Boost Tighten Style", juce::StringArray { "Mid hump", "Low cut", "Focus" }, 0));
        layout.add (std::make_unique<F> (ID { "bsLevel", 1 }, "Boost Level (dB)",  R (-12.0f, 12.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<F> (ID { "bsMix", 1 },   "Boost Mix",         R (0.0f, 1.0f, 0.001f), 1.0f));   // v94
        layout.add (std::make_unique<Bool> (ID { "bsEqOn", 1 }, "Boost EQ On", false));
        layout.add (std::make_unique<Choice> (ID { "bsEqPos", 1 }, "Boost EQ Position", juce::StringArray { "Before saturation", "After saturation" }, 1));
        layout.add (std::make_unique<F> (ID { "bsLoHz", 1 },  "Boost EQ Low Shelf (Hz)", R (30.0f, 400.0f, 1.0f, 0.5f), 100.0f));
        layout.add (std::make_unique<F> (ID { "bsLoDb", 1 },  "Boost EQ Low Shelf (dB)", R (-15.0f, 15.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<F> (ID { "bsM1Hz", 1 }, "Boost EQ Band 1 (Hz)", R (100.0f, 2000.0f, 1.0f, 0.5f), 400.0f));
        layout.add (std::make_unique<F> (ID { "bsM1Db", 1 }, "Boost EQ Band 1 (dB)", R (-15.0f, 15.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<F> (ID { "bsM1Q", 1 },  "Boost EQ Band 1 Q",    R (0.3f, 6.0f, 0.01f, 0.5f), 1.0f));
        layout.add (std::make_unique<F> (ID { "bsM2Hz", 1 }, "Boost EQ Band 2 (Hz)", R (500.0f, 8000.0f, 1.0f, 0.5f), 2500.0f));
        layout.add (std::make_unique<F> (ID { "bsM2Db", 1 }, "Boost EQ Band 2 (dB)", R (-15.0f, 15.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<F> (ID { "bsM2Q", 1 },  "Boost EQ Band 2 Q",    R (0.3f, 6.0f, 0.01f, 0.5f), 1.0f));
        layout.add (std::make_unique<F> (ID { "bsHiHz", 1 }, "Boost EQ High Shelf (Hz)", R (2000.0f, 16000.0f, 1.0f, 0.5f), 6000.0f));
        layout.add (std::make_unique<F> (ID { "bsHiDb", 1 }, "Boost EQ High Shelf (dB)", R (-15.0f, 15.0f, 0.1f), 0.0f));

        // Two modulation slots (flanger, through-zero flanger, vibrato, Uni-V, rotary, auto-pan, harmonic tremolo)
        for (int s = 0; s < 2; ++s)
        {
            const juce::String p = s == 0 ? "mfA" : "mfB", nm = s == 0 ? "Mod A " : "Mod B ";
            layout.add (std::make_unique<Bool> (ID { p + "On", 1 }, nm + "On", false));
            layout.add (std::make_unique<Choice> (ID { p + "Type", 1 }, nm + "Type",
                        juce::StringArray { "Flanger", "Through-zero flanger", "Vibrato", "Uni-V", "Rotary", "Auto-pan", "Harmonic tremolo" }, s == 0 ? 0 : 2));
            layout.add (std::make_unique<Choice> (ID { p + "Pos", 1 }, nm + "Position", juce::StringArray { "Before the amp", "After the cab" }, 0));
            layout.add (std::make_unique<Choice> (ID { p + "Shape", 1 }, nm + "LFO Shape", juce::StringArray { "Sine", "Triangle", "Soft square" }, 0));
            layout.add (std::make_unique<F> (ID { p + "Rate", 1 }, nm + "Rate (Hz)", R (0.05f, 12.0f, 0.01f, 0.4f), s == 0 ? 0.35f : 5.0f));
            layout.add (std::make_unique<Choice> (ID { p + "Note", 1 }, nm + "Rate Note", lfoNotes, 0));
            layout.add (std::make_unique<F> (ID { p + "Depth", 1 }, nm + "Depth", R (0.0f, 1.0f, 0.001f), 0.7f));
            layout.add (std::make_unique<F> (ID { p + "Feedback", 1 }, nm + "Feedback", R (-0.9f, 0.9f, 0.001f), s == 0 ? 0.5f : 0.0f));
            layout.add (std::make_unique<F> (ID { p + "Manual", 1 }, nm + "Manual", R (0.0f, 1.0f, 0.001f), 0.5f));
            layout.add (std::make_unique<F> (ID { p + "Tone", 1 }, nm + "Tone (Hz)", R (1500.0f, 16000.0f, 1.0f, 0.4f), 9000.0f));
            layout.add (std::make_unique<F> (ID { p + "Mix", 1 }, nm + "Mix", R (0.0f, 1.0f, 0.001f), s == 0 ? 0.5f : 1.0f));
        }

        // Post delay: reverse
        layout.add (std::make_unique<Choice> (ID { "postDelayStyle", 1 }, "Post-Amp Delay Style", juce::StringArray { "Forward", "Reverse" }, 0));

        // Reverb: more ways to shape the space
        layout.add (std::make_unique<F> (ID { "reverbDiffuse", 1 }, "Reverb Diffuse", R (0.0f, 1.0f, 0.001f), 0.5f));
        layout.add (std::make_unique<F> (ID { "reverbSpace", 1 },   "Reverb Space (x)", R (0.4f, 2.5f, 0.001f, 0.55f), 1.0f));
        layout.add (std::make_unique<F> (ID { "reverbMod", 1 },     "Reverb Mod", R (0.0f, 1.0f, 0.001f), 0.0f));
        layout.add (std::make_unique<F> (ID { "reverbDrag", 1 },    "Reverb Drag", R (0.0f, 1.0f, 0.001f), 0.5f));
        layout.add (std::make_unique<Choice> (ID { "reverbDragMode", 1 }, "Reverb Drag Steps", juce::StringArray { "Smooth", "Semitones", "Fifths and octaves", "Octaves" }, 0));
        layout.add (std::make_unique<F> (ID { "reverbBloom", 1 },   "Reverb Bloom", R (0.0f, 1.0f, 0.001f), 0.0f));
        layout.add (std::make_unique<F> (ID { "reverbSymph", 1 },   "Reverb Symphony", R (0.0f, 1.0f, 0.001f), 0.0f));
        layout.add (std::make_unique<Choice> (ID { "reverbSymphPitch", 1 }, "Reverb Symphony Pitch", juce::StringArray { "Octave up", "Octave down", "Fifth up", "Two octaves up" }, 0));

        // Granular: scales, grain shapes, scanning, tone
        layout.add (std::make_unique<Choice> (ID { "granScale", 1 }, "Grain Scale",
                    juce::StringArray { "Free", "Octaves", "Fifths", "Major pentatonic", "Minor pentatonic", "Major", "Minor", "Whole tone" }, 0));
        layout.add (std::make_unique<Choice> (ID { "granShape", 1 }, "Grain Shape", juce::StringArray { "Smooth", "Swell", "Pluck", "Flat" }, 0));
        layout.add (std::make_unique<F> (ID { "granScan", 1 }, "Grain Scan", R (0.0f, 1.0f, 0.001f), 0.0f));
        layout.add (std::make_unique<F> (ID { "granTone", 1 }, "Grain Tone (Hz)", R (500.0f, 20000.0f, 1.0f, 0.4f), 20000.0f));

        // Pitch effects: which guitar
        layout.add (std::make_unique<Choice> (ID { "tpRange", 1 }, "Pitch Effects String Range", juce::StringArray { "6-string", "7-string", "8-string" }, 0));

        // NAM slots: Amp B and two pedal slots
        layout.add (std::make_unique<Bool> (ID { "namBOn", 1 }, "NAM Amp B On", false));
        layout.add (std::make_unique<F> (ID { "namBIn", 1 },  "NAM Amp B Input (dB)",  R (-24.0f, 24.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<F> (ID { "namBOut", 1 }, "NAM Amp B Output (dB)", R (-24.0f, 24.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<Bool> (ID { "namBNorm", 1 }, "NAM Amp B Match Loudness", true));
        for (int s = 0; s < 2; ++s)
        {
            const juce::String p = s == 0 ? "nmP1" : "nmP2", nm = s == 0 ? "NAM Pedal 1 " : "NAM Pedal 2 ";
            layout.add (std::make_unique<Bool> (ID { p + "On", 1 }, nm + "On", false));
            layout.add (std::make_unique<F> (ID { p + "In", 1 },  nm + "Input (dB)",  R (-24.0f, 24.0f, 0.1f), 0.0f));
            layout.add (std::make_unique<F> (ID { p + "Out", 1 }, nm + "Output (dB)", R (-24.0f, 24.0f, 0.1f), 0.0f));
            layout.add (std::make_unique<Bool> (ID { p + "Norm", 1 }, nm + "Match Loudness", true));
            layout.add (std::make_unique<F> (ID { p + "Mix", 1 }, nm + "Mix", R (0.0f, 1.0f, 0.001f), 1.0f));
        }
        layout.add (std::make_unique<Bool> (ID { "nmP2Stereo", 1 }, "NAM Pedal 2 Stereo (one model per side)", false));

        // Riff recorder (v52)
        layout.add (std::make_unique<Choice> (ID { "riffMode", 1 }, "Riff Recorder Mode", juce::StringArray { "DI (before the chain)", "Full tone (after the chain)" }, 0));
        layout.add (std::make_unique<Bool> (ID { "riffLoop", 1 }, "Riff Loop", true));
        layout.add (std::make_unique<F> (ID { "riffLevel", 1 }, "Riff Playback Level", R (0.0f, 1.5f, 0.001f), 1.0f));
        layout.add (std::make_unique<Bool> (ID { "riffMute", 1 }, "Riff Mute Live Input While Playing", true));

        // Harmonizer (v52)
        layout.add (std::make_unique<Bool> (ID { "hmOn", 1 }, "Harmonizer On", false));
        layout.add (std::make_unique<Choice> (ID { "hmPos", 1 }, "Harmonizer Position", juce::StringArray { "Before the amp", "After the cab" }, 0));
        layout.add (std::make_unique<Choice> (ID { "hmMode", 1 }, "Harmonizer Mode", juce::StringArray { "Interval", "Scale" }, 1));
        layout.add (std::make_unique<Choice> (ID { "hmKey", 1 }, "Harmonizer Key", juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 4));
        layout.add (std::make_unique<Choice> (ID { "hmScale", 1 }, "Harmonizer Scale",
                    juce::StringArray { "Major", "Natural minor", "Dorian", "Mixolydian", "Harmonic minor", "Major pentatonic", "Minor pentatonic" }, 0));
        layout.add (std::make_unique<F> (ID { "hmAInt", 1 }, "Harmony A Interval (semitones)", R (-24.0f, 24.0f, 1.0f), 4.0f));
        layout.add (std::make_unique<F> (ID { "hmBInt", 1 }, "Harmony B Interval (semitones)", R (-24.0f, 24.0f, 1.0f), 7.0f));
        layout.add (std::make_unique<F> (ID { "hmADeg", 1 }, "Harmony A Scale Step", R (-7.0f, 7.0f, 1.0f), 2.0f));
        layout.add (std::make_unique<F> (ID { "hmBDeg", 1 }, "Harmony B Scale Step", R (-7.0f, 7.0f, 1.0f), 4.0f));
        layout.add (std::make_unique<F> (ID { "hmALvl", 1 }, "Harmony A Level", R (0.0f, 1.0f, 0.001f), 0.6f));
        layout.add (std::make_unique<F> (ID { "hmBLvl", 1 }, "Harmony B Level", R (0.0f, 1.0f, 0.001f), 0.0f));
        layout.add (std::make_unique<F> (ID { "hmAPan", 1 }, "Harmony A Pan", R (-1.0f, 1.0f, 0.001f), -0.5f));
        layout.add (std::make_unique<F> (ID { "hmBPan", 1 }, "Harmony B Pan", R (-1.0f, 1.0f, 0.001f), 0.5f));
        layout.add (std::make_unique<F> (ID { "hmDry", 1 }, "Harmony Dry Level", R (0.0f, 1.0f, 0.001f), 1.0f));
        layout.add (std::make_unique<F> (ID { "hmTone", 1 }, "Harmony Tone", R (0.0f, 1.0f, 0.001f), 0.7f));
        layout.add (std::make_unique<F> (ID { "hmGlide", 1 }, "Harmony Glide (ms)", R (1.0f, 400.0f, 1.0f, 0.5f), 25.0f));
        layout.add (std::make_unique<Choice> (ID { "hmTrack", 1 }, "Harmonizer Tracking", juce::StringArray { "Single note", "Chords", "Chords (fast)" }, 0));

        // Delays (v52): ducking, modulation, diffusion, hold / freeze / infinite, multi-tap
        for (int d = 0; d < 2; ++d)
        {
            const juce::String p = d == 0 ? "preDelay" : "postDelay", nm = d == 0 ? "Pre-Amp Delay " : "Post-Amp Delay ";
            layout.add (std::make_unique<F> (ID { p + "Duck", 1 },    nm + "Duck",          R (0.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { p + "DuckRel", 1 }, nm + "Duck Release (ms)", R (20.0f, 2000.0f, 1.0f, 0.5f), 250.0f));
            layout.add (std::make_unique<F> (ID { p + "ModDepth", 1 }, nm + "Mod Depth",    R (0.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<F> (ID { p + "ModRate", 1 }, nm + "Mod Rate (Hz)", R (0.05f, 10.0f, 0.01f, 0.5f), 1.0f));
            layout.add (std::make_unique<F> (ID { p + "Diffuse", 1 }, nm + "Diffusion",     R (0.0f, 1.0f, 0.001f), 0.0f));
            layout.add (std::make_unique<Bool> (ID { p + "Hold", 1 }, nm + "Hold", false));
            layout.add (std::make_unique<Choice> (ID { p + "HoldMode", 1 }, nm + "Hold Mode", juce::StringArray { "Freeze", "Infinite" }, 0));
            layout.add (std::make_unique<Choice> (ID { p + "Taps", 1 }, nm + "Multi-Tap",
                        juce::StringArray { "Off", "Half + full", "Thirds", "Quarters", "Off-beat", "Dotted cluster" }, 0));
            layout.add (std::make_unique<F> (ID { p + "TapLevel", 1 }, nm + "Tap Level",    R (0.0f, 1.0f, 0.001f), 0.6f));
        }
    }

    // v79: circuit-model amps (appended; Off keeps every older session exactly as it was)
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampCircuit", 1 }, "Circuit Amp (full tube-circuit model)",
                juce::StringArray { "Circuit Amp: Off", "Circuit Amp: Lead 50 (full circuit)" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampStageModel", 1 }, "Amp A Stage Model",
                juce::StringArray { "Stage model: classic", "Stage model: circuit-derived" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampBStageModel", 1 }, "Amp B Stage Model",
                juce::StringArray { "Stage model: classic", "Stage model: circuit-derived" }, 0));
    layout.add (std::make_unique<F> (ID { "ampMaster", 1 }, "Circuit Amp Master Volume", R (0.0f, 1.0f, 0.001f), 0.5f));

    // v83: spring reverb driver (an effect, not part of the amp models), tremolo shape "Optical" is appended to tremShape
    layout.add (std::make_unique<F> (ID { "reverbDrive", 1 }, "Spring Reverb Drive", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampDerived", 1 }, "Amp A Circuit-Derived Level",
                juce::StringArray { "Derived: off (classic)", "Derived: power section", "Derived: power + supply", "Derived: power + supply + voicing" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampDual", 1 }, "Amp A Dual Voice",
                juce::StringArray { "Dual voice: off", "Dual voice: on (two first stages in parallel)" }, 0));
    layout.add (std::make_unique<F> (ID { "ampDualBal", 1 }, "Amp A Voice Mix", R (0.0f, 1.0f, 0.001f), 0.5f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampBDerived", 1 }, "Amp B Circuit-Derived Level",
                juce::StringArray { "Derived: off (classic)", "Derived: power section", "Derived: power + supply", "Derived: power + supply + voicing" }, 0));
    // v94: Tight, Screamer, Distortion and Transparent are hand-tuned again by default; this switch turns on their circuit models.
    layout.add (std::make_unique<juce::AudioParameterBool> (ID { "odCircuit", 1 }, "OD Circuit Mode (Transparent, Tight, Screamer, Distortion)", false));
    // v94: the amp Mode menus are back. Classic is the original voicing (the default); Circuit-derived uses the stage shapes, power
    // section, supply and component-value voicing worked out from the circuits. Same ids as v87 to v92 so old sessions keep their choice.
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampMode", 1 }, "Amp A Mode",
                juce::StringArray { "Classic", "Circuit-derived" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "ampBMode", 1 }, "Amp B Mode",
                juce::StringArray { "Classic", "Circuit-derived" }, 0));
    // v91: wah sweep range. "Generic" uses the Heel and Toe knobs; "Classic circuit" fixes the sweep at 450 Hz to 1.6 kHz (the published figures).
    layout.add (std::make_unique<juce::AudioParameterChoice> (ID { "wahVoice", 1 }, "Wah Voice",
                juce::StringArray { "Generic (Heel and Toe knobs)", "Classic circuit (450 Hz to 1.6 kHz)" }, 0));
    return layout;
}

//==============================================================================
// Parallel Amp B (v40). A mono copy of the amp's gain stages and tone stack with its own settings,
// run on the same signal as Amp A (after the overdrive pedal's linear half). It has its own
// oversamplers of the same kind as Amp A's, so both amps report and have the same latency.
// Hand-tuned like the rest; Amp A and Amp B use the same voicing tables.
//==============================================================================
struct StarterAmpProcessor::ParallelAmp
{
    using Filt = juce::dsp::IIR::Filter<float>;
    using Co   = juce::dsp::IIR::Coefficients<float>;

    juce::dsp::Oversampling<float> os4 { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::Oversampling<float> os2 { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    Filt voicePeak, hp1, hp2, hp3, lp2, lp3, presence, odTone, brightF;   // run at the oversampled rate
    bool lpOn = false;
    Filt dc, bass, lowMid, highMid, treble, resoF;                // run at the normal rate
    bool  brightOn = false, resoOn = false;
    float powerDrive = 0.0f, deadZone = 0.0f;
    int   mode = 0;                                         // 0 = 4x, 1 = 2x, 2 = off
    int   stages = 3;
    float bias[3] = { 0, 0, 0 }, sag = 0.0f, sagEnv = 0.0f, osRate = 44100.0f * 4.0f;
    PowerSection::Params psp;  PowerSection::Chain psChain;  PowerSection::Supply psSup;  float psFbLast = 0.0f;     // v83
    float tg[3] = { 1, 1, 1 }, lg[3] = { 1, 1, 1 };         // stage gains: target and last block's
    float tTrim = 1.0f, lTrim = 1.0f, tOdVol = 1.0f, lOdVol = 1.0f;
    float tLevel = 1.0f, lLevel = 1.0f;                     // the Amp B level alone (used with a neural capture)

    void prepare (int blockSize, int newMode)
    {
        mode = newMode;
        os4.initProcessing ((size_t) blockSize);
        os2.initProcessing ((size_t) blockSize);
        resetState();
    }

    void resetState()
    {
        os4.reset();
        os2.reset();
        for (auto* f : { &voicePeak, &hp1, &hp2, &hp3, &lp2, &lp3, &presence, &odTone, &brightF, &dc, &bass, &lowMid, &highMid, &treble, &resoF })
            f->reset();
        sagEnv = 0.0f;
        psChain.reset();  psSup.reset();  psFbLast = 0.0f;
        for (int i = 0; i < 3; ++i) lg[i] = tg[i];
        lTrim = tTrim;
        lLevel = tLevel;
        lOdVol = tOdVol;
    }

    int latency() const
    {
        return mode == 0 ? (int) std::lround (os4.getLatencyInSamples())
             : mode == 1 ? (int) std::lround (os2.getLatencyInSamples()) : 0;
    }

    // Called whenever any parameter moved (from updateFilters).
    void update (double sr, double osSr, int newMode, const AmpVoicing& v, float knobGain, float presenceAmt,
                 float bassDb, float lowMidDb, float highMidDb, float trebleDb, float levelDb, float odVolume,
                 const Co::Ptr& sharedOdTone, bool bright, float brightDb, float power, float dead, float resoAmt)
    {
        brightOn = bright;  powerDrive = power;  deadZone = dead;  resoOn = resoAmt > 0.01f;
        {
            float bHz = 2800.0f, bDb = brightDb;
            derivedBright (v, knobGain, bHz, bDb);
            brightF.coefficients = Co::makeHighShelf (osSr, bHz, 0.707f, dbToGain (bDb));
        }
        resoF.coefficients   = Co::makePeakFilter (sr, 105.0f, 1.1f, dbToGain (6.0f * resoAmt));
        mode = newMode;
        osRate = (float) osSr;
        stages = v.stages;
        for (int i = 0; i < 3; ++i) { bias[i] = v.bias[i]; tg[i] = stageGain (v, i, knobGain); }
        sag = v.sag;
        tTrim = dbToGain (v.trimDb + levelDb);
        tLevel = dbToGain (levelDb);
        tOdVol = odVolume;
        odTone.coefficients = sharedOdTone;                 // follows Amp A's OD tone

        voicePeak.coefficients = Co::makePeakFilter (osSr, v.voiceHz, v.voiceQ, dbToGain (v.voiceDb));
        hp1.coefficients = Co::makeHighPass (osSr, v.hp[0]);
        hp2.coefficients = Co::makeHighPass (osSr, v.hp[1]);
        hp3.coefficients = Co::makeHighPass (osSr, v.hp[2]);
        lpOn = v.stageLpHz > 0.0f;
        lp2.coefficients = Co::makeFirstOrderLowPass (osSr, juce::jmin (lpOn ? (double) v.stageLpHz : 8000.0, 0.45 * osSr));
        lp3.coefficients = lp2.coefficients;
        presence.coefficients = Co::makeHighShelf (osSr, v.presenceHz, 0.707f, dbToGain (presenceAmt * v.presenceMaxDb));
        psp = v.ps;
        if (psp.filtersOn)
        {
            float g[4];
            PowerSection::gainsFor (psp.family, psp.feel, presenceAmt, g);
            psChain.design (osSr, g);
        }
        dc.coefficients = Co::makeHighPass (sr, 25.0f);
        bass.coefficients    = Co::makeLowShelf   (sr, v.bassHz, 0.707f, dbToGain (bassDb + v.bassOffsetDb));
        lowMid.coefficients  = Co::makePeakFilter (sr, v.lowMidHz,  v.lowMidQ,  dbToGain (lowMidDb  + v.lowMidOffsetDb));
        highMid.coefficients = Co::makePeakFilter (sr, v.highMidHz, v.highMidQ, dbToGain (highMidDb + v.highMidOffsetDb));
        treble.coefficients  = Co::makeHighShelf  (sr, v.trebleHz, 0.707f, dbToGain (trebleDb + v.trebleOffsetDb));
    }

    // Amp B with a neural capture (v51): the overdrive pedal's clipping and tone first (at the oversampled rate, as for
    // the built-in preamp), then the capture, then Amp B's own tone stack and level.
    PassiveStack passive;                    // v81
    int stackSel = 0;
    float posT = 0.5f, posB = 0.5f, posM = 0.5f;
    double stackRate = 48000.0;
    StageShaper::Plan plan;                  // v80: circuit-derived stage shapes (inactive = the classic ones)
    StageShaper::StageState stageSt[3];
    float biasKnob = 0.0f;
    void processNam (float* x, int n, bool useOd, int odStyle, NamSlot& s, float inG, float outG)
    {
        float* chans[1] = { x };
        juce::dsp::AudioBlock<float> in (chans, (size_t) 1, (size_t) n);
        bool viaOs = false;
        if (useOd)
        {
            auto& os = (mode == 0) ? os4 : os2;
            auto ob = (mode == 2) ? in : os.processSamplesUp (in);
            const int m = (int) ob.getNumSamples();
            float* y = ob.getChannelPointer (0);
            juce::dsp::ProcessContextReplacing<float> ctx (ob);
            const auto clip = odClipFor (odStyle);
            for (int i = 0; i < m; ++i) y[i] = clip (y[i]);
            odTone.process (ctx);
            const float step = (tOdVol - lOdVol) / (float) m;
            float g = lOdVol;
            for (int i = 0; i < m; ++i) { y[i] *= g; g += step; }
            if (mode != 2) { os.processSamplesDown (in); viaOs = true; }
        }
        lOdVol = tOdVol;

        {
            const float si = (inG - s.lastIn) / (float) n;
            float g = s.lastIn;
            for (int i = 0; i < n; ++i) { x[i] *= g; g += si; }
            s.lastIn = inG;
        }
        s.cur[0]->process (x, n);
        {
            const float so = (outG - s.lastOut) / (float) n;
            float g = s.lastOut;
            for (int i = 0; i < n; ++i)
            {
                const float v = std::isfinite (x[i]) ? juce::jlimit (-16.0f, 16.0f, x[i]) : 0.0f;
                x[i] = v * g;
                g += so;
            }
            s.lastOut = outG;
        }
        if (! viaOs)
            s.delayInPlace (x, n, latency());       // the built-in amp's oversampler delay, so switching keeps the timing

        juce::dsp::ProcessContextReplacing<float> base (in);
        dc.process (base);
        bass.process (base);
        lowMid.process (base);
        highMid.process (base);
        treble.process (base);

        const float step = (tLevel - lLevel) / (float) n;
        float g = lLevel;
        for (int i = 0; i < n; ++i) { x[i] *= g; g += step; }
        lLevel = tLevel;
        lTrim = tTrim;
    }

    // x: mono signal, processed in place.
    void process (float* x, int n, bool useOd, int odStyle)
    {
        float* chans[1] = { x };
        juce::dsp::AudioBlock<float> in (chans, (size_t) 1, (size_t) n);
        auto& os = (mode == 0) ? os4 : os2;
        auto ob = (mode == 2) ? in : os.processSamplesUp (in);
        const int m = (int) ob.getNumSamples();
        float* y = ob.getChannelPointer (0);
        juce::dsp::ProcessContextReplacing<float> ctx (ob);

        auto shape = [&] (float from, float to, auto fn)
        {
            const float step = (to - from) / (float) m;
            float g = from;
            for (int i = 0; i < m; ++i) { y[i] = fn (g, y[i]); g += step; }
        };

        if (useOd)
        {
            const auto clip = odClipFor (odStyle);
            shape (1.0f, 1.0f, [clip] (float, float s) { return clip (s); });
            odTone.process (ctx);
            shape (lOdVol, tOdVol, [] (float g, float s) { return g * s; });
        }
        lOdVol = tOdVol;

        if (brightOn)
            brightF.process (ctx);
        voicePeak.process (ctx);
        auto stage = [&] (Filt& hp, float from, float to, float b)
        {
            hp.process (ctx);
            const float offset = fastTanh (b);
            shape (from, to, [b, offset] (float g, float s) { return fastTanh (g * s + b) - offset; });
        };
        const float blockAttack  = 1.0f - std::exp (-1.0f / (0.0015f * osRate));
        const float blockRelease = 1.0f - std::exp (-1.0f / (0.045f  * osRate));
        auto stageM = [&] (Filt& hp, float from, float to, int idx)
        {
            hp.process (ctx);
            const float step = (to - from) / (float) m;
            float g = from;
            for (int i = 0; i < m; ++i) { y[i] = stageSt[idx].process (plan.pre[idx], g * y[i] + 0.3f * biasKnob, blockAttack, blockRelease); g += step; }
        };
        if (plan.active)
        {
            stageM (hp1, lg[0], tg[0], 0);
            if (lpOn) lp2.process (ctx);
            stageM (hp2, lg[1], tg[1], 1);
            if (stages >= 3)
            {
                if (lpOn) lp3.process (ctx);
                stageM (hp3, lg[2], tg[2], 2);
            }
        }
        else
        {
        stage (hp1, lg[0], tg[0], bias[0]);
        if (lpOn) lp2.process (ctx);
        stage (hp2, lg[1], tg[1], bias[1]);
        if (stages >= 3)
        {
            if (lpOn) lp3.process (ctx);
            stage (hp3, lg[2], tg[2], bias[2]);
        }
        }

        if (sag > 0.0f)
        {
            const float release = std::exp (-1.0f / (0.12f * osRate));
            float env = sagEnv;
            for (int i = 0; i < m; ++i)
            {
                const float a = std::abs (y[i]);
                env = (a > env) ? a : a + release * (env - a);
                y[i] *= 1.0f / (1.0f + sag * env);
            }
            sagEnv = env;
        }
        if (psp.supplyOn) psSup.droop (y, m, 0, psp, osRate);
        if (powerDrive > 0.0f || deadZone > 0.0f)
        {
            if (psp.fbLg > 0.0f)
            {
                const float comp = std::sqrt (1.0f + 5.0f * powerDrive * powerDrive), hotAmt = juce::jmax (0.0f, biasKnob);
                if (plan.active)
                    PowerSection::feedbackStage (y, m, psp.fbLg, comp, psFbLast,
                                                 [&] (float s) { return StageShaper::powerSample (s, powerDrive, deadZone, plan.pwrMed, plan.pwrHot, hotAmt); });
                else
                    PowerSection::feedbackStage (y, m, psp.fbLg, comp, psFbLast, [&] (float s) { return powerSample (s, powerDrive, deadZone); });
            }
            else if (plan.active) StageShaper::powerStage (y, m, powerDrive, deadZone, plan.pwrMed, plan.pwrHot, juce::jmax (0.0f, biasKnob));
            else powerStage (y, m, powerDrive, deadZone);
        }
        if (psp.supplyOn) psSup.transformer (y, m, 0, psp, osRate);
        if (psp.filtersOn) psChain.process (y, m, 0);
        else presence.process (ctx);
        for (int i = 0; i < 3; ++i) lg[i] = tg[i];

        if (mode != 2)
            os.processSamplesDown (in);

        juce::dsp::ProcessContextReplacing<float> base (in);
        dc.process (base);
        if (resoOn)
            resoF.process (base);
        if (stackSel >= 2)
        {
            passive.set (stackSel - 2, posT, posB, posM, stackRate);
            passive.process (x, n);
        }
        else
        {
            bass.process (base);
            lowMid.process (base);
            highMid.process (base);
            treble.process (base);
        }

        const float step = (tTrim - lTrim) / (float) n;
        float g = lTrim;
        for (int i = 0; i < n; ++i) { x[i] *= g; g += step; }
        lTrim = tTrim;
    }
};

StarterAmpProcessor::StarterAmpProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    ampB = std::make_unique<ParallelAmp>();
    circuitA = std::make_unique<Tube::LeadFiftyCircuit>();
    apvts.state.setProperty ("ampOrder", 2, nullptr);   // marks the amp list order used by this version
    apvts.state.setProperty ("reverbDecay", 1, nullptr); // marks that Reverb Size is a time in seconds
    apvts.state.setProperty ("ampV87", 1, nullptr);      // marks that the amp Mode menu exists (older sessions are migrated on load)

    // Rebuild a built-in cab IR whenever one of its controls changes
    for (int blk = 0; blk < kCabBlocks; ++blk)
        for (const char* base : { "cabSrc", "cabSpk", "cabCbn", "cabMic", "cabPos", "cabDist", "cabAng", "cabTight", "cabBite", "cabBox" })
            apvts.addParameterListener (base + cabLetter (blk), this);
    apvts.addParameterListener ("cabRoom", this);
    apvts.addParameterListener ("cabRoomType", this);
    apvts.addParameterListener ("cabMeasured", this);

    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
            midiParams.push_back (rp);
    midiResetDefaults();
    for (int g = 0; g < 2; ++g)
        for (int k = 0; k < kFxBlocks; ++k)
            fxOrder[g][k] = k;

    refreshAllSlots();
}

StarterAmpProcessor::~StarterAmpProcessor()
{
    cancelPendingUpdate();
    for (int blk = 0; blk < kCabBlocks; ++blk)
        for (const char* base : { "cabSrc", "cabSpk", "cabCbn", "cabMic", "cabPos", "cabDist", "cabAng", "cabTight", "cabBite", "cabBox" })
            apvts.removeParameterListener (base + cabLetter (blk), this);
    apvts.removeParameterListener ("cabRoom", this);
    apvts.removeParameterListener ("cabRoomType", this);
    apvts.removeParameterListener ("cabMeasured", this);
}

bool StarterAmpProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && out == layouts.getMainInputChannelSet();
}

//==============================================================================
void StarterAmpProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    namMaxBlock = std::max (512, samplesPerBlock);
    lofi.prepare (sampleRate);
    lofiWasOn = false;
    boost.prepare (sampleRate);  boostWasOn = false;
    for (auto& m : modFx) m.prepare (sampleRate);
    modWasOn[0] = modWasOn[1] = false;
    wetEnsemble.prepare (sampleRate);
    symphShifter.prepare (sampleRate);  symphShifter.setWindowMs (45.0f);  symphWasOn = false;
    bloomGain = 1.0f;
    for (auto& ns : namSlot) ns.prepare (namMaxBlock);
    namDirty = true;                       // rebuild the model for this sample rate
    triggerAsyncUpdate();
    osMode = wantedAmpQuality();
    const double factor = 4.0;                                          // the filters below are sized for the biggest case
    osSampleRate = sampleRate * (osMode == 0 ? 4.0 : osMode == 1 ? 2.0 : 1.0);

    const auto numCh = (juce::uint32) getTotalNumOutputChannels();
    juce::dsp::ProcessSpec spec   { sampleRate,   (juce::uint32) samplesPerBlock, numCh };
    juce::dsp::ProcessSpec osSpec { osSampleRate, (juce::uint32) ((double) samplesPerBlock * factor), numCh };

    for (auto* f : { &ampResoPk, &preHighPass, &d1.odGainHighPass, &d1.odMidPeak, &d1.odEdgeShelf, &d2.odGainHighPass, &d2.odMidPeak, &d2.odEdgeShelf,
                     &bassShelf, &lowMidPeak, &highMidPeak, &trebleShelf, &postLowPass, &dcBlock,
                     &cabLowCut, &cabHighCut, &cabTiltLow, &cabTiltHigh,
                     &cabHp, &cabLowPeak, &cabMidPeak, &cabLp,
                     &cabResoPeak, &cabAirShelf,
                     &acHp, &acQuack, &acBright, &acHollow, &acBodyLp, &acWarm, &acSpark,
                     &acBody[0], &acBody[1], &acBody[2], &acBody[3], &acBody[4],
                     &fmtCut1, &fmtBoost1, &fmtCut2, &fmtBoost2, &metalFizz, &metalDepth })
    {
        f->prepare (spec);
        f->reset();
    }
    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        cabLowCutF[blk].prepare (spec);   cabLowCutF[blk].reset();
        cabHighCutF[blk].prepare (spec);  cabHighCutF[blk].reset();
        cabRsoF[blk].prepare (spec);      cabRsoF[blk].reset();
        cabPhsF[blk].prepare (spec);      cabPhsF[blk].reset();
        cabTiltLoF[blk].prepare (spec);   cabTiltLoF[blk].reset();
        cabTiltHiF[blk].prepare (spec);   cabTiltHiF[blk].reset();
        for (auto& f : cabEqF[blk])
        {
            f.prepare (spec);
            f.reset();
        }
    }
    for (auto& f : eqBands)
    {
        f.prepare (spec);
        f.reset();
    }
    for (auto* f : { &eqHp1, &eqHp2, &eqLp1, &eqLp2 })
    {
        f->prepare (spec);
        f->reset();
    }
    for (auto* f : { &d1.osOdTone, &d2.osOdTone, &osVoicePeak, &osAmpHighPass1, &osAmpHighPass2, &osAmpHighPass3, &osAmpLowPass2, &osAmpLowPass3, &osPresence, &osBright })
    {
        f->prepare (osSpec);
        f->reset();
    }

    // Fixed filters
    *preHighPass.state    = *Coeffs::makeHighPass (sampleRate, 80.0f);
    for (DriveState* d : { &d1, &d2 })
    {
        d->odCircBp.reset();  d->odCircGe.reset();  d->odCircDiode.reset();  d->odCircRail.reset();
        d->ds1.prepare (sampleRate);  d->newPed.prepare (sampleRate);  d->klon.prepare (sampleRate);
        *d->odGainHighPass.state = *Coeffs::makeHighPass (sampleRate, 120.0f);  // gain path ignores deep lows
    }
    dyna.prepare (sampleRate);
    *dcBlock.state        = *Coeffs::makeHighPass (sampleRate, 25.0f);   // removes DC left by asymmetric clipping

    updateFilters();

    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler2.initProcessing ((size_t) samplesPerBlock);
    for (DriveState* d : { &d1, &d2 }) { d->osA.initProcessing ((size_t) samplesPerBlock); d->osB.initProcessing ((size_t) samplesPerBlock);
                                         d->osDryA.initProcessing ((size_t) samplesPerBlock); d->osDryB.initProcessing ((size_t) samplesPerBlock); }
    transposer.prepare (sampleRate);
    wowT.prepare (sampleRate);  octV1.prepare (sampleRate);  octV2.prepare (sampleRate);
    riff.prepare (sampleRate);
    harm.prepare (sampleRate);  harmWasOn = false;  harmWindowMs = 0;  harmPosPrev = -1;  harmLatency.store (0);
    for (auto* b : { &pitchBuf1, &pitchBuf2, &pitchDry })
    {
        b->setSize (1, std::max (samplesPerBlock, 16));
        b->clear();
    }
    wowWasOn = octWasOn = false;
    wowWindowMs = octWindowMs = 0;
    wowLatency.store (0);  octLatency.store (0);
    octLp1 = octLp2 = 0.0f;
    for (int blk = 0; blk < kCabBlocks; ++blk)
        speakerDyn[blk].prepare (sampleRate);
    cabTamer.prepare (sampleRate);  cabMotion.prepare (sampleRate);
    cabTamerWasOn = cabMotionWasOn = false;
    tpBuf.setSize (1, std::max (samplesPerBlock, 16));
    tpBuf.clear();
    tpWasOn = false;
    tpWindowMs = 0;
    tpLatency.store (0);
    chugGate.prepare (sampleRate);
    ampB->prepare (samplesPerBlock, osMode);
    ampBBuf.setSize (1, std::max (samplesPerBlock, 16));
    metro.prepare (sampleRate);
    lastAbTrimGain = dbToGain (abTrimTargetDb.load());

    // Reserve every scratch buffer now, so the audio thread never has to allocate in processBlock.
    pitchBuf1.setSize (1, std::max (samplesPerBlock, 16));
    pitchBuf2.setSize (1, std::max (samplesPerBlock, 16));
    pitchDry.setSize  (1, std::max (samplesPerBlock, 16));
    autoRef.assign ((size_t) std::max (samplesPerBlock, 16), 0.0f);
    swellGains.assign ((size_t) std::max (samplesPerBlock, 16), 1.0f);
    ampBBuf.clear();
    ampBWasOn = false;
    lastALevel = 1.0f;
    swellGains.assign ((size_t) std::max (samplesPerBlock, 16), 1.0f);

    // Every parameter's live value, so processBlock can tell cheaply whether anything moved
    allParamPtrs.clear();
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (auto* raw = apvts.getRawParameterValue (rp->paramID))
                allParamPtrs.push_back (raw);
    lastParamVals.assign (allParamPtrs.size(), -1.0e9f);
    filtersDirty = true;

    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        conv[blk].prepare (spec);
        conv[blk].reset();
        cabBuffers[blk].setSize (1, samplesPerBlock);      // the cab runs on one channel (a guitar is mono)
        cabBuffers[blk].clear();
    }
    auditionConv.prepare (spec);
    auditionConv.reset();
    auditionBuf.setSize (1, samplesPerBlock);
    auditionBuf.clear();

    delayPre.prepare  (spec, param ("preDelayTime"),  param ("preDelayMix"),  1234);
    delayPost.prepare (spec, param ("postDelayTime"), param ("postDelayMix"), 5678);
    noise.prepare (sampleRate);
    gross.prepare (sampleRate);
    ring.prepare (sampleRate);
    fuzz.prepare (sampleRate, samplesPerBlock);
    fuzz2.prepare (sampleRate, samplesPerBlock);
    fuzzLeveler.prepare (sampleRate);
    fuzzLeveler2.prepare (sampleRate);
    eqLeveler.prepare (sampleRate);
    autoRef.assign ((size_t) samplesPerBlock, 0.0f);
    limiter.prepare (sampleRate);
    limWasOn = false;
    limLatency.store (0);

    // Cab section: one alignment delay line per block (mono, up to 11 ms) and the gain ramps
    juce::dsp::ProcessSpec monoSpec { sampleRate, (juce::uint32) samplesPerBlock, 1 };
    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        cabDelayLines[blk].setMaximumDelayInSamples ((int) (0.011 * sampleRate) + 4);
        cabDelayLines[blk].prepare (monoSpec);
        cabDelayLines[blk].reset();
        cabDelaySmooth[blk].reset (sampleRate, 0.03);
        cabDelaySmooth[blk].setCurrentAndTargetValue (param (kCabIds[blk].delay) * 0.001f * (float) sampleRate);
    }
    cabGainsInit = false;

    wah.prepare (sampleRate);
    tremolo.prepare (sampleRate);
    doubler.prepare (sampleRate);
    widener.prepare (sampleRate);
    wobble.prepare (sampleRate);
    swell.prepare (sampleRate);
    granular.prepare (sampleRate);
    swellGains.assign ((size_t) samplesPerBlock + 16, 1.0f);
    phaser.prepare (sampleRate);
    chorus.prepare (sampleRate);
    phaserWasOn = chorusWasOn = false;

    // Reverb pre-delay (up to 250 ms), wet filters and ducking
    reverbPreDelay.setMaximumDelayInSamples ((int) (0.26 * sampleRate) + 4);
    reverbPreDelay.prepare (monoSpec);
    reverbPreDelay.reset();
    reverbPreSmooth.reset (sampleRate, 0.05);
    reverbPreSmooth.setCurrentAndTargetValue (param ("reverbPreDelay") * 0.001f * (float) sampleRate);
    for (int ch = 0; ch < 2; ++ch) { revHp[ch].reset(); revLp[ch].reset(); }
    lastRevHp = lastRevLp = -1.0f;
    duckEnv = 0.0f;
    lastEqLevel = dbToGain (param ("eqLevel"));

    reverb.prepare (spec);
    reverb.reset();

    shimmerReverb.setSampleRate (sampleRate);
    shimmerReverb.reset();
    fdnReverb.prepare (sampleRate);
    revIdle = false;
    for (auto& s : shimmerShifters) { s.prepare (sampleRate); s.setWindowMs (45.0f); }
    shimmerWasOn = false;
    shimmerLast = shimmerLpState = shimmerHpState = 0.0f;
    {
        const int roomLen = (int) (0.06 * sampleRate) + 8;
        for (int ch = 0; ch < 2; ++ch)
        {
            acRoomBuf[ch].assign ((size_t) roomLen, 0.0f);
            acRoomLp[ch] = 0.0f;
        }
        acRoomPos = 0;
        acEnv = 0.0f;
    }
    oversampler.reset();
    oversampler2.reset();
    for (DriveState* d : { &d1, &d2 }) { d->osA.reset(); d->osB.reset(); d->osDryA.reset(); d->osDryB.reset(); }
    circuitA->prepare (2.0 * sampleRate);          // the circuit runs at twice the host rate
    circuitWasActive = false;
    setLatencySamples (osLatencySamples());

    for (DriveState* d : { &d1, &d2 }) { d->odGainBuffer.setSize ((int) numCh, samplesPerBlock); d->odGainBuffer.clear(); d->odGate.prepare (sampleRate); }
    drivePar.setSize (1, samplesPerBlock);
    gate.prepare (sampleRate);
    comp.prepare (sampleRate);
    deNoise.prepare (sampleRate);

    // Start the gain ramps at the current knob positions (no initial sweep).
    lastInputGain  = dbToGain (param ("inGain"));
    lastOutputGain = dbToGain (param ("outGain"));
    {
        for (int w = 0; w < 2; ++w)
        {
            const juce::String pre = w == 0 ? "od" : "od2";
            auto P = [&] (const char* n) { return param ((pre + n).toRawUTF8()); };
            DriveState& d = w == 0 ? d1 : d2;
            const int odCode0 = odCodeOf ((int) P ("Style"), P ("Range") > 0.5f, (int) P ("Clip"), param ("odCircuit") > 0.5f);
            d.lastOdGainPath = odGainPathLevel (P ("Gain"), odCode0) * (odCode0 >= 13 ? 1.0f : odVoiceScale ((int) P ("Style"), (int) P ("Voice")));
            d.lastOdClean    = odCleanLevel    (P ("Gain"), odCode0);
            d.lastOdVolume   = dbToGain (odVolumeDbEff (apvts, pre.toRawUTF8()));
        }
    }
    {
        const auto& v = ampVoicingFor (apvts, false);
        lastAmpGain1 = stageGain (v, 0, param ("ampGain"));
        lastAmpGain2 = stageGain (v, 1, param ("ampGain"));
        lastAmpGain3 = stageGain (v, 2, param ("ampGain"));
        lastAmpTrim  = dbToGain (v.trimDb);
        sagEnv[0] = sagEnv[1] = 0.0f;
        psChainA.reset();  psSupplyA.reset();  psFbLastA = 0.0f;
    }
}

//==============================================================================
// NOTE: these Coeffs::make... calls allocate memory, which real-time audio
// code should avoid. Fine for a learning project; fix it later by only
// updating when a knob actually changes.
//==============================================================================
// Acoustic / piezo model. A piezo under-saddle pickup sounds thin, quacky (a nasal peak around
// 2 kHz) and glassy. This chain (1) removes some of that, (2) adds a little preamp warmth and
// squeezes the dynamics, (3) imposes a body: the "modes" of an acoustic guitar (air resonance
// near 100 Hz, top plate around 200 Hz, and so on), (4) adds sparkle/warmth shelves, and
// (5) a few early reflections for room and width. All numbers are my own rule-of-thumb
// approximations of how acoustic bodies behave, not measurements of any real guitar.
void StarterAmpProcessor::processAcoustic (juce::AudioBuffer<float>& buffer, juce::dsp::ProcessContextReplacing<float>& context,
                                           int numChannels, int numSamples)
{
    acHp.process (context);
    if (param ("acFix") > 0.01f)
    {
        acQuack.process (context);
        acBright.process (context);
        acHollow.process (context);
    }

    // Preamp warmth (soft saturation) and squeeze (a gentle compressor)
    const float drive   = param ("acPreamp");
    const float squeeze = param ("acSqueeze");
    const float k       = 1.0f + 3.0f * drive;
    const float thrDb   = -6.0f - 24.0f * squeeze;
    const float ratio   = 1.5f + 3.0f * squeeze;
    const float makeup  = dbToGain (0.5f * (-thrDb) * (1.0f - 1.0f / ratio));
    const float sr      = (float) currentSampleRate;
    const float atk     = std::exp (-1.0f / (0.004f * sr));
    const float rel     = std::exp (-1.0f / (0.150f * sr));

    for (int i = 0; i < numSamples; ++i)
    {
        float level = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            level = std::max (level, std::abs (buffer.getReadPointer (ch)[i]));
        acEnv = level > acEnv ? atk * acEnv + (1.0f - atk) * level : rel * acEnv + (1.0f - rel) * level;

        float gain = 1.0f;
        if (squeeze > 0.01f)
        {
            const float envDb = juce::Decibels::gainToDecibels (acEnv + 1.0e-6f);
            if (envDb > thrDb)
                gain = dbToGain ((thrDb - envDb) * (1.0f - 1.0f / ratio)) * makeup;
            else
                gain = makeup;
        }

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            float x = d[i] * gain;
            if (drive > 0.01f)
                x = fastTanh (k * x) / k;
            d[i] = x;
        }
    }

    for (auto& f : acBody)
        f.process (context);
    if ((int) param ("acBody") == 3)
        acBodyLp.process (context);
    if (std::abs (param ("acWarmth"))  > 0.05f) acWarm.process (context);
    if (std::abs (param ("acSparkle")) > 0.05f) acSpark.process (context);

    // Room: early reflections, crossing between the channels for width
    const float room = param ("acRoom");
    const int   len  = (int) acRoomBuf[0].size();
    if (room > 0.01f && len > 0)
    {
        static const float tapMs[4]   = { 7.0f, 13.0f, 19.0f, 29.0f };
        static const float tapGain[4] = { 0.50f, 0.40f, 0.30f, 0.22f };
        const float lpCoef = 1.0f - std::exp (-2.0f * juce::MathConstants<float>::pi * 4000.0f / sr);

        for (int i = 0; i < numSamples; ++i)
        {
            for (int ch = 0; ch < numChannels; ++ch)
                acRoomBuf[ch][(size_t) acRoomPos] = buffer.getReadPointer (ch)[i];

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float wet = 0.0f;
                for (int t = 0; t < 4; ++t)
                {
                    const int pos = (acRoomPos - (int) (tapMs[t] * 0.001f * sr) + len * 4) % len;
                    const int src = (numChannels == 2 && (t & 1) == 0) ? 1 - ch : ch;   // even taps cross over
                    wet += tapGain[t] * acRoomBuf[src][(size_t) pos];
                }
                acRoomLp[ch] += lpCoef * (wet - acRoomLp[ch]);
                buffer.getWritePointer (ch)[i] += room * 0.5f * acRoomLp[ch];
            }
            acRoomPos = (acRoomPos + 1) % len;
        }
    }

    buffer.applyGain (0, numSamples, dbToGain (param ("acLevel")));
}

// Latency added by the amp's oversampling (0 when it is off), as reported to the host.
int StarterAmpProcessor::osLatencySamples() const
{
    const int tp = tpLatency.load() + limLatency.load() + wowLatency.load() + octLatency.load() + harmLatency.load();
    if (osMode == 0) return tp + (int) std::lround (oversampler.getLatencyInSamples());
    if (osMode == 1) return tp + (int) std::lround (oversampler2.getLatencyInSamples());
    return tp;
}

// True when any parameter has moved since the last call. (Rebuilding ~40 filter coefficient sets
// every block was a lot of work for nothing when no knob is moving.)
bool StarterAmpProcessor::paramsChanged()
{
    bool changed = false;
    for (size_t i = 0; i < allParamPtrs.size(); ++i)
    {
        const float v = allParamPtrs[i]->load();
        if (v != lastParamVals[i])
        {
            lastParamVals[i] = v;
            changed = true;
        }
    }
    return changed;
}

void StarterAmpProcessor::updateDrive (int which)
{
    DriveState& d = which == 0 ? d1 : d2;
    const juce::String pre = which == 0 ? "od" : "od2";
    auto P = [&] (const char* name) { return param ((pre + name).toRawUTF8()); };
    const float odGain = P ("Gain");
    const float odTone = P ("Tone");
    const float odEdge = P ("Edge");

    const int odSel = (int) P ("Style");
    const bool circ = odCircuitActive (odSel, param ("odCircuit") > 0.5f);
    if (odSel >= 6)
    {
        // v92: Op-Amp Distortion (kind 2) and Stack Drive (kind 3) from published analyses (see Circuit::NewPedals). The Clipping menu picks the diodes where there are any.
        const int clipSel = juce::jlimit (0, 3, (int) P ("Clip"));
        Circuit::DiodeSpec dp = Circuit::diodeSilicon(), dn = dp;
        if (odSel == 7) dp = dn = Circuit::diodeLed();
        else if (clipSel == 1) dp = dn = Circuit::diodeLed();
        else if (clipSel == 2) dp = dn = Circuit::diodeGermanium();
        else if (clipSel == 3) dn = Circuit::diodeSiliconPair();
        d.newPed.setup (odSel - 4, odGain, odTone, dp, dn);
        *d.osOdTone.state = *Coeffs::makeHighShelf (osSampleRate, 1800.0f, 0.707f, 1.0f);
    }
    else if (circ && odSel == 0)
    {
        // v93: Transparent is the Centaur-type circuit (Circuit::KlonCircuit). Gain, Tone (the treble shelf) and Edge (extra top into the clipper).
        d.klon.setup (odGain, odTone, odEdge, P ("Grip") > 0.5f);
        *d.osOdTone.state = *Coeffs::makeHighShelf (osSampleRate, 1800.0f, 0.707f, 1.0f);
    }
    else if (circ && odSel == 4)
    {
        // v91: the Distortion circuit (see Circuit::Ds1Circuit). The Clipping menu picks the diode pair.
        const int clipSel = juce::jlimit (0, 3, (int) P ("Clip"));
        Circuit::DiodeSpec dp = Circuit::diodeSilicon(), dn = dp;
        if (clipSel == 1) dp = dn = Circuit::diodeLed();
        else if (clipSel == 2) dp = dn = Circuit::diodeGermanium();
        else if (clipSel == 3) dn = Circuit::diodeSiliconPair();
        d.ds1.setup (odGain, odTone, dp, dn);
        *d.osOdTone.state = *Coeffs::makeHighShelf (osSampleRate, 1800.0f, 0.707f, 1.0f);      // flat: the tone network is inside the circuit
    }
    else if (circ)
    {
        // v77 circuit modes. Component values come from the published schematics (see MODELED_GEAR.md).
        const int clipSel = juce::jlimit (0, 4, (int) P ("Clip"));
        Circuit::DiodeSpec dp = Circuit::diodeSilicon(), dn = dp;
        if (clipSel == 1) dp = dn = Circuit::diodeLed();
        else if (clipSel == 2) dp = dn = Circuit::diodeGermanium();
        else if (clipSel == 3) dn = Circuit::diodeSiliconPair();
        d.odCircRailMode = clipSel == 4;

        const double fsBase = currentSampleRate;
        float r1, c1, rf, c4, edgeDb = 0.0f;
        const float k = juce::jlimit (0.0f, 1.0f, odGain);
        const float pi2 = 6.2831853f;
        Circuit::BiquadCoeffs tone;
        if (odSel == 3)
        {
            static const float voiceRf[3] = { 1.0f, 0.7f, 1.3f }, brightDb[3] = { 0.0f, -1.0f, 2.0f }, humpDb[3] = { 0.0f, -3.0f, -2.0f };
            const int voice = juce::jlimit (0, 2, (int) P ("Voice"));
            const float bass = P ("Bass");
            // 0.4 is the stock 720 Hz corner; the travel covers about 70 Hz to 1.8 kHz
            const float corner = 720.0f * std::pow (10.0f, bass < 0.4f ? (0.4f - bass) : (0.4f - bass) * 1.667f) * (voice == 1 ? 1.25f : 1.0f);
            r1 = 4700.0f;  c1 = 1.0f / (pi2 * r1 * corner);
            rf = (k < 0.3f ? 1500.0f * std::pow (51000.0f / 1500.0f, k / 0.3f)
                           : 51000.0f + 500000.0f * Circuit::audioTaper ((k - 0.3f) / 0.7f)) * voiceRf[voice];
            c4 = 51.0e-12f;
            edgeDb = 10.0f * (odEdge - 0.3f) + brightDb[voice];
            const float midHz = 350.0f * std::pow (6.0f, P ("MidFreq"));
            *d.odMidPeak.state = *Coeffs::makePeakFilter (currentSampleRate, midHz, 0.9f,
                                                        dbToGain ((P ("Mid") - 0.5f) * 16.0f + humpDb[voice]));
            tone = Circuit::designToneStage (osSampleRate, 1000.0f, 0.22e-6f, 1000.0f, 220.0f, 0.22e-6f, 20000.0f * (1.0f - odTone));
        }
        else
        {
            static const float attackCap[6] = { 470.0e-9f, 330.0e-9f, 220.0e-9f, 100.0e-9f, 68.0e-9f, 47.0e-9f };
            r1 = 1000.0f;  c1 = attackCap[juce::jlimit (0, 5, (int) std::lround (P ("Attack")))];
            rf = k < 0.12f ? 1500.0f * std::pow (10000.0f / 1500.0f, k / 0.12f)
                           : 10000.0f + 500000.0f * Circuit::audioTaper ((k - 0.12f) / 0.88f);
            c4 = 47.0e-12f;
            tone = Circuit::designToneStage (osSampleRate, 1.0f, 1.0e-9f, 3900.0f, 1000.0f, 47.0e-9f, 5000.0f * (1.0f - odTone));
        }
        const auto gs = Circuit::designGainStage (fsBase, r1, c1, rf, c4, 3.0e6f, edgeDb);
        d.odCircBp.set (gs.bandPass);  d.odCircGe.set (gs.gbwAndEdge);  d.odCircPeak = gs.peakGain;
        d.odCircDiode.setup (rf, dp, dn);
        d.odCircRail.setRail (4.5f);
        Coeffs::Ptr toneCoeffs = new Coeffs ((float) tone.b0, (float) tone.b1, (float) tone.b2, 1.0f, (float) tone.a1, (float) tone.a2);
        *d.osOdTone.state = *toneCoeffs;
    }
    else if (odSel == 5)
    {
        // Tube Driver (v64): a full-range, warm, creamy tube-style drive. Bass sets a low cut on the gain path (tight at 0, full at 1),
        // a broad mid push grows with Drive, and Treble tilts the top after the clipper. Much of the clean signal stays under it.
        *d.odGainHighPass.state = *Coeffs::makeHighPass (currentSampleRate, 250.0f * std::pow (0.12f, P ("Bass")));
        *d.odMidPeak.state      = *Coeffs::makePeakFilter (currentSampleRate, 800.0f, 0.7f, dbToGain (3.0f + 4.0f * odGain));
        *d.odEdgeShelf.state    = *Coeffs::makeHighShelf (currentSampleRate, 3000.0f, 0.707f, dbToGain (10.0f * odEdge));
        *d.osOdTone.state       = *Coeffs::makeHighShelf (osSampleRate, 1800.0f, 0.707f, dbToGain (-9.0f + 18.0f * odTone));
    }
    else if (odSel == 2)
    {
        // Open style: a mid-gain drive with a wide, open response. The range switch picks a high-gain range
        // (tighter lows, more mid push, more gain) or a low-gain range (fuller lows, flatter mids, less gain).
        const bool hi = P ("Range") > 0.5f;
        *d.odGainHighPass.state = *Coeffs::makeHighPass (currentSampleRate, hi ? 150.0f : 70.0f);
        *d.odMidPeak.state      = *Coeffs::makePeakFilter (currentSampleRate, hi ? 900.0f : 1100.0f, 0.8f,
                                                         dbToGain (hi ? 2.0f + 6.0f * odGain : 1.0f + 2.5f * odGain));
        *d.odEdgeShelf.state    = *Coeffs::makeHighShelf (currentSampleRate, 3000.0f, 0.707f, dbToGain (hi ? 3.0f : 1.5f));
        *d.osOdTone.state       = *Coeffs::makeHighShelf (osSampleRate, 2000.0f, 0.707f, dbToGain (-9.0f + 18.0f * odTone));
    }
    else if (odSel == 3)
    {
        // Screamer (v58, restored v94): the classic drive-pedal layout. The clean path stays, and the gain path has a low cut that Bass
        // moves (tight and mid-focused at 0, full at 1) and a mid peak that Mid and Mid Freq place. The voicing switch
        // changes the low cut, the mid hump, the brightness (and the gain, in odVoiceScale).
        static const float hpScale[3] = { 1.0f, 0.75f, 1.2f }, humpDb[3] = { 3.0f, 0.0f, 1.0f }, brightDb[3] = { 0.0f, -1.0f, 2.0f };
        const int voice = juce::jlimit (0, 2, (int) P ("Voice"));
        const float hp    = 700.0f * std::pow (0.1f, P ("Bass")) * hpScale[voice];
        const float midHz = 350.0f * std::pow (6.0f, P ("MidFreq"));
        const float midDb = (P ("Mid") - 0.5f) * 16.0f + humpDb[voice] + 2.0f * odGain;
        *d.odGainHighPass.state = *Coeffs::makeHighPass (currentSampleRate, hp);
        *d.odMidPeak.state      = *Coeffs::makePeakFilter (currentSampleRate, midHz, 0.9f, dbToGain (midDb));
        *d.odEdgeShelf.state    = *Coeffs::makeHighShelf (currentSampleRate, 3000.0f, 0.707f, dbToGain (10.0f * odEdge + brightDb[voice]));
        *d.osOdTone.state       = *Coeffs::makeHighShelf (osSampleRate, 1500.0f, 0.707f, dbToGain (-10.0f + 18.0f * odTone));
    }
    else if (odSel == 4)
    {
        // Distortion (v58, restored v94): everything goes through a high-gain stage and a hard clipper. Tone tilts the highs around
        // 1.2 kHz, Edge is the pre-clip bright, and the Mids switch scoops, flattens or boosts around 700 Hz.
        static const float midModDb[3] = { -5.0f, 0.0f, 6.0f };
        *d.odGainHighPass.state = *Coeffs::makeHighPass (currentSampleRate, 90.0f);
        *d.odMidPeak.state      = *Coeffs::makePeakFilter (currentSampleRate, 700.0f, 0.7f,
                                                           dbToGain (midModDb[juce::jlimit (0, 2, (int) P ("MidMod"))]));
        *d.odEdgeShelf.state    = *Coeffs::makeHighShelf (currentSampleRate, 3000.0f, 0.707f, dbToGain (10.0f * odEdge));
        *d.osOdTone.state       = *Coeffs::makeHighShelf (osSampleRate, 1200.0f, 0.707f, dbToGain (-10.0f + 20.0f * odTone));
    }
    else if (odSel == 1)
    {
        // Tight style (restored v94): a tight, modern metal-style boost with Drive, Volume, Bright, Gate and a
        // 6-position Attack setting that tightens the low end and adds mid bite (my own approximation).
        static const float attackHighPassHz[6] = { 60.0f, 100.0f, 160.0f, 240.0f, 350.0f, 520.0f };
        static const float attackMidBoostDb[6] = { 0.0f, 1.0f, 2.0f, 3.5f, 5.0f, 6.5f };
        const int attack = juce::jlimit (0, 5, (int) std::lround (P ("Attack")));
        *d.odGainHighPass.state = *Coeffs::makeHighPass (currentSampleRate, attackHighPassHz[attack]);
        *d.odMidPeak.state      = *Coeffs::makePeakFilter (currentSampleRate, 750.0f, 0.8f,
                                                           dbToGain (4.0f + attackMidBoostDb[attack] + 4.0f * odGain));
        *d.odEdgeShelf.state    = *Coeffs::makeHighShelf (currentSampleRate, 3000.0f, 0.707f, dbToGain (9.0f * odTone));      // Bright
        *d.osOdTone.state       = *Coeffs::makeHighShelf (osSampleRate, 1800.0f, 0.707f, dbToGain (-4.0f));
    }
    else
    {
        // Transparent, hand-tuned (restored v94): a mid push around 1 kHz that grows with Gain, plus "Edge", a treble shelf
        // that feeds extra top end into the clipper; Tone is a treble shelf after the clipper.
        // Grip (v94): the gain path's low cut moves up (tighter low mids into the clipper) and the Edge shelf starts lower with a little extra.
        const bool grip = P ("Grip") > 0.5f;
        *d.odGainHighPass.state = *Coeffs::makeHighPass (currentSampleRate, grip ? 240.0f : 120.0f);
        *d.odMidPeak.state      = *Coeffs::makePeakFilter (currentSampleRate, 1000.0f, 0.8f, dbToGain (3.0f + 9.0f * odGain));
        *d.odEdgeShelf.state    = *Coeffs::makeHighShelf  (currentSampleRate, grip ? 1500.0f : 3000.0f, 0.707f, dbToGain (10.0f * odEdge + (grip ? 1.5f : 0.0f)));
        *d.osOdTone.state       = *Coeffs::makeHighShelf (osSampleRate, 1800.0f, 0.707f, dbToGain (-9.0f + 18.0f * odTone));
    }

}

void StarterAmpProcessor::resolveDriveRoute()
{
    const bool on1 = param ("odOn") > 0.5f, on2 = param ("od2On") > 0.5f;
    const bool par = (int) param ("od2Route") == 1;
    parallelBoth = on1 && on2 && par;
    seriesBoth   = on1 && on2 && ! par;
    frontDrive   = (on2 && ! parallelBoth) ? 1 : 0;       // the drive built into the amp's oversampled path
    frontOn      = parallelBoth ? false : (on1 || on2);
}

int StarterAmpProcessor::driveCode (int which)
{
    const juce::String pre = which == 0 ? "od" : "od2";
    auto P = [&] (const char* n) { return param ((pre + n).toRawUTF8()); };
    return odCodeOf ((int) P ("Style"), P ("Range") > 0.5f, (int) P ("Clip"), odCircuitActive ((int) P ("Style"), param ("odCircuit") > 0.5f));
}

int StarterAmpProcessor::runDriveLinear (int which, juce::AudioBuffer<float>& buffer, int numSamples, bool& circ)
{
    DriveState& d = which == 0 ? d1 : d2;
    const juce::String pre = which == 0 ? "od" : "od2";
    auto P = [&] (const char* n) { return param ((pre + n).toRawUTF8()); };
    const float odGain = P ("Gain");
    const int odStyleRaw = (int) P ("Style");
    circ = odCircuitActive (odStyleRaw, param ("odCircuit") > 0.5f);
    const int odStyle = odCodeOf (odStyleRaw, P ("Range") > 0.5f, (int) P ("Clip"), circ);
    d.dryValid = P ("Mix") < 0.999f || d.lastMix < 0.999f;
    if (d.dryValid) { d.dryBuf.setSize (1, numSamples, false, false, true); d.dryBuf.copyFrom (0, 0, buffer, 0, 0, numSamples); }

    // The tight style has a built-in noise gate, ahead of the gain.
    if (odStyleRaw == 1 && P ("Gate") > 0.01f)
        d.odGate.process (buffer, -75.0f + 40.0f * P ("Gate"));

    const float gainPath = odGainPathLevel (odGain, odStyle) * (circ ? 1.0f : odVoiceScale (odStyleRaw, (int) P ("Voice")));
    const float clean    = odCleanLevel (odGain, odStyle);

    d.odGainBuffer.setSize (1, numSamples, false, false, true);
    d.odGainBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
    juce::dsp::AudioBlock<float> gainBlock (d.odGainBuffer.getArrayOfWritePointers(), (size_t) 1, (size_t) numSamples);
    juce::dsp::ProcessContextReplacing<float> gainContext (gainBlock);
    if (circ)
    {
        if (odStyleRaw == 3) d.odMidPeak.process (gainContext);
        float* g = d.odGainBuffer.getWritePointer (0);
        const float* c = buffer.getReadPointer (0);
        if (odStyleRaw >= 6)
            for (int i = 0; i < numSamples; ++i) g[i] = d.newPed.process (c[i]) * odPedTrim[odStyleRaw - 6];
        else if (odStyleRaw == 0)
            for (int i = 0; i < numSamples; ++i) g[i] = d.klon.process (c[i]) * odKlonTrim;
        else if (odStyleRaw == 4)
            for (int i = 0; i < numSamples; ++i) g[i] = d.ds1.process (c[i]);
        else
            for (int i = 0; i < numSamples; ++i)
            {
                float v = d.odCircBp.run (g[i]);
                v = d.odCircGe.run (v) * d.odCircPeak;
                g[i] = d.odCircRailMode ? d.odCircRail.process (c[i] + v) - c[i] : d.odCircDiode.process (v);
            }
    }
    else
    {
        d.odGainHighPass.process (gainContext);
        d.odMidPeak.process (gainContext);
        d.odEdgeShelf.process (gainContext);
    }
    d.odGainBuffer.applyGainRamp (0, numSamples, d.lastOdGainPath, gainPath);
    buffer.applyGainRamp         (0, numSamples, d.lastOdClean, clean);
    d.lastOdGainPath = gainPath;
    d.lastOdClean    = clean;
    buffer.addFrom (0, 0, d.odGainBuffer, 0, 0, numSamples);
    return odStyle;
}

void StarterAmpProcessor::blendDriveDry (DriveState& d, float* y, int m, int numSamples, float mix)
{
    if (! d.dryValid) { d.lastMix = mix; return; }
    const float* dry = nullptr;
    if (osMode == 2) dry = d.dryBuf.getReadPointer (0);
    else
    {
        juce::dsp::AudioBlock<float> blk (d.dryBuf.getArrayOfWritePointers(), (size_t) 1, (size_t) numSamples);
        auto ob = (osMode == 0 ? d.osDryA : d.osDryB).processSamplesUp (blk);
        dry = ob.getChannelPointer (0);
    }
    const float step = (mix - d.lastMix) / (float) m;
    float g = d.lastMix;
    for (int i = 0; i < m; ++i) { y[i] = g * y[i] + (1.0f - g) * dry[i]; g += step; }
    d.lastMix = mix;
}

void StarterAmpProcessor::runDriveStandalone (int which, juce::AudioBuffer<float>& buffer, int numSamples)
{
    DriveState& d = which == 0 ? d1 : d2;
    bool circ = false;
    const int code = runDriveLinear (which, buffer, numSamples, circ);
    const float vol = dbToGain (odVolumeDbEff (apvts, which == 0 ? "od" : "od2"));
    juce::dsp::AudioBlock<float> blk (buffer.getArrayOfWritePointers(), (size_t) 1, (size_t) numSamples);
    auto& os = osMode == 0 ? d.osA : d.osB;
    auto ob = osMode == 2 ? blk : os.processSamplesUp (blk);
    const int m = (int) ob.getNumSamples();
    float* y = ob.getChannelPointer (0);
    const auto clip = odClipFor (code);
    for (int i = 0; i < m; ++i) y[i] = clip (y[i]);
    juce::dsp::ProcessContextReplacing<float> ctx (ob);
    d.osOdTone.process (ctx);
    blendDriveDry (d, y, m, numSamples, param (which == 0 ? "odMix" : "od2Mix"));
    const float step = (vol - d.lastOdVolume) / (float) m;
    float g = d.lastOdVolume;
    for (int i = 0; i < m; ++i) { y[i] *= g; g += step; }
    d.lastOdVolume = vol;
    if (osMode != 2) os.processSamplesDown (blk);
}

void StarterAmpProcessor::updateFilters()
{
    resolveDriveRoute();
    updateDrive (0);
    updateDrive (1);

    // Amp voicing: the Amp Model setting picks the tight-ness of each stage,
    // the pre-gain boost, the tone stack's centre frequencies, and the speaker
    // roll-off used when no IR is loaded.
    const auto& v = ampVoicingFor (apvts, false);

    *osVoicePeak.state    = *Coeffs::makePeakFilter (osSampleRate, v.voiceHz, v.voiceQ, dbToGain (v.voiceDb));
    *osAmpHighPass1.state = *Coeffs::makeHighPass   (osSampleRate, v.hp[0]);
    *osAmpHighPass2.state = *Coeffs::makeHighPass   (osSampleRate, v.hp[1]);
    *osAmpHighPass3.state = *Coeffs::makeHighPass   (osSampleRate, v.hp[2]);
    {
        const double lpHz = juce::jmin (v.stageLpHz > 0.0f ? (double) v.stageLpHz : 8000.0, 0.45 * osSampleRate);
        *osAmpLowPass2.state = *Coeffs::makeFirstOrderLowPass (osSampleRate, lpHz);
        *osAmpLowPass3.state = *Coeffs::makeFirstOrderLowPass (osSampleRate, lpHz);
    }
    *postLowPass.state    = *Coeffs::makeLowPass    (currentSampleRate, v.cabLowpassHz);

    // Amp realism: the bright switch (a treble shelf ahead of the first stage that matters less as the gain goes up)
    // and the low-end resonance of the power amp and speaker
    {
        float bHz = 2800.0f, bDb = 8.0f * (1.0f - 0.75f * param ("ampGain"));
        derivedBright (v, param ("ampGain"), bHz, bDb);
        *osBright.state  = *Coeffs::makeHighShelf (osSampleRate, bHz, 0.707f, dbToGain (bDb));
    }
    *ampResoPk.state = *Coeffs::makePeakFilter (currentSampleRate, 105.0f, 1.1f, dbToGain (6.0f * param ("ampReso")));

    // Presence: a treble shelf on the power-amp stage. 0 = off, 1 = full boost
    // for the chosen model. (On a real amp it works by reducing negative
    // feedback at high frequencies, which has a similar effect.)
    *osPresence.state = *Coeffs::makeHighShelf (osSampleRate, v.presenceHz, 0.707f,
                                                dbToGain (param ("presence") * v.presenceMaxDb));
    psParamsA = v.ps;
    if (psParamsA.filtersOn)
    {
        float g[4];
        PowerSection::gainsFor (psParamsA.family, psParamsA.feel, param ("presence"), g);
        psChainA.design (osSampleRate, g);
    }

    // Amp tone stack (the knobs are added to each model's built-in offsets)
    *bassShelf.state    = *Coeffs::makeLowShelf   (currentSampleRate, v.bassHz,    0.707f,
                                                   dbToGain (param ("bass")    + v.bassOffsetDb));
    *lowMidPeak.state   = *Coeffs::makePeakFilter (currentSampleRate, v.lowMidHz,  v.lowMidQ,
                                                   dbToGain (param ("lowMid")  + v.lowMidOffsetDb));
    *highMidPeak.state  = *Coeffs::makePeakFilter (currentSampleRate, v.highMidHz, v.highMidQ,
                                                   dbToGain (param ("highMid") + v.highMidOffsetDb));
    *trebleShelf.state  = *Coeffs::makeHighShelf  (currentSampleRate, v.trebleHz,  0.707f,
                                                   dbToGain (param ("treble")  + v.trebleOffsetDb));

    // Parallel Amp B (its own voicing and tone stack; shares Amp Focus and the OD tone with Amp A)
    {
        const auto vb = ampVoicingFor (apvts, true);
        const float bBias = param ("ampBBias");
        const int   bModelId = ampBModelValue ((int) param ("ampBModel"));
        const float ampBLevelDb = param ("ampBLevel")
            + (param ("ampBAutoLevel") > 0.5f ? autoAmpLevelDb (bModelId, (int) param ("ampBVoicing"), param ("ampBGain")) : 0.0f);
        ampB->plan = (int) param ("ampBMode") == 1 ? StageShaper::planFor (bModelId, (int) param ("ampBTube"), (int) param ("ampBPwrTube")) : StageShaper::Plan();
        ampB->biasKnob = bBias;
        ampB->stackSel = (int) param ("ampBStack");
        ampB->stackRate = currentSampleRate;
        ampB->posT = juce::jlimit (0.0f, 1.0f, (param ("ampBTreble") + 12.0f) / 24.0f);
        ampB->posB = juce::jlimit (0.0f, 1.0f, (param ("ampBBass") + 12.0f) / 24.0f);
        ampB->posM = 0.5f * (juce::jlimit (0.0f, 1.0f, (param ("ampBLowMid") + 12.0f) / 24.0f) + juce::jlimit (0.0f, 1.0f, (param ("ampBHighMid") + 12.0f) / 24.0f));
        if (! ampB->plan.active) for (auto& s : ampB->stageSt) s.shift = 0.0f;
        ampB->update (currentSampleRate, osSampleRate, osMode, vb, param ("ampBGain"), param ("ampBPresence"),
                      param ("ampBBass"), param ("ampBLowMid"), param ("ampBHighMid"), param ("ampBTreble"),
                      ampBLevelDb, dbToGain (odVolumeDbEff (apvts, frontDrive == 0 ? "od" : "od2")), (frontDrive == 0 ? d1 : d2).osOdTone.state,
                      param ("ampBBright") > 0.5f, 8.0f * (1.0f - 0.75f * param ("ampBGain")),
                      effectivePower (param ("ampBPower"), vb), bBias < 0.0f ? -0.12f * bBias : 0.0f, param ("ampBReso"));
    }

    // Modern metal: fizz tamer (a low-pass before the tone stack) and depth (a low-end shelf)
    {
        const float fizz  = param ("mtFizz");
        const float depth = param ("mtDepth");
        *metalFizz.state  = *Coeffs::makeLowPass  (currentSampleRate,
                                std::min (12000.0f * std::pow (3800.0f / 12000.0f, fizz), 0.45f * (float) currentSampleRate), 0.6f);
        *metalDepth.state = *Coeffs::makeLowShelf (currentSampleRate, 90.0f, 0.8f, dbToGain (6.0f * depth));
    }

    // Transpose "formant" shift: an EQ-style approximation. Spectral emphasis around two reference
    // regions is cut and re-added shifted by the formant amount (plus the opposite of the transpose
    // amount when Formant Lock is on, so the tone stays put as the pitch moves).
    {
        const float semis = param ("tpSemis") + 0.01f * param ("tpFine");
        const float eff   = param ("tpFormant") + (param ("tpLock") > 0.5f ? -semis : 0.0f);
        const float ratio = std::pow (2.0f, juce::jlimit (-24.0f, 24.0f, eff) / 12.0f);
        const float g     = std::min (9.0f, std::abs (eff) * 0.6f);
        fmtGainDb = (std::abs (eff) > 0.05f) ? g : 0.0f;
        const float nyq = 0.45f * (float) currentSampleRate;
        *fmtCut1.state   = *Coeffs::makePeakFilter (currentSampleRate, 700.0f,  0.9f, dbToGain (-g));
        *fmtBoost1.state = *Coeffs::makePeakFilter (currentSampleRate, juce::jlimit (80.0f, nyq, 700.0f * ratio),  0.9f, dbToGain (g));
        *fmtCut2.state   = *Coeffs::makePeakFilter (currentSampleRate, 2200.0f, 0.9f, dbToGain (-g));
        *fmtBoost2.state = *Coeffs::makePeakFilter (currentSampleRate, juce::jlimit (80.0f, nyq, 2200.0f * ratio), 0.9f, dbToGain (g));
    }

    // Cab section: low cut, high cut and tilt apply to everything the cab makes
    // (IRs or the built-in cab). Tilt is a low shelf and a high shelf in opposite directions.
    const float tilt = param ("cabTilt");
    *cabLowCut.state   = *Coeffs::makeHighPass  (currentSampleRate, param ("cabLowCut"));
    *cabHighCut.state  = *Coeffs::makeLowPass   (currentSampleRate,
                                                 std::min (param ("cabHighCut"), 0.45f * (float) currentSampleRate));
    *cabTiltLow.state  = *Coeffs::makeLowShelf  (currentSampleRate, 800.0f, 0.707f, dbToGain (-0.5f * tilt));
    *cabTiltHigh.state = *Coeffs::makeHighShelf (currentSampleRate, 800.0f, 0.707f, dbToGain ( 0.5f * tilt));

    // Per-IR low cut and high cut, plus the cab's resonance and air
    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        *cabLowCutF[blk].state  = *Coeffs::makeHighPass (currentSampleRate, param (kCabIds[blk].lowCut));
        *cabHighCutF[blk].state = *Coeffs::makeLowPass  (currentSampleRate, std::min (param (kCabIds[blk].highCut), 0.45f * (float) currentSampleRate));

        // Per-block shaping (v41): five EQ bands, a resonance peak at the cab's Reso Hz, and a phase all-pass
        const auto& cid = kCabIds[blk];
        const double csr = currentSampleRate;
        *cabEqF[blk][0].state = *Coeffs::makeLowShelf   (csr, 110.0f,  0.7f, dbToGain (param (cid.eq[0])));
        *cabEqF[blk][1].state = *Coeffs::makePeakFilter (csr, 330.0f,  1.0f, dbToGain (param (cid.eq[1])));
        *cabEqF[blk][2].state = *Coeffs::makePeakFilter (csr, 1000.0f, 1.0f, dbToGain (param (cid.eq[2])));
        *cabEqF[blk][3].state = *Coeffs::makePeakFilter (csr, 2800.0f, 1.2f, dbToGain (param (cid.eq[3])));
        *cabEqF[blk][4].state = *Coeffs::makeHighShelf  (csr, 6500.0f, 0.7f, dbToGain (param (cid.eq[4])));
        *cabRsoF[blk].state   = *Coeffs::makePeakFilter (csr, param ("cabResoHz"), 1.2f, dbToGain (param (cid.rso)));
        *cabPhsF[blk].state   = *Coeffs::makeAllPass    (csr, std::min (700.0f * std::pow (2.0f, 3.0f * param (cid.phs)), 0.45f * (float) csr), 0.6f);
        // v70 Tilt: positive = brighter. Half the dB below 800 Hz (cut) and half above (boost), so the level around 800 Hz stays put.
        *cabTiltLoF[blk].state = *Coeffs::makeLowShelf  (csr, 800.0f, 0.707f, dbToGain (-0.5f * param (cid.tilt)));
        *cabTiltHiF[blk].state = *Coeffs::makeHighShelf (csr, 800.0f, 0.707f, dbToGain ( 0.5f * param (cid.tilt)));
    }
    *cabResoPeak.state = *Coeffs::makePeakFilter (currentSampleRate, param ("cabResoHz"), 1.2f, dbToGain (param ("cabReso")));
    *cabAirShelf.state = *Coeffs::makeHighShelf  (currentSampleRate, 6000.0f, 0.707f, dbToGain (param ("cabAir")));

    // Acoustic / piezo model
    if ((int) param ("ampModel") == 5)
    {
        struct Mode { float hz, q, db; };
        static const Mode bodies[4][5] =
        {
            { {  95.0f, 3.0f,  4.5f }, { 190.0f, 2.5f,  3.0f }, {  420.0f, 1.6f, -2.5f }, { 1150.0f, 1.1f,  1.5f }, { 3200.0f, 1.2f, -1.5f } },   // Dreadnought
            { {  85.0f, 3.0f,  5.5f }, { 165.0f, 2.5f,  3.5f }, {  340.0f, 1.6f, -2.0f }, {  950.0f, 1.1f,  1.5f }, { 2300.0f, 1.2f,  1.5f } },   // Jumbo
            { { 125.0f, 3.0f,  3.5f }, { 245.0f, 2.5f,  3.5f }, {  520.0f, 1.5f, -1.5f }, { 1500.0f, 1.1f,  2.5f }, { 3600.0f, 1.2f,  2.0f } },   // Parlor / OM
            { { 100.0f, 3.0f,  4.5f }, { 205.0f, 2.5f,  2.5f }, {  410.0f, 1.5f,  1.5f }, {  820.0f, 1.2f, -2.5f }, { 2600.0f, 1.2f, -2.5f } },   // Nylon
        };
        const int   body = juce::jlimit (0, 3, (int) param ("acBody"));
        const float amt  = param ("acBodyAmt");
        const float fix  = param ("acFix");
        const float sr   = (float) currentSampleRate;

        *acHp.state     = *Coeffs::makeHighPass   (currentSampleRate, 70.0f);
        *acQuack.state  = *Coeffs::makePeakFilter (currentSampleRate, 2300.0f, 1.4f, dbToGain (-10.0f * fix));
        *acBright.state = *Coeffs::makePeakFilter (currentSampleRate, std::min (4800.0f, 0.45f * sr), 1.0f, dbToGain (-6.0f * fix));
        *acHollow.state = *Coeffs::makePeakFilter (currentSampleRate, 300.0f, 0.9f, dbToGain (3.0f * fix));
        for (int i = 0; i < 5; ++i)
            *acBody[i].state = *Coeffs::makePeakFilter (currentSampleRate, std::min (bodies[body][i].hz, 0.45f * sr),
                                                        bodies[body][i].q, dbToGain (bodies[body][i].db * amt));
        const float lpHz = body == 3 ? 20000.0f - (20000.0f - 5500.0f) * amt : 20000.0f;
        *acBodyLp.state = *Coeffs::makeLowPass    (currentSampleRate, std::min (lpHz, 0.45f * sr));
        *acWarm.state   = *Coeffs::makeLowShelf   (currentSampleRate, 200.0f, 0.707f, dbToGain (param ("acWarmth")));
        *acSpark.state  = *Coeffs::makeHighShelf  (currentSampleRate, std::min (5000.0f, 0.45f * sr), 0.707f, dbToGain (param ("acSparkle")));
    }

    // Post EQ
    if (param ("eqOn") > 0.5f)
    {
        const bool  modMode = param ("eqMode") > 0.5f;
        const float qKnob   = param ("eqQ");

        const bool  steep = param ("eqSlope") > 0.5f;
        const float hpHz  = param ("eqLowCut");
        const float lpHz  = std::min (param ("eqHighCut"), 0.45f * (float) currentSampleRate);
        *eqHp1.state = *Coeffs::makeHighPass (currentSampleRate, hpHz, eqPassQ (steep, 0));
        *eqHp2.state = *Coeffs::makeHighPass (currentSampleRate, hpHz, eqPassQ (steep, 1));
        *eqLp1.state = *Coeffs::makeLowPass  (currentSampleRate, lpHz, eqPassQ (steep, 0));
        *eqLp2.state = *Coeffs::makeLowPass  (currentSampleRate, lpHz, eqPassQ (steep, 1));
        for (int b = 0; b < kEqBands; ++b)
        {
            const float gainDb = param (kEqIds[b]);
            *eqBands[b].state = *Coeffs::makePeakFilter (currentSampleRate,
                                                         std::min (kEqBandHz[b], 0.45f * (float) currentSampleRate),
                                                         eqBandQ (b, modMode, qKnob), dbToGain (gainDb));
        }
    }

    // The built-in cab model (only used when no IR is loaded)
    const int cabType = (int) param ("cabType");
    if (cabType >= 1)
    {
        const auto& cm = getCabModel (cabType);
        *cabHp.state      = *Coeffs::makeHighPass   (currentSampleRate, cm.hpHz);
        *cabLowPeak.state = *Coeffs::makePeakFilter (currentSampleRate, cm.lowHz, cm.lowQ, dbToGain (cm.lowDb));
        *cabMidPeak.state = *Coeffs::makePeakFilter (currentSampleRate, cm.midHz, cm.midQ, dbToGain (cm.midDb));
        *cabLp.state      = *Coeffs::makeLowPass    (currentSampleRate, cm.lpHz);
    }
}

//==============================================================================
void StarterAmpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    if (! midiMessages.isEmpty())
        handleMidi (midiMessages);

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // A guitar is mono. The Input setting picks which channel is used (Left, Right) or
    // adds the two (Mono), and that one signal is then sent to both channels.
    if (numChannels == 2)
    {
        const int inputMode = (int) param ("inputMode");
        if (inputMode == 1)
        {
            buffer.copyFrom (0, 0, buffer, 1, 0, numSamples);
        }
        else if (inputMode == 2)
        {
            float* l = buffer.getWritePointer (0);
            const float* r = buffer.getReadPointer (1);
            for (int i = 0; i < numSamples; ++i)
                l[i] = 0.5f * (l[i] + r[i]);
        }
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    // Riff recorder, DI mode: record the guitar here, or play the recording in as if it were the guitar
    riff.loop.store (param ("riffLoop") > 0.5f);
    if (numChannels >= 1 && riffRecMode.load() == 0)
        riff.processInput (buffer.getWritePointer (0), numChannels > 1 ? buffer.getWritePointer (1) : nullptr, numSamples, param ("riffLevel"), param ("riffMute") > 0.5f);

    // Input level meter, and the tuner's copy of the input
    if (numChannels > 0)
    {
        const float* in0 = buffer.getReadPointer (0);
        float pk = 0.0f;
        for (int i = 0; i < numSamples; ++i)
            pk = std::max (pk, std::abs (in0[i]));
        if (pk > meterIn.load (std::memory_order_relaxed))
            meterIn.store (pk, std::memory_order_relaxed);
        if (tunerActive.load (std::memory_order_relaxed))
            tunerTap.push (buffer, numSamples);
    }

    // Tempo for the delays' note values: this plugin's own tempo (tap tempo / BPM box), or the host's
    double bpm = (double) param ("tempoBpm");
    if (param ("tempoHost") > 0.5f)
        if (auto* playHead = getPlayHead())
            if (auto position = playHead->getPosition())
                if (auto hostBpm = position->getBpm())
                    bpm = *hostBpm;
    bpm = juce::jlimit (20.0, 400.0, bpm);

    // The amp quality (oversampling) setting; changing it moves the amp's sample rate and latency.
    {
        const int wanted = wantedAmpQuality();
        if (wanted != osMode)
        {
            osMode = wanted;
            osSampleRate = currentSampleRate * (osMode == 0 ? 4.0 : osMode == 1 ? 2.0 : 1.0);
            oversampler.reset();
            oversampler2.reset();
            ampB->mode = osMode;
            ampB->resetState();
            for (auto* f : { &d1.osOdTone, &d2.osOdTone, &osVoicePeak, &osAmpHighPass1, &osAmpHighPass2, &osAmpHighPass3, &osAmpLowPass2, &osAmpLowPass3, &osPresence, &osBright })
                f->reset();
            for (DriveState* d : { &d1, &d2 }) { d->osA.reset(); d->osB.reset(); d->osDryA.reset(); d->osDryB.reset(); }
            sagEnv[0] = sagEnv[1] = 0.0f;
            psChainA.reset();  psSupplyA.reset();  psFbLastA = 0.0f;
            filtersDirty = true;
            latencyDirty.store (true);
            triggerAsyncUpdate();
        }
    }

    if (paramsChanged() || filtersDirty)     // only rebuild the filters when something changed
    {
        updateFilters();
        filtersDirty = false;
    }

    // 1. Input gain
    const float inGain = dbToGain (param ("inGain"));
    buffer.applyGainRamp (0, numSamples, lastInputGain, inGain);
    lastInputGain = inGain;

    // 1b. Pickup de-noise, ahead of the gate. The gate then listens to the
    //     cleaned signal, so hum no longer holds it open. (Left channel only,
    //     then copied to the right, since the guitar is mono.)
    if (param ("deNoiseOn") > 0.5f)
    {
        deNoise.process (buffer.getWritePointer (0), numSamples,
                         (int) param ("deNoiseHz") == 1,
                         param ("deNoiseHum"), param ("deNoiseHiss"), param ("gate"));
        if (numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    // 2. Noise gate, then a high-pass to keep the lows tight
    gate.process (buffer, param ("gate"));

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    preHighPass.process (context);

    // Harmonizer: its pitch tracker listens to the clean input (before any pitch effect)
    const bool harmOn = param ("hmOn") > 0.5f;
    if (harmOn && (int) param ("hmMode") == 1)
    {
        if ((int) param ("hmTrack") > 0) harm.feedChords (buffer.getReadPointer (0), numSamples);
        else                             harm.getTracker().feed (buffer.getReadPointer (0), numSamples);
    }

    // Transpose / formant: the first effect in the chain (left channel; the guitar is mono).
    // While it is on, the wet and dry signals are both delayed by half the shifter's window, and
    // that delay is reported to the host as latency.
    {
        const float semis  = param ("tpSemis") + 0.01f * param ("tpFine");
        const bool  active = param ("tpOn") > 0.5f && (std::abs (semis) > 0.001f || fmtGainDb > 0.0f);
        // String range (v51): 7- and 8-string guitars need a longer window (and a longer search for the splice points)
        const int   tpRange = juce::jlimit (0, 2, (int) param ("tpRange"));
        const float rangeLowHz = tpRange == 0 ? 0.0f : tpRange == 1 ? 58.0f : 40.0f;
        const int   baseMs = perfMode() == 1 ? 20 : (perfMode() == 2 ? 55 : ((int) param ("tpQuality") == 1 ? 55 : 28));
        const int   windowMs = rangeLowHz > 0.0f ? std::max (baseMs, (int) std::ceil (2200.0f / rangeLowHz)) : baseMs;
        transposer.setLowHz (rangeLowHz);

        if (active && (! tpWasOn || windowMs != tpWindowMs))
        {
            transposer.setWindowMs ((float) windowMs);
            transposer.reset();
            tpWindowMs = windowMs;
        }

        const int wantedLatency = active ? transposer.latencySamples() : 0;
        if (wantedLatency != tpLatency.load())
        {
            tpLatency.store (wantedLatency);
            latencyDirty.store (true);
            triggerAsyncUpdate();
        }

        if (active)
        {
            float* x = buffer.getWritePointer (0);
            tpBuf.setSize (1, numSamples, false, false, true);
            float* wet = tpBuf.getWritePointer (0);
            std::copy (x, x + numSamples, wet);
            transposer.process (wet, x, numSamples, semis);      // wet = shifted, x = the delayed dry

            if (fmtGainDb > 0.0f)
            {
                juce::dsp::AudioBlock<float> wetBlock (tpBuf.getArrayOfWritePointers(), (size_t) 1, (size_t) numSamples);
                juce::dsp::ProcessContextReplacing<float> wetContext (wetBlock);
                fmtCut1.process (wetContext);   fmtBoost1.process (wetContext);
                fmtCut2.process (wetContext);   fmtBoost2.process (wetContext);
            }

            const float mix = param ("tpMix");
            for (int i = 0; i < numSamples; ++i)
                x[i] += mix * (wet[i] - x[i]);
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        }
        tpWasOn = active;
    }

    // Octaver (two independent octave voices added to the dry signal) and the pitch pedal (a heel-to-toe pitch
    // bend), both after the Transpose. They use the same delay-line shifter as Transpose, so each adds half its
    // window as delay (reported to the host) while it is on. Left channel, then copied to the right.
    {
        const int tpRange2 = juce::jlimit (0, 2, (int) param ("tpRange"));
        const float rangeLowHz2 = tpRange2 == 0 ? 0.0f : tpRange2 == 1 ? 58.0f : 40.0f;
        const int baseMs2 = perfMode() == 1 ? 20 : (perfMode() == 2 ? 55 : ((int) param ("tpQuality") == 1 ? 55 : 28));
        const int windowMs = rangeLowHz2 > 0.0f ? std::max (baseMs2, (int) std::ceil (2200.0f / rangeLowHz2)) : baseMs2;
        octV1.setLowHz (rangeLowHz2);  octV2.setLowHz (rangeLowHz2);  wowT.setLowHz (rangeLowHz2);
        float* x = buffer.getWritePointer (0);
        bool touched = false;

        // ---- Octaver
        const bool octActive = param ("octOn") > 0.5f && (param ("octALvl") > 0.001f || param ("octBLvl") > 0.001f);
        if (octActive && (! octWasOn || windowMs != octWindowMs))
        {
            octV1.setWindowMs ((float) windowMs);  octV2.setWindowMs ((float) windowMs);
            octV1.reset();  octV2.reset();
            octLp1 = octLp2 = 0.0f;
            octWindowMs = windowMs;
        }
        const int octLat = octActive ? octV1.latencySamples() : 0;
        if (octLat != octLatency.load())
        {
            octLatency.store (octLat);
            latencyDirty.store (true);
            triggerAsyncUpdate();
        }
        if (octActive)
        {
            static const float octSemis[4] = { -24.0f, -12.0f, 12.0f, 24.0f };
            pitchBuf1.setSize (1, numSamples, false, false, true);
            pitchBuf2.setSize (1, numSamples, false, false, true);
            pitchDry.setSize  (1, numSamples, false, false, true);
            float* w1 = pitchBuf1.getWritePointer (0);
            float* w2 = pitchBuf2.getWritePointer (0);
            float* dry = pitchDry.getWritePointer (0);
            std::copy (x, x + numSamples, w1);
            std::copy (x, x + numSamples, w2);
            octV1.process (w1, dry, numSamples, octSemis[juce::jlimit (0, 3, (int) param ("octA"))]);
            octV2.process (w2, dry, numSamples, octSemis[juce::jlimit (0, 3, (int) param ("octB"))]);

            const float fc = 800.0f * std::pow (10.0f, param ("octTone"));
            const float k  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / (float) currentSampleRate);
            const float l1 = param ("octALvl"), l2 = param ("octBLvl"), dl = param ("octDry");
            for (int i = 0; i < numSamples; ++i)
            {
                octLp1 += k * (w1[i] - octLp1);
                octLp2 += k * (w2[i] - octLp2);
                x[i] = dl * dry[i] + l1 * octLp1 + l2 * octLp2;
            }
            touched = true;
        }
        octWasOn = octActive;

        // ---- Pitch pedal
        const bool wowActive = param ("wowOn") > 0.5f;
        if (wowActive && (! wowWasOn || windowMs != wowWindowMs))
        {
            wowT.setWindowMs ((float) windowMs);
            wowT.reset();
            wowWindowMs = windowMs;
        }
        const int wowLat = wowActive ? wowT.latencySamples() : 0;
        if (wowLat != wowLatency.load())
        {
            wowLatency.store (wowLat);
            latencyDirty.store (true);
            triggerAsyncUpdate();
        }
        if (wowActive)
        {
            float heel = 0.0f, toe = 12.0f;
            bool curved = false;
            switch ((int) param ("wowMode"))
            {
                case 0: toe = 12.0f;  break;
                case 1: toe = 24.0f;  break;
                case 2: toe = -12.0f; break;
                case 3: toe = -24.0f; break;
                case 4: toe = -24.0f; curved = true; break;       // dive bomb: slow at first, then falls away
                case 5: heel = -12.0f; toe = 12.0f; break;
                default: heel = param ("wowHeel"); toe = param ("wowToe"); break;
            }
            const float pos = param ("wowPos");
            const float target = heel + (toe - heel) * (curved ? std::pow (pos, 2.2f) : pos);
            const float glideMs = param ("wowGlide");
            if (! wowWasOn || glideMs < 1.0f)
                wowSemis = target;
            else
                wowSemis += (1.0f - std::exp (-(float) numSamples / (0.001f * glideMs * (float) currentSampleRate))) * (target - wowSemis);

            pitchBuf1.setSize (1, numSamples, false, false, true);
            pitchDry.setSize  (1, numSamples, false, false, true);
            float* wet = pitchBuf1.getWritePointer (0);
            float* dry = pitchDry.getWritePointer (0);
            std::copy (x, x + numSamples, wet);
            wowT.process (wet, dry, numSamples, wowSemis);
            const float mix = param ("wowMix");
            for (int i = 0; i < numSamples; ++i)
                x[i] = dry[i] + mix * (wet[i] - dry[i]);
            touched = true;
        }
        wowWasOn = wowActive;

        if (touched && numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    // Harmonizer: two voices that follow the note you play. Placed before the amp (mono, so the amp distorts the
    // harmony along with the note) or after the cab (stereo, with a pan for each voice).
    auto runHarmonizer = [&] (int position)
    {
        const int pos = juce::jlimit (0, 1, (int) param ("hmPos"));
        const bool active = harmOn && pos == position;
        if (position == 0 && pos != harmPosPrev && harmPosPrev >= 0) harmWasOn = false;     // moved: start clean
        if (position == 0) harmPosPrev = pos;
        if (! active)
        {
            if (position == pos && harmLatency.load() != 0) { harmLatency.store (0); latencyDirty.store (true); triggerAsyncUpdate(); }
            if (position == pos) harmWasOn = false;
            return;
        }
        const int tpR = juce::jlimit (0, 2, (int) param ("tpRange"));
        const float lowHz = tpR == 0 ? 0.0f : tpR == 1 ? 58.0f : 40.0f;
        const int baseMs = perfMode() == 1 ? 20 : (perfMode() == 2 ? 55 : ((int) param ("tpQuality") == 1 ? 55 : 28));
        const int winMs = lowHz > 0.0f ? std::max (baseMs, (int) std::ceil (2200.0f / lowHz)) : baseMs;
        // v53: Chords tracking uses the spectral shifter (Chords = long window, Chords (fast) = short window)
        const int track = juce::jlimit (0, 2, (int) param ("hmTrack"));
        const int cfgKey = track == 0 ? winMs : 100000 + track;
        if (! harmWasOn || cfgKey != harmWindowMs)
        {
            harm.setRange (lowHz, (float) winMs);
            harm.setPoly (track > 0, track == 1);
            harm.reset();
            harmWindowMs = cfgKey;
        }
        harmWasOn = true;
        const int lat = harm.latencySamples();
        if (lat != harmLatency.load()) { harmLatency.store (lat); latencyDirty.store (true); triggerAsyncUpdate(); }

        HarmSettings hs;
        hs.scaleMode = (int) param ("hmMode") == 1;
        hs.poly = track > 0;
        hs.key = (int) param ("hmKey");  hs.scale = (int) param ("hmScale");
        hs.semisA = param ("hmAInt");  hs.semisB = param ("hmBInt");
        hs.degA = (int) std::lround (param ("hmADeg"));  hs.degB = (int) std::lround (param ("hmBDeg"));
        hs.lvlA = param ("hmALvl");  hs.lvlB = param ("hmBLvl");
        hs.panA = param ("hmAPan");  hs.panB = param ("hmBPan");
        hs.dry = param ("hmDry");  hs.tone = param ("hmTone");  hs.glideMs = param ("hmGlide");
        hs.noteOffset = param ("tpOn") > 0.5f ? param ("tpSemis") + 0.01f * param ("tpFine") : 0.0f;
        const bool stereo = position == 1 && numChannels == 2;
        harm.process (buffer.getWritePointer (0), stereo ? buffer.getWritePointer (1) : nullptr, harmScratch, numSamples, hs);
        if (! stereo && numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    };
    runHarmonizer (0);

    // Noise pedal: runs on the left channel, then copied to the right
    // (the guitar is mono). With both switches up it is a plain pass-through.
    // The NOISE PEDAL ON switch releases all four layers (they fade out as if you let go).
    const bool noiseOn = param ("noiseOn") > 0.5f;
    noise.process (buffer.getWritePointer (0), numSamples,
                   noiseOn && param ("noiseOct") > 0.5f, noiseOn && param ("noiseOct2") > 0.5f,
                   param ("noiseRise"), param ("noiseSpeed"),
                   param ("noisePanic"), param ("noiseChaos"));
    gross.process (buffer.getWritePointer (0), numSamples,
                   noiseOn && param ("noiseGross") > 0.5f, (int) param ("noiseGrossScene"),
                   param ("noiseNuke"), param ("noiseSicken"),
                   param ("noiseRise"), param ("noiseSpeed"));
    ring.process (buffer.getWritePointer (0), numSamples,
                  noiseOn && param ("noiseRing") > 0.5f, (int) param ("ringMode"), (int) param ("ringWave"),
                  param ("ringFreq"), param ("ringMix"), param ("ringWobble"),
                  param ("noiseRise"), param ("noiseSpeed"));
    if (numChannels == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);

    auto runFuzz = [&]()
    {
        // Fuzz pedals (Fuzz 1 and, since v93, Fuzz 2). Left channel only, copied to the right. Skipped entirely when switched off.
        const bool on1 = param ("fuzzOn") > 0.5f, on2 = param ("fuzz2On") > 0.5f;
        if (! on1 && ! on2) return;
        auto runOne = [&] (int which, float* data)
        {
            FuzzEffect& fx = which == 0 ? fuzz : fuzz2;
            AutoLeveler& lev = which == 0 ? fuzzLeveler : fuzzLeveler2;
            const juce::String pre = which == 0 ? "fuzz" : "fuzz2";
            auto P = [&] (const char* n) { return param ((pre + n).toRawUTF8()); };
            const float bzLow  = which == 0 ? param ("buzzLow")  : param ("fuzz2BuzzLow");
            const float bzHigh = which == 0 ? param ("buzzHigh") : param ("fuzz2BuzzHigh");
            auto BZ = [&] (const char* n) { return param ((juce::String (which == 0 ? "buzz" : "fuzz2Buzz") + n).toRawUTF8()); };
            const bool fuzzAuto = P ("Auto") > 0.5f;
            if (fuzzAuto)
            {
                if ((int) autoRef.size() < numSamples) autoRef.resize ((size_t) numSamples);
                std::copy (data, data + numSamples, autoRef.begin());
            }
            fx.process (data, numSamples,
                        FuzzEffect::Params { (int) P ("Style"), P ("Gain"), P ("Tone"),
                                             fuzzAuto ? 1.0f : dbToGain (P ("Level")), P ("Gate"), P ("Stab"),
                                             bzLow, bzHigh, wantedFuzzStages(), P ("Mix"),
                                             P ("Ge") > 0.5f, P ("Starve"), P ("Input"), P ("Comp"),
                                             (int) P ("BenderModel"), P ("Buffer") > 0.5f,
                                             (int) P ("MuffVer"), (int) P ("MuffDiode"),
                                             P ("MuffOpAmp") > 0.5f, P ("MuffTonePass") > 0.5f, P ("MuffMids"),
                                             P ("Drift"), P ("Hiss"), P ("Octave") > 0.5f,
                                             P ("Shape"), P ("Crush"), P ("Bits"), P ("Rate"),
                                             BZ ("Mid"), BZ ("Soft") > 0.5f, BZ ("Hard") > 0.5f, BZ ("Blend"), BZ ("Pol") > 0.5f });
            if (fuzzAuto)
            {
                float* fz[1] = { data };
                lev.process (autoRef.data(), fz, 1, numSamples, dbToGain (P ("Level") + 9.0f));   // Level knob is relative to its default (-9 dB)
            }
            else
                lev.reset();
        };
        if (on1 && on2 && (int) param ("fuzz2Route") == 1)
        {
            // Parallel: both fuzzes get the same input; outputs are added (-3 dB each)
            fuzzPar.setSize (1, numSamples, false, false, true);
            fuzzPar.copyFrom (0, 0, buffer, 0, 0, numSamples);
            runOne (0, buffer.getWritePointer (0));
            runOne (1, fuzzPar.getWritePointer (0));
            buffer.addFrom (0, 0, fuzzPar, 0, 0, numSamples);
            buffer.applyGain (0, 0, numSamples, 0.7071f);
        }
        else
        {
            if (on1) runOne (0, buffer.getWritePointer (0));
            if (on2) runOne (1, buffer.getWritePointer (0));      // series (or Fuzz 2 on its own)
        }
        if (numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    };

    auto runComp = [&]()
    {
        // Compressor, after the fuzz and before the pre-amp delay, overdrive
        // and amp. Left channel only, copied to the right. The meter shows 0 when it is off.
        if (param ("compOn") > 0.5f)
        {
            if ((int) param ("compStyle") == 6)
            {
                // v93: the Dyna-type compressor (Circuit::DynaComp). Threshold sets the squeeze (lower = more), the other Comp knobs act as named.
                const float gr = dyna.process (buffer.getWritePointer (0), numSamples, juce::jlimit (0.0f, 1.0f, -param ("compThresh") / 40.0f),
                                               param ("compAttack"), param ("compRelease"), param ("compOutput"), param ("compMix"));
                if (numChannels == 2)
                    buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
                compGrDb.store (gr);
            }
            else
            {
            comp.process (buffer.getWritePointer (0), numSamples,
                          CompressorEffect::Params { param ("compThresh"), (int) param ("compRatio"),
                                                     param ("compAttack"), param ("compRelease"),
                                                     param ("compOutput") + (param ("compAuto") > 0.5f ? autoCompMakeupDb (param ("compThresh"), (int) param ("compRatio"), param ("compMix")) : 0.0f), param ("compMix"),
                                                     (int) param ("compDetHp"), (int) param ("compSat"),
                                                     param ("compBloom"), param ("compSmack"), param ("compClarity"),
                                                     (int) param ("compStyle") });
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
            compGrDb.store (comp.getGainReductionDb());
            }
        }
        else
        {
            compGrDb.store (0.0f);
        }

        // Swell: find the start of each note here (before the amp); the fade-in is applied after the Post EQ.
        {
            const bool sOn = param ("swellOn") > 0.5f;
            if (sOn)
            {
                if (! swellWasOn) swell.reset();
                if ((int) swellGains.size() < numSamples) swellGains.resize ((size_t) numSamples, 1.0f);
                swell.analyse (buffer.getReadPointer (0), numSamples, syncedMs ("swellNote", "swellAttack", bpm, 4000.0f), param ("swellSens"), swellGains);
            }
            swellWasOn = sOn;
        }
    };

    auto runWah = [&]()
    {
        // Wah: after the compressor, before the phaser, chorus, overdrive and amp.
        {
            const bool wOn = param ("wahOn") > 0.5f;
            if (wOn)
            {
                if (! wahWasOn) wah.reset();
                wah.process (buffer.getWritePointer (0), numSamples,
                             Wah::Params { (int) param ("wahMode"), param ("wahPos"), param ("wahVoice") > 0.5f ? 450.0f : param ("wahHeel"), param ("wahVoice") > 0.5f ? 1600.0f : param ("wahToe"),
                                           param ("wahQ"), param ("wahMix"), param ("wahSens"), param ("wahDecay"),
                                           std::min (20.0f, lfoRateHz ("wahNote", "wahRate", bpm)), param ("wahDepth") });
                if (numChannels == 2)
                    buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
            }
            wahWasOn = wOn;
        }
    };

    // Phaser, then chorus (default order): both before the overdrive and amp, so they get driven by it.
    // Left channel only (the guitar is mono), copied to the right.
    auto runPhaser = [&]()
    {
        const bool phOn = param ("phaserOn") > 0.5f;
        if (phOn)
        {
            if (! phaserWasOn) phaser.reset();
            phaser.process (buffer.getWritePointer (0), numSamples,
                            Phaser::Params { 2 * ((int) param ("phaserStages") + 1), (int) param ("phaserShape"),
                                             std::min (20.0f, lfoRateHz ("phaserNote", "phaserRate", bpm)), param ("phaserDepth"), param ("phaserCenter"),
                                             param ("phaserFeedback"), param ("phaserMix"), param ("phaserEnv") });
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        }
        phaserWasOn = phOn;
    };

    auto runChorus = [&]()
    {
        const bool chOn = param ("chorusOn") > 0.5f;
        if (chOn)
        {
            if (! chorusWasOn) chorus.reset();
            chorus.process (buffer.getWritePointer (0), numSamples,
                            BbdChorus::Params { (int) param ("chorusMode"), param ("chorusMix"), param ("chorusRate"),
                                                 param ("chorusDepth"), param ("chorusTone"), param ("chorusHiss") });
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        }
        chorusWasOn = chOn;
    };

    auto runPreDelay = [&]()
    {
        // Pre-amp delay: its repeats go through the overdrive and preamp, so they
        // compress and distort along with your playing. It has its own settings,
        // separate from the post-amp delay further down.
        delayPre.process (buffer, DelayParams { delayTimeMs ("preDelayNote", "preDelayTime", bpm), param ("preDelayFeedback"),
                                                param ("preDelayOn") > 0.5f ? param ("preDelayMix") : 0.0f,
                                                param ("preDelayLofi"), param ("preDelayWarble"), (int) param ("preDelayMode"),
                                                0, 0.5f, 1.0f, 0,
                                                param ("preDelayDuck"), param ("preDelayDuckRel"), param ("preDelayModDepth"), param ("preDelayModRate"),
                                                param ("preDelayDiffuse"), param ("preDelayHold") > 0.5f, (int) param ("preDelayHoldMode"),
                                                (int) param ("preDelayTaps"), param ("preDelayTapLevel") });
    };

    // The pre-amp effects run in the order chosen on the chain strip (default: fuzz, compressor, wah,
    // phaser, chorus, delay). Each keeps its own state, so reordering never loses anything.
    {
        const auto order = getFxOrder (0);
        for (int k = 0; k < kFxBlocks; ++k)
            switch (order[(size_t) k])
            {
                case 0: runFuzz();     break;
                case 1: runComp();     break;
                case 2: runWah();      break;
                case 3: runPhaser();   break;
                case 4: runChorus();   break;
                default: runPreDelay(); break;
            }
    }

    // Modulation slots (v51) set to run before the amp: after the reorderable effects
    bool modActive[2];
    for (int s = 0; s < 2; ++s)
    {
        modActive[s] = param (s == 0 ? "mfAOn" : "mfBOn") > 0.5f;
        if (modActive[s] && ! modWasOn[s]) modFx[s].reset();
        modWasOn[s] = modActive[s];
    }
    auto runModFx = [&] (int s, bool post)
    {
        static const char* const ids[2][11] = {
            { "mfAOn", "mfAType", "mfAPos", "mfAShape", "mfARate", "mfANote", "mfADepth", "mfAFeedback", "mfAManual", "mfATone", "mfAMix" },
            { "mfBOn", "mfBType", "mfBPos", "mfBShape", "mfBRate", "mfBNote", "mfBDepth", "mfBFeedback", "mfBManual", "mfBTone", "mfBMix" } };
        if (! modActive[s] || (((int) param (ids[s][2]) == 1) != post)) return;
        ModFx::Params mp;
        mp.type = juce::jlimit (0, 6, (int) param (ids[s][1]));
        mp.shape = juce::jlimit (0, 2, (int) param (ids[s][3]));
        mp.rateHz = juce::jlimit (0.02f, 20.0f, lfoRateHz (ids[s][5], ids[s][4], bpm));
        mp.depth = param (ids[s][6]);  mp.feedback = param (ids[s][7]);  mp.manual = param (ids[s][8]);
        mp.toneHz = param (ids[s][9]);  mp.mix = param (ids[s][10]);
        float* l = buffer.getWritePointer (0);
        float* r = (post && numChannels == 2) ? buffer.getWritePointer (1) : nullptr;
        modFx[s].process (l, r, numSamples, mp);
        if (! post && numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    };
    runModFx (0, false);
    runModFx (1, false);

    // Lo-Fi Warble (v50): a fixed stage after the reorderable effects, just before the overdrive
    {
        const bool lfOn = param ("lfOn") > 0.5f;
        if (lfOn)
        {
            if (! lofiWasOn) lofi.reset();
            lofi.process (buffer.getWritePointer (0), numSamples,
                          LoFiWarble::Params { (int) param ("lfShape"), param ("lfRate"), param ("lfDepth"), param ("lfDrift"), param ("lfTone"),
                                               param ("lfDirt"), param ("lfNoise"), param ("lfDropouts"), param ("lfMix"), param ("lfLevel") });
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        }
        lofiWasOn = lfOn;
    }

    // Boost (v51): tighten, parametric EQ and saturation, just before the overdrive and the amp
    {
        const bool bsOn = param ("bsOn") > 0.5f;
        if (bsOn)
        {
            if (! boostWasOn) boost.reset();
            BoostStage::Params bp;
            bp.mode = juce::jlimit (0, 6, (int) param ("bsMode"));
            bp.gainDb = param ("bsGain");  bp.sat = param ("bsSat");  bp.tight = param ("bsTight");
            bp.tightStyle = juce::jlimit (0, 2, (int) param ("bsTightStyle"));
            bp.levelDb = param ("bsLevel");
            bp.eqOn = param ("bsEqOn") > 0.5f;  bp.eqPos = (int) param ("bsEqPos");
            bp.lo = { param ("bsLoHz"), param ("bsLoDb"), 0.7f };
            bp.m1 = { param ("bsM1Hz"), param ("bsM1Db"), param ("bsM1Q") };
            bp.m2 = { param ("bsM2Hz"), param ("bsM2Db"), param ("bsM2Q") };
            bp.hi = { param ("bsHiHz"), param ("bsHiDb"), 0.7f };
            const float bsMix = param ("bsMix");
            if (bsMix < 0.999f)
            {
                boostDry.setSize (1, numSamples, false, false, true);
                boostDry.copyFrom (0, 0, buffer, 0, 0, numSamples);
            }
            boost.process (buffer.getWritePointer (0), numSamples, bp);
            if (bsMix < 0.999f)
            {
                float* w = buffer.getWritePointer (0);  const float* dr = boostDry.getReadPointer (0);
                for (int i = 0; i < numSamples; ++i) w[i] = dr[i] + bsMix * (w[i] - dr[i]);
            }
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        }
        boostWasOn = bsOn;
    }

    // NAM pedal 1 (v51): a neural capture of a pedal (or anything else) in front of the overdrive and the amp
    {
        namSlot[2].pickUp();
        const bool touched = namSlot[2].processPedal (buffer.getWritePointer (0), nullptr, numSamples,
                                                      NamSlot::PedalSettings { param ("nmP1On") > 0.5f, param ("nmP1In"), param ("nmP1Out"),
                                                                               param ("nmP1Mix"), param ("nmP1Norm") > 0.5f, false });
        if (touched && numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    // 3. Transparent overdrive, linear half: clean path + gain path, summed.
    //    (The clipping itself happens below, at the higher sample rate.)
    resolveDriveRoute();
    const bool odOn = frontOn;
    DriveState& dF = frontDrive == 0 ? d1 : d2;
    const char* frPre = frontDrive == 0 ? "od" : "od2";
    const int odStyle = driveCode (frontDrive);
    if (parallelBoth)
    {
        // Both drives side by side from the same input, then summed (-3 dB each so the sum stays near unity)
        drivePar.setSize (1, numSamples, false, false, true);
        drivePar.copyFrom (0, 0, buffer, 0, 0, numSamples);
        runDriveStandalone (0, buffer, numSamples);
        runDriveStandalone (1, drivePar, numSamples);
        buffer.addFrom (0, 0, drivePar, 0, 0, numSamples);
        buffer.applyGain (0, 0, numSamples, 0.7071f);
    }
    else if (seriesBoth)
        runDriveStandalone (0, buffer, numSamples);       // Drive 1 runs on its own; Drive 2 is the one built into the amp's oversampled path
    if (odOn)
    {
        bool circUnused = false;
        runDriveLinear (frontDrive, buffer, numSamples, circUnused);
    }

    // The mono block the amp and tone stack work on
    auto ampBlock = block.getSingleChannelBlock (0);
    juce::dsp::ProcessContextReplacing<float> ampContext (ampBlock);

    // 4. Everything non-linear runs at 4x the sample rate (less aliasing).
    const float ampGain  = param ("ampGain");
    const auto& voicing  = ampVoicingFor (apvts, false);
    const float g1       = stageGain (voicing, 0, ampGain);
    const float g2       = stageGain (voicing, 1, ampGain);
    const float g3       = stageGain (voicing, 2, ampGain);
    const float odVolume = dbToGain (odVolumeDbEff (apvts, frPre));

    const bool acoustic = (int) param ("ampModel") == 5;

    // Parallel Amp B: same input as Amp A, processed first into its own buffer.
    const bool ampBActive = param ("ampBOn") > 0.5f && ! acoustic;
    if (ampBActive)
    {
        ampBBuf.setSize (1, numSamples, false, false, true);
        ampBBuf.copyFrom (0, 0, buffer, 0, 0, numSamples);
        namSlot[1].pickUp();
        const bool namBActive = param ("namBOn") > 0.5f && namSlot[1].cur[0] != nullptr;
        if (namBActive)
        {
            ampB->processNam (ampBBuf.getWritePointer (0), numSamples, odOn && param ("ampBUseOd") > 0.5f, odStyle, namSlot[1],
                              dbToGain (param ("namBIn")),
                              dbToGain (param ("namBOut") + (param ("namBNorm") > 0.5f ? namSlot[1].normDb.load() : 0.0f)));
        }
        else
            ampB->process (ampBBuf.getWritePointer (0), numSamples, odOn && param ("ampBUseOd") > 0.5f, odStyle);
        if (namBActive != namSlot[1].wasActive)
            ampBBuf.applyGainRamp (0, 0, numSamples, 0.0f, 1.0f);       // no click when Amp B switches between the capture and the built-in preamp
        namSlot[1].wasActive = namBActive;
        ampBWasOn = true;
    }
    else if (ampBWasOn)
    {
        ampB->resetState();
        ampBWasOn = false;
        namSlot[1].wasActive = false;
    }

    // NAM model (v50): a new model handed over from the message thread is picked up here (a pointer swap, no allocation)
    namSlot[0].pickUp();
    NamSlot& namA = namSlot[0];
    const bool namActive = ! acoustic && param ("namOn") > 0.5f && namA.cur[0] != nullptr;
    const bool circuitActive = false;                 // v93: the full-circuit amp mode is gone; circuit-derived is the only mode
    const bool circuitMode = (int) param ("ampMode") == 1;
    const auto shapePlan = circuitMode && ! acoustic && ! namActive
        ? StageShaper::planFor ((int) param ("ampModel"), (int) param ("ampTube"), (int) param ("ampPwrTube")) : StageShaper::Plan();
    if (! shapePlan.active) for (auto& s : stageSt) s.shift = 0.0f;

    if (acoustic)
    {
        if (numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        processAcoustic (buffer, context, numChannels, numSamples);
    }
    else if (namActive)
    {
        // The neural capture stands in for the preamp and power stage. It runs at its own sample rate (usually 48 kHz),
        // so the amp's oversampling is not used for it. The overdrive pedal's clipping and tone come first (v51: run at
        // the oversampled rate, like they are for the built-in preamp), and its Volume drives the capture.
        float* x = buffer.getWritePointer (0);
        bool viaOs = false;
        if (odOn)
        {
            auto& activeOs = (osMode == 0) ? oversampler : oversampler2;
            auto osB = (osMode == 2) ? ampBlock : activeOs.processSamplesUp (ampBlock);
            const int m = (int) osB.getNumSamples();
            float* y = osB.getChannelPointer (0);
            const auto clip = odClipFor (odStyle);
            for (int i = 0; i < m; ++i) y[i] = clip (y[i]);
            juce::dsp::ProcessContextReplacing<float> osCtx (osB);
            dF.osOdTone.process (osCtx);
            blendDriveDry (dF, y, m, numSamples, param (frontDrive == 0 ? "odMix" : "od2Mix"));
            const float step = (odVolume - dF.lastOdVolume) / (float) m;
            float g = dF.lastOdVolume;
            for (int i = 0; i < m; ++i) { y[i] *= g; g += step; }
            dF.lastOdVolume = odVolume;
            if (osMode != 2) { activeOs.processSamplesDown (ampBlock); viaOs = true; }
        }
        const float inG  = dbToGain (param ("namIn"));
        const float outG = dbToGain (param ("namOut") + (param ("namNorm") > 0.5f ? namA.normDb.load() : 0.0f));
        buffer.applyGainRamp (0, 0, numSamples, namA.lastIn, inG);
        namA.lastIn = inG;
        namA.cur[0]->process (x, numSamples);
        for (int i = 0; i < numSamples; ++i)
            x[i] = std::isfinite (x[i]) ? juce::jlimit (-16.0f, 16.0f, x[i]) : 0.0f;
        buffer.applyGainRamp (0, 0, numSamples, namA.lastOut, outG);
        namA.lastOut = outG;

        // The built-in amp reports the oversampler's delay to the host; match it so switching NAM on does not shift the timing
        if (! viaOs)
        {
            const int lat = osMode == 0 ? (int) std::lround (oversampler.getLatencyInSamples())
                          : osMode == 1 ? (int) std::lround (oversampler2.getLatencyInSamples()) : 0;
            namA.delayInPlace (x, numSamples, lat);
        }
        if (! namA.wasActive)
            buffer.applyGainRamp (0, 0, numSamples, 0.0f, 1.0f);          // fade in: no click when switching to the capture
        dcBlock.process (ampContext);
    }
    else if (circuitActive)
    {
        // v79: a whole-amp circuit model (preamp, tone stack, phase inverter, power tubes, output transformer, supply).
        // The pedal in front runs at the chosen oversampling, then the circuit runs at twice the host rate.
        float* x = buffer.getWritePointer (0);
        bool viaOs = false;
        if (odOn)
        {
            auto& activeOs = (osMode == 0) ? oversampler : oversampler2;
            auto osB = (osMode == 2) ? ampBlock : activeOs.processSamplesUp (ampBlock);
            const int m = (int) osB.getNumSamples();
            float* y = osB.getChannelPointer (0);
            const auto clip = odClipFor (odStyle);
            for (int i = 0; i < m; ++i) y[i] = clip (y[i]);
            juce::dsp::ProcessContextReplacing<float> osCtx (osB);
            dF.osOdTone.process (osCtx);
            blendDriveDry (dF, y, m, numSamples, param (frontDrive == 0 ? "odMix" : "od2Mix"));
            const float step = (odVolume - dF.lastOdVolume) / (float) m;
            float g = dF.lastOdVolume;
            for (int i = 0; i < m; ++i) { y[i] *= g; g += step; }
            dF.lastOdVolume = odVolume;
            if (osMode != 2) { activeOs.processSamplesDown (ampBlock); viaOs = true; }
        }
        if (! circuitWasActive)
            circuitA->reset();
        {
            auto knob = [this] (const char* id) { return juce::jlimit (0.0f, 1.0f, (param (id) + 12.0f) / 24.0f); };
            Tube::AmpControls cc;
            cc.gain = param ("ampGain");
            cc.bass = knob ("bass");
            cc.mid = 0.5f * (knob ("lowMid") + knob ("highMid"));
            cc.treble = knob ("treble");
            cc.master = param ("ampMaster");
            cc.presence = param ("presence");
            cc.bias = param ("ampBias");
            cc.sag = param ("ampSagAmt");
            cc.preTube = (int) param ("ampTube");
            cc.pwrTube = (int) param ("ampPwrTube");
            cc.siliconRect = false;
            circuitA->configure (cc);
        }
        {
            auto osB = oversampler2.processSamplesUp (ampBlock);
            float* y = osB.getChannelPointer (0);
            const int m = (int) osB.getNumSamples();
            const double outScale = 0.7 / Tube::LeadFiftyCircuit::kPeakFull;
            for (int i = 0; i < m; ++i)
            {
                const double o = circuitA->process ((double) y[i]) * outScale;
                y[i] = std::isfinite (o) ? (float) juce::jlimit (-4.0, 4.0, o) : 0.0f;
            }
            oversampler2.processSamplesDown (ampBlock);
        }
        {
            const int want = osMode == 0 ? (int) std::lround (oversampler.getLatencyInSamples())
                           : osMode == 1 ? (int) std::lround (oversampler2.getLatencyInSamples()) : 0;
            const int have = (int) std::lround (oversampler2.getLatencyInSamples()) + (viaOs ? want : 0);
            namSlot[0].delayInPlace (x, numSamples, want - have);
        }
        if (! circuitWasActive)
            buffer.applyGainRamp (0, 0, numSamples, 0.0f, 1.0f);
        dcBlock.process (ampContext);
    }
    else
    {
    auto& activeOs = (osMode == 0) ? oversampler : oversampler2;
    auto osBlock = (osMode == 2) ? ampBlock : activeOs.processSamplesUp (ampBlock);
    const int    osSamples  = (int) osBlock.getNumSamples();
    const size_t osChannels = osBlock.getNumChannels();
    juce::dsp::ProcessContextReplacing<float> osContext (osBlock);

    // Runs fn(gain, sample) on every oversampled sample, sliding the gain
    // from 'from' to 'to' across the block.
    auto shape = [&] (float from, float to, auto fn)
    {
        const float step = (to - from) / (float) osSamples;
        for (size_t ch = 0; ch < osChannels; ++ch)
        {
            float* x = osBlock.getChannelPointer (ch);
            float g = from;
            for (int i = 0; i < osSamples; ++i)
            {
                x[i] = fn (g, x[i]);
                g += step;
            }
        }
    };

    if (odOn)
    {
        // Diode-style clipper: nearly linear for small signals, then it
        // bends over smoothly. 'k' sets where the bend starts.
        const auto clip = odClipFor (odStyle);
        shape (1.0f, 1.0f, [clip] (float, float s) { return clip (s); });

        dF.osOdTone.process (osContext);
        blendDriveDry (dF, osBlock.getChannelPointer (0), osSamples, numSamples, param (frontDrive == 0 ? "odMix" : "od2Mix"));
        shape (dF.lastOdVolume, odVolume, [] (float g, float s) { return g * s; });
        dF.lastOdVolume = odVolume;
    }

    // Preamp: two or three cascaded gain stages, each preceded by a high-pass.
    // 'bias' shifts the tanh curve a little so the clipping is asymmetric,
    // which adds even harmonics and a more "tube-like" feel.
    if (param ("ampBright") > 0.5f)
        osBright.process (osContext);
    osVoicePeak.process (osContext);

    auto stage = [&] (StereoFilter& highPass, float from, float to, float bias)
    {
        highPass.process (osContext);
        const float offset = fastTanh (bias);
        shape (from, to, [bias, offset] (float g, float s) { return fastTanh (g * s + bias) - offset; });
    };

    // v80: stage shapes taken from tube-circuit simulation (asymmetric, with grid blocking) instead of the plain tanh
    const float blockAttack  = 1.0f - std::exp (-1.0f / (0.0015f * (float) osSampleRate));
    const float blockRelease = 1.0f - std::exp (-1.0f / (0.045f  * (float) osSampleRate));
    const float biasShift = 0.3f * param ("ampBias");
    auto stageM = [&] (StereoFilter& highPass, float from, float to, int idx)
    {
        highPass.process (osContext);
        const float* curve = shapePlan.pre[idx];
        auto& st = stageSt[idx];
        float* x = osBlock.getChannelPointer (0);
        const float step = (to - from) / (float) osSamples;
        float g = from;
        for (int i = 0; i < osSamples; ++i)
        {
            x[i] = st.process (curve, g * x[i] + biasShift, blockAttack, blockRelease);
            g += step;
        }
    };

    const bool stageLp = voicing.stageLpHz > 0.0f;
    if (shapePlan.active)
    {
        stageM (osAmpHighPass1, lastAmpGain1, g1, 0);
        if (stageLp) osAmpLowPass2.process (osContext);
        stageM (osAmpHighPass2, lastAmpGain2, g2, 1);
        if (voicing.stages >= 3)
        {
            if (stageLp) osAmpLowPass3.process (osContext);
            stageM (osAmpHighPass3, lastAmpGain3, g3, 2);
        }
    }
    else
    {
    stage (osAmpHighPass1, lastAmpGain1, g1, voicing.bias[0]);
    if (stageLp) osAmpLowPass2.process (osContext);
    stage (osAmpHighPass2, lastAmpGain2, g2, voicing.bias[1]);
    if (voicing.stages >= 3)
    {
        if (stageLp) osAmpLowPass3.process (osContext);
        stage (osAmpHighPass3, lastAmpGain3, g3, voicing.bias[2]);
    }
    }

    // Power-amp sag: loud passages squash the level a little, then recover.
    if (voicing.sag > 0.0f)
    {
        const float release = std::exp (-1.0f / (0.12f * (float) osSampleRate));
        for (size_t ch = 0; ch < osChannels && ch < 2; ++ch)
        {
            float* x = osBlock.getChannelPointer (ch);
            float env = sagEnv[ch];
            for (int i = 0; i < osSamples; ++i)
            {
                const float a = std::abs (x[i]);
                env = (a > env) ? a : a + release * (env - a);
                x[i] *= 1.0f / (1.0f + voicing.sag * env);
            }
            sagEnv[ch] = env;
        }
    }

    // v83: supply droop with two time constants (replaces the single classic one when chosen)
    if (psParamsA.supplyOn)
        for (size_t ch = 0; ch < osChannels && ch < 2; ++ch)
            psSupplyA.droop (osBlock.getChannelPointer (ch), osSamples, (int) ch, psParamsA, osSampleRate);

    // Power stage: saturation and (for a cold bias) crossover distortion
    {
        const float pw = effectivePower (param ("ampPower"), voicing), bi = param ("ampBias");
        const float dz = bi < 0.0f ? -0.12f * bi : 0.0f;
        if (pw > 0.0f || dz > 0.0f)
        {
            if (psParamsA.fbLg > 0.0f)
            {
                // v86: the output stage inside its global feedback loop
                const float comp = std::sqrt (1.0f + 5.0f * pw * pw), hotAmt = juce::jmax (0.0f, bi);
                float* px = osBlock.getChannelPointer (0);
                if (shapePlan.active)
                    PowerSection::feedbackStage (px, osSamples, psParamsA.fbLg, comp, psFbLastA,
                                                 [&] (float s) { return StageShaper::powerSample (s, pw, dz, shapePlan.pwrMed, shapePlan.pwrHot, hotAmt); });
                else
                    PowerSection::feedbackStage (px, osSamples, psParamsA.fbLg, comp, psFbLastA, [&] (float s) { return powerSample (s, pw, dz); });
            }
            else if (shapePlan.active)
                StageShaper::powerStage (osBlock.getChannelPointer (0), osSamples, pw, dz, shapePlan.pwrMed, shapePlan.pwrHot, juce::jmax (0.0f, bi));
            else
                powerStage (osBlock.getChannelPointer (0), osSamples, pw, dz);
        }
    }

    // Output transformer (v83), then presence: the shelf, or the feedback network when the power section is derived
    if (psParamsA.supplyOn)
        for (size_t ch = 0; ch < osChannels && ch < 2; ++ch)
            psSupplyA.transformer (osBlock.getChannelPointer (ch), osSamples, (int) ch, psParamsA, osSampleRate);
    if (psParamsA.filtersOn)
    {
        for (size_t ch = 0; ch < osChannels && ch < 2; ++ch)
            psChainA.process (osBlock.getChannelPointer (ch), osSamples, (int) ch);
    }
    else
        osPresence.process (osContext);

    lastAmpGain1 = g1;
    lastAmpGain2 = g2;
    lastAmpGain3 = g3;

    if (osMode != 2)
        activeOs.processSamplesDown (ampBlock);
    dcBlock.process (ampContext);
    }   // end of the electric amp
    if (namA.wasActive && ! namActive && ! acoustic)
        buffer.applyGainRamp (0, 0, numSamples, 0.0f, 1.0f);            // fade back in when leaving the capture
    namA.wasActive = namActive;
    if (circuitWasActive && ! circuitActive && ! acoustic)
        buffer.applyGainRamp (0, 0, numSamples, 0.0f, 1.0f);
    circuitWasActive = circuitActive;

    // 5. Tone stack, then a level trim so the three amps sit at similar volume
    if (! acoustic)
    {
        const int  ampIdx  = (int) param ("ampModel");
        const bool metalOn = hasMetalControls (ampIdx) && ! namActive && ! circuitActive;
        if (metalOn)
        {
            if (param ("mtFizz")  > 0.01f) metalFizz.process (ampContext);
            if (param ("mtDepth") > 0.01f) metalDepth.process (ampContext);
        }
        if (param ("ampReso") > 0.01f && ! namActive && ! circuitActive)
            ampResoPk.process (ampContext);
        const int stackSel = (int) param ("ampStack");
        if (circuitActive)
        {
            // the circuit has its own tone stack
        }
        else if (circuitMode && stackSel == 0 && ampIdx == 9)     // v93 (circuit-derived mode only): the 5E3's single Tone control (Treble), then Bass and Mid on top as before
        {
            auto pos = [this] (const char* id) { return juce::jlimit (0.0f, 1.0f, (param (id) + 12.0f) / 24.0f); };
            tweedA.set (pos ("treble"), currentSampleRate);
            tweedA.process (ampBlock.getChannelPointer (0), numSamples);
            bassShelf.process (ampContext);
            lowMidPeak.process (ampContext);
            highMidPeak.process (ampContext);
        }
        else if (stackSel >= 2)     // v81: a real passive bass / mid / treble stack solved from its component values
        {
            auto pos = [this] (const char* id) { return juce::jlimit (0.0f, 1.0f, (param (id) + 12.0f) / 24.0f); };
            passiveA.set (stackSel - 2, pos ("treble"), pos ("bass"), 0.5f * (pos ("lowMid") + pos ("highMid")), currentSampleRate);
            passiveA.process (ampBlock.getChannelPointer (0), numSamples);
        }
        else
        {
            bassShelf.process (ampContext);
            lowMidPeak.process (ampContext);
            highMidPeak.process (ampContext);
            trebleShelf.process (ampContext);
        }
    }
    else
    {
        bassShelf.process (context);
        lowMidPeak.process (context);
        highMidPeak.process (context);
        trebleShelf.process (context);
    }

    const float ampTrim = (namActive || circuitActive) ? 1.0f : dbToGain (voicing.trimDb + (shapePlan.active ? shapePlan.trimDb : 0.0f)
        + (param ("ampAutoLevel") > 0.5f && ! acoustic ? autoAmpLevelDb ((int) param ("ampModel"), (int) param ("ampVoicing"), ampGain) : 0.0f));
    buffer.applyGainRamp (0, numSamples, lastAmpTrim, ampTrim);
    lastAmpTrim = ampTrim;

    // Modern metal chug gate: fast gate on the amp's output (before the cab)
    {
        const int  ampIdx = (int) param ("ampModel");
        const float chug  = param ("mtChug");
        const bool chugOn = ! acoustic && ! namActive && ! circuitActive && hasMetalControls (ampIdx) && chug > 0.01f;
        if (chugOn)
            chugGate.process (buffer.getWritePointer (0), numSamples, chug);
        else if (chugWasOn)
            chugGate.reset();
        chugWasOn = chugOn;
    }
    // With Amp B running, Amp A has its own level control
    {
        const float aLvl = ampBActive ? dbToGain (param ("ampALevel")) : 1.0f;
        if (std::abs (aLvl - lastALevel) > 1.0e-6f || aLvl != 1.0f)
            buffer.applyGainRamp (0, numSamples, lastALevel, aLvl);
        lastALevel = aLvl;
    }
    if (! acoustic && numChannels == 2)
        buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);   // back to two identical channels
    // Cab section.
    //   Up to four blocks (A to D). Each active block gets its own copy of the signal, then:
    //   a delay (for phase alignment), the IR (a file or a built-in model), its own low cut and
    //   high cut, then a level, polarity and pan. The blocks are added together (scaled by
    //   1/sqrt(number of blocks), so adding a block does not make it louder). Spread pushes
    //   the blocks apart in the stereo field. With no block active, a built-in cab filter
    //   stands in. Cab-wide low cut, high cut, tilt, reso and air apply to the result.
    //   With Cab On off, only the amp's own low-pass is used.
    const bool cabOn = param ("cabOn") > 0.5f && ! acoustic;   // the acoustic body model replaces the guitar cab
    bool blockActive[kCabBlocks];
    int  numActive = 0;
    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        blockActive[blk] = param (kCabIds[blk].on) > 0.5f && irActive[blk].load() && conv[blk].getCurrentIRSize() > 0;
        if (blockActive[blk]) ++numActive;
    }

    // Without IR blocks (built-in cab, or cab off) both amps are simply added (-3 dB each) before the cab filters.
    const bool auditioning = auditionOn.load() && auditionLoaded.load() && ! acoustic;   // v70
    const bool useBlocks = cabOn && numActive > 0 && ! auditioning;

    if (ampBActive && ! useBlocks)
    {
        float* a = buffer.getWritePointer (0);
        const float* b = ampBBuf.getReadPointer (0);
        for (int i = 0; i < numSamples; ++i)
            a[i] = 0.70710678f * (a[i] + b[i]);
        if (numChannels == 2)
            buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
    }

    const float dynAmt  = param ("cabDyn");
    const int   dynMode = (int) param ("cabDynMode");

    if (auditioning)
    {
        // The IR library's audition path: the amp's signal through the chosen file, in place of the cab blocks
        auditionBuf.setSize (1, numSamples, false, false, true);
        auditionBuf.copyFrom (0, 0, buffer, 0, 0, numSamples);
        juce::dsp::AudioBlock<float> blockX (auditionBuf.getArrayOfWritePointers(), (size_t) 1, (size_t) numSamples);
        juce::dsp::ProcessContextReplacing<float> contextX (blockX);
        auditionConv.process (contextX);
        buffer.copyFrom (0, 0, auditionBuf, 0, 0, numSamples);
        if (numChannels == 2)
            buffer.copyFrom (1, 0, auditionBuf, 0, 0, numSamples);
    }
    else if (useBlocks)
    {
        const int   channels = std::min (numChannels, 2);
        const float spread   = numActive > 1 ? param ("cabSpread") : 0.0f;
        const float norm     = 1.0f / std::sqrt ((float) numActive);
        static const float spreadPosition[kCabBlocks] = { -1.0f, 1.0f, -0.5f, 0.5f };

        float target[kCabBlocks * 2] = {};
        for (int blk = 0; blk < kCabBlocks; ++blk)
        {
            if (! blockActive[blk])
                continue;
            const auto& id = kCabIds[blk];
            const float w   = norm * dbToGain (param (id.level)) * (param (id.pol) > 0.5f ? -1.0f : 1.0f);
            const float pan = (channels == 2) ? juce::jlimit (-1.0f, 1.0f, param (id.pan) + spread * spreadPosition[blk]) : 0.0f;
            float gl, gr;
            panGains (pan, gl, gr);
            target[blk * 2]     = w * gl;
            target[blk * 2 + 1] = w * gr;
        }

        if (! cabGainsInit)
        {
            for (int k = 0; k < kCabBlocks * 2; ++k) lastCabGain[k] = target[k];
            cabGainsInit = true;
        }

        const float sr = (float) currentSampleRate;
        const float maxDelay = 0.0105f * sr;

        for (int blk = 0; blk < kCabBlocks; ++blk)
        {
            if (! blockActive[blk])
                continue;
            const auto& id = kCabIds[blk];
            auto& cb = cabBuffers[blk];

            // This block's copy of the guitar signal (channel 0), delayed on its own
            cb.setSize (1, numSamples, false, false, true);
            const int ampSrc = ampBActive ? juce::jlimit (0, 2, (int) param (id.amp)) : 0;   // 0 = A, 1 = B, 2 = A + B
            if (ampSrc == 0)
                cb.copyFrom (0, 0, buffer, 0, 0, numSamples);
            else if (ampSrc == 1)
                cb.copyFrom (0, 0, ampBBuf, 0, 0, numSamples);
            else
            {
                float* d0 = cb.getWritePointer (0);
                const float* a0 = buffer.getReadPointer (0);
                const float* b0 = ampBBuf.getReadPointer (0);
                for (int i = 0; i < numSamples; ++i)
                    d0[i] = 0.70710678f * (a0[i] + b0[i]);
            }

            if (dynAmt > 0.001f)
                speakerDyn[blk].process (cb.getWritePointer (0), numSamples, dynAmt, dynMode);

            cabDelaySmooth[blk].setTargetValue (param (id.delay) * 0.001f * sr);
            float* d = cb.getWritePointer (0);
            for (int i = 0; i < numSamples; ++i)
            {
                cabDelayLines[blk].pushSample (0, d[i]);
                d[i] = cabDelayLines[blk].popSample (0, juce::jlimit (0.0f, maxDelay, cabDelaySmooth[blk].getNextValue()));
            }
            juce::dsp::AudioBlock<float> blockX (cb.getArrayOfWritePointers(), (size_t) 1, (size_t) numSamples);
            juce::dsp::ProcessContextReplacing<float> contextX (blockX);

            conv[blk].process (contextX);
            if (param (id.lowCut)  > 20.5f)    cabLowCutF[blk].process (contextX);
            if (param (id.highCut) < 19900.0f) cabHighCutF[blk].process (contextX);
            for (int band = 0; band < 5; ++band)
                if (std::abs (param (id.eq[band])) > 0.05f)
                    cabEqF[blk][band].process (contextX);
            if (std::abs (param (id.tilt)) > 0.05f) { cabTiltLoF[blk].process (contextX); cabTiltHiF[blk].process (contextX); }
            if (std::abs (param (id.rso)) > 0.05f) cabRsoF[blk].process (contextX);
            if (param (id.phs) > 0.01f)            cabPhsF[blk].process (contextX);
        }

        // Add the blocks, with the gains sliding from last block's values to this block's.
        float* outL = buffer.getWritePointer (0);
        float* outR = (numChannels == 2) ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < numSamples; ++i)
        {
            const float t = (float) i / (float) numSamples;
            float l = 0.0f, r = 0.0f;
            for (int blk = 0; blk < kCabBlocks; ++blk)
            {
                if (! blockActive[blk])
                    continue;
                const float x = cabBuffers[blk].getReadPointer (0)[i];
                l += x * (lastCabGain[blk * 2]     + (target[blk * 2]     - lastCabGain[blk * 2])     * t);
                r += x * (lastCabGain[blk * 2 + 1] + (target[blk * 2 + 1] - lastCabGain[blk * 2 + 1]) * t);
            }
            outL[i] = l;
            if (outR != nullptr)
                outR[i] = r;
        }
        for (int k = 0; k < kCabBlocks * 2; ++k) lastCabGain[k] = target[k];
    }
    else if (cabOn)
    {
        // No IR loaded: the built-in cab model
        if (dynAmt > 0.001f)
        {
            speakerDyn[0].process (buffer.getWritePointer (0), numSamples, dynAmt, dynMode);
            if (numChannels == 2)
                buffer.copyFrom (1, 0, buffer, 0, 0, numSamples);
        }
        if ((int) param ("cabType") >= 1)
        {
            cabHp.process (context);
            cabLowPeak.process (context);
            cabMidPeak.process (context);
            cabLp.process (context);
        }
        else
        {
            postLowPass.process (context);
        }
    }
    else if (! acoustic)
    {
        postLowPass.process (context);
    }

    if (cabOn)
    {
        if (param ("cabLowCut")  > 20.5f)    cabLowCut.process (context);
        if (param ("cabHighCut") < 19900.0f) cabHighCut.process (context);
        if (std::abs (param ("cabTilt")) > 0.05f)
        {
            cabTiltLow.process (context);
            cabTiltHigh.process (context);
        }
        if (std::abs (param ("cabReso")) > 0.05f)  cabResoPeak.process (context);
        if (std::abs (param ("cabAir"))  > 0.05f)  cabAirShelf.process (context);
    }

    // v65: the resonance tamer and the motion, after the cab (both off by default, and then not touched)
    {
        const float tame = cabOn ? param ("cabTame") : 0.0f;
        if (tame > 0.001f)
        {
            cabTamer.process (buffer.getWritePointer (0), numChannels == 2 ? buffer.getWritePointer (1) : nullptr, numSamples,
                              tame, param ("cabTameDelta") > 0.5f);
            cabTamerWasOn = true;
        }
        else if (cabTamerWasOn) { cabTamer.reset();  cabTamerWasOn = false; }

        const float mot = cabOn && numChannels == 2 ? param ("cabMotion") : 0.0f;
        if (mot > 0.001f)
        {
            cabMotion.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples, mot, param ("cabMotionRate"));
            cabMotionWasOn = true;
        }
        else if (cabMotionWasOn) { cabMotion.reset();  cabMotionWasOn = false; }
    }

    // NAM pedal 2 and the post-cab modulation slots (v51), before the post effects
    {
        namSlot[3].pickUp();
        namSlot[3].processPedal (buffer.getWritePointer (0), numChannels == 2 ? buffer.getWritePointer (1) : nullptr, numSamples,
                                 NamSlot::PedalSettings { param ("nmP2On") > 0.5f, param ("nmP2In"), param ("nmP2Out"), param ("nmP2Mix"),
                                                          param ("nmP2Norm") > 0.5f, param ("nmP2Stereo") > 0.5f });
        runModFx (0, true);
        runModFx (1, true);
        runHarmonizer (1);
    }

    auto runPostEq = [&]()
    {
        if (analyserWanted.load (std::memory_order_relaxed) || toneListening.load (std::memory_order_relaxed))
            preEqTap.push (buffer, numSamples);   // what the EQ is about to receive

        // Post EQ: a 9-band graphic EQ after the cab (and before the delay and reverb)
        if (param ("eqOn") > 0.5f)
        {
            const bool eqAuto = param ("eqAuto") > 0.5f;
            if (eqAuto)
            {
                if ((int) autoRef.size() < numSamples) autoRef.resize ((size_t) numSamples);
                std::copy (buffer.getReadPointer (0), buffer.getReadPointer (0) + numSamples, autoRef.begin());
            }
            const bool eqSteep = param ("eqSlope") > 0.5f;
            if (param ("eqLowCut") > 20.5f)
            {
                eqHp1.process (context);
                if (eqSteep) eqHp2.process (context);
            }
            if (param ("eqHighCut") < 19900.0f)
            {
                eqLp1.process (context);
                if (eqSteep) eqLp2.process (context);
            }

            for (int b = 0; b < kEqBands; ++b)
                if (std::abs (param (kEqIds[b])) > 0.05f)
                    eqBands[b].process (context);

            if (eqAuto)
                eqLeveler.process (autoRef.data(), buffer.getArrayOfWritePointers(), numChannels, numSamples, 1.0f);
            else
                eqLeveler.reset();

            const float eqLevel = dbToGain (param ("eqLevel"));
            buffer.applyGainRamp (0, numSamples, lastEqLevel, eqLevel);
            lastEqLevel = eqLevel;
        }
    };

    auto runTextures = [&]()
    {
        // Textures (after the Post EQ): tremolo, wobble, swell fade-in, then the granular cloud.
        {
            float* tl = buffer.getWritePointer (0);
            float* tr = numChannels > 1 ? buffer.getWritePointer (1) : nullptr;

            if (param ("tremOn") > 0.5f)
            {
                if (! tremWasOn) tremolo.prepare (currentSampleRate);
                tremolo.process (tl, tr, numSamples,
                                 Tremolo::Params { (int) param ("tremShape"), (int) param ("tremMode") == 1,
                                                   lfoRateHz ("tremNote", "tremRate", bpm), param ("tremDepth") });
            }
            tremWasOn = param ("tremOn") > 0.5f;

            if (param ("wobOn") > 0.5f)
                wobble.process (tl, tr, numSamples, Wobble::Params { std::min (20.0f, lfoRateHz ("wobNote", "wobRate", bpm)), param ("wobDepth"), param ("wobDrift"), param ("wobMix") });

            if (param ("swellOn") > 0.5f && (int) swellGains.size() >= numSamples)
                for (int i = 0; i < numSamples; ++i)
                {
                    tl[i] *= swellGains[(size_t) i];
                    if (tr != nullptr) tr[i] *= swellGains[(size_t) i];
                }

            const bool gOn = param ("granOn") > 0.5f;
            if (gOn)
            {
                if (! granWasOn) granular.reset();
                granular.process (tl, tr, numSamples,
                                  Granular::Params { syncedMs ("granSizeNote", "granSize", bpm, 600.0f),
                                                     juce::jlimit (1.0f, 80.0f, lfoRateHz ("granDensNote", "granDensity", bpm)),
                                                     syncedMs ("granPosNote", "granPos", bpm, 3000.0f), param ("granSpray"),
                                                     param ("granPitch"), param ("granPitchRand"), param ("granReverse"),
                                                     param ("granWidth"), param ("granFeedback"), param ("granMix"),
                                                     param ("granFreeze") > 0.5f,
                                                     (int) param ("granScale"), (int) param ("granShape"), param ("granScan"), param ("granTone") });
            }
            granWasOn = gOn;
        }
    };

    auto runDoubler = [&]()
    {
        // Doubler (stereo only)
        {
            const bool dOn = param ("dblOn") > 0.5f && numChannels == 2;
            if (dOn)
            {
                if (! dblWasOn) doubler.reset();
                doubler.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples,
                                 Doubler::Params { param ("dblTime"), param ("dblVar"), param ("dblSpread"), param ("dblMix") });
            }
            dblWasOn = dOn;
        }
    };

    auto runPostDelay = [&]()
    {
        // 6. Post-amp delay: clean repeats of the amped sound, with its own settings
        delayPost.process (buffer, DelayParams { delayTimeMs ("postDelayNote", "postDelayTime", bpm), param ("postDelayFeedback"),
                                                 param ("postDelayOn") > 0.5f ? param ("postDelayMix") : 0.0f,
                                                 param ("postDelayLofi"), param ("postDelayWarble"), (int) param ("postDelayMode"),
                                                 (int) param ("postDelayStereo"), param ("postDelayOffset"), param ("postDelaySpread"),
                                                 (int) param ("postDelayStyle"),
                                                 param ("postDelayDuck"), param ("postDelayDuckRel"), param ("postDelayModDepth"), param ("postDelayModRate"),
                                                 param ("postDelayDiffuse"), param ("postDelayHold") > 0.5f, (int) param ("postDelayHoldMode"),
                                                 (int) param ("postDelayTaps"), param ("postDelayTapLevel") });
    };

    auto runReverb = [&]()
    {
        // 7. Reverb (normal, and shimmer).
        //    One reverb engine (juce::Reverb, run one sample at a time so the shimmer
        //    feedback loop is sample-accurate). Around it:
        //      pre-delay -> [shimmer feedback added] -> reverb -> low cut / high cut -> (ducking) -> mix
        //    With Shimmer on, the filtered reverb output is also fed back into its input
        //    through a pitch shifter, so each trip around the loop climbs higher in pitch
        //    and fades. The low cut and high cut sit inside that loop, so they also keep
        //    the shimmer from piling up lows and harsh highs. The direct signal is untouched.
        {
            const bool  shimmer = param ("shimmerOn") > 0.5f;
            const float mix     = param ("reverbMix");
            const float twoPi   = juce::MathConstants<float>::twoPi;
            const float sr      = (float) currentSampleRate;

            // Shimmer voices. Each interval is one or two voices (semitones). A little Detune splits the
            // pitch into two voices a few cents either side, each drifting slowly, which widens and softens
            // the shimmer and hides the grain of the shifter. Two voices share the level so the loop stays stable.
            struct Interval { float a, b; bool two; };
            static const Interval intervals[8] = { { 12.0f, 12.0f, false }, { 7.0f, 7.0f, false }, { 24.0f, 24.0f, false },
                                                   { -12.0f, -12.0f, false }, { 12.0f, 7.0f, true }, { 12.0f, -12.0f, true },
                                                   { 5.0f, 5.0f, false }, { 19.0f, 19.0f, false } };
            const auto& iv = intervals[juce::jlimit (0, 7, (int) param ("shimmerPitch"))];
            const float detuneCents = param ("shimmerDetune");
            const bool  twoVoices = iv.two || detuneCents > 0.5f;
            const int   shimmerMode = juce::jlimit (0, 2, (int) param ("shimmerMode"));
            float voiceGain = 1.0f;
            if (shimmer)
            {
                if (! shimmerWasOn)
                    for (auto& s : shimmerShifters) s.reset();
                for (int v = 0; v < 2; ++v)
                {
                    shimmerLfo[v] += (v == 0 ? 0.23f : 0.31f) * (float) numSamples / (float) currentSampleRate;
                    if (shimmerLfo[v] >= 1.0f) shimmerLfo[v] -= 1.0f;
                    const float drift = 1.0f + 0.5f * std::sin (juce::MathConstants<float>::twoPi * shimmerLfo[v]);
                    const float cents = (v == 0 ? 1.0f : -1.0f) * detuneCents * drift;
                    shimmerShifters[v].setSemitones ((v == 0 ? iv.a : iv.b) + cents * 0.01f);
                }
                // Two voices of the same note are partly in step, so they share the level (0.5 each) until
                // the detune is wide enough to make them independent (0.75 each); different notes are independent.
                voiceGain = ! twoVoices ? 1.0f
                          : (iv.two ? 0.75f : 0.5f + 0.25f * juce::jlimit (0.0f, 1.0f, (detuneCents - 0.5f) / 9.5f));
            }
            shimmerWasOn = shimmer;

            juce::Reverb::Parameters rp;
            // The classic reverb has no decay time of its own: pick the room size whose feedback gives
            // the wanted decay (its comb filters average about 35 ms; the feedback runs from 0.70 to 0.98,
            // so it covers roughly 0.7 to 12 seconds, and longer settings stay at its maximum).
            {
                const float decayS = std::max (0.1f, param ("reverbSize"));
                const float fb = std::pow (10.0f, -3.0f * 0.035f / decayS);
                rp.roomSize = juce::jlimit (0.0f, 1.0f, (fb - 0.7f) / 0.28f);
            }
            rp.damping    = param ("reverbDamp");
            rp.width      = param ("reverbWidth");
            rp.freezeMode = param ("reverbFreeze") > 0.5f ? 1.0f : 0.0f;
            rp.dryLevel   = 0.0f;    // we add the wet signal to the dry ourselves
            rp.wetLevel   = 0.33f;   // JUCE multiplies wet by 3, so this is about unity
            shimmerReverb.setParameters (rp);

            const int revType = juce::jlimit (0, FdnReverb::kTypes, (int) param ("reverbType"));
            if (revType != lastRevType)
            {
                lastRevType = revType;
                fdnReverb.reset();
                shimmerReverb.reset();
            }
            if (revType > 0)
            {
                FdnReverb::Extra ex;
                ex.diffuse = param ("reverbDiffuse");
                ex.space   = param ("reverbSpace");
                ex.mod     = param ("reverbMod");
                {
                    // Drag: the whole space is stretched or squeezed; the delays glide there, so the tail bends in pitch on the way
                    const float dk = (param ("reverbDrag") - 0.5f) * 48.0f;           // -24 .. +24 semitones
                    const int   dm = (int) param ("reverbDragMode");
                    float q = dk;
                    if (dm == 1) q = std::round (dk);
                    else if (dm == 3) q = 12.0f * std::round (dk / 12.0f);
                    else if (dm == 2)
                    {
                        static const float steps[9] = { -24.0f, -17.0f, -12.0f, -5.0f, 0.0f, 7.0f, 12.0f, 19.0f, 24.0f };
                        q = steps[0];
                        for (float st : steps) if (std::abs (st - dk) < std::abs (q - dk)) q = st;
                    }
                    ex.dragSemis = q;
                }
                fdnReverb.setParams (revType, param ("reverbSize"), param ("reverbDamp"), param ("reverbWidth"),
                                     param ("reverbFreeze") > 0.5f, ex);
            }
            const float bloomAmt = param ("reverbBloom");
            const float ensAmt   = param ("reverbMod");
            const float symAmt   = param ("reverbSymph");
            {
                static const float symSemis[4] = { 12.0f, -12.0f, 7.0f, 24.0f };
                symphShifter.setSemitones (symSemis[juce::jlimit (0, 3, (int) param ("reverbSymphPitch"))]);
                if (symAmt > 0.001f && ! symphWasOn) symphShifter.reset();
                symphWasOn = symAmt > 0.001f;
            }
            const float bloomRise = 1.0f - std::exp (-3.0f / ((0.05f + 2.5f * bloomAmt * bloomAmt) * (float) currentSampleRate));
            const float bloomFall = 1.0f - std::exp (-1.0f / (0.12f * (float) currentSampleRate));

            // Wet filters (updated only when the knobs move)
            const float hpHz = param ("reverbLowCut"), lpHz = std::min (param ("reverbHighCut"), 0.45f * sr);
            if (std::abs (hpHz - lastRevHp) > 0.001f * hpHz)
            {
                for (auto& f : revHp) f.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, hpHz);
                lastRevHp = hpHz;
            }
            if (std::abs (lpHz - lastRevLp) > 0.001f * lpHz)
            {
                for (auto& f : revLp) f.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (currentSampleRate, lpHz);
                lastRevLp = lpHz;
            }

            reverbPreSmooth.setTargetValue (syncedMs ("reverbPreNote", "reverbPreDelay", bpm, 250.0f) * 0.001f * sr);
            const float maxPre = 0.25f * sr;

            const float amount  = shimmer ? param ("shimmerAmount") * 0.75f : 0.0f;   // feedback: always below 1
            const float shimmerInputBoost = (shimmerMode == 1) ? 1.3f : 1.0f;          // no loop in Input mode, so it can be louder
            const float wetTarget = (param ("reverbOn") > 0.5f ? mix : 0.0f) * 0.75f;
            const float duck    = param ("reverbDuck");
            const float duckAtk = 1.0f - std::exp (-1.0f / (0.005f * sr));
            const float duckRel = 1.0f - std::exp (-1.0f / (0.250f * sr));
            const float drive   = param ("reverbDrive");
            const float drvHpC  = 1.0f - std::exp (-twoPi * (40.0f + 100.0f * drive * drive) / sr);
            const float drvLpC  = 1.0f - std::exp (-twoPi * (std::min (12000.0f, 0.45f * sr) - 6500.0f * drive) / sr);
            const float lpCoef  = 1.0f - std::exp (-twoPi * param ("shimmerTone") / sr);
            const float hpCoef  = 1.0f - std::exp (-twoPi * 200.0f / sr);

            float* left  = buffer.getWritePointer (0);
            float* right = (numChannels > 1) ? buffer.getWritePointer (1) : nullptr;

            // Skip the whole reverb while it is off (or at mix 0) and has faded out. The engines are
            // cleared on the way back in, so an old tail does not reappear.
            const bool revSilent = wetTarget <= 0.0f && revWetSmooth < 1.0e-5f;
            if (revSilent)
            {
                revIdle = true;
                revWetSmooth = 0.0f;
            }
            else if (revIdle)
            {
                revIdle = false;
                fdnReverb.reset();
                shimmerReverb.reset();
                shimmerLast = shimmerLpState = shimmerHpState = 0.0f;
                for (auto& f : revHp) f.reset();
                for (auto& f : revLp) f.reset();
                revDrvHp = revDrvLp = 0.0f;
            }

            for (int i = 0; i < numSamples && ! revSilent; ++i)
            {
                const float in = right ? 0.5f * (left[i] + right[i]) : left[i];

                // Ducking: follow the dry signal and turn the reverb down while you play
                const float level = std::abs (in);
                duckEnv += ((level > duckEnv) ? duckAtk : duckRel) * (level - duckEnv);
                const float duckGain = 1.0f - duck * std::min (1.0f, duckEnv * 8.0f);

                reverbPreDelay.pushSample (0, in);
                float fed = reverbPreDelay.popSample (0, juce::jlimit (0.0f, maxPre, reverbPreSmooth.getNextValue()));

                // Bloom: after a pause, the send swells in slowly, so the reverb blooms up behind your notes
                if (bloomAmt > 0.001f)
                {
                    const float target = duckEnv > 0.003f ? 1.0f : 0.0f;
                    bloomGain += (target > bloomGain ? bloomRise : bloomFall) * (target - bloomGain);
                    fed *= bloomGain;
                }
                else
                    bloomGain = 1.0f;

                // Spring driver (Spring type only): the reverb tank is fed through a small transformer and a driver tube. The
                // transformer thins the lows, the tube clips softly and a touch lopsided when pushed (the "drip" and "crash"
                // of a spring hit hard), and the recovery stage rolls off the top.
                if (drive > 0.001f && revType == 6)
                {
                    revDrvHp += drvHpC * (fed - revDrvHp);
                    // soft ceiling that comes down as the drive goes up; small signals pass at unity gain, hard hits flatten and splash
                    const float ceil = 0.55f - 0.4f * drive, bias = 0.25f * drive;
                    float v = fed - revDrvHp;
                    v = ceil * (fastTanh (v / ceil + bias) - fastTanh (bias)) / (1.0f - fastTanh (bias) * fastTanh (bias));
                    revDrvLp += drvLpC * (v - revDrvLp);
                    fed = revDrvLp;
                }

                if (shimmer)
                {
                    // What gets shifted: the reverb's own output (Feedback, so it blooms and climbs), the
                    // incoming signal (Input, a single shifted layer with no build-up), or both. It is darkened
                    // (Shimmer Tone), thinned out in the lows and limited, then shifted.
                    float fb = (shimmerMode != 1 ? shimmerLast : 0.0f) + (shimmerMode != 0 ? in : 0.0f);
                    shimmerLpState += lpCoef * (fb - shimmerLpState);
                    fb = shimmerLpState;
                    shimmerHpState += hpCoef * (fb - shimmerHpState);
                    fb -= shimmerHpState;
                    fb = fastTanh (fb);
                    float shifted = shimmerShifters[0].process (fb);
                    if (twoVoices) shifted += shimmerShifters[1].process (fb);
                    fed += amount * shimmerInputBoost * voiceGain * shifted;
                }

                float l = fed, r = fed;
                if (revType == 0) shimmerReverb.processStereo (&l, &r, 1);
                else              fdnReverb.process (fed, l, r);

                if (ensAmt > 0.001f)
                    wetEnsemble.process (l, r, ensAmt, 1.0f + ensAmt);
                if (symAmt > 0.001f)
                {
                    const float sy = symAmt * 0.8f * symphShifter.process (0.5f * (l + r));
                    l += sy;  r += sy;
                }

                l = revLp[0].processSample (revHp[0].processSample (l));
                r = revLp[1].processSample (revHp[1].processSample (r));
                shimmerLast = 0.5f * (l + r);

                revWetSmooth += 0.002f * (wetTarget - revWetSmooth);   // ~10 ms, so the switch does not click
                const float g = revWetSmooth * duckGain;
                if (right) { left[i] += g * l; right[i] += g * r; }
                else       { left[i] += g * 0.5f * (l + r); }
            }
        }
    };

    auto runWidener = [&]()
    {
        // Widener (stereo only): after the reverb, so the reverb gets widened too
        {
            const bool wOn = param ("widOn") > 0.5f && numChannels == 2;
            if (wOn)
            {
                if (! widWasOn) widener.reset();
                widener.process (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples,
                                 Widener::Params { (int) param ("widMode"), param ("widWow"), param ("widSpace"), param ("widBass"),
                                                   param ("widBassHz"), param ("widMono"), param ("widMix"), dbToGain (param ("widLevel")) });
            }
            widWasOn = wOn;
        }
    };

    // The post-amp effects run in the order chosen on the chain strip (default: Post EQ, textures,
    // doubler, delay, reverb, widener).
    {
        const auto order = getFxOrder (1);
        for (int k = 0; k < kFxBlocks; ++k)
            switch (order[(size_t) k])
            {
                case 0: runPostEq();    break;
                case 1: runTextures();  break;
                case 2: runDoubler();   break;
                case 3: runPostDelay(); break;
                case 4: runReverb();    break;
                default: runWidener();  break;
            }
    }

    // 8. Output gain
    const float outGain = dbToGain (param ("outGain"));
    buffer.applyGainRamp (0, numSamples, lastOutputGain, outGain);
    lastOutputGain = outGain;

    // A/B level match. While a slot is playing, its loudness (before any trim) is tracked with a slow
    // average over the stretches where you are playing. When you switch slots, the editor works out a
    // trim so the new slot is as loud as the old one was. The trim then stays fixed, so changing a knob
    // on the new slot is still audible.
    {
        double sum = 0.0;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* x = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                sum += (double) x[i] * (double) x[i];
        }
        const double meanSq = sum / (double) std::max (1, numSamples * numChannels);
        const float  rmsDb  = 10.0f * std::log10 ((float) meanSq + 1.0e-12f);
        if (rmsDb > -55.0f)
        {
            const int   slot  = abIdx.load();
            const float old   = abLoudDb[slot].load();
            const float alpha = 1.0f - std::exp (-(float) numSamples / (2.0f * (float) currentSampleRate));   // about 2 s
            abLoudDb[slot] = old < -150.0f ? rmsDb : old + alpha * (rmsDb - old);
        }
        const float trimGain = dbToGain (abTrimTargetDb.load());
        if (trimGain != 1.0f || lastAbTrimGain != 1.0f)
        {
            buffer.applyGainRamp (0, numSamples, lastAbTrimGain, trimGain);
            lastAbTrimGain = trimGain;
        }
    }

    // Mono output: both sides get the sum (to check how the sound folds down to mono)
    if (numChannels == 2 && param ("outMono") > 0.5f)
    {
        float* l = buffer.getWritePointer (0);
        float* r = buffer.getWritePointer (1);
        for (int i = 0; i < numSamples; ++i)
            l[i] = r[i] = 0.5f * (l[i] + r[i]);
    }

    // 9. Optional final limiter. Look-ahead: none in Low Latency, 1 ms in Normal, 2 ms in High Quality.
    {
        const bool limOn = param ("limOn") > 0.5f;
        const int  look  = limOn ? (int) std::lround (currentSampleRate * (perfMode() == 1 ? 0.0 : (perfMode() == 2 ? 0.002 : 0.001))) : 0;
        if (limOn && (! limWasOn || look != limiter.lookahead()))
            limiter.setLookahead (look);
        if (limOn != limWasOn || (limOn && look != limLatency.load()))
        {
            limLatency.store (limOn ? look : 0);
            latencyDirty.store (true);
            triggerAsyncUpdate();
        }
        limWasOn = limOn;
        if (limOn)
            limiter.process (buffer.getWritePointer (0), numChannels > 1 ? buffer.getWritePointer (1) : nullptr,
                             numSamples, dbToGain (param ("limCeil")), param ("limRel"), currentSampleRate);
    }

    // Riff recorder, full tone mode: record the finished sound, or add the recording on top
    if (numChannels >= 1 && riffRecMode.load() == 1)
        riff.processOutput (buffer.getWritePointer (0), numChannels > 1 ? buffer.getWritePointer (1) : nullptr, numSamples, param ("riffLevel"));

    // Metronome: added after everything else, so the amp and effects never touch it
    {
        Metronome::Params mp;
        mp.on = param ("metroOn") > 0.5f;
        mp.bpm = bpm;
        mp.beats = (int) param ("metroBeats");
        mp.sub = 1 + (int) param ("metroSub");
        mp.sound = (int) param ("metroSound");
        mp.gain = dbToGain (param ("metroVol"));
        mp.accent = param ("metroAccent") > 0.5f;
        if (mp.on && param ("tempoHost") > 0.5f)
            if (auto* playHead = getPlayHead())
                if (auto position = playHead->getPosition())
                    if (position->getIsPlaying())
                        if (auto ppq = position->getPpqPosition())
                            mp.hostPpq = *ppq;
        metro.process (buffer, numSamples, mp);
    }

    // The tuner mutes the output (smoothly) while it is open
    {
        const float target = tunerActive.load (std::memory_order_relaxed) ? 0.0f : 1.0f;
        if (target != 1.0f || lastTunerGain != 1.0f)
        {
            buffer.applyGainRamp (0, numSamples, lastTunerGain, target);
            lastTunerGain = target;
        }
    }

    {
        float pk = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* x = buffer.getReadPointer (ch);
            for (int i = 0; i < numSamples; ++i)
                pk = std::max (pk, std::abs (x[i]));
        }
        if (pk > meterOut.load (std::memory_order_relaxed))
            meterOut.store (pk, std::memory_order_relaxed);
    }

    if (analyserWanted.load (std::memory_order_relaxed))
        outputTap.push (buffer, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* StarterAmpProcessor::createEditor()
{
    return new StarterAmpEditor (*this);
}

//==============================================================================
// Effect order, undo / redo, A/B
//==============================================================================
static const char* const kFxNames[2][StarterAmpProcessor::kFxBlocks] =
{
    { "Fuzz", "Compressor", "Wah", "Phaser", "Chorus", "Pre-amp delay" },
    { "Post EQ", "Textures", "Doubler", "Post-amp delay", "Reverb", "Widener" }
};

const char* StarterAmpProcessor::fxName (int group, int block)
{
    return kFxNames[juce::jlimit (0, 1, group)][juce::jlimit (0, kFxBlocks - 1, block)];
}

std::array<int, StarterAmpProcessor::kFxBlocks> StarterAmpProcessor::getFxOrder (int group) const
{
    std::array<int, kFxBlocks> order;
    unsigned seen = 0;
    bool valid = true;
    for (int k = 0; k < kFxBlocks; ++k)
    {
        order[(size_t) k] = fxOrder[group][k].load (std::memory_order_relaxed);
        if (order[(size_t) k] < 0 || order[(size_t) k] >= kFxBlocks || (seen & (1u << order[(size_t) k])))
            valid = false;
        else
            seen |= 1u << order[(size_t) k];
    }
    if (! valid)
        for (int k = 0; k < kFxBlocks; ++k) order[(size_t) k] = k;
    return order;
}

void StarterAmpProcessor::setFxOrder (int group, const std::array<int, kFxBlocks>& order)
{
    unsigned seen = 0;
    for (int b : order)
    {
        if (b < 0 || b >= kFxBlocks || (seen & (1u << b))) return;     // not a permutation: ignore
        seen |= 1u << b;
    }
    for (int k = 0; k < kFxBlocks; ++k)
        fxOrder[group][k] = order[(size_t) k];
    apvts.state.setProperty (group == 0 ? "fxPreOrder" : "fxPostOrder", fxOrderToString (group), nullptr);
}

void StarterAmpProcessor::moveFxBlock (int group, int from, int to)
{
    auto order = getFxOrder (group);
    from = juce::jlimit (0, kFxBlocks - 1, from);
    to   = juce::jlimit (0, kFxBlocks - 1, to);
    if (from == to) return;
    const int moved = order[(size_t) from];
    if (from < to) for (int k = from; k < to; ++k) order[(size_t) k] = order[(size_t) k + 1];
    else           for (int k = from; k > to; --k) order[(size_t) k] = order[(size_t) k - 1];
    order[(size_t) to] = moved;
    setFxOrder (group, order);
}

void StarterAmpProcessor::resetFxOrder()
{
    std::array<int, kFxBlocks> order;
    for (int k = 0; k < kFxBlocks; ++k) order[(size_t) k] = k;
    setFxOrder (0, order);
    setFxOrder (1, order);
}

juce::String StarterAmpProcessor::fxOrderToString (int group) const
{
    juce::StringArray parts;
    for (int b : getFxOrder (group)) parts.add (juce::String (b));
    return parts.joinIntoString (",");
}

void StarterAmpProcessor::fxOrderFromString (int group, const juce::String& text)
{
    juce::StringArray parts;
    parts.addTokens (text, ",", "");
    std::array<int, kFxBlocks> order;
    for (int k = 0; k < kFxBlocks; ++k) order[(size_t) k] = k;
    if (parts.size() == kFxBlocks)
        for (int k = 0; k < kFxBlocks; ++k) order[(size_t) k] = parts[k].getIntValue();
    // setFxOrder ignores anything that is not a permutation, so fall back to the default order
    for (int k = 0; k < kFxBlocks; ++k) fxOrder[group][k] = k;
    setFxOrder (group, order);
}

StarterAmpProcessor::Snapshot StarterAmpProcessor::captureSnapshot() const
{
    Snapshot s;
    s.values.reserve (midiParams.size() + 2 * kFxBlocks);
    for (auto* prm : midiParams)
        s.values.push_back (prm->getValue());
    for (int g = 0; g < 2; ++g)
        for (int b : getFxOrder (g))
            s.values.push_back ((float) b);
    s.ab = abIdx.load();
    return s;
}

void StarterAmpProcessor::applySnapshot (const Snapshot& s)
{
    if (s.values.size() != midiParams.size() + 2 * (size_t) kFxBlocks)
        return;
    for (size_t i = 0; i < midiParams.size(); ++i)
        if (midiParams[i]->getValue() != s.values[i])
            midiParams[i]->setValueNotifyingHost (s.values[i]);
    for (int g = 0; g < 2; ++g)
    {
        std::array<int, kFxBlocks> order;
        for (int k = 0; k < kFxBlocks; ++k)
            order[(size_t) k] = (int) std::lround (s.values[midiParams.size() + (size_t) (g * kFxBlocks + k)]);
        setFxOrder (g, order);
    }
    abIdx = s.ab;
}

void StarterAmpProcessor::undoTick (bool userActive)
{
    const auto now = captureSnapshot();
    if (! undoCurrentValid) { undoCurrent = now; undoCurrentValid = true; return; }
    if (now != undoCurrent)
    {
        if (userActive && ! undoPending)
        {
            undoPendingBefore = undoCurrent;
            undoPending = true;
        }
        undoCurrent = now;                      // a change nobody was making by hand is just adopted
    }
    if (undoPending && ! userActive)
    {
        if (undoPendingBefore != undoCurrent)
        {
            undoStack.push_back (undoPendingBefore);
            if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
            redoStack.clear();
        }
        undoPending = false;
    }
}

bool StarterAmpProcessor::undo()
{
    undoTick (false);                           // close the step that is still open
    if (undoStack.empty()) return false;
    const auto target = undoStack.back();
    undoStack.pop_back();
    redoStack.push_back (captureSnapshot());
    const int wasSlot = abIdx.load();
    applySnapshot (target);
    if (abIdx.load() != wasSlot) abComputeTrim (wasSlot, abIdx.load());
    undoCurrent = captureSnapshot();
    return true;
}

bool StarterAmpProcessor::redo()
{
    undoTick (false);
    if (redoStack.empty()) return false;
    const auto target = redoStack.back();
    redoStack.pop_back();
    undoStack.push_back (captureSnapshot());
    const int wasSlot = abIdx.load();
    applySnapshot (target);
    if (abIdx.load() != wasSlot) abComputeTrim (wasSlot, abIdx.load());
    undoCurrent = captureSnapshot();
    return true;
}

void StarterAmpProcessor::abComputeTrim (int from, int to)
{
    float trim = 0.0f;
    const float loudFrom = abLoudDb[from].load(), loudTo = abLoudDb[to].load();
    if (abMatch.load() && loudFrom > -150.0f && loudTo > -150.0f)
    {
        const float heard = loudFrom + abTrimTargetDb.load();           // how loud the old slot actually was
        trim = juce::jlimit (-12.0f, 12.0f, heard - loudTo);
    }
    abTrimTargetDb = trim;
}

void StarterAmpProcessor::abSelect (int slot)
{
    slot = juce::jlimit (0, 1, slot);
    if (slot == abIdx.load()) return;
    undoTick (false);                           // anything pending belongs to the old slot
    const auto before = captureSnapshot();
    abSlot[abIdx.load()] = before;
    abHas[abIdx.load()] = true;
    if (! abHas[slot]) { abSlot[slot] = before; abHas[slot] = true; }   // the first time, B starts as a copy of A

    undoStack.push_back (before);
    if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
    redoStack.clear();

    const int from = abIdx.load();
    auto after = abSlot[slot];
    after.ab = slot;
    abComputeTrim (from, slot);
    applySnapshot (after);
    undoCurrent = captureSnapshot();
}

void StarterAmpProcessor::abCopyToOther()
{
    const int active = abIdx.load();
    abSlot[active] = captureSnapshot();
    abHas[active] = true;
    abSlot[1 - active] = abSlot[active];
    abSlot[1 - active].ab = 1 - active;
    abHas[1 - active] = true;
    abLoudDb[1 - active] = abLoudDb[active].load();     // the same sound is as loud as the original
}

void StarterAmpProcessor::abSetLevelMatch (bool on)
{
    abMatch = on;
    if (! on) abTrimTargetDb = 0.0f;
}

void StarterAmpProcessor::abForgetLevels()
{
    abLoudDb[0] = abLoudDb[1] = -200.0f;
    abTrimTargetDb = 0.0f;
}

//==============================================================================
// MIDI
//==============================================================================
int StarterAmpProcessor::midiIndexOf (const juce::String& paramId) const
{
    for (int i = 0; i < midiParamCount(); ++i)
        if (midiParams[(size_t) i]->paramID == paramId)
            return i;
    return -1;
}

int StarterAmpProcessor::midiSlotFor (int paramIndex) const
{
    for (int s = 0; s < kMidiSlots; ++s)
        if (midiTarget[s].load() == paramIndex)
            return s;
    return -1;
}

void StarterAmpProcessor::midiBind (int slot, int paramIndex, bool invert)
{
    if (! juce::isPositiveAndBelow (slot, kMidiSlots) || ! juce::isPositiveAndBelow (paramIndex, midiParamCount()))
        return;
    for (int s = 0; s < kMidiSlots; ++s)           // a parameter answers to one control only
        if (s != slot && midiTarget[s].load() == paramIndex)
            midiTarget[s] = -1;
    midiInv[slot] = invert;
    midiLastSent[slot] = -1;
    midiTarget[slot] = paramIndex;
}

void StarterAmpProcessor::midiUnbind (int slot)
{
    if (juce::isPositiveAndBelow (slot, kMidiSlots))
        midiTarget[slot] = -1;
}

void StarterAmpProcessor::midiResetDefaults()
{
    for (int s = 0; s < kMidiSlots; ++s) { midiTarget[s] = -1; midiInv[s] = false; midiLastSent[s] = -1; }
    midiBind (11, midiIndexOf ("wahPos"), false);                  // expression pedal -> wah position
    midiBind (kPitchBendSlot, midiIndexOf ("wowPos"), false);      // pitch wheel -> pitch pedal position
}

// Audio thread. Only reads atomics and sets parameters (no allocation, no locks).
void StarterAmpProcessor::handleMidi (const juce::MidiBuffer& midi)
{
    const int wantedChannel = midiChannel.load();
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (wantedChannel > 0 && msg.getChannel() != wantedChannel)
            continue;

        int slot = -1, value = 0, maxValue = 127;
        if (msg.isController())
        {
            slot = msg.getControllerNumber();  value = msg.getControllerValue();
        }
        else if (msg.isPitchWheel())
        {
            slot = kPitchBendSlot;  value = msg.getPitchWheelValue();  maxValue = 16383;
        }
        else if (msg.isProgramChange())
        {
            const int program = msg.getProgramChangeNumber();
            if (program < factory::kNumPresets)
            {
                midiLastProgram = program;
                pendingProgram = program;
                triggerAsyncUpdate();
            }
            continue;
        }
        else
            continue;

        if (! juce::isPositiveAndBelow (slot, kMidiSlots))
            continue;

        midiLastSlot = slot;  midiLastValue = value;  ++midiActivity;

        const int learning = midiLearnIndex.load();
        if (learning >= 0)
        {
            midiBind (slot, learning, false);
            midiLearnIndex = -1;
        }

        const int target = midiTarget[slot].load();
        if (target < 0 || value == midiLastSent[slot])
            continue;
        midiLastSent[slot] = value;

        float norm = (float) value / (float) maxValue;
        if (midiInv[slot].load()) norm = 1.0f - norm;
        midiParams[(size_t) target]->setValueNotifyingHost (norm);
    }
}

// The mapping is saved with the session, as a small "midiMap" node.
void StarterAmpProcessor::midiToState (juce::ValueTree& state) const
{
    state.removeChild (state.getChildWithName ("midiMap"), nullptr);
    juce::ValueTree node ("midiMap");
    node.setProperty ("channel", midiChannel.load(), nullptr);
    for (int s = 0; s < kMidiSlots; ++s)
    {
        const int t = midiTarget[s].load();
        if (t < 0) continue;
        juce::ValueTree m ("map");
        m.setProperty ("slot", s, nullptr);
        m.setProperty ("id", midiParams[(size_t) t]->paramID, nullptr);
        m.setProperty ("inv", midiInv[s].load(), nullptr);
        node.appendChild (m, nullptr);
    }
    state.appendChild (node, nullptr);
}

void StarterAmpProcessor::midiFromState (const juce::ValueTree& state)
{
    const auto node = state.getChildWithName ("midiMap");
    if (! node.isValid())
        return;                                                    // older sessions keep the defaults
    for (int s = 0; s < kMidiSlots; ++s) { midiTarget[s] = -1; midiInv[s] = false; midiLastSent[s] = -1; }
    midiChannel = juce::jlimit (0, 16, (int) node.getProperty ("channel", 0));
    for (const auto m : node)
        midiBind ((int) m.getProperty ("slot", -1), midiIndexOf (m.getProperty ("id").toString()), (bool) m.getProperty ("inv", false));
}

void StarterAmpProcessor::saveState (juce::MemoryBlock& destData, bool withMidi)
{
    auto state = apvts.copyState();
    state.setProperty ("fxPreOrder",  fxOrderToString (0), nullptr);
    state.setProperty ("fxPostOrder", fxOrderToString (1), nullptr);
    state.setProperty ("ampSplit", 1, nullptr);
    if (withMidi)
        midiToState (state);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void StarterAmpProcessor::getStateInformation (juce::MemoryBlock& destData) { saveState (destData, true); }
void StarterAmpProcessor::getPresetData (juce::MemoryBlock& destData)       { saveState (destData, false); }
void StarterAmpProcessor::setStateInformation (const void* data, int sizeInBytes) { restoreState (data, sizeInBytes, true); }
bool StarterAmpProcessor::loadPresetData (const void* data, int sizeInBytes)       { return restoreState (data, sizeInBytes, false); }

bool StarterAmpProcessor::restoreState (const void* data, int sizeInBytes, bool withMidi)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            if (withMidi)
                midiFromState (apvts.state);
            apvts.state.removeChild (apvts.state.getChildWithName ("midiMap"), nullptr);
            fxOrderFromString (0, apvts.state.getProperty ("fxPreOrder").toString());
            fxOrderFromString (1, apvts.state.getProperty ("fxPostOrder").toString());

            // Before v19 the amp list was: Clean, Hot-Rod, High-Gain, Chime. It is now ordered
            // by gain (Clean, Chime, Hot-Rod, Lead, High-Gain), so translate older sessions.
            if (! apvts.state.hasProperty ("ampOrder"))
            {
                auto node = apvts.state.getChildWithProperty ("id", "ampModel");
                if (node.isValid())
                {
                    static const int oldToNew[4] = { 0, 2, 4, 1 };
                    const int oldIndex = juce::jlimit (0, 3, (int) node.getProperty ("value"));
                    node.setProperty ("value", oldToNew[oldIndex], nullptr);
                }
            }
            apvts.state.setProperty ("ampOrder", 2, nullptr);

            // v62: the Classic Rock amp's third voicing ("Master volume") became its own amp (value 15, voicing 0). Sessions and
            // presets saved before then that used it are moved across, so they sound the same.
            if (! apvts.state.hasProperty ("ampSplit"))
            {
                auto param = [this] (const char* id) { return apvts.state.getChildWithProperty ("id", id); };
                auto modelA = param ("ampModel"), voicA = param ("ampVoicing");
                if (modelA.isValid() && voicA.isValid() && (int) modelA.getProperty ("value") == 6 && (int) voicA.getProperty ("value") == 2)
                {
                    modelA.setProperty ("value", 15, nullptr);
                    voicA.setProperty ("value", 0, nullptr);
                }
                auto modelB = param ("ampBModel"), voicB = param ("ampBVoicing");
                if (modelB.isValid() && voicB.isValid() && (int) modelB.getProperty ("value") == 5 && (int) voicB.getProperty ("value") == 2)
                {
                    modelB.setProperty ("value", 14, nullptr);               // Amp B's menu position of the new amp
                    voicB.setProperty ("value", 0, nullptr);
                }
            }
            apvts.state.setProperty ("ampSplit", 1, nullptr);

            // Before v31 the reverb's Size was a 0 to 1 knob; turn it into the decay time it gave.
            if (! apvts.state.hasProperty ("reverbDecay"))
            {
                auto sizeNode = apvts.state.getChildWithProperty ("id", "reverbSize");
                if (sizeNode.isValid())
                {
                    const float oldSize = juce::jlimit (0.0f, 1.0f, (float) sizeNode.getProperty ("value"));
                    auto typeNode = apvts.state.getChildWithProperty ("id", "reverbType");
                    const int type = typeNode.isValid() ? juce::jlimit (0, 6, (int) typeNode.getProperty ("value")) : 0;
                    static const float typeDecay[7] = { 0.0f, 0.55f, 2.2f, 1.6f, 4.2f, 6.5f, 1.8f };
                    float seconds;
                    if (type == 0)
                    {
                        const float fb = 0.7f + 0.28f * oldSize;
                        seconds = -3.0f * 0.035f / std::log10 (fb);
                    }
                    else
                        seconds = typeDecay[type] * (0.4f + 1.2f * oldSize);
                    setParamValue ("reverbSize", juce::jlimit (0.1f, 30.0f, seconds));
                }
            }
            apvts.state.setProperty ("reverbDecay", 1, nullptr);

            // Older sessions: before v21 there was no IR source setting (if an IR file was used, keep
            // using it), and before v22 there were only blocks A and B, with no On switch for B.
            for (int blk = 0; blk < kCabBlocks; ++blk)
            {
                const juce::String s = cabLetter (blk);
                const juce::File irFile (apvts.state.getProperty ("irPath" + s).toString());
                auto srcNode = apvts.state.getChildWithProperty ("id", "cabSrc" + s);
                if (! srcNode.isValid() && irFile.existsAsFile())
                    setParamValue (("cabSrc" + s).toRawUTF8(), 1.0f);
                if (blk == 1 && ! apvts.state.getChildWithProperty ("id", "cabOnB").isValid())
                {
                    const bool wasUsed = (srcNode.isValid() && (int) srcNode.getProperty ("value") == 0) || irFile.existsAsFile();
                    setParamValue ("cabOnB", wasUsed ? 1.0f : 0.0f);
                }
            }

            // Before v71 the position and proximity laws were the earlier, stronger ones: keep them for older sessions.
            if (! apvts.state.getChildWithProperty ("id", "cabMeasured").hasProperty ("value"))
                setParamValue ("cabMeasured", 0.0f);

            // Before v29 the cabinet shape was one shared menu; now each block has its own.
            {
                auto oldNode = apvts.state.getChildWithProperty ("id", "cabCabinet");
                if (oldNode.isValid())
                    for (int blk = 0; blk < kCabBlocks; ++blk)
                        if (! apvts.state.getChildWithProperty ("id", "cabCbn" + cabLetter (blk)).isValid())
                            setParamValue (("cabCbn" + cabLetter (blk)).toRawUTF8(), (float) (int) oldNode.getProperty ("value"));
            }

            // v87: Circuit Amp, Stage Model and Derived level became one Mode per amp; Power Scaling, Output Class, Rectifier,
            // Metal Sag and Dual voice were folded into Power Drive and Sag or removed; the tone stack menu lost its two
            // classic entries. Older sessions are moved across (close, not identical).
            if (! apvts.state.hasProperty ("ampV87"))
            {
                auto val = [this] (const juce::String& id, float def)
                {
                    auto n = apvts.state.getChildWithProperty ("id", id);
                    return n.isValid() && n.hasProperty ("value") ? (float) n.getProperty ("value") : def;
                };
                for (int b = 0; b < 2; ++b)
                {
                    const juce::String pre = b == 0 ? "amp" : "ampB";
                    const bool circuit = b == 0 && val ("ampCircuit", 0.0f) > 0.5f;
                    const bool derivedOn = val (pre + "StageModel", 0.0f) > 0.5f || val (pre + "Derived", 0.0f) > 0.5f;
                    setParamValue ((pre + "Mode").toRawUTF8(), circuit ? 2.0f : derivedOn ? 1.0f : 0.0f);

                    float power = val (pre + "Power", 0.0f), sag = val (pre + "SagAmt", 0.5f);
                    const int watt = (int) val (pre + "Watt", 0.0f);
                    power += watt == 1 ? 0.15f : watt == 2 ? 0.3f : 0.0f;
                    if (val (pre + "Rect", 0.0f) > 0.5f) sag *= 0.35f;
                    if (b == 0 && hasMetalControls ((int) val ("ampModel", 0.0f))) sag *= val ("mtSag", 0.5f) / 0.5f;
                    setParamValue ((pre + "Power").toRawUTF8(), juce::jlimit (0.0f, 1.0f, power));
                    setParamValue ((pre + "SagAmt").toRawUTF8(), juce::jlimit (0.0f, 1.0f, sag));

                    const int st = (int) val (pre + "Stack", 0.0f);
                    static const int stackMap[4] = { 0, 2, 3, 1 };
                    setParamValue ((pre + "Stack").toRawUTF8(), (float) (st < 4 ? stackMap[juce::jmax (0, st)] : st - 2));
                }
            }
            apvts.state.setProperty ("ampV87", 1, nullptr);

            // Reload the IRs this session was using (built-in models are rebuilt, files are reloaded)
            refreshAllSlots();
            namDirty = true;                      // load the .nam model this session was using
            triggerAsyncUpdate();
            return true;
        }
    return false;
}

//==============================================================================
// IR loading. Call this from the UI thread (not the audio thread).
// The path is stored in the plugin state so DAW sessions remember it.
//==============================================================================
void StarterAmpProcessor::loadIR (const juce::File& file, int slot, bool allowAutoAlign)
{
    auto& convolution = conv[slot];

    // Trim::no keeps any silence at the start of the file. That matters here: the
    // time offset between two IRs is what the alignment works on, and trimming each
    // one separately would throw that offset away.
    convolution.loadImpulseResponse (file,
                              juce::dsp::Convolution::Stereo::no,
                              juce::dsp::Convolution::Trim::no,
                              0,   // 0 = use the whole file
                              juce::dsp::Convolution::Normalise::yes);

    apvts.state.setProperty ("irPath" + cabLetter (slot), file.getFullPathName(), nullptr);
    irActive[slot] = true;

    analyseIR (file, slot);

    if (allowAutoAlign && param ("cabAutoAlign") > 0.5f)
        alignIRs();
}

//==============================================================================
// v70: IR library support
//==============================================================================
void StarterAmpProcessor::auditionLoad (const juce::File& file)
{
    if (! file.existsAsFile())
        return;
    auditionConv.loadImpulseResponse (file, juce::dsp::Convolution::Stereo::no, juce::dsp::Convolution::Trim::no,
                                      0, juce::dsp::Convolution::Normalise::yes);
    auditionLoaded = true;
}

void StarterAmpProcessor::setAuditionActive (bool on)
{
    auditionOn = on;
}

void StarterAmpProcessor::sendIRToBlock (const juce::File& file, int slot)
{
    if (slot < 0 || slot >= kCabBlocks || ! file.existsAsFile())
        return;
    const auto s = cabLetter (slot);
    loadIR (file, slot, true);
    setParamValue (("cabSrc" + s).toRawUTF8(), 1.0f);   // "IR file"
    setParamValue (("cabOn" + s).toRawUTF8(), 1.0f);
}

juce::StringArray StarterAmpProcessor::builtinSpeakerNames() { return cabbuilder::speakerNames(); }

const std::vector<irlib::Curve>& StarterAmpProcessor::builtinSpeakerCurves()
{
    static const std::vector<irlib::Curve> curves = []
    {
        std::vector<irlib::Curve> v;
        for (int k = 0; k < cabbuilder::kNumSpeakers; ++k)
        {
            cabbuilder::Params p;
            p.speaker = k;
            p.mic = 5;               // Flat (no mic)
            p.cabinet = 3;           // 4x12 Closed
            p.position = 0.3f;
            p.distanceIn = 0.0f;
            irlib::Curve c {};
            irlib::curveFromPower ([&p] (double f) { return std::pow (10.0, cabbuilder::responseDb (p, f) / 10.0); }, c.data());
            v.push_back (c);
        }
        return v;
    }();
    return curves;
}

//==============================================================================
// Built-in cab models
//==============================================================================
void StarterAmpProcessor::parameterChanged (const juce::String& id, float)
{
    // May be called from any thread (host automation), so only note what changed.
    if (id == "cabRoom" || id == "cabRoomType" || id == "cabMeasured")
    {
        for (auto& d : slotDirty)
            d = true;
    }
    else
    {
        const int slot = juce::jlimit (0, kCabBlocks - 1, (int) (id.getLastCharacter() - 'A'));
        slotDirty[slot] = true;
        if (id.startsWith ("cabSrc"))
            slotForce[slot] = true;
    }
    triggerAsyncUpdate();
}

void StarterAmpProcessor::handleAsyncUpdate()
{
    asyncBusy.store (true);
    // A Program Change arrived on the audio thread; loading a preset has to happen here.
    const int program = pendingProgram.exchange (-1);
    if (program >= 0)
    {
        loadFactoryPreset (program);
        ++presetLoadCounter;
    }

    if (latencyDirty.exchange (false))
        setLatencySamples (osLatencySamples());

    if (namDirty.exchange (false))
        reloadNamFromState();

    for (int slot = 0; slot < kCabBlocks; ++slot)
        if (slotDirty[slot].exchange (false))
            refreshSlot (slot, slotForce[slot].exchange (false));
    asyncBusy.store (false);
}

void StarterAmpProcessor::refreshAllSlots()
{
    for (int slot = 0; slot < kCabBlocks; ++slot)
        refreshSlot (slot, true);
}

// Loads whatever this slot is set to use: a freshly built model, or its IR file.
// 'force' is false when only a model control changed, so a slot using a file is left alone.
void StarterAmpProcessor::refreshSlot (int slot, bool force)
{
    const juce::String s = cabLetter (slot);
    auto get = [this, &s] (const char* base) { return apvts.getRawParameterValue (base + s)->load(); };
    const bool useModel = (int) get ("cabSrc") == 0;

    if (! useModel && ! force)
        return;

    auto& convolution = conv[slot];

    if (useModel)
    {
        cabbuilder::Params p;
        p.speaker    = (int) get ("cabSpk");
        p.mic        = (int) get ("cabMic");
        p.position   = get ("cabPos");
        p.distanceIn = get ("cabDist");
        p.angleDeg   = get ("cabAng");
        p.cabinet    = (int) get ("cabCbn");
        p.tight      = get ("cabTight");
        p.bite       = get ("cabBite");
        p.box        = get ("cabBox");
        p.room       = param ("cabRoom");
        p.roomType   = (int) param ("cabRoomType");
        p.measured   = param ("cabMeasured") > 0.5f;   // v71

        const auto ir = cabbuilder::buildIR (p);

        juce::AudioBuffer<float> buffer (1, (int) ir.size());
        std::copy (ir.begin(), ir.end(), buffer.getWritePointer (0));
        convolution.loadImpulseResponse (std::move (buffer), kIRAnalysisRate,
                                  juce::dsp::Convolution::Stereo::no,
                                  juce::dsp::Convolution::Trim::no,
                                  juce::dsp::Convolution::Normalise::yes);
        irActive[slot] = true;

        // For the IR display and the alignment: unit energy, like the convolution's Normalise
        std::vector<float> snapshot (ir.begin(), ir.begin() + (std::ptrdiff_t) std::min<size_t> (ir.size(), 4096));
        double energy = 0.0;
        for (float v : snapshot)
            energy += (double) v * (double) v;
        if (energy > 1.0e-12)
            for (auto& v : snapshot)
                v *= (float) (1.0 / std::sqrt (energy));
        {
            const juce::ScopedLock lock (irLock);
            irAnalysis[slot] = std::move (snapshot);
        }
        ++irVersion;
    }
    else
    {
        const juce::File file (apvts.state.getProperty ("irPath" + cabLetter (slot)).toString());
        if (file.existsAsFile())
        {
            loadIR (file, slot);
        }
        else
        {
            irActive[slot] = false;
            {
                const juce::ScopedLock lock (irLock);
                irAnalysis[slot].clear();
            }
            ++irVersion;
        }
    }
}

// Reads the start of the IR file again, as 48 kHz mono, for the alignment and the IR display.
void StarterAmpProcessor::analyseIR (const juce::File& file, int slot)
{
    constexpr int maxSamples = 4096;   // about 85 ms at 48 kHz

    std::vector<float> result;

    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader != nullptr && reader->lengthInSamples > 0 && reader->sampleRate > 0.0 && reader->numChannels > 0)
    {
        const int numRead = (int) std::min<juce::int64> (reader->lengthInSamples,
                                                         (juce::int64) (reader->sampleRate * 0.15));

        juce::AudioBuffer<float> tmp ((int) reader->numChannels, numRead + 16);   // a little padding at the end
        tmp.clear();
        reader->read (&tmp, 0, numRead, 0, true, true);

        const double ratio = reader->sampleRate / kIRAnalysisRate;
        const int outN = std::min (maxSamples, (int) std::floor ((double) numRead / ratio));

        if (outN > 0)
        {
            result.assign ((size_t) outN, 0.0f);

            if (std::abs (ratio - 1.0) < 1.0e-6)
                std::copy (tmp.getReadPointer (0), tmp.getReadPointer (0) + outN, result.begin());
            else
            {
                juce::LagrangeInterpolator interpolator;
                interpolator.process (ratio, tmp.getReadPointer (0), result.data(), outN);
            }

            // Scale to unit energy, like the Normalise option of the convolution
            double energy = 0.0;
            for (float v : result) energy += (double) v * (double) v;
            if (energy > 1.0e-12)
            {
                const float scale = (float) (1.0 / std::sqrt (energy));
                for (auto& v : result) v *= scale;
            }
        }
    }

    {
        const juce::ScopedLock lock (irLock);
        irAnalysis[slot] = std::move (result);
    }
    ++irVersion;
}

std::vector<float> StarterAmpProcessor::getIRSnapshot (int slot) const
{
    const juce::ScopedLock lock (irLock);
    return irAnalysis[juce::jlimit (0, kCabBlocks - 1, slot)];
}

int StarterAmpProcessor::factoryPresetCount() { return factory::kNumPresets; }
juce::StringArray StarterAmpProcessor::factoryPresetTags (int index)
{
    juce::StringArray tags;
    if (! juce::isPositiveAndBelow (index, factory::kNumPresets))
        return tags;
    const auto& preset = factory::kPresets[index];
    tags.add (preset.category);
    auto get = [&preset] (const char* id, float fallback)
    {
        for (int i = 0; i < preset.numEntries; ++i)
            if (std::strcmp (preset.entries[i].id, id) == 0)
                return preset.entries[i].value;
        return fallback;
    };
    static const char* const ampTags[16] = { "Clean", "Clean", "Crunch", "Lead", "High Gain", "Acoustic", "Classic Rock", "Lead", "High Gain", "Crunch", "Clean", "Lead", "High Gain", "High Gain", "High Gain", "Classic Rock" };
    const int model = juce::jlimit (0, 15, (int) std::lround (get ("ampModel", 0.0f)));
    tags.addIfNotAlreadyThere (ampTags[model]);
    if (get ("fuzzOn", 0.0f) > 0.5f || get ("fuzz2On", 0.0f) > 0.5f)   tags.add ("Fuzz");
    if (get ("wahOn", 0.0f) > 0.5f)    tags.add ("Wah");
    if (get ("wowOn", 0.0f) > 0.5f || get ("octOn", 0.0f) > 0.5f || get ("tpOn", 0.0f) > 0.5f) tags.add ("Pitch");
    if (get ("phaserOn", 0.0f) > 0.5f || get ("chorusOn", 0.0f) > 0.5f || get ("tremOn", 0.0f) > 0.5f || get ("wobOn", 0.0f) > 0.5f) tags.add ("Modulation");
    if (get ("shimmerOn", 0.0f) > 0.5f || get ("granOn", 0.0f) > 0.5f || (get ("reverbOn", 1.0f) > 0.5f && get ("reverbMix", 0.0f) >= 0.3f)) tags.add ("Ambient");
    if (get ("postDelayOn", 0.0f) > 0.5f || get ("preDelayOn", 0.0f) > 0.5f) tags.add ("Delay");
    return tags;
}

juce::String StarterAmpProcessor::factoryPresetName (int i)     { return juce::isPositiveAndBelow (i, factory::kNumPresets) ? juce::String (factory::kPresets[i].name) : juce::String(); }
juce::String StarterAmpProcessor::factoryPresetCategory (int i) { return juce::isPositiveAndBelow (i, factory::kNumPresets) ? juce::String (factory::kPresets[i].category) : juce::String(); }

void StarterAmpProcessor::loadFactoryPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, factory::kNumPresets))
        return;

    // Settings about the machine or the session, not the sound, are left as the user has them.
    static const char* const keep[] = { "inputMode", "perfMode", "tempoBpm", "tempoHost", "ampQuality", "outMono", "limOn", "limCeil", "limRel",
                                     "metroOn", "metroBeats", "metroSub", "metroSound", "metroVol", "metroAccent", "tunerRef" };
    for (auto* prm : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
        {
            bool skip = false;
            for (auto* k : keep) skip = skip || rp->paramID == k;
            if (! skip)
                rp->setValueNotifyingHost (rp->getDefaultValue());
        }

    resetFxOrder();                                    // factory sounds are made for the standard order
    apvts.state.setProperty ("presetTags", juce::String(), nullptr);
    const auto& preset = factory::kPresets[index];
    for (int i = 0; i < preset.numEntries; ++i)
        if (auto* rp = apvts.getParameter (preset.entries[i].id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (preset.entries[i].value));
        else
            jassertfalse;
}

juce::String StarterAmpProcessor::exportCabBlend (const juce::File& file)
{
    using F  = juce::dsp::IIR::Filter<float>;
    const double sr = kIRAnalysisRate;

    std::vector<float> irs[kCabBlocks];
    int numActive = 0;
    size_t maxLen = 0;
    {
        const juce::ScopedLock lock (irLock);
        for (int blk = 0; blk < kCabBlocks; ++blk)
            if (param (kCabIds[blk].on) > 0.5f && irAnalysis[blk].size() >= 64)
            {
                irs[blk] = irAnalysis[blk];
                maxLen = std::max (maxLen, irs[blk].size());
                ++numActive;
            }
    }
    if (numActive == 0)
        return "Nothing to export: switch on at least one cab block that has an IR.";

    const size_t len = maxLen + 1024;
    std::vector<float> left (len, 0.0f), right (len, 0.0f);
    const float norm = 1.0f / std::sqrt ((float) numActive);
    const float spread = numActive > 1 ? param ("cabSpread") : 0.0f;
    static const float spreadPosition[kCabBlocks] = { -1.0f, 1.0f, -0.5f, 0.5f };

    auto runFilter = [] (F& f, std::vector<float>& v, juce::dsp::IIR::Coefficients<float>::Ptr coeffs)
    {
        f.coefficients = coeffs;
        f.reset();
        for (auto& s : v)
            s = f.processSample (s);
    };
    using Co = juce::dsp::IIR::Coefficients<float>;

    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        if (irs[blk].empty())
            continue;
        const auto& id = kCabIds[blk];
        std::vector<float> v (len, 0.0f);
        std::copy (irs[blk].begin(), irs[blk].end(), v.begin());

        F f;
        if (param (id.lowCut)  > 20.5f)    runFilter (f, v, Co::makeHighPass (sr, param (id.lowCut)));
        if (param (id.highCut) < 19900.0f) runFilter (f, v, Co::makeLowPass  (sr, std::min (param (id.highCut), 0.45f * (float) sr)));
        static const float hz[5] = { 110.0f, 330.0f, 1000.0f, 2800.0f, 6500.0f };
        static const float qq[5] = { 0.7f, 1.0f, 1.0f, 1.2f, 0.7f };
        for (int band = 0; band < 5; ++band)
        {
            const float g = dbToGain (param (id.eq[band]));
            if (std::abs (param (id.eq[band])) > 0.05f)
                runFilter (f, v, band == 0 ? Co::makeLowShelf (sr, hz[band], qq[band], g)
                               : band == 4 ? Co::makeHighShelf (sr, hz[band], qq[band], g)
                                           : Co::makePeakFilter (sr, hz[band], qq[band], g));
        }
        if (std::abs (param (id.tilt)) > 0.05f)
        {
            runFilter (f, v, Co::makeLowShelf  (sr, 800.0f, 0.707f, dbToGain (-0.5f * param (id.tilt))));
            runFilter (f, v, Co::makeHighShelf (sr, 800.0f, 0.707f, dbToGain ( 0.5f * param (id.tilt))));
        }
        if (std::abs (param (id.rso)) > 0.05f)
            runFilter (f, v, Co::makePeakFilter (sr, param ("cabResoHz"), 1.2f, dbToGain (param (id.rso))));
        if (param (id.phs) > 0.01f)
            runFilter (f, v, Co::makeAllPass (sr, std::min (700.0f * std::pow (2.0f, 3.0f * param (id.phs)), 0.45f * (float) sr), 0.6f));

        const int delay = (int) std::lround (param (id.delay) * 0.001f * (float) sr);
        const float w = norm * dbToGain (param (id.level)) * (param (id.pol) > 0.5f ? -1.0f : 1.0f);
        float gl, gr;
        panGains (juce::jlimit (-1.0f, 1.0f, param (id.pan) + spread * spreadPosition[blk]), gl, gr);
        for (size_t i = 0; i + (size_t) delay < len; ++i)
        {
            left [i + (size_t) delay] += w * gl * v[i];
            right[i + (size_t) delay] += w * gr * v[i];
        }
    }

    // Cab-wide filters
    for (auto* ch : { &left, &right })
    {
        F f;
        if (param ("cabLowCut")  > 20.5f)    runFilter (f, *ch, Co::makeHighPass (sr, param ("cabLowCut")));
        if (param ("cabHighCut") < 19900.0f) runFilter (f, *ch, Co::makeLowPass  (sr, std::min (param ("cabHighCut"), 0.45f * (float) sr)));
        if (std::abs (param ("cabTilt")) > 0.05f)
        {
            runFilter (f, *ch, Co::makeLowShelf  (sr, 800.0f, 0.707f, dbToGain (-0.5f * param ("cabTilt"))));
            runFilter (f, *ch, Co::makeHighShelf (sr, 800.0f, 0.707f, dbToGain ( 0.5f * param ("cabTilt"))));
        }
        if (std::abs (param ("cabReso")) > 0.05f) runFilter (f, *ch, Co::makePeakFilter (sr, param ("cabResoHz"), 1.2f, dbToGain (param ("cabReso"))));
        if (std::abs (param ("cabAir"))  > 0.05f) runFilter (f, *ch, Co::makeHighShelf (sr, 6000.0f, 0.707f, dbToGain (param ("cabAir"))));
    }

    float peak = 0.0f;
    for (size_t i = 0; i < len; ++i)
        peak = std::max ({ peak, std::abs (left[i]), std::abs (right[i]) });
    const float scale = peak > 0.98f ? 0.98f / peak : 1.0f;     // never write a clipped file
    for (size_t i = 0; i < len; ++i) { left[i] *= scale; right[i] *= scale; }

    juce::AudioBuffer<float> out (2, (int) len);
    out.copyFrom (0, 0, left.data(),  (int) len);
    out.copyFrom (1, 0, right.data(), (int) len);

    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> stream (file.createOutputStream());
    if (stream == nullptr)
        return "Could not write to " + file.getFullPathName();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> writer (wav.createWriterFor (stream.get(), sr, 2, 24, {}, 0));
    if (writer == nullptr)
        return "Could not create the WAV file.";
    stream.release();                                           // the writer owns the stream now
    writer->writeFromAudioSampleBuffer (out, 0, (int) len);
    return "Exported " + juce::String (numActive) + " block" + (numActive > 1 ? "s" : "") + " to " + file.getFileName()
           + " (stereo, 24-bit, 48 kHz, " + juce::String ((double) len / sr * 1000.0, 0) + " ms"
           + (scale < 1.0f ? ", level lowered to avoid clipping" : "") + ").";
}

juce::String StarterAmpProcessor::getAlignReport() const
{
    const juce::ScopedLock lock (irLock);
    return alignReport;
}

void StarterAmpProcessor::setAlignReport (const juce::String& text)
{
    {
        const juce::ScopedLock lock (irLock);
        alignReport = text;
    }
    ++irVersion;
}

void StarterAmpProcessor::setParamValue (const char* id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

//==============================================================================
// Phase alignment.
//
// Two mics on the same speaker hear it at slightly different times, and when
// the two recordings are mixed that offset (or an inverted polarity) makes
// some frequencies cancel, which sounds thin and hollow. To fix it:
//   1. Look at both IRs through a band-pass filter (150 Hz to 3.5 kHz), where
//      the cabinet's tone is.
//   2. Slide IR B against IR A by up to +/- 5 ms and measure how alike they
//      are at each position (cross-correlation).
//   3. The position with the strongest match gives the time offset. If the match
//      is negative, the IRs are upside down relative to each other, so B is inverted.
//   4. The earlier IR is delayed by the offset, so both arrive together.
// The result is written into the Delay and Invert controls, so you can fine-tune it.
//==============================================================================
bool StarterAmpProcessor::alignIRs()
{
    // The blocks that are on and have an IR. The first of them is the reference.
    std::vector<int> ids;
    std::vector<std::vector<float>> irs;
    {
        const juce::ScopedLock lock (irLock);
        for (int blk = 0; blk < kCabBlocks; ++blk)
            if (param (kCabIds[blk].on) > 0.5f && irAnalysis[blk].size() >= 64)
            {
                ids.push_back (blk);
                irs.push_back (irAnalysis[blk]);
            }
    }

    if (ids.size() < 2)
    {
        setAlignReport ("Turn on and load at least two cab blocks to align them.");
        return false;
    }

    // 1. Band-limit copies of all of them
    const double rate = kIRAnalysisRate;
    for (auto& v : irs)
    {
        juce::dsp::IIR::Filter<float> hp, lp;
        hp.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass (rate, 150.0f);
        lp.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass  (rate, 3500.0f);
        for (auto& s : v)
            s = lp.processSample (hp.processSample (s));
    }

    // 2. Slide every other block against the reference by up to +/- 5 ms (cross-correlation over ~21 ms)
    const auto& ref = irs[0];
    const int maxLag = (int) (0.005 * rate);

    struct Result { bool matched = false; int lag = 0; double correlation = 0.0; };
    std::vector<Result> results (ids.size());
    results[0].matched = true;

    for (size_t k = 1; k < ids.size(); ++k)
    {
        const auto& other = irs[k];
        const int window = (int) std::min<size_t> ({ 1024, ref.size(), other.size() });

        double energyA = 0.0, energyB = 0.0;
        for (int n = 0; n < window; ++n)
        {
            energyA += (double) ref[(size_t) n] * ref[(size_t) n];
            energyB += (double) other[(size_t) n] * other[(size_t) n];
        }
        const double norm = std::sqrt (energyA * energyB);
        if (norm < 1.0e-12)
            continue;

        int bestLag = 0;
        double best = 0.0;
        for (int lag = -maxLag; lag <= maxLag; ++lag)
        {
            double sum = 0.0;
            for (int n = 0; n < window; ++n)
            {
                const int j = n + lag;
                if (j >= 0 && j < (int) other.size())
                    sum += (double) ref[(size_t) n] * (double) other[(size_t) j];
            }
            const double cc = sum / norm;
            if (std::abs (cc) > std::abs (best))
            {
                best = cc;
                bestLag = lag;
            }
        }

        if (std::abs (best) >= 0.15)
        {
            results[k].matched = true;
            results[k].lag = bestLag;      // positive = this block arrives later than the reference
            results[k].correlation = best;
        }
    }

    // 3. Everything that matched is lined up with the latest arrival: block k gets a delay of (latest - its arrival)
    int latest = 0;
    int numMatched = 0;
    for (auto& r : results)
        if (r.matched)
        {
            latest = std::max (latest, r.lag);
            ++numMatched;
        }

    if (numMatched < 2)
    {
        setAlignReport ("No clear match between the blocks (best match below 0.15), so the settings were left alone.");
        return false;
    }

    juce::String text = "Aligned to block " + cabLetter (ids[0]) + ": ";
    for (size_t k = 0; k < ids.size(); ++k)
    {
        const int blk = ids[k];
        const auto& r = results[k];
        const juce::String letter = cabLetter (blk);

        if (! r.matched)
        {
            text << letter << " no clear match (left alone); ";
            continue;
        }

        const float delayMs = (float) (1000.0 * (latest - r.lag) / rate);
        const bool  invert  = r.correlation < 0.0;
        setParamValue (kCabIds[blk].delay, delayMs);
        setParamValue (kCabIds[blk].pol, invert ? 1.0f : 0.0f);

        text << letter << " " << juce::String (delayMs, 2) << " ms" << (invert ? " inverted" : "");
        if (k > 0)
            text << " (match " << juce::String (std::abs (r.correlation), 2) << ")";
        text << "; ";
    }
    setAlignReport (text.trimCharactersAtEnd ("; ") + ".");
    return true;
}

juce::String StarterAmpProcessor::getIRName (int slot) const
{
    return juce::File (apvts.state.getProperty ("irPath" + cabLetter (slot)).toString())
               .getFileName();
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StarterAmpProcessor();
}


//==============================================================================
// NAM model loading (message thread)
//==============================================================================
const char* StarterAmpProcessor::namPathKey (int slot)
{
    static const char* keys[kNamSlots] = { "namPath", "namPathB", "namPathP1", "namPathP2" };
    return keys[juce::jlimit (0, kNamSlots - 1, slot)];
}

bool StarterAmpProcessor::loadNamFile (int slot, const juce::File& file, juce::String& message)
{
    slot = juce::jlimit (0, kNamSlots - 1, slot);
    juce::String err;
    auto model = NamLite::loadFile (file, err);
    if (model == nullptr)
    {
        message = err;
        return false;
    }
    const auto info = model->info;
    const double rate = currentSampleRate > 0 ? currentSampleRate : 48000.0;
    auto player = std::make_unique<NamLite::Player> (std::move (model), rate, namMaxBlock);
    std::unique_ptr<NamLite::Player> second;
    if (slot == 3)                       // the pedal after the cab can run one player per side
    {
        juce::String err2;
        if (auto m2 = NamLite::loadFile (file, err2))
            second = std::make_unique<NamLite::Player> (std::move (m2), rate, namMaxBlock);
    }
    namSlot[slot].install (std::move (player), std::move (second), info.hasLoudness ? -18.0f - info.loudnessDb : 0.0f);
    apvts.state.setProperty (namPathKey (slot), file.getFullPathName(), nullptr);
    juce::String s = (info.name.isNotEmpty() ? info.name : file.getFileNameWithoutExtension());
    if (info.gear.isNotEmpty()) s << " (" << info.gear << ")";
    s << ": " << info.architecture << ", " << juce::String (info.parameters) << " parameters, " << juce::String (info.sampleRate / 1000.0, 1) << " kHz";
    if (info.hasLoudness) s << ", loudness " << juce::String (info.loudnessDb, 1) << " dB";
    if (std::abs (rate - info.sampleRate) > 0.5) s << ". Resampling from " << juce::String (rate / 1000.0, 1) << " kHz";
    namSlot[slot].summary = s;
    message = s;
    return true;
}

void StarterAmpProcessor::clearNam (int slot)
{
    slot = juce::jlimit (0, kNamSlots - 1, slot);
    namSlot[slot].install (nullptr, nullptr, 0.0f);
    apvts.state.removeProperty (namPathKey (slot), nullptr);
    namSlot[slot].summary = {};
}

void StarterAmpProcessor::reloadNamFromState()
{
    for (int slot = 0; slot < kNamSlots; ++slot)
    {
        const juce::String path = apvts.state.getProperty (namPathKey (slot)).toString();
        if (path.isEmpty())
        {
            namSlot[slot].install (nullptr, nullptr, 0.0f);
            namSlot[slot].summary = {};
            continue;
        }
        juce::String msg;
        if (! loadNamFile (slot, juce::File (path), msg))
        {
            namSlot[slot].install (nullptr, nullptr, 0.0f);
            namSlot[slot].summary = "Could not load " + juce::File (path).getFileName() + ": " + msg;
        }
    }
}
