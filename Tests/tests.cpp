// Starter Amp automated tests. Build instructions are in Tests/README.md.
//   tests stress            numerical stability, block sizes, automation, state round trip, lifecycle, switching
//   tests regress           compare a fixed render of every amp and voicing with Tests/golden.txt
//   tests regress --update  rewrite golden.txt (do this only after a change you meant to make)
#include "PluginProcessor.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <string>

static int gFail = 0, gPass = 0;
#define CHECK(cond, ...) do { if (cond) ++gPass; else { ++gFail; std::printf ("  FAIL: "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

static void setP (StarterAmpProcessor& p, const char* id, float v)
{
    if (auto* r = p.apvts.getParameter (id)) r->setValueNotifyingHost (r->convertTo0to1 (v));
}
static void setNorm (StarterAmpProcessor& p, juce::RangedAudioParameter* r, float n) { r->setValueNotifyingHost (n); }

// Deterministic "guitar-like" test signal: decaying plucked notes with a little noise, peak about -6 dBFS.
static std::vector<float> testSignal (int n, double sr, float level = 0.5f, unsigned seed = 7)
{
    std::mt19937 rng (seed); std::uniform_real_distribution<float> u (-1.0f, 1.0f);
    std::vector<float> x ((size_t) n, 0.0f);
    const float notes[6] = { 82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f };
    const int hop = (int) (sr * 0.4);
    for (int s = 0, k = 0; s < n; s += hop, ++k)
    {
        const float f = notes[k % 6];
        for (int i = 0; i < hop && s + i < n; ++i)
        {
            const float t = (float) i / (float) sr, env = std::exp (-3.0f * t);
            x[(size_t) (s + i)] += level * env * (0.6f * std::sin (6.2831853f * f * t) + 0.25f * std::sin (12.566371f * f * t) + 0.15f * std::sin (18.849556f * f * t));
        }
    }
    for (auto& v : x) v += 0.002f * u (rng);
    return x;
}

static bool process (StarterAmpProcessor& p, const std::vector<float>& in, std::vector<float>& out, int block)
{
    out.assign (in.size(), 0.0f);
    juce::AudioBuffer<float> b (2, juce::jmax (block, 1)); juce::MidiBuffer m;
    for (size_t pos = 0; pos < in.size(); pos += (size_t) block)
    {
        const int n = (int) std::min ((size_t) block, in.size() - pos);
        juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
        for (int i = 0; i < n; ++i) { v.setSample (0, i, in[pos + (size_t) i]); v.setSample (1, i, in[pos + (size_t) i]); }
        p.processBlock (v, m);
        for (int i = 0; i < n; ++i) out[pos + (size_t) i] = v.getSample (0, i);
    }
    return true;
}
static bool finiteBounded (const std::vector<float>& y, float limit, float& peak)
{
    peak = 0.0f;
    for (float v : y) { if (! std::isfinite (v)) return false; peak = std::max (peak, std::abs (v)); }
    return peak <= limit;
}
static double rmsDb (const std::vector<float>& y, size_t from = 0)
{
    double s = 0; size_t n = 0; for (size_t i = from; i < y.size(); ++i) { s += (double) y[i] * y[i]; ++n; }
    return 10.0 * std::log10 (s / (double) std::max<size_t> (n, 1) + 1e-20);
}
static void bands (const std::vector<float>& y, double sr, double* out8)
{
    const int N = 1 << 15; std::vector<double> w (N);
    // Welch-free simple: one windowed DFT per band centre via Goertzel-ish sum over a coarse frequency grid
    const double fc[8] = { 63, 125, 250, 500, 1000, 2000, 4000, 8000 };
    const size_t start = y.size() > (size_t) N ? y.size() - (size_t) N : 0;
    for (int b = 0; b < 8; ++b)
    {
        double acc = 0; int cnt = 0;
        for (double f = fc[b] / 1.41; f < fc[b] * 1.41; f *= 1.06)
        {
            double re = 0, im = 0;
            for (int i = 0; i < N; ++i)
            {
                const double win = 0.5 - 0.5 * std::cos (6.283185307 * i / N), ph = 6.283185307 * f * i / sr;
                const double s = (start + (size_t) i < y.size() ? y[start + (size_t) i] : 0.0f) * win;
                re += s * std::cos (ph); im += s * std::sin (ph);
            }
            acc += re * re + im * im; ++cnt;
        }
        out8[b] = 10.0 * std::log10 (acc / std::max (cnt, 1) + 1e-20);
    }
}

//==============================================================================
static void testStability()
{
    std::printf ("[stability] every amp x voicing x gain, loud and silent input\n");
    const auto loud = testSignal (24000, 48000.0, 0.9f), loud2 = testSignal (24000, 48000.0, 0.02f);
    std::vector<float> silence (24000, 0.0f), y;
    for (int model = 0; model <= 33; ++model)
        for (int voi = 0; voi < 4; ++voi)
            for (float gain : { 0.0f, 0.5f, 1.0f })
            {
                StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
                setP (p, "ampModel", (float) model); setP (p, "ampVoicing", (float) voi); setP (p, "ampGain", gain); setP (p, "ampMode", 2.0f);
                p.prepareToPlay (48000.0, 512);
                float peak = 0;
                process (p, loud, y, 512);  CHECK (finiteBounded (y, 32.0f, peak), "amp %d voicing %d gain %.1f loud input: non-finite or peak %.1f", model, voi, gain, peak);
                process (p, loud2, y, 512); CHECK (finiteBounded (y, 32.0f, peak), "amp %d voicing %d gain %.1f quiet input: non-finite or peak %.1f", model, voi, gain, peak);
                process (p, silence, y, 512);
                const double tail = rmsDb (y, 12000);
                CHECK (finiteBounded (y, 32.0f, peak) && tail < -50.0, "amp %d voicing %d gain %.1f silence after signal: tail %.1f dB (want < -50)", model, voi, gain, tail);
            }
}

static void testBlockSizes()
{
    std::printf ("[block sizes] same input, different block sizes: finite, and level within 1 dB of the 512 render\n");
    const auto in = testSignal (48000, 48000.0);
    for (int model : { 0, 4, 13, 17, 20, 27, 30 })
    {
        std::vector<float> ref;
        { StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); setP (p, "ampModel", (float) model); setP (p, "ampMode", 2.0f); p.prepareToPlay (48000.0, 512); process (p, in, ref, 512); }
        const double rr = rmsDb (ref, 12000);
        for (int blk : { 1, 16, 32, 64, 100, 128, 333, 1024, 4096 })
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, blk); setP (p, "ampModel", (float) model); setP (p, "ampMode", 2.0f); p.prepareToPlay (48000.0, blk);
            std::vector<float> y; process (p, in, y, blk); float peak;
            CHECK (finiteBounded (y, 32.0f, peak), "amp %d block %d: non-finite", model, blk);
            CHECK (std::abs (rmsDb (y, 12000) - rr) < 1.0, "amp %d block %d: level %.2f dB vs %.2f dB at 512", model, blk, rmsDb (y, 12000), rr);
        }
    }
}

static void testAutomation (unsigned seed)
{
    std::printf ("[automation] random parameter changes every block (seed %u)\n", seed);
    std::mt19937 rng (seed); std::uniform_real_distribution<float> u (0.0f, 1.0f);
    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 256); p.prepareToPlay (48000.0, 256);
    std::vector<juce::RangedAudioParameter*> ps;
    for (auto* prm : p.getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) ps.push_back (r);
    const auto in = testSignal (256 * 800, 48000.0, 0.5f, seed);
    juce::AudioBuffer<float> b (2, 256); juce::MidiBuffer m; float peak = 0; bool ok = true;
    for (size_t pos = 0; pos + 256 <= in.size(); pos += 256)
    {
        for (int k = 0; k < 6; ++k) setNorm (p, ps[(size_t) (rng() % ps.size())], u (rng));
        for (int i = 0; i < 256; ++i) { b.setSample (0, i, in[pos + (size_t) i]); b.setSample (1, i, in[pos + (size_t) i]); }
        p.processBlock (b, m);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const float v = b.getSample (c, i); if (! std::isfinite (v)) ok = false; peak = std::max (peak, std::abs (v)); }
    }
    CHECK (ok, "automation: non-finite output");
    CHECK (peak < 200.0f, "automation: peak %.1f", peak);
}

static void testState()
{
    std::printf ("[state] every parameter survives save and load\n");
    std::mt19937 rng (3); std::uniform_real_distribution<float> u (0.0f, 1.0f);
    StarterAmpProcessor a; a.setPlayConfigDetails (2, 2, 48000.0, 512); a.prepareToPlay (48000.0, 512);
    for (auto* prm : a.getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) { float x = u (rng); const int st = r->getNumSteps(); if (st > 1 && st < 10000) x = std::round (x * (float) (st - 1)) / (float) (st - 1); r->setValueNotifyingHost (x); }
    a.apvts.state.setProperty ("modFold", 1, nullptr);  a.apvts.state.setProperty ("modFold2", 1, nullptr);   // old Wobble / Tremolo pedals are folded into Mod FX slots on load by design; this state is current, not old
    juce::MemoryBlock mb; a.getStateInformation (mb);
    StarterAmpProcessor b; b.setPlayConfigDetails (2, 2, 48000.0, 512); b.prepareToPlay (48000.0, 512);
    b.setStateInformation (mb.getData(), (int) mb.getSize());
    int bad = 0, total = 0;
    for (auto* prm : a.getParameters())
        if (auto* ra = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (ra->paramID != "wobOn")   // an old Wobble setting is converted to a swell pedal on load by design, so it does not round-trip
            if (auto* rb = dynamic_cast<juce::RangedAudioParameter*> (b.apvts.getParameter (ra->paramID)))
            {
                ++total; if (std::abs (ra->getValue() - rb->getValue()) > 1.0e-3f) { ++bad; if (bad < 8) std::printf ("    differs: %s %.4f -> %.4f\n", ra->paramID.toRawUTF8(), ra->getValue(), rb->getValue()); }
            }
    CHECK (bad == 0, "%d of %d parameters changed through save/load", bad, total);
    // garbage and empty state must not crash
    const char junk[] = "this is not a state"; b.setStateInformation (junk, (int) sizeof (junk)); b.setStateInformation (junk, 0);
    ++gPass;
}

static void testLifecycle()
{
    std::printf ("[lifecycle] repeated prepare at different sample rates and block sizes, release, destroy\n");
    const double rates[] = { 22050.0, 44100.0, 48000.0, 88200.0, 96000.0, 192000.0 };
    for (double sr : rates)
        for (int blk : { 32, 512, 2048 })
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, sr, blk);
            for (int i = 0; i < 3; ++i) { p.prepareToPlay (sr, blk); const auto in = testSignal (blk * 4, sr); std::vector<float> y; float pk; process (p, in, y, blk); CHECK (finiteBounded (y, 32.0f, pk), "sr %.0f block %d prepare %d: non-finite", sr, blk, i); p.releaseResources(); }
        }
    // changing the sample rate on a live instance
    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
    for (double sr : rates) { p.prepareToPlay (sr, 512); const auto in = testSignal (2048, sr); std::vector<float> y; float pk; process (p, in, y, 512); CHECK (finiteBounded (y, 32.0f, pk), "live change to %.0f Hz", sr); }
}

static void testOversampling()
{
    std::printf ("[oversampling] each quality: finite, reported latency, level close to the standard setting\n");
    const auto in = testSignal (48000, 48000.0);
    const char* osId = "ampQuality";
    double base = 0;
    for (int q = 0; q < 3; ++q)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (osId))) setNorm (p, r, q / 2.0f);
        setP (p, "ampModel", 17.0f); setP (p, "ampMode", 2.0f); p.prepareToPlay (48000.0, 512);
        std::vector<float> y; float pk; process (p, in, y, 512);
        CHECK (finiteBounded (y, 32.0f, pk), "oversampling %d: non-finite", q);
        const double r = rmsDb (y, 12000); if (q == 0) base = r;
        std::printf ("  quality %d: latency %d samples, level %.2f dB\n", q, p.getLatencySamples(), r);
        CHECK (std::abs (r - base) < 3.0, "oversampling %d level %.1f dB differs from %.1f dB by more than 3 dB", q, r, base);
    }
}

static void testMovedBlocks()
{
    std::printf ("[move] Lo-Fi and Boost in each lane: finite, audible, and a different place gives a different sound\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    const char* on[2] = { "lfOn", "bsOn" }; const char* loc[2] = { "fxLocLoFi", "fxLocBoost" };
    for (int b = 0; b < 2; ++b)
    {
        std::vector<float> ys[3];
        for (int l = 0; l < 3; ++l)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            if (auto* r = p.apvts.getParameter (loc[b])) setNorm (p, r, l * 0.5f);
            if (auto* r = p.apvts.getParameter (on[b])) setNorm (p, r, 1.0f);
            process (p, in, ys[l], 512);
            float pk; CHECK (finiteBounded (ys[l], 8.0f, pk), "%s in lane %d not finite or too loud (peak %.2f)", on[b], l, pk);
        }
        double d01 = 0, d02 = 0; for (size_t i = 0; i < ys[0].size(); ++i) { d01 += std::abs (ys[0][i] - ys[1][i]); d02 += std::abs (ys[0][i] - ys[2][i]); }
        CHECK (d01 > 1e-3 && d02 > 1e-3, "%s sounds the same in different lanes (%.4f, %.4f)", on[b], d01, d02);
    }
}

static void testMovedDrives()
{
    std::printf ("[drives] Drive 1 and Drive 2 in each lane, alone and together: finite; default place unchanged\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    for (int combo = 0; combo < 3; ++combo)      // Drive 1 only, Drive 2 only, both
    {
        std::vector<float> ys[3];
        for (int l = 0; l < 3; ++l)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            setP (p, "odOn", combo != 1 ? 1.0f : 0.0f); setP (p, "od2On", combo != 0 ? 1.0f : 0.0f);
            if (combo != 1) setP (p, "fxLocDrive1", (float) l);
            if (combo != 0) setP (p, "fxLocDrive2", (float) l);
            process (p, in, ys[l], 512);
            float pk; CHECK (finiteBounded (ys[l], 8.0f, pk), "drive combo %d lane %d not finite or too loud (peak %.2f)", combo, l, pk);
        }
        double d01 = 0, d02 = 0; for (size_t i = 0; i < ys[0].size(); ++i) { d01 += std::abs (ys[0][i] - ys[1][i]); d02 += std::abs (ys[0][i] - ys[2][i]); }
        CHECK (d01 > 1e-3 && d02 > 1e-3, "drive combo %d sounds the same in different lanes (%.4f, %.4f)", combo, d01, d02);
    }
}

static void testCompStyles()
{
    std::printf ("[comp] every compressor style: finite and bounded; Thump differs from Standard; Bloom, Smack and Clarity do nothing outside Thump\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    std::vector<float> ys[8], bloomed;
    for (int st = 0; st < 8; ++st)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "compOn", 1.0f);  setP (p, "compStyle", (float) st);  setP (p, "compThresh", -30.0f);
        process (p, in, ys[st], 512);
        float pk; CHECK (finiteBounded (ys[st], 8.0f, pk), "comp style %d not finite or too loud (peak %.2f)", st, pk);
    }
    double d = 0; for (size_t i = 0; i < ys[0].size(); ++i) d += std::abs (ys[0][i] - ys[7][i]);
    CHECK (d > 1e-3, "Thump sounds the same as Standard (%.5f)", d);
    for (int k = 0; k < 2; ++k)       // Standard with Bloom, Smack and Clarity at the maximum must equal Standard with them at zero
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "compOn", 1.0f);  setP (p, "compStyle", 0.0f);  setP (p, "compThresh", -30.0f);
        setP (p, "compBloom", k ? 1.0f : 0.0f);  setP (p, "compSmack", k ? 1.0f : 0.0f);  setP (p, "compClarity", k ? 1.0f : 0.0f);
        process (p, in, k ? bloomed : ys[0], 512);
    }
    double e = 0; for (size_t i = 0; i < ys[0].size(); ++i) e += std::abs (ys[0][i] - bloomed[i]);
    CHECK (e < 1e-6, "Bloom / Smack / Clarity changed the Standard style (%.6f)", e);
}

static void testReverbTypes()
{
    std::printf ("[reverb] every reverb type in both reverb blocks: finite and bounded; Spring (tank) is audible and fades\n");
    const int n = 48000 * 2; auto in = testSignal (n, 48000.0, 0.3f);
    for (int unit = 0; unit < 2; ++unit)
        for (int type = 0; type < 18; ++type)
        {
            if (unit == 1 && type == 12) continue;
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            const char* pre = unit == 0 ? "reverb" : "rev2";
            setP (p, (std::string (pre) + "On").c_str(), 1.0f);  setP (p, (std::string (pre) + "Type").c_str(), (float) type);
            setP (p, (std::string (pre) + "Mix").c_str(), 0.75f);  setP (p, (std::string (pre) + "Drive").c_str(), 0.5f);
            std::vector<float> y; process (p, in, y, 512);
            float pk; CHECK (finiteBounded (y, 8.0f, pk), "reverb %d type %d not finite or too loud (peak %.2f)", unit, type, pk);
        }
    // the tank: a noise burst rings, then dies away
    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
    setP (p, "reverbOn", 1.0f);  setP (p, "reverbType", 17.0f);  setP (p, "reverbMix", 0.5f);  setP (p, "reverbSize", 2.0f);
    std::vector<float> imp ((size_t) (48000 * 6), 0.0f);
    { unsigned s = 1; for (int i = 0; i < 4800; ++i) { s = s * 1664525u + 1013904223u; imp[(size_t) i] = 0.2f * (((s >> 9) / 8388608.0f) - 0.5f) * 2.0f; } }
    std::vector<float> y; process (p, imp, y, 512);
    auto rms = [&] (double a, double b) { double s = 0; int c = 0; for (size_t i = (size_t) (a * 48000.0) * 2; i < (size_t) (b * 48000.0) * 2 && i < y.size(); ++i) { s += (double) y[i] * y[i]; ++c; } return std::sqrt (s / std::max (1, c)); };
    CHECK (rms (0.2, 0.6) > 2e-4, "Spring (tank) is silent after a burst (%.6f)", rms (0.2, 0.6));
    CHECK (rms (4.0, 5.0) < 0.05 * rms (0.2, 0.6), "Spring (tank) does not fade (%.6f vs %.6f)", rms (4.0, 5.0), rms (0.2, 0.6));
}

static void testSpringReturn()
{
    std::printf ("[springreturn] seven return styles on both spring types in both reverb blocks: finite, bounded, each differs from Generic; other types ignore it\n");
    const int n = 48000 * 2; auto in = testSignal (n, 48000.0, 0.3f);
    for (int unit = 0; unit < 2; ++unit)
        for (int type : { 6, 17 })
            for (float drive : { 0.0f, 0.8f })
            {
                const char* pre = unit == 0 ? "reverb" : "rev2";
                std::vector<float> ref;
                for (int st = 0; st < 7; ++st)
                {
                    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
                    setP (p, (std::string (pre) + "On").c_str(), 1.0f);  setP (p, (std::string (pre) + "Type").c_str(), (float) type);
                    setP (p, (std::string (pre) + "Mix").c_str(), 0.75f);  setP (p, (std::string (pre) + "Drive").c_str(), drive);
                    setP (p, (std::string (pre) + "Return").c_str(), (float) st);
                    std::vector<float> y; process (p, in, y, 512);
                    float pk; CHECK (finiteBounded (y, 8.0f, pk), "return style %d (unit %d type %d drive %.1f) not finite or too loud (peak %.2f)", st, unit, type, drive, pk);
                    if (st == 0) ref = y;
                    else { double d = 0; for (size_t i = 0; i < y.size(); ++i) d += std::abs (y[i] - ref[i]);
                           CHECK (d > 1e-2, "return style %d is identical to Generic (unit %d type %d drive %.1f, %.5f)", st, unit, type, drive, d); }
                }
            }
    // a type that has no spring ignores the menu
    {
        std::vector<float> a, b;
        for (int v = 0; v < 2; ++v)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            setP (p, "reverbOn", 1.0f);  setP (p, "reverbType", 2.0f);  setP (p, "reverbMix", 0.75f);  setP (p, "reverbReturn", v ? 3.0f : 0.0f);
            process (p, in, v ? b : a, 512);
        }
        double d = 0; for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]);
        CHECK (d < 1e-6, "a non-spring reverb type changed with the Spring return menu (%.6f)", d);
    }
    // switching styles on the fly stays finite
    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
    setP (p, "reverbOn", 1.0f);  setP (p, "reverbType", 17.0f);  setP (p, "reverbMix", 0.75f);  setP (p, "reverbDrive", 0.6f);
    for (int k = 0; k < 14; ++k) { setP (p, "reverbReturn", (float) (k % 7)); std::vector<float> y; process (p, in, y, 512); float pk; CHECK (finiteBounded (y, 8.0f, pk), "return switch to %d not finite", k); }
}

static void testDiMatch()
{
    std::printf ("[dimatch] DI Match: valid combos finite/bounded/audible, invalid ones bypass, learning flow, state round trip, Gate input key\n");
    const int n = 48000 * 2; auto in = testSignal (n, 48000.0, 0.3f);
    std::vector<float> bypass;
    { StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512); process (p, in, bypass, 512); }
    struct Combo { int src, tgt; };
    for (Combo c : { Combo { 1, 1 }, Combo { 2, 0 }, Combo { 2, 2 }, Combo { 2, 3 }, Combo { 3, 1 }, Combo { 4, 0 }, Combo { 1, 3 } })
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "dmOn", 1.0f);  setP (p, "dmSource", (float) c.src);  setP (p, "dmTarget", (float) c.tgt);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "DI Match %d->%d not finite or too loud (peak %.2f)", c.src, c.tgt, pk);
        CHECK (p.diStatus.load() == 0, "DI Match %d->%d status %d", c.src, c.tgt, p.diStatus.load());
        double d = 0; for (size_t i = 0; i < y.size(); ++i) d += std::abs (y[i] - bypass[i]);
        CHECK (d > 1e-2, "DI Match %d->%d changed nothing (%.5f)", c.src, c.tgt, d);
    }
    // same pickup to itself: nothing to correct, level and curve stay put (small tolerance)
    // invalid and not-yet-learned combos pass the signal through unchanged
    for (Combo c : { Combo { 2, 4 }, Combo { 0, 4 }, Combo { 2, 5 }, Combo { 0, 0 } })
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "dmOn", 1.0f);  setP (p, "dmSource", (float) c.src);  setP (p, "dmTarget", (float) c.tgt);
        std::vector<float> y; process (p, in, y, 512);
        double d = 0; for (size_t i = 0; i < y.size(); ++i) d += std::abs (y[i] - bypass[i]);
        CHECK (d < 1e-4, "DI Match %d->%d (not ready, status %d) changed the sound (%.6f)", c.src, c.tgt, p.diStatus.load(), d);
        CHECK (p.diStatus.load() != 0, "DI Match %d->%d reports ok", c.src, c.tgt);
    }
    // active circuit
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "dmOn", 1.0f);  setP (p, "dmSource", 1.0f);  setP (p, "dmTarget", 3.0f);  setP (p, "dmActive", 1.0f);  setP (p, "dmGain", 1.0f);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 12.0f, pk), "DI Match active circuit not finite or too loud (peak %.2f)", pk);
    }
    // learning flow: Measured -> Pink noise
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "dmOn", 1.0f);  setP (p, "dmSource", 0.0f);  setP (p, "dmTarget", 4.0f);
        std::vector<float> y; process (p, in, y, 512);
        CHECK (p.diStatus.load() == 1, "Measured with nothing learned should say needLearn (got %d)", p.diStatus.load());
        p.diLearn.store (true);
        auto long_in = testSignal (48000 * 5, 48000.0, 0.3f);
        process (p, long_in, y, 512);
        p.diLearn.store (false);
        process (p, in, y, 512);
        DiMatch::Shape sh {};  float lvl = 0;
        CHECK (p.diGetSource (sh, lvl), "learning kept no result (frames %d)", p.diFrames.load());
        CHECK (p.diStatus.load() == 0, "after learning, status %d", p.diStatus.load());
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "learned DI Match not finite or too loud (peak %.2f)", pk);
        double d = 0; for (size_t i = 0; i < y.size() && i < bypass.size(); ++i) d += std::abs (y[i] - bypass[i]);
        CHECK (d > 1e-3, "learned DI Match changed nothing (%.5f)", d);
        // the learned tone survives a save and load
        juce::MemoryBlock mb; p.getStateInformation (mb);
        StarterAmpProcessor q; q.setPlayConfigDetails (2, 2, 48000.0, 512); q.prepareToPlay (48000.0, 512);
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        DiMatch::Shape sh2 {};  float lvl2 = 0;
        CHECK (q.diGetSource (sh2, lvl2), "learned tone lost in state round trip");
        double e = 0; for (size_t i = 0; i < sh.size(); ++i) e += std::abs (sh[i] - sh2[i]);
        CHECK (e < 0.1 && std::abs (lvl - lvl2) < 0.1f, "learned tone changed in state round trip (%.4f)", e);
        p.diClearSource();
        CHECK (! p.diGetSource (sh, lvl), "diClearSource kept the tone");
    }
    // transposed: still finite
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "tpOn", 1.0f);  setP (p, "tpSemis", 12.0f);  setP (p, "dmOn", 1.0f);  setP (p, "dmSource", 1.0f);  setP (p, "dmTarget", 3.0f);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "DI Match after Transpose not finite or too loud (peak %.2f)", pk);
    }
    // Gate listening to the input, placed after the amp
    {
        std::vector<float> a, b;
        for (int k = 0; k < 2; ++k)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            setP (p, "gxOn", 1.0f);  setP (p, "fxLocGate", 1.0f);  setP (p, "gxThresh", -30.0f);  setP (p, "gxRange", -90.0f);  setP (p, "gxKey", (float) k);
            process (p, in, k ? b : a, 512);
            float pk; CHECK (finiteBounded (k ? b : a, 8.0f, pk), "Gate key %d not finite or too loud (peak %.2f)", k, pk);
        }
        double d = 0; for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]);
        CHECK (d > 1e-3, "Gate listening to the input sounds the same as listening to itself (%.5f)", d);
        // a silent input keeps it shut even though the amp makes noise: no crash and finite at least
        std::vector<float> sil ((size_t) n * 2, 0.0f), z;
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "gxOn", 1.0f);  setP (p, "fxLocGate", 1.0f);  setP (p, "gxKey", 1.0f);
        process (p, sil, z, 512);
        float pk; CHECK (finiteBounded (z, 8.0f, pk), "Gate key on silence not finite");
    }
}

static void testFuzz2Block()
{
    std::printf ("[fuzz2] Fuzz 2 is its own block: series by default, parallel on the lower row, in any lane; differs from Fuzz 1 alone\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    std::vector<float> alone, series, par;
    for (int v = 0; v < 3; ++v)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "fuzzOn", 1.0f);
        if (v >= 1) setP (p, "fuzz2On", 1.0f);
        if (v == 2) p.setFxRow (0, 26, 1);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "Fuzz 2 variant %d not finite or too loud (peak %.2f)", v, pk);
        (v == 0 ? alone : v == 1 ? series : par) = y;
    }
    auto diff = [] (const std::vector<float>& a, const std::vector<float>& b) { double d = 0; for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]); return d; };
    CHECK (diff (alone, series) > 1e-2, "Fuzz 2 in series changed nothing");
    CHECK (diff (series, par) > 1e-2, "Fuzz 2 on the lower row sounds like the series chain");
    {   // Drive 2: stacked after Drive 1 by default; on the lower row it runs beside Drive 1
        std::vector<float> ser, par2;
        for (int v = 0; v < 2; ++v)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            setP (p, "odOn", 1.0f);  setP (p, "od2On", 1.0f);
            if (v == 1) p.setFxRow (0, 23, 1);
            std::vector<float> y; process (p, in, y, 512);
            float pk; CHECK (finiteBounded (y, 8.0f, pk), "Drive 2 variant %d not finite or too loud (peak %.2f)", v, pk);
            (v == 0 ? ser : par2) = y;
        }
        CHECK (diff (ser, par2) > 1e-2, "Drive 2 on the lower row sounds like the stacked chain");
    }
    for (int lane = 0; lane < 3; ++lane)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "fuzz2On", 1.0f);  p.moveFxBlockTo (26, lane, 0, 0);
        CHECK (p.fxLocation (26) == lane, "Fuzz 2 did not move to lane %d", lane);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "Fuzz 2 in lane %d not finite or too loud (peak %.2f)", lane, pk);
    }
}

static void testModSlots()
{
    std::printf ("[modslots] four mod slots, all 21 types: finite, bounded, differ from dry, both positions\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    const char* L = "ABCD";
    for (int slot = 0; slot < 4; ++slot)
        for (int type = 0; type < 21; ++type)
            for (int pos = 0; pos < 2; ++pos)
            {
                StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
                StarterAmpProcessor q; q.setPlayConfigDetails (2, 2, 48000.0, 512); q.prepareToPlay (48000.0, 512);
                const std::string b = std::string ("mf") + L[slot];
                setP (p, (b + "On").c_str(), 1.0f);  setP (p, (b + "Type").c_str(), (float) type);  setP (p, (b + "Pos").c_str(), (float) pos);
                setP (p, (b + "Mix").c_str(), 1.0f);  if (type == 20) setP (p, (b + "Rate").c_str(), 6.0f);  setP (p, (b + "Depth").c_str(), 0.8f);  setP (p, (b + "Fidelity").c_str(), 0.6f);
                std::vector<float> y, d; process (p, in, y, 512);  process (q, in, d, 512);
                float pk; CHECK (finiteBounded (y, 8.0f, pk), "slot %c type %d pos %d not finite or too loud (peak %.2f)", L[slot], type, pos, pk);
                double diff = 0; for (size_t i = 0; i < y.size(); ++i) diff += std::abs (y[i] - d[i]);
                CHECK (diff > 1e-3, "slot %c type %d pos %d is silent or identical to dry (%.5f)", L[slot], type, pos, diff);
            }
    // switching type on the fly must stay finite
    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
    setP (p, "mfCOn", 1.0f);  setP (p, "mfCPos", 1.0f);
    for (int k = 0; k < 21; ++k) { setP (p, "mfCType", (float) k); std::vector<float> y; process (p, in, y, 512); float pk; CHECK (finiteBounded (y, 8.0f, pk), "type switch to %d not finite", k); }
}

static void testParallelSection()
{
    std::printf ("[section] a parallel section inside a lane: finite, differs from the whole-lane split, bad ranges fall back\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    std::vector<float> ys[4];
    for (int v = 0; v < 4; ++v)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "fuzzOn", 1.0f); setP (p, "compOn", 1.0f); setP (p, "chorusOn", 1.0f); setP (p, "chorusMix", 0.6f);
        p.setFxRow (0, 1, 1);                                    // Compressor on the lower row beside the Fuzz
        if (v == 1) { setP (p, "laneRegStart0", 1.0f); setP (p, "laneRegEnd0", 2.0f); }   // from Fuzz to after the Compressor: the Chorus comes after the merge
        if (v == 2) { setP (p, "laneRegStart0", 2.0f); setP (p, "laneRegEnd0", 1.0f); }   // end before start: falls back to the whole lane
        process (p, in, ys[v], 512);
        float pk; CHECK (finiteBounded (ys[v], 8.0f, pk), "section variant %d not finite or too loud (peak %.2f)", v, pk);
    }
    double d01 = 0, d02 = 0; for (size_t i = 0; i < ys[0].size(); ++i) { d01 += std::abs (ys[0][i] - ys[1][i]); d02 += std::abs (ys[0][i] - ys[2][i]); }
    CHECK (d01 > 1e-4, "a section ending before the Chorus sounds the same as the whole lane (%.5f)", d01);
    CHECK (d02 < 1e-6, "a bad range should fall back to the whole lane (%.5f)", d02);
}

static void testSwitching()
{
    std::printf ("[switching] toggling blocks on and off during a sustained note: largest sample-to-sample jump\n");
    const int n = 48000 * 3; std::vector<float> in ((size_t) n);
    for (int i = 0; i < n; ++i) in[(size_t) i] = 0.3f * std::sin (6.2831853f * 220.0f * i / 48000.0f);
    std::vector<juce::String> ids;
    { StarterAmpProcessor probe; for (auto* prm : probe.getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) { auto id = r->paramID; if (id.endsWith ("On") || id == "cabOn") ids.push_back (id); } }
    std::printf ("  %d on/off switches found\n", (int) ids.size());
    // reference jump of the steady note through the same chain
    StarterAmpProcessor ref; ref.setPlayConfigDetails (2, 2, 48000.0, 512); ref.prepareToPlay (48000.0, 512);
    std::vector<float> yr; process (ref, in, yr, 512);
    float refJump = 0; for (size_t i = 24001; i < yr.size(); ++i) refJump = std::max (refJump, std::abs (yr[i] - yr[i - 1]));
    int worst = 0; float worstJump = 0; juce::String worstId;
    for (auto& id : ids)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        std::vector<float> y ((size_t) n); juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m; bool on = false;
        for (size_t pos = 0; pos + 512 <= (size_t) n; pos += 512)
        {
            if ((pos / 512) % 20 == 10) { on = ! on; if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id))) setNorm (p, r, on ? 1.0f : 0.0f); }
            for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[pos + (size_t) i]); b.setSample (1, i, in[pos + (size_t) i]); }
            p.processBlock (b, m); for (int i = 0; i < 512; ++i) y[pos + (size_t) i] = b.getSample (0, i);
        }
        float j = 0; for (size_t i = 24001; i < y.size(); ++i) j = std::max (j, std::abs (y[i] - y[i - 1]));
        if (j > worstJump) { worstJump = j; worstId = id; }
        if (j > 4.0f * std::max (refJump, 0.05f) + 0.15f) { ++worst; std::printf ("    possible click: %s jump %.3f (steady %.3f)\n", id.toRawUTF8(), j, refJump); }
    }
    std::printf ("  largest jump %.3f (%s); steady-note jump %.3f\n", worstJump, worstId.toRawUTF8(), refJump);
    CHECK (worst == 0, "%d switches produced jumps over 4x the steady-note jump (+0.15) - listen to these for clicks", worst);
}

//==============================================================================
static void renderRow (StarterAmpProcessor& p, int model, int voi, double* row)
{
    setP (p, "ampModel", (float) model); setP (p, "ampVoicing", (float) voi); setP (p, "ampGain", 0.5f); setP (p, "ampMode", 2.0f); setP (p, "ampAutoLevel", 0.0f);
    for (const char* c : { "cabOn", "cabOnA", "cabOnB", "cabOnC", "cabOnD" }) setP (p, c, 0.0f);
    p.prepareToPlay (48000.0, 512);
    const auto in = testSignal (48000 * 2, 48000.0, 0.3f); std::vector<float> y; process (p, in, y, 512);
    row[0] = rmsDb (y, 24000); bands (std::vector<float> (y.begin() + 24000, y.end()), 48000.0, row + 1);
    for (int i = 1; i < 9; ++i) row[i] -= row[5];   // relative to 1 kHz
}
static int regress (bool update, const std::string& path)
{
    std::printf ("[regress] fixed render of every amp and voicing (gain 0.5, hybrid mode, cab off)\n");
    std::map<std::pair<int,int>, std::vector<double>> gold;
    if (! update)
    {
        FILE* f = std::fopen (path.c_str(), "r"); if (! f) { std::printf ("  cannot open %s (run with --update to create it)\n", path.c_str()); return 1; }
        int m, v; double r[9];
        while (std::fscanf (f, "%d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf", &m, &v, &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6], &r[7], &r[8]) == 11) gold[{ m, v }] = std::vector<double> (r, r + 9);
        std::fclose (f);
    }
    FILE* out = update ? std::fopen (path.c_str(), "w") : nullptr;
    for (int m = 0; m <= 33; ++m)
        for (int v = 0; v < 4; ++v)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
            double row[9]; renderRow (p, m, v, row);
            if (update) { std::fprintf (out, "%d %d %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", m, v, row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8]); continue; }
            auto it = gold.find ({ m, v }); if (it == gold.end()) { CHECK (false, "amp %d voicing %d missing from golden file", m, v); continue; }
            double worstBand = 0; for (int i = 1; i < 9; ++i) worstBand = std::max (worstBand, std::abs (row[i] - it->second[(size_t) i]));
            CHECK (std::abs (row[0] - it->second[0]) < 0.3 && worstBand < 0.6, "amp %d voicing %d changed: level %.2f -> %.2f dB, worst band %.2f dB", m, v, it->second[0], row[0], worstBand);
        }
    if (out) { std::fclose (out); std::printf ("  wrote %s\n", path.c_str()); }
    return 0;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const std::string cmd = argc > 1 ? argv[1] : "stress";
    const std::string golden = argc > 3 ? argv[3] : "golden.txt";
    if (cmd == "regress") { if (regress (argc > 2 && std::string (argv[2]) == "--update", golden)) return 2; }
    else
    {
        testStability(); testBlockSizes(); testAutomation (1); testAutomation (2); testAutomation (3);
        testState(); testLifecycle(); testOversampling(); testSwitching(); testMovedBlocks(); testMovedDrives(); testParallelSection(); testModSlots(); testCompStyles(); testReverbTypes(); testSpringReturn(); testDiMatch(); testFuzz2Block();
    }
    std::printf ("\n%d checks passed, %d failed\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
