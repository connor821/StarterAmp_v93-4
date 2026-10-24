// Guitar synth (v0.2): a single-note pitch tracker (YIN on a decimated copy of the input, short adaptive window) driving a
// small subtractive synth (two oscillators and a sub, a resonant low-pass, the guitar's own envelope on the filter and the level).
// Mono only: it follows the highest-energy single note. There is no chord tracking.
// Latency: the tracker needs about two pitch periods of audio before it can name a note (roughly 15 to 25 ms on the low E,
// under 10 ms up high). The dry signal is not delayed, so the synth trails a played note by that amount.
#pragma once
#include <atomic>
#include <cmath>
#include <algorithm>

class GuitarSynth
{
public:
    struct Params
    {
        int   wave1 = 0, wave2 = 0;          // 0 saw, 1 square, 2 pulse (25%), 3 triangle, 4 sine
        int   oct2 = 0;                      // -1, 0, +1 octave for oscillator 2 (menu 0..2)
        float detuneCents = 8.0f;            // oscillator 2 detune
        float sub = 0.0f;                    // sub oscillator level (square, one octave down)
        float grit = 0.0f;                   // soft clip before the filter
        float cutoffHz = 2500.0f, res = 0.2f;
        float envAmt = 0.5f;                 // how much the guitar's own level opens the filter
        float keyTrack = 0.5f;               // how much the filter follows the note
        float attackMs = 3.0f, releaseMs = 120.0f;
        float touch = 0.7f;                  // how much your picking dynamics control the level
        float glideMs = 0.0f;
        bool  bend = true;                   // follow bends and slides (otherwise snap to semitones)
        float sens = 0.5f;                   // input threshold
        float level = 0.8f, dry = 0.0f;
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        decim = std::max (1, (int) std::floor (sampleRate / 8000.0));
        fsd = sr / (float) decim;
        minLag = std::max (4, (int) (fsd / 1100.0f));
        maxLag = std::min (kHist / 2 - 2, (int) (fsd / 44.0f));
        hopD = std::max (8, (int) (fsd * 0.0015f));
        reset();
    }

    void reset()
    {
        std::fill (hist, hist + kHist, 0.0f);
        hpos = 0;  acc = 0.0f;  accN = 0;  sinceHop = 0;
        envFast = envSlow = peak = 0.0f;
        gateOpen = false;  holdCount = 0;
        haveEst = false;  estMidi = 0.0f;  estConf = 1.0f;  lastLag = 0.0f;
        curMidi = 0.0f;  smoothMidi = 0.0f;  noteOn = false;  pending = 0;  candMidi = 0.0f;  candCount = 0;
        ph1 = ph2 = phSub = 0.0f;  z1 = z2 = 0.0f;  ampEnv = 0.0f;  hpState = 0.0f;
        shownNote.store (-1);
    }

    int shownNoteNumber() const { return shownNote.load(); }      // for the display, -1 when silent

    // In place: x holds the guitar, and gets the synth (and the dry signal) written over it
    void process (float* x, int n, const Params& p)
    {
        const float attackK  = coef (0.5f), relFastK = coef (25.0f), slowK = coef (250.0f);
        const float onTh  = 0.002f + 0.05f * std::pow (1.0f - p.sens, 3.0f);   // sens 1 = very sensitive
        const float offTh = onTh * 0.45f;
        const float aK = coef (std::max (0.3f, p.attackMs)), rK = coef (std::max (5.0f, p.releaseMs));
        const float glideK = p.glideMs < 1.0f ? 1.0f : coef (p.glideMs);

        for (int i = 0; i < n; ++i)
        {
            const float in = x[i];
            feed (in);

            // ---- the guitar's level
            const float a = std::abs (in);
            envFast += (a > envFast ? attackK : relFastK) * (a - envFast);
            envSlow += slowK * (a - envSlow);
            peak = std::max (peak * 0.99995f, envFast);

            const bool wasOpen = gateOpen;
            if (! gateOpen && envFast > onTh) { gateOpen = true; holdCount = 0; }
            else if (gateOpen)
            {
                if (envFast < offTh) { if (++holdCount > (int) (0.04f * sr)) gateOpen = false; }
                else holdCount = 0;
            }
            // a new pluck while the gate is already open: the level jumps well above its own recent average
            const bool pluck = gateOpen && wasOpen && envFast > 2.2f * envSlow + onTh && envFast > 1.3f * prevFast;
            prevFast = envFast;
            if (pluck && noteOn) { pending = 1; candCount = 0; lastLag = 0.0f; }       // wait for a fresh pitch estimate, then retrigger
            if (! wasOpen && gateOpen) { pending = 1; candCount = 0; haveEst = false; lastLag = 0.0f; }
            if (! gateOpen) { noteOn = false; pending = 0; }

            // ---- decide the note from the newest estimate
            if (estFresh)
            {
                estFresh = false;
                if (gateOpen && haveEst)
                {
                    if (pending)
                    {
                        // wait for two estimates within half a semitone before starting (stops octave glitches at the attack)
                        if (candCount > 0 && std::abs (estMidi - candMidi) < 0.5f) ++candCount; else candCount = 1;
                        candMidi = estMidi;
                        if (candCount >= 2)
                        {
                            curMidi = std::round (estMidi);  if (p.bend) curMidi = estMidi;
                            if (! noteOn) smoothMidi = curMidi;
                            noteOn = true;  pending = 0;  candCount = 0;
                            ampEnv = std::min (ampEnv, 0.0f);
                            retrig = true;
                        }
                    }
                    else if (noteOn)
                    {
                        const float d = estMidi - curMidi;
                        if (p.bend && std::abs (d) < 2.5f) curMidi = estMidi;                   // a bend or vibrato: follow it
                        else if (! p.bend && std::abs (d) > 0.68f && std::abs (d) < 2.5f) curMidi = std::round (estMidi);
                        else if (std::abs (d) >= 2.5f)
                        {
                            // a jump: take it only when two estimates in a row agree (hammer-ons, slides, and not octave errors)
                            if (candCount > 0 && std::abs (estMidi - candMidi) < 0.6f) ++candCount; else candCount = 1;
                            candMidi = estMidi;
                            if (candCount >= 3) { curMidi = p.bend ? estMidi : std::round (estMidi); candCount = 0; }
                        }
                        else candCount = 0;
                    }
                }
            }
            if (noteOn) smoothMidi += glideK * (curMidi - smoothMidi);
            shownNote.store (noteOn ? (int) std::lround (curMidi) : -1);

            // ---- amplitude envelope
            const float target = noteOn ? 1.0f : 0.0f;
            ampEnv += (target > ampEnv ? aK : rK) * (target - ampEnv);

            float s = 0.0f;
            if (ampEnv > 0.0005f || noteOn)
            {
                const float f1 = 440.0f * std::pow (2.0f, (smoothMidi - 69.0f) / 12.0f);
                const float f2 = f1 * std::pow (2.0f, (float) (p.oct2 - 1) + p.detuneCents / 1200.0f);
                const float o1 = osc (p.wave1, ph1, f1), o2 = osc (p.wave2, ph2, f2);
                const float sb = osc (1, phSub, f1 * 0.5f);
                float v = 0.55f * o1 + 0.45f * o2 + p.sub * 0.7f * sb;
                if (p.grit > 0.001f) { const float g = 1.0f + 6.0f * p.grit; v = std::tanh (v * g) / std::tanh (g * 0.6f) * 0.6f; }
                // filter: cutoff follows the note and the guitar's own level
                const float lvl = peak > 1.0e-5f ? std::min (1.0f, envFast / peak) : 0.0f;
                const float oct = p.envAmt * 4.0f * (lvl - 0.5f) + p.keyTrack * (smoothMidi - 60.0f) / 12.0f;
                const float fc = std::min (0.45f * sr, std::max (40.0f, p.cutoffHz * std::pow (2.0f, oct)));
                v = svf (v, fc, p.res);
                const float dyn = (1.0f - p.touch) + p.touch * std::min (1.0f, 0.2f + 0.9f * lvl);
                s = v * ampEnv * dyn;
            }
            x[i] = p.dry * in + p.level * 0.8f * s;
        }
    }

private:
    static constexpr int kHist = 1024;
    float sr = 48000.0f, fsd = 8000.0f;
    int decim = 6, minLag = 8, maxLag = 130, hopD = 12;
    float hist[kHist] = {}, work[kHist] = {}, d[kHist] = {};
    int hpos = 0, accN = 0, sinceHop = 0;
    float acc = 0.0f;
    bool estFresh = false, haveEst = false;
    float estMidi = 0.0f, estConf = 1.0f, lastLag = 0.0f;

    float envFast = 0.0f, envSlow = 0.0f, peak = 0.0f, prevFast = 0.0f;
    bool gateOpen = false, noteOn = false, retrig = false;
    int holdCount = 0, pending = 0, candCount = 0;
    float candMidi = 0.0f, curMidi = 0.0f, smoothMidi = 0.0f, ampEnv = 0.0f;
    float ph1 = 0.0f, ph2 = 0.0f, phSub = 0.0f, z1 = 0.0f, z2 = 0.0f, hpState = 0.0f;
    std::atomic<int> shownNote { -1 };

    float coef (float ms) const { return 1.0f - std::exp (-1.0f / (0.001f * ms * sr)); }

    static float polyBlep (float t, float dt)
    {
        if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }
    float osc (int wave, float& ph, float f) const
    {
        const float dt = std::min (0.45f, f / sr);
        ph += dt;  if (ph >= 1.0f) ph -= 1.0f;
        switch (wave)
        {
            case 0:  return 2.0f * ph - 1.0f - polyBlep (ph, dt);
            case 1:  { float v = ph < 0.5f ? 1.0f : -1.0f; v += polyBlep (ph, dt); v -= polyBlep (std::fmod (ph + 0.5f, 1.0f), dt); return v; }
            case 2:  { float v = ph < 0.25f ? 1.0f : -1.0f; v += polyBlep (ph, dt); v -= polyBlep (std::fmod (ph + 0.75f, 1.0f), dt); return v - 0.5f; }
            case 3:  return 4.0f * std::abs (ph - 0.5f) - 1.0f;
            default: return std::sin (6.2831853f * ph);
        }
    }
    // Two-pole state-variable low-pass (trapezoidal), resonance 0..1
    float svf (float v, float fc, float res)
    {
        const float g = std::tan (3.14159265f * fc / sr);
        const float k = 2.0f - 1.9f * std::min (1.0f, std::max (0.0f, res));
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = v - z2;
        const float v1 = a1 * z1 + a2 * v3;
        const float v2 = z2 + a2 * z1 + a3 * v3;
        z1 = 2.0f * v1 - z1;  z2 = 2.0f * v2 - z2;
        return v2;
    }

    // ---- the tracker
    void feed (float in)
    {
        acc += in;
        if (++accN >= decim)
        {
            hist[hpos] = acc / (float) decim;
            hpos = (hpos + 1) % kHist;
            acc = 0.0f;  accN = 0;
            if (++sinceHop >= hopD) { sinceHop = 0; analyse(); }
        }
    }
    void analyse()
    {
        // window: about 2.4 periods of the last note, 64 to 170 samples (8 to 21 ms)
        const int W = lastLag > 0.0f ? std::min (170, std::max (64, (int) (2.4f * lastLag))) : 170;
        const int need = W + maxLag + 2;
        for (int i = 0; i < need; ++i) work[i] = hist[(hpos - need + i + kHist * 2) % kHist];     // oldest first
        const float* w = work + (need - W - maxLag - 1);
        float en = 0.0f;
        for (int j = 0; j < W; ++j) en += w[maxLag + 1 + j] * w[maxLag + 1 + j];
        if (en / (float) W < 1.0e-8f) return;

        const float* a = w + maxLag + 1;                         // the newest W samples
        d[0] = 1.0f;
        float run = 0.0f;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            float s = 0.0f;
            const float* b = a - tau;
            for (int j = 0; j < W; ++j) { const float df = a[j] - b[j]; s += df * df; }
            run += s;
            d[tau] = run > 1.0e-12f ? s * (float) tau / run : 1.0f;
        }
        int best = -1;
        for (int tau = minLag; tau < maxLag; ++tau)
            if (d[tau] < 0.16f)
            {
                while (tau + 1 < maxLag && d[tau + 1] < d[tau]) ++tau;
                best = tau;  break;
            }
        if (best < 0) return;                                    // no confident period: keep the old note
        float tauF = (float) best;
        const float y0 = d[best - 1], y1 = d[best], y2 = d[best + 1];
        const float den = y0 - 2.0f * y1 + y2;
        if (std::abs (den) > 1.0e-9f) tauF += 0.5f * (y0 - y2) / den;
        const float hz = fsd / tauF;
        if (hz < 45.0f || hz > 1200.0f) return;
        estMidi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
        lastLag = tauF;
        haveEst = true;  estFresh = true;
    }
};
