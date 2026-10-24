#pragma once
#include <vector>
#include <atomic>
#include <cmath>
#include <algorithm>

// A one-button looper that records the finished sound (after the amp, cab and effects) and plays it back
// on top of what you play. One button steps Record -> Play -> Overdub -> Play.
// Commands can come from any thread (the editor, a MIDI footswitch, the host); the audio thread acts on them
// at the start of its next block.
class Looper
{
public:
    enum State { Empty = 0, Armed, Recording, Playing, Overdub, Stopped };
    enum Command { Cycle = 0, Stop, Undo, Clear, kCommands };

    struct Params
    {
        bool  armOnNote = true;        // wait for the first note before recording
        int   bars = 0;                // 0 = free, else the loop closes by itself after this many bars
        int   beatsPerBar = 4;
        float level = 1.0f;            // playback gain
        double bpm = 120.0;
    };

    static constexpr double kMaxSeconds = 40.0;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        maxLen = (int) (kMaxSeconds * sampleRate);
        for (int c = 0; c < 2; ++c)
        {
            loop[c].assign ((size_t) maxLen, 0.0f);
            undoBuf[c].assign ((size_t) maxLen, 0.0f);
        }
        hardClear();
    }

    void command (int c) { if (c >= 0 && c < kCommands) ++issued[c]; }

    int   getState() const        { return state.load (std::memory_order_relaxed); }
    float getPosition() const     { const int n = len.load(); return n > 0 ? (float) playPos.load() / (float) n : 0.0f; }
    float getSeconds() const      { return sr > 0 ? (float) len.load() / (float) sr : 0.0f; }
    float getRecordedSeconds() const { return sr > 0 ? (float) recPos.load() / (float) sr : 0.0f; }
    bool  canUndo() const         { return undoReady.load(); }

    void process (float* l, float* r, int n, const Params& p, float inputPeak)
    {
        if (maxLen <= 0) return;
        // ---- commands
        for (int c = 0; c < kCommands; ++c)
            while (handled[c] != issued[c].load())
            {
                ++handled[c];
                run (c, p);
            }

        if (state.load() == Armed && inputPeak > 0.01f)       // about -40 dBFS at the input
            beginRecording (p);

        const float fadeStep = 1.0f / 96.0f;
        for (int i = 0; i < n; ++i)
        {
            const int st = state.load (std::memory_order_relaxed);
            const float dl = l[i], dr = r != nullptr ? r[i] : l[i];
            if (st == Recording)
            {
                loop[0][(size_t) recIdx] = dl;  loop[1][(size_t) recIdx] = dr;
                ++recIdx;
                if (recIdx >= maxLen || (targetLen > 0 && recIdx >= targetLen))
                    closeLoop (recIdx >= maxLen ? maxLen : targetLen, Playing);
                recPos.store (recIdx, std::memory_order_relaxed);
            }
            else if (st == Playing || st == Overdub || odGain > 0.0f)
            {
                const int length = len.load (std::memory_order_relaxed);
                if (length > 0)
                {
                    const size_t k = (size_t) pos;
                    const float yl = loop[0][k], yr = loop[1][k];
                    const float target = (st == Overdub) ? 1.0f : 0.0f;
                    odGain += lim (-fadeStep, fadeStep, target - odGain);
                    if (odGain > 0.0f)
                    {
                        if (ovCount < length)
                        {
                            undoBuf[0][k] = yl;  undoBuf[1][k] = yr;
                            ++ovCount;
                        }
                        loop[0][k] = yl + dl * odGain;
                        loop[1][k] = yr + dr * odGain;
                    }
                    if (st == Playing || st == Overdub)
                    {
                        l[i] = dl + yl * gain;
                        if (r != nullptr) r[i] = dr + yr * gain;
                    }
                    if (++pos >= length) pos = 0;
                }
            }
            gain += (p.level - gain) * 0.002f;
        }
        playPos.store (pos, std::memory_order_relaxed);
        if (odGain <= 0.0f && ovPending)
        {
            ovPending = false;
            undoReady.store (ovCount > 0);
            undone = false;
        }
    }

private:
    static float lim (float lo, float hi, float v) { return v < lo ? lo : (v > hi ? hi : v); }

    void hardClear()
    {
        state.store (Empty);  len.store (0);  recPos.store (0);  playPos.store (0);
        pos = 0;  recIdx = 0;  targetLen = 0;  odGain = 0.0f;  ovCount = 0;  ovStart = 0;
        ovPending = false;  undoReady.store (false);  undone = false;
        for (int c = 0; c < kCommands; ++c) handled[c] = issued[c].load();
    }

    void beginRecording (const Params& p)
    {
        recIdx = 0;
        targetLen = 0;
        if (p.bars > 0)
            targetLen = std::min (maxLen, (int) std::lround ((double) p.bars * p.beatsPerBar * 60.0 / std::max (20.0, p.bpm) * sr));
        recPos.store (0);
        undoReady.store (false);
        state.store (Recording);
    }

    void closeLoop (int length, int nextState)
    {
        if (length < (int) (0.05 * sr))
        {
            hardClear();                                       // too short to be a loop
            return;
        }
        const int f = std::min (96, length / 4);               // short fades so the join does not click
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < f; ++i)
            {
                const float g = (float) i / (float) f;
                loop[c][(size_t) i] *= g;
                loop[c][(size_t) (length - 1 - i)] *= g;
            }
        len.store (length);
        pos = 0;  odGain = 0.0f;  ovCount = 0;  ovPending = false;
        undoReady.store (false);
        state.store (nextState);
    }

    void endOverdub()
    {
        // the fade-out of the overdub gain finishes in the next samples; the session closes when it reaches zero
        ovPending = true;
    }

    void swapUndo()
    {
        const int length = len.load();
        if (length <= 0 || ovCount <= 0) return;
        int k = ovStart;
        for (int i = 0; i < ovCount && i < length; ++i)
        {
            std::swap (loop[0][(size_t) k], undoBuf[0][(size_t) k]);
            std::swap (loop[1][(size_t) k], undoBuf[1][(size_t) k]);
            if (++k >= length) k = 0;
        }
        undone = ! undone;
    }

    void run (int c, const Params& p)
    {
        const int st = state.load();
        switch (c)
        {
            case Cycle:
                if (st == Empty)          { if (p.armOnNote) state.store (Armed); else beginRecording (p); }
                else if (st == Armed)     beginRecording (p);
                else if (st == Recording) closeLoop (recIdx, Playing);
                else if (st == Playing)   { ovStart = pos;  ovCount = 0;  ovPending = false;  undoReady.store (false);  state.store (Overdub); }
                else if (st == Overdub)   { state.store (Playing);  endOverdub(); }
                else if (st == Stopped)   { pos = 0;  state.store (Playing); }
                break;
            case Stop:
                if (st == Armed)          hardClear();
                else if (st == Recording) closeLoop (recIdx, Stopped);
                else if (st == Playing || st == Overdub)
                {
                    if (st == Overdub) { odGain = 0.0f;  endOverdub(); }
                    state.store (Stopped);  pos = 0;
                }
                break;
            case Undo:
                if (st == Armed || st == Recording) { hardClear(); break; }
                if (st == Overdub) { state.store (Playing);  endOverdub(); }
                if (undoReady.load() || ovPending)
                {
                    if (ovPending) { odGain = 0.0f; ovPending = false; undoReady.store (ovCount > 0); }
                    swapUndo();
                }
                else if (st != Empty)
                    hardClear();                                // nothing to undo but the first take: remove it
                break;
            case Clear:
                hardClear();
                break;
        }
    }

    double sr = 48000.0;
    int maxLen = 0;
    std::vector<float> loop[2], undoBuf[2];
    std::atomic<int> state { Empty }, len { 0 }, recPos { 0 }, playPos { 0 };
    std::atomic<bool> undoReady { false };
    std::atomic<int> issued[kCommands] {};
    int handled[kCommands] = {};
    int pos = 0, recIdx = 0, targetLen = 0, ovCount = 0, ovStart = 0;
    float odGain = 0.0f, gain = 1.0f;
    bool ovPending = false, undone = false;
};
