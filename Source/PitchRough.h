#pragma once
// b78: the pitch pedal's "bad tracking" characters. A deliberately crude pitch shifter, the kind inside the early digital pitch pedals:
//   two taps on a delay line whose length sweeps (a sawtooth), cross-faded with squared-sine windows. There is no waveform matching, so you get the
//   warble at the sweep rate, a slightly wrong pitch on some notes, and, with the options below, the grit of low-resolution converters.
//   Characters: 1 Grainy Sweep (a 32 ms sweep, a lower sample rate, 13 to 9 bit, a dark top), 2 Glitchy (a 64 ms sweep, with random wrong-pitch jumps,
//   dropouts and slurs), 3 Abrasive (a 16 ms sweep, hard-driven, rectified, 10 to 5 bit, no filtering), 4 Noisy (a 32 ms sweep, hiss that follows your playing, a drifting pitch).
//   b97: 5 Warble, the slow soft wobble of a long shifter window (380 ms at Grit 0, 110 ms at Grit 1), no damage stages. b118: a long window means a long lag (half the window, 55 to 190 ms), and the wobble rate (2 x |ratio - 1| / window) is tied to it, so Warble now shifts with a short 12 ms waveform-matched two-tap sweep (about 6 ms of lag) and the swell and fade is put in at the rate the long window would have given. windowMs() for Warble is that wobble window, not a delay.
//   A shifted note swells and fades at 2 x |ratio - 1| / window (8 Hz for an octave up at 250 ms), partly from the two taps interfering and partly from an
//   explicit dip at the crossover (so every note wobbles, not only those that line up badly with the window). It reports no latency, so the shifted
//   copy trails the dry one by up to half the window; that looseness is part of the sound.
//   b117: partial waveform matching, so the first four characters are a shifted note with grit rather than a ring-modulated one. With no matching, the two taps
//   are a half-window apart whatever the note is, so a note whose period does not divide that gap adds up out of step and the level pumps at the cross-fade rate
//   (5 to 14 dB on a plain tone: the ring-mod feel). Now each time a tap restarts (while it is silent), it may move later by up to 10 ms to the position where
//   its recent past lines up best with the other tap (a normalised correlation over the last 15 ms, preferring the nearest match). Not every restart is lined up: 90 percent at Grit 0, 70 percent
//   at Grit 0.5, 40 percent at Grit 1, the rest are left as the old sweep had them, so you still get the odd wrong splice (the middle between the old rough
//   sweep and a fully matched shifter). Moving only part of the way was tried first and did not work: half a move can be worse than none. Warble keeps its own wobble on purpose.
//   The cost is up to a few ms more delay on a low note, which is not reported (the dry path is matched to the average only).
//   All of this is my own design from the general character of those pedals, NOT measured from any unit. Grit (0 to 1) scales how bad it gets.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

class RoughShifter
{
public:
    struct Params { float semis = 0.0f; int character = 1; float grit = 0.5f; };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        buf.assign ((size_t) (0.42 * sampleRate) + 16, 0.0f);
        refLen = std::max (16, (int) (0.015 * sampleRate / kStride));
        ref.assign ((size_t) refLen, 0.0f);
        reset();
    }
    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        wp = 0;  phase = 0.0f;  phaseV = 0.0f;  holdPhase = 0.0f;  held = 0.0f;  lp1 = lp2 = 0.0f;  dcX = dcY = 0.0f;  env = 0.0f;
        offA = offB = 0.0f;  prevPhaseB = 0.5f;
        winEff = 0.0f;  rng = 0x9E3779B9u;  evTimer = (int) (0.2f * sr);  evKind = 0;  evLeft = 0;  evSemis = 0.0f;  mute = 1.0f;  drift = 0.0f;  driftT = 0.0f;  lagSemis = 0.0f;  qSemis = 0.0f;
    }
    static float windowMs (int character, float grit = 0.5f)
    {
        if (character >= 5) return 380.0f - 270.0f * std::min (1.0f, std::max (0.0f, grit));
        return character == 2 ? 64.0f : character == 3 ? 16.0f : 32.0f;
    }
    // The average delay of the shifter (half the sweep), which the caller matches in the dry path. Warble reports none (see above).
    int latency (int character) const { return character >= 5 ? 0 : (int) (0.5f * windowMs (character) * 0.001f * sr); }

    float process (float x, const Params& p)
    {
        const float grit = std::min (1.0f, std::max (0.0f, p.grit));
        const int ch = std::min (5, std::max (1, p.character));
        float winS = windowMs (ch, grit) * 0.001f * sr;
        if (ch == 5)                        // the Grit knob moves the window: glide it so turning the knob does not click
        {
            if (winEff <= 0.0f) winEff = winS;
            winEff += 0.0005f * (winS - winEff);
            winS = winEff;
        }

        // ---- pitch: the pedal position, then what each character does to it
        float semis = p.semis;
        if (ch == 1 || ch == 4)             // the old pedals read the treadle with a coarse converter: the pitch moves in steps
        {
            const float step = 0.02f + 0.45f * grit * grit;
            qSemis = std::floor (semis / step + 0.5f) * step;
            semis = qSemis;
        }
        if (ch == 4)                        // a slow random drift, up to about 25 cents at full Grit
        {
            if (--driftT <= 0.0f) { driftT = 0.12f * sr;  driftTarget = (rnd() - 0.5f) * 2.0f * 0.25f * grit; }
            drift += 0.0004f * (driftTarget - drift);
            semis += drift;
        }
        if (ch == 2)                        // random events: a wrong-pitch jump, a dropout, or a slur (the pitch stops following the pedal for a moment)
        {
            if (evLeft > 0) --evLeft; else evKind = 0;
            if (--evTimer <= 0)
            {
                const float u = rnd();
                evTimer = (int) ((0.08f + 0.5f * rnd()) / (0.15f + 1.2f * grit) * sr);
                evLeft = (int) ((0.02f + 0.09f * rnd()) * sr);
                evKind = u < 0.45f ? 1 : u < 0.75f ? 2 : 3;
                evSemis = (rnd() < 0.5f ? -1.0f : 1.0f) * std::floor (1.0f + 11.0f * rnd());
                lagSemis = semis;
            }
            if (evKind == 1) semis += evSemis * (0.4f + 0.6f * grit);
            else if (evKind == 3) semis = lagSemis;
            const float targetMute = (evKind == 2) ? 0.0f : 1.0f;
            mute += (targetMute - mute) * (targetMute < mute ? 0.01f : 0.004f);       // about 2 ms down, 5 ms up
        }
        const float ratio = std::exp2 (semis / 12.0f);

        // ---- the two-tap sweep
        buf[(size_t) wp] = x;
        const float tw = ch == 5 ? 0.012f * sr : winS;                  // Warble: the taps use a short 12 ms sweep (b118); winS is then only the wobble period
        const float oldPhase = phase;
        phase += (1.0f - ratio) / tw;
        phase -= std::floor (phase);
        if (ch == 5) { phaseV += (1.0f - ratio) / winS;  phaseV -= std::floor (phaseV); }
        const float phB = std::fmod (phase + 0.5f, 1.0f);
        {
            const float m = ch == 5 ? 1.0f : std::max (0.0f, 0.9f - 0.5f * grit);         // the share of restarts that are lined up (the rest are left rough, as in the old sweep)
            if (std::fabs (phase - oldPhase) > 0.5f)   offA = rnd() < m ? matchOffset (phB * tw + offB, phase * tw) : 0.0f;      // tap A just restarted: line it up with tap B
            if (std::fabs (phB - prevPhaseB) > 0.5f)   offB = rnd() < m ? matchOffset (phase * tw + offA, phB * tw) : 0.0f;     // tap B just restarted: line it up with tap A
        }
        prevPhaseB = phB;
        const float dA = phase * tw + offA, dB = phB * tw + offB;
        const float sA = std::sin (3.14159265f * phase), sB = std::cos (3.14159265f * phase);
        float y = tap (dA) * (sA * sA) + tap (dB) * (sB * sB);
        if (++wp >= (int) buf.size()) wp = 0;
        if (ch == 5)
        {
            // (b118: the taps now sweep 12 ms and are lined up, so the wobble is all this explicit dip, driven by phaseV, the phase of the long wobble window.)
            // The two taps only interfere when the note lines up with the window (a half-window that is a whole number of periods adds up in phase,
            // so some notes would not wobble at all). So the dip is also put in explicitly, at the moment the taps cross over: a note-independent
            // swell and fade at the crossfade rate, none at zero shift. Makeup gain keeps the average level near the dry level.
            const float s2 = std::sin (6.2831853f * phaseV);
            const float dip = 0.8f * std::min (1.0f, std::fabs (semis) * 0.5f);
            return y * (1.0f - dip * s2 * s2) * (1.0f + 0.3f * std::min (1.0f, std::fabs (semis) * 0.5f));
        }

        // ---- the character's own damage
        if (ch == 3)
        {
            const float drive = 1.0f + 7.0f * grit;
            y = std::tanh (y * drive * 1.6f) * 0.7f;
            const float rect = std::fabs (y);
            y = y + grit * 0.9f * (rect - 0.5f * 0.7f);                                 // half-wave-ish rectifying: octave-up junk
        }
        // sample-rate reduction by sample and hold
        {
            const float hold = (ch == 3 ? 1.0f + 2.2f * grit : ch == 2 ? 1.0f + 0.5f * grit : 1.0f + 0.9f * grit);
            holdPhase += 1.0f / hold;
            if (holdPhase >= 1.0f) { holdPhase -= 1.0f;  held = y; }
            y = held;
        }
        // bit depth
        {
            const float bits = ch == 3 ? 10.0f - 5.0f * grit : ch == 2 ? 14.0f - 3.0f * grit : ch == 4 ? 12.0f - 3.0f * grit : 13.0f - 4.0f * grit;
            const float q = std::exp2 (bits - 1.0f);
            float d = 0.0f;
            if (ch != 3) d = (rnd() - rnd()) * 0.5f;                                      // triangular dither except on the Abrasive
            y = (std::floor (y * q + d + 0.5f)) / q;
        }
        // noise that follows the playing
        if (ch == 4)
        {
            env += (std::fabs (x) - env) * (std::fabs (x) > env ? 0.01f : 0.0006f);
            const float n = (rnd() - 0.5f) * 2.0f;
            y += n * (0.0008f + (0.012f + 0.06f * grit) * std::sqrt (std::min (1.0f, env * 4.0f)));
        }
        // the dark top of the old converters
        if (ch != 3)
        {
            const float fc = ch == 1 ? 9000.0f - 4200.0f * grit : ch == 4 ? 7500.0f - 3000.0f * grit : 12000.0f - 4000.0f * grit;
            const float a = 1.0f - std::exp (-6.2831853f * fc / sr);
            lp1 += a * (y - lp1);  lp2 += a * (lp1 - lp2);
            y = lp2;
        }
        // keep the DC out (the rectifier and the quantiser can leave some)
        {
            const float o = y - dcX + 0.995f * dcY;
            dcX = y;  dcY = o;  y = o;
        }
        if (ch == 2) y *= mute;
        return y;
    }

private:
    static constexpr int kStride = 3;
    // The extra delay (samples, 0 to 10 ms) that makes the signal a tap with this nominal delay would read best match the signal the other tap is reading.
    // dOther is the other tap's full delay now, dNew this tap's nominal delay after its restart.
    float matchOffset (float dOther, float dNew)
    {
        const int n = (int) buf.size();
        const int range = (int) (0.010f * sr);
        const int step = 2;
        if ((int) dOther + refLen * kStride + 4 >= n || (int) dNew + range + refLen * kStride + 4 >= n) return 0.0f;
        double refE = 0.0;
        for (int k = 0; k < refLen; ++k)
        {
            int i = wp - (int) dOther - k * kStride;  i %= n;  if (i < 0) i += n;
            ref[(size_t) k] = buf[(size_t) i];  refE += (double) ref[(size_t) k] * ref[(size_t) k];
        }
        if (refE < 1.0e-9) return 0.0f;                                        // nothing to line up with
        double best = -1.0e30;  int bestD = 0;
        for (int d = 0; d <= range; d += step)
        {
            double dot = 0.0, e = 0.0;
            for (int k = 0; k < refLen; ++k)
            {
                int i = wp - ((int) dNew + d) - k * kStride;  i %= n;  if (i < 0) i += n;
                const double v = buf[(size_t) i];
                dot += v * ref[(size_t) k];  e += v * v;
            }
            const double score = dot / std::sqrt (e * refE + 1.0e-12) - 0.04 * (double) d / (double) range;      // prefer the nearest good match: less delay
            if (score > best) { best = score;  bestD = d; }
        }
        return (float) bestD;
    }
    float tap (float d) const
    {
        const int n = (int) buf.size();
        float r = (float) wp - d;
        while (r < 0.0f) r += (float) n;
        // b91: adding n to a tiny negative r can round to exactly n (float), which indexed one past the end (found by AddressSanitizer)
        if (r >= (float) n) r -= (float) n;
        const int i0 = std::min ((int) r, n - 1), i1 = (i0 + 1 >= n) ? 0 : i0 + 1;
        const float f = std::max (0.0f, r - (float) i0);
        return buf[(size_t) i0] + f * (buf[(size_t) i1] - buf[(size_t) i0]);
    }
    float rnd() { rng = rng * 1664525u + 1013904223u;  return (float) (rng >> 8) * (1.0f / 16777216.0f); }

    std::vector<float> buf;
    float sr = 48000.0f;
    int   wp = 0;
    float phase = 0.0f, holdPhase = 0.0f, held = 0.0f, lp1 = 0.0f, lp2 = 0.0f, dcX = 0.0f, dcY = 0.0f, env = 0.0f;
    uint32_t rng = 0x9E3779B9u;
    int   evTimer = 0, evKind = 0, evLeft = 0;
    float phaseV = 0.0f;                 // Warble: the phase of the (virtual) long window that sets the swell rate
    float winEff = 0.0f;
    float offA = 0.0f, offB = 0.0f, prevPhaseB = 0.5f;
    int   refLen = 240;
    std::vector<float> ref;
    float evSemis = 0.0f, mute = 1.0f, drift = 0.0f, driftT = 0.0f, driftTarget = 0.0f, lagSemis = 0.0f, qSemis = 0.0f;
};
