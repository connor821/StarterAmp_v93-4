#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "NamPlayer.h"
#include "IrLibrary.h"
#include "NamSlot.h"
#include "BoostStage.h"
#include "ModFx.h"
#include "FxExtra.h"
#include "DiMatch.h"
#include "Looper.h"
#include "LoFi.h"
#include "CircuitModels.h"
#include "MetalPedals.h"
#include "FuzzVerb.h"
#include "StageShaper.h"
#include "PassiveStack.h"
#include "PowerSection.h"

//==============================================================================
// Post EQ band data (shared by the audio code and the EQ display).
// A 9-band graphic EQ, one band per octave from 63 Hz to 16 kHz. In "Graphic" mode
// every band has the same Q. In "Mod" mode each band has its own Q (narrower in the
// mids, wider at the ends), scaled by the Q knob.
//==============================================================================
inline constexpr int   kEqBands = 9;
inline constexpr float kEqBandHz[kEqBands] = { 63.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f };
inline constexpr float kEqModQ[kEqBands]   = { 0.8f, 1.0f, 1.6f, 2.2f, 1.4f, 2.4f, 1.8f, 1.2f, 0.8f };

// Parameter names used on the audio thread (looked up by name, so they are built once, here).
inline constexpr const char* kEqIds[kEqBands] = { "eqB1", "eqB2", "eqB3", "eqB4", "eqB5", "eqB6", "eqB7", "eqB8", "eqB9" };

// Cab blocks: up to four, A to D.
inline constexpr int kCabBlocks = 4;
inline juce::String cabLetter (int blk) { return juce::String::charToString ((juce::juce_wchar) ('A' + juce::jlimit (0, kCabBlocks - 1, blk))); }
// Hybrid amp mode: the share of the circuit-derived stage and power curves mixed with the classic ones (0 = classic, 1 = circuit)
constexpr float kHybridShare = 0.5f;

struct CabIds { const char *on, *level, *pan, *delay, *pol, *lowCut, *highCut, *amp; const char* eq[5]; const char *rso, *phs, *tilt; };
inline constexpr CabIds kCabIds[kCabBlocks] =
{
    { "cabOnA", "cabLevelA", "cabPanA", "cabDelayA", "cabPolA", "cabLowCutA", "cabHighCutA", "cabAmpA", { "cabEq1A", "cabEq2A", "cabEq3A", "cabEq4A", "cabEq5A" }, "cabRsoA", "cabPhsA", "cabTiltA" },
    { "cabOnB", "cabLevelB", "cabPanB", "cabDelayB", "cabPolB", "cabLowCutB", "cabHighCutB", "cabAmpB", { "cabEq1B", "cabEq2B", "cabEq3B", "cabEq4B", "cabEq5B" }, "cabRsoB", "cabPhsB", "cabTiltB" },
    { "cabOnC", "cabLevelC", "cabPanC", "cabDelayC", "cabPolC", "cabLowCutC", "cabHighCutC", "cabAmpC", { "cabEq1C", "cabEq2C", "cabEq3C", "cabEq4C", "cabEq5C" }, "cabRsoC", "cabPhsC", "cabTiltC" },
    { "cabOnD", "cabLevelD", "cabPanD", "cabDelayD", "cabPolD", "cabLowCutD", "cabHighCutD", "cabAmpD", { "cabEq1D", "cabEq2D", "cabEq3D", "cabEq4D", "cabEq5D" }, "cabRsoD", "cabPhsD", "cabTiltD" },
};

// Cheap replacements for std::tanh / log2 / exp2 in per-sample code. They are accurate to
// about 1e-4 (tanh) and 1e-4 (log2 / exp2), far below anything audible, and they compile to
// plain arithmetic that the compiler can vectorise.
// v0.2: hard symmetric clip (diodes, zeners, op-amp rails): linear up to about 0.7, then a sharp knee to 1. Same scale as fastTanh.
inline float hardClip (float u)
{
    const float u2 = u * u;
    return u / std::sqrt (std::sqrt (1.0f + u2 * u2));
}
inline float fastTanh (float x)
{
    x = std::max (-4.97f, std::min (4.97f, x));
    const float x2 = x * x;
    return x * (135135.0f + x2 * (17325.0f + x2 * (378.0f + x2)))
             / (135135.0f + x2 * (62370.0f + x2 * (3150.0f + x2 * 28.0f)));
}
inline float fastLog2 (float x)
{
    std::uint32_t i;  std::memcpy (&i, &x, 4);
    const float e = (float) ((int) (i >> 23) - 127);
    i = (i & 0x007fffffu) | 0x3f800000u;
    float m;  std::memcpy (&m, &i, 4);
    const float t = m - 1.0f;
    return e + t * (1.4412674f + t * (-0.70570416f + t * (0.40872174f + t * (-0.18772264f + t * 0.043428908f))));
}
inline float fastExp2 (float x)
{
    x = std::max (-126.0f, std::min (126.0f, x));
    const float fl = std::floor (x), f = x - fl;
    const float p = 1.0f + f * (0.6931472f + f * (0.2402265f + f * (0.0555041f + f * (0.0096181f + f * 0.0013333f))));
    std::uint32_t i = (std::uint32_t) ((int) fl + 127) << 23;
    float s;  std::memcpy (&s, &i, 4);
    return p * s;
}

// Delay note values: length in quarter-note beats (index 0 = free time in ms).
inline constexpr float kNoteBeats[11] = { 0.0f, 2.0f, 1.5f, 1.0f, 2.0f / 3.0f, 0.75f, 0.5f, 1.0f / 3.0f, 0.375f, 0.25f, 1.0f / 6.0f };

// Butterworth Qs for the Post EQ's high-pass / low-pass: one stage = 12 dB/oct, two stages = 24 dB/oct.
inline float eqPassQ (bool steep, int stage) { return steep ? (stage == 0 ? 0.5412f : 1.3066f) : 0.7071f; }

inline float eqBandQ (int band, bool modMode, float qKnob)
{
    return modMode ? kEqModQ[band] * (qKnob / 1.4f) : 1.4f;
}

//==============================================================================
// A simple noise gate. It watches the loudest channel, and fades the signal
// out when it stays below the threshold. Hysteresis + hold stop it chattering.
//==============================================================================
class NoiseGate
{
public:
    void prepare (double sampleRate)
    {
        envRelease  = (float) std::exp (-1.0 / (0.050 * sampleRate));
        gainAttack  = (float) std::exp (-1.0 / (0.002 * sampleRate));
        gainRelease = (float) std::exp (-1.0 / (0.080 * sampleRate));
        holdSamples = (int) (0.030 * sampleRate);
        reset();
    }

    void reset() { env = 0.0f; gain = 0.0f; holdCounter = 0; isOpen = false; }

    void process (juce::AudioBuffer<float>& buffer, float thresholdDb)
    {
        const float openThreshold  = juce::Decibels::decibelsToGain (thresholdDb);
        const float closeThreshold = openThreshold * 0.7f;   // hysteresis
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int i = 0; i < numSamples; ++i)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < numChannels; ++ch)
                peak = std::max (peak, std::abs (buffer.getSample (ch, i)));

            // Envelope follower: rises instantly, falls slowly.
            env = (peak > env) ? peak : peak + envRelease * (env - peak);

            if (env > openThreshold)
            {
                isOpen = true;
                holdCounter = holdSamples;
            }
            else if (isOpen && env < closeThreshold)
            {
                if (holdCounter > 0) --holdCounter;
                else                 isOpen = false;
            }

            const float target = isOpen ? 1.0f : 0.0f;
            const float coef   = (target > gain) ? gainAttack : gainRelease;
            gain = target + coef * (gain - target);

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.getWritePointer (ch)[i] *= gain;
        }
    }

private:
    float env = 0.0f, gain = 0.0f;
    float envRelease = 0.0f, gainAttack = 0.0f, gainRelease = 0.0f;
    int holdSamples = 0, holdCounter = 0;
    bool isOpen = false;
};

//==============================================================================
// A simple pitch shifter for the shimmer reverb. Two read taps slide through a
// delay line, crossfaded so the jump back to the start is hidden. It's a basic
// technique: slightly grainy on its own, but the reverb blurs that into the
// shimmer sound.
//==============================================================================
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        windowSamples = (float) (0.060 * sampleRate);                 // 60 ms
        buffer.assign ((size_t) (0.100 * sampleRate) + 8, 0.0f);
        writePos = 0;
        phase = 0.0f;
    }

    // The crossfade window (at most 80 ms). A shorter window means less delay but a rougher sound.
    void setWindowMs (float ms) { windowSamples = (float) (std::min (80.0f, std::max (5.0f, ms)) * 0.001 * sr); }
    float getWindowSamples() const { return windowSamples; }
    void reset() { std::fill (buffer.begin(), buffer.end(), 0.0f); writePos = 0; phase = 0.0f; }

    void setSemitones (float semitones) { ratio = std::pow (2.0f, semitones / 12.0f); }

    // Records a sample without computing any output. Used while an effect is idle, so the memory
    // is still full of your playing when the effect is switched on.
    void feed (float x)
    {
        buffer[(size_t) writePos] = x;
        if (++writePos >= (int) buffer.size()) writePos = 0;
    }

    float process (float x)
    {
        buffer[(size_t) writePos] = x;

        float out = 0.0f;
        for (int tap = 0; tap < 2; ++tap)
        {
            float p = phase + 0.5f * (float) tap;
            if (p >= 1.0f) p -= 1.0f;

            const float delay  = windowSamples * (1.0f - p);          // shrinks as p grows
            const float weight = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p);
            out += weight * readDelayed (delay);
        }

        // ratio > 1 (pitch up): the delay must shrink by (ratio - 1) samples per sample.
        phase += (ratio - 1.0f) / windowSamples;
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f)  phase += 1.0f;      // pitching down: the delay grows, so the phase runs backwards

        if (++writePos >= (int) buffer.size()) writePos = 0;
        return out;
    }

private:
    float readDelayed (float delay) const
    {
        const int size = (int) buffer.size();
        float readPos = (float) writePos - delay;
        if (readPos < 0.0f) readPos += (float) size;

        const int   i0   = (int) readPos;
        const float frac = readPos - (float) i0;
        const int   i1   = (i0 + 1) % size;
        return buffer[(size_t) i0] * (1.0f - frac) + buffer[(size_t) i1] * frac;
    }

    std::vector<float> buffer;
    int   writePos = 0;
    float phase = 0.0f, ratio = 2.0f, windowSamples = 1.0f;
    double sr = 44100.0;
};

//==============================================================================
// Final output limiter (optional). A look-ahead brickwall limiter on the sample peaks:
//   - For each sample the gain needed to keep both channels under the ceiling is found.
//   - The smallest needed gain over the last (lookahead + 1) samples is held, so the gain is
//     already down when the loud sample arrives.
//   - That gain is slowed on the way back up (Release), then averaged over the look-ahead window,
//     which turns every gain change into a smooth ramp.
//   - The audio is delayed by the look-ahead so it meets the ramp. With a look-ahead of 0 samples
//     (Low Latency mode) there is no delay and no ramp: the gain drops at once, which is
//     safe but can sound harsher on big overshoots.
// It limits sample peaks, not true (inter-sample) peaks, so leave a little headroom (the default
// ceiling is -1 dB).
//==============================================================================
class OutputLimiter
{
public:
    void prepare (double sampleRate)
    {
        maxLook = (int) (0.005 * sampleRate) + 2;
        ringSize = maxLook + 2;
        audio[0].assign ((size_t) ringSize, 0.0f);
        audio[1].assign ((size_t) ringSize, 0.0f);
        avg.assign ((size_t) ringSize, 1.0f);
        dqVal.assign ((size_t) ringSize, 1.0f);
        dqIdx.assign ((size_t) ringSize, 0);
        setLookahead (look);
    }

    int lookahead() const { return look; }

    void setLookahead (int n)
    {
        look = std::max (0, std::min (n, maxLook));
        std::fill (audio[0].begin(), audio[0].end(), 0.0f);
        std::fill (audio[1].begin(), audio[1].end(), 0.0f);
        std::fill (avg.begin(), avg.end(), 1.0f);
        std::fill (dqVal.begin(), dqVal.end(), 1.0f);
        std::fill (dqIdx.begin(), dqIdx.end(), 0);
        head = tail = 0;  n = 0;  sum = (double) (look + 1);  s = 1.0f;
    }

    // 'right' may be null (mono). ceiling is linear; release in ms.
    void process (float* left, float* right, int numSamples, float ceiling, float releaseMs, double sampleRate)
    {
        const float rel = 1.0f - std::exp (-1.0f / (std::max (1.0f, releaseMs) * 0.001f * (float) sampleRate));
        const int N = look, R = ringSize;

        for (int i = 0; i < numSamples; ++i, ++n)
        {
            const float l = left[i], r = right != nullptr ? right[i] : l;
            const float peak = std::max (std::abs (l), std::abs (r));
            const float need = peak > ceiling ? ceiling / peak : 1.0f;

            // sliding-window minimum over the last N + 1 samples (monotonic queue)
            while (tail > head && dqVal[(size_t) ((tail - 1) % R)] >= need) --tail;
            dqVal[(size_t) (tail % R)] = need;  dqIdx[(size_t) (tail % R)] = n;  ++tail;
            while (dqIdx[(size_t) (head % R)] < n - N) ++head;
            const float held = dqVal[(size_t) (head % R)];

            s = held < s ? held : s + rel * (held - s);          // instant down, slow up

            const long long gone = n - (N + 1);                  // the value leaving the window
            const double leaving = (double) avg[(size_t) (((gone % R) + R) % R)];
            avg[(size_t) (n % R)] = s;
            sum += (double) s - leaving;
            const float g = (float) (sum / (double) (N + 1));

            audio[0][(size_t) (n % R)] = l;
            audio[1][(size_t) (n % R)] = r;
            const size_t d = (size_t) ((n - N) % R + (n - N < 0 ? R : 0)) % (size_t) R;
            const float ol = audio[0][d] * g, orr = audio[1][d] * g;

            left[i] = std::max (-ceiling, std::min (ceiling, ol));              // safety clamp
            if (right != nullptr) right[i] = std::max (-ceiling, std::min (ceiling, orr));
        }
    }

private:
    std::vector<float> audio[2], avg, dqVal;
    std::vector<long long> dqIdx;
    long long head = 0, tail = 0, n = 0;
    double sum = 1.0;
    float s = 1.0f;
    int look = 0, maxLook = 0, ringSize = 8;
};

//==============================================================================
//==============================================================================
// GuitarShifter: the pitch shifter used by the Transposer. Like the simple shifter above it slides
// two read taps through a delay line and crossfades them, but it is tuned for guitar:
//   - Waveform-aligned splices. Every time a tap jumps back to the start of the window, the plugin
//     searches a few milliseconds either side for the position whose recent waveform best matches
//     the other tap (a normalised cross-correlation). The two taps are then in step, so the
//     crossfade does not cancel or beat, which is what makes a plain delay-line shifter warble on
//     sustained notes. The search covers 7 ms, enough for notes down to about 70 Hz.
//   - A small penalty for big jumps, so the delay stays near its nominal value.
//   - Cubic (Hermite) interpolation instead of linear, so the highs are not dulled.
//   - A smoothed shift amount, so turning the knob glides instead of clicking.
// It is still a time-domain shifter, so it works best on single notes and dyads; big chords
// and extreme shifts will sound rougher.
//==============================================================================
class GuitarShifter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        windowSamples = (float) (0.028 * sampleRate);
        size = (int) (0.200 * sampleRate) + 16;
        buffer.assign ((size_t) size, 0.0f);
        searchRange = searchDefault = (int) (0.007 * sampleRate);
        compareHalf = compareDefault = std::max (32, (int) (0.004 * sampleRate) / 2);       // samples compared, at stride 2
        refCapacity = std::max (compareHalf, (int) (0.014 * sampleRate) + 8);
        ref.assign ((size_t) refCapacity, 0.0f);
        smoothCoef = 1.0f - std::exp (-1.0f / (0.015f * (float) sampleRate));
        reset();
    }

    void setWindowMs (float ms) { windowSamples = (float) (std::min (80.0f, std::max (8.0f, ms)) * 0.001 * sr); }
    float getWindowSamples() const { return windowSamples; }

    // The lowest note you play, in Hz (0 = the normal setting, tuned for a six-string guitar). For 7- and 8-string
    // guitars the search for the best splice point has to look back a whole period of the lowest note, and the
    // comparison has to be that long too, otherwise the splices do not line up and the sound turns rough.
    void setLowHz (float hz)
    {
        lowHz = hz;
        if (hz <= 0.0f)
        {
            searchRange = searchDefault;
            compareHalf = compareDefault;
            return;
        }
        hz = std::max (35.0f, hz);
        searchRange = std::max (searchDefault, (int) (0.6f / hz * (float) sr));
        compareHalf = std::min (refCapacity, std::max (compareDefault, (int) (0.45f / hz * (float) sr)));
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        writePos = 0;  phase = 0.0f;
        off[0] = off[1] = 0.0f;
        lastP[0] = 0.0f;  lastP[1] = 0.5f;
        ratio = ratioTarget;
    }

    void setSemitones (float semitones) { ratioTarget = std::pow (2.0f, semitones / 12.0f); }

    float process (float x)
    {
        ratio += smoothCoef * (ratioTarget - ratio);
        buffer[(size_t) writePos] = x;

        float out = 0.0f;
        for (int tap = 0; tap < 2; ++tap)
        {
            float p = phase + 0.5f * (float) tap;
            if (p >= 1.0f) p -= 1.0f;

            if (std::abs (p - lastP[tap]) > 0.5f)       // this tap just jumped back: it is silent now, so re-aim it
                realign (tap, p);
            lastP[tap] = p;

            const float delay  = windowSamples * (1.0f - p) + off[tap];
            const float weight = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p);
            out += weight * readDelayed (delay);
        }

        phase += (ratio - 1.0f) / windowSamples;
        if (phase >= 1.0f) phase -= 1.0f;
        if (phase < 0.0f)  phase += 1.0f;

        if (++writePos >= size) writePos = 0;
        return out;
    }

private:
    float at (int delay) const        // the sample 'delay' samples ago (integer)
    {
        int i = writePos - delay;
        if (i < 0) i += size;
        return buffer[(size_t) i];
    }

    // Chooses the new offset of tap 'tap', whose phase has just wrapped to 'p'.
    void realign (int tap, float p)
    {
        const int other = 1 - tap;
        float pOther = phase + 0.5f * (float) other;
        if (pOther >= 1.0f) pOther -= 1.0f;
        const float refDelay = windowSamples * (1.0f - pOther) + off[other];
        const float nominal  = windowSamples * (1.0f - p);

        const int refD = (int) std::lround (refDelay);
        for (int k = 0; k < compareHalf; ++k)
            ref[(size_t) k] = at (std::max (2, refD) + 2 * k);

        const int lo = std::max (4, (int) std::lround (nominal) - searchRange);
        const int hi = std::min (size - 2 * compareHalf - 8, (int) std::lround (nominal) + searchRange);

        float bestScore = -1.0e30f;
        int   best = (int) std::lround (nominal);
        for (int c = lo; c <= hi; ++c)
        {
            float dot = 0.0f, energy = 1.0e-6f;
            for (int k = 0; k < compareHalf; ++k)
            {
                const float v = at (c + 2 * k);
                dot += ref[(size_t) k] * v;
                energy += v * v;
            }
            const float distance = std::abs ((float) c - nominal) / (float) searchRange;
            const float score = dot / std::sqrt (energy) * (1.0f - 0.1f * distance);
            if (score > bestScore) { bestScore = score; best = c; }
        }
        off[tap] = (float) best - nominal;
    }

    // Cubic (Hermite) interpolation of the delay line
    float readDelayed (float delay) const
    {
        delay = std::max (2.0f, delay);
        float readPos = (float) writePos - delay;
        if (readPos < 0.0f) readPos += (float) size;
        const int   i1 = (int) readPos;
        const float t  = readPos - (float) i1;
        const int   i0 = i1 > 0 ? i1 - 1 : size - 1;
        const int   i2 = i1 + 1 < size ? i1 + 1 : i1 + 1 - size;
        const int   i3 = i2 + 1 < size ? i2 + 1 : i2 + 1 - size;
        const float y0 = buffer[(size_t) i0], y1 = buffer[(size_t) i1], y2 = buffer[(size_t) i2], y3 = buffer[(size_t) i3];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * t + c2) * t + c1) * t + y1;
    }

    std::vector<float> buffer, ref;
    int   size = 8, writePos = 0, searchRange = 100, compareHalf = 64, searchDefault = 100, compareDefault = 64, refCapacity = 64;
    float lowHz = 0.0f;
    float phase = 0.0f, ratio = 1.0f, ratioTarget = 1.0f, windowSamples = 1.0f, smoothCoef = 0.01f;
    float off[2] { 0.0f, 0.0f }, lastP[2] { 0.0f, 0.5f };
    double sr = 44100.0;
};

// Transposer: shifts the pitch of what you play up or down by up to two octaves, at the very start
// of the chain. It uses the delay-line pitch shifter above, with a shorter window, and delays the
// dry signal by the same amount so the Mix control blends them in time. The delay it adds is half
// the window (reported to the host while the effect is on). Works best on single notes.
//==============================================================================
class Transposer
{
public:
    void prepare (double sampleRate)
    {
        shifter.prepare (sampleRate);
        dry.assign ((size_t) (0.09 * sampleRate) + 8, 0.0f);
        reset();
    }
    void setLowHz (float hz) { shifter.setLowHz (hz); }
    void reset() { shifter.reset(); std::fill (dry.begin(), dry.end(), 0.0f); pos = 0; }
    void setWindowMs (float ms) { shifter.setWindowMs (ms); }
    int  latencySamples() const { return (int) (shifter.getWindowSamples() * 0.5f); }

    // wetInOut: the input on the way in, the shifted signal on the way out.
    // dryOut: filled with the input delayed by latencySamples().
    void process (float* wetInOut, float* dryOut, int n, float semitones)
    {
        shifter.setSemitones (semitones);
        const int size = (int) dry.size();
        const int d = std::min (latencySamples(), size - 2);
        for (int i = 0; i < n; ++i)
        {
            const float in = wetInOut[i];
            dry[(size_t) pos] = in;
            dryOut[i] = dry[(size_t) ((pos - d + size) % size)];
            wetInOut[i] = shifter.process (in);
            if (++pos >= size) pos = 0;
        }
    }

private:
    GuitarShifter shifter;
    std::vector<float> dry;
    int pos = 0;
};

//==============================================================================
// Chug gate: a fast noise gate that sits after the preamp, so the gain's hiss is cut between
// palm-muted notes. 'amount' 0 = off, 1 = a high threshold (-30 dBFS). Attack is almost instant,
// release is short, so staccato playing stays tight.
//==============================================================================
class ChugGate
{
public:
    void prepare (double sr)
    {
        envRelease = (float) std::exp (-1.0 / (0.030 * sr));
        gainAttack = (float) std::exp (-1.0 / (0.0007 * sr));
        gainRelease = (float) std::exp (-1.0 / (0.012 * sr));
        reset();
    }
    void reset() { env = 0.0f; gain = 1.0f; open = true; }

    void process (float* x, int n, float amount)
    {
        const float openAt  = std::pow (10.0f, (-68.0f + 38.0f * amount) / 20.0f);
        const float closeAt = openAt * 0.5f;                       // 6 dB lower: no chattering
        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (x[i]);
            env = (a > env) ? a : a + envRelease * (env - a);
            if (open && env < closeAt) open = false;
            else if (! open && env > openAt) open = true;
            const float target = open ? 1.0f : 0.0f;
            const float coef = (target > gain) ? gainAttack : gainRelease;
            gain = target + coef * (gain - target);
            x[i] *= gain;
        }
    }

private:
    float env = 0.0f, gain = 1.0f, envRelease = 0.0f, gainAttack = 0.0f, gainRelease = 0.0f;
    bool open = true;
};

//==============================================================================
// Speaker dynamics (v41): a rough model of how a real speaker changes with level, applied to the signal
// going into a cab block (before the impulse response, which stays a fixed snapshot). Four things happen
// as 'amount' goes up:
//   - the lows (below about 200 Hz) are soft-limited, like a cone running out of travel
//   - the mids get a little cone breakup
//   - a slow "voice-coil heating" compression pulls the level down a little on sustained loud playing
//   - loud passages get darker: more of the top is rolled off while the level is high
// mode 0 = Tight (modern), 1 = Vintage (spongier: lower limit, more heating, more darkening).
// v65: modes 2 and 3 (Real: tight, Real: vintage) do the same and add three level-dependent cone behaviours:
//   - the cone's bass resonance gets higher and less pronounced as the cone moves further (a real cone stiffens)
//   - the top is lost while the cone is working hard in the lows (the voice coil's inductance changes with its position)
//   - a band-limited, one-sided breakup between 1.5 and 4 kHz (the cone's break-up modes), which leaves the lows alone
// This is a simple approximation I made up, not a measurement of any speaker.
//==============================================================================
class SpeakerDynamics
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        lowK  = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 200.0f / sampleRate);
        fastA = std::exp (-1.0f / (0.003f * sampleRate));  fastR = std::exp (-1.0f / (0.06f * sampleRate));
        slowA = std::exp (-1.0f / (0.15f * sampleRate));   slowR = std::exp (-1.0f / (0.6f * sampleRate));
        reset();
    }
    void reset() { low = 0.0f; envF = envS = 0.0f; hf = 0.0f; ic1 = ic2 = 0.0f; envLow = 0.0f; lp4 = lpHi = lpLo = 0.0f; }

    void process (float* x, int n, float amt, int mode)
    {
        const bool vintage = (mode & 1) != 0;
        const bool real = mode >= 2;
        const float fs0 = vintage ? 85.0f : 100.0f;
        const float th   = (vintage ? 0.30f : 0.50f) * (1.0f - 0.6f * amt);
        const float heat = (vintage ? 1.8f : 0.9f) * amt;
        const float darkW = (vintage ? 0.7f : 0.5f) * amt;
        const float fc = vintage ? 3500.0f : 5500.0f;
        const float hfK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / sampleRate);
        for (int i = 0; i < n; ++i)
        {
            const float in = x[i];
            low += lowK * (in - low);
            float high = in - low;
            const float lowS = th * fastTanh (low / th);
            const float lowOut = low + amt * (lowS - low);
            high += 0.5f * amt * (fastTanh (1.8f * high) / 1.8f - high);
            float y = lowOut + high;

            if (real)
            {
                // Excursion: how hard the low band is working (0 to 1)
                const float al = std::abs (low);
                envLow += (al > envLow ? 0.02f : 0.002f) * (al - envLow);
                const float exc = std::min (1.0f, envLow / 0.3f);
                // 1. Bass resonance that rises and fades as the cone moves (a state-variable band-pass)
                const float fc = fs0 * (1.0f + 0.4f * amt * exc);
                const float g = juce::MathConstants<float>::pi * fc / sampleRate, k = 1.0f / 1.6f;
                const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
                const float v3 = y - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
                ic1 = 2.0f * v1 - ic1;  ic2 = 2.0f * v2 - ic2;
                y += amt * (vintage ? 0.8f : 0.6f) * (1.0f - 0.8f * exc) * k * v1;
                // 2. Top lost while the lows work hard
                lp4 += 0.45f * (y - lp4);                                   // one pole near 4.5 kHz at 48 kHz
                y += amt * std::min (1.0f, 1.2f * exc) * (lp4 - y);
                // 3. One-sided breakup of the 1.5 to 4 kHz band only
                lpLo += 0.18f * (y - lpLo);                                 // below about 1.5 kHz
                lpHi += 0.45f * (y - lpHi);                                 // below about 4.5 kHz
                const float bnd = lpHi - lpLo;
                y += amt * (0.5f * (fastTanh (2.4f * bnd + 0.25f) - fastTanh (0.25f)) / 1.2f - bnd * 1.0f) * 0.6f;
            }

            const float a = std::abs (y);
            envF = (a > envF) ? a + fastA * (envF - a) : a + fastR * (envF - a);
            envS = (a > envS) ? a + slowA * (envS - a) : a + slowR * (envS - a);
            y /= (1.0f + heat * envS);

            hf += hfK * (y - hf);
            const float w = darkW * std::min (1.0f, envF * 3.0f);
            x[i] = y + w * (hf - y);
        }
    }

private:
    float sampleRate = 48000.0f, lowK = 0.02f, fastA = 0.9f, fastR = 0.99f, slowA = 0.99f, slowR = 0.999f;
    float low = 0.0f, envF = 0.0f, envS = 0.0f, hf = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f, envLow = 0.0f, lp4 = 0.0f, lpHi = 0.0f, lpLo = 0.0f;   // v65 (real modes)
};

//==============================================================================
// Delay with two optional "character" controls:
//   Lo-Fi  : each repeat gets darker, crunchier (soft saturation), and
//            degraded (lower sample rate and bit depth).
//   Warble : tape-style pitch wobble. The delay time is nudged by a slow
//            "wow", a faster "flutter", and a little random drift.
// The amp uses two of these (one before the amp, one after).
//==============================================================================
#include "Harmonizer.h"
#include "GuitarSynth.h"
#include "CabRealism.h"
#include "RiffRecorder.h"
#include "IrReverb.h"

struct DelayParams
{
    float timeMs, feedback, mix, lofi, warble;
    int   mode;      // 0 Digital, 1 Analog, 2 Tape, 3 Lo-Fi, 4 Oil-can, 5 Organ
    int   stereo = 0;        // 0 Stereo (each side on its own), 1 Ping-Pong (left first), 2 Ping-Pong (right first), 3 Dual (right side offset)
    float offset = 0.5f;     // Dual: the right delay is (1 + offset) times the left
    float spread = 1.0f;     // narrows the echoes toward the centre (1 = as is, 0 = mono)
    int   style = 0;         // 0 Forward, 1 Reverse (each echo plays backwards)
    // v52 additions (all neutral at their defaults)
    float duck = 0.0f;       // how far the echoes dip while you play (0 = off)
    float duckRelMs = 250.0f;// how quickly they come back after you stop
    float modDepth = 0.0f;   // slow chorus-like wobble of the delay time
    float modRate = 1.0f;    // Hz
    float diffuse = 0.0f;    // smears each echo (allpass chain in the loop)
    bool  hold = false;
    int   holdMode = 0;      // 0 Freeze (stops taking input, loops what it has), 1 Infinite (keeps taking input, repeats never die)
    int   taps = 0;          // multi-tap pattern, 0 = off
    float tapLevel = 0.6f;
};

class DelayEffect
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec, float initialTimeMs,
                  float initialMix, int randomSeed)
    {
        sampleRate  = (float) spec.sampleRate;
        numChannels = (int) spec.numChannels;

        line.setMaximumDelayInSamples ((int) (2.0f * sampleRate));
        line.prepare (spec);
        line.reset();

        timeSmoothed.reset (spec.sampleRate, 0.05);
        timeSmoothed.setCurrentAndTargetValue (initialTimeMs);
        lastMix = initialMix;

        for (int ch = 0; ch < 2; ++ch)
        {
            lowpass[ch] = 0.0f;
            lowpass2[ch] = 0.0f;
            held[ch] = 0.0f;
            holdCounter[ch] = 0;
        }

        random.setSeed (randomSeed);
        wowPhase = random.nextFloat();       // different starting points for
        flutterPhase = random.nextFloat();   // the two delay instances
        drift = driftTarget = 0.0f;
        driftCountdown = 0;
        revPhase[0] = revPhase[1] = 0.0f;
        duckEnv = 0.0f;  duckGain = 1.0f;  holdAmt = 0.0f;  modPhase = 0.0f;
        static const float apMs[4] = { 1.7f, 2.9f, 4.3f, 6.1f };
        for (int ch = 0; ch < 2; ++ch)
            for (int k = 0; k < 4; ++k)
                ap[ch][k].init ((int) (apMs[k] * 0.001f * sampleRate) + 1);
    }

    void process (juce::AudioBuffer<float>& buffer, const DelayParams& p)
    {
        const int   numSamples = buffer.getNumSamples();
        const int   channels   = std::min ({ buffer.getNumChannels(), numChannels, 2 });
        const float twoPi      = juce::MathConstants<float>::twoPi;

        // The delay mode sets the basic character; the Lo-Fi and Warble knobs add to it.
        //   Digital : clean, bright (16 kHz), linear feedback
        //   Analog  : bucket-brigade style: darker (4.5 kHz, two poles), soft saturation, a little clock wobble
        //   Tape    : darker still (3.2 kHz), tape-style saturation, a lot of wow, flutter and drift
        //   Lo-Fi   : dark (2.6 kHz) plus reduced sample rate and bit depth, a little wobble
        struct ModeSet { float cutoffHz, drive, lofiBase, warbleBase; bool linear; };
        static const ModeSet modes[6] = { { 16000.0f, 1.0f, 0.0f, 0.00f, true  },
                                          {  4500.0f, 1.8f, 0.0f, 0.12f, false },
                                          {  3200.0f, 2.6f, 0.0f, 0.50f, false },
                                          {  2600.0f, 2.0f, 0.55f, 0.15f, false },
                                          {  2300.0f, 2.4f, 0.0f, 0.10f, false },
                                          {  2600.0f, 2.0f, 0.0f, 0.05f, false } };   // Organ (short oil-can) + Oil-can: dark, warm, with a built-in heavy smooth wobble (below)
        const auto& mode   = modes[juce::jlimit (0, 5, p.mode)];
        const float lofi   = mode.lofiBase + p.lofi   * (1.0f - mode.lofiBase);
        const float warble = mode.warbleBase + p.warble * (1.0f - mode.warbleBase);
        const bool  twoPole = p.mode != 0;

        // Lo-fi settings
        const float cutoff  = std::min (0.45f * sampleRate, mode.cutoffHz * std::pow (1200.0f / 3500.0f, p.lofi));
        const float lpCoef  = 1.0f - std::exp (-twoPi * cutoff / sampleRate);
        const float drive   = mode.drive + 3.0f * p.lofi;
        const int   holdLen = 1 + (int) (lofi * 6.0f);                          // 1 to 7 samples
        const float levels  = std::pow (2.0f, 15.0f - 6.0f * lofi);             // 16-bit down to about 9-bit

        // Warble settings (depths are in samples)
        const float wowDepth     = warble * 0.0040f  * sampleRate;
        const float flutterDepth = warble * 0.00025f * sampleRate;
        const float driftDepth   = warble * 0.0015f  * sampleRate;
        const float wowInc       = 0.6f / sampleRate;
        const float flutterInc   = 6.3f / sampleRate;
        const float driftCoef    = 1.0f - std::exp (-twoPi * 0.7f / sampleRate);

        timeSmoothed.setTargetValue (p.mode == 5 ? juce::jlimit (6.0f, 70.0f, p.timeMs * 0.05f) : p.timeMs);

        // v52 settings
        const bool  duckOn   = p.duck > 0.001f;
        const float duckAtk  = 1.0f - std::exp (-1.0f / (0.004f * sampleRate));
        const float duckRel  = 1.0f - std::exp (-1.0f / (0.001f * juce::jlimit (20.0f, 2000.0f, p.duckRelMs) * sampleRate));
        const bool  oilCan   = p.mode >= 4, organ = p.mode == 5;
        // Organ: the delay time knob is squeezed into a short 6 to 70 ms range, with a deeper, faster wobble: a lush vibrato at 100% mix
        const float modInc   = juce::jlimit (0.05f, 10.0f, organ ? std::max (1.5f, std::min (p.modRate, 6.0f)) : oilCan ? std::min (p.modRate, 2.5f) : p.modRate) / sampleRate;
        const float modDepthS = (organ ? 0.6f + 0.4f * p.modDepth : oilCan ? 0.35f + 0.65f * p.modDepth : p.modDepth) * (organ ? 0.0018f : 0.006f) * sampleRate;                 // up to +-6 ms
        const bool  diffOn   = p.diffuse > 0.001f;
        const float apG      = 0.35f + 0.3f * p.diffuse;
        const float holdTarget = p.hold ? 1.0f : 0.0f;
        const float holdCoef = 1.0f - std::exp (-1.0f / (0.03f * sampleRate));
        // Multi-tap patterns: extra reads at fractions of the delay time (the main echo stays at the full time)
        struct TapSet { int n; float frac[4]; float gain[4]; };
        static const TapSet tapSets[8] = {
            { 0, { 0, 0, 0, 0 },               { 0, 0, 0, 0 } },
            { 1, { 0.5f, 0, 0, 0 },            { 1.0f, 0, 0, 0 } },                     // 1/2 + 1
            { 2, { 1.0f/3, 2.0f/3, 0, 0 },     { 1.0f, 0.85f, 0, 0 } },                 // thirds
            { 3, { 0.25f, 0.5f, 0.75f, 0 },    { 0.9f, 0.8f, 0.7f, 0 } },               // quarters (four evenly spaced)
            { 2, { 0.25f, 0.75f, 0, 0 },       { 0.9f, 0.8f, 0, 0 } },                  // off-beat
            { 3, { 0.375f, 0.5f, 0.875f, 0 },  { 0.9f, 0.7f, 0.8f, 0 } },              // dotted-style cluster
            { 1, { 1.0f/3, 0, 0, 0 },          { 1.0f, 0, 0, 0 } },                     // magneto: heads 1 + 3 (heads spaced 1:2:3)
            { 1, { 2.0f/3, 0, 0, 0 },          { 1.0f, 0, 0, 0 } } };                   // magneto: heads 2 + 3
        const TapSet& tapSet = tapSets[juce::jlimit (0, 7, p.taps)];
        const bool  tapsOn = tapSet.n > 0 && p.tapLevel > 0.001f;

        // Reverse style: the signal is read backwards. Two read points slide back through the last stretch of sound
        // (twice as fast as it is written), each faded in and out with a Hann window, half a cycle apart, so one
        // takes over as the other fades. A stretch is as long as the delay time.
        const bool reverse = p.style == 1;
        auto readWet = [&] (int ch, float d) -> float
        {
            if (! reverse) return line.popSample (ch, d);
            const float L = std::max (64.0f, std::min (d, sampleRate - 4.0f));
            float& ph = revPhase[ch];
            float pb = ph + 0.5f;
            if (pb >= 1.0f) pb -= 1.0f;
            const float wa = 0.5f - 0.5f * std::cos (twoPi * ph), wb = 0.5f - 0.5f * std::cos (twoPi * pb);
            const float va = line.popSample (ch, 2.0f * L * ph + 2.0f, false);               // (the read pointer moves once per sample, on the second read)
            const float v = wa * va + wb * line.popSample (ch, 2.0f * L * pb + 2.0f);
            ph += 1.0f / L;
            if (ph >= 1.0f) ph -= 1.0f;
            return v;
        };

        for (int i = 0; i < numSamples; ++i)
        {
            // Slowly wandering random value for the "drift" part of the warble.
            if (--driftCountdown <= 0)
            {
                driftTarget = random.nextFloat() * 2.0f - 1.0f;
                driftCountdown = (int) (1.4f * sampleRate);
            }
            drift += driftCoef * (driftTarget - drift);

            wowPhase     += wowInc;      if (wowPhase     >= 1.0f) wowPhase     -= 1.0f;
            flutterPhase += flutterInc;  if (flutterPhase >= 1.0f) flutterPhase -= 1.0f;

            const float modulation = wowDepth     * std::sin (twoPi * wowPhase)
                                   + flutterDepth * std::sin (twoPi * flutterPhase)
                                   + driftDepth   * drift;

            modPhase += modInc;  if (modPhase >= 1.0f) modPhase -= 1.0f;
            const float modL = modDepthS * std::sin (twoPi * modPhase);
            const float modRdiff = modDepthS * std::sin (twoPi * (modPhase + 0.25f)) - modL;   // right side a quarter turn later: wider

            // Ducking: follow the loudness of what you play; dip the echoes while it is loud
            float duckMul = 1.0f;
            if (duckOn)
            {
                float lvl = 0.0f;
                for (int ch = 0; ch < channels; ++ch)
                    lvl = std::max (lvl, std::abs (buffer.getReadPointer (ch)[i]));
                duckEnv += (lvl > duckEnv ? duckAtk : duckRel) * (lvl - duckEnv);
                duckMul = 1.0f - p.duck * std::min (1.0f, duckEnv * 10.0f);
            }

            holdAmt += holdCoef * (holdTarget - holdAmt);
            if (holdTarget == 0.0f && holdAmt < 1.0e-4f) holdAmt = 0.0f;
            const bool freezeMode = p.holdMode == 0;
            const float inMul = (holdAmt > 0.0f && freezeMode) ? 1.0f - holdAmt : 1.0f;

            float delaySamples = timeSmoothed.getNextValue() * 0.001f * sampleRate + modulation + modL;
            delaySamples = juce::jlimit (2.0f, 2.0f * sampleRate - 2.0f, delaySamples);

            const float mix = lastMix + (p.mix - lastMix) * (float) i / (float) numSamples;

            // Feedback for one channel: darken, then soft-saturate (small signals pass unchanged).
            auto feedbackOf = [&] (int ch, float wet)
            {
                lowpass[ch] += lpCoef * (wet - lowpass[ch]);
                float filtered = lowpass[ch];
                if (twoPole)
                {
                    lowpass2[ch] += lpCoef * (lowpass[ch] - lowpass2[ch]);
                    filtered = lowpass2[ch];
                }
                float r = mode.linear ? juce::jlimit (-1.5f, 1.5f, p.feedback * filtered)
                                      : fastTanh (drive * p.feedback * filtered) / drive;
                if (holdAmt > 0.0f)
                {
                    // Freeze: the loop is closed with no loss. Infinite: nearly closed, and the saturation keeps it from running away.
                    const float h = freezeMode ? juce::jlimit (-1.5f, 1.5f, wet)
                                               : (mode.linear ? juce::jlimit (-1.5f, 1.5f, 0.995f * filtered)
                                                              : fastTanh (drive * 0.995f * filtered) / drive);
                    r += holdAmt * (h - r);
                }
                return r;
            };
            // Write one value into a channel's delay line, through the lo-fi stages.
            auto writeTo = [&] (int ch, float w)
            {
                if (diffOn)
                {
                    float y = w;
                    for (int k = 0; k < 4; ++k) y = ap[ch][k].run (y, apG);
                    w += p.diffuse * (y - w);
                }
                if (holdAmt > 0.0f && ! freezeMode)
                    w = juce::jlimit (-1.2f, 1.2f, w);

                // Lo-fi: lower sample rate (hold each value a few samples)...
                if (holdCounter[ch] <= 0) { held[ch] = w; holdCounter[ch] = holdLen; }
                --holdCounter[ch];
                w = held[ch];

                // ...and lower bit depth.
                if (lofi > 0.001f)
                    w = std::round (w * levels) / levels;

                line.pushSample (ch, w);
            };

            // Extra taps at fractions of the delay time (read first, without moving the read pointer)
            auto tapsOf = [&] (int ch, float d) -> float
            {
                if (! tapsOn) return 0.0f;
                float a = 0.0f;
                for (int t = 0; t < tapSet.n; ++t)
                    a += tapSet.gain[t] * line.popSample (ch, std::max (2.0f, d * tapSet.frac[t]), false);
                return a * p.tapLevel;
            };

            if (p.stereo != 0 && channels == 2)
            {
                float* xl = buffer.getWritePointer (0);
                float* xr = buffer.getWritePointer (1);

                // Dual: the right side runs at a different time (1x to 2x the left).
                const float delayR = juce::jlimit (2.0f, 2.0f * sampleRate - 2.0f,
                                                   ((p.stereo == 3) ? delaySamples * (1.0f + juce::jlimit (0.0f, 1.0f, p.offset)) : delaySamples) + modRdiff);

                const float tapL = tapsOf (0, delaySamples), tapR = tapsOf (1, delayR);
                const float wetL = readWet (0, delaySamples);
                const float wetR = readWet (1, delayR);
                const float fbL  = feedbackOf (0, wetL);
                const float fbR  = feedbackOf (1, wetR);

                if (p.stereo == 3)                      // Dual: each side feeds itself
                {
                    writeTo (0, inMul * xl[i] + fbL);
                    writeTo (1, inMul * xr[i] + fbR);
                }
                else                                    // Ping-Pong: the input goes into one side and the echoes bounce across
                {
                    const float mono = 0.5f * (xl[i] + xr[i]);
                    if (p.stereo == 1) { writeTo (0, inMul * mono + fbR); writeTo (1, fbL); }
                    else               { writeTo (1, inMul * mono + fbL); writeTo (0, fbR); }
                }

                // Spread: narrow the echoes toward the centre
                const float outL = (wetL + tapL) * duckMul, outR = (wetR + tapR) * duckMul;
                const float mid  = 0.5f * (outL + outR);
                const float side = 0.5f * (outL - outR) * juce::jlimit (0.0f, 1.0f, p.spread);
                xl[i] += mix * (mid + side);
                xr[i] += mix * (mid - side);
            }
            else
            {
                for (int ch = 0; ch < channels; ++ch)
                {
                    float* x = buffer.getWritePointer (ch);
                    const float dly = ch == 0 ? delaySamples : juce::jlimit (2.0f, 2.0f * sampleRate - 2.0f, delaySamples + modRdiff);
                    const float tap = tapsOf (ch, dly);
                    const float wet = readWet (ch, dly);
                    writeTo (ch, inMul * x[i] + feedbackOf (ch, wet));
                    x[i] += mix * ((wet + tap) * duckMul);
                }
            }
        }
        lastMix = p.mix;
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> line;
    juce::SmoothedValue<float> timeSmoothed;
    juce::Random random;

    float sampleRate = 44100.0f;
    int   numChannels = 2;
    float lastMix = 0.0f;
    float lowpass[2] = { 0.0f, 0.0f }, lowpass2[2] = { 0.0f, 0.0f };
    float held[2]    = { 0.0f, 0.0f };
    int   holdCounter[2] = { 0, 0 };
    float revPhase[2] = { 0.0f, 0.0f };
    float wowPhase = 0.0f, flutterPhase = 0.0f, drift = 0.0f, driftTarget = 0.0f;
    int   driftCountdown = 0;

    // v52
    float duckEnv = 0.0f, duckGain = 1.0f, holdAmt = 0.0f, modPhase = 0.0f;
    struct Allpass
    {
        std::vector<float> buf; int pos = 0;
        void init (int n) { buf.assign ((size_t) std::max (2, n), 0.0f); pos = 0; }
        float run (float x, float g)
        {
            const float d = buf[(size_t) pos];
            const float v = x + g * d;
            buf[(size_t) pos] = v;
            if (++pos >= (int) buf.size()) pos = 0;
            return d - g * v;
        }
    };
    Allpass ap[2][4];
};

//==============================================================================
// "Noise" pedal: a pitch-shifting noise generator. Two momentary switches jump the pitch up
// one or two octaves; four knobs make it unstable:
//   Rise  : how long the pitch takes to glide up (and back down)
//   Speed : rate of the wobble and the random jumps
//   Panic : depth of a smooth, warbling pitch wobble
//   Chaos : depth of random stepped pitch jumps (glitchy, dissonant)
// The knob roles are my own interpretation, not the original's documented
// behaviour. With no switch held, the pedal passes your signal through.
//==============================================================================
class NoiseEffect
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        shifter.prepare (newSampleRate);
        semis = lfoPhase = randomValue = randomTarget = 0.0f;
        holdCountdown = 0;
        random.setSeed (4242);
    }

    // Processes one channel in place.
    void process (float* x, int numSamples, bool octUp, bool twoOctUp,
                  float rise, float speed, float panic, float chaos)
    {
        const float twoPi = juce::MathConstants<float>::twoPi;

        const float targetSemis = twoOctUp ? 24.0f : (octUp ? 12.0f : 0.0f);
        const float riseSeconds = 0.005f * std::pow (300.0f, rise);          // 5 ms up to 1.5 s
        const float glideCoef   = 1.0f - std::exp (-1.0f / (riseSeconds * sampleRate));
        const float rateHz      = 0.3f * std::pow (40.0f, speed);            // 0.3 Hz up to 12 Hz
        const float lfoInc      = rateHz / sampleRate;
        const int   holdLength  = std::max (1, (int) (sampleRate / (2.0f * rateHz)));
        const float jumpCoef    = 1.0f - std::exp (-1.0f / (0.012f * sampleRate));   // 12 ms steps

        // Idle: nothing is shifted and nothing is coming. Only keep the memory filled.
        if (targetSemis == 0.0f && semis < 1.0e-4f)
        {
            semis = 0.0f;
            for (int i = 0; i < numSamples; ++i)
                shifter.feed (x[i]);
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            semis += glideCoef * (targetSemis - semis);

            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

            if (--holdCountdown <= 0)
            {
                randomTarget  = random.nextFloat() * 2.0f - 1.0f;
                holdCountdown = holdLength;
            }
            randomValue += jumpCoef * (randomTarget - randomValue);

            // 0 = dry, 1 = fully shifted. The wobble only applies while shifted.
            const float wet = juce::jlimit (0.0f, 1.0f, semis * 0.5f);
            const float modulation = panic * 1.5f * std::sin (twoPi * lfoPhase)
                                   + chaos * 7.0f * randomValue;

            // Never below 0 semitones (the shifter only handles upward shifts).
            shifter.setSemitones (juce::jlimit (0.0f, 36.0f, semis + wet * modulation));

            // The shifter always runs, so its memory is full when you press.
            const float shifted = shifter.process (x[i]);
            x[i] = x[i] * (1.0f - wet) + shifted * wet;
        }
    }

private:
    PitchShifter shifter;
    juce::Random random;
    float sampleRate = 44100.0f;
    float semis = 0.0f, lfoPhase = 0.0f, randomValue = 0.0f, randomTarget = 0.0f;
    int   holdCountdown = 0;
};


//==============================================================================
// "Clash" layer for the Noise pedal: a pitch effect for the start of the chain with two
// knobs (DRIVE = input gain into the pitch engine, BLEND = wet/dry) and three scenes
// (Bright = tense, Dark = more sinister, Full = maximum dissonance, three-note chaos).
// Everything below is my own design:
//   - Up to three pitch-shifted voices at dissonant intervals above the note
//   - Slow random detune on each voice, so the dissonance is unstable
//   - Drive pushes the signal into soft clipping before the shifters
//   - A high-pass on the wet signal keeps the low end tight
// The shifters only go up, so every interval is above the played note.
//==============================================================================
class GrossEngine
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        for (auto& s : shifters)
            s.prepare (newSampleRate);

        for (int v = 0; v < 3; ++v)
        {
            semis[v] = 6.0f;
            weights[v] = 0.0f;
        }
        amount = lfoPhase1 = lfoPhase2 = 0.0f;
        lpState = hpState = 0.0f;
    }

    // Processes one channel in place.
    //   engaged : the Clash switch (latched or momentary)
    //   scene   : 0 Bright, 1 Dark, 2 Full
    //   push    : 0..1 drive into the pitch engine
    //   blend  : 0..1 wet/dry mix
    //   rise    : same Rise knob as the octave switches (how fast it fades in and out)
    //   speed   : same Speed knob (rate of the detune wobble)
    void process (float* x, int numSamples, bool engaged, int scene,
                  float push, float blend, float rise, float speed)
    {
        struct Scene { float semitones[3]; float weight[3]; float lowpassHz; };
        static const Scene scenes[3] =
        {
            { {  6.0f, 13.0f,  6.0f }, { 1.0f, 0.75f, 0.0f }, 9000.0f },   // Bright: tritone + flat ninth, bright
            { {  1.0f,  6.0f,  6.0f }, { 1.0f, 0.80f, 0.0f }, 2600.0f },   // Dark: minor second + tritone, dark
            { {  1.0f,  6.0f, 11.0f }, { 0.9f, 0.90f, 0.9f }, 5500.0f }    // Full: three clashing voices
        };
        const Scene& sc = scenes[juce::jlimit (0, 2, scene)];

        const float twoPi       = juce::MathConstants<float>::twoPi;
        const float riseSeconds = 0.005f * std::pow (300.0f, rise);
        const float fadeCoef    = 1.0f - std::exp (-1.0f / (riseSeconds * sampleRate));
        const float moveCoef    = 1.0f - std::exp (-1.0f / (0.02f * sampleRate));   // 20 ms: scene changes glide
        const float rateHz      = 0.15f * std::pow (40.0f, speed);                  // 0.15 Hz up to 6 Hz
        const float inc1        = rateHz / sampleRate;
        const float inc2        = 1.37f * rateHz / sampleRate;
        const float detuneDepth = 0.04f + 0.5f * push;                              // semitones
        const float drive       = 1.0f + 8.0f * push;
        const float driveTrim   = 1.0f / std::sqrt (drive);
        const float lpCoef      = 1.0f - std::exp (-twoPi * sc.lowpassHz / sampleRate);
        const float hpCoef      = 1.0f - std::exp (-twoPi * 150.0f / sampleRate);
        const float target      = engaged ? 1.0f : 0.0f;

        // Idle: switched off and faded out. The output would be the dry signal, so only keep the
        // three shifter memories filled and skip the rest.
        if (! engaged && amount < 1.0e-5f)
        {
            amount = 0.0f;
            for (int i = 0; i < numSamples; ++i)
            {
                const float shaped = fastTanh (drive * x[i]) * driveTrim;
                for (auto& sh : shifters)
                    sh.feed (shaped);
            }
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            amount += fadeCoef * (target - amount);

            lfoPhase1 += inc1;  if (lfoPhase1 >= 1.0f) lfoPhase1 -= 1.0f;
            lfoPhase2 += inc2;  if (lfoPhase2 >= 1.0f) lfoPhase2 -= 1.0f;

            const float dry    = x[i];
            const float shaped = fastTanh (drive * dry) * driveTrim;

            float wet = 0.0f;
            for (int v = 0; v < 3; ++v)
            {
                semis[v]   += moveCoef * (sc.semitones[v] - semis[v]);
                weights[v] += moveCoef * (sc.weight[v]    - weights[v]);

                const float detune = detuneDepth * (0.7f * std::sin (twoPi * (lfoPhase1 + 0.33f * (float) v))
                                                  + 0.3f * std::sin (twoPi * (lfoPhase2 + 0.21f * (float) v)));
                shifters[v].setSemitones (juce::jlimit (0.0f, 36.0f, semis[v] + detune));

                // The shifters always run, so their memory is full when you press.
                wet += weights[v] * shifters[v].process (shaped);
            }
            wet *= 0.7f;

            // Scene tone, then keep the lows out of the wet signal
            lpState += lpCoef * (wet - lpState);
            hpState += hpCoef * (lpState - hpState);
            const float wetFiltered = lpState - hpState;

            const float mix = blend * amount;
            x[i] = dry * (1.0f - mix) + wetFiltered * mix;
        }
    }

private:
    PitchShifter shifters[3];
    float sampleRate = 44100.0f;
    float semis[3] = { 6.0f, 6.0f, 6.0f };
    float weights[3] = { 0.0f, 0.0f, 0.0f };
    float amount = 0.0f, lfoPhase1 = 0.0f, lfoPhase2 = 0.0f, lpState = 0.0f, hpState = 0.0f;
};


//==============================================================================
// Ring modulator layer of the Noise pedal. A carrier oscillator is multiplied
// with your signal, which replaces each note's harmonics with sum and
// difference tones: bells, metallic clangs and robot voices.
//   Mode  : Ring = plain multiplication (the classic sound, the original notes
//           disappear); AM = amplitude modulation (the original notes stay, with
//           a shimmering tremolo-like layer on top).
//   Wave  : the carrier shape: Sine (cleanest), Square (harsher) or Triangle.
//   Freq  : the carrier frequency. Low values (1 to 20 Hz) are a tremolo-like
//           chop; 50 to 500 Hz gives growl and metal; higher gives bells and alien tones.
//   Mix   : wet/dry.
//   Wobble: moves the carrier up and down by up to an octave, at the Speed rate.
// The carrier is not band-limited, so Square and Triangle at high frequencies
// alias a bit. That is part of the sound here.
//==============================================================================
class RingModEffect
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        amount = carrierPhase = lfoPhase = 0.0f;
        freqSmoothed = 330.0f;
    }

    // Processes one channel in place.
    void process (float* x, int numSamples, bool engaged, int mode, int wave,
                  float freqHz, float mix, float wobble, float rise, float speed, float track = 0.0f)
    {
        const float twoPi       = juce::MathConstants<float>::twoPi;
        const float detA        = 1.0f - std::exp (-twoPi * 900.0f / sampleRate);
        const float riseSeconds = 0.005f * std::pow (300.0f, rise);
        const float fadeCoef    = 1.0f - std::exp (-1.0f / (riseSeconds * sampleRate));
        const float freqCoef    = 1.0f - std::exp (-1.0f / (0.03f * sampleRate));   // 30 ms glide
        const float lfoInc      = 0.15f * std::pow (40.0f, speed) / sampleRate;
        const float target      = engaged ? 1.0f : 0.0f;

        if (! engaged && amount < 1.0e-5f)      // idle: the output is the dry signal
        {
            amount = 0.0f;
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            amount += fadeCoef * (target - amount);

            lfoPhase += lfoInc;  if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            if (track > 0.001f)
            {
                // Pitch follower: two low-pass stages, then the time between rising zero crossings (held between notes)
                dLp1 += detA * (x[i] - dLp1);  dLp2 += detA * (dLp1 - dLp2);
                ++sinceCross;
                if (dLp2 > 0.01f && dArmed)
                {
                    dArmed = false;
                    const float hz = sampleRate / (float) std::max (1, sinceCross);
                    sinceCross = 0;
                    if (hz > 45.0f && hz < 1300.0f) detHz += 0.35f * (hz - detHz);
                }
                else if (dLp2 < -0.01f) dArmed = true;
                if (sinceCross > (int) (0.05f * sampleRate)) sinceCross = (int) (0.05f * sampleRate);
            }
            // Track: the carrier follows the note you play, Freq becoming a ratio (330 Hz = the same pitch as the note)
            const float targetHz = track > 0.001f
                ? std::exp ((1.0f - track) * std::log (freqHz) + track * std::log (detHz * freqHz / 330.0f))
                : freqHz;
            freqSmoothed += freqCoef * (targetHz - freqSmoothed);

            const float f = juce::jlimit (0.5f, 0.45f * sampleRate,
                                          freqSmoothed * std::exp2 (wobble * std::sin (twoPi * lfoPhase)));
            carrierPhase += f / sampleRate;
            if (carrierPhase >= 1.0f) carrierPhase -= 1.0f;

            float c;
            if (wave == 0)      c = std::sin (twoPi * carrierPhase);
            else if (wave == 1) c = fastTanh (6.0f * std::sin (twoPi * carrierPhase));   // slightly rounded square
            else                c = 1.0f - 4.0f * std::abs (carrierPhase - 0.5f);         // triangle

            const float dry = x[i];
            const float wet = (mode == 0) ? 1.2f * dry * c
                                          : 1.5f * dry * (0.5f + 0.5f * c);

            const float m = mix * amount;
            x[i] = dry * (1.0f - m) + wet * m;
        }
    }

private:
    float sampleRate = 44100.0f;
    float amount = 0.0f, carrierPhase = 0.0f, lfoPhase = 0.0f, freqSmoothed = 330.0f;
    float dLp1 = 0.0f, dLp2 = 0.0f, detHz = 220.0f;
    int   sinceCross = 0;
    bool  dArmed = true;
};

//==============================================================================
// A one-pole filter (6 dB per octave). Cheap, and fine for shaping fuzz tones.
//==============================================================================
// b20: fitted constants for the Factory fuzz; the test build (STARTERAMP_TUNE) can override them from the environment
inline float ffTune (const char* name, float def)
{
   #ifdef STARTERAMP_TUNE
    if (const char* e = std::getenv (name)) return (float) std::atof (e);
   #else
    (void) name;
   #endif
    return def;
}
#define FF_K1A ffTune ("FF_K1A", -1.0f)
#define FF_K1B ffTune ("FF_K1B", 0.0f)
#define FF_K2A ffTune ("FF_K2A", 8.0f)
#define FF_K2B ffTune ("FF_K2B", 10.0f)
#define FF_MA  ffTune ("FF_MA", -3.0f)
#define FF_MB  ffTune ("FF_MB", 3.0f)
#define FF_HP  ffTune ("FF_HP", 5.0f)
#define FF_COMP ffTune ("FF_COMP", 4.0f)
#define FF_FB  ffTune ("FF_FB", 1.0f)
#define FF_BIAS ffTune ("FF_BIAS", 6.5f)
#define FF_DR  ffTune ("FF_DR", 1.25f)
struct OnePole
{
    float a = 0.1f, z = 0.0f;

    void setCutoff (float fc, float sampleRate)
    {
        fc = juce::jlimit (5.0f, 0.45f * sampleRate, fc);
        a = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * fc / sampleRate);
    }
    float lp (float x) { z += a * (x - z); return z; }
    float hp (float x) { return x - lp (x); }
    void reset() { z = 0.0f; }
};

//==============================================================================
// Fuzz pedal with four switchable styles, each a hand-built approximation of
// the general character of a classic (not a component-level model):
//   0  Wall         : two cascaded clipping stages + the Muff tone control
//                     (a blend of a low-pass and a high-pass, so mids scoop).
//   1  Tight        : a high-gain op-amp stage whose bandwidth shrinks as the
//                     gain goes up, hard diode clipping, then a low-pass.
//   2  Gated        : asymmetric clipping with a "starved" bias (Gate) and
//                     positive feedback (Stab) for gated, spitty, squealing fuzz.
//   3  Buzzsaw      : a very-high-gain stage into diode clipping, then a three-band
//                     split with the mids scooped and Low / High boosts. Tone moves
//                     the band corners (the "color" sweep). Thick, sawing, death-metal buzz.
// It runs at 4x the sample rate (2x on the Balanced / Fast amp qualities) in its own oversampler.
//==============================================================================
// AutoLeveler: keeps the level of an effect close to the level that went into it.
// It compares slow RMS readings of the signal before and after the effect and moves a gain
// (within about +/-26 dB) towards the ratio. Used for the Fuzz and the Post EQ "Auto level" options.
// It holds its gain during silence. 'trim' is the user's own Level knob, relative to its default.
//==============================================================================
class AutoLeveler
{
public:
    void prepare (double sr)
    {
        eC = 1.0f - std::exp (-1.0f / (float) (sr * 0.6));
        gC = 1.0f - std::exp (-1.0f / (float) (sr * 0.12));
        reset();
    }
    void reset() { inE = outE = 0.0f; gain = 1.0f; }

    // ref = channel 0 before the effect; chans = the signal after it (channel 0 is measured, all are scaled)
    void process (const float* ref, float* const* chans, int numCh, int n, float trim)
    {
        for (int i = 0; i < n; ++i)
        {
            const float o = chans[0][i];
            inE  += eC * (ref[i] * ref[i] - inE);
            outE += eC * (o * o - outE);
            if (inE > 1.0e-9f && outE > 1.0e-10f)
                gain += gC * (juce::jlimit (0.05f, 20.0f, std::sqrt (inE / outE)) - gain);
            const float g = gain * trim;
            for (int c = 0; c < numCh; ++c)
                chans[c][i] *= g;
        }
    }

private:
    float eC = 1.0e-4f, gC = 1.0e-3f, inE = 0.0f, outE = 0.0f, gain = 1.0f;
};

//==============================================================================
class FuzzEffect
{
public:
    struct Params
    {
        int   style;
        float gain, tone, level, gate, stab;   // 'level' is a linear gain
        float buzzLow = 0.6f, buzzHigh = 0.5f;  // Buzzsaw style only
        int   stages = 2;                       // 2 = run at 4x, 1 = run at 2x, 0 = no oversampling (zero added delay)
        float mix = 1.0f;                       // 1 = all fuzz; less blends in the clean guitar (delay-matched)
        // ---- v56: the Face, Bender and Factory styles (4 to 6) and options for every style. All default to "off". ----
        bool  germanium = true;                 // styles 4 to 6: germanium (soft, warm, leaky) or silicon (tight, bright, hard)
        float starve = 0.0f;                    // styles 4, 5: 0 = full battery, 1 = nearly dead (gated, spitty)
        float input = 1.0f;                     // styles 4 to 6: the guitar's volume knob (1 = full; lower cleans up)
        float comp = 0.5f;                      // style 6: compression
        int   benderModel = 1;                  // style 5: 0 early (2 stage), 1 classic (3 stage), 2 late (3 stage + tone sweep)
        bool  buffer = false;                   // styles 4 to 6: a buffer in front (no pickup loading: brighter, tighter)
        int   muffVersion = 0;                  // style 0: 0 standard (as before), 1 smooth, 2 gritty, 3 mid-forward, 4 heavy, 5 to 7 circuits (3 variants), 8 op-amp circuit
        int   muffDiode = 0;                    // style 0: clipping diodes: 0 silicon (as before), 1 LED, 2 germanium
        bool  muffOpAmp = false;                // style 0: op-amp clipping stages instead of transistor stages
        bool  muffTonePass = false;             // style 0: bypass the tone control
        float muffMids = 0.5f;                  // style 0: 0.5 = as before, lower scoops the mids more, higher fills them in
        float drift = 0.0f, hiss = 0.0f;        // all styles: temperature drift of the gain / bias, and transistor hiss
        float octave = 0.0f;                   // all styles: an octave-up rectifier in front
        // ---- v66: the Nuke (7) and Crusher (8) styles ----
        float shape = 0.5f;                     // Nuke: which wave-shaping curve (0 to 1 moves through five tables, blending neighbours)
        float crush = 0.0f;                     // Nuke: bit depth and sample rate reduced together (0 = off)
        float bits = 8.0f;                      // Crusher: bit depth, 1 to 16
        float rate = 0.3f;                      // Crusher: sample-rate reduction, 0 = none, 1 = about 1/32
        // ---- v94: Buzzsaw extras (from the N.E.W. DIY HM-2 schematic) ----
        float buzzMid = 0.5f;                   // the split MIDS pot
        bool  buzzSoft = true, buzzHard = true; // the SOFT (feedback diodes) and HARD (shunt diodes) switches; both on = the classic circuit
        float buzzBlend = 0.0f;                 // clean copy of the input mixed in (0 = none)
        bool  buzzPol = false;                  // the clean copy inverted
        // ---- v95: style 9 (power-chip distortion: Low and High reuse buzzLow / buzzHigh) and style 10 (octave + RAT-type + boost) ----
        float lifeOct = 0.0f, lifeBoost = 0.0f;
        // ---- v97: the metal pedal styles (11 to 14) ----
        float verbMix = 0.4f, verbDecay = 4.0f, verbMod = 0.3f; int verbProgram = 0, verbOrder = 0; bool verbHold = false;
        float lCut = 0.3f, lEq0 = 0.5f, lEq1 = 0.5f, lEq2 = 0.5f, lEq3 = 0.5f, lEq4 = 0.5f, lDepth = 0.5f;
        float sub = 0.0f;                       // all styles: an octave-down (flip-flop divider) square in front, 0 = off
    };

    void prepare (double sampleRate, int maxBlockSize)
    {
        baseRate = (float) sampleRate;  verbReady = false;  vHp = vLp = 0.0f;
        dryBuf.assign ((size_t) maxBlockSize + 16, 0.0f);      // so processing never has to grow it
        fuzzVerb.prepare (baseRate); verbShifter.prepare (baseRate); verbShifter.setWindowMs (45.0f); verbShifter.setSemitones (-12.0f); verbReady = true;
        oversampling.initProcessing ((size_t) maxBlockSize);
        oversampling.reset();
        oversampling1.initProcessing ((size_t) maxBlockSize);
        oversampling1.reset();

        for (auto* f : { &inHp, &stage1Lp, &stageHp, &stage2Lp, &toneLow, &toneHigh,
                         &emphHp, &opAmpLp, &ratFilter, &fbHp, &fbLp, &dcBlock, &outTone,
                         &bzLow, &bzHigh, &bzOut, &nLoad, &nHp1, &nLp1, &nMidA, &nMidB, &octLp, &hissLp, &dcBlock2, &subLpA, &subLpB, &subOutLp })
            f->reset();
        subEnv = 0.0f;  subState = 1.0f;  subArmed = true;  subCut = 450.0f;  subF = 300.0f;  subCount = 0;

        lpIc1 = lpIc2 = dcLow = holdCnt = heldV = 0.0f;
        yPrev = 0.0f;
        env = 0.0f;  drift1 = 0.0f;  drift2 = 1.7f;  driftGain = 1.0f;  driftBias = 0.0f;
        lastLevel = -1.0f;  muffCircuit.reset();  opAmpMuff.reset();  hm2.reset();  circFs = 0.0f;
        powerChip.reset();  lifeCirc.reset();  chipFs = 0.0f;  metalPed.prepare (48000.0);
        lastMix = 1.0f;
        dryRing.fill (0.0f);
        random.setSeed (777);
    }

    // Processes one channel in place. With Mix below 1 the clean guitar is blended back in, delayed by the
    // same amount as the oversampled fuzz path, so the two stay in phase (no comb filtering).
    void process (float* x, int numSamples, const Params& p)
    {
        const bool blend = p.mix < 0.9999f || lastMix < 0.9999f;
        if (! blend)
        {
            processWet (x, numSamples, p);
            lastMix = 1.0f;
            return;
        }

        if ((int) dryBuf.size() < numSamples) dryBuf.resize ((size_t) numSamples);
        const int lat = juce::jlimit (0, kDryMask,
                           p.stages >= 2 ? (int) std::lround (oversampling.getLatencyInSamples())
                         : p.stages == 1 ? (int) std::lround (oversampling1.getLatencyInSamples()) : 0);
        for (int i = 0; i < numSamples; ++i)
        {
            dryRing[(size_t) (dryPos & kDryMask)] = x[i];
            dryBuf[(size_t) i] = dryRing[(size_t) ((dryPos - lat) & kDryMask)];
            ++dryPos;
        }

        processWet (x, numSamples, p);

        for (int i = 0; i < numSamples; ++i)
        {
            const float m = lastMix + (p.mix - lastMix) * (float) i / (float) numSamples;
            x[i] = dryBuf[(size_t) i] + m * (x[i] - dryBuf[(size_t) i]);
        }
        lastMix = p.mix;
    }

private:
    void processWet (float* x, int numSamples, const Params& p)
    {
        if (p.style != 12) { processWetCore (x, numSamples, p); return; }
        FuzzVerb::Params vp;  vp.mix = p.verbMix;  vp.decaySec = p.verbDecay;  vp.mod = p.verbMod;  vp.program = p.verbProgram;  vp.hold = p.verbHold;
        auto shift = [this] (float v) { return verbShifter.process (v); };
        if (! verbReady) { fuzzVerb.prepare (baseRate); verbShifter.prepare (baseRate); verbShifter.setWindowMs (45.0f); verbShifter.setSemitones (-12.0f); verbReady = true; }
        if (p.verbOrder == 1) { fuzzVerb.process (x, numSamples, vp, shift); processWetCore (x, numSamples, p); }
        else                  { processWetCore (x, numSamples, p); fuzzVerb.process (x, numSamples, vp, shift); }
    }

    void processWetCore (float* x, int numSamples, const Params& p)
    {
        osRate = baseRate * (p.stages >= 2 ? 4.0f : (p.stages == 1 ? 2.0f : 1.0f));
        configure (p);

        if (p.stages <= 0)
        {
            run (x, numSamples, p);
            applyLevel (x, numSamples, p);
            return;
        }

        float* channels[1] = { x };
        juce::dsp::AudioBlock<float> block (channels, 1, (size_t) numSamples);

        auto& os = (p.stages >= 2) ? oversampling : oversampling1;
        auto up = os.processSamplesUp (block);
        float* d = up.getChannelPointer (0);
        const int m = (int) up.getNumSamples();

        run (d, m, p);

        os.processSamplesDown (block);
        applyLevel (x, numSamples, p);
    }

    void run (float* d, int m, const Params& p)
    {
        // v56 options in front of every style (each costs nothing while it is off)
        if (p.drift > 0.0001f)
        {
            const float twoPi = juce::MathConstants<float>::twoPi;
            drift1 += twoPi * 0.11f * (float) m / osRate;   if (drift1 > twoPi) drift1 -= twoPi;
            drift2 += twoPi * 0.29f * (float) m / osRate;   if (drift2 > twoPi) drift2 -= twoPi;
            const float dv = 0.6f * std::sin (drift1) + 0.4f * std::sin (drift2);    // -1 to 1, slow and uneven
            driftGain = std::pow (10.0f, 3.0f * p.drift * dv / 20.0f);                // up to +/-3 dB of gain
            driftBias = 0.08f * p.drift * dv;                                          // and a wandering bias point
        }
        else
        {
            driftGain = 1.0f;  driftBias = 0.0f;
        }
        if (p.hiss > 0.0001f || p.octave > 0.001f || p.drift > 0.0001f || p.sub > 0.001f)
        {
            subLpA.setCutoff (subCut, osRate);  subLpB.setCutoff (subCut, osRate);  subOutLp.setCutoff (320.0f, osRate);
            const float subAtk = 1.0f - std::exp (-1.0f / (0.004f * osRate)), subRel = 1.0f - std::exp (-1.0f / (0.060f * osRate));
            const float hissAmp = p.hiss * 0.0015f * std::sqrt (osRate / 48000.0f);
            octLp.setCutoff (60.0f, osRate);
            hissLp.setCutoff (7000.0f, osRate);
            for (int i = 0; i < m; ++i)
            {
                float x = d[i] * driftGain;
                if (p.octave > 0.001f)
                {
                    // Octave up (full-wave rectifier). Same law as Octave down below: the dry note ducks as the amount rises
                    const float r = std::fabs (x);
                    x = x * (1.0f - 0.6f * p.octave) + p.octave * 2.0f * (r - octLp.lp (r));
                }
                if (p.sub > 0.001f)
                {
                    // Octave down the old analogue way: a flip-flop toggled by each rising zero crossing of the (low-passed) note gives
                    // a square wave an octave down, scaled by the playing level. It tracks single notes best and glitches on chords
                    // and on the highest notes, which is part of the sound.
                    const float a = std::fabs (x);
                    subEnv += (a > subEnv ? subAtk : subRel) * (a - subEnv);
                    const float s = subLpB.lp (subLpA.lp (x));
                    const float thr = std::max (0.004f, 0.12f * subEnv);
                    ++subCount;
                    if (s > thr && subArmed)
                    {
                        subState = -subState;  subArmed = false;
                        const float hz = osRate / (float) std::max (1, subCount);          // the pitch filter follows the note (1.4 x its pitch)
                        subCount = 0;
                        if (hz > 40.0f && hz < 1200.0f)
                        {
                            subF += 0.5f * (hz - subF);
                            subCut = juce::jlimit (70.0f, 700.0f, 1.4f * subF);
                            subLpA.setCutoff (subCut, osRate);  subLpB.setCutoff (subCut, osRate);
                        }
                    }
                    else if (s < -0.5f * thr)          subArmed = true;
                    const float sq = subOutLp.lp (subState) * std::min (1.0f, subEnv * 40.0f);
                    x = x * (1.0f - 0.6f * p.sub) + p.sub * 1.7f * subEnv * sq;
                }
                if (hissAmp > 0.0f)
                    x += hissAmp * hissLp.lp (2.0f * (random.nextFloat() - 0.5f));
                d[i] = x;
            }
        }

        if      (p.style == 9)
        {
            if (std::fabs (chipFs - osRate) > 0.5f) { powerChip.prepare (osRate); lifeCirc.prepare (osRate); metalPed.prepare (osRate); chipFs = osRate; }
            powerChip.configure (p.buzzLow, p.buzzHigh, std::pow (10.0f, (p.gain - 0.5f) * 1.5f));
            for (int i = 0; i < m; ++i) d[i] = kPowerChipTrim * powerChip.process (d[i]);
        }
        else if (p.style == 10)
        {
            if (std::fabs (chipFs - osRate) > 0.5f) { powerChip.prepare (osRate); lifeCirc.prepare (osRate); metalPed.prepare (osRate); chipFs = osRate; }
            Circuit::LifeCircuit::Params lp;
            lp.dist = p.gain;  lp.filter = p.tone;  lp.octave = p.lifeOct;  lp.boost = p.lifeBoost;  lp.diode = p.muffDiode;
            lifeCirc.configure (lp);
            for (int i = 0; i < m; ++i) d[i] = kLifeTrim * lifeCirc.process (d[i]);
        }
        else if (p.style == 11)
        {
            if (std::fabs (chipFs - osRate) > 0.5f) { powerChip.prepare (osRate); lifeCirc.prepare (osRate); metalPed.prepare (osRate); chipFs = osRate; }
            Circuit::MetalPedal::Params mp;
            mp.style = 14;  mp.gain = p.gain;  mp.cut = p.lCut;  mp.eq[0] = p.lEq0;  mp.eq[1] = p.lEq1;  mp.eq[2] = p.lEq2;  mp.eq[3] = p.lEq3;  mp.eq[4] = p.lEq4;
            mp.depth = p.lDepth;  mp.comp = p.comp;  mp.boost = p.lifeBoost;  mp.gate = p.gate;  mp.diode = p.muffDiode;
            metalPed.configure (mp);
            const float trim = kMetalTrim[3];
            for (int i = 0; i < m; ++i) d[i] = trim * metalPed.process (d[i]);
        }
        else if (p.style == 12)
        {
            // Distortion for the fuzz + reverb style: a high-pass, gain, an asymmetric soft clip, then a tone low-pass. Gain at 0 is a clean pass.
            const float g = 1.0f + 120.0f * p.gain * p.gain, norm = 1.0f / std::pow (g, 0.40f);
            const float hpC = 1.0f - std::exp (-6.2831853f * 70.0f / osRate);
            const float lpC = 1.0f - std::exp (-6.2831853f * (1400.0f + 7500.0f * p.tone * p.tone) / osRate);
            for (int i = 0; i < m; ++i)
            {
                vHp += hpC * (d[i] - vHp);
                const float x = (d[i] - vHp) * g;
                const float y = std::tanh (x + 0.12f * x * x * (x > 0.0f ? 1.0f : 0.4f)) * norm * 3.0f;
                vLp += lpC * (y - vLp);
                d[i] = kVerbDistTrim * vLp;
            }
        }
        else if (p.style == 0 && ((p.muffVersion >= 5 && p.muffVersion <= 7) || p.muffVersion == 9 || p.muffVersion == 10))
        {
            if (std::fabs (circFs - osRate) > 0.5f) { muffCircuit.prepare (osRate); opAmpMuff.prepare (osRate); circFs = osRate; }
            Circuit::MuffCircuit::Params cp;
            cp.sustain = p.gain;  cp.tone = p.tone;  cp.diode = p.muffDiode;  cp.opAmp = p.muffOpAmp;  cp.tonePass = p.muffTonePass;
            cp.variant = p.muffVersion == 10 ? 4 : p.muffVersion == 9 ? 3 : p.muffVersion - 5;
            muffCircuit.configure (cp);
            const float trim = kMuffCircuitTrim[p.muffVersion == 10 ? 5 : p.muffVersion == 9 ? 4 : p.muffVersion - 5];
            for (int i = 0; i < m; ++i) d[i] = trim * muffCircuit.process (d[i]);
        }
        else if (p.style == 0 && p.muffVersion == 8)
        {
            if (std::fabs (circFs - osRate) > 0.5f) { muffCircuit.prepare (osRate); opAmpMuff.prepare (osRate); circFs = osRate; }
            Circuit::OpAmpMuffCircuit::Params op;
            op.sustain = p.gain;  op.tone = p.tone;  op.diode = p.muffDiode;  op.tonePass = p.muffTonePass;
            opAmpMuff.configure (op);
            for (int i = 0; i < m; ++i) d[i] = kMuffCircuitTrim[3] * opAmpMuff.process (d[i]);
        }
        else if (p.style == 0) { if (muffIsDefault (p)) for (int i = 0; i < m; ++i) d[i] = muff (d[i], p);
                                 else                   for (int i = 0; i < m; ++i) d[i] = muff2 (d[i], p); }
        else if (p.style == 1) for (int i = 0; i < m; ++i) d[i] = tightFuzz (d[i]);
        else if (p.style == 2) for (int i = 0; i < m; ++i) d[i] = factory (d[i]);
        else if (p.style == 4) for (int i = 0; i < m; ++i) d[i] = face (d[i], p);
        else if (p.style == 5) for (int i = 0; i < m; ++i) d[i] = bender (d[i], p);
        else if (p.style == 6) for (int i = 0; i < m; ++i) d[i] = factory2 (d[i], p);
        else if (p.style == 7) nuke (d, m, p);
        else if (p.style == 8) crusher (d, m, p);
        else
        {
            // Buzzsaw (v93): the heavy-metal pedal's circuit (two gain stages, op-amp, clipper, and its two-band tone network).
            if (std::fabs (circFs - osRate) > 0.5f) { muffCircuit.prepare (osRate); opAmpMuff.prepare (osRate); circFs = osRate; hm2.prepare (osRate); }
            Circuit::Hm2Circuit::Params hp;  hp.dist = p.gain;  hp.low = p.buzzLow;  hp.high = p.buzzHigh;
            hp.mid = p.buzzMid;  hp.soft = p.buzzSoft;  hp.hard = p.buzzHard;  hp.blend = p.buzzBlend;  hp.pol = p.buzzPol;
            hm2.configure (hp);
            for (int i = 0; i < m; ++i) d[i] = kHm2Trim * hm2.process (d[i]);
        }
    }

    // ---- v66: Nuke: a wave-shaper fuzz. Drive, then the signal is mapped through a curve (a "wavetable" of five
    //      families, with Shape morphing between neighbours: soft, sine-fold, odd-harmonic, staircase, asymmetric fold),
    //      then an optional bit and sample-rate reduction (Crush), then a slightly resonant two-pole output filter (Tone).
    //      My own simple design from a general description of wave-table distortion, not a copy of any product.
    void buildNukeTables()
    {
        if (nukeBuilt) return;
        for (int i = 0; i <= kNukeN; ++i)
        {
            const float x = -1.0f + 2.0f * (float) i / (float) kNukeN;
            const float x2 = x * x, x3 = x2 * x, x5 = x3 * x2;
            nukeTab[0][i] = std::tanh (2.2f * x);
            nukeTab[1][i] = std::sin (3.9f * x);
            nukeTab[2][i] = 0.6f * (4.0f * x3 - 3.0f * x) + 0.4f * (16.0f * x5 - 20.0f * x3 + 5.0f * x);
            nukeTab[3][i] = 0.8f * (std::round (x * 5.0f) / 5.0f) + 0.2f * x;
            nukeTab[4][i] = std::sin (2.6f * x + 1.4f * x2 * (x < 0 ? -1.0f : 1.0f) + 0.5f);
        }
        for (int t = 0; t < 5; ++t)
        {
            float mx = 1.0e-6f;
            for (int i = 0; i <= kNukeN; ++i) mx = std::max (mx, std::abs (nukeTab[t][i]));
            for (int i = 0; i <= kNukeN; ++i) nukeTab[t][i] /= mx;
        }
        nukeBuilt = true;
    }
    inline float nukeLookup (float x, int t0, int t1, float w) const
    {
        const float u = (juce::jlimit (-1.0f, 1.0f, x) + 1.0f) * 0.5f * (float) kNukeN;
        const int   i = std::min ((int) u, kNukeN - 1);
        const float f = u - (float) i;
        const float a = nukeTab[t0][i] + f * (nukeTab[t0][i + 1] - nukeTab[t0][i]);
        const float b = nukeTab[t1][i] + f * (nukeTab[t1][i + 1] - nukeTab[t1][i]);
        return a + w * (b - a);
    }
    // A two-pole low-pass with a little resonance (state-variable), the "output filter"
    inline float outputLp (float x, float g, float k)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - lpIc2, v1 = a1 * lpIc1 + a2 * v3, v2 = lpIc2 + a2 * lpIc1 + a3 * v3;
        lpIc1 = 2.0f * v1 - lpIc1;  lpIc2 = 2.0f * v2 - lpIc2;
        return v2;
    }
    inline float crushStep (float x, float holdLen, float levels)
    {
        holdCnt += 1.0f;
        if (holdCnt >= holdLen) { holdCnt -= holdLen; heldV = x; }
        return std::round (heldV * levels) / levels;
    }
    void nuke (float* d, int m, const Params& p)
    {
        buildNukeTables();
        const float drive = 3.0f + 40.0f * p.gain * p.gain;
        const float pos = juce::jlimit (0.0f, 1.0f, p.shape) * 4.0f;
        const int   t0 = std::min ((int) pos, 3);
        const float w = pos - (float) t0;
        const float c = juce::jlimit (0.0f, 1.0f, p.crush);
        const float holdLen = (1.0f + 11.0f * c) * (osRate / baseRate);
        const float levels = std::pow (2.0f, 15.0f - 13.0f * c - 1.0f);
        const float fc = std::min (0.45f * osRate, 1500.0f * std::pow (12.0f, p.tone));
        const float g = std::tan (juce::MathConstants<float>::pi * fc / osRate);
        const float dcK = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 25.0f / osRate);
        for (int i = 0; i < m; ++i)
        {
            float y = std::tanh (drive * d[i]);
            y = nukeLookup (y, t0, t0 + 1, w);
            if (c > 0.001f) y = crushStep (y, holdLen, levels);
            dcLow += dcK * (y - dcLow);
            y -= dcLow;
            d[i] = 1.0f * outputLp (y, g, 1.0f / 0.9f);
        }
    }
    // ---- v66: Crusher: a bit-crusher with drive in front (Gain), sample-rate reduction (Rate), bit depth (Bits) and a low-pass (Tone)
    void crusher (float* d, int m, const Params& p)
    {
        const float drive = 1.0f + 7.0f * p.gain;
        const float holdLen = std::pow (2.0f, 5.0f * juce::jlimit (0.0f, 1.0f, p.rate)) * (osRate / baseRate);
        const float levels = std::pow (2.0f, juce::jlimit (1.0f, 16.0f, p.bits) - 1.0f);
        const float fc = std::min (0.45f * osRate, 1500.0f * std::pow (12.0f, p.tone));
        const float g = std::tan (juce::MathConstants<float>::pi * fc / osRate);
        for (int i = 0; i < m; ++i)
        {
            float y = std::tanh (drive * d[i]);
            y = crushStep (y, holdLen, levels);
            d[i] = 2.0f * outputLp (y, g, 1.0f / 0.8f);
        }
    }
    static constexpr int kNukeN = 1024;
    float nukeTab[5][kNukeN + 1] {};
    bool  nukeBuilt = false;
    float lpIc1 = 0.0f, lpIc2 = 0.0f, dcLow = 0.0f, holdCnt = 0.0f, heldV = 0.0f;

    Circuit::MuffCircuit muffCircuit;
    Circuit::Hm2Circuit hm2;
    static constexpr float kHm2Trim = 1.15f;
    Circuit::OpAmpMuffCircuit opAmpMuff;
    float circFs = 0.0f;
    static constexpr float kMuffCircuitTrim[6] = { 1.45f, 1.45f, 1.45f, 1.05f, 0.78f, 1.29f };
    Circuit::PowerChipCircuit powerChip;
    Circuit::LifeCircuit lifeCirc;
    Circuit::MetalPedal metalPed;
    FuzzVerb fuzzVerb;  GuitarShifter verbShifter;  bool verbReady = false;  float vHp = 0.0f, vLp = 0.0f;
    static constexpr float kVerbDistTrim = 1.4f;
    static constexpr float kMetalTrim[4] = { 1.57f, 1.3f, 2.08f, 3.2f };    // set from measurement
    float chipFs = 0.0f;
    static constexpr float kPowerChipTrim = 0.34f, kLifeTrim = 1.3f;     // set from a measurement against the original Wall at the same settings

    static bool muffIsDefault (const Params& p)
    {
        return p.muffVersion == 0 && p.muffDiode == 0 && ! p.muffOpAmp && ! p.muffTonePass
            && std::fabs (p.muffMids - 0.5f) < 0.0005f;
    }

    // Output level (ramped across the block to avoid zipper noise)
    void applyLevel (float* x, int numSamples, const Params& p)
    {
        if (lastLevel < 0.0f) lastLevel = p.level;
        for (int i = 0; i < numSamples; ++i)
            x[i] *= lastLevel + (p.level - lastLevel) * (float) i / (float) numSamples;
        lastLevel = p.level;
    }

    // Smooth hard clip to +/-1 (a sharper bend than tanh).
    static float hardKnee (float x)
    {
        const float a2 = x * x;
        return x / std::sqrt (std::sqrt (1.0f + a2 * a2));
    }

    // Sets the filters and gains for this block (cheap: no allocation).
    void configure (const Params& p)
    {
        const float fs = osRate;
        inHp.setCutoff (p.style == 2 ? 120.0f : (p.style == 1 ? 30.0f : (p.style == 3 ? 40.0f : 50.0f)), fs);
        dcBlock.setCutoff (30.0f, fs);
        dcBlock2.setCutoff (30.0f, fs);

        if (p.style == 0)            // Wall
        {
            stageGain1 = 4.0f * std::pow (15.0f, p.gain);     // 4 to 60
            stageGain2 = 3.0f * std::pow (3.3f,  p.gain);     // 3 to 10
            float lp1Hz = 6000.0f, lp2Hz = 4500.0f;
            stageHp.setCutoff  (70.0f, fs);
            toneLow.setCutoff  (408.0f, fs);                  // the Muff tone control's
            toneHigh.setCutoff (1855.0f, fs);                 // two filter corners
            muffOut = 1.6f;
            if (p.muffVersion > 0)                            // v56: the versions
            {
                //                         gain1 gain2  hp    lp1     lp2    toneLo toneHi out
                static const float ver[5][8] = { { 1.0f, 1.0f,  70.0f, 6000.0f, 4500.0f, 408.0f, 1855.0f, 1.6f },
                                                 { 0.9f, 0.9f,  70.0f, 5000.0f, 4000.0f, 380.0f, 2050.0f, 1.7f },
                                                 { 1.1f, 1.0f,  55.0f, 7000.0f, 5000.0f, 420.0f, 1700.0f, 1.5f },
                                                 { 1.0f, 1.1f,  95.0f, 6500.0f, 5200.0f, 500.0f, 1500.0f, 1.5f },
                                                 { 1.25f, 1.1f, 45.0f, 5000.0f, 4000.0f, 360.0f, 2000.0f, 1.5f } };
                const float* v = ver[juce::jlimit (0, 4, p.muffVersion)];
                stageGain1 *= v[0];  stageGain2 *= v[1];
                stageHp.setCutoff (v[2], fs);  lp1Hz = v[3];  lp2Hz = v[4];
                toneLow.setCutoff (v[5], fs);  toneHigh.setCutoff (v[6], fs);
                muffOut = v[7];
            }
            // v0.1: the inter-stage roll-offs follow Sustain. Real Muffs keep nearly the same tonal balance from low to high
            // sustain (captures: under 2 dB of drift at 8 kHz), but more clipping alone adds top-end harmonics, so the top
            // used to climb about 5 dB. The gain range itself is unchanged.
            stage1Lp.setCutoff (lp1Hz * std::pow (0.6f, p.gain), fs);
            stage2Lp.setCutoff (lp2Hz * std::pow (0.6f, p.gain), fs);
        }
        else if (p.style == 1)       // Tight
        {
            ratGain = std::pow (10.0f, (6.0f + 54.0f * p.gain) / 20.0f);   // +6 dB to +60 dB
            emphHp.setCutoff (1500.0f, fs);
            opAmpLp.setCutoff (juce::jlimit (600.0f, 20000.0f, 800000.0f / ratGain), fs);
            ratFilter.setCutoff (475.0f * std::pow (32000.0f / 475.0f, p.tone), fs);
        }
        else if (p.style == 3)       // Buzzsaw
        {
            bzGain = std::pow (10.0f, (26.0f + 30.0f * p.gain) / 20.0f);   // +26 dB to +56 dB
            opAmpLp.setCutoff (juce::jlimit (3000.0f, 16000.0f, 1200000.0f / bzGain), fs);
            bzLow.setCutoff  (110.0f * std::pow (3.5f, p.tone), fs);      // 110 Hz to 385 Hz
            bzHigh.setCutoff (1200.0f * std::pow (3.3f, p.tone), fs);     // 1.2 kHz to 4 kHz
            bzOut.setCutoff  (9000.0f, fs);
            bzLowGain  = 0.4f + 2.2f * p.buzzLow;
            bzHighGain = 0.4f + 2.2f * p.buzzHigh;
        }
        else if (p.style == 4)       // Face (v56): two transistors, no tone control (Tone is a gentle top-end roll-off)
        {
            const bool ge = p.germanium;
            inHp.setCutoff (35.0f, fs);
            nLoad.setCutoff (p.buffer ? 20000.0f : (ge ? 3600.0f : 4800.0f), fs);     // the pickup loads an unbuffered input
            inGain = p.input * p.input;                                               // the guitar's volume pot (audio taper)
            g1 = (ge ? 6.0f : 8.0f) * std::pow (14.0f, p.gain);                       // 6 to 84 (8 to 112)
            g2 = ge ? 2.2f : 3.0f;
            vmax = 1.0f - 0.6f * p.starve;                                            // a weaker battery gives less headroom
            bias = (ge ? 0.28f : 0.08f) + 0.30f * p.starve;
            biasOff = vmax * trClip (bias / vmax, ge);
            dead = p.starve * (ge ? 0.28f : 0.20f) * vmax;                            // and a gap at the zero crossing: gated, spitty
            makeup = 1.0f + 1.2f * p.starve;
            outTone.setCutoff ((ge ? 1500.0f : 2200.0f) * std::pow (ge ? 7.0f : 6.0f, p.tone), fs);
            dcBlock.setCutoff (30.0f, fs);
            nHp1.setCutoff (80.0f, fs);
            outScale = ge ? 0.75f : 0.64f;
        }
        else if (p.style == 5)       // Bender (v56): two or three transistors, raspy, with a mid emphasis
        {
            const bool ge = p.germanium;
            const int mdl = juce::jlimit (0, 2, p.benderModel);
            static const float hpHz[3]   = { 60.0f, 90.0f, 140.0f };
            static const float midLo[3]  = { 500.0f, 1200.0f, 1000.0f };
            static const float midHi[3]  = { 1500.0f, 3200.0f, 2800.0f };
            static const float midAmt[3] = { 0.8f, 1.2f, 0.9f };
            inHp.setCutoff (hpHz[mdl], fs);
            nLoad.setCutoff (p.buffer ? 20000.0f : (ge ? 5000.0f : 6500.0f), fs);
            inGain = p.input * p.input;
            const float sil = ge ? 1.0f : 1.3f;
            g1 = sil * (mdl == 0 ? 5.0f : mdl == 1 ? 6.0f : 7.0f) * std::pow (mdl == 0 ? 12.0f : 14.0f, p.gain);
            g2 = sil * (mdl == 0 ? 3.5f : 4.0f);
            g3 = mdl == 0 ? 0.0f : (mdl == 1 ? 2.2f : 3.0f);
            vmax = 1.0f - 0.6f * p.starve;
            bias = (ge ? 0.30f : 0.10f) + 0.30f * p.starve;
            biasOff = vmax * trClip (bias / vmax, ge);
            dead = p.starve * (ge ? 0.30f : 0.22f) * vmax;
            makeup = 1.0f + 1.2f * p.starve;
            nMidA.setCutoff (midLo[mdl], fs);
            nMidB.setCutoff (midHi[mdl], fs);
            midAmt1 = midAmt[mdl];
            if (mdl == 2) outTone.setCutoff (1200.0f * std::pow (10.0f, p.tone), fs);
            else          outTone.setCutoff ((mdl == 0 ? 2500.0f : 2200.0f) * std::pow (mdl == 0 ? 5.0f : 6.0f, p.tone), fs);
            dcBlock.setCutoff (30.0f, fs);
            nHp1.setCutoff (90.0f, fs);
            nLp1.setCutoff (7500.0f, fs);
            outScale = ge ? 0.6f : 0.5f;
        }
        else if (p.style == 6)       // Factory (v56): gate, comp, drive, stab, with a germanium or silicon transistor
        {
            const bool ge = p.germanium;
            inHp.setCutoff (FF_HP, fs);                                  // b20: was 120 Hz; the captures keep more low end
            nLoad.setCutoff (p.buffer ? 20000.0f : 14000.0f, fs);        // b20: was 7.5 kHz (the captures are bright)
            ffS1.setCutoff (1500.0f, fs);  ffS2.setCutoff (5000.0f, fs);
            ffK1 = std::pow (10.0f, (FF_K1A + FF_K1B * p.stab) / 20.0f) - 1.0f;
            ffK2 = std::pow (10.0f, (FF_K2A + FF_K2B * p.stab) / 20.0f) - 1.0f;
            ffMake = std::pow (10.0f, (FF_MA + FF_MB * p.stab) / 20.0f);
            inGain = p.input * p.input;
            ffDrive = FF_DR * (ge ? 5.0f : 6.5f) * std::pow (20.0f, p.gain);   // 5 to 100
            ffBias  = p.gate * FF_BIAS;
            ffNoise = p.gate * 0.06f;
            ffFeedback = (1.0f - p.stab) * FF_FB;
            fbHp.setCutoff (200.0f, fs);
            fbLp.setCutoff (2500.0f, fs);
            outTone.setCutoff (3000.0f * std::pow (10.0f, 1.2f * p.tone), fs);   // b20: 3 kHz to 48 kHz (was 1.2 to 12 kHz; the real pedal has no tone control)
            dcBlock.setCutoff (30.0f, fs);
            envC = 1.0f - std::exp (-1.0f / (0.004f * fs));            // a 4 ms level follower for Comp
            compAmt = p.comp * FF_COMP;
            outScale = ge ? 0.8f : 0.7f;
        }
        else                         // Gated
        {
            ffDrive = 4.0f * std::pow (25.0f, p.gain);        // 4 to 100
            ffBias  = p.gate * 3.0f;                          // starves the stage
            ffNoise = p.gate * 0.06f;                         // tiny randomness at the threshold
            ffFeedback = (1.0f - p.stab) * 3.0f;
            fbHp.setCutoff (200.0f, fs);
            fbLp.setCutoff (2500.0f, fs);
            outTone.setCutoff (1200.0f * std::pow (10.0f, p.tone), fs);    // 1.2 kHz to 12 kHz
        }
    }

    float muff (float x, const Params& p)
    {
        float v = inHp.hp (x);
        v = fastTanh (stageGain1 * v);        // clipping stage 1
        v = stage1Lp.lp (v);
        v = stageHp.hp (v);
        v = fastTanh (stageGain2 * v);        // clipping stage 2
        v = stage2Lp.lp (v);

        // Muff tone control: blend a low-pass and a high-pass. Mid-way, the
        // mids dip, which is the classic scooped, wall-of-sound shape.
        const float low  = toneLow.lp (v);
        const float high = v - toneHigh.lp (v);
        return 1.6f * ((1.0f - p.tone) * low + p.tone * high);
    }

    // ---- v56 ----

    // A transistor stage's clip: germanium is soft (tanh); silicon is harder-edged.
    static float trClip (float x, bool ge)
    {
        return ge ? fastTanh (x) : 0.9f * hardKnee (1.25f * x) + 0.1f * fastTanh (x);
    }

    // The gap at the zero crossing that a starved (dying battery) stage has: small signals are cut, large ones pass.
    float deadZone (float y) const
    {
        return dead > 0.0001f ? y - dead * fastTanh (y / dead) : y;
    }

    float face (float x, const Params& p)
    {
        const bool ge = p.germanium;
        float v = nLoad.lp (inHp.hp (x) * inGain);
        float y = vmax * trClip ((g1 * v + bias + driftBias) / vmax, ge) - biasOff;
        y = deadZone (dcBlock.hp (y)) * makeup;
        y = nHp1.hp (y);
        const float z = vmax * trClip ((g2 * y + 0.5f * bias) / vmax, ge);
        return outScale * outTone.lp (dcBlock2.hp (z));
    }

    float bender (float x, const Params& p)
    {
        const bool ge = p.germanium;
        float v = nLoad.lp (inHp.hp (x) * inGain);
        float y = vmax * trClip ((g1 * v + bias + driftBias) / vmax, ge) - biasOff;
        y = deadZone (dcBlock.hp (y)) * makeup;
        y = nHp1.hp (y);
        y = vmax * trClip ((g2 * y + 0.5f * bias) / vmax, ge);                // second stage
        y += midAmt1 * (nMidB.lp (y) - nMidA.lp (y));                       // the mid emphasis that gives the rasp
        if (g3 > 0.0f)
            y = vmax * trClip ((g3 * nLp1.lp (y) + 0.25f * bias) / vmax, ge);   // third stage
        return outScale * outTone.lp (dcBlock2.hp (y));
    }

    float factory2 (float x, const Params& p)
    {
        const bool ge = p.germanium;
        float in = nLoad.lp (inHp.hp (x) * inGain);
        env += envC * (std::fabs (in) - env);
        in /= 1.0f + compAmt * env;                                          // Comp: squashes the input, which lifts quiet notes
        const float fbIn  = fbLp.lp (fbHp.hp (yPrev));
        const float noise = ffNoise * (random.nextFloat() - 0.5f);
        const float u = in * ffDrive - ffBias + driftBias * 4.0f + ffFeedback * fbIn + noise;
        const float y = ge ? ((u >= 0.0f) ? hardKnee (0.8f * u) : 0.45f * fastTanh (0.6f * u))
                           : ((u >= 0.0f) ? hardKnee (1.0f * u) : 0.6f  * fastTanh (0.9f * u));
        yPrev = y;
        float o = outTone.lp (dcBlock.hp (fastTanh (1.4f * y)));
        o += ffK1 * (o - ffS1.lp (o));  o += ffK2 * (o - ffS2.lp (o));
        return outScale * ffMake * o;
    }

    // The Muff stage: a diode-limited gain stage. Silicon is the original (tanh); LED has more headroom; germanium
    // clips early. With the op-amp option the stage runs out of swing first, then the diodes clip hard.
    float muffStage (float v, float g, float vd, bool opAmp) const
    {
        if (! opAmp)
            return vd * fastTanh (g * v / vd);
        const float r  = 4.5f * fastTanh (g * v / 4.5f);
        const float vk = 0.6f * vd;
        return (vk / 0.6f) * hardKnee (r / vk);
    }

    float muff2 (float x, const Params& p)
    {
        const float vd = p.muffDiode == 1 ? 1.6f : p.muffDiode == 2 ? 0.55f : 1.0f;
        float v = inHp.hp (x);
        v = muffStage (stageGain1 * v, 1.0f, vd, p.muffOpAmp);
        v = stage1Lp.lp (v);
        v = stageHp.hp (v);
        v = muffStage (stageGain2 * v, 1.0f, vd, p.muffOpAmp);
        v = stage2Lp.lp (v);
        v /= vd;

        if (p.muffTonePass)
            return muffOut * 0.44f * v;
        const float low  = toneLow.lp (v);
        const float high = v - toneHigh.lp (v);
        float out = (1.0f - p.tone) * low + p.tone * high;
        if (std::fabs (p.muffMids - 0.5f) > 0.0005f)
        {
            out += (p.muffMids - 0.5f) * 2.4f * (v - low - high);            // fill in (or scoop) the middle band
            out *= p.muffMids > 0.5f ? 1.0f / (1.0f + 0.83f * (p.muffMids - 0.5f))    // keep the level about the same
                                     : 1.0f + 0.4f * (0.5f - p.muffMids);
        }
        return muffOut * out;
    }

    float tightFuzz (float x)
    {
        // Op-amp gain stage. Highs get much more gain than lows, as in the pedal.
        const float v = inHp.hp (x);
        float amp = ratGain * (0.12f * v + 0.88f * emphHp.hp (v));
        amp = opAmpLp.lp (amp);                              // less bandwidth at high gain

        const float rail = 4.5f * fastTanh (amp / 4.5f);    // the op-amp runs out of swing
        const float clipped = hardKnee (rail / 0.6f);        // diodes to ground, about +/-0.6 V
        return 0.8f * ratFilter.lp (clipped);
    }

    float buzzsaw (float x)
    {
        float v = opAmpLp.lp (bzGain * inHp.hp (x));
        // Two diodes to ground, slightly unmatched, so the clipping is a little lopsided.
        v = (v >= 0.0f) ? hardKnee (v / 0.7f) : 0.92f * hardKnee (v / 0.62f);

        // Three bands: low and high are boosted, the middle is scooped hard.
        const float lo  = bzLow.lp (v);
        const float hi  = v - bzHigh.lp (v);
        const float mid = v - lo - hi;
        return 0.55f * bzOut.lp (bzLowGain * lo + 0.22f * mid + bzHighGain * hi);
    }

    float factory (float x)
    {
        const float in    = inHp.hp (x);
        const float fbIn  = fbLp.lp (fbHp.hp (yPrev));       // band-limited feedback
        const float noise = ffNoise * (random.nextFloat() - 0.5f);

        // Gate (bias) pushes the signal below the threshold, so only the tops
        // of the waveform get through, and they vanish as the note decays.
        const float u = in * ffDrive - ffBias + ffFeedback * fbIn + noise;

        // Asymmetric: hard on the positive side, soft and small on the negative.
        const float y = (u >= 0.0f) ? hardKnee (0.8f * u)
                                    : 0.35f * fastTanh (0.5f * u);
        yPrev = y;

        return 0.8f * outTone.lp (dcBlock.hp (y));
    }

    juce::dsp::Oversampling<float> oversampling  { 1, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };   // 4x
    juce::dsp::Oversampling<float> oversampling1 { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };   // 2x

    OnePole inHp, stage1Lp, stageHp, stage2Lp, toneLow, toneHigh;   // Wall
    OnePole emphHp, opAmpLp, ratFilter;                              // Tight
    OnePole fbHp, fbLp, dcBlock, outTone;                            // Gated
    OnePole bzLow, bzHigh, bzOut;                                    // Buzzsaw
    OnePole subLpA, subLpB, subOutLp;                                // the octave-down divider (Sub)
    float   subEnv = 0.0f, subState = 1.0f, subCut = 450.0f, subF = 300.0f;  int subCount = 0;  bool subArmed = true;
    OnePole nLoad, nHp1, nLp1, nMidA, nMidB, octLp, hissLp;          // v56: Face, Bender, Factory and the options
    OnePole dcBlock2;
    OnePole ffS1, ffS2;  float ffK1 = 0.0f, ffK2 = 0.0f, ffMake = 1.0f;   // b20: Factory style, the Stab-dependent top-end lift and output level (fitted to the captures)
    juce::Random random;

    float baseRate = 44100.0f, osRate = 176400.0f, lastLevel = -1.0f, yPrev = 0.0f;
    static constexpr int kDryMask = 255;      // delay line for the clean (dry) path of the Mix knob
    std::array<float, kDryMask + 1> dryRing {};
    std::vector<float> dryBuf;
    int   dryPos = 0;
    float lastMix = 1.0f;
    float stageGain1 = 4.0f, stageGain2 = 3.0f, ratGain = 10.0f;
    float ffDrive = 10.0f, ffBias = 0.0f, ffNoise = 0.0f, ffFeedback = 0.0f;
    float bzGain = 100.0f, bzLowGain = 1.0f, bzHighGain = 1.0f;
    float inGain = 1.0f, g1 = 10.0f, g2 = 2.0f, g3 = 0.0f, vmax = 1.0f, bias = 0.2f, biasOff = 0.0f, dead = 0.0f;
    float makeup = 1.0f, outScale = 0.7f, midAmt1 = 1.0f, env = 0.0f, envC = 0.01f, compAmt = 4.0f, muffOut = 1.6f;
    float drift1 = 0.0f, drift2 = 1.7f, driftGain = 1.0f, driftBias = 0.0f;
};

//==============================================================================
// A biquad notch filter (cuts one narrow band, leaves the rest alone).
//==============================================================================
// Compressor, placed before the amp. It has: a set
// of ratio curves (1:1 up to 20:1, a 10:1 "Soft" curve that behaves like a
// light-controlled compressor, and "Limit", a brickwall limiter), attack from
// about 50 microseconds to 50 ms, release up to a few seconds, a high-pass
// filter in the detector, and two harmonic distortion modes: Tube-like (
// 2nd harmonic) and Tape-like (3rd harmonic). This is my own simple
// version of those ideas, not a circuit model:
//   - Feed-forward compressor: a short level detector, a gain computer with a
//     knee, and attack/release smoothing of the gain reduction.
//   - Opto: soft knee, and the release gets slower the harder it is compressing.
//   - Detector HP: keeps low notes from pumping the whole signal.
//   - Dist 2 / Dist 3: saturation after the gain control. It gets stronger as
//     gain reduction increases, like the real unit's.
//   - Mix: parallel compression (blend with the dry signal).
//   - Bloom, Smack, Clarity and Transformer (v37): a deeper, slower primary squeeze; a second, fixed
//     parallel compressor with lifted lows; a tilt EQ; and low-end-only transformer saturation.
//==============================================================================
class CompressorEffect
{
public:
    struct Params
    {
        float thresholdDb;
        int   ratioIndex;      // 0 1:1, 1 2:1, 2 3:1, 3 4:1, 4 6:1, 5 10:1, 6 20:1, 7 Limit (b32: "10:1 Soft" was removed; the Opto style covers it)
        float attackMs, releaseMs;
        float makeupDb;
        float mix;
        int   detectorHp;      // 0 off, 1 80 Hz, 2 160 Hz, 3 320 Hz
        int   saturation;      // 0 off, 1 Dist 2, 2 Dist 3, 3 both, 4 transformer, 5 FET stage, 6 opto tube stage, 7 mu iron + tube, 8 diode
        float bloom   = 0.0f;  // 0 to 1: deeper, slower, "blooming" compression
        float smack   = 0.0f;  // 0 to 1: level of the second, parallel compressor
        float clarity = 0.0f;  // -1 to 1: tilt EQ (negative = warmer, positive = brighter)
        int   style   = 0;     // 0 Standard (the original), 1 FET, 2 Opto, 3 Variable-mu, 4 Sustainer, 5 Bus
    };

    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        squared = grDb = detectorState = dcState = 0.0f;
        squared2 = grDb2 = parLow = tiltLow = xformLow = 0.0f;
        memDb = grOptoA = grOptoB = 0.0f;
        shownScale = 1.0f;
    }

    float getGainReductionDb() const { return grDb * shownScale; }

    // Processes one channel in place.
    void process (float* x, int numSamples, const Params& p)
    {
        static const float ratios[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 10.0f, 20.0f, 100.0f };
        static const float hpHz[4]   = { 0.0f, 80.0f, 160.0f, 320.0f };
        const float twoPi = juce::MathConstants<float>::twoPi;

        const int   ratioIndex = juce::jlimit (0, 7, p.ratioIndex);
        const bool  thump = (p.style == 7);                       // b31: Thump = the Standard compressor plus Bloom, Smack and Clarity (they do nothing in the other styles)
        const int   style = thump ? 0 : juce::jlimit (0, 5, p.style);
        const bool  opto  = false;
        const bool  push  = (ratioIndex == 7);
        float slope = 1.0f - 1.0f / ratios[ratioIndex];                // dB of gain reduction per dB over the threshold
        float knee  = push ? 0.0f : (opto ? 12.0f : 6.0f);
        // Styles 1 to 5 reshape the detector, knee, ratio and the attack and release behaviour. Style 0 is the original.
        float attackScale = 1.0f, attackFloorMs = 0.02f, levelMs = 1.5f;
        // FET: fast, hard. A published attack range for this kind of unit is 20 to 800 microseconds and the release 50 ms to 1.1 s, so the
        // detector is a fast peak follower (50 us up, 2 ms down; the 0.6 ms averaging window it replaces was slower than the shortest attack),
        // and the attack floor is 5 us, below the shortest published value, so the Attack knob (0.05 to 50 ms, times 0.25) spans 12.5 us to 12.5 ms.
        if (style == 1)      { knee = push ? 0.0f : 2.0f;  attackScale = 0.25f; attackFloorMs = 0.005f; levelMs = 0.6f; }
        // opto: soft, slow to grab. A published average attack for this kind of unit is 10 ms, so the default Attack knob (5 ms) times 2 gives it;
        // the floor of 1.5 ms and the 50 ms top end (100 ms) put the real value well inside the range. The 1966 manual calls the attack "essentially
        // instantaneous" and a vendor page says 10 ms, so only the vendor figure is used as the centre.
        else if (style == 2) { knee = push ? 0.0f : 14.0f; attackScale = 2.0f;  attackFloorMs = 1.5f; levelMs = 3.0f; }
        else if (style == 3) { knee = push ? 0.0f : 18.0f; attackScale = 3.0f;  attackFloorMs = 3.0f; levelMs = 4.0f;
                               slope = 1.0f - 1.0f / (1.0f + (ratios[ratioIndex] - 1.0f) * 0.7f); }                             // variable-mu: gentle, deep curve
        else if (style == 4) { knee = push ? 0.0f : 8.0f;  attackFloorMs = 4.0f; levelMs = 2.0f;
                               slope = 1.0f - 1.0f / (1.0f + (ratios[ratioIndex] - 1.0f) * 1.5f); }                             // sustainer: pick gets through, long hold
        else if (style == 5) { knee = push ? 0.0f : 10.0f; attackFloorMs = 10.0f; levelMs = 3.0f;
                               slope = std::min (slope, 1.0f - 1.0f / 4.0f); }                                                  // bus: 4:1 at most, transients pass

        // Bloom: the primary compressor works harder (up to 2.5x the gain reduction) and lets go up to
        // 3x slower, so notes swell back up after the squeeze.
        const float bloom = thump ? juce::jlimit (0.0f, 1.0f, p.bloom) : 0.0f;
        const float grScale = 1.0f + 1.5f * bloom;
        shownScale = grScale;

        const float attackCoef  = std::exp (-1.0f / (std::max (attackFloorMs, p.attackMs * attackScale) * 0.001f * sampleRate));
        const float baseRelease = std::max (1.0f, p.releaseMs) * (1.0f + 2.0f * bloom);
        const float fixedReleaseCoef = std::exp (-1.0f / (baseRelease * 0.001f * sampleRate));
        const float levelCoef   = 1.0f - std::exp (-1.0f / ((style == 0 ? 0.0015f : levelMs * 0.001f) * sampleRate));   // 1.5 ms level detector (style 0)
        const float memCoef     = 1.0f - std::exp (-1.0f / (1.2f * sampleRate));          // slow memory of how long it has been squeezing
        const float optoFastCoef = std::exp (-1.0f / (0.020f * (baseRelease * (1.0f / 250.0f)) * sampleRate));   // first stage: 95 % of its half in 60 ms at the default
        const float fetUpCoef   = 1.0f - std::exp (-1.0f / (0.00005f * sampleRate));       // FET peak follower: 50 us up, 2 ms down
        const float fetDownCoef = 1.0f - std::exp (-1.0f / (0.002f * sampleRate));

        const float detHz   = hpHz[juce::jlimit (0, 3, p.detectorHp)];
        const float detCoef = detHz > 0.0f ? 1.0f - std::exp (-twoPi * detHz / sampleRate) : 0.0f;

        const float makeup = juce::Decibels::decibelsToGain (p.makeupDb);
        const bool  dist2  = (p.saturation == 1 || p.saturation == 3 || p.saturation == 6 || p.saturation == 7);
        const bool  dist3  = (p.saturation == 2 || p.saturation == 3 || p.saturation == 5);
        const bool  fetSat = (p.saturation == 5), diode = (p.saturation == 8);
        const float k2Scale = (p.saturation == 6) ? 0.5f : 1.0f;          // the opto tube stage is softer
        const float k3Scale = fetSat ? 1.4f : 1.0f;                       // the FET stage leans on odd harmonics
        const float bias   = 0.3f;
        const float biasTanh = fastTanh (bias);
        const float biasSlope = 1.0f - biasTanh * biasTanh;
        const float dcCoef = 1.0f - std::exp (-twoPi * 15.0f / sampleRate);
        const float mix    = juce::jlimit (0.0f, 1.0f, p.mix);
        const bool  xform  = (p.saturation == 4 || p.saturation == 7);

        // Second, parallel compressor ("Smack"): fixed settings, transparent and fast to grab, with the
        // lows lifted so the low end hits harder. Its level is the Smack knob.
        const float smack = thump ? juce::jlimit (0.0f, 1.0f, p.smack) : 0.0f;
        const bool  smackOn = smack > 0.001f;
        const float atk2 = std::exp (-1.0f / (0.008f * sampleRate));       // 8 ms: the first moment of a note gets through
        const float rel2 = std::exp (-1.0f / (0.110f * sampleRate));
        const float lev2 = 1.0f - std::exp (-1.0f / (0.003f * sampleRate));
        const float lowC = 1.0f - std::exp (-twoPi * 140.0f / sampleRate);
        const float xfC  = 1.0f - std::exp (-twoPi * 160.0f / sampleRate);

        // Clarity: a tilt EQ around 900 Hz. Positive lifts the highs and trims the lows by the same amount.
        const float clarity = thump ? juce::jlimit (-1.0f, 1.0f, p.clarity) : 0.0f;
        const bool  tiltOn = std::abs (clarity) > 0.005f;
        const float tiltC = 1.0f - std::exp (-twoPi * 900.0f / sampleRate);
        const float gLow  = juce::Decibels::decibelsToGain (-4.5f * clarity);
        const float gHigh = juce::Decibels::decibelsToGain ( 4.5f * clarity);

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = x[i];

            // Level detector (with the optional high-pass)
            float det = in;
            if (detCoef > 0.0f)
            {
                detectorState += detCoef * (in - detectorState);
                det = in - detectorState;
            }
            const float d2 = det * det;
            squared += (style == 1 ? (d2 > squared ? fetUpCoef : fetDownCoef) : levelCoef) * (d2 - squared);
            // (the peak follower reads a sine 3 dB higher than the averaging window did; take it off so the threshold means the same as before)
            const float levelDb = 3.0103f * fastLog2 (squared + 1.0e-9f) - (style == 1 ? 3.0103f : 0.0f);

            // Gain computer: how many dB of gain reduction this level calls for
            const float over = levelDb - p.thresholdDb;
            float target = 0.0f;
            if (knee > 0.0f && 2.0f * over < -knee)
                target = 0.0f;
            else if (knee > 0.0f && 2.0f * std::abs (over) <= knee)
                target = slope * (over + 0.5f * knee) * (over + 0.5f * knee) / (2.0f * knee);
            else if (over > 0.0f)
                target = slope * over;

            // Attack and release (in dB). Opto: release slows down as compression gets deeper.
            float releaseCoef = fixedReleaseCoef;
            if (style != 0)
            {
                memDb += memCoef * (grDb - memDb);
                const float deep = juce::jlimit (0.0f, 1.0f, grDb / 10.0f), held = juce::jlimit (0.0f, 1.0f, memDb / 8.0f);
                float f = 1.0f;
                if (style == 1)      f = 0.5f + 0.9f * held;                         // FET: program dependent: fast after a short peak, about 3 times slower after a long squeeze (the manual describes a fast release after transients and a slow one after sustained high-RMS signals)
                else if (style == 2) f = 1.0f;                                       // opto: has its own two-stage release below
                else if (style == 3) f = 1.0f + 1.5f * deep;                         // variable-mu: slow, smooth
                else if (style == 4) f = 1.5f + 1.0f * held;                         // sustainer: long hold, notes bloom
                else                 f = 0.6f + 0.9f * held;                         // bus: program-dependent glue
                releaseCoef = std::exp (-1.0f / (baseRelease * f * 0.001f * sampleRate));    // (fastExp2 is not accurate enough this close to 1)
            }
            else if (opto)
            {
                const float releaseMs = baseRelease * (0.5f + 2.5f * juce::jlimit (0.0f, 1.0f, grDb / 10.0f));
                releaseCoef = fastExp2 (-1.442695f / (releaseMs * 0.001f * sampleRate));
            }
            if (style == 2)
            {
                // Opto release in two stages, as published: about 60 ms for the first 50 %, then a slow tail whose length depends on how much and how
                // long it was compressing (0.5 to 5 s for complete release in the 1966 manual, 1 to 15 s in a vendor page; the first is used and the
                // Release knob, which scales both stages around its default of 250 ms, reaches the second). Each stage carries half of the gain reduction.
                const float held2 = juce::jlimit (0.0f, 1.0f, memDb / 8.0f);
                const float tgt = 0.5f * target;
                const float x = 1.0f / ((0.11f + held2) * (baseRelease * (1.0f / 250.0f)) * sampleRate);      // 99 % in 0.5 s (short squeeze) to 5 s (long, deep squeeze) at 250 ms
                const float slowCoef = 1.0f - x + 0.5f * x * x;                                              // exp (-x), x is tiny
                grOptoA = tgt + (tgt > grOptoA ? attackCoef : optoFastCoef) * (grOptoA - tgt);
                grOptoB = tgt + (tgt > grOptoB ? attackCoef : slowCoef)     * (grOptoB - tgt);
                grDb = grOptoA + grOptoB;
            }
            else
            {
                const float coef = (target > grDb) ? attackCoef : releaseCoef;
                grDb = target + coef * (grDb - target);
            }

            float y = in * fastExp2 (-grDb * grScale * 0.166096f);    // 10^(-gr/20)

            // Distortion, stronger with more gain reduction
            if (dist2 || dist3 || diode)
            {
                const float depth = juce::jlimit (0.0f, 1.5f, 0.2f + grDb / 15.0f);
                const float pre   = 3.0f;       // guitar signals are small; this sets where the curve starts to bend
                float z = y * pre;

                if (dist2)    // asymmetric: adds even harmonics (tube-like)
                {
                    const float k = 1.0f + 2.0f * depth * k2Scale;
                    z = (fastTanh (k * z + bias) - biasTanh) / (k * biasSlope);
                }
                if (dist3)    // symmetric: adds odd harmonics (tape-like)
                {
                    const float k = 1.0f + 3.0f * depth * k3Scale;
                    z = fastTanh (k * z) / k;
                }
                if (diode)    // pedal-style diode: a softer knee that rounds the peaks symmetrically
                {
                    const float k = 1.0f + 2.5f * depth;
                    z = z / (1.0f + std::abs (k * z)) * (1.0f + k * 0.3f);
                }
                y = z / pre;

                if (dist2)    // remove the DC the asymmetry creates
                {
                    dcState += dcCoef * (y - dcState);
                    y -= dcState;
                }
            }

            // Transformer: soft-saturates the low end only (the iron saturates first on bass), so
            // the lows thicken and round off while the highs stay clean. Stronger with more gain reduction.
            if (xform)
            {
                const float depth = juce::jlimit (0.0f, 1.5f, 0.2f + grDb * grScale / 15.0f);
                const float k = 1.0f + 3.0f * depth, pre = 3.0f;
                xformLow += xfC * (y - xformLow);
                y = (y - xformLow) + fastTanh (k * pre * xformLow) / (k * pre);
            }

            // Parallel compressor, added to the primary one
            if (smackOn)
            {
                squared2 += lev2 * (in * in - squared2);
                const float over2 = 3.0103f * fastLog2 (squared2 + 1.0e-9f) + 32.0f;     // threshold -32 dB
                const float target2 = over2 > 0.0f ? over2 * (1.0f - 1.0f / 8.0f) : 0.0f;  // 8:1
                grDb2 = target2 + ((target2 > grDb2) ? atk2 : rel2) * (grDb2 - target2);
                float par = in * fastExp2 (-grDb2 * 0.166096f);
                parLow += lowC * (par - parLow);
                par += 0.6f * parLow;                                                      // low-end impact
                y += 1.4f * smack * par;
            }

            float out = in * (1.0f - mix) + y * makeup * mix;

            if (tiltOn)
            {
                tiltLow += tiltC * (out - tiltLow);
                out = tiltLow * gLow + (out - tiltLow) * gHigh;
            }
            x[i] = out;
        }
    }

private:
    float sampleRate = 44100.0f;
    float squared = 0.0f, grDb = 0.0f, detectorState = 0.0f, dcState = 0.0f;
    float squared2 = 0.0f, grDb2 = 0.0f, parLow = 0.0f, tiltLow = 0.0f, xformLow = 0.0f, shownScale = 1.0f, memDb = 0.0f;
    float grOptoA = 0.0f, grOptoB = 0.0f;        // the opto style's two release stages (dB of gain reduction each)
};

//==============================================================================
struct NotchFilter
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 1.0f, a1 = 0.0f, a2 = 0.0f, z1 = 0.0f, z2 = 0.0f;

    void setNotch (float freq, float q, float sampleRate)
    {
        const float w0    = juce::MathConstants<float>::twoPi * freq / sampleRate;
        const float alpha = std::sin (w0) / (2.0f * q);
        const float cosw  = std::cos (w0);
        const float a0    = 1.0f + alpha;
        b0 = 1.0f / a0;
        b1 = -2.0f * cosw / a0;
        b2 = 1.0f / a0;
        a1 = -2.0f * cosw / a0;
        a2 = (1.0f - alpha) / a0;
    }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void reset() { z1 = z2 = 0.0f; }
};

//==============================================================================
// Pickup de-noise. It sits in front of the noise gate and does two jobs:
//   Hum reduction  : narrow notches at the mains frequency (50 or 60 Hz) and its
//                    first four harmonics, the buzz that single-coil pickups pick up.
//   Hiss reduction : a low-pass that closes when you aren't playing (between notes
//                    and in decays) and opens quickly when you do. It follows the
//                    gate threshold: closed at the threshold, fully open 24 dB above.
// Because the gate comes after this, the gate listens to the cleaned signal, so
// hum no longer holds it open.
//==============================================================================
class PickupDeNoise
{
public:
    void prepare (double newSampleRate)
    {
        sampleRate = (float) newSampleRate;
        for (auto& n : notches) n.reset();
        lastHumHz = 0.0f;
        env = openness = lp1 = lp2 = 0.0f;
    }

    // Processes one channel in place.
    void process (float* x, int numSamples, bool mains50, float humAmount,
                  float hissAmount, float gateThresholdDb)
    {
        const float humHz = mains50 ? 50.0f : 60.0f;
        if (humHz != lastHumHz)
        {
            for (int k = 0; k < numNotches; ++k)
                notches[(size_t) k].setNotch (humHz * (float) (k + 1), 25.0f, sampleRate);
            lastHumHz = humHz;
        }

        const float twoPi      = juce::MathConstants<float>::twoPi;
        const float lowLevel   = juce::Decibels::decibelsToGain (gateThresholdDb);
        const float span       = 24.0f / 8.686f;                       // 24 dB, in nepers
        const float envRelease = std::exp (-1.0f / (0.060f * sampleRate));
        const float openCoef   = 1.0f - std::exp (-1.0f / (0.005f * sampleRate));
        const float fMax       = std::min (16000.0f, 0.45f * sampleRate);
        const float fQuiet     = fMax * std::pow (1500.0f / fMax, hissAmount);  // cutoff when quiet

        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = x[i];

            // Hum: run through all the notches, then blend by the Amount.
            float notched = dry;
            for (auto& n : notches)
                notched = n.process (notched);
            const float cleaned = dry + humAmount * (notched - dry);

            // Level detector on the cleaned signal (instant attack, slow release).
            const float a = std::abs (cleaned);
            env = (a > env) ? a : a + envRelease * (env - a);

            const float target = (env > lowLevel) ? juce::jlimit (0.0f, 1.0f, std::log (env / lowLevel) / span)
                                                  : 0.0f;
            openness += openCoef * (target - openness);

            // Hiss: a two-pole low-pass whose cutoff follows how open it is.
            const float fc   = fMax * std::pow (fQuiet / fMax, 1.0f - openness);
            const float coef = 1.0f - std::exp (-twoPi * fc / sampleRate);
            lp1 += coef * (cleaned - lp1);
            lp2 += coef * (lp1 - lp2);

            x[i] = (hissAmount > 0.001f) ? lp2 : cleaned;
            if (hissAmount <= 0.001f)
                lp1 = lp2 = cleaned;        // keep the filter in step while it's bypassed
        }
    }

private:
    static constexpr int numNotches = 5;     // the hum frequency + 4 harmonics
    std::array<NotchFilter, (size_t) numNotches> notches;
    float sampleRate = 44100.0f, lastHumHz = 0.0f;
    float env = 0.0f, openness = 0.0f, lp1 = 0.0f, lp2 = 0.0f;
};

//==============================================================================
// StarterAmp v2
//
//   input gain -> PICKUP DE-NOISE -> noise gate -> high-pass -> NOISE PEDAL -> FUZZ PEDAL -> PRE-AMP DELAY
//   -> TRANSPARENT OVERDRIVE (clean path + gain path, summed, then clipped)
//   -> [everything below runs at 4x oversampling]
//        overdrive clip -> overdrive tone -> overdrive volume
//        -> preamp: 2 or 3 cascaded stages (high-pass, gain, tanh),
//           voiced by the Amp Model setting, plus a little power-amp sag
//        -> presence (treble shelf on the power-amp stage)
//   -> bass / low mid / high mid / treble
//   -> CAB: two impulse responses (A and B) blended by Cab Mix
//      (a basic low-pass filter stands in if no IR is loaded)
//   -> POST-AMP DELAY -> reverb (with optional shimmer) -> output gain
//==============================================================================
// Bucket-brigade style chorus.
// This kind of chorus is two bucket-brigade delay lines, swept by a slow
// triangle LFO, with the two lines moving in opposite directions. It has no knobs,
// only buttons: I (slow), II (a little faster) and I+II (fast and shallow, a vibrato-ish
// shimmer). The rates (0.513 Hz, 0.863 Hz and 9.75 Hz) and the delay ranges (1.66 to 5.35 ms for I and II, 3.3 to 3.7 ms for I+II)
// are the figures a published analysis measured from recordings of a unit (the service notes list 0.5, 0.83 and 1 Hz, and the
// analysis suspects the 1 Hz is a typo for about 10 Hz). I have not measured a unit myself.
// It sits before the amp, so it is mono here (the two lines are summed); the extra
// knobs (rate scale, depth, tone, hiss) are mine.
//==============================================================================
class BbdChorus
{
public:
    struct Params { int mode; float mix, rateScale, depth, toneHz, hiss; };

    void prepare (double sr)
    {
        sampleRate = sr;
        size = 1;
        while (size < (int) (0.03 * sr) + 8)
            size <<= 1;
        buf.assign ((size_t) size, 0.0f);
        reset();
    }

    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        writePos = 0;
        phase = 0.0f;
        phase2 = 0.0f;
        rateSm = 0.5f;
        swingSm = 0.0f;
        for (auto& f : lp) f.reset();
        lastTone = -1.0f;
        noiseState = 22222u;
    }

    void process (float* x, int numSamples, const Params& p)
    {
        static const float rates[3]   = { 0.513f, 0.863f, 9.75f };
        static const float swingMs[3] = { 1.845f, 1.845f, 0.2f };   // how far each line sweeps either side of the centre (I and II share one range; I+II is nearly static)
        const int mode = std::min (6, std::max (0, p.mode));
        if (mode >= 3) { processMulti (x, numSamples, p, mode); return; }
        const int m = mode;
        const float targetRate  = rates[m] * p.rateScale;
        const float targetSwing = swingMs[m] * p.depth;
        const float sr = (float) sampleRate;
        const float centreMs = 3.505f;

        const float tone = std::min (p.toneHz, 0.45f * sr);
        if (std::abs (tone - lastTone) > 0.5f)
        {
            for (auto& f : lp)
                *f.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, tone);
            lastTone = tone;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            rateSm  += 0.001f * (targetRate - rateSm);
            swingSm += 0.001f * (targetSwing - swingSm);

            phase += rateSm / sr;
            if (phase >= 1.0f) phase -= 1.0f;
            const float tri = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;   // -1 to 1

            const float dry = x[i];
            buf[(size_t) writePos] = dry;

            const float a = readDelayed ((centreMs + swingSm * tri) * 0.001f * sr);
            const float b = readDelayed ((centreMs - swingSm * tri) * 0.001f * sr);

            // The BBD's dark top end, and (optionally) its hiss
            // (v52: the two lines used to get the same noise with opposite signs, which cancelled when they were summed,
            //  so the Hiss knob did nothing. Each line now has its own noise, filtered by the Tone control like the signal.)
            noiseState = noiseState * 1664525u + 1013904223u;
            const float noiseA = ((float) ((noiseState >> 8) & 0xFFFFu) / 32768.0f - 1.0f) * 0.004f * p.hiss;
            noiseState = noiseState * 1664525u + 1013904223u;
            const float noiseB = ((float) ((noiseState >> 8) & 0xFFFFu) / 32768.0f - 1.0f) * 0.004f * p.hiss;
            const float wa = lp[0].processSample (a + noiseA);
            const float wb = lp[1].processSample (b + noiseB);

            x[i] = dry * (1.0f - p.mix) + p.mix * 0.5f * (wa + wb);
            writePos = (writePos + 1) & (size - 1);
        }
    }

private:
    // The extra chorus voicings (modes 3 to 6). Each is built from the same delay line:
    //   3 Dimension: two lines moving in opposite directions, very slow and shallow, a soft widening rather than a sweep
    //   4 Tri-chorus: three lines a third of a cycle apart, a smooth, wide, three-voice swirl
    //   5 Ensemble: three lines with a slow sweep and a faint fast shimmer on top, a string-machine style
    //   6 Detune: two longer lines drifting very slowly in opposite directions, a few cents sharp and flat
    void processMulti (float* x, int numSamples, const Params& p, int mode)
    {
        const float sr = (float) sampleRate;
        const float tone = std::min (p.toneHz, 0.45f * sr);
        if (std::abs (tone - lastTone) > 0.5f)
        {
            for (auto& f : lp)
                *f.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass (sampleRate, tone);
            lastTone = tone;
        }
        //                          Dimension  Tri   Ensemble  Detune
        static const float rate[4]   = { 0.31f, 0.55f, 0.60f, 0.18f };
        static const float swing[4]  = { 0.35f, 1.8f,  1.6f,  2.0f };
        static const float centre[4] = { 4.5f,  5.5f,  6.5f, 11.0f };
        const int k = mode - 3;
        const int voices = (mode == 3 || mode == 6) ? 2 : 3;
        const float targetRate = rate[k] * p.rateScale, targetSwing = swing[k] * p.depth;
        const float twoPi = juce::MathConstants<float>::twoPi;
        for (int i = 0; i < numSamples; ++i)
        {
            rateSm  += 0.001f * (targetRate - rateSm);
            swingSm += 0.001f * (targetSwing - swingSm);
            phase += rateSm / sr;       if (phase >= 1.0f)  phase -= 1.0f;
            phase2 += 6.3f * p.rateScale / sr;  if (phase2 >= 1.0f) phase2 -= 1.0f;

            const float dry = x[i];
            buf[(size_t) writePos] = dry;
            float wet = 0.0f;
            for (int v = 0; v < voices; ++v)
            {
                float lfo;
                if (voices == 2) lfo = (v == 0 ? 1.0f : -1.0f) * std::sin (twoPi * phase);
                else             lfo = std::sin (twoPi * (phase + (float) v / 3.0f));
                float d = centre[k] + swingSm * lfo;
                if (mode == 5) d += 0.22f * p.depth * std::sin (twoPi * (phase2 + (float) v / 3.0f));    // the fast shimmer
                noiseState = noiseState * 1664525u + 1013904223u;
                const float nz = ((float) ((noiseState >> 8) & 0xFFFFu) / 32768.0f - 1.0f) * 0.004f * p.hiss;
                wet += lp[v].processSample (readDelayed (d * 0.001f * sr) + nz);
            }
            wet *= 1.0f / (float) voices;
            x[i] = dry * (1.0f - p.mix) + p.mix * wet;
            writePos = (writePos + 1) & (size - 1);
        }
    }

    float readDelayed (float delaySamples) const
    {
        const float pos = (float) writePos - delaySamples;
        const float fl  = std::floor (pos);
        const float fr  = pos - fl;
        const int i0 = ((int) fl) & (size - 1), i1 = (i0 + 1) & (size - 1);
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }

    double sampleRate = 44100.0;
    std::vector<float> buf;
    int size = 1, writePos = 0;
    float phase = 0.0f, phase2 = 0.0f, rateSm = 0.5f, swingSm = 0.0f, lastTone = -1.0f;
    juce::dsp::IIR::Filter<float> lp[3];
    uint32_t noiseState = 22222u;
};

//==============================================================================
// Phaser with more options than the classic one.
// A classic phaser is four all-pass stages, swept together by one LFO (one Speed knob), mixed
// 50/50 with the dry sound. Here the same idea has more to adjust: 2 to 12 stages, rate,
// depth, centre frequency, feedback (positive or negative), LFO shape, dry/wet mix,
// and an envelope follower so your playing can push the sweep.
//==============================================================================
// Wah pedal: a resonant band-pass filter whose centre frequency sweeps between a "heel" (low)
// and a "toe" (high) setting. Manual = you set the Position; Envelope = how hard you play moves
// it (auto-wah); LFO = it sweeps by itself. The filter is a standard state-variable filter.
// Generic voice (circuit = 0): the sweep range and Q are my own approximation of a classic wah.
// Classic circuit voice (circuit = 1): the position sets the loop gain T0 of the pedal's resonant feedback loop,
// as in the published analyses (inductor 0.5 H and 10 nF give f0 = 2.25 kHz): the peak sits at f0 / sqrt (1 + T0),
// its Q rises as Q0 * sqrt (1 + T0) (so it is narrow at the heel and wide at the toe, as measured), and its height falls
// from about +18 dB at the heel to about +1 dB at the toe. The band-pass skirts are 6 dB per octave, as in the measured fit.
class Wah
{
public:
    struct Params { int mode; float pos, heelHz, toeHz, q, mix, sens, decayMs, rate, depth; int envDir = 0, envShape = 0; int circuit = 0; };   // v0.2: envDir 0 = up, 1 = down; envShape 0 = band-pass, 1 = low-pass, 2 = high-pass (Envelope Filter mode); circuit 1 = Classic circuit voice (heelHz / toeHz / q are then the knobs, scaled around the published values)

    void prepare (double sr) { sampleRate = sr; reset(); }
    void reset() { ic1 = ic2 = 0.0f; env = 0.0f; lfoPhase = 0.0f; posSmooth = 0.5f; first = true; }

    void process (float* x, int numSamples, const Params& p)
    {
        const float sr = (float) sampleRate;
        const float envAtk = 1.0f - std::exp (-1.0f / ((p.mode == 3 ? 0.002f : 0.003f) * sr));
        const float envRel = 1.0f - std::exp (-1.0f / (std::max (5.0f, p.decayMs) * 0.001f * sr));
        const float smooth = 1.0f - std::exp (-1.0f / (0.010f * sr));
        float k            = 1.0f / std::max (0.5f, p.q);
        const float ratio  = std::max (1.5f, p.toeHz) / std::max (50.0f, p.heelHz);
        float gainBp       = 1.6f;                       // generic voice: the peak is always 1.6x (+4 dB)

        // Classic circuit voice (not the Q-Tron-style Envelope Filter mode). Constants, all from the published analyses:
        //   f0 = 1 / (2 pi sqrt (0.5 H * 10 nF)) = 2.25 kHz; the toe keeps a little loop gain (T0 = 0.98) so that the toe is 1.6 kHz and
        //   the heel (T0 = 24) is 450 Hz; Q0 * f0 = 4000 (the mean of three measured Q * f points: 9.4 * 464, 4.0 * 838, 1.9 * 2252);
        //   the peak is +18 dB at the heel and scales with T0 / sqrt (1 + T0) (about +1 dB at the toe); the pot is an audio taper (x^1.8).
        // The Heel and Toe knobs scale the published 450 Hz and 1.6 kHz by (knob / default), Resonance scales the real Q by (knob / 5),
        // so the defaults give the real pedal and the knob ranges reach well past it on both ends.
        const bool  circuitOn = p.circuit != 0 && p.mode != 3;
        constexpr float kT0Toe = 0.98f, kQ0 = 1.777f, kTaper = 1.8f, kPeakHeel = 7.943f;
        const float heelEff = 450.0f * std::max (50.0f, p.heelHz) / 400.0f;
        const float toeEff  = 1600.0f * std::max (50.0f, p.toeHz) / 2200.0f;
        const float f0c     = toeEff * 1.40712f;         // sqrt (1 + kT0Toe)
        const float rHeel   = f0c / heelEff;
        const float t0Heel  = std::max (kT0Toe + 0.05f, rHeel * rHeel - 1.0f);
        const float pkHeel  = t0Heel / std::sqrt (1.0f + t0Heel);
        const float qMul    = std::max (0.05f, p.q) * 0.2f;

        // The exp / sin / pow / tan below only need to be refreshed every few samples (the sweep is
        // slow and smoothed), so they run once per 8 samples instead of every sample.
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, shaped = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            const float in = x[i];
            const bool  control = (i & 7) == 0;
            float target = p.pos;

            if (p.mode == 1 || p.mode == 3)   // envelope follower (3 = the Q-Tron-style Envelope Filter: it starts at one end of the range and the playing level pushes it toward the other)
            {
                const float a = std::abs (in);
                env += (a > env ? envAtk : envRel) * (a - env);
                if (control) shaped = 1.0f - std::exp (-env * p.sens * 20.0f);
                target = p.mode == 3 ? (p.envDir == 0 ? shaped : 1.0f - shaped) : p.pos + (1.0f - p.pos) * shaped;
            }
            else if (p.mode == 2)       // LFO
            {
                lfoPhase += p.rate / sr;
                if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
                if (control) shaped = std::sin (juce::MathConstants<float>::twoPi * lfoPhase);
                target = p.pos + p.depth * 0.5f * shaped;
            }
            target = std::min (1.0f, std::max (0.0f, target));

            if (first) { posSmooth = target; first = false; }
            posSmooth += smooth * (target - posSmooth);

            if (control)
            {
                float f;
                if (circuitOn)
                {
                    const float t  = std::pow (std::max (0.0f, 1.0f - posSmooth), kTaper);    // 0 at the toe, 1 at the heel
                    const float t0 = kT0Toe + (t0Heel - kT0Toe) * t;                          // loop gain of the resonant feedback
                    const float sq = std::sqrt (1.0f + t0);
                    f      = std::min (0.45f * sr, f0c / sq);
                    k      = 1.0f / std::max (0.3f, kQ0 * sq * qMul);
                    gainBp = kPeakHeel * (t0 / sq) / pkHeel;
                }
                else
                    f = std::min (0.45f * sr, p.heelHz * std::pow (ratio, posSmooth));
                const float g = std::tan (juce::MathConstants<float>::pi * f / sr);
                a1 = 1.0f / (1.0f + g * (g + k));
                a2 = g * a1;
                a3 = g * a2;
            }
            const float v3 = in - ic2;
            const float v1 = a1 * ic1 + a2 * v3;
            const float v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;

            float wet = gainBp * k * v1;   // band-pass, 1.0 at its peak times gainBp (generic: 1.6, a little make-up; classic circuit: +18 dB at the heel down to about +1 dB at the toe)
            if (p.mode == 3 && p.envShape == 1)      wet = 0.6f * v2;                // low-pass (a resonant peak is loud, so a little less)
            else if (p.mode == 3 && p.envShape == 2) wet = 0.6f * (in - k * v1 - v2); // high-pass
            x[i] = in * (1.0f - p.mix) + wet * p.mix;
        }
    }

private:
    double sampleRate = 44100.0;
    float ic1 = 0.0f, ic2 = 0.0f, env = 0.0f, lfoPhase = 0.0f, posSmooth = 0.5f;
    bool first = true;
};

// ---------------------------------------------------------------------------------------------
// Texture effects: tremolo, wobble (tremolo-arm style pitch vibrato), swell (auto volume swell),
// and a granular cloud. They sit after the Post EQ and before the post-amp delay and reverb.
// All are simple hand-written designs; see README for what each does.
// ---------------------------------------------------------------------------------------------
class Tremolo
{
public:
    struct Params { int shape; bool stereo; float rateHz, depth; };

    void prepare (double sr)
    {
        sampleRate = sr; phase = 0.0f;
        lampA = lampB = cellA = cellB = 0.5f;
        lampUp = 1.0f - std::exp (-1.0f / (0.012f * (float) sr));       // filament warms in about 12 ms
        lampDown = 1.0f - std::exp (-1.0f / (0.030f * (float) sr));     // and cools in about 30 ms
        cellUp = 1.0f - std::exp (-1.0f / (0.004f * (float) sr));       // photocell lights quickly
        cellDown = 1.0f - std::exp (-1.0f / (0.045f * (float) sr));     // and recovers slowly
    }

    // v0.1 b7: just the pulse, as a dip from 0 (full level) to 1 (deepest), one value per sample. Used by the
    // in-amp tremolo, which applies it between the preamp and the power stage. Mono; the depth is applied there.
    void lfoDip (float* out, int n, int shape, float rateHz)
    {
        const float inc = rateHz / (float) sampleRate;
        for (int i = 0; i < n; ++i)
        {
            float a;
            if (shape == 3)
            {
                const float ph = phase - std::floor (phase);
                const float dA = 0.5f * (1.0f + std::sin (juce::MathConstants<float>::twoPi * ph));
                a = opticalStep (lampA, cellA, dA);
            }
            else
                a = Tremolo::shape (phase, shape);
            out[i] = 0.5f * (1.0f - a);
            phase += inc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float inc = p.rateHz / (float) sampleRate;
        for (int i = 0; i < n; ++i)
        {
            float a, b;
            if (p.shape == 3)
            {
                // Optical (bulb and photocell): the LFO drives a lamp, the lamp lights a photocell. The lamp is slow to
                // warm and slower to cool, the cell is quick to light and slow to recover, and its resistance falls
                // off non-linearly, so the pulse is rounded, a little lopsided and shallower at fast rates.
                const float ph = phase - std::floor (phase);
                const float dA = 0.5f * (1.0f + std::sin (juce::MathConstants<float>::twoPi * ph));
                const float dB = 0.5f * (1.0f + std::sin (juce::MathConstants<float>::twoPi * (ph + 0.5f)));
                a = opticalStep (lampA, cellA, dA);
                b = p.stereo ? opticalStep (lampB, cellB, dB) : a;
            }
            else
            {
                a = shape (phase, p.shape);
                b = p.stereo ? shape (phase + 0.5f, p.shape) : a;
            }
            const float gl = 1.0f - p.depth * 0.5f * (1.0f - a);                 // a in -1..1: 1 = full level
            const float gr = 1.0f - p.depth * 0.5f * (1.0f - b);
            l[i] *= gl;
            if (r != nullptr) r[i] *= gr;
            phase += inc;
            if (phase >= 1.0f) phase -= 1.0f;
        }
    }

private:
    // Returns -1..1 like the other shapes (1 = full level)
    float opticalStep (float& lamp, float& cell, float drive)
    {
        lamp += (drive > lamp ? lampUp : lampDown) * (drive - lamp);
        const float light = std::pow (std::max (0.0f, lamp), 1.4f);
        cell += (light > cell ? cellUp : cellDown) * (light - cell);
        return 1.0f - 2.0f * std::pow (cell, 0.8f);
    }
    float lampA = 0.5f, lampB = 0.5f, cellA = 0.5f, cellB = 0.5f, lampUp = 0.01f, lampDown = 0.01f, cellUp = 0.01f, cellDown = 0.01f;

    static float shape (float ph, int type)
    {
        ph -= std::floor (ph);
        const float s = std::sin (juce::MathConstants<float>::twoPi * ph);
        if (type == 1) return ph < 0.5f ? 4.0f * ph - 1.0f : 3.0f - 4.0f * ph;
        if (type == 2) return fastTanh (5.0f * s) / fastTanh (5.0f);
        if (type == 4)
        {
            // Bias (amp-style): the dips are narrow and the full-level part is wide, as when a tremolo wobbles the power
            // tubes' bias: they only get squeezed at the bottom of the swing. A shape I chose, not a measured circuit.
            const float dip = std::pow (0.5f * (1.0f - s), 1.7f);
            return 1.0f - 2.0f * dip;
        }
        return s;
    }
    double sampleRate = 44100.0;
    float phase = 0.0f;
};

// v0.1 b7: the amp-style tremolo. 'dip' is the pulse (0 = full level, 1 = deepest) and 'envIn' the level of what
// you play (about 0.25 = a hard pick), both per sample at the normal rate; they are applied to the oversampled signal
// between the preamp and the power stage. Pick Response makes the pulse shallower as you play harder (a loud signal
// squeezes the bias swing), so soft notes throb more than hard ones. The level is measured at the input because the
// signal after the preamp is squashed to about the same level whatever you play.
inline void applyAmpTrem (float* y, int m, const float* dip, const float* envIn, int nBase, float depth, float pick)
{
    if (nBase <= 0 || m <= 0) return;
    const float ratio = (float) nBase / (float) m;
    for (int i = 0; i < m; ++i)
    {
        const float pos = ((float) i + 0.5f) * ratio - 0.5f;
        const int i0 = juce::jlimit (0, nBase - 1, (int) std::floor (pos));
        const int i1 = std::min (nBase - 1, i0 + 1);
        const float fr = juce::jlimit (0.0f, 1.0f, pos - (float) i0);
        const float d = dip[i0] + fr * (dip[i1] - dip[i0]);
        const float e = envIn[i0] + fr * (envIn[i1] - envIn[i0]);
        y[i] *= 1.0f - (depth / (1.0f + pick * e * 2.0f)) * d;
    }
}

class Wobble
{
public:
    struct Params { float rateHz, depth, drift, mix; };

    void prepare (double sr)
    {
        sampleRate = sr;
        const int len = (int) (0.030 * sr) + 8;
        for (auto& b : buf) b.assign ((size_t) len, 0.0f);
        pos = 0; phase = 0.0f; driftState = 0.0f; rng = 12345u;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float sr = (float) sampleRate;
        const int   len = (int) buf[0].size();
        float* ch[2] = { l, r };
        const int channels = r != nullptr ? 2 : 1;
        const float centre = 0.0065f * sr;                       // 6.5 ms centre delay
        const float depthSamples = p.depth * 0.0045f * sr;       // up to +-4.5 ms
        const float inc = p.rateHz / sr;
        const float driftCoef = 1.0f - std::exp (-1.0f / (0.35f * sr));

        for (int i = 0; i < n; ++i)
        {
            // slow random drift, like an unsteady arm
            if ((rng = rng * 1664525u + 1013904223u) % 4096u == 0u)
                driftTarget = ((float) (rng >> 8 & 0xffff) / 32768.0f) - 1.0f;
            driftState += driftCoef * (driftTarget - driftState);

            const float lfo = std::sin (juce::MathConstants<float>::twoPi * phase);
            phase += inc; if (phase >= 1.0f) phase -= 1.0f;
            const float mod = lfo * (1.0f - 0.5f * p.drift) + p.drift * driftState * 1.5f;

            for (int c = 0; c < channels; ++c)
            {
                buf[c][(size_t) pos] = ch[c][i];
                const float chOffset = c == 1 ? -0.15f * mod : 0.0f;     // slightly different on the right
                float d = centre + depthSamples * (mod + chOffset);
                d = std::min ((float) (len - 4), std::max (1.0f, d));
                float rp = (float) pos - d;
                if (rp < 0.0f) rp += (float) len;
                const int i0 = (int) rp, i1 = (i0 + 1) % len;
                const float fr = rp - (float) i0;
                const float wet = buf[c][(size_t) i0] * (1.0f - fr) + buf[c][(size_t) i1] * fr;
                ch[c][i] = ch[c][i] * (1.0f - p.mix) + wet * p.mix;
            }
            pos = (pos + 1) % len;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> buf[2];
    int pos = 0;
    float phase = 0.0f, driftState = 0.0f, driftTarget = 0.0f;
    unsigned int rng = 12345u;
};

// Auto volume swell ("slow gear"): finds the start of each note and fades it in.
class Swell
{
public:
    void prepare (double sr) { sampleRate = sr; reset(); }
    void reset() { fast = slow = 0.0f; gain = 1.0f; sinceOnset = 100000; }

    // Look at the (mono) input and fill 'gains' with a fade-in curve for each sample.
    void analyse (const float* x, int n, float attackMs, float sens, std::vector<float>& gains)
    {
        gains.resize ((size_t) n);
        const float sr = (float) sampleRate;
        const float fa = 1.0f - std::exp (-1.0f / (0.001f * sr)), fr = 1.0f - std::exp (-1.0f / (0.030f * sr));
        const float sa = 1.0f - std::exp (-1.0f / (0.120f * sr)), srl = 1.0f - std::exp (-1.0f / (0.400f * sr));
        const float step = 1.0f / std::max (1.0f, attackMs * 0.001f * sr);
        const float thresh = 0.002f + 0.03f * (1.0f - sens);
        const int   refractory = (int) (0.12f * sr);

        for (int i = 0; i < n; ++i)
        {
            const float a = std::abs (x[i]);
            fast += (a > fast ? fa : fr) * (a - fast);
            slow += (a > slow ? sa : srl) * (a - slow);
            ++sinceOnset;
            if (fast > 1.8f * slow + thresh && sinceOnset > refractory)
            {
                gain = 0.0f;                  // a new note: start the fade-in
                sinceOnset = 0;
            }
            gain = std::min (1.0f, gain + step);
            gains[(size_t) i] = gain * gain * (3.0f - 2.0f * gain);   // smooth S-curve
        }
    }

private:
    double sampleRate = 44100.0;
    float fast = 0.0f, slow = 0.0f, gain = 1.0f;
    int sinceOnset = 100000;
};

// Granular cloud: records the sound into a 4-second loop and plays back lots of tiny, overlapping,
// windowed pieces ("grains"), each with its own position, pitch, direction and pan.
class Granular
{
public:
    // scale: 0 free, 1 octaves, 2 fifths, 3 major pentatonic, 4 minor pentatonic, 5 major, 6 minor, 7 whole tone (random pitches land on its notes)
    // shape: 0 smooth, 1 swell (slow rise, quick fall), 2 pluck (quick rise, slow fall), 3 flat (short fades)
    struct Params { float sizeMs, density, posMs, spray, pitch, pitchRand, reverse, width, feedback, mix; bool freeze;
                    int scale = 0, shape = 0; float scan = 0.0f, toneHz = 20000.0f; };

    void prepare (double sr)
    {
        sampleRate = sr;
        bufLen = (int) (4.0 * sr);
        for (auto& b : buf) b.assign ((size_t) bufLen, 0.0f);
        writePos = 0; spawnAcc = 0.0f; rng = 987654321u;
        for (auto& g : grains) g.active = false;
        fbL = fbR = 0.0f;
    }

    void reset()      // clears the loop without allocating (safe on the audio thread)
    {
        for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f);
        writePos = 0; spawnAcc = 0.0f;
        for (auto& g : grains) g.active = false;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        const float sr = (float) sampleRate;
        const float sizeS = std::max (64.0f, p.sizeMs * 0.001f * sr);
        const float overlap = p.density * sizeS / sr;
        const float amp = 1.0f / std::sqrt (std::max (1.0f, 0.5f * overlap));
        const float mixDry = std::cos (p.mix * juce::MathConstants<float>::halfPi);
        const float mixWet = std::sin (p.mix * juce::MathConstants<float>::halfPi);

        for (int i = 0; i < n; ++i)
        {
            const float inL = l[i], inR = r != nullptr ? r[i] : l[i];

            // start new grains
            spawnAcc += p.density * (0.7f + 0.6f * rand01()) / sr;
            while (spawnAcc >= 1.0f)
            {
                spawnAcc -= 1.0f;
                spawn (p, sizeS, amp, sr);
            }

            // mix the active grains
            float outL = 0.0f, outR = 0.0f;
            for (auto& g : grains)
            {
                if (! g.active) continue;
                float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) g.age / (float) g.length);
                if (p.shape != 0 && p.shape != 4)
                {
                    const float xw = (float) g.age / (float) g.length;
                    if (p.shape == 1)      w = 6.75f * xw * xw * (1.0f - xw);
                    else if (p.shape == 2) w = std::min (1.0f, 12.0f * xw) * (1.0f - xw) * (1.0f - xw);
                    else                   w = std::min (1.0f, 5.0f * std::min (xw, 1.0f - xw));
                }
                const int i0 = (int) g.pos, i1 = (i0 + 1) % bufLen;
                const float fr = g.pos - (float) i0;
                const float sL = buf[0][(size_t) i0] * (1.0f - fr) + buf[0][(size_t) i1] * fr;
                const float sR = buf[1][(size_t) i0] * (1.0f - fr) + buf[1][(size_t) i1] * fr;
                outL += sL * w * g.gainL;
                outR += sR * w * g.gainR;

                g.pos += g.inc;
                if (g.pos >= (float) bufLen) g.pos -= (float) bufLen;
                if (g.pos < 0.0f) g.pos += (float) bufLen;
                if (++g.age >= g.length) g.active = false;
            }

            if (p.toneHz < 19500.0f)                               // darken the cloud (and with it each generation of feedback)
            {
                const float tk = 1.0f - std::exp (-6.2831853f * p.toneHz / sr);
                toneL += tk * (outL - toneL);  toneR += tk * (outR - toneR);
                outL = toneL;  outR = toneR;
            }
            scanPhase += 0.13f / sr;
            if (scanPhase >= 1.0f) scanPhase -= 1.0f;

            // record (unless frozen); the cloud can feed back into the recording
            if (! p.freeze)
            {
                buf[0][(size_t) writePos] = inL + p.feedback * fastTanh (outL);
                buf[1][(size_t) writePos] = inR + p.feedback * fastTanh (outR);
                writePos = (writePos + 1) % bufLen;
            }

            l[i] = inL * mixDry + outL * mixWet;
            if (r != nullptr) r[i] = inR * mixDry + outR * mixWet;
        }
    }

private:
    struct Grain { bool active = false; float pos = 0.0f, inc = 1.0f, gainL = 0.0f, gainR = 0.0f; int age = 0, length = 1; };

    float rand01() { rng = rng * 1664525u + 1013904223u; return (float) (rng >> 8) / 16777216.0f; }

    // A random offset within +/- range semitones that lands on a note of the chosen scale
    float pickScaleOffset (int scale, int range)
    {
        static const int sets[8][8] = { { 0 }, { 0 }, { 0, 7 }, { 0, 2, 4, 7, 9 }, { 0, 3, 5, 7, 10 },
                                        { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 4, 6, 8, 10 } };
        static const int counts[8] = { 1, 1, 2, 5, 5, 7, 7, 6 };
        const int sc = std::max (1, std::min (7, scale));
        int candidates[64], n = 0;
        for (int o = -range; o <= range && n < 64; ++o)
        {
            const int m = ((o % 12) + 12) % 12;
            for (int k = 0; k < counts[sc]; ++k)
                if (sets[sc][k] == m) { candidates[n++] = o; break; }
        }
        if (n == 0) return 0.0f;
        return (float) candidates[std::min (n - 1, (int) (rand01() * (float) n))];
    }

    void spawn (const Params& p, float sizeS, float amp, float sr)
    {
        Grain* slot = nullptr;
        for (auto& g : grains) if (! g.active) { slot = &g; break; }
        if (slot == nullptr) return;

        const bool smear = p.shape == 4, glitch = p.shape == 5;
        const float len = sizeS * (0.8f + 0.4f * rand01()) * (smear ? 2.5f : glitch ? 0.45f : 1.0f);
        float semis = p.pitch + p.pitchRand * (rand01() * 2.0f - 1.0f);
        if (smear) semis += 0.12f * (rand01() * 2.0f - 1.0f);            // a few cents of drift per grain: the cloud smears and shimmers
        if (glitch && rand01() < 0.2f) semis += rand01() < 0.5f ? -12.0f : 12.0f;
        if (p.scale != 0 && p.pitchRand >= 0.5f)
            semis = p.pitch + pickScaleOffset (p.scale, (int) std::floor (p.pitchRand));
        float inc = std::pow (2.0f, semis / 12.0f);
        const bool reversed = rand01() < (glitch ? std::max (p.reverse, 0.2f) : p.reverse);
        if (reversed) inc = -inc;

        // How far back to start. The grain's read point must not run into the write point
        // (or off the oldest end of the loop), so the earliest/latest start depends on pitch and direction.
        const float writeRate = p.freeze ? 0.0f : 1.0f;
        const float gapChange = len * (writeRate - inc);          // change in distance behind the writer over the grain
        const float dMin = 64.0f + std::max(0.0f, -gapChange);
        const float dMax = (float) bufLen - 64.0f - std::max (0.0f, gapChange);
        if (dMin >= (float) bufLen - 64.0f) return;

        float d = p.posMs * 0.001f * sr + p.spray * rand01() * 1.5f * sr + p.scan * 1.2f * sr * (0.5f + 0.5f * std::sin (6.2831853f * scanPhase));
        d = std::min (std::max (d, dMin), std::max (dMin, dMax));

        if (glitch)                                                  // stutter: often replay the same spot as the last grain
        {
            if (rand01() < 0.45f) d = lastD; else lastD = d;
        }
        float start = (float) writePos - d;
        while (start < 0.0f) start += (float) bufLen;

        const float pan = p.width * (rand01() * 2.0f - 1.0f);
        const float ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        slot->active = true;
        slot->pos = start;
        slot->inc = inc;
        slot->age = 0;
        slot->length = std::max (32, (int) len);
        slot->gainL = amp * std::cos (ang) * 1.4142f;
        slot->gainR = amp * std::sin (ang) * 1.4142f;
    }

    double sampleRate = 44100.0;
    int bufLen = 1, writePos = 0;
    std::vector<float> buf[2];
    Grain grains[48];
    float lastD = 4800.0f;
    float spawnAcc = 0.0f, fbL = 0.0f, fbR = 0.0f, toneL = 0.0f, toneR = 0.0f, scanPhase = 0.0f;
    unsigned int rng = 1u;
};

// ---------------------------------------------------------------------------------------------
// Doubler: imitates double-tracking. Two copies of the (mono) signal are delayed by slightly
// different, slowly and randomly changing amounts (like two takes that never line up exactly),
// darkened a little differently, and spread left and right.
// ---------------------------------------------------------------------------------------------
class Doubler
{
public:
    struct Params { float timeMs, variation, spread, mix; };

    void prepare (double sr)
    {
        sampleRate = sr;
        len = (int) (0.1 * sr) + 8;
        buf.assign ((size_t) len, 0.0f);
        reset();
    }
    void reset()
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        pos = 0; phase[0] = 0.0f; phase[1] = 0.37f;
        for (int v = 0; v < 2; ++v) { walk[v] = target[v] = 0.0f; lp[v] = 0.0f; }
        rng = 4242u;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        if (r == nullptr || len < 16) return;
        const float sr = (float) sampleRate;
        const float walkCoef = 1.0f - std::exp (-1.0f / (0.25f * sr));
        const float lpCoef[2] = { 1.0f - std::exp (-6.2831853f * 6500.0f / sr), 1.0f - std::exp (-6.2831853f * 9000.0f / sr) };
        const float rate[2] = { 0.23f / sr, 0.34f / sr };
        const float dryG = 1.0f - 0.3f * p.mix, wetG = 0.9f * p.mix;
        const float s = p.spread;

        for (int i = 0; i < n; ++i)
        {
            buf[(size_t) pos] = 0.5f * (l[i] + r[i]);

            float voice[2];
            for (int v = 0; v < 2; ++v)
            {
                if ((rng = rng * 1664525u + 1013904223u) % 2048u == 0u)
                    target[v] = ((float) ((rng >> 8) & 0xffff) / 32768.0f) - 1.0f;
                walk[v] += walkCoef * (target[v] - walk[v]);
                phase[v] += rate[v]; if (phase[v] >= 1.0f) phase[v] -= 1.0f;

                // the wander is up to 3 ms, scaled down for very short times so a 5 ms double does not collapse onto the dry sound
                const float depth = 0.003f * sr * std::min (1.0f, std::max (0.2f, p.timeMs / 20.0f));
                const float mod = (0.6f * walk[v] + 0.4f * std::sin (6.2831853f * phase[v])) * p.variation * depth;
                float d = p.timeMs * 0.001f * sr * (v == 0 ? 0.8f : 1.2f) + mod;
                d = std::min ((float) (len - 4), std::max (1.0f, d));
                float rp = (float) pos - d;
                if (rp < 0.0f) rp += (float) len;
                const int i0 = (int) rp, i1 = (i0 + 1) % len;
                const float fr = rp - (float) i0;
                const float x = buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
                lp[v] += lpCoef[v] * (x - lp[v]);
                voice[v] = lp[v];
            }

            const float wl = voice[0] * (0.5f + 0.5f * s) + voice[1] * (0.5f - 0.5f * s);
            const float wr = voice[1] * (0.5f + 0.5f * s) + voice[0] * (0.5f - 0.5f * s);
            l[i] = l[i] * dryG + wl * wetG;
            r[i] = r[i] * dryG + wr * wetG;
            pos = (pos + 1) % len;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> buf;
    int len = 0, pos = 0;
    float phase[2] {}, walk[2] {}, target[2] {}, lp[2] {};
    unsigned int rng = 4242u;
};

// ---------------------------------------------------------------------------------------------
// Widener: a stereo "Wide" enhancer plus a "Bass Lift" bass enhancer. Wide works on the difference between left and right (the side signal): the
// lows are taken out of it (so the bass stays in the middle), it is smeared slightly in time
// and phase, its lows-mids and highs are lifted, and it is made louder. Bass Lift makes bass
// audible on small speakers by adding harmonics of the low notes. It needs a stereo signal to work
// with (from the doubler, a stereo cab, the reverb...), since a pure mono signal has no side part.
// ---------------------------------------------------------------------------------------------
class Widener
{
public:
    struct Params { int mode; float wow, space, bass, bassHz, monoHz, mix, levelGain; };

    void prepare (double sr)
    {
        sampleRate = sr;
        ringLen = (int) (0.006 * sr) + 8;
        ring.assign ((size_t) ringLen, 0.0f);
        reset();
    }
    void reset()
    {
        std::fill (ring.begin(), ring.end(), 0.0f);
        ringPos = 0;
        sLow1 = sLow2 = lp3k = lp600 = bassLp1 = bassLp2 = hpState = hpPrev = apX1 = apY1 = 0.0f;
    }

    void process (float* l, float* r, int n, const Params& p)
    {
        if (r == nullptr || ringLen < 16) return;
        const float sr = (float) sampleRate;
        auto coef = [sr] (float hz) { return 1.0f - std::exp (-6.2831853f * std::min (hz, 0.45f * sr) / sr); };

        // My own guesses at three flavours (the real unit has desktop and rackmount variants)
        struct Mode { float delayMs, apHz, hiBoost, loBoost; };
        static const Mode modes[3] = { { 0.30f, 1800.0f, 0.8f, 0.5f }, { 0.45f, 1200.0f, 1.0f, 0.8f }, { 0.20f, 2600.0f, 0.6f, 0.3f } };
        const Mode& m = modes[std::min(2, std::max(0, p.mode))];

        const float monoA = coef (std::max (20.0f, p.monoHz)), a3k = coef (3000.0f), a600 = coef (600.0f);
        const float bassA = coef (p.bassHz * 1.5f), hpA = coef (p.bassHz * 1.4f);
        const float tn = std::tan (3.14159265f * std::min (m.apHz, 0.45f * sr) / sr);
        const float apA = (tn - 1.0f) / (tn + 1.0f);
        const int   delaySamples = std::max (1, (int) (m.delayMs * 0.001f * sr * (0.3f + 0.7f * p.space)));
        const bool  monoBass = p.monoHz > 20.5f;

        for (int i = 0; i < n; ++i)
        {
            const float L = l[i], R = r[i];
            const float mid = 0.5f * (L + R), side = 0.5f * (L - R);

            // Side signal: remove its lows (keeps the bass centred)
            float s1 = side;
            if (monoBass)
            {
                sLow1 += monoA * (side - sLow1);
                sLow2 += monoA * (sLow1 - sLow2);
                s1 = side - sLow2;
            }

            // Smear it in time and phase (a short delay plus an all-pass), more with Space
            ring[(size_t) ringPos] = s1;
            const float sd = ring[(size_t) ((ringPos - delaySamples + ringLen) % ringLen)];
            const float ap = apA * sd + apX1 - apA * apY1;
            apX1 = sd; apY1 = ap;
            const float s2 = s1 * (1.0f - 0.5f * p.space) + ap * (0.5f * p.space);
            ringPos = (ringPos + 1) % ringLen;

            // Lift the side's lows-mids and highs, then make it louder
            lp3k  += a3k  * (s2 - lp3k);
            lp600 += a600 * (s2 - lp600);
            const float high = s2 - lp3k;
            const float sFinal = (s2 + p.wow * (m.hiBoost * high + m.loBoost * lp600)) * (1.0f + 2.0f * p.wow);
            const float mFinal = mid * (1.0f - 0.2f * p.wow);

            // Bass Lift: harmonics of the low notes (full-wave rectified bass, high-passed) plus a little fundamental
            bassLp1 += bassA * (mid - bassLp1);
            bassLp2 += bassA * (bassLp1 - bassLp2);
            const float rect = std::abs (bassLp2);
            hpState += hpA * (rect - hpState);
            const float harmonics = rect - hpState;
            const float bassAdd = p.bass * (3.0f * harmonics + 0.35f * bassLp2);

            const float wetL = mFinal + sFinal + bassAdd;
            const float wetR = mFinal - sFinal + bassAdd;
            l[i] = (L * (1.0f - p.mix) + wetL * p.mix) * p.levelGain;
            r[i] = (R * (1.0f - p.mix) + wetR * p.mix) * p.levelGain;
        }
    }

private:
    double sampleRate = 44100.0;
    std::vector<float> ring;
    int ringLen = 0, ringPos = 0;
    float sLow1 = 0.0f, sLow2 = 0.0f, lp3k = 0.0f, lp600 = 0.0f, bassLp1 = 0.0f, bassLp2 = 0.0f, hpState = 0.0f, hpPrev = 0.0f, apX1 = 0.0f, apY1 = 0.0f;
};

// The stages follow the published circuit of the classic four-stage phaser pedal: each is an op-amp all-pass (47 nF, a 2N5952 JFET in parallel with 24k, 10k / 10k),
// output = 2 * high-pass (input) - input, so a stage swings from -1 at low frequencies to +1 at high ones. Because the JFET's conductance
// is 2 * beta * (Vgs - Vp), the corner frequency follows the LFO voltage LINEARLY (not exponentially): with the published JFET and LFO
// swing it runs from about 330 Hz to 3 kHz (first notch 0.41 * fc, second 2.41 * fc). Depth sets the sweep ends in octaves around Centre
// (2.5 octaves per unit of depth either side, so the default 0.7 gives about 11:1, the real pedal's 10:1), the path between them is linear in Hz.
// Feedback is the later pedals' 24k resistor ("R28"): the last stage's output goes into the INVERTING input of the 2nd stage. 'feedback' is its
// conductance 10k / R28 (0.417 = the real 24k; up to 0.9 = about 11k, negative inverts it). That stage then has gain 2 + feedback from its
// non-inverting side, which is the extra mid-range hump of the later pedals. The loop is solved exactly (no extra sample of delay).
class Phaser
{
public:
    struct Params { int stages, shape; float rate, depth, centreHz, feedback, mix, envAmount, lagMs = 0.0f; };

    void prepare (double sr) { sampleRate = sr; lagBuf.assign ((size_t) (0.012 * sr) + 8, 0.0f); lagPos = 0; reset(); }

    void reset()
    {
        for (auto& v : z) v = 0.0f;
        phase = 0.0f; fbState = 0.0f; env = 0.0f;
        rndCur = rndTarget = 0.0f;
        noiseState = 777u;
        std::fill (lagBuf.begin(), lagBuf.end(), 0.0f); lagPos = 0;
    }

    void process (float* x, int numSamples, const Params& p)
    {
        const float sr = (float) sampleRate;
        const float lagSamples = lagBuf.empty() ? 0.0f : std::min ((float) lagBuf.size() - 4.0f, std::max (0.0f, p.lagMs * 0.001f * sr));
        const int   n  = std::min (12, std::max (2, p.stages));
        const float c  = std::min (0.9f, std::max (-0.9f, p.feedback));    // 10k / R28 into the 2nd stage's inverting input (0 = the first pedals, 0.417 = the later 24k)
        const float h  = 0.5f * (2.0f + c);                                // half the 2nd stage's gain from its non-inverting side
        const float envAtk = 1.0f - std::exp (-1.0f / (0.005f * sr));
        const float envRel = 1.0f - std::exp (-1.0f / (0.120f * sr));
        const float twoPi = juce::MathConstants<float>::twoPi;
        // Sweep ends: Centre is the geometric middle, Depth the half-width in octaves (2.5 per unit; the real pedal is about +-1.66, depth 0.66)
        const float sweepOct = 2.5f * std::min (1.0f, std::max (0.0f, p.depth));
        const float fLo = std::min (0.45f * sr, std::max (20.0f, p.centreHz * std::exp2 (-sweepOct)));
        const float fHi = std::min (0.45f * sr, std::max (20.0f, p.centreHz * std::exp2 (sweepOct)));
        float a = 0.0f;
        float xin[12], ain[12];

        for (int i = 0; i < numSamples; ++i)
        {
            const float dry = x[i];

            const float lvl = std::abs (dry);
            env += ((lvl > env) ? envAtk : envRel) * (lvl - env);

            phase += p.rate / sr;
            if (phase >= 1.0f)
            {
                phase -= 1.0f;
                noiseState = noiseState * 1664525u + 1013904223u;
                rndTarget = (float) ((noiseState >> 8) & 0xFFFFu) / 32768.0f - 1.0f;
            }
            if (p.shape == 2)
                rndCur += 0.002f * (rndTarget - rndCur) * (p.rate > 0.0f ? 1.0f : 0.0f);

            // The sweep is slow, so the LFO shape and tan only need refreshing every 4 samples.
            if ((i & 3) == 0)
            {
                float lfo;
                if (p.shape == 0)      lfo = std::sin (twoPi * phase);
                else if (p.shape == 1) lfo = phase < 0.5f ? 4.0f * phase - 1.0f : 3.0f - 4.0f * phase;
                else                   lfo = rndCur;

                const float envPush = p.envAmount * (2.0f * std::min (1.0f, env * 8.0f) - 1.0f);
                const float sweep = std::min (1.0f, std::max (-1.0f, lfo + envPush));

                // All stages share one frequency (one control voltage moves them all). The JFET's conductance is linear in that voltage, so is the corner.
                const float fc = fLo + (fHi - fLo) * (0.5f + 0.5f * sweep);
                const float t  = std::tan (juce::MathConstants<float>::pi * fc / sr);
                a = (t - 1.0f) / (t + 1.0f);
            }

            // Lag: the feedback arrives a few milliseconds late, which smears and slows the resonant peaks (0 = the plain phaser)
            float fbIn = fbState;
            if (lagSamples > 0.01f)
            {
                const int sz = (int) lagBuf.size();
                float rp = (float) lagPos - lagSamples;
                if (rp < 0.0f) rp += (float) sz;
                const int i0 = (int) rp;  const float fr = rp - (float) i0;
                fbIn = lagBuf[(size_t) i0 % sz] * (1.0f - fr) + lagBuf[(size_t) (i0 + 1) % sz] * fr;
            }
            // Each stage is the inverting op-amp all-pass: y = -A (x), A = (a + z^-1) / (1 + a z^-1) (A = +1 at DC, -1 at high frequencies).
            // The 2nd stage also takes -c * (last output): y2 = (c/2) x - h * A (x) - c * yN, with h = 1 + c/2.
            // yN is affine in y2 (yN = P * y2 + O) through the stages after it, so the loop is solved in closed form.
            float yv = dry;
            xin[0] = yv;  ain[0] = a * yv + z[0];  yv = -ain[0];                    // 1st stage
            xin[1] = yv;  ain[1] = a * yv + z[1];                                  // 2nd stage (takes the feedback)
            const float base = 0.5f * c * yv - h * ain[1];
            float ym;
            if (lagSamples > 0.01f)
                ym = base - c * fbIn;                                              // Lag: the feedback arrives late, no loop to solve
            else
            {
                float P = 1.0f, O = 0.0f;
                for (int s = 2; s < n; ++s) { P *= -a; O = -a * O - z[s]; }
                ym = (base - c * O) / (1.0f + c * P);                              // |c * P| <= 0.9, so never singular
            }
            yv = ym;
            for (int s = 2; s < n; ++s) { xin[s] = yv; ain[s] = a * yv + z[s]; yv = -ain[s]; }
            for (int s = 0; s < n; ++s) z[s] = xin[s] - a * ain[s];
            fbState = yv;
            if (! lagBuf.empty()) { lagBuf[(size_t) lagPos] = fbState; if (++lagPos >= (int) lagBuf.size()) lagPos = 0; }

            x[i] = dry * (1.0f - p.mix) + p.mix * yv;
        }
    }

private:
    double sampleRate = 44100.0;
    float z[12] {};
    std::vector<float> lagBuf;
    int lagPos = 0;
    float phase = 0.0f, fbState = 0.0f, env = 0.0f, rndCur = 0.0f, rndTarget = 0.0f;
    uint32_t noiseState = 777u;
};

// ---------------------------------------------------------------------------------------------
// FdnReverb: a feedback-delay-network reverb with six "types". Eight delay lines feed back into
// each other through an orthogonal (Householder) matrix, so it cannot blow up; each line has its
// own damping filter and a gain set so the tail decays in the chosen time. Before the lines sit
// four all-pass filters that smear the input (diffusion). The type picks the delay lengths, decay
// time, brightness, diffusion and how much the lines slowly wobble (which removes metallic ringing).
// All of this is my own hand-tuned design, not a model of any particular unit; the types are
// "in the character of" Room, Hall, Plate, Ambient, Cathedral and Spring, nothing more.
// ---------------------------------------------------------------------------------------------
class FdnReverb
{
public:
    static constexpr int N = 8;
    static constexpr int kTypes = 16;

    void prepare (double sr)
    {
        sampleRate = (float) sr;
        const int maxLen = (int) (0.65 * sr) + 8;
        for (auto& d : line) d.assign ((size_t) maxLen, 0.0f);
        for (auto& a : ap) a.buf.assign ((size_t) (0.02 * sr) + 8, 0.0f);
        mask = maxLen;
        reset();
    }

    void reset()
    {
        for (auto& d : line) std::fill (d.begin(), d.end(), 0.0f);
        for (auto& a : ap) { std::fill (a.buf.begin(), a.buf.end(), 0.0f); a.pos = 0; }
        for (int i = 0; i < N; ++i) { lp[i] = 0.0f; }
        pos = 0; phase = 0.0f;
        outLp[0] = outLp[1] = 0.0f;
        lenInit = false;
    }

    // v51: extra shaping (all neutral by default, which gives exactly the sound of earlier versions)
    //   diffuse 0..1 (0.5 = as voiced): low = the echoes tumble over each other, high = a smooth wash
    //   space   x    : scales the size of the space (the delay lengths) on top of the decay time
    //   dragSemis    : the delay lengths glide to a new scale (a pitch glide while they move), in semitones (+ = shorter)
    //   mod     0..1 : more movement in the tail (depth and speed of the line modulation)
    struct Extra { float diffuse = 0.5f, space = 1.0f, dragSemis = 0.0f, mod = 0.0f; };

    // type 1..11 (0 is the classic reverb, handled elsewhere): 10 Chorale (vowel formants sweep through the tail), 11 Nonlinear (gated); size, damp 0..1
    // decaySeconds: the time for the tail to fall by 60 dB. Longer decays also use longer delay
    // lines (up to 2.5x), so the space gets bigger as well as longer.
    void setParams (int type, float decaySeconds, float damp, float width, bool freezeOn) { setParams (type, decaySeconds, damp, width, freezeOn, Extra()); }
    void setParams (int type, float decaySeconds, float damp, float width, bool freezeOn, const Extra& ex)
    {
        const auto& c = cfg (type);
        const float rt60 = std::max (0.05f, decaySeconds);
        float s = 0.6f + 1.9f * std::min (1.0f, rt60 / (4.0f * c.rt60));   // delay-length scale
        s *= ex.space;
        if (ex.dragSemis != 0.0f) s *= std::pow (2.0f, -ex.dragSemis / 12.0f);
        for (int i = 0; i < N; ++i)
        {
            const float ms = c.baseMs * kRatio[i] * s;
            const float target = std::min (ms * 0.001f * sampleRate, (float) mask - 64.0f);
            targetLen[i] = target;
            if (! lenInit) lenSamples[i] = target;
            else if (std::abs (target - lenSamples[i]) > 0.05f) lenMoving = true;
            const float g = std::pow (10.0f, -3.0f * (target / sampleRate) / std::max (0.05f, rt60));
            fbGain[i] = freezeOn ? 0.9995f : std::min (0.9995f, g);
        }
        lenInit = true;
        lenCoef = 1.0f - std::exp (-1.0f / (0.12f * sampleRate));
        const float hz = c.dampMaxHz * std::pow (c.dampMinHz / c.dampMaxHz, damp);
        dampCoef = freezeOn ? 1.0f : 1.0f - std::exp (-6.2831853f * hz / sampleRate);
        diff = ex.diffuse == 0.5f ? c.diff
             : ex.diffuse < 0.5f ? c.diff * ex.diffuse * 2.0f
             : c.diff + (0.85f - c.diff) * (ex.diffuse - 0.5f) * 2.0f;
        modDepth = c.modMs * 0.001f * sampleRate * (1.0f + 4.0f * ex.mod);
        modInc = 6.2831853f * c.modHz * (1.0f + ex.mod) / sampleRate;
        early = c.early;
        chorale = type == 10;
        gated = type == 11;
        gateHold = (int) (sampleRate * std::min (0.9f, 0.12f + 0.22f * rt60));
        static const float trims[kTypes] = { 0.55f, 0.65f, 0.45f, 0.60f, 0.75f, 0.60f, 0.50f, 0.65f, 0.60f, 0.42f, 0.55f, 0.42f, 0.38f, 0.60f, 0.60f, 0.55f };   // roughly equal loudness
        trim = trims[std::max (1, std::min (kTypes, type)) - 1];
        for (int i = 0; i < 4; ++i)
            apLen[i] = std::max (1, std::min ((int) (c.apMs[i] * 0.001f * sampleRate), (int) ap[i].buf.size() - 2));
        inGain = freezeOn ? 0.0f : 1.0f;
        wideness = width;
    }

    void process (float in, float& outL, float& outR)
    {
        if (lenMoving)                                   // the delay lengths glide to their new values
        {
            lenMoving = false;
            for (int i = 0; i < N; ++i)
            {
                const float d = targetLen[i] - lenSamples[i];
                if (std::abs (d) < 0.05f) lenSamples[i] = targetLen[i];
                else { lenSamples[i] += lenCoef * d; lenMoving = true; }
            }
        }
        // input diffusion
        float x = in * inGain;
        for (int i = 0; i < 4; ++i)
        {
            auto& a = ap[i];
            const int rp = (a.pos - apLen[i] + (int) a.buf.size()) % (int) a.buf.size();
            const float d = a.buf[(size_t) rp];
            const float v = x - diff * d;
            a.buf[(size_t) a.pos] = v;
            a.pos = (a.pos + 1) % (int) a.buf.size();
            x = d + diff * v;
        }

        phase += modInc;
        if (phase > 6.2831853f) phase -= 6.2831853f;
        const float s = std::sin (phase), c = std::cos (phase);

        float y[N], sum = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            const float m = modDepth * ((i & 1) ? s : c) * (0.6f + 0.1f * (float) i);
            float rd = (float) pos - (lenSamples[i] + m);
            while (rd < 0.0f) rd += (float) mask;
            const int i0 = (int) rd;
            const float fr = rd - (float) i0;
            const int i1 = (i0 + 1) % mask;
            const float v = line[i][(size_t) (i0 % mask)] * (1.0f - fr) + line[i][(size_t) i1] * fr;
            lp[i] += dampCoef * (v - lp[i]);
            y[i] = lp[i] * fbGain[i];
            sum += y[i];
        }

        const float k = 2.0f / (float) N;
        float l = 0.0f, r = 0.0f;
        for (int i = 0; i < N; ++i)
        {
            const float sign = (i & 2) ? -1.0f : 1.0f;
            const float fbv = y[i] - k * sum;                   // Householder mixing
            line[i][(size_t) pos] = fbv + x * (0.5f * sign);
            if (i & 1) r += sign * y[i]; else l += sign * y[i];
        }
        pos = (pos + 1) % mask;

        // a little of the diffused input straight out (early reflections), then width
        const float e = early * x;
        l = 0.55f * l + e; r = 0.55f * r + e;
        const float mid = 0.5f * (l + r), side = 0.5f * (l - r) * wideness;
        outL = (mid + side) * trim;
        outR = (mid - side) * trim;

        if (chorale)
        {
            // two band-pass formants that glide through a, o, u, i: the tail sings a slow vowel
            fPhase += 0.09f / sampleRate;  if (fPhase >= 1.0f) fPhase -= 1.0f;
            static const float f1[4] = { 730.0f, 570.0f, 300.0f, 270.0f }, f2[4] = { 1090.0f, 840.0f, 870.0f, 2290.0f };
            const float pos = fPhase * 4.0f;
            const int a = (int) pos & 3, b = (a + 1) & 3;
            float t = pos - std::floor (pos);  t = t * t * (3.0f - 2.0f * t);
            const float fc1 = f1[a] + t * (f1[b] - f1[a]), fc2 = f2[a] + t * (f2[b] - f2[a]);
            const float g1 = 2.0f * std::sin (3.14159265f * fc1 / sampleRate), g2 = 2.0f * std::sin (3.14159265f * fc2 / sampleRate);
            const float kq = 0.22f;
            float in2[2] = { outL, outR }, res[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = in2[ch], sum = 0.0f;
                for (int f = 0; f < 2; ++f)
                {
                    auto& st = fs[ch][f];
                    const float g = f == 0 ? g1 : g2;
                    st.lo += g * st.bd;
                    const float hi = x - st.lo - kq * st.bd;
                    st.bd += g * hi;
                    sum += kq * st.bd * (f == 0 ? 1.0f : 0.8f);
                }
                res[ch] = 0.4f * x + 2.4f * sum;
            }
            outL = res[0];  outR = res[1];
        }
        else if (gated)
        {
            // a gated tail: it holds at full level for a while after the last loud note, then drops away quickly
            if (std::abs (in) > 0.02f) gateTimer = gateHold;
            if (gateTimer > 0) { --gateTimer; gateGain += 0.01f * (1.0f - gateGain); }
            else gateGain *= 0.9990f;
            outL *= gateGain;  outR *= gateGain;
        }
    }

private:
    struct Cfg { float baseMs, rt60, dampMaxHz, dampMinHz, diff, modMs, modHz, early; float apMs[4]; };
    static const Cfg& cfg (int type)
    {
        static const Cfg table[kTypes] =
        {
            //   baseMs rt60 dampMax dampMin diff  modMs modHz early   all-pass lengths (ms)
            {  7.0f, 0.55f,  9000.0f, 2500.0f, 0.55f, 0.05f, 0.30f, 0.30f, { 1.7f, 2.9f, 4.3f, 6.1f } },   // 1 Room
            { 22.0f, 2.20f,  7500.0f, 1800.0f, 0.65f, 0.25f, 0.25f, 0.10f, { 3.1f, 5.3f, 7.9f, 11.3f } },  // 2 Hall
            { 12.0f, 1.60f, 14000.0f, 4000.0f, 0.75f, 0.12f, 0.45f, 0.00f, { 2.3f, 3.7f, 5.9f, 8.3f } },   // 3 Plate
            { 33.0f, 4.20f,  8500.0f, 2200.0f, 0.70f, 0.90f, 0.33f, 0.00f, { 4.1f, 6.7f, 9.7f, 13.9f } },  // 4 Ambient
            { 48.0f, 6.50f,  5500.0f, 1200.0f, 0.68f, 0.40f, 0.18f, 0.05f, { 5.3f, 8.9f, 12.1f, 17.3f } }, // 5 Cathedral
            {  9.0f, 1.80f,  4200.0f, 1500.0f, 0.78f, 0.06f, 1.10f, 0.15f, { 1.1f, 1.9f, 2.9f, 3.9f } },   // 6 Spring
            { 14.0f, 2.40f, 11000.0f, 3000.0f, 0.80f, 0.35f, 0.60f, 0.00f, { 2.0f, 3.3f, 5.1f, 7.7f } },   // 7 Soft Plate: dense and moving
            { 18.0f, 2.00f,  6500.0f, 1800.0f, 0.12f, 0.60f, 0.25f, 0.12f, { 3.7f, 6.1f, 8.9f, 12.7f } },  // 8 Tumble: echoes that tumble over each other
            { 55.0f, 9.00f,  6000.0f,  900.0f, 0.75f, 1.10f, 0.15f, 0.00f, { 6.1f, 9.3f, 13.7f, 19.0f } }, // 9 Abyss: huge, dark, drifting
            { 28.0f, 3.20f,  9000.0f, 3000.0f, 0.72f, 0.40f, 0.35f, 0.00f, { 3.7f, 6.3f, 9.1f, 13.3f } },  // 10 Chorale: a lush hall that sings vowels
            { 15.0f, 1.20f, 12000.0f, 4500.0f, 0.82f, 0.08f, 0.30f, 0.08f, { 2.1f, 3.5f, 5.3f, 7.7f } },   // 11 Nonlinear: dense and bright, then gated
            { 55.0f, 9.00f,  6000.0f,  900.0f, 0.75f, 1.10f, 0.15f, 0.00f, { 6.1f, 9.3f, 13.7f, 19.0f } }, // 12 (unused: the choice list has the IR Reverb here)
            { 62.0f, 14.0f,  9500.0f, 2200.0f, 0.84f, 1.40f, 0.12f, 0.00f, { 4.9f, 8.3f, 12.7f, 17.9f } }, // 13 Black Hole: enormous, ultra-dense, slowly swirling
            { 14.0f,  1.20f, 12000.0f, 4000.0f, 0.78f, 0.08f, 0.35f, 0.45f, { 2.3f, 3.7f, 5.9f, 8.3f } },  // 14 Reverse: a short dense plate that smooths the reversed grains
            { 26.0f, 2.80f,  8500.0f, 2500.0f, 0.72f, 0.45f, 0.30f, 0.00f, { 3.5f, 5.9f, 8.7f, 12.3f } },  // 15 ModEcho: a lush hall behind the echoes
            { 30.0f, 3.50f,  7000.0f, 1500.0f, 0.80f, 0.70f, 0.50f, 0.00f, { 4.3f, 7.1f, 10.3f, 14.9f } }, // 16 Mangled: dense and dark, the mangling happens around it
        };
        return table[std::max (1, std::min (kTypes, type)) - 1];
    }

    static constexpr float kRatio[N] = { 1.00f, 1.13f, 1.27f, 1.41f, 1.57f, 1.71f, 1.87f, 2.03f };

    struct Allpass { std::vector<float> buf; int pos = 0; };

    float sampleRate = 44100.0f;
    int mask = 1, pos = 0;
    std::vector<float> line[N];
    Allpass ap[4];
    int apLen[4] { 1, 1, 1, 1 };
    float lenSamples[N] {}, targetLen[N] {}, fbGain[N] {}, lp[N] {};
    float lenCoef = 0.001f;
    bool lenInit = false, lenMoving = false;
    float dampCoef = 0.5f, diff = 0.6f, modDepth = 0.0f, modInc = 0.0f, phase = 0.0f, early = 0.0f, inGain = 1.0f, wideness = 1.0f, trim = 0.5f;
    float outLp[2] {};
    bool chorale = false, gated = false;
    float fPhase = 0.0f, gateGain = 0.0f;
    int gateTimer = 0, gateHold = 1;
    struct Formant { float lo = 0.0f, bd = 0.0f; } fs[2][2];
};

//==============================================================================
// Wet ensemble (v51): a three-voice chorus for the reverb's output, the soft, shimmering movement of some plate-style
// reverb pedals. Each side has three slowly moving delays; Amount blends them in.
//==============================================================================
class WetEnsemble
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        size = 1;
        while (size < (int) (0.04 * sr) + 8) size <<= 1;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        reset();
    }
    void reset() { for (auto& b : buf) std::fill (b.begin(), b.end(), 0.0f); w = 0; for (auto& p : ph) p = 0.0f; }

    void process (float& l, float& r, float amount, float rateScale)
    {
        buf[0][(size_t) w] = l;  buf[1][(size_t) w] = r;
        static const float baseMs[3] = { 8.0f, 13.5f, 19.0f }, hz[3] = { 0.31f, 0.47f, 0.63f };
        float sumL = 0.0f, sumR = 0.0f;
        for (int v = 0; v < 3; ++v)
        {
            ph[v] += hz[v] * rateScale / sampleRate;
            if (ph[v] >= 1.0f) ph[v] -= 1.0f;
            const float mod = 2.2f * amount;                                              // ms of movement
            sumL += read (0, (baseMs[v] + mod * std::sin (6.2831853f * ph[v])) * 0.001f * sampleRate);
            sumR += read (1, (baseMs[v] + mod * std::sin (6.2831853f * (ph[v] + 0.33f))) * 0.001f * sampleRate);
        }
        w = (w + 1) & (size - 1);
        l = l * (1.0f - 0.45f * amount) + amount * 0.55f * (sumL / 3.0f) * 1.8f;
        r = r * (1.0f - 0.45f * amount) + amount * 0.55f * (sumR / 3.0f) * 1.8f;
    }

private:
    float read (int c, float d) const
    {
        float rp = (float) w - d;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp, i1 = (i0 + 1) & (size - 1);
        const float f = rp - (float) i0;
        return buf[c][(size_t) (i0 & (size - 1))] * (1.0f - f) + buf[c][(size_t) i1] * f;
    }
    std::vector<float> buf[2];
    float ph[3] { 0.0f, 0.33f, 0.66f }, sampleRate = 44100.0f;
    int size = 1, w = 0;
};

//==============================================================================
// Metronome: a click on every beat (and optionally every subdivision), with an accented first beat.
// It is added to the output after everything else. When the plugin follows the host tempo and the
// host is playing, the clicks follow the host's beat position; otherwise it runs freely from the
// moment it is switched on.
//==============================================================================
class Metronome
{
public:
    struct Params
    {
        bool on = false;
        double bpm = 120.0;
        int beats = 4, sub = 1, sound = 0;
        float gain = 0.2f;
        bool accent = true;
        double hostPpq = -1.0;      // >= 0 when the host is playing and reports a position
    };

    void prepare (double newSampleRate) { sampleRate = newSampleRate; reset(); }
    void reset() { freeTick = 0.0; wasOn = false; env = 0.0f; attack = 0; }

    void process (juce::AudioBuffer<float>& buffer, int numSamples, const Params& p)
    {
        if (! p.on)
        {
            wasOn = false;
            if (env < 1.0e-5f) return;                       // finished ringing
        }
        else if (! wasOn)
        {
            wasOn = true;
            freeTick = 0.0;                                  // start on a downbeat
        }

        const int sub = juce::jlimit (1, 4, p.sub);
        const int beats = juce::jlimit (1, 12, p.beats);
        const double ticksPerSample = (p.bpm / 60.0) * (double) sub / sampleRate;
        const bool hosted = p.on && p.hostPpq >= 0.0;
        const double startTick = hosted ? p.hostPpq * (double) sub : freeTick;
        const double endTick = startTick + ticksPerSample * (double) numSamples;

        int nextAt = -1, nextKind = 0;                       // the next click in this block
        long long t = p.on ? (long long) std::ceil (startTick - 1.0e-9) : 0;
        auto scheduleFrom = [&] (long long tick)
        {
            if (! p.on || (double) tick >= endTick) { nextAt = -1; return; }
            nextAt = juce::jlimit (0, numSamples - 1, (int) std::floor (((double) tick - startTick) / ticksPerSample));
            const long long barTicks = (long long) beats * sub;
            const long long m = ((tick % barTicks) + barTicks) % barTicks;
            nextKind = (m == 0 && p.accent && beats > 1) ? 2 : ((m % sub == 0) ? 1 : 0);   // 2 accent, 1 beat, 0 subdivision
        };
        scheduleFrom (t);

        float* l = buffer.getWritePointer (0);
        float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
        for (int i = 0; i < numSamples; ++i)
        {
            while (nextAt == i)
            {
                trigger (p.sound, nextKind);
                ++t;
                scheduleFrom (t);
                if (nextAt >= 0 && nextAt < i) nextAt = i;
            }
            if (env > 1.0e-5f)
            {
                float s = std::sin (phase) + (sound == 1 ? 0.5f * std::sin (2.4f * phase) : 0.0f);
                float a = env;
                if (attack > 0) { a *= 1.0f - (float) attack / (0.001f * (float) sampleRate); --attack; }
                s *= a * amp * p.gain;
                l[i] += s;
                if (r != nullptr) r[i] += s;
                phase += step;
                if (phase > juce::MathConstants<float>::twoPi) phase -= juce::MathConstants<float>::twoPi;
                env *= decay;
            }
        }
        if (! hosted) freeTick = endTick;
    }

private:
    void trigger (int soundType, int kind)
    {
        static const float freq[3][3] = { { 1200.0f, 1700.0f, 2400.0f },    // Click:  sub, beat, accent
                                          {  700.0f,  950.0f, 1400.0f },    // Wood
                                          {  660.0f,  880.0f, 1320.0f } };  // Beep
        static const float decayMs[3] = { 6.0f, 22.0f, 45.0f };
        sound = juce::jlimit (0, 2, soundType);
        step = juce::MathConstants<float>::twoPi * freq[sound][kind] / (float) sampleRate;
        decay = std::exp (-1.0f / (0.001f * decayMs[sound] * (float) sampleRate));
        amp = kind == 2 ? 1.0f : (kind == 1 ? 0.7f : 0.4f);
        env = 1.0f;
        phase = 0.0f;
        attack = (int) (0.001f * (float) sampleRate);
    }

    double sampleRate = 44100.0, freeTick = 0.0;
    bool wasOn = false;
    float env = 0.0f, phase = 0.0f, step = 0.1f, decay = 0.99f, amp = 1.0f;
    int attack = 0, sound = 0;
};

//==============================================================================
// One reverb unit (b12): everything the reverb block keeps between blocks. The processor has two (Reverb 1 and Reverb 2).
//==============================================================================
struct ReverbUnit
{
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> reverbPreDelay;
    juce::SmoothedValue<float> reverbPreSmooth;                      // in samples
    float revDrvHp = 0.0f, revDrvLp = 0.0f;                          // spring driver state
    juce::dsp::IIR::Filter<float> revHp[2], revLp[2];                // low cut and high cut on the wet signal
    float lastRevHp = -1.0f, lastRevLp = -1.0f;
    float duckEnv = 0.0f;
    float revWetSmooth = 0.0f;
    juce::Reverb shimmerReverb;           // the "Classic" reverb type
    FxExtra::SpringTank springTank;       // b32: the Spring (tank) type
    FxExtra::SpringReturn springReturn;   // b33: the amp-style driver and return around either Spring type
    FdnReverb fdnReverb;                  // the Room / Hall / Plate / Ambient / Cathedral / Spring ... types
    int  lastRevType = -1;
    bool revIdle = false;                 // true while the reverb is skipped (off, or mix at 0)
    GuitarShifter shimmerShifters[2];     // the shimmer's two pitch voices
    float shimmerLfo[2] { 0.0f, 0.37f };  // slow detune drift
    bool  shimmerWasOn = false;
    float shimmerLast = 0.0f, shimmerLpState = 0.0f, shimmerHpState = 0.0f;
    std::vector<float> shimmerBelowBuf[2];  int shimmerBelowW = 0;      // v97: delay for the octave-down voice(s)
    WetEnsemble wetEnsemble;
    GuitarShifter symphShifter;           // the reverb's "symphony" layer (an octave or so of the tail)
    bool symphWasOn = false;
    float bloomGain = 1.0f;
    float gravGain = 1.0f, gravT = 0.0f;  bool gravWasActive = false;     // Black Hole gravity
    // b12: Reverse type (overlapping reversed grains of the send), wet drive, focus filter and wet tremolo
    std::vector<float> revHist;  int revHistW = 0, revHistMask = 0;
    int revGrainStart[2] { 0, 0 }, revGrainAge[2] { 0, 0 }, revGrainLen[2] { 1, 1 }, revHop = 0, revNextGrain = 0;
    float crushHold = 0.0f;  int crushCount = 0;
    float svf1[2] { 0.0f, 0.0f }, svf2[2] { 0.0f, 0.0f };
    float tremPhase = 0.0f;
    // b13: ModEcho (modulated feedback echo into the reverb) and Mangled (distorted feedback around the reverb)
    std::vector<float> echoBuf;  int echoW = 0, echoMask = 0;
    float echoLp = 0.0f, echoLfo = 0.0f;
    float mangleFb = 0.0f, mangleHp = 0.0f, mangleLp = 0.0f, mangleHold = 0.0f;  int mangleCount = 0;
    void prepareExtra (double sr)
    {
        int n = 1;
        while ((double) n < 2.6 * sr) n <<= 1;
        revHist.assign ((size_t) n, 0.0f);  revHistMask = n - 1;
        { int e = 1; while ((double) e < 2.1 * sr) e <<= 1;  echoBuf.assign ((size_t) e, 0.0f);  echoMask = e - 1; }
        shimmerBelowBuf[0].assign (32768, 0.0f);  shimmerBelowBuf[1].assign (32768, 0.0f);  shimmerBelowW = 0;
        springTank.prepare (sr);
        springReturn.prepare (sr);
        resetExtra();
    }
    void resetExtra()
    {
        springTank.reset();
        springReturn.reset();
        std::fill (revHist.begin(), revHist.end(), 0.0f);
        revHistW = 0;  revHop = 0;  revNextGrain = 0;
        for (int g = 0; g < 2; ++g) { revGrainStart[g] = 0; revGrainAge[g] = 1; revGrainLen[g] = 1; }
        crushHold = 0.0f;  crushCount = 0;
        svf1[0] = svf1[1] = svf2[0] = svf2[1] = 0.0f;
        tremPhase = 0.0f;
        std::fill (echoBuf.begin(), echoBuf.end(), 0.0f);  echoW = 0;  echoLp = 0.0f;  echoLfo = 0.0f;
        mangleFb = mangleHp = mangleLp = mangleHold = 0.0f;  mangleCount = 0;
    }
};

class StarterAmpProcessor : public juce::AudioProcessor,
                            private juce::AsyncUpdater,
                            private juce::Timer,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    StarterAmpProcessor();
    ~StarterAmpProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockChunk (juce::AudioBuffer<float>&, juce::MidiBuffer&);   // the real work; never more than the prepared block size
    int preparedBlockSize = 512;
    juce::MidiBuffer emptyMidi;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Starter Amp v22"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // True while the message thread still has slow work to do for this instance (cab slots, neural models, latency).
    // Used by the Tone Lab, which runs a private copy of the plugin and has to wait for these before measuring.
    bool hasPendingRefresh() const
    {
        for (int s = 0; s < kCabBlocks; ++s) if (slotDirty[s].load()) return true;
        return asyncBusy.load() || asyncWanted.load() || namDirty.load() || latencyDirty.load() || pendingProgram.load() >= 0;
    }
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Cab impulse responses (short WAV recordings of a speaker cabinet).
    // slot 0 = IR A, slot 1 = IR B.
    // 'allowAutoAlign' is true when the user loads a file (not when a session is restored),
    // so a restored session keeps the delay and polarity settings it was saved with.
    void loadIR (const juce::File& file, int slot, bool allowAutoAlign = false);
    bool loadReverbIr (const juce::File& file, juce::String& message);      // v0.2: the IR reverb's own file (message thread)
    juce::String getReverbIrName() const { return juce::File (apvts.state.getProperty ("reverbIrPath").toString()).getFileNameWithoutExtension(); }
    juce::String getIRName (int slot) const;

    // Phase alignment of the two IRs: finds the time offset and polarity at which
    // IR B best lines up with IR A, and sets the Cab delay and Invert controls to match.
    // Returns false if it couldn't (for example only one IR is loaded).
    bool alignIRs();
    juce::String getAlignReport() const;

    // A copy of an IR's first few thousand samples at 48 kHz (for the IR display).
    static constexpr double kIRAnalysisRate = 48000.0;
    std::vector<float> getIRSnapshot (int slot) const;

    // v70, IR library: hear an IR file in place of the cab blocks without changing them.
    // The audition path takes the amp's signal, convolves it with the chosen file and replaces the cab blocks'
    // sum while it is on. Call these from the message thread.
    void auditionLoad (const juce::File& file);
    void setAuditionActive (bool on);
    bool isAuditionActive() const { return auditionOn.load(); }
    // Puts a file into a cab block: sets the block to "IR file", switches it on, and loads the file.
    void sendIRToBlock (const juce::File& file, int slot);
    // The smoothed curves of the built-in speakers (Flat mic, 4x12 Closed, Position 0.3), on the library's grid.
    static const std::vector<irlib::Curve>& builtinSpeakerCurves();
    static juce::StringArray builtinSpeakerNames();
    int getIRVersion() const { return irVersion.load(); }

    // A copy of the last few thousand output samples (mono mix) for the spectrum display.
    // The audio thread writes, the display thread reads; a rare torn read only shows as a
    // tiny glitch in the picture, never in the sound.
    struct AnalyserTap
    {
        static constexpr int size = 8192;
        std::array<float, size> data {};
        std::atomic<int> pos { 0 };

        void push (const juce::AudioBuffer<float>& b, int numSamples)
        {
            int w = pos.load (std::memory_order_relaxed);
            const int channels = b.getNumChannels();
            for (int i = 0; i < numSamples; ++i)
            {
                float s = 0.0f;
                for (int c = 0; c < channels; ++c)
                    s += b.getReadPointer (c)[i];
                data[(size_t) w] = channels > 0 ? s / (float) channels : 0.0f;
                w = (w + 1) & (size - 1);
            }
            pos.store (w, std::memory_order_release);
        }

        void copyLatest (float* dest, int n) const
        {
            const int w = pos.load (std::memory_order_acquire);
            for (int i = 0; i < n; ++i)
                dest[i] = data[(size_t) ((w - n + i) & (size - 1))];
        }
    };
    std::atomic<bool> toneListening { false };    // set by the tone-matching window while it is listening (fills preEqTap)
    // b34: DI Match. The learned curve of your playing (measured before the Transpose) and a loaded reference DI live here; the editor
    // starts and stops learning and loads references, the audio thread does the measuring and the correction.
    std::atomic<bool> diLearn { false };          // set by the DI Match panel; the audio thread clears it again after about 25 seconds
    std::atomic<int>  diFrames { 0 };             // frames measured so far in this learning run
    std::atomic<int>  diStatus { 0 };             // DiMatch::Status of the current settings
    bool diGetSource (DiMatch::Shape& s, float& lvl) const { const juce::SpinLock::ScopedLockType l (diLock); s = diSrc; lvl = diSrcLvl; return diHaveSrc; }
    bool diGetReference (DiMatch::Shape& s, float& lvl, juce::String& name) const { const juce::SpinLock::ScopedLockType l (diLock); s = diRef; lvl = diRefLvl; name = diRefName; return diHaveRef; }
    void diSetReference (const DiMatch::Shape& s, float lvl, const juce::String& name)
    { const juce::SpinLock::ScopedLockType l (diLock); diRef = s; diRefLvl = lvl; diRefName = name; diHaveRef = true; ++diVersion; }
    void diClearSource()    { const juce::SpinLock::ScopedLockType l (diLock); diHaveSrc = false; ++diVersion; }
    void diClearReference() { const juce::SpinLock::ScopedLockType l (diLock); diHaveRef = false; diRefName = {}; ++diVersion; }
    void diSaveToState (juce::ValueTree& state) const;
    void diLoadFromState (const juce::ValueTree& state);
    std::atomic<bool> analyserWanted { false };   // set by the spectrum display while it is on screen

    // Level meters (the highest sample since the display last asked; the display reads and resets them),
    // and the tuner: while it is active the input is copied into tunerTap and the output is muted.
    // The tuner is a display mode, not a saved setting, so a session never loads muted.
    std::atomic<float> meterIn { 0.0f }, meterOut { 0.0f };
    std::atomic<bool>  tunerActive { false };
    AnalyserTap preEqTap, outputTap, tunerTap;     // before the Post EQ, the final output, and the tuner's input

    // Compressor gain reduction in dB (0 or more), for the meter in the editor.
    float getCompGainReductionDb() const { return compGrDb.load(); }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateFilters();
    void processAcoustic (juce::AudioBuffer<float>&, juce::dsp::ProcessContextReplacing<float>&, int numChannels, int numSamples);
    void analyseIR (const juce::File& file, int slot);

    // Built-in cab models: when a slot's Source is "Built-in model", its IR is synthesised
    // from the speaker / mic / position / distance / angle controls (plus the shared cabinet
    // and room controls) and loaded the same way as a file IR.
    void parameterChanged (const juce::String& id, float) override;
    void handleAsyncUpdate() override;
    void timerCallback() override;
    // The audio thread must not call triggerAsyncUpdate() (it can block on the message system), so it sets a flag
    // and a message-thread timer picks it up.
    void requestAsync() noexcept { asyncWanted.store (true, std::memory_order_release); }
    std::atomic<bool> asyncWanted { false };
    std::atomic<int>  osModePub { 0 };
    std::atomic<int>  osLatPub[2] { { 0 }, { 0 } };
    void refreshSlot (int slot, bool force);
    std::atomic<bool> alignWanted { false };   // set when a cab block changed; handleAsyncUpdate then reruns Auto Align if it is on
    void refreshAllSlots();
    std::atomic<bool> slotDirty[kCabBlocks] {};
    std::atomic<bool> slotForce[kCabBlocks] {};
    std::atomic<bool> irActive[kCabBlocks]  {};   // does this block currently have an IR?
    void setAlignReport (const juce::String& text);
    void setParamValue (const char* id, float value);
    // Reads a parameter. The lookup by name is slow (a string map search), and processBlock asks for
    // about 150 values per block, so the result is cached per thread in a small table that is checked
    // by owner, address and spelling. If anything does not match, it falls back to the slow lookup.
    const std::uint64_t paramCacheId = []{ static std::atomic<std::uint64_t> n { 0 }; return ++n; }();   // unique per instance, so a new plugin at a reused address never sees stale entries
    // Joins two short names in a caller-supplied buffer (no heap use), for building IDs such as "od2Gain" on the audio thread.
    static const char* cat (char (&buf)[64], const char* a, const char* b)
    {
        std::size_t n = 0;
        for (const char* s : { a, b }) for (; *s != 0 && n < 63; ++s) buf[n++] = *s;
        buf[n] = 0;
        return buf;
    }
    float param (const char* id) const
    {
        struct Entry { std::uint64_t owner = 0; const char* key = nullptr; const std::atomic<float>* p = nullptr; char name[40] {}; };
        thread_local Entry table[512];
        std::uint32_t hv = 2166136261u;                       // hashed by spelling, so a name built on the stack each call still hits
        for (const char* c = id; *c != 0; ++c) hv = (hv ^ (std::uint8_t) *c) * 16777619u;
        Entry& e = table[hv & 511u];
        if (e.owner == paramCacheId && e.p != nullptr && std::strcmp (e.name, id) == 0)
            return e.p->load();
        auto* raw = apvts.getRawParameterValue (id);
        if (std::strlen (id) < sizeof (e.name))
        {
            e.owner = paramCacheId;  e.key = id;  e.p = raw;
            std::strcpy (e.name, id);
        }
        return raw->load();
    }

    // The global Mode (Normal / Low Latency / High Quality) decides the quality settings that the
    // individual menus would otherwise pick. In Normal the menus apply as before.
    int perfMode() const { return juce::jlimit (0, 2, (int) param ("perfMode")); }
    int wantedAmpQuality() const
    {
        const int m = perfMode();
        return m == 1 ? 2 : (m == 2 ? 0 : juce::jlimit (0, 2, (int) param ("ampQuality")));   // 0 = 4x, 1 = 2x, 2 = 1x
    }
    int wantedFuzzStages() const   // 2 = 4x, 1 = 2x, 0 = none
    {
        const int m = perfMode();
        return m == 1 ? 0 : (m == 2 ? 2 : (wantedAmpQuality() == 0 ? 2 : 1));
    }

    // A delay's time in ms: its own Time knob, or, if a note value is chosen, that note at the tempo.
    float delayTimeMs (const char* noteId, const char* timeId, double bpm) const
    {
        const int note = (int) param (noteId);
        if (note <= 0)
            return param (timeId);
        return std::min (1990.0f, (float) (60000.0 / bpm) * kNoteBeats[std::min (note, 10)]);
    }

    // Same idea for any millisecond setting (grain size, swell fade-in, reverb pre-delay...): the knob's own
    // value, or one note length at the tempo, never above maxMs.
    float syncedMs (const char* noteId, const char* msId, double bpm, float maxMs) const
    {
        const int note = (int) param (noteId);
        if (note <= 0)
            return param (msId);
        return std::min (maxMs, (float) (60000.0 / bpm) * kNoteBeats[std::min (note, 10)]);
    }

    using Filter       = juce::dsp::IIR::Filter<float>;
    using Coeffs       = juce::dsp::IIR::Coefficients<float>;
    using ArrCo        = juce::dsp::IIR::ArrayCoefficients<float>;     // same maths, no allocation when assigned into an existing Coefficients
    using StereoFilter = juce::dsp::ProcessorDuplicator<Filter, Coeffs>;

    PickupDeNoise deNoise;                // hum notches + dynamic hiss filter, ahead of the gate
    NoiseGate gate;
    juce::dsp::Convolution conv[kCabBlocks];                 // the cab: up to four blocks (IRs)
    juce::dsp::Convolution auditionConv;                     // v70: the IR library's audition path
    juce::dsp::Convolution irRevConv;                        // v0.2: the IR reverb type (loaded file or generated response)
    juce::AudioBuffer<float> irRevBuf, irRevRaw, irRevShaped;
    std::vector<float> irRevPre;
    double irRevRawRate = 48000.0;
    juce::String irRevRawPath, irRevSig;                     // message thread only
    void rebuildIrReverb();
    juce::AudioBuffer<float> auditionBuf;
    std::atomic<bool> auditionOn { false }, auditionLoaded { false };
    juce::AudioBuffer<float> cabBuffers[kCabBlocks];         // one scratch copy of the signal per block

    // Cab section extras
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> cabDelayLines[kCabBlocks];
    juce::SmoothedValue<float> cabDelaySmooth[kCabBlocks];         // in samples
    float lastCabGain[kCabBlocks * 2] = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };   // left, right for each block
    bool  cabGainsInit = false;

    // What the IR display and the alignment work on
    std::vector<float> irAnalysis[kCabBlocks];
    juce::String alignReport;
    mutable juce::CriticalSection irLock;
    std::atomic<int> irVersion { 0 };

    DelayEffect delayPre, delayPost;      // two independent delays, each with its own controls
    NoiseEffect noise;                    // pitch-shifting noise pedal
    GrossEngine gross;                    // dissonant "Clash" layer of the noise pedal
    RingModEffect ring;                   // ring modulator layer of the noise pedal
    FuzzEffect fuzz2;                     // v93: second fuzz block
    juce::AudioBuffer<float> fuzzPar;     // scratch copy for the parallel route
    FuzzEffect fuzz;                      // three-style fuzz
    CompressorEffect comp;                // compressor, before the amp
    std::atomic<float> compGrDb { 0.0f }; // gain reduction, for the meter
    juce::dsp::Reverb reverb;             // (no longer used: the reverb below does both normal and shimmer)

    // Reverb extras: pre-delay, wet filters, ducking

    Doubler doubler;                      // after the textures, before the post-amp delay
    Widener widener;                      // after the reverb, before the output level
    bool dblWasOn = false, widWasOn = false;
    float lastWidLevel = 1.0f;
    Tremolo tremolo;                      // texture effects, after the Post EQ
    FxExtra::FilterFx filterFx;  FxExtra::VolumePanFx volPanFx;  FxExtra::MultibandCompFx mbComp;  FxExtra::GateExpFx gateFx;   // b15
    bool filtWasOn = false, vpWasOn = false, mbWasOn = false, gxWasOn = false;
    Looper looper;                         // b16
    float loopInPk = 0.0f, loopPrev[4] = {};
    bool  loopPrimed = false;
    int   loopParamIndex[4] = { -1, -1, -1, -1 };
    Wobble  wobble;
    Swell   swell;
    Granular granular;
    std::vector<float> swellGains;
    Tremolo ampTrem;  std::vector<float> tremDip, tremEnvIn;  float tremEnvState = 0.0f;     // b7: tremolo inside the amp
    juce::AudioBuffer<float> midSave;  bool ampTremWasOn = false;
    bool tremInAmp = false;  float tremInDepth = 0.0f, tremInPick = 0.0f;
    bool tremWasOn = false, wobWasOn = false, swellWasOn = false, granWasOn = false;

    // LFO rate in Hz: its own Rate knob, or a note value at the tempo (the note's length is one LFO cycle).
    float lfoRateHz (const char* noteId, const char* hzId, double bpm) const
    {
        const int note = (int) param (noteId);
        if (note <= 0)
            return param (hzId);
        return (float) (bpm / 60.0 / kNoteBeats[std::min (note, 10)]);
    }

    Wah wah;                              // before the amp
    bool wahWasOn = false;
    Phaser phaser;                        // before the amp
    BbdChorus chorus;                    // before the amp
    bool phaserWasOn = false, chorusWasOn = false;

    // Shimmer mode: its own reverb (run one sample at a time, so the feedback
    // loop is sample-accurate) plus a pitch shifter in the feedback path.
    OutputLimiter limiter;
    Transposer transposer;                // pitch shift at the start of the chain
    // b34: DI Match (after the Transpose)
    DiMatch::Engine diEngine;
    DiMatch::Analyzer diAnalyzer;
    std::vector<float> diFrame = std::vector<float> ((size_t) DiMatch::Analyzer::size, 0.0f);
    int diFrameFill = 0;  bool diWasLearn = false;
    mutable juce::SpinLock diLock;
    DiMatch::Shape diSrc {}, diRef {};  float diSrcLvl = -18.0f, diRefLvl = -18.0f;  bool diHaveSrc = false, diHaveRef = false;
    juce::String diRefName;
    std::atomic<uint32_t> diVersion { 1 };
    ChugGate chugGate;                    // fast gate after the preamp (modern metal controls)

    // Pitch pedal and octaver (v41)
    Transposer wowT, octV1, octV2;
    GuitarSynth synth;  bool synWasOn = false;                // v0.2
public:
    int synthNote() const { return synth.shownNoteNumber(); }
private:
    HarmonizerEngine harm;                       // v52
public:
    RiffRecorder riff;                           // v52
    std::atomic<int> riffRecMode { 0 };          // the mode the current recording was made in (0 DI, 1 Full tone)
    void riffRecord() { riffRecMode.store ((int) param ("riffMode")); riff.record(); }
    void riffPlay()   { riff.play(); }
    void riffStop()   { riff.stop(); }
    void riffClear()  { riff.clear(); }
    uint64_t harmHeardNotes() const { return harm.heardNotes(); }     // v53: notes the harmonizer hears (bit i = MIDI note 24 + i)
private:
    juce::AudioBuffer<float> harmScratch;
    bool  harmWasOn = false;  int harmWindowMs = 0, harmPosPrev = -1;
    std::atomic<int> harmLatency { 0 };
    juce::AudioBuffer<float> pitchBuf1, pitchBuf2, pitchDry;
    bool  wowWasOn = false, octWasOn = false;
    int   wowWindowMs = 0, octWindowMs = 0;
    std::atomic<int> wowLatency { 0 }, octLatency { 0 };
    float wowSemis = 0.0f, octLp1 = 0.0f, octLp2 = 0.0f;

    // Cab blocks (v41): per-block 5-band EQ, resonance and phase, and the speaker dynamics
    StereoFilter cabEqF[kCabBlocks][5], cabRsoF[kCabBlocks], cabPhsF[kCabBlocks];
    StereoFilter cabTiltLoF[kCabBlocks], cabTiltHiF[kCabBlocks];   // v70: per-block Tilt
    SpeakerDynamics speakerDyn[kCabBlocks];
    cabreal::ResonanceTamer cabTamer;      // v65
    cabreal::MotionDrift    cabMotion;
    bool cabTamerWasOn = false, cabMotionWasOn = false;

    // Parallel Amp B (v40): a second, complete preamp / tone stack that runs beside the main amp.
    // It is defined in PluginProcessor.cpp (it needs the voicing tables there).
    struct ParallelAmp;
    std::unique_ptr<ParallelAmp> ampB;
    juce::AudioBuffer<float> ampBBuf;     // Amp B's output (mono)
    std::unique_ptr<ParallelAmp> ampC, ampD;      // v0.2 Triple Blend (amp 31): parts two and three (part one is Amp A's own engine)
    juce::AudioBuffer<float> ampCBuf, ampDBuf;
    bool  blendWasOn = false;
    float lastBlendGain = 1.0f;
    bool  ampBWasOn = false;
    float lastALevel = 1.0f;
    juce::AudioBuffer<float> tpBuf;       // scratch for the transposer's wet signal
    std::vector<float> gateKey;  bool gateKeyOk = false;     // b34: the Gate pedal's input key
    StereoFilter fmtCut1, fmtBoost1, fmtCut2, fmtBoost2;   // the EQ-style "formant" shift
    StereoFilter metalFizz, metalDepth;   // modern metal: fizz low-pass and low-end depth
    float fmtGainDb = 0.0f;
    bool  tpWasOn = false, chugWasOn = false;
    int   tpWindowMs = 0;
    std::atomic<int> tpLatency { 0 };
    std::atomic<int> limLatency { 0 };
    bool limWasOn = false;

    // v93: everything one drive pedal needs. Drive 1 (d1) is the original; Drive 2 (d2) is a full copy for stacking or running in parallel.
    // osA / osB are extra oversamplers (4x and 2x, like the amp's) used only when the drive runs on its own (Drive 2, or Drive 1 in parallel).
    struct DriveState
    {
        StereoFilter odGainHighPass, odMidPeak, odEdgeShelf, osOdTone;
        NoiseGate odGate;
        juce::AudioBuffer<float> odGainBuffer;
        float lastOdGainPath = 1.0f, lastOdClean = 1.0f, lastOdVolume = 1.0f;
        Circuit::Bq odCircBp, odCircGe;
        Circuit::DiodeLimiter odCircDiode;
        Circuit::RailLimiter odCircRail;
        Circuit::NewPedals newPed;
        Circuit::KlonCircuit klon;
        Circuit::Ds1Circuit ds1;
        Circuit::OcdCircuit ocd;
        float odCircPeak = 1.0f;
        bool  odCircRailMode = false;
        juce::dsp::Oversampling<float> osA { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
        juce::dsp::Oversampling<float> osB { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
        // v94: the Mix knob. A copy of the input, taken before the pedal, goes through its own oversampler of the same kind so it lines up with the wet path.
        juce::dsp::Oversampling<float> osDryA { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
        juce::dsp::Oversampling<float> osDryB { 1, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
        juce::AudioBuffer<float> dryBuf;  bool dryValid = false;  float lastMix = 1.0f;
    };
    DriveState d1, d2;
    // Upsampled dry copy for the Mix knob (nullptr when Mix is 1). m = its length. Blends it into y (m samples): y = mix * y + (1 - mix) * dry.
    void blendDriveDry (DriveState& d, float* y, int m, int numSamples, float mix);
    juce::AudioBuffer<float> drivePar;     // scratch copy for the parallel route
    int  frontDrive = 0;  bool frontOn = true, seriesBoth = false, parallelBoth = false;  bool driveMoved[2] = { false, false };
    void resolveDriveRoute();
    int  driveCode (int which);
    void updateDrive (int which);                                              // refresh one drive's filters and circuits from its parameters
    int  runDriveLinear (int which, juce::AudioBuffer<float>& buffer, int numSamples, bool& circ);   // the base-rate half; returns the internal code
    void runDriveStandalone (int which, juce::AudioBuffer<float>& buffer, int numSamples);           // the whole drive on its own (own oversampling)

    // Filters running at the normal sample rate
    StereoFilter preHighPass;                                  // tighten lows
    StereoFilter bassShelf, lowMidPeak, highMidPeak, trebleShelf, postLowPass; // tone stack + speaker
    StereoFilter dcBlock;                                      // removes DC after the amp
    StereoFilter cabLowCut, cabHighCut, cabTiltLow, cabTiltHigh;   // cab section: global filters
    StereoFilter cabHp, cabLowPeak, cabMidPeak, cabLp;             // built-in cab (used when no IR is loaded)
    StereoFilter cabLowCutF[kCabBlocks], cabHighCutF[kCabBlocks];  // per-block filters
    StereoFilter cabResoPeak, cabAirShelf;                         // cab resonance and air
    StereoFilter eqBands[kEqBands];                                // Post EQ
    StereoFilter eqHp1, eqHp2, eqLp1, eqLp2;                       // Post EQ high-pass and low-pass (two stages each for 24 dB/oct)
    float lastEqLevel = 1.0f;

    // Filters running at the oversampled rate (they sit between the clippers)
    StereoFilter osVoicePeak, osAmpHighPass1, osAmpHighPass2, osAmpHighPass3, osAmpLowPass2, osAmpLowPass3, osPresence, osBright;
    StereoFilter ampResoPk;                                    // amp realism: low-end resonance (normal rate)

    // The amp runs on one channel (a guitar is mono), at 4x or 2x the sample rate, or not oversampled.
    juce::dsp::Oversampling<float> oversampler  { 1, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };   // 4x
    juce::dsp::Oversampling<float> oversampler2 { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };   // 2x
    int    osMode = 0;                    // 0 = 4x, 1 = 2x, 2 = off
    bool   filtersDirty = true;           // forces updateFilters() on the next block
    std::vector<std::atomic<float>*> allParamPtrs;   // every parameter's value, for change detection
    std::vector<float> lastParamVals;
    bool paramsChanged();
    int  osLatencySamples() const;
    std::atomic<bool> acousticNoOs { false };     // v0.2: the Acoustic amp does not run through the amp oversampler, so it reports no delay for it
public:
    // Writes the current cab blend (the active blocks with their level, pan, delay, polarity, filters, EQ,
    // resonance and phase, and the cab-wide filters) as a stereo 24-bit WAV file at 48 kHz. Returns a message.
    // Factory presets (built in, generic names): reset the sound to defaults, then apply the preset.
    static int factoryPresetCount();
    static juce::StringArray factoryPresetTags (int index);   // style, amp type and character, worked out from the preset's settings
    static juce::String factoryPresetName (int index);
    static juce::String factoryPresetCategory (int index);
    void loadFactoryPreset (int index);

    // ---- MIDI ----------------------------------------------------------------------------
    // Program Change n loads factory preset n (counting from 0). Control changes (CC 0 to 127) and
    // pitch bend (slot 128) can each drive any one parameter; Learn binds the next one that arrives.
    static constexpr int kMidiSlots = 129;
    static constexpr int kPitchBendSlot = 128;
    int  midiParamCount() const { return (int) midiParams.size(); }
    juce::String midiParamId (int index) const   { return juce::isPositiveAndBelow (index, midiParamCount()) ? midiParams[(size_t) index]->paramID : juce::String(); }
    juce::String midiParamName (int index) const { return juce::isPositiveAndBelow (index, midiParamCount()) ? midiParams[(size_t) index]->getName (64) : juce::String(); }
    int  midiIndexOf (const juce::String& paramId) const;
    int  midiTargetOf (int slot) const { return midiTarget[juce::jlimit (0, kMidiSlots - 1, slot)].load(); }
    bool midiInverted (int slot) const { return midiInv[juce::jlimit (0, kMidiSlots - 1, slot)].load(); }
    int  midiSlotFor (int paramIndex) const;                       // first slot bound to this parameter, or -1
    void midiBind (int slot, int paramIndex, bool invert);        // one parameter per slot; a parameter keeps one slot
    void midiUnbind (int slot);
    void midiSetInverted (int slot, bool invert) { midiInv[juce::jlimit (0, kMidiSlots - 1, slot)] = invert; }
    void midiResetDefaults();
    void midiStartLearn (int paramIndex) { midiLearnIndex = paramIndex; }
    void midiCancelLearn() { midiLearnIndex = -1; }
    int  midiLearning() const { return midiLearnIndex.load(); }
    static juce::String midiSlotName (int slot) { return slot == kPitchBendSlot ? juce::String ("Pitch bend") : "CC " + juce::String (slot); }
    std::atomic<int> midiChannel { 0 };          // 0 = every channel (omni), 1 to 16 = only that channel
    std::atomic<int> midiLastSlot { -1 };        // the last control that moved (for the display)
    std::atomic<int> midiLastValue { 0 };
    std::atomic<int> midiLastProgram { -1 };
    std::atomic<int> midiActivity { 0 };         // counts incoming mapped messages
    std::atomic<int> presetLoadCounter { 0 };    // goes up when a Program Change loaded a preset, so the editor can refresh

    // ---- Effect order -------------------------------------------------------------------
    // Two groups can be reordered: group 0 (before the amp) and group 1 (after the cab). Each has six
    // blocks, numbered in the default order. The list says which block runs first, second, and so on.
    //   group 0: 0 Fuzz, 1 Compressor, 2 Wah, 3 Phaser, 4 Chorus, 5 Pre-amp delay
    //   group 1: 0 Post EQ, 1 Textures (tremolo, wobble, swell fade, granular), 2 Doubler, 3 Post-amp delay, 4 Reverb, 5 Widener
    // b8: every effect below can sit Before the amp (group 0), After the cab (group 1) or In the amp's FX loop (group 2).
    // Block numbers (the same in all three groups): 0 Fuzz, 1 Compressor, 2 Wah, 3 Phaser, 4 Chorus, 5 Pre-amp delay,
    // 6 Mod FX A, 7 Mod FX B, 8 Post EQ, 9 Wobble + Swell, 10 Doubler (after the cab only), 11 Post-amp delay, 12 Reverb,
    // 13 Widener (after the cab only), 14 Tremolo. A block runs in the group its location setting names; each group's
    // list gives the order of the blocks that sit there.
    static constexpr int kFxBlocks = 27;   // b23: 20 Lo-Fi, 21 Boost (movable); 22 Drive 1, 23 Drive 2 (not movable yet)
    static constexpr int kFxGroups = 3;
    Looper& getLooper() { return looper; }
    int fxLocation (int block) const;                    // 0 = before the amp, 1 = after the cab, 2 = in the amp's loop
    bool fxCanMove (int block) const { return block != 10 && block != 13; }
    void moveFxBlockTo (int block, int toGroup, int visiblePosition, int row);   // cross-group move: sets the location and the place in that list
    static const char* fxName (int group, int block);
    std::array<int, kFxBlocks> getFxOrder (int group) const;
    void setFxOrder (int group, const std::array<int, kFxBlocks>& order);
    void moveFxBlock (int group, int fromPosition, int toPosition);
    void resetFxOrder();
    // v0.1 pedalboard: each block can sit on the main rail (0) or a parallel rail (1) of its group; the rails are crossfaded by the group's Row Balance
    int  getFxRow (int group, int block) const { return fxRow[juce::jlimit (0, kFxGroups - 1, group)][juce::jlimit (0, kFxBlocks - 1, block)].load(); }
    void setFxRow (int group, int block, int row);
    void placeFxBlock (int group, int block, int toPosition, int row);      // move to a position and set its rail in one step
    // b9: a pedal on the lower rail is its own parallel branch (fed from the lane's input). Per-pedal level, polarity and (after the cab) pan.
    static bool fxHasBranchControls (int block) { return block >= 0 && block < kFxBlocks && block != 10 && block != 13; }
    static juce::String fxBranchId (const char* kind, int block) { return juce::String (kind) + juce::String (block); }   // kind: fxLvl, fxPan, fxPol
    bool fxBlockOn (int block) const;

    // ---- Presets (a preset keeps the sound and the effect order, but not the MIDI mappings) ----
    void getPresetData (juce::MemoryBlock& destData);
    bool loadPresetData (const void* data, int sizeInBytes);

    // ---- Undo / redo and A/B ------------------------------------------------------------------
    // A snapshot is every parameter value plus the effect order and the A/B slot. The editor calls
    // undoTick about 10 times a second; changes made while you are working the mouse or keyboard are
    // grouped into undo steps, and changes that arrive by themselves (MIDI, host automation) are
    // adopted without making one.
    struct Snapshot
    {
        std::vector<float> values;
        int ab = 0;
        bool operator== (const Snapshot& o) const { return ab == o.ab && values == o.values; }
        bool operator!= (const Snapshot& o) const { return ! (*this == o); }
    };
    Snapshot captureSnapshot() const;
    void applySnapshot (const Snapshot&);
    void undoTick (bool userActive);
    bool undo();
    bool redo();
    bool canUndo() const { return ! undoStack.empty() || undoPending; }
    bool canRedo() const { return ! redoStack.empty(); }

    int  abActive() const { return abIdx.load(); }
    void abSelect (int slot);
    void abCopyToOther();                       // copies the active slot into the other one
    void abSetLevelMatch (bool on);
    bool abLevelMatch() const { return abMatch.load(); }
    float abTrimDb() const { return abTrimTargetDb.load(); }
    void abForgetLevels();
    bool abHasSlot (int slot) const { return abHas[juce::jlimit (0, 1, slot)]; }

    juce::String exportCabBlend (const juce::File& file);

    // Neural amp captures (.nam). Four places a model can live: 0 = Amp A, 1 = Amp B, 2 = pedal before the amp,
    // 3 = pedal after the cab. Loading runs on the message thread; the audio thread only ever swaps a pointer.
    static constexpr int kNamSlots = 4;
    bool loadNamFile (int slot, const juce::File& file, juce::String& message);   // true if the model is now loaded
    bool loadNamFile (const juce::File& file, juce::String& message) { return loadNamFile (0, file, message); }
    void clearNam (int slot = 0);
    bool namHasModel (int slot = 0) const { return namSlot[juce::jlimit (0, kNamSlots - 1, slot)].loaded.load(); }
    juce::String getNamSummary (int slot = 0) const { return namSlot[juce::jlimit (0, kNamSlots - 1, slot)].summary; }   // what the model is, or why it did not load
    static const char* namPathKey (int slot);                                       // where a slot's file path is kept in the session
    juce::String namStatus;                                                         // last load message (message thread only)
private:
    std::atomic<bool> latencyDirty { false };

    // ---- NAM model player (v50, four slots from v51)
    void reloadNamFromState();
    NamSlot namSlot[kNamSlots];
    std::atomic<bool> namDirty { false };
    int namMaxBlock = 512;
    LoFiWarble lofi;
    bool lofiWasOn = false;

    // v51
    BoostStage boost;
    juce::AudioBuffer<float> boostDry;       // v94: the dry copy for the Boost Mix knob
    bool boostWasOn = false;
    ModFx modFx[4];                                  // b28: four mod slots (A to D)
    bool modWasOn[4] { false, false, false, false };
    Wobble modWobble[4];
    bool modWobWasOn[4] { false, false, false, false };
    BbdChorus chorusSlot[4];                         // b28: the Chorus, Phaser and Lo-Fi types of a slot have their own processors
    Phaser phaserSlot[4];
    LoFiWarble lofiSlot[4];
    FxExtra::FilterFx filterSlot[4];                 // b29: the Filter and Slicer types of a slot
    FxExtra::SlicerFx slicerSlot[4];
    int modKind[4] { -1, -1, -1, -1 };
    static constexpr const char* modOnId[4]   = { "mfAOn", "mfBOn", "mfCOn", "mfDOn" };
    static constexpr const char* modTypeId[4] = { "mfAType", "mfBType", "mfCType", "mfDType" };


    // Last block's gains, so we can ramp smoothly (no zipper noise).
    float lastInputGain = 1.0f, lastOutputGain = 1.0f, lastTunerGain = 1.0f;
    Circuit::DynaComp dyna;                 // v93: Comp style Dyna (OTA)
    AutoLeveler fuzzLeveler, fuzzLeveler2, eqLeveler;       // the Fuzz and Post EQ "Auto level" options
    // Effect order, A/B, undo
    std::atomic<int> fxOrder[kFxGroups][kFxBlocks];
    std::atomic<int> fxRow[kFxGroups][kFxBlocks];
    std::atomic<float>* fxLocPtr[kFxBlocks] = {};
    std::atomic<float>* fxLvlPtr[kFxBlocks] = {};
    std::atomic<float>* fxPanPtr[kFxBlocks] = {};
    std::atomic<float>* fxPolPtr[kFxBlocks] = {};
    std::atomic<float>* fxOnPtr[kFxBlocks][2] = {};
    // b10: split / merge per lane (how the lower row is fed and merged)
    std::atomic<float>* laneSplitPtr[kFxGroups] = {};
    std::atomic<float>* laneXoverPtr[kFxGroups] = {};
    std::atomic<float>* laneBLvlPtr[kFxGroups] = {};
    std::atomic<float>* laneBPanPtr[kFxGroups] = {};
    std::atomic<float>* laneBPolPtr[kFxGroups] = {};
    std::atomic<float>* laneRegStartPtr[kFxGroups] = {};   // b25: where each lane's parallel section starts and ends
    std::atomic<float>* laneRegEndPtr[kFxGroups] = {};
    juce::dsp::LinkwitzRileyFilter<float> laneXover[kFxGroups];
    bool laneXoverActive[kFxGroups] = {};
    ReverbUnit revU[2];
    std::atomic<bool> loopHopOn { false };       // b8: the amp is split in two (preamp, then the loop's effects, then the power stage)
    juce::AudioBuffer<float> railIn, railMain;     // the parallel rails: the group's input, and the main rail's result
    juce::String fxRowToString (int group) const;
    void fxRowFromString (int group, const juce::String& text);
    juce::String fxOrderToString (int group) const;
    void fxOrderFromString (int group, const juce::String& text);
    void saveState (juce::MemoryBlock& destData, bool withMidi);
    bool restoreState (const void* data, int sizeInBytes, bool withMidi);
    std::vector<Snapshot> undoStack, redoStack;
    Snapshot undoCurrent, undoPendingBefore, abSlot[2];
    bool undoCurrentValid = false, undoPending = false, abHas[2] { false, false };
    std::atomic<int>   abIdx { 0 };
    std::atomic<bool>  abMatch { true };
    std::atomic<float> abLoudDb[2] { { -200.0f }, { -200.0f } };   // loudness of each slot before the trim; -200 = not measured yet
    std::atomic<float> abTrimTargetDb { 0.0f };
    float lastAbTrimGain = 1.0f;
    void abComputeTrim (int from, int to);
    Metronome metro;
    // MIDI state. midiParams never changes after the constructor; the arrays are read by the audio thread.
    std::vector<juce::RangedAudioParameter*> midiParams;
    std::atomic<int>  midiTarget[kMidiSlots];    // index into midiParams, or -1
    std::atomic<bool> midiInv[kMidiSlots];
    int               midiLastSent[kMidiSlots];  // audio thread only: avoids re-sending a value that did not change
    std::atomic<int>  midiLearnIndex { -1 };
    std::atomic<int>  pendingProgram { -1 };
    std::atomic<bool> asyncBusy { false };       // true while handleAsyncUpdate is running (see hasPendingRefresh)
    void handleMidi (const juce::MidiBuffer&);
    void midiToState (juce::ValueTree& state) const;
    friend class StarterAmpEditor;
    void midiFromState (const juce::ValueTree& state);
    std::vector<float> autoRef;               // scratch: channel 0 before the effect being levelled
    float lastAmpGain1 = 1.0f, lastAmpGain2 = 1.0f, lastAmpGain3 = 1.0f, lastAmpTrim = 1.0f;
    float sagEnv[2] = { 0.0f, 0.0f };   // power-amp sag detector
    std::vector<float> hybridScratch;   // Hybrid mode: the classic power curve's result
    PowerSection::Params psParamsA;  PowerSection::Chain psChainA;  PowerSection::Supply psSupplyA;  float psFbLastA = 0.0f;   // v83
    TweedTone tweedA;                     // v93: the tweed combo's single Tone control
    PassiveStack passiveA;                // v81: true passive tone stacks (Amp A)
    StageShaper::StageState stageSt[3];   // v80: grid-blocking state of the circuit-derived preamp stages

    // Acoustic / piezo model
    StereoFilter acHp, acQuack, acBright, acHollow, acBodyLp, acWarm, acSpark;
    StereoFilter acBody[5];
    float acAirLp[2] = { 0.0f, 0.0f };
    std::vector<float> acRoomBuf[2];
    float acRoomLp[2] = { 0.0f, 0.0f };
    int   acRoomPos = 0;
    float acEnv = 0.0f;

    double currentSampleRate = 44100.0;
    double osSampleRate = 44100.0 * 4.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StarterAmpProcessor)
};
