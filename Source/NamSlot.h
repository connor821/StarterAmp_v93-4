#pragma once
// A place a neural capture (.nam model) can live (v51). The plugin has four: Amp A, Amp B, and two pedal slots.
// The model is built on the message thread and handed to the audio thread through a short lock that the audio
// thread only ever try-locks, so loading never blocks or allocates on the audio thread.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>
#include <juce_core/juce_core.h>
#include "NamPlayer.h"

struct NamSlot
{
    static constexpr int kPlayers = 2;                 // a second player is only used by the stereo pedal slot

    juce::SpinLock lock;
    std::unique_ptr<NamLite::Player> cur[kPlayers];    // used by the audio thread only
    std::unique_ptr<NamLite::Player> inbox[kPlayers];  // handed over under 'lock' (a new model, or the retired one)
    bool inboxFresh = false;
    std::atomic<bool> loaded { false };
    std::atomic<float> normDb { 0.0f };                // output gain that evens the model to the NAM reference loudness
    juce::String summary;                              // message thread only

    // audio-thread state
    float lastIn = 1.0f, lastOut = 1.0f, lastMix = 1.0f;
    bool  wasActive = false;
    std::vector<float> delayBuf;                       // power of two; used to line things up in time
    std::vector<float> dryBuf, wetA, wetB, dline[2];
    int   delayPos = 0, dpos[2] { 0, 0 };
    bool  dualWas = false;

    void prepare (int maxBlock)
    {
        delayBuf.assign (1024, 0.0f);
        dryBuf.assign ((size_t) std::max (512, maxBlock) + 16, 0.0f);
        delayPos = 0;  dpos[0] = dpos[1] = 0;
        for (auto& d : dline) d.assign (256, 0.0f);
        wetA.assign ((size_t) std::max (512, maxBlock) + 16, 0.0f);
        wetB.assign (wetA.size(), 0.0f);
        lastIn = lastOut = lastMix = 1.0f;
        wasActive = false;
        dualWas = false;
    }

    // Audio thread: pick up a model that was installed since the last block (a pointer swap, no allocation).
    void pickUp()
    {
        const juce::SpinLock::ScopedTryLockType l (lock);
        if (l.isLocked() && inboxFresh)
        {
            for (int k = 0; k < kPlayers; ++k)
                std::swap (cur[k], inbox[k]);
            inboxFresh = false;
        }
    }

    // Message thread: hand over new players (or none, to unload). The old ones are destroyed here, off the audio thread.
    void install (std::unique_ptr<NamLite::Player> a, std::unique_ptr<NamLite::Player> b, float newNormDb)
    {
        std::unique_ptr<NamLite::Player> old[kPlayers];
        normDb.store (newNormDb);
        loaded.store (a != nullptr);
        {
            const juce::SpinLock::ScopedLockType l (lock);
            for (int k = 0; k < kPlayers; ++k)
                old[k] = std::move (inbox[k]);     // either a retired model or one the audio thread has not taken yet
            inbox[0] = std::move (a);
            inbox[1] = std::move (b);
            inboxFresh = true;
        }
    }

    // ---- A neural capture used as a pedal (v51): input and output gain, a dry/wet mix, and optional dual-mono.
    struct PedalSettings { bool on; float inDb, outDb, mix; bool matchLoudness, dual; };

    // l: the signal (left), r: the right channel or nullptr for mono. Audio thread; call pickUp() first.
    // With a single player and a stereo signal the capture sees the mono sum and the same result goes to both sides,
    // blended with each side's own dry signal. With 'dual' (and a second player) each side has its own player.
    bool processPedal (float* l, float* r, int n, const PedalSettings& st)
    {
        const bool have = cur[0] != nullptr;
        const bool want = st.on && have;
        if (! have || (! want && ! wasActive))
        {
            wasActive = false;
            return false;
        }
        if ((int) dryBuf.size() < 2 * n) dryBuf.resize ((size_t) 2 * n + 16);
        float* dryL = dryBuf.data();
        float* dryR = dryBuf.data() + n;
        const bool stereo = r != nullptr;
        const bool useDual = stereo && st.dual && cur[1] != nullptr;
        if (useDual && ! dualWas)
            cur[1]->reset();
        dualWas = useDual;

        std::copy (l, l + n, dryL);
        if (stereo) std::copy (r, r + n, dryR);
        const int lat = cur[0]->latencyHostSamples();
        for (int ch = 0; ch < (stereo ? 2 : 1); ++ch)
        {
            auto& d = dline[ch];
            if (d.empty()) d.assign (256, 0.0f);
            if (lat > 0 && lat < (int) d.size())
            {
                float* x = ch == 0 ? dryL : dryR;
                const int mask = (int) d.size() - 1;
                for (int i = 0; i < n; ++i)
                {
                    d[(size_t) (dpos[ch] & mask)] = x[i];
                    x[i] = d[(size_t) ((dpos[ch] - lat) & mask)];
                    ++dpos[ch];
                }
            }
        }

        const float inG  = std::pow (10.0f, st.inDb / 20.0f);
        const float outG = std::pow (10.0f, (st.outDb + (st.matchLoudness ? normDb.load() : 0.0f)) / 20.0f);
        auto run = [&] (NamLite::Player& pl, float* x, const float* src, bool sum, const float* src2)
        {
            for (int i = 0; i < n; ++i) x[i] = sum ? 0.5f * (src[i] + src2[i]) : src[i];
            const float si = (inG - lastIn) / (float) n, so = (outG - lastOut) / (float) n;
            float gi = lastIn;
            for (int i = 0; i < n; ++i) { x[i] *= gi; gi += si; }
            pl.process (x, n);
            float go = lastOut;
            for (int i = 0; i < n; ++i)
            {
                const float v = std::isfinite (x[i]) ? std::max (-16.0f, std::min (16.0f, x[i])) : 0.0f;
                x[i] = v * go;
                go += so;
            }
        };
        wetA.resize ((size_t) n);
        if (useDual) wetB.resize ((size_t) n);
        if (! stereo)             run (*cur[0], wetA.data(), l, false, nullptr);
        else if (useDual)       { run (*cur[0], wetA.data(), l, false, nullptr);  run (*cur[1], wetB.data(), r, false, nullptr); }
        else                      run (*cur[0], wetA.data(), l, true, r);   // the mono sum of both sides
        lastIn = inG;
        lastOut = outG;

        const float mixTarget = want ? st.mix : 0.0f;
        const float from = wasActive ? lastMix : 0.0f;
        const float step = (mixTarget - from) / (float) n;
        float m = from;
        for (int i = 0; i < n; ++i)
        {
            l[i] = dryL[i] + m * (wetA[(size_t) i] - dryL[i]);
            if (stereo) r[i] = dryR[i] + m * ((useDual ? wetB[(size_t) i] : wetA[(size_t) i]) - dryR[i]);
            m += step;
        }
        lastMix = mixTarget;
        wasActive = want;
        return true;
    }

    // Audio thread: puts the signal through a small delay of 'samples' (0 = none); in place.
    void delayInPlace (float* x, int n, int samples)
    {
        if (samples <= 0 || samples >= (int) delayBuf.size()) return;
        const int mask = (int) delayBuf.size() - 1;
        for (int i = 0; i < n; ++i)
        {
            delayBuf[(size_t) (delayPos & mask)] = x[i];
            x[i] = delayBuf[(size_t) ((delayPos - samples) & mask)];
            ++delayPos;
        }
    }
};
