#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

// b107: two modulators that move knobs. Each makes one control signal once per audio block (so it is block-rate, about 10 ms at 512 samples)
// and adds it, scaled by an Amount, to up to three target knobs. The knob you set stays the base: the modulation is added on top of it
// in the knob's own travel (0 to 1 along its range, so a frequency knob moves in octaves, not in Hz). Nothing is stored in the knob itself.
namespace ModSource
{
    enum Source { Off = 0, Envelope, Sine, Triangle, Steps, Glide, Sequence, kSources };     // b120: Sequence (a 16-step pattern in time with the tempo)
    static constexpr int kModulators = 2, kDestinations = 3;

    // b120: the Sequence source. A step lasts one of these note values; the pattern is 16, 12, 8, 6 or 4 steps long.
    static constexpr double kDivBeats[6] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    static constexpr int kLens[5] = { 16, 12, 8, 6, 4 };
    static constexpr int kMaxSteps = 16;

    struct Target { const char* id; const char* label; };

    // The knobs a modulator can move (a choice's index is this index + 1; 0 is None). Continuous knobs only.
    static constexpr Target kTargets[] =
    {
        { "inGain",          "Input level" },
        { "ampGain",         "Amp A: Gain" },
        { "ampALevel",       "Amp A: Level" },
        { "ampAPan",         "Amp A: Pan" },
        { "bass",            "Amp: Bass" },
        { "treble",          "Amp: Treble" },
        { "presence",        "Amp: Presence" },
        { "ampBGain",        "Amp B: Gain" },
        { "ampBLevel",       "Amp B: Level" },
        { "ampBPan",         "Amp B: Pan" },
        { "fuzzGain",        "Fuzz: Gain" },
        { "fuzzTone",        "Fuzz: Tone" },
        { "fuzzMix",         "Fuzz: Mix" },
        { "fuzzLevel",       "Fuzz: Level" },
        { "odGain",          "Drive 1: Gain" },
        { "odTone",          "Drive 1: Tone" },
        { "od2Gain",         "Drive 2: Gain" },
        { "compMix",         "Compressor: Mix" },
        { "compThresh",      "Compressor: Threshold" },
        { "bsGain",          "Boost: Gain" },
        { "bsLevel",         "Boost: Level" },
        { "wahPos",          "Wah: Pedal position" },
        { "wowPos",          "Pitch pedal: Position" },
        { "filtFreq",        "Filter: Frequency" },
        { "filtRes",         "Filter: Resonance" },
        { "filtMix",         "Filter: Mix" },
        { "chorusMix",       "Chorus: Mix" },
        { "chorusDepth",     "Chorus: Depth" },
        { "chorusRate",      "Chorus: Rate" },
        { "tremDepth",       "Tremolo: Depth" },
        { "tremRate",        "Tremolo: Rate" },
        { "mfADepth",        "Mod A: Depth" },
        { "mfAMix",          "Mod A: Mix" },
        { "mfARate",         "Mod A: Rate" },
        { "mfBDepth",        "Mod B: Depth" },
        { "mfBMix",          "Mod B: Mix" },
        { "mfBRate",         "Mod B: Rate" },
        { "mfCDepth",        "Mod C: Depth" },
        { "mfCMix",          "Mod C: Mix" },
        { "mfCRate",         "Mod C: Rate" },
        { "mfDDepth",        "Mod D: Depth" },
        { "mfDMix",          "Mod D: Mix" },
        { "mfDRate",         "Mod D: Rate" },
        { "preDelayMix",     "Delay 1: Mix" },
        { "preDelayFeedback","Delay 1: Feedback" },
        { "postDelayMix",    "Delay 2: Mix" },
        { "postDelayFeedback","Delay 2: Feedback" },
        { "reverbMix",       "Reverb: Mix" },
        { "reverbSize",      "Reverb: Decay" },
        { "reverbDamp",      "Reverb: Damp" },
        { "cabHighCut",      "Cab: High cut" },
        { "cabLowCut",       "Cab: Low cut" },
        { "repMix",          "Repeater: Mix" },
        { "repGlitch",       "Repeater: Glitch" },
        { "repPitch",        "Repeater: Pitch per repeat" },
    };
    static constexpr int kNumTargets = (int) (sizeof (kTargets) / sizeof (kTargets[0]));

    // One modulator's state. step() is called once per block.
    struct Modulator
    {
        double sr = 48000.0;
        float phase = 0.0f, env = 0.0f, raw = 0.0f, target = 0.0f, out = 0.0f;
        std::uint32_t seed = 22222u;

        void prepare (double sampleRate) { sr = sampleRate; reset(); }
        void reset() { phase = 0.0f;  env = 0.0f;  raw = 0.0f;  target = 0.0f;  out = 0.0f; }

        float rnd()
        {
            seed = seed * 1664525u + 1013904223u;
            return (float) (seed >> 8) / 8388608.0f - 1.0f;
        }

        int seqIdx = 0;

        // b120: the Sequence source. steps: the pattern (-1 to 1). beat: the position in quarter notes at the start of the block.
        float stepSeq (const float* steps, int len, double beatsPerStep, double beat, float smooth, int n)
        {
            const double pos = beat / beatsPerStep;
            long k = (long) std::floor (pos);
            seqIdx = (int) (((k % len) + len) % len);
            const float dt = (float) n / (float) sr;
            const float tau = 0.002f + 0.5f * smooth * smooth;
            out += (1.0f - std::exp (-dt / tau)) * (steps[seqIdx] - out);
            return out;
        }

        // type: a Source. rateHz: LFO and random speed. smooth: 0 to 1 (envelope release, and a glide on the others).
        // inPeak: the largest sample of the block. n: the block length. Returns 0 to 1 for the Envelope and -1 to 1 for the others.
        float step (int type, float rateHz, float smooth, float inPeak, int n)
        {
            const float dt = (float) n / (float) sr;
            float v = 0.0f;
            if (type == Envelope)
            {
                // -10 dBFS (the level the input calibration aims at for hard playing) is full scale for the modulator
                const float x = std::min (1.0f, inPeak / 0.316f);
                const float rel = 0.02f * std::pow (75.0f, smooth);            // 20 ms to 1.5 s
                env = std::max (x, env * std::exp (-dt / rel));
                v = env;
            }
            else if (type >= Sine)
            {
                phase += rateHz * dt;
                bool wrapped = false;
                while (phase >= 1.0f) { phase -= 1.0f;  wrapped = true; }
                if (type == Sine)          v = std::sin (6.2831853f * phase);
                else if (type == Triangle) v = 4.0f * std::abs (phase - 0.5f) - 1.0f;
                else
                {
                    if (wrapped) target = rnd();
                    if (type == Steps) v = target;
                    else { raw += std::min (1.0f, 3.0f * rateHz * dt) * (target - raw);  v = raw; }
                }
            }
            if (type == Envelope) { out = v;  return out; }
            const float tau = 0.002f + 0.5f * smooth * smooth;
            out += (1.0f - std::exp (-dt / tau)) * (v - out);
            return out;
        }
    };
}
