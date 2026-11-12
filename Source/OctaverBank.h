#pragma once
// b79: the octaver's second engine. Up to eight voices (each a pitch and a level) added to the dry guitar, with
//   Tracking   : 0 Single note (the delay-line shifter, one per voice), 1 Chords (the spectral shifter, long window), 2 Chords (fast, short window)
//   Character  : 0 Clean, 1 to 4 the pitch pedal's bad-tracking shifter (PitchRough.h), one per voice; Grit scales the damage
//   Attack     : the voices fade in after each new note (the dry guitar can follow, see dryFx)
//   Decay      : the voices die away after each new note (so they are a short bloom, not a held tone)
//   Filter     : a low-pass over the voices (one pole when Resonance is 0, two poles with Q up to 6 above it)
//   Detune     : the voices an octave or more up move by up to +/-50 cents
// Ideas taken from the public descriptions of the EHX POG2 (five sliders, Attack, LP Filter with Q, Detune, Dry FX routing) and the HOG2 manual (voices at
// the fifth, the twelfth, the seventeenth, three octaves; Envelope with fade-in and decay). Their circuits and DSP are not public, so none of this is a model of them.
#include <algorithm>
#include <cmath>
#include <vector>
#include "FxExtra.h"
#include "PitchRough.h"

class OctaverBank
{
public:
    static constexpr int kVoices = 8;
    struct Params
    {
        int   nVoices = 0;
        float semis[kVoices] = {};
        float level[kVoices] = {};
        int   track = 0, character = 0;
        float grit = 0.5f;
        float detuneCents = 0.0f;
        float attack = 0.0f, decay = 0.0f;
        float tone = 0.5f, res = 0.0f;
        float dry = 1.0f;
        int   dryFx = 0;              // 0 untouched, 1 through the filter, 2 through attack and decay, 3 through both
        float windowMs = 28.0f, lowHz = 0.0f;       // the delay-line shifter's window (from the Transpose quality / range menus)
    };

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& t : tp) t.prepare (sampleRate);
        for (auto& s : ss) s.prepare (sampleRate);
        for (auto& r : rg) r.prepare (sampleRate);
        shortN = ss[0].windowSize();  longN = 2 * shortN;                // the spectral shifter's two window lengths (it starts on the short one)
        dly.prepare ((int) (0.12 * sampleRate));
        for (auto& b : wbuf) b.assign ((size_t) kMaxBlock, 0.0f);
        scratch.assign ((size_t) kMaxBlock, 0.0f);
        reset();
    }
    void reset()
    {
        for (auto& t : tp) t.reset();
        for (auto& s : ss) s.reset();
        for (auto& r : rg) r.reset();
        dly.reset();
        for (auto& a : wasActive) a = false;
        lpV = lpD = 0.0f;  bqV.reset();  bqD.reset();
        e1 = e2 = 0.0f;  hold = 0;  rampT = 1.0e9f;  decayG = 1.0f;  engine = -1;  winMs = -1.0f;
    }
    int latency (const Params& p) const
    {
        if (p.character > 0) return rg[0].latency (std::min (5, p.character));
        if (p.track > 0) return p.track == 1 ? longN : shortN;
        return (int) (0.5f * p.windowMs * 0.001f * sr);
    }

    void process (float* x, int n, const Params& p)
    {
        for (int pos = 0; pos < n; pos += kMaxBlock) chunk (x + pos, std::min (kMaxBlock, n - pos), p);
    }

private:
    static constexpr int kMaxBlock = 4096;

    void chunk (float* x, int n, const Params& p)
    {
        const int eng = p.character > 0 ? 10 + std::min (5, p.character) : std::min (2, p.track);
        if (eng != engine || (eng == 0 && std::abs (p.windowMs - winMs) > 0.01f))
        {
            for (auto& t : tp) { t.setLowHz (p.lowHz);  t.setWindowMs (p.windowMs);  t.reset(); }
            if (eng == 1 || eng == 2) for (auto& s : ss) s.setSize (eng == 1 ? 1 << 20 : 0);
            for (auto& s : ss) s.reset();
            for (auto& r : rg) r.reset();
            dly.reset();
            for (auto& a : wasActive) a = false;
            engine = eng;  winMs = p.windowMs;
        }
        const int lat = latency (p);
        const int nv = std::min (kVoices, p.nVoices);
        bool act[kVoices] = {};
        float semis[kVoices] = {};
        for (int v = 0; v < nv; ++v)
        {
            act[v] = p.level[v] > 0.001f;
            semis[v] = p.semis[v] + (p.semis[v] >= 11.5f ? p.detuneCents * 0.01f : 0.0f);
        }
        // ---- the voices
        if (eng == 0)
        {
            for (int v = 0; v < nv; ++v)
            {
                if (! act[v]) { wasActive[v] = false;  continue; }
                if (! wasActive[v]) tp[v].reset();
                std::copy (x, x + n, wbuf[v].begin());
                tp[v].process (wbuf[v].data(), scratch.data(), n, semis[v]);
                wasActive[v] = true;
            }
        }
        else if (eng <= 2)
        {
            for (int pr = 0; pr < kVoices / 2; ++pr)
            {
                const int a = 2 * pr, b = 2 * pr + 1;
                const bool ua = a < nv && act[a], ub = b < nv && act[b];
                if (! ua && ! ub) { wasActive[a] = wasActive[b] = false;  continue; }
                if (! wasActive[a] && ! wasActive[b]) ss[pr].reset();
                ss[pr].scaleMode = false;  ss[pr].nNotes = 0;
                ss[pr].alphaInterval[0] = std::pow (2.0f, (a < nv ? semis[a] : 0.0f) / 12.0f);
                ss[pr].alphaInterval[1] = std::pow (2.0f, (b < nv ? semis[b] : 0.0f) / 12.0f);
                for (int i = 0; i < n; ++i) { float y0, y1;  ss[pr].process (x[i], y0, y1);  wbuf[a][(size_t) i] = y0;  wbuf[b][(size_t) i] = y1; }
                wasActive[a] = ua;  wasActive[b] = ub;
            }
        }
        else
        {
            RoughShifter::Params rp;  rp.character = eng - 10;  rp.grit = p.grit;
            for (int v = 0; v < nv; ++v)
            {
                if (! act[v]) { wasActive[v] = false;  continue; }
                if (! wasActive[v]) rg[v].reset();
                rp.semis = semis[v];
                for (int i = 0; i < n; ++i) wbuf[v][(size_t) i] = rg[v].process (x[i], rp);
                wasActive[v] = true;
            }
        }
        // ---- the envelope, the filter and the mix (per sample)
        const float attackS = p.attack < 0.01f ? 0.0f : 0.02f * std::pow (75.0f, p.attack);
        const float decayK  = p.decay < 0.01f ? 1.0f : std::exp (-1.0f / (3.0f * std::pow (0.05f, p.decay) * sr));
        const float fc = std::min (0.45f * sr, 800.0f * std::pow (10.0f, p.tone));
        const bool twoPole = p.res >= 0.01f;
        const float k1 = 1.0f - std::exp (-6.2831853f * fc / sr);
        if (twoPole)
        {
            const float q = 0.70710678f * std::pow (8.5f, std::min (1.0f, p.res));
            bqV.lowpass (fc, sr, q);
            bqD.b0 = bqV.b0;  bqD.b1 = bqV.b1;  bqD.b2 = bqV.b2;  bqD.a1 = bqV.a1;  bqD.a2 = bqV.a2;      // the dry path's filter shares the coefficients but keeps its own state
        }
        const bool dryFilt = (p.dryFx & 1) != 0, dryEnv = (p.dryFx & 2) != 0;
        for (int i = 0; i < n; ++i)
        {
            const float d = dly.process (x[i], lat);
            // onset detector on the delayed dry (so the fade lines up with the shifted voices)
            const float a = std::fabs (d);
            e1 += (a - e1) * (a > e1 ? 0.05f : 0.002f);
            e2 += (a - e2) * (a > e2 ? 0.0005f : 0.00005f);
            if (hold > 0) --hold;
            else if (e1 > 1.8f * e2 + 0.003f) { rampT = 0.0f;  decayG = 1.0f;  hold = (int) (0.15f * sr); }
            float g = 1.0f;
            if (attackS > 0.0f && rampT < attackS) { const float u = rampT / attackS;  g = u * u * (3.0f - 2.0f * u); }
            rampT += 1.0f / sr;
            decayG *= decayK;
            g *= decayG;
            float s = 0.0f;
            for (int v = 0; v < nv; ++v) if (act[v]) s += p.level[v] * wbuf[v][(size_t) i];
            s *= g;
            float dd = dryEnv ? d * g : d;
            if (twoPole)
            {
                s = bqV.process (s);
                if (dryFilt) dd = bqD.process (dd);
            }
            else
            {
                lpV += k1 * (s - lpV);  s = lpV;
                if (dryFilt) { lpD += k1 * (dd - lpD);  dd = lpD; }
            }
            x[i] = p.dry * dd + s;
        }
    }

    Transposer tp[kVoices];
    SpectralShifter ss[kVoices / 2];
    RoughShifter rg[kVoices];
    FixedDelay dly;
    std::vector<float> wbuf[kVoices], scratch;
    bool  wasActive[kVoices] = {};
    FxExtra::Biquad bqV, bqD;
    float lpV = 0.0f, lpD = 0.0f, e1 = 0.0f, e2 = 0.0f, rampT = 1.0e9f, decayG = 1.0f, winMs = -1.0f, sr = 48000.0f;
    int   hold = 0, engine = -1, shortN = 2048, longN = 4096;
};
