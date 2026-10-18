#pragma once
// Riff recorder (v52): record a phrase and loop it back.
//   DI mode        : records the guitar at the input and plays it back into the input, so the whole chain
//                    (pedals, amp, cab, effects) processes the playback exactly as if you were playing it.
//   Full tone mode : records the finished sound at the output and plays it back at the output, after everything.
// The recording lives in memory (up to 60 seconds, stereo) and is not saved with the session; use Export to keep it.
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>
#include <juce_audio_formats/juce_audio_formats.h>

class RiffRecorder
{
public:
    enum State { Idle = 0, Recording = 1, Playing = 2 };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        capacity = (int) (60.0 * sampleRate);
        if ((int) data[0].size() != capacity)
            for (auto& d : data) d.assign ((size_t) capacity, 0.0f);
        // a recording made at another rate is no use at the new one
        if (std::abs (recordedRate - sampleRate) > 1.0) { length.store (0); recordedRate = sampleRate; }
        // v94: a second prepare at the same rate (hosts do this when settings change) must not throw away a recording or stop a playing riff
        if (std::abs (preparedRate - sampleRate) > 1.0) { state.store (Idle); pos = 0; }
        preparedRate = sampleRate;
    }

    // ---- transport (message thread)
    void record()  { if (busy.load()) return; state.store (Idle); pos = 0; posPub.store (0); length.store (0); recordedRate = sr; state.store (Recording); }
    void play()    { if (busy.load() || length.load() <= 0) return; state.store (Idle); pos = 0; posPub.store (0); state.store (Playing); }
    void stop()    { if (state.load() == Recording) length.store (std::min (posPub.load(), capacity)); state.store (Idle); }
    void clear()   { state.store (Idle); length.store (0); pos = 0; posPub.store (0); }
    int  getState() const { return state.load(); }
    int  getLength() const { return length.load(); }
    double getSeconds() const { return length.load() / std::max (1.0, sr); }
    double getPositionSeconds() const { return std::min (posPub.load(), capacity) / std::max (1.0, sr); }
    float getPlayhead() const   // 0..1 across the recording (or across the 60 s while recording)
    {
        const int s = state.load();
        const int p = posPub.load();
        if (s == Recording) return (float) p / (float) std::max (1, capacity);
        const int len = length.load();
        return len > 0 ? (float) std::min (p, len) / (float) len : 0.0f;
    }

    // Peak bins for the waveform strip (message thread)
    void getPeaks (float* out, int bins) const
    {
        const int len = length.load();
        for (int b = 0; b < bins; ++b) out[b] = 0.0f;
        if (len <= 0) return;
        for (int b = 0; b < bins; ++b)
        {
            const int a = (int) ((int64_t) len * b / bins), z = std::max (a + 1, (int) ((int64_t) len * (b + 1) / bins));
            float pk = 0.0f;
            const int step = std::max (1, (z - a) / 64);
            for (int i = a; i < z; i += step) pk = std::max (pk, std::abs (data[0][(size_t) i]));
            out[b] = pk;
        }
    }

    // ---- audio thread
    // DI tap: call at the input. Records the block, or replaces / adds the playback.
    void processInput (float* L, float* R, int n, float level, bool muteLive)
    {
        run (L, R, n, level, muteLive, true);
    }
    // Full-tone tap: call at the output. Records the block or adds the playback.
    void processOutput (float* L, float* R, int n, float level)
    {
        run (L, R, n, level, false, false);
    }

    // ---- files (message thread)
    bool exportWav (const juce::File& file, juce::String& message)
    {
        if (length.load() <= 0) { message = "There is nothing recorded yet."; return false; }
        busy.store (true);  stop();
        const int len = length.load();
        juce::WavAudioFormat wav;
        file.deleteFile();
        std::unique_ptr<juce::FileOutputStream> out (file.createOutputStream());
        bool ok = false;
        if (out != nullptr)
        {
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (out.get(), recordedRate, 2, 24, {}, 0));
            if (w != nullptr)
            {
                out.release();
                const float* ch[2] = { data[0].data(), data[1].data() };
                ok = w->writeFromFloatArrays (ch, 2, len);
            }
        }
        busy.store (false);
        message = ok ? "Saved " + file.getFileName() : "Could not write " + file.getFileName();
        return ok;
    }

    bool importWav (const juce::File& file, juce::String& message)
    {
        juce::AudioFormatManager fm;  fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (file));
        if (r == nullptr || r->lengthInSamples <= 0) { message = "Could not read " + file.getFileName(); return false; }
        busy.store (true);  stop();
        const int srcLen = (int) std::min<int64_t> (r->lengthInSamples, (int64_t) (61.0 * r->sampleRate));
        juce::AudioBuffer<float> tmp ((int) std::max<unsigned> (2, r->numChannels), srcLen);
        r->read (&tmp, 0, srcLen, 0, true, true);
        if (r->numChannels == 1) tmp.copyFrom (1, 0, tmp, 0, 0, srcLen);
        const double ratio = r->sampleRate / sr;
        const int dstLen = (int) std::min<double> ((double) capacity, std::floor ((double) srcLen / ratio));
        for (int ch = 0; ch < 2; ++ch)
        {
            const float* s = tmp.getReadPointer (ch);
            for (int i = 0; i < dstLen; ++i)
            {
                const double p = i * ratio;  const int i0 = (int) p;  const float f = (float) (p - i0);
                const float a = s[std::min (i0, srcLen - 1)], b = s[std::min (i0 + 1, srcLen - 1)];
                data[ch][(size_t) i] = a + f * (b - a);
            }
        }
        recordedRate = sr;
        length.store (dstLen);  pos = 0;
        busy.store (false);
        message = "Loaded " + file.getFileName() + " (" + juce::String (dstLen / sr, 1) + " s)";
        return dstLen > 0;
    }

    std::atomic<bool> loop { true };

private:
    void run (float* L, float* R, int n, float level, bool muteLive, bool di)
    {
        runImpl (L, R, n, level, muteLive, di);
        posPub.store (pos, std::memory_order_relaxed);
    }
    void runImpl (float* L, float* R, int n, float level, bool muteLive, bool di)
    {
        const int st = state.load();
        if (st == Idle || busy.load()) return;
        if (st == Recording)
        {
            for (int i = 0; i < n && pos < capacity; ++i, ++pos)
            {
                data[0][(size_t) pos] = L[i];
                data[1][(size_t) pos] = R ? R[i] : L[i];
            }
            if (pos >= capacity) { length.store (capacity); state.store (Idle); }
            return;
        }
        // Playing
        const int len = length.load();
        if (len <= 0) { state.store (Idle); return; }
        const bool looping = loop.load();
        for (int i = 0; i < n; ++i)
        {
            if (pos >= len)
            {
                if (looping) pos = 0;
                else { state.store (Idle); pos = 0; if (di && muteLive) for (int k = i; k < n; ++k) { L[k] = 0.0f; if (R) R[k] = 0.0f; } return; }
            }
            const float a = level * data[0][(size_t) pos], b = level * data[1][(size_t) pos];
            if (di && muteLive) { L[i] = a; if (R) R[i] = a; }       // the guitar is mono: the DI side uses the left channel
            else                { L[i] += a; if (R) R[i] += (di ? a : b); }
            ++pos;
        }
    }

    std::vector<float> data[2];
    std::atomic<int> state { Idle }, length { 0 };
    std::atomic<bool> busy { false };
    int    pos = 0, capacity = 0;
    std::atomic<int> posPub { 0 };
    double sr = 44100.0, recordedRate = 0.0, preparedRate = 0.0;
};
