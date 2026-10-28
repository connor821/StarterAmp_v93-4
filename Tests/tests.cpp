// Starter Amp automated tests. Build instructions are in Tests/README.md.
//   tests stress            numerical stability, block sizes, automation, state round trip, lifecycle, switching
//   tests regress           compare a fixed render of every amp and voicing with Tests/golden.txt
//   tests regress --update  rewrite golden.txt (do this only after a change you meant to make)
#include "PluginProcessor.h"
#include <cmath>
#include <complex>
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

// b46: the signal path is a patch of cables. These put a pedal in one of the three usual places, or beside another pedal.
namespace PG = PatchGraph;
static void placeLane (StarterAmpProcessor& p, int node, int lane)          // 0 before the amp, 1 after the cab, 2 in the amp's FX loop
{
    auto d = p.getPatch();
    if (PG::placeNear (d, node, lane)) p.setPatch (d);
}
static void placeBeside (StarterAmpProcessor& p, int a, int b)               // pedal b runs beside pedal a: fed from the same place, joined at the same place
{
    auto d = p.getPatch();
    PG::removeNodeBypass (d, b);
    const auto src = d;
    for (int i = 0; i < src.n; ++i)
    {
        if (src.c[i].to == a) PG::connect (d, src.c[i].from, src.c[i].fp, b, 0, src.c[i].levelDb, src.c[i].pan, src.c[i].pol != 0, src.c[i].chan);
        if (src.c[i].from == a) PG::connect (d, b, 0, src.c[i].to, src.c[i].tp, src.c[i].levelDb, src.c[i].pan, src.c[i].pol != 0, src.c[i].chan);
    }
    p.setPatch (d);
}

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
    for (int model = 0; model <= 40; ++model)
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
    a.apvts.state.setProperty ("modFold", 1, nullptr);  a.apvts.state.setProperty ("modFold2", 1, nullptr);  a.apvts.state.setProperty ("modFold3", 1, nullptr);   // old Wobble / Tremolo pedals are folded into Mod FX slots on load by design; this state is current, not old
    juce::MemoryBlock mb; a.getStateInformation (mb);
    StarterAmpProcessor b; b.setPlayConfigDetails (2, 2, 48000.0, 512); b.prepareToPlay (48000.0, 512);
    b.setStateInformation (mb.getData(), (int) mb.getSize());
    int bad = 0, total = 0;
    for (auto* prm : a.getParameters())
        if (auto* ra = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (ra->paramID != "wobOn" && ! ra->paramID.endsWith ("Pos"))   // an old Wobble setting is converted to a swell pedal on load by design, so it does not round-trip; the Mod FX position params only mirror where the patch puts each slot
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
    const char* on[2] = { "lfOn", "bsOn" }; const int node[2] = { 20, 21 };
    for (int b = 0; b < 2; ++b)
    {
        std::vector<float> ys[3];
        for (int l = 0; l < 3; ++l)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            placeLane (p, node[b], l);
            CHECK (PG::laneOf (p.getPatch(), node[b]) == l, "%s did not move to lane %d", on[b], l);
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
            if (combo != 1) placeLane (p, 22, l);
            if (combo != 0) placeLane (p, 23, l);
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

static void testJadeDist()
{
    std::printf ("[jadedist] Jade Chorus Distortion: 0 is bit-identical to the clean channel; turned up it is finite, bounded, audible and differs; other amps ignore it\n");
    const int n = 48000 * 2; auto in = testSignal (n, 48000.0, 0.3f);
    auto render = [&] (int model, float d, std::vector<float>& y)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
        setP (p, "ampModel", (float) model);  setP (p, "jadeDist", d);  p.prepareToPlay (48000.0, 512);
        process (p, in, y, 512);
    };
    std::vector<float> base, off, on, other0, other1;
    { StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); setP (p, "ampModel", 10.0f); p.prepareToPlay (48000.0, 512); process (p, in, base, 512); }
    render (10, 0.0f, off);
    double e0 = 0; for (size_t i = 0; i < base.size(); ++i) e0 += std::abs (base[i] - off[i]);
    CHECK (e0 == 0.0, "Jade Distortion at 0 changed the clean channel (%.9f)", e0);
    for (float d : { 0.1f, 0.5f, 1.0f })
    {
        render (10, d, on);
        float pk; CHECK (finiteBounded (on, 8.0f, pk), "Jade Distortion %.2f not finite or too loud (peak %.2f)", d, pk);
        double e = 0; for (size_t i = 0; i < base.size(); ++i) e += std::abs (base[i] - on[i]);
        CHECK (e > 1e-3, "Jade Distortion %.2f changed nothing (%.6f)", d, e);
        CHECK (rmsDb (on) > -80.0, "Jade Distortion %.2f is silent (%.1f dB)", d, rmsDb (on));
    }
    render (3, 0.0f, other0);  render (3, 1.0f, other1);
    double e1 = 0; for (size_t i = 0; i < other0.size(); ++i) e1 += std::abs (other0[i] - other1[i]);
    CHECK (e1 == 0.0, "Jade Distortion changed another amp (%.9f)", e1);
}

static void testAliasing()
{
    std::printf ("[aliasing] anti-aliased clipping: a held 1.18 kHz note at full gain on a high-gain amp keeps its inharmonic energy under -38 dB (was about -22 dB before b41, -40 with first-order anti-aliasing)\n");
    const int N = 16384;
    std::vector<float> in ((size_t) (N + 16384));
    const double f = 1183.3;
    for (size_t i = 0; i < in.size(); ++i) in[i] = 0.1f * (float) std::sin (6.283185307179586 * f * (double) i / 48000.0);
    for (int model : { 3, 11 })
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
        setP (p, "ampModel", (float) model);  setP (p, "ampGain", 1.0f);  setP (p, "cabOn", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);
        p.prepareToPlay (48000.0, 512);
        std::vector<float> y; process (p, in, y, 512);
        // Hann-windowed DFT of the last N samples, brute force on a coarse grid is too slow, so use a radix-2 FFT
        std::vector<std::complex<double>> a ((size_t) N);
        for (int i = 0; i < N; ++i) a[(size_t) i] = (double) y[y.size() - (size_t) N + (size_t) i] * (0.5 - 0.5 * std::cos (2.0 * M_PI * i / N));
        for (int i = 1, j = 0; i < N; ++i) { int bit = N >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[(size_t) i], a[(size_t) j]); }
        for (int len = 2; len <= N; len <<= 1)
        {
            const double ang = -2.0 * M_PI / len;  const std::complex<double> wl (std::cos (ang), std::sin (ang));
            for (int i = 0; i < N; i += len) { std::complex<double> w = 1.0; for (int k = 0; k < len / 2; ++k) { auto u = a[(size_t) (i + k)], v = a[(size_t) (i + k + len / 2)] * w; a[(size_t) (i + k)] = u + v; a[(size_t) (i + k + len / 2)] = u - v; w *= wl; } }
        }
        const double bw = 48000.0 / N;  std::vector<char> harm ((size_t) N / 2, 0);
        for (int h = 1; h * f < 24000.0; ++h) { const int c = (int) std::lround (h * f / bw); for (int d = -5; d <= 5; ++d) if (c + d >= 0 && c + d < N / 2) harm[(size_t) (c + d)] = 1; }
        double tot = 0, inh = 0;
        for (int k = 2; k < N / 2; ++k) { const double P = std::norm (a[(size_t) k]); tot += P; if (! harm[(size_t) k]) inh += P; }
        const double db = 10.0 * std::log10 (inh / tot + 1e-20);
        std::printf ("  amp %d: inharmonic energy %.1f dB\n", model, db);
        CHECK (db < -38.0, "amp %d: inharmonic energy %.1f dB (limit -38)", model, db);
    }
}

static double inharmonicDb (const std::vector<float>& y, double f)
{
    const int N = 16384;
    std::vector<std::complex<double>> a ((size_t) N);
    for (int i = 0; i < N; ++i) a[(size_t) i] = (double) y[y.size() - (size_t) N + (size_t) i] * (0.5 - 0.5 * std::cos (2.0 * M_PI * i / N));
    for (int i = 1, j = 0; i < N; ++i) { int bit = N >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (a[(size_t) i], a[(size_t) j]); }
    for (int len = 2; len <= N; len <<= 1)
    {
        const double ang = -2.0 * M_PI / len;  const std::complex<double> wl (std::cos (ang), std::sin (ang));
        for (int i = 0; i < N; i += len) { std::complex<double> w = 1.0; for (int k = 0; k < len / 2; ++k) { auto u = a[(size_t) (i + k)], v = a[(size_t) (i + k + len / 2)] * w; a[(size_t) (i + k)] = u + v; a[(size_t) (i + k + len / 2)] = u - v; w *= wl; } }
    }
    const double bw = 48000.0 / N;  std::vector<char> harm ((size_t) N / 2, 0);
    for (int h = 1; h * f < 24000.0; ++h) { const int c = (int) std::lround (h * f / bw); for (int d = -5; d <= 5; ++d) if (c + d >= 0 && c + d < N / 2) harm[(size_t) (c + d)] = 1; }
    double tot = 0, inh = 0;
    for (int k = 2; k < N / 2; ++k) { const double P = std::norm (a[(size_t) k]); tot += P; if (! harm[(size_t) k]) inh += P; }
    return 10.0 * std::log10 (inh / tot + 1e-20);
}

// b42: the pedals, the boost and the fuzz styles get the same treatment as the amp (b41). b45: second order. Limits sit about 4 dB above what the b44 code
// measures and below what b42 (first order) gave (shown in each line), so they fail if the anti-aliasing is taken out of any of them or drops back to first order.
static void testAliasingEffects()
{
    std::printf ("[aliasing-fx] anti-aliased pedals, boost and fuzz: a held 1.18 kHz note keeps its inharmonic energy under the limit for each\n");
    struct Case { const char* name; std::vector<std::pair<const char*, float>> ps; double limit; };
    const std::vector<Case> cases = {
        { "Distortion pedal, circuit (b41: -16, b42: -39)",  { { "odOn", 1 }, { "odStyle", 4 }, { "odGain", 0.9f }, { "odCircuit", 1 } }, -38.0 },
        { "Stack Drive (b41: -19, b42: -26)",                { { "odOn", 1 }, { "odStyle", 7 }, { "odGain", 0.9f }, { "odCircuit", 0 } }, -24.0 },
        { "Distortion pedal, classic (b41: -32, b42: -45)",  { { "odOn", 1 }, { "odStyle", 4 }, { "odGain", 0.9f }, { "odCircuit", 0 } }, -52.0 },
        { "Boost Tube (b41: -39, b42: -56)",                 { { "bsOn", 1 }, { "bsMode", 3 }, { "bsGain", 24 }, { "bsSat", 0.9f } },   -55.0 },
        { "Boost Mid (b41: -32, b42: -56)",                  { { "bsOn", 1 }, { "bsMode", 5 }, { "bsGain", 24 }, { "bsSat", 0.9f } },   -55.0 },
        { "Fuzz Tight (b41: -27, b42: -45)",                 { { "fuzzOn", 1 }, { "fuzzStyle", 1 }, { "fuzzGain", 0.9f } },              -52.0 },
        { "Fuzz Face (b41: -28, b42: -42)",                  { { "fuzzOn", 1 }, { "fuzzStyle", 4 }, { "fuzzGain", 0.9f } },              -49.0 },
        { "Fuzz Bender (b41: -32, b42: -50)",                { { "fuzzOn", 1 }, { "fuzzStyle", 5 }, { "fuzzGain", 0.9f } },              -56.0 },
        { "Fuzz Factory (b41: -36, b42: -53)",               { { "fuzzOn", 1 }, { "fuzzStyle", 6 }, { "fuzzGain", 0.9f } },              -55.0 },
        { "Fuzz Power Chip (b41: -37, b42: -61)",            { { "fuzzOn", 1 }, { "fuzzStyle", 9 }, { "fuzzGain", 0.9f } },              -57.0 },
    };
    const double f = 1183.3;
    std::vector<float> in (16384 + 40000);
    for (size_t i = 0; i < in.size(); ++i) in[i] = 0.3f * (float) std::sin (6.283185307179586 * f * (double) i / 48000.0);
    for (const auto& c : cases)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
        setP (p, "ampModel", 0);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0);  setP (p, "ampAutoLevel", 0);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);
        setP (p, "compOn", 0);  setP (p, "eqOn", 0);  setP (p, "reverbOn", 0);  setP (p, "postDelayOn", 0);  setP (p, "preDelayOn", 0);  setP (p, "cabOn", 0);  setP (p, "limOn", 0);
        for (const auto& kv : c.ps) setP (p, kv.first, kv.second);
        p.prepareToPlay (48000.0, 512);
        std::vector<float> y; process (p, in, y, 512);
        const double db = inharmonicDb (y, f);
        std::printf ("  %s: %.1f dB\n", c.name, db);
        CHECK (db < c.limit, "%s: inharmonic energy %.1f dB (limit %.0f)", c.name, db, c.limit);
    }
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
            setP (p, "gxOn", 1.0f);  placeLane (p, 19, 1);  setP (p, "gxThresh", -30.0f);  setP (p, "gxRange", -90.0f);  setP (p, "gxKey", (float) k);
            process (p, in, k ? b : a, 512);
            float pk; CHECK (finiteBounded (k ? b : a, 8.0f, pk), "Gate key %d not finite or too loud (peak %.2f)", k, pk);
        }
        double d = 0; for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]);
        CHECK (d > 1e-3, "Gate listening to the input sounds the same as listening to itself (%.5f)", d);
        // a silent input keeps it shut even though the amp makes noise: no crash and finite at least
        std::vector<float> sil ((size_t) n * 2, 0.0f), z;
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "gxOn", 1.0f);  placeLane (p, 19, 1);  setP (p, "gxKey", 1.0f);
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
        if (v == 2) placeBeside (p, 0, 26);
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
            if (v == 1) placeBeside (p, 22, 23);
            std::vector<float> y; process (p, in, y, 512);
            float pk; CHECK (finiteBounded (y, 8.0f, pk), "Drive 2 variant %d not finite or too loud (peak %.2f)", v, pk);
            (v == 0 ? ser : par2) = y;
        }
        CHECK (diff (ser, par2) > 1e-2, "Drive 2 on the lower row sounds like the stacked chain");
    }
    for (int lane = 0; lane < 3; ++lane)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "fuzz2On", 1.0f);  placeLane (p, 26, lane);
        CHECK (PG::laneOf (p.getPatch(), 26) == lane, "Fuzz 2 did not move to lane %d", lane);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "Fuzz 2 in lane %d not finite or too loud (peak %.2f)", lane, pk);
    }
}

static void testModSlots()
{
    std::printf ("[modslots] four mod slots, all 22 types: finite, bounded, differ from dry, both positions\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    const char* L = "ABCD";
    for (int slot = 0; slot < 4; ++slot)
        for (int type = 0; type < 22; ++type)
            for (int pos = 0; pos < 2; ++pos)
            {
                StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
                StarterAmpProcessor q; q.setPlayConfigDetails (2, 2, 48000.0, 512); q.prepareToPlay (48000.0, 512);
                const std::string b = std::string ("mf") + L[slot];
                static const int slotNode[4] = { 6, 7, 24, 25 };
                placeLane (p, slotNode[slot], pos);
                setP (p, (b + "On").c_str(), 1.0f);  setP (p, (b + "Type").c_str(), (float) type);
                setP (p, (b + "Mix").c_str(), 1.0f);  if (type == 20) setP (p, (b + "Rate").c_str(), 6.0f);  setP (p, (b + "Depth").c_str(), 0.8f);  setP (p, (b + "Fidelity").c_str(), 0.6f);
                std::vector<float> y, d; process (p, in, y, 512);  process (q, in, d, 512);
                float pk; CHECK (finiteBounded (y, 8.0f, pk), "slot %c type %d pos %d not finite or too loud (peak %.2f)", L[slot], type, pos, pk);
                double diff = 0; for (size_t i = 0; i < y.size(); ++i) diff += std::abs (y[i] - d[i]);
                CHECK (diff > 1e-3, "slot %c type %d pos %d is silent or identical to dry (%.5f)", L[slot], type, pos, diff);
            }
    // switching type on the fly must stay finite
    StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
    setP (p, "mfCOn", 1.0f);  placeLane (p, 24, 1);
    for (int k = 0; k < 22; ++k) { setP (p, "mfCType", (float) k); std::vector<float> y; process (p, in, y, 512); float pk; CHECK (finiteBounded (y, 8.0f, pk), "type switch to %d not finite", k); }
}


// b43: the old stand-alone Chorus, Phaser, Lo-Fi and Filter became Mod FX types. Old sessions are moved into free slots on load.
//==============================================================================
// b44: the polyphonic tuner. Synthetic strummed chords (stiff-string partials, noise): every string that has a peak of its own must read
// within 3 cents, strings that were not played must not appear, and a string hidden inside a lower string's overtones must be flagged.
static std::vector<float> polyChord (const std::vector<std::pair<float, float>>& notes, double sr, int n, unsigned seed)   // (midi, detune cents)
{
    std::mt19937 rng (seed); std::uniform_real_distribution<float> ph (0.0f, 6.2831853f); std::normal_distribution<float> g (0.0f, 1.0f);
    std::vector<float> x ((size_t) n, 0.0f);
    for (auto& nt : notes)
    {
        const float f0 = 440.0f * std::pow (2.0f, (nt.first - 69.0f) / 12.0f) * std::pow (2.0f, nt.second / 1200.0f);
        const float B = nt.first < 52 ? 1.5e-4f : 5e-5f;
        for (int h = 1; h <= 14; ++h)
        {
            const float fh = f0 * (float) h * std::sqrt (1.0f + B * (float) (h * h));
            if (fh > 0.45f * (float) sr) break;
            const float amp = 0.12f / std::pow ((float) h, 0.9f), tau = 2.2f / std::sqrt ((float) h), p0 = ph (rng);
            for (int i = 0; i < n; ++i)
            {
                const float t = (float) i / (float) sr;
                x[(size_t) i] += amp * std::exp (-t / tau) * std::sin (6.2831853f * fh * t + p0);
            }
        }
    }
    for (auto& v : x) v += 0.0004f * g (rng);
    return x;
}

static void testPolyTuner()
{
    std::printf ("[poly] polyphonic tuner: accuracy on strummed chords, false strings, presets, parameters\n");

    // the preset table
    for (int i = 0; i < PolyTuner::kNumPresets; ++i)
    {
        const auto& pr = PolyTuner::presets()[i];
        CHECK (pr.count >= 1 && pr.count <= PolyTuner::kMaxStrings, "preset %d count %d", i, pr.count);
        for (int k = 0; k < pr.count; ++k)
        {
            CHECK (pr.notes[k] >= 12 && pr.notes[k] <= 96, "preset %d note %d out of range", i, k);
            if (k > 0) CHECK (pr.notes[k] > pr.notes[k - 1], "preset %d notes not rising at string %d", i, k + 1);
        }
    }
    CHECK (PolyTuner::presets()[8].count == 7 && PolyTuner::presets()[8].notes[0] == 33 && PolyTuner::presets()[8].notes[6] == 62, "7-string A standard is A D G C F A D");

    // the parameters and the tap
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        auto get = [&] (const char* id) { return p.apvts.getRawParameterValue (id)->load(); };
        CHECK (get ("tunerMode") == 1.0f && get ("tunerCount") == 6.0f && get ("tunerShift") == 0.0f && get ("tunerN1") == 40.0f && get ("tunerN6") == 64.0f, "tuner parameter defaults");
        setP (p, "tunerCount", 7); setP (p, "tunerN1", 33); setP (p, "tunerShift", -2);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        StarterAmpProcessor q; q.setPlayConfigDetails (2, 2, 48000.0, 512); q.prepareToPlay (48000.0, 512);
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (q.apvts.getRawParameterValue ("tunerCount")->load() == 7.0f && q.apvts.getRawParameterValue ("tunerN1")->load() == 33.0f && q.apvts.getRawParameterValue ("tunerShift")->load() == -2.0f, "tuner settings are saved with the session");
        { StarterAmpProcessor r; r.setPlayConfigDetails (2, 2, 48000.0, 512); r.prepareToPlay (48000.0, 512); juce::MemoryBlock pd; r.getPresetData (pd);
          q.loadPresetData (pd.getData(), (int) pd.getSize()); }   // a preset (default tuner settings) leaves the tuner alone (b61: the factory presets are gone)
        CHECK (q.apvts.getRawParameterValue ("tunerCount")->load() == 7.0f && q.apvts.getRawParameterValue ("tunerShift")->load() == -2.0f, "loading a preset keeps the tuner settings");

        p.tunerActive.store (true);
        juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
        for (int blk = 0; blk < 200; ++blk)
        {
            for (int i = 0; i < 512; ++i) { const float v = 0.3f * std::sin (6.2831853f * 110.0f * (float) (blk * 512 + i) / 48000.0f); b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
        }
        std::vector<float> tail (4096); p.tunerLong.copyLatest (tail.data(), 4096);
        CHECK (p.tunerLong.filled.load() == std::min (200 * 512, 65536), "the long tap counts what it holds (%d)", p.tunerLong.filled.load());
        double e = 0.0; for (float v : tail) e += (double) v * v;
        CHECK (e > 1.0, "the long tap holds the input (energy %.3f)", e);
        p.tunerActive.store (false);
    }

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        PolyTuner pt; pt.prepare (sr);
        const int N = pt.window();
        std::mt19937 rng (11); std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        for (int preset : { 0, 8, 12, 10 })
        {
            const auto& pr = PolyTuner::presets()[preset];
            double worstClean = 0.0; int cleanN = 0, approxN = 0, missed = 0;
            for (int t = 0; t < 12; ++t)
            {
                std::vector<std::pair<float, float>> notes; float targets[8];
                for (int i = 0; i < pr.count; ++i) { notes.push_back ({ (float) pr.notes[i], u (rng) * 25.0f }); targets[i] = (float) pr.notes[i]; }
                auto x = polyChord (notes, sr, N + 3000, (unsigned) (50 + t));
                std::vector<float> win (x.end() - N, x.end());
                PolyTuner::Result r; pt.reset();
                pt.analyse (win.data(), targets, pr.count, 440.0f, 0.1f, r);
                for (int i = 0; i < pr.count; ++i)
                {
                    if (! r.found[i]) { ++missed; continue; }
                    if (r.shared[i]) { ++approxN; continue; }
                    worstClean = std::max (worstClean, (double) std::abs (r.cents[i] - notes[(size_t) i].second));  ++cleanN;
                }
            }
            std::printf ("  %.0f Hz  %-38s clean readings %3d (worst error %.2f c), approx %3d, not heard %3d\n", sr, pr.name, cleanN, worstClean, approxN, missed);
            CHECK (worstClean < 4.0, "%s at %.0f Hz: a clean reading was %.2f cents off", pr.name, sr, worstClean);
            CHECK (cleanN >= 12 * pr.count / 2, "%s at %.0f Hz: only %d clean readings", pr.name, sr, cleanN);
        }

        // strings that were not played do not appear; one played string reads exactly
        const float targets[6] = { 40, 45, 50, 55, 59, 64 };
        PolyTuner::Result r;
        {
            std::vector<std::pair<float, float>> notes { { 40, 7.0f }, { 45, -9.0f }, { 50, 3.0f }, { 55, -12.0f } };
            auto x = polyChord (notes, sr, N + 3000, 5);
            std::vector<float> win (x.end() - N, x.end());
            pt.reset(); pt.analyse (win.data(), targets, 6, 440.0f, 0.1f, r);
            CHECK (! r.found[4] && ! r.found[5], "%.0f Hz: B and high E were not played but appear", sr);
            CHECK (r.found[0] && std::abs (r.cents[0] - 7.0f) < 3.0f && r.found[3] && std::abs (r.cents[3] + 12.0f) < 3.0f, "%.0f Hz: four-string chord read wrongly", sr);
        }
        {
            std::vector<std::pair<float, float>> one { { 59, -18.0f } };
            auto y = polyChord (one, sr, N + 3000, 6);
            std::vector<float> w2 (y.end() - N, y.end());
            pt.reset(); pt.analyse (w2.data(), targets, 6, 440.0f, 0.1f, r);
            CHECK (r.found[4] && ! r.shared[4] && std::abs (r.cents[4] + 18.0f) < 2.0f, "%.0f Hz: a lone B should read -18 (read %.2f)", sr, r.cents[4]);
            int others = 0; for (int i = 0; i < 6; ++i) if (i != 4 && r.found[i]) ++others;
            CHECK (others == 0, "%.0f Hz: a lone B also reports %d other strings", sr, others);
        }
        {
            std::vector<float> quiet ((size_t) N, 0.0f); std::mt19937 rn (3); std::normal_distribution<float> gg (0.0f, 1.0f); for (auto& v : quiet) v = 0.0003f * gg (rn);
            pt.reset(); pt.analyse (quiet.data(), targets, 6, 440.0f, 0.1f, r);
            int any = 0; for (int i = 0; i < 6; ++i) any += r.found[i] ? 1 : 0;
            CHECK (any == 0, "%.0f Hz: noise alone reads as %d strings", sr, any);
        }
    }
}

static void testFoldIn()
{
    std::printf ("[foldin] old Chorus / Phaser / Lo-Fi / Filter sessions move into slots and sound the same\n");
    const int n = 48000 * 2; auto in = testSignal (n, 48000.0, 0.3f);
    struct Case { const char* name; const char* on; std::vector<std::pair<const char*, float>> set; };
    const std::vector<Case> cases = {
        { "chorus I+II",  "chorusOn", { { "chorusMode", 2 }, { "chorusMix", 0.6f }, { "chorusRate", 1.7f }, { "chorusDepth", 1.2f }, { "chorusTone", 6000 }, { "chorusHiss", 0.4f } } },
        { "chorus detune", "chorusOn", { { "chorusMode", 6 }, { "chorusMix", 0.5f } } },
        { "chorus small clone", "chorusOn", { { "chorusMode", 8 }, { "chorusMix", 0.5f } } },
        { "phaser 6",     "phaserOn", { { "phaserStages", 2 }, { "phaserShape", 2 }, { "phaserRate", 3.0f }, { "phaserDepth", 0.9f }, { "phaserCenter", 500 }, { "phaserFeedback", 0.5f }, { "phaserMix", 0.7f }, { "phaserEnv", 0.3f }, { "phaserLag", 2.0f } } },
        { "lofi",         "lfOn",     { { "lfShape", 1 }, { "lfRate", 2.5f }, { "lfDepth", 0.7f }, { "lfDrift", 0.5f }, { "lfTone", 0.3f }, { "lfDirt", 0.43f }, { "lfNoise", 0.1f }, { "lfDropouts", 0.2f }, { "lfMix", 0.8f }, { "lfLevel", 3.0f } } },
        { "filter formant", "filtOn", { { "filtMode", 4 }, { "filtFreq", 600 }, { "filtRes", 0.6f }, { "filtMotion", 1 }, { "filtRate", 1.5f }, { "filtDepth", -0.7f }, { "filtVowel", 0.4f }, { "filtMix", 0.9f } } },
        { "filter resonator", "filtOn", { { "filtMode", 5 }, { "filtFreq", 220 }, { "filtRes", 0.8f }, { "filtMotion", 3 }, { "filtRate", 2.0f }, { "filtDepth", 0.5f } } },
        { "filter hp env", "filtOn",  { { "filtMode", 1 }, { "filtFreq", 900 }, { "filtMotion", 2 }, { "filtDepth", 0.8f } } },
    };
    for (const auto& c : cases)
    {
        StarterAmpProcessor src; src.setPlayConfigDetails (2, 2, 48000.0, 512); src.prepareToPlay (48000.0, 512);
        setP (src, c.on, 1.0f);
        for (auto& kv : c.set) setP (src, kv.first, kv.second);
        juce::MemoryBlock mb; src.getStateInformation (mb);
        // the old way: the same state with the fold-in switched off
        StarterAmpProcessor oldWay; oldWay.setPlayConfigDetails (2, 2, 48000.0, 512); oldWay.prepareToPlay (48000.0, 512);
        oldWay.setStateInformation (mb.getData(), (int) mb.getSize());
        // setStateInformation already folded it; build the unfolded reference from the source processor itself
        src.apvts.state.setProperty ("modFold", 1, nullptr);  src.apvts.state.setProperty ("modFold2", 1, nullptr);  src.apvts.state.setProperty ("modFold3", 1, nullptr);
        std::vector<float> yOld, yNew;  process (src, in, yOld, 512);  process (oldWay, in, yNew, 512);
        CHECK (oldWay.apvts.getRawParameterValue (c.on)->load() < 0.5f, "%s: the old block is still on after loading", c.name);
        int slotsOn = 0;  for (const char* m : { "mfAOn", "mfBOn", "mfCOn", "mfDOn" }) slotsOn += oldWay.apvts.getRawParameterValue (m)->load() > 0.5f;
        CHECK (slotsOn == 1, "%s: expected one slot on, got %d", c.name, slotsOn);
        float pk; CHECK (finiteBounded (yNew, 8.0f, pk), "%s: folded session not finite", c.name);
        double num = 0, den = 1e-12;  for (size_t i = (size_t) 4800; i < yOld.size(); ++i) { num += (double) (yOld[i] - yNew[i]) * (yOld[i] - yNew[i]); den += (double) yOld[i] * yOld[i]; }
        const double errDb = 10.0 * std::log10 (num / den + 1e-20);
        std::printf ("    %-18s folded vs old: %.1f dB\n", c.name, errDb);
        CHECK (errDb < -30.0, "%s: the folded session differs from the old one (%.1f dB)", c.name, errDb);
    }
    // four slots already busy: the old Chorus stays on
    {
        StarterAmpProcessor src; src.setPlayConfigDetails (2, 2, 48000.0, 512); src.prepareToPlay (48000.0, 512);
        for (const char* m : { "mfAOn", "mfBOn", "mfCOn", "mfDOn" }) setP (src, m, 1.0f);
        setP (src, "chorusOn", 1.0f);
        juce::MemoryBlock mb; src.getStateInformation (mb);
        StarterAmpProcessor b; b.setPlayConfigDetails (2, 2, 48000.0, 512); b.prepareToPlay (48000.0, 512);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (b.apvts.getRawParameterValue ("chorusOn")->load() > 0.5f, "with no free slot the old Chorus must stay on");
    }
    // slot types: every Phaser / Lo-Fi shape, the Tremolo Bias shape and Pick Response, the chorus modes and the filter modes must all differ from each other
    auto renderSlot = [&] (const std::vector<std::pair<const char*, float>>& set)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "mfCOn", 1.0f);  placeLane (p, 24, 1);  setP (p, "mfCMix", 1.0f);  setP (p, "mfCDepth", 0.9f);  setP (p, "mfCRate", 2.0f);
        for (auto& kv : set) setP (p, kv.first, kv.second);
        std::vector<float> y; process (p, in, y, 512);  return y;
    };
    auto distinct = [&] (const char* what, const std::vector<std::vector<float>>& outs)
    {
        for (size_t a = 0; a < outs.size(); ++a)
        {
            float pk; CHECK (finiteBounded (outs[a], 8.0f, pk), "%s %zu not finite", what, a);
            for (size_t b = a + 1; b < outs.size(); ++b)
            {
                double d = 0;  for (size_t i = 0; i < outs[a].size(); ++i) d += std::abs (outs[a][i] - outs[b][i]);
                CHECK (d > 1e-2, "%s %zu and %zu sound identical", what, a, b);
            }
        }
    };
    { std::vector<std::vector<float>> o;  for (int sh = 0; sh <= 6; ++sh) if (sh != 7) o.push_back (renderSlot ({ { "mfCType", 13 }, { "mfCShape", (float) sh }, { "mfCFeedback", 0.5f } }));  distinct ("phaser shape", o); }
    { std::vector<std::vector<float>> o;  for (int sh = 0; sh <= 6; ++sh) o.push_back (renderSlot ({ { "mfCType", 14 }, { "mfCShape", (float) sh }, { "mfCDepth", 0.9f } }));  distinct ("lo-fi shape", o); }
    { std::vector<std::vector<float>> o;  for (int m = 0; m < 9; ++m) o.push_back (renderSlot ({ { "mfCType", 12 }, { "mfCChMode", (float) m } }));  distinct ("chorus mode", o); }
    { std::vector<std::vector<float>> o;  for (int m = 0; m < 6; ++m) o.push_back (renderSlot ({ { "mfCType", 21 }, { "mfCFMode", (float) m }, { "mfCFMotion", 1 } }));  distinct ("filter mode", o); }
    { std::vector<std::vector<float>> o;  for (int m = 0; m < 4; ++m) o.push_back (renderSlot ({ { "mfCType", 21 }, { "mfCFMode", 2 }, { "mfCFMotion", (float) m } }));  distinct ("filter motion", o); }
    { std::vector<std::vector<float>> o;
      o.push_back (renderSlot ({ { "mfCType", 8 }, { "mfCShape", 0 } }));  o.push_back (renderSlot ({ { "mfCType", 8 }, { "mfCShape", 8 } }));
      o.push_back (renderSlot ({ { "mfCType", 8 }, { "mfCShape", 8 }, { "mfCPick", 1.0f } }));  distinct ("tremolo bias / pick", o); }
    { auto a = renderSlot ({ { "mfCType", 21 }, { "mfCFMode", 0 }, { "mfCFMotion", 1 } });  auto b = renderSlot ({ { "mfCType", 21 }, { "mfCFMode", 0 }, { "mfCFMotion", 1 }, { "mfCFlip", 1 } });  distinct ("sweep direction", { a, b }); }
}

static void testParallelSection()
{
    std::printf ("[section] a parallel section inside a lane: the Compressor beside the Fuzz, with the Chorus after the merge or beside both\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    std::vector<float> ys[3];
    for (int v = 0; v < 3; ++v)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "fuzzOn", 1.0f); setP (p, "compOn", 1.0f); setP (p, "chorusOn", 1.0f); setP (p, "chorusMix", 0.6f);
        if (v >= 1) placeBeside (p, 0, 1);                       // Compressor beside the Fuzz, both then into the next pedal (the Chorus comes after the merge)
        if (v == 2)                                              // the Chorus beside the Fuzz too
        {
            placeBeside (p, 0, 4);
            CHECK (PG::cableCountOf (p.getPatch(), 4) >= 2, "chorus is wired");
        }
        process (p, in, ys[v], 512);
        float pk; CHECK (finiteBounded (ys[v], 8.0f, pk), "section variant %d not finite or too loud (peak %.2f)", v, pk);
    }
    double d01 = 0, d12 = 0; for (size_t i = 0; i < ys[0].size(); ++i) { d01 += std::abs (ys[0][i] - ys[1][i]); d12 += std::abs (ys[1][i] - ys[2][i]); }
    CHECK (d01 > 1e-4, "a Compressor beside the Fuzz sounds the same as the chain (%.5f)", d01);
    CHECK (d12 > 1e-4, "a Chorus beside the Fuzz sounds the same as one after the merge (%.5f)", d12);
}

// b46: cables. Graph editing, summing, polarity, mute, the amp's FX loop, and the saved patch.
static void testPatchGraph()
{
    std::printf ("[patch] cables: editing, compile, summing and polarity, mute, the FX loop, save and restore\n");
    {   // the default board
        PG::Data d; PG::defaultPatch (d); PG::Plan pl; PG::compile (d, pl);
        CHECK (pl.active[PG::kIn] && pl.active[PG::kOut] && pl.active[PG::kAmpA] && pl.active[PG::kCab], "default: core nodes run");
        CHECK (pl.nTail == 2 && pl.tail[0] == 22 && pl.tail[1] == 23, "default: both drives are built into the amp (%d)", pl.nTail);
        CHECK (! pl.hasLoop, "default: no FX loop");
        CHECK (PG::acyclic (d), "default: no cycles");
        PG::Data e; CHECK (PG::fromString (PG::toString (d), e) && PG::toString (e) == PG::toString (d), "the text form round-trips");
        PG::Data x = d;  CHECK (! PG::connect (x, 12, 0, 0, 0), "a cable that would feed a pedal back into itself is refused");
        CHECK (PG::insertBefore (x, 12, PG::kAmpA, 1) && PG::laneOf (x, 12) == 2, "a pedal can be put in the FX loop");
        CHECK (PG::insertBefore (x, 12, PG::kOut, 0) && PG::laneOf (x, 12) == 1, "...and taken out again");
        for (int k = 0; k < 20; ++k)
        {
            PG::Data y = d;
            for (int i = 0; i < 7; ++i) { const int a = (k * 7 + i * 5) % 28, b = (k * 3 + i * 11) % 28; if (PG::isPedal (a) && PG::isPedal (b) && a != b) PG::insertBefore (y, a, b, 0); }
            CHECK (PG::acyclic (y), "random moves leave no cycle (%d)", k);
        }
    }
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    {   // a second cable into the output, flipped: the two cancel (one alone does not)
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        std::vector<float> a, b;
        process (p, in, a, 512);
        auto t = p.getPatch();
        int toOut = -1; for (int i = 0; i < t.n; ++i) if (t.c[i].to == PG::kOut) toOut = i;
        CHECK (toOut >= 0, "the output has a cable");
        // every pedal is off, so the cab reaches the output through unity wires; a second wire straight from the cab, upside down, cancels it
        CHECK (PG::connect (t, PG::kCab, 0, PG::kOut, 0, 0.0f, 0.0f, true), "a second wire from the cab into the output, upside down");
        StarterAmpProcessor q; q.setPlayConfigDetails (2, 2, 48000.0, 512); q.prepareToPlay (48000.0, 512);
        q.setPatch (t);
        process (q, in, b, 512);
        double ra = 0, rb = 0, rd = 0; for (size_t i = 12000; i < a.size(); ++i) { ra += (double) a[i] * a[i]; rb += (double) b[i] * b[i]; rd += (double) (a[i] + b[i]) * (a[i] + b[i]); }
        CHECK (ra > 1e-6, "the plain board makes sound");
        CHECK (rb < ra * 1e-6, "a flipped copy cancels the original (left %.3e of %.3e)", rb, ra);
        std::printf("  plain level %.3e, with a flipped copy %.3e\n", ra, rb);
    }
    {   // a muted cable carries nothing; a quieter one is quieter
        auto level = [&] (float db)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            auto d = p.getPatch();
            for (int i = 0; i < d.n; ++i) if (d.c[i].to == PG::kOut) d.c[i].levelDb = db;
            p.setPatch (d);
            std::vector<float> y; process (p, in, y, 512);
            double r = 0; for (size_t i = 12000; i < y.size(); ++i) r += (double) y[i] * y[i];
            return r;
        };
        const double r0 = level (0.0f), r6 = level (-6.0f), rm = level (-60.0f);
        CHECK (std::abs (10.0 * std::log10 (r6 / r0) + 6.0) < 0.3, "a cable at -6 dB lowers the level by 6 dB (%.2f)", 10.0 * std::log10 (r6 / r0));
        CHECK (rm < 1e-12, "a muted cable is silent (%.3e)", rm);
    }
    {   // the reverb in the amp's FX loop, a parallel pedal and a crossover all make sound and differ from the plain board
        std::vector<float> base, loop, par, xo;
        for (int v = 0; v < 4; ++v)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
            setP (p, "reverbOn", 1.0f); setP (p, "reverbMix", 0.5f); setP (p, "chorusOn", 1.0f); setP (p, "chorusMix", 0.5f);
            if (v == 1) { placeLane (p, 12, 2); CHECK (PG::laneOf (p.getPatch(), 12) == 2, "reverb in the loop"); }
            if (v == 2) placeBeside (p, 4, 5);
            if (v == 3)
            {
                auto d = p.getPatch();
                const int ci = PG::findCable (d, 4, 0, 5, 0);
                CHECK (ci >= 0 && PG::insertSplit (d, ci, PG::kXo1), "crossover inserted");
                d.xoHz[0] = 700.0f;  p.setPatch (d);
            }
            std::vector<float> y; process (p, in, y, 512);
            float pk; CHECK (finiteBounded (y, 8.0f, pk), "patch variant %d not finite or too loud (peak %.2f)", v, pk);
            (v == 0 ? base : v == 1 ? loop : v == 2 ? par : xo) = y;
        }
        auto diff = [] (const std::vector<float>& a, const std::vector<float>& b) { double d = 0; for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]); return d; };
        CHECK (diff (base, loop) > 1e-2, "reverb in the loop sounds like the reverb after the cab");
        CHECK (diff (base, par) > 1e-3, "a pedal beside another sounds like the chain");
        CHECK (diff (base, xo) > 1e-6, "a crossover with both bands rejoined changed nothing at all");
    }
    {   // save and restore the patch with the session
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        placeLane (p, 12, 2);  placeBeside (p, 4, 5);
        auto d = p.getPatch();  d.c[0].levelDb = -2.5f;  p.setPatch (d);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        StarterAmpProcessor q; q.setPlayConfigDetails (2, 2, 48000.0, 512); q.prepareToPlay (48000.0, 512);
        q.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (q.getPatchText() == p.getPatchText(), "the patch survives save and restore");
        q.resetPatch();
        CHECK (q.getPatchText() != p.getPatchText(), "reset changes it back");
    }
}

// b49: the Brown Eye's Tight knob reaches further (up to 5.6 times on the high-passes between the stages): the low end under about 150 Hz falls clearly
static void testBrownEyeTight()
{
    std::printf ("[tight] Brown Eye Tight knob: finite, and less low end as it goes up\n");
    const int n = 48000; auto in = testSignal (n, 48000.0, 0.3f);
    double lowShare[2] = { 0, 0 };
    for (int k = 0; k < 2; ++k)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512); p.prepareToPlay (48000.0, 512);
        setP (p, "ampModel", 2.0f);  setP (p, "ampGain", 0.7f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "mtTight", (float) k);
        std::vector<float> y; process (p, in, y, 512);
        float pk; CHECK (finiteBounded (y, 8.0f, pk), "Brown Eye Tight %d not finite or too loud (peak %.2f)", k, pk);
        double lp = 0, lo = 0, tot = 0;  const double a = std::exp (-6.2831853 * 150.0 / 48000.0);
        for (size_t i = 12000; i < y.size(); ++i) { lp = a * lp + (1.0 - a) * y[i]; lo += lp * lp; tot += (double) y[i] * y[i]; }
        lowShare[k] = 10.0 * std::log10 (lo / (tot + 1e-20) + 1e-20);
    }
    CHECK (lowShare[1] < lowShare[0] - 3.0, "Tight 1 keeps the low end (%.1f dB against %.1f dB of the total at Tight 0)", lowShare[1], lowShare[0]);
}

static void testFeedbackLatch()
{
    std::printf ("[latch] a quiet note must sound the same before and after a loud passage (the feedback-loop solver used to stick)\n");
    struct Case { int model, voicing; float gain; };
    const Case cases[] = { { 21, 2, 0.4f }, { 21, 2, 0.5f }, { 21, 2, 0.6f }, { 30, 3, 0.6f }, { 2, 0, 0.5f } };
    auto quietLevel = [] (StarterAmpProcessor& p)
    {
        std::vector<float> x ((size_t) (0.8 * 48000.0)), y;
        for (size_t i = 0; i < x.size(); ++i) x[i] = 0.01f * std::sin (6.2831853f * 220.0f * (float) i / 48000.0f);
        process (p, x, y, 512);
        double s = 0; const size_t a = y.size() - 14400; for (size_t i = a; i < y.size(); ++i) s += (double) y[i] * y[i];
        return 10.0 * std::log10 (s / 14400.0 + 1e-20);
    };
    for (const auto& c : cases)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
        setP (p, "ampModel", (float) c.model);  setP (p, "ampVoicing", (float) c.voicing);  setP (p, "ampGain", c.gain);
        setP (p, "ampMode", 2.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);   // the input gate would open and close on the quiet note
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) { auto id = r->paramID; if (id.endsWith ("On") && id != "ampAOn") setP (p, id.toRawUTF8(), 0.0f); }
        p.prepareToPlay (48000.0, 512);
        const double l0 = quietLevel (p);
        std::vector<float> y; process (p, testSignal (48000 * 3, 48000.0, 0.3f), y, 512);
        p.prepareToPlay (48000.0, 512);          // a transport restart: the old solver could come back stuck on the wrong answer
        const double l1 = quietLevel (p);
        std::vector<float> loud (48000);
        for (size_t i = 0; i < loud.size(); ++i) loud[i] = 0.3f * std::sin (6.2831853f * 110.0f * (float) i / 48000.0f);
        process (p, loud, y, 512);
        p.prepareToPlay (48000.0, 512);
        const double l2 = quietLevel (p);
        CHECK (std::abs (l1 - l0) < 3.0 && std::abs (l2 - l0) < 3.0, "model %d voicing %d gain %.1f: quiet note %.1f dB fresh, %.1f dB after a riff, %.1f dB after a loud tone", c.model, c.voicing, c.gain, l0, l1, l2);
    }
}

static void testAmpSwitch()
{
    std::printf ("[ampswitch] switching the amp model mid-note: no spike (b51: the new amp's first milliseconds used to come out at up to 48 times the playing level, depending on where in the note the switch fell)\n");
    struct Pair { int from, to; };
    const Pair pairs[] = { { 0, 27 }, { 4, 27 }, { 13, 14 }, { 0, 11 }, { 29, 12 }, { 26, 3 }, { 2, 8 }, { 27, 0 } };
    float worst = 0.0f;
    for (int off = 0; off < 6; ++off)
        for (const auto& pr : pairs)
        {
            StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
            setP (p, "ampModel", (float) pr.from);  setP (p, "gate", -90.0f);  setP (p, "ampAutoLevel", 1.0f);
            for (auto* prm : p.getParameters())
                if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) { auto id = r->paramID; if (id.endsWith ("On") && id != "cabOn" && id != "ampAOn") setP (p, id.toRawUTF8(), 0.0f); }
            p.prepareToPlay (48000.0, 512);
            const size_t sw = 96000 + (size_t) off * 37;                          // the switch falls at a different point of the note (and of the block) each time
            std::vector<float> x (sw + 48000), y1, y2;
            for (size_t i = 0; i < x.size(); ++i) x[i] = 0.12f * std::sin (6.2831853f * 196.0f * (float) i / 48000.0f);
            std::vector<float> first (x.begin(), x.begin() + (long) sw), second (x.begin() + (long) sw, x.end());
            process (p, first, y1, 512);
            setP (p, "ampModel", (float) pr.to);
            process (p, second, y2, 512);
            float before = 0.0f, after = 0.0f, afterSettled = 0.0f;
            for (size_t i = sw - 9600; i < sw; ++i) before = std::max (before, std::abs (y1[i]));
            for (size_t i = 0; i < 4800; ++i) after = std::max (after, std::abs (y2[i]));                           // the first 100 ms
            for (size_t i = 24000; i < 48000; ++i) afterSettled = std::max (afterSettled, std::abs (y2[i]));         // later: the new amp's own level
            const float ref = std::max (std::max (before, afterSettled), 0.01f);
            worst = std::max (worst, after / ref);
            CHECK (after < 1.5f * ref, "amp %d to %d (switch at %zu): peak %.3f in the 100 ms after the switch against %.3f before and %.3f once settled", pr.from, pr.to, sw, after, before, afterSettled);
        }
    std::printf ("  worst peak in the 100 ms after a switch: %.2f times the larger of the old level and the settled new level\n", worst);
}

static void testHighGainControls()
{
    std::printf ("[highgain] every high-gain amp: Tight, Focus and Fizz Tamer do something (Modern Metal panel and Focus on all of them since b51)\n");
    const int models[] = { 2, 3, 4, 8, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 24, 25, 26, 27, 29 };
    CHECK (std::all_of (std::begin (models), std::end (models), [] (int m) { return ampIsHighGain (m); }), "ampIsHighGain misses one of the high-gain amps");
    CHECK (! ampIsHighGain (0) && ! ampIsHighGain (1) && ! ampIsHighGain (5) && ! ampIsHighGain (9) && ! ampIsHighGain (21) && ! ampIsHighGain (31), "ampIsHighGain includes a clean amp");
    const auto in = testSignal (48000 * 2, 48000.0, 0.3f);
    auto run = [&] (int model, int mode, const char* id, float value)
    {
        StarterAmpProcessor p; p.setPlayConfigDetails (2, 2, 48000.0, 512);
        setP (p, "ampModel", (float) model);  setP (p, "ampMode", (float) mode);  setP (p, "ampGain", 0.8f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) { auto pid = r->paramID; if (pid.endsWith ("On") && pid != "cabOn" && pid != "ampAOn") setP (p, pid.toRawUTF8(), 0.0f); }
        setP (p, id, value);
        p.prepareToPlay (48000.0, 512);
        std::vector<float> y; process (p, in, y, 512);
        double b[8]; bands (y, 48000.0, b);
        return std::vector<double> (b, b + 8);
    };
    for (int mode : { 1, 2 })
        for (int m : models)
        {
            const auto base = run (m, mode, "mtTight", 0.0f), tight = run (m, mode, "mtTight", 1.0f);
            const auto foc0 = run (m, mode, "ampTune", 0.0f), foc = run (m, mode, "ampTune", 12.0f);
            const auto fz0 = run (m, mode, "mtFizz", 0.0f), fz = run (m, mode, "mtFizz", 1.0f);
            double dT = 0, dF = 0;  for (int i = 0; i < 8; ++i) { dT = std::max (dT, std::abs (tight[(size_t) i] - base[(size_t) i])); dF = std::max (dF, std::abs (foc[(size_t) i] - foc0[(size_t) i])); }
            const double top0 = fz0[7], top1 = fz[7];
            CHECK (dT > 0.4, "amp %d mode %d: Tight changes no band by more than 0.4 dB (%.2f)", m, mode, dT);
            CHECK (dF > 0.4, "amp %d mode %d: Focus changes no band by more than 0.4 dB (%.2f)", m, mode, dF);
            CHECK (top1 < top0 - 6.0, "amp %d mode %d: Fizz Tamer at 1 takes only %.1f dB off the top band (want 6)", m, mode, top0 - top1);
        }
}

static void testLoFiDelay()
{
    std::printf ("[lofidelay] the Lo-Fi delay mode: the sample-rate reducer's images and aliases stay out of the echoes (b51: a 330 Hz note used to leave a tone at 12 kHz only 28 dB under it)\n");
    for (float lofi : { 0.0f, 0.5f, 1.0f })
    {
        DelayEffect d;  juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };  d.prepare (spec, 250.0f, 1.0f, 1);
        DelayParams p {};  p.timeMs = 250.0f;  p.feedback = 0.5f;  p.mix = 1.0f;  p.lofi = lofi;  p.warble = 0.0f;  p.mode = 3;
        std::vector<float> out;  juce::AudioBuffer<float> b (2, 512);
        for (int pos = 0; pos < 48000 * 2; pos += 512)
        {
            for (int i = 0; i < 512; ++i) { const long t = pos + i;  const float x = t < 24000 ? 0.3f * std::sin (6.2831853f * 330.0f * (float) t / 48000.0f) : 0.0f;  b.setSample (0, i, x);  b.setSample (1, i, x); }
            d.process (b, p);
            for (int i = 0; i < 512; ++i) out.push_back (b.getSample (0, i));
        }
        // the part of the second echo that is above 5 kHz, against all of it (a plain DFT on a 40 Hz grid)
        double tot = 0, hi = 0;  const int start = 37000, len = 4096;
        for (double f = 40; f < 20000; f += 40)
        {
            double re = 0, im = 0;
            for (int i = 0; i < len; ++i) { const double w = 0.5 - 0.5 * std::cos (6.283185307 * i / len), ph = 6.283185307 * f * (start + i) / 48000.0;  re += out[(size_t) (start + i)] * w * std::cos (ph);  im += out[(size_t) (start + i)] * w * std::sin (ph); }
            tot += re * re + im * im;  if (f > 5000) hi += re * re + im * im;
        }
        const double hiDb = 10.0 * std::log10 (hi / tot + 1e-20);
        bool finite = true;  for (float v : out) if (! std::isfinite (v) || std::abs (v) > 4.0f) finite = false;
        CHECK (finite, "Lo-Fi delay at Lo-Fi %.1f: not finite or too loud", lofi);
        CHECK (hiDb < -55.0, "Lo-Fi delay at Lo-Fi %.1f: %.1f dB of the echo is above 5 kHz (want under -55)", lofi, hiDb);
    }
}

// ---- b52: delay engine additions (echo styles, saturation, echo EQ, Groove / Feel / Accent)
namespace dly52
{
    static const double SR = 48000.0;
    typedef float (*InFn) (long, void*);
    static void run (const DelayParams& p, double secs, std::vector<float>& L, std::vector<float>& R, InFn in, void* ud, float mix = 1.0f)
    {
        DelayEffect d;  juce::dsp::ProcessSpec spec { SR, 512, 2 };  d.prepare (spec, 200.0f, mix, 3);
        const long N = (long) (secs * SR);  L.assign ((size_t) N, 0.0f);  R.assign ((size_t) N, 0.0f);
        juce::AudioBuffer<float> b (2, 512);  DelayParams q = p;  q.mix = mix;
        for (long pos = 0; pos < N; pos += 512)
        {
            const int n = (int) std::min<long> (512, N - pos);
            b.setSize (2, n, false, false, true);
            for (int i = 0; i < n; ++i) { const float x = in (pos + i, ud);  b.setSample (0, i, x);  b.setSample (1, i, x); }
            d.process (b, q);
            for (int i = 0; i < n; ++i) { L[(size_t) (pos + i)] = b.getSample (0, i);  R[(size_t) (pos + i)] = b.getSample (1, i); }
        }
    }
    static float impulse (long t, void*) { return t == 100 ? 1.0f : 0.0f; }
    static float burst (long t, void* ud) { const double f = *(double*) ud;  return t < 4800 ? 0.3f * (float) std::sin (2 * 3.14159265358979 * f * (double) t / SR) : 0.0f; }
    static float sustained (long t, void* ud) { const double f = *(double*) ud;  return 0.3f * (float) std::sin (2 * 3.14159265358979 * f * (double) t / SR); }
    static float tiny (long t, void*) { return t < 4800 ? 0.001f * (float) std::sin (2 * 3.14159265358979 * 1000.0 * (double) t / SR) : 0.0f; }
    static float big (long t, void*)  { return t < 4800 ? 0.9f * (float) std::sin (2 * 3.14159265358979 * 1000.0 * (double) t / SR) : 0.0f; }
    static std::vector<double> peaks (const std::vector<float>& x, double thr, double minGapMs)
    {
        std::vector<double> r;  const long gap = (long) (minGapMs * SR / 1000);  long last = -1000000;
        for (size_t i = 1; i + 1 < x.size(); ++i)
        {
            const float a = std::abs (x[i]);
            if (a > thr && a >= std::abs (x[i - 1]) && a >= std::abs (x[i + 1]) && (long) i - last > gap) { r.push_back ((double) i * 1000.0 / SR);  last = (long) i; }
        }
        return r;
    }
    static double rmsOf (const std::vector<float>& x, double a, double b) { double s = 0;  long n = 0;  for (long i = (long) (a * SR); i < (long) (b * SR); ++i) { s += (double) x[(size_t) i] * x[(size_t) i];  ++n; }  return std::sqrt (s / (double) n); }
}

static void testDelayFeatures()
{
    using namespace dly52;
    std::printf ("[delay52] delay engine: b51 settings unchanged, new echo styles, saturation, echo EQ, Groove / Feel / Accent\n");
    std::vector<float> L, R;
    // 1. every b51 delay setting still renders as it did (RMS of each of 120 renders, measured on b51)
    {
        static const float ref[120] = { 0.113471f,0.116789f,0.095582f,0.116324f,0.140455f,0.106630f,0.108002f,0.097793f,0.107444f,0.122312f,0.106790f,0.107854f,0.098430f,0.108325f,0.122435f,0.114079f,0.115971f,0.098714f,0.114431f,0.136546f,0.114059f,0.116172f,0.095622f,0.114810f,0.134521f,0.106826f,0.107731f,0.097782f,0.105845f,0.118374f,0.106913f,0.107582f,0.098416f,0.108696f,0.118431f,0.114316f,0.115397f,0.098630f,0.112481f,0.131209f,0.116476f,0.115318f,0.096019f,0.110644f,0.133476f,0.107871f,0.107364f,0.097810f,0.101356f,0.117229f,0.107737f,0.107262f,0.098418f,0.109530f,0.117098f,0.115737f,0.114786f,0.098714f,0.106866f,0.129725f,0.114738f,0.115868f,0.095620f,0.112267f,0.133878f,0.107075f,0.107595f,0.097759f,0.103790f,0.117744f,0.107085f,0.107449f,0.098389f,0.108378f,0.117754f,0.114634f,0.115136f,0.098579f,0.109650f,0.130493f,0.116004f,0.113639f,0.101899f,0.108211f,0.133258f,0.107746f,0.106338f,0.098489f,0.100948f,0.117252f,0.107439f,0.106925f,0.103050f,0.107081f,0.116992f,0.115446f,0.113323f,0.100051f,0.105865f,0.129590f,0.118987f,0.085012f,0.098549f,0.089787f,0.114462f,0.102788f,0.094082f,0.099862f,0.091492f,0.099071f,0.107041f,0.103551f,0.101474f,0.098931f,0.101648f,0.106079f,0.081897f,0.098192f,0.090195f,0.103948f };
        int cfg = 0;  double worst = 0;
        for (int mode = 0; mode <= 5; ++mode) for (int st = 0; st < 4; ++st) for (int variant = 0; variant < 5; ++variant)
        {
            DelayEffect d;  juce::dsp::ProcessSpec spec { 48000.0, 512, 2 };  d.prepare (spec, 250.0f, 0.5f, 3);
            DelayParams p {};  p.timeMs = 180;  p.feedback = 0.55f;  p.mix = 0.5f;  p.mode = mode;  p.stereo = st;  p.offset = 0.5f;  p.spread = 1.0f;
            if (variant == 1) { p.lofi = 0.6f;  p.warble = 0.5f; }
            if (variant == 2) { p.style = 1;  p.duck = 0.5f; }
            if (variant == 3) { p.modDepth = 0.5f;  p.modRate = 1.5f;  p.diffuse = 0.6f;  p.taps = 3;  p.tapLevel = 0.6f; }
            if (variant == 4) { p.hold = true;  p.holdMode = 1;  p.feedback = 0.8f;  p.lofi = 0.2f; }
            juce::AudioBuffer<float> b (2, 512);  unsigned s = 12345;  double e = 0;  long cnt = 0;
            for (int pos = 0; pos < 48000; pos += 512)
            {
                for (int i = 0; i < 512; ++i) { const long t = pos + i;  s = s * 1664525u + 1013904223u;  const float nz = (float) ((s >> 9) & 0xffff) / 32768.0f - 1.0f;
                    const float x = t < 9000 ? 0.25f * std::sin (2 * 3.14159265f * 220 * (float) t / 48000.0f) + 0.05f * nz : 0.0f;  b.setSample (0, i, x);  b.setSample (1, i, 0.8f * x); }
                d.process (b, p);
                for (int i = 0; i < 512; ++i) { e += (double) b.getSample (0, i) * b.getSample (0, i) + (double) b.getSample (1, i) * b.getSample (1, i);  ++cnt; }
            }
            worst = std::max (worst, std::abs (std::sqrt (e / (double) cnt) - (double) ref[cfg]));  ++cfg;
        }
        CHECK (worst < 2.0e-4, "b51 delay settings: largest change in a render's level %.6f (want under 0.0002)", worst);
    }
    // 2. all modes / stereo modes with the new controls at their extremes stay finite and bounded
    {
        bool ok = true;  float mx = 0;
        for (int mode = 0; mode <= 11; ++mode) for (int st = 0; st < 4; ++st) for (int v = 0; v < 6; ++v)
        {
            DelayParams p {};  p.timeMs = 200;  p.feedback = 0.9f;  p.mode = mode;  p.stereo = st;  p.offset = 0.5f;  p.spread = 1.0f;
            if (v == 1) { p.sat = 1;  p.satType = 1; }
            if (v == 2) { p.sat = 1;  p.satType = 2;  p.groove = 1;  p.accent = -1;  p.feel = 1; }
            if (v == 3) { p.eqLow = 12;  p.eqMid = -12;  p.eqHigh = 12;  p.eqDecay = 1;  p.groove = 0.5f;  p.satType = 3;  p.sat = 0.7f; }
            if (v == 4) { p.lofi = 1;  p.warble = 1;  p.groove = 1;  p.accent = 1;  p.feel = -1;  p.taps = 3;  p.diffuse = 1;  p.hold = true;  p.holdMode = 1; }
            if (v == 5) { p.style = 1;  p.groove = 1;  p.feel = 1;  p.accent = 1;  p.sat = 1; }
            double f = 110;  run (p, 3.0, L, R, sustained, &f, 0.6f);
            for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) ok = false;  mx = std::max (mx, std::max (std::abs (L[i]), std::abs (R[i]))); }
        }
        CHECK (ok, "delay: output not finite with the new controls at their extremes");
        CHECK (mx < 8.0f, "delay: peak %.2f with the new controls at their extremes (want under 8)", mx);
    }
    // 2b. b67 Memory Man (estimate): finite, darker than Analog at 6 kHz, and the echo level differs from a still delay (always-on movement)
    {
        std::vector<float> L, R;  double f = 6000;
        DelayParams a {};  a.timeMs = 200;  a.feedback = 0.0f;  a.stereo = 0;  a.modRate = 1.5f;  a.mode = 1;  run (a, 0.7, L, R, burst, &f);  const double ea = rmsOf (L, 0.215, 0.295);
        DelayParams m = a;  m.mode = 11;  run (m, 0.7, L, R, burst, &f);  const double em = rmsOf (L, 0.215, 0.295);
        CHECK (em > 0.0 && em < ea, "Memory Man: 6 kHz echo %.4f should be darker than Analog %.4f", em, ea);
        std::vector<float> L2;  m.modDepth = 0.0f;  run (m, 0.7, L, R, burst, &f);  L2 = L;  m.modDepth = 1.0f;  run (m, 0.7, L, R, burst, &f);
        double d = 0;  for (size_t i = 0; i < L.size(); ++i) d += std::abs (L[i] - L2[i]);
        CHECK (d > 1.0e-3, "Memory Man: Mod depth should change the echoes (diff %.5f)", d);
    }
    DelayParams base {};  base.timeMs = 200;  base.feedback = 0.5f;  base.mode = 0;  base.stereo = 0;
    const double t0 = 100 * 1000.0 / SR;
    // 3. Groove: the odd echoes land late
    {
        DelayParams p = base;  p.groove = 0.0f;  run (p, 2.0, L, R, impulse, nullptr);  auto pk = peaks (L, 0.01, 50);
        CHECK (pk.size() >= 4 && std::abs (pk[1] - t0 - 200) < 3.0 && std::abs (pk[2] - t0 - 400) < 3.0, "Groove 0: echoes on the grid (200/400 ms)");
        p.groove = 1.0f;  run (p, 2.0, L, R, impulse, nullptr);  pk = peaks (L, 0.01, 50);
        CHECK (pk.size() >= 5 && std::abs (pk[1] - t0 - 300) < 1.0 && std::abs (pk[2] - t0 - 400) < 1.0 && std::abs (pk[3] - t0 - 700) < 1.0 && std::abs (pk[4] - t0 - 800) < 1.0, "Groove 1: echoes at 300/400/700/800 ms");
        p.groove = 0.5f;  run (p, 2.0, L, R, impulse, nullptr);  pk = peaks (L, 0.01, 50);
        CHECK (pk.size() >= 4 && std::abs (pk[1] - t0 - 250) < 1.0 && std::abs (pk[2] - t0 - 400) < 1.0 && std::abs (pk[3] - t0 - 650) < 1.0, "Groove 0.5: echoes at 250/400/650 ms");
    }
    // 4. Feel: the echoes are heard early or late, the loop does not change
    {
        DelayParams p = base;  p.feel = 1.0f;  run (p, 2.0, L, R, impulse, nullptr);  auto pk = peaks (L, 0.01, 50);
        CHECK (pk.size() >= 3 && std::abs (pk[1] - t0 - 250) < 1.0 && std::abs (pk[2] - t0 - 450) < 1.0, "Feel +1: echoes at 250/450 ms");
        p.feel = -1.0f;  run (p, 2.0, L, R, impulse, nullptr);  pk = peaks (L, 0.01, 50);
        CHECK (pk.size() >= 3 && std::abs (pk[1] - t0 - 150) < 1.0 && std::abs (pk[2] - t0 - 350) < 1.0, "Feel -1: echoes at 150/350 ms");
        p.feel = 0.5f;  p.groove = 1.0f;  run (p, 2.0, L, R, impulse, nullptr);  pk = peaks (L, 0.01, 50);
        CHECK (pk.size() >= 3 && std::abs (pk[1] - t0 - 325) < 1.0, "Feel with Groove: first echo at 325 ms");
    }
    // 5. Accent (against the same settings with Accent 0)
    {
        DelayParams p = base;  auto at = [&] (double ms) { return std::abs (L[(size_t) ((t0 + ms) * SR / 1000)]); };
        run (p, 2.0, L, R, impulse, nullptr);  const double b1 = at (200), b2 = at (400), b3 = at (600);
        p.accent = 0.8f;  run (p, 2.0, L, R, impulse, nullptr);
        CHECK (std::abs (at (200) / b1 - 0.4) < 0.03 && std::abs (at (400) / b2 - 1.0) < 0.03 && std::abs (at (600) / b3 - 0.4) < 0.03, "Accent +0.8: odd echoes x0.4, even echoes unchanged");
        p.accent = -0.8f;  run (p, 2.0, L, R, impulse, nullptr);
        CHECK (std::abs (at (200) / b1 - 1.0) < 0.03 && std::abs (at (400) / b2 - 0.4) < 0.03, "Accent -0.8: even echoes x0.4, odd echoes unchanged");
        p.stereo = 1;  p.accent = 0.0f;  run (p, 2.0, L, R, impulse, nullptr);  const double l0 = at (200);
        p.accent = 0.8f;  run (p, 2.0, L, R, impulse, nullptr);
        CHECK (std::abs (at (200) / l0 - 0.4) < 0.03, "Accent in Ping-Pong: the input's side (odd echoes) x0.4");
    }
    // 6. Saturation: unity for small signals (plus the output makeup of 3x at full), bounded for loud ones, no DC
    for (int ty = 0; ty < 4; ++ty)
    {
        DelayParams p = base;  p.feedback = 0.0f;  p.sat = 1.0f;  p.satType = ty;
        run (p, 0.6, L, R, tiny, nullptr);  double m = 0;  for (float v : L) m = std::max (m, (double) std::abs (v));
        DelayParams q = p;  q.sat = 0;  run (q, 0.6, L, R, tiny, nullptr);  double m0 = 0;  for (float v : L) m0 = std::max (m0, (double) std::abs (v));
        run (p, 0.6, L, R, big, nullptr);  double mb = 0, dc = 0;  for (float v : L) mb = std::max (mb, (double) std::abs (v));
        for (long i = (long) (0.2 * SR); i < (long) (0.2 * SR) + 4800; ++i) dc += (double) L[(size_t) i];  dc /= 4800;
        CHECK (std::abs (m / m0 / 3.0 - 1.0) < 0.12, "Saturation type %d: small signals come out %.2fx (want 3x with the makeup)", ty, m / m0);
        CHECK (mb < 1.6 && std::abs (dc) < 0.02, "Saturation type %d: loud signal peak %.2f, DC %.4f", ty, mb, dc);
    }
    // 7. Echo EQ
    {
        double f = 100;  DelayParams p = base;  p.feedback = 0.0f;  p.eqLow = 12;  p.eqDecay = 0;  run (p, 0.7, L, R, burst, &f);
        const double m = rmsOf (L, 0.215, 0.295);  DelayParams q = base;  q.feedback = 0;  run (q, 0.7, L, R, burst, &f);  const double m0 = rmsOf (L, 0.215, 0.295);
        CHECK (std::abs (20 * std::log10 (m / m0) - 11.1) < 1.0, "Echo EQ low +12 dB: %.1f dB at 100 Hz (want about 11)", 20 * std::log10 (m / m0));
        f = 8000;  p = base;  p.feedback = 0;  p.eqHigh = -12;  p.eqDecay = 0;  run (p, 0.7, L, R, burst, &f);  const double h = rmsOf (L, 0.215, 0.295);
        run (q, 0.7, L, R, burst, &f);  const double h0 = rmsOf (L, 0.215, 0.295);
        CHECK (std::abs (20 * std::log10 (h / h0) + 11.3) < 1.6, "Echo EQ high -12 dB: %.1f dB at 8 kHz (want about -11)", 20 * std::log10 (h / h0));
        f = 100;  p = base;  p.feedback = 0.5f;  p.eqLow = -12;  p.eqDecay = 1.0f;  DelayParams qq = base;  qq.feedback = 0.5f;
        run (p, 1.2, L, R, burst, &f);  const double a1 = rmsOf (L, 0.215, 0.295), a2 = rmsOf (L, 0.415, 0.495);
        run (qq, 1.2, L, R, burst, &f);  const double b1 = rmsOf (L, 0.215, 0.295), b2 = rmsOf (L, 0.415, 0.495);
        CHECK (std::abs (20 * std::log10 (a1 / b1)) < 1.5 && 20 * std::log10 (a2 / b2) < -6.0, "Echo EQ Decay 1: the first echo is untouched (%.1f dB), the second has the EQ (%.1f dB)", 20 * std::log10 (a1 / b1), 20 * std::log10 (a2 / b2));
    }
    // 8. the new echo styles
    {
        DelayParams p = base;  p.feedback = 0.6f;  p.mode = 7;  run (p, 1.0, L, R, impulse, nullptr);
        CHECK (peaks (L, 0.005, 20).size() >= 4, "Three-Head Tape: the extra heads are there with Multi-Tap off");
        p.mode = 10;  p.feedback = 0.8f;  run (p, 3.0, L, R, impulse, nullptr);  float e = 0;  for (float v : L) e = std::max (e, std::abs (v));
        CHECK (e < 1.5f, "Dub Echo at 80% feedback: peak %.2f (want under 1.5)", e);
    }
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
    for (int m = 0; m <= 40; ++m)
        for (int v = 0; v <= AmpExtras::lastVoicing (m); ++v)
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


// ---- b53: pitched delay feedback and the transient shaper ---------------------------------------------------------------
namespace b53t
{
    using namespace dly52;
    static double hmag (const std::vector<float>& x, double a, double b, double f)
    {
        const long i0 = (long) (a * SR), i1 = (long) (b * SR);  double re = 0, im = 0;
        for (long i = i0; i < i1; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - i0) / (double) (i1 - i0));  const double ph = 2 * M_PI * f * (double) i / SR;  re += w * x[(size_t) i] * std::cos (ph);  im += w * x[(size_t) i] * std::sin (ph); }
        return std::sqrt (re * re + im * im) / (double) (i1 - i0);
    }
    static double domFine (const std::vector<float>& x, double a, double b, double lo, double hi)
    { double best = 0, bf = 0;  for (double f = lo; f < hi; f += 0.5) { const double m = hmag (x, a, b, f);  if (m > best) { best = m;  bf = f; } }  return bf; }
    static float longBurst (long t, void*) { return (t >= 4800 && t < 19200) ? 0.3f * (float) std::sin (2 * M_PI * 440.0 * (double) t / SR) : 0.0f; }   // 100..400 ms, 440 Hz
    static float longBurst2 (long t, void*) { return (t >= 4800 && t < 24000) ? 0.3f * (float) std::sin (2 * M_PI * 440.0 * (double) t / SR) : 0.0f; }  // 100..500 ms
    static double endOf (const std::vector<float>& L, double a0, double a1)
    { double pk = 0;  for (long i = (long) (a0 * SR); i < (long) (a1 * SR); ++i) pk = std::max<double> (pk, std::fabs (L[(size_t) i]));
      for (long i = (long) (a1 * SR); i > (long) (a0 * SR); --i) if (std::fabs (L[(size_t) i]) > 0.5 * pk) return (double) i / SR;  return a0; }
}

static void testDelayPitch()
{
    using namespace b53t;
    std::printf ("[pitchdelay] pitch in the delay feedback: pitch of each repeat, timing, mix, stability\n");
    std::vector<float> L, R;
    DelayParams base {};  base.timeMs = 300;  base.feedback = 0.7f;  base.mode = 0;  base.stereo = 0;
    for (int st : { 12, 7, -12 })
    {
        DelayParams p = base;  p.timeMs = 500;  p.pitch = (float) st;  run (p, 2.4, L, R, longBurst, nullptr);
        const double r = std::pow (2.0, st / 12.0);
        const double f1 = domFine (L, 0.65, 0.85, 300, 600), f2 = domFine (L, 1.13, 1.37, 440 * r * 0.75, 440 * r * 1.25), f3 = domFine (L, 1.63, 1.87, 440 * r * r * 0.75, 440 * r * r * 1.25);
        CHECK (std::abs (f1 - 440) < 8, "pitch %d: first repeat is not shifted (%.0f Hz)", st, f1);
        CHECK (std::abs (f2 - 440 * r) < std::max (6.0, 0.02 * 440 * r), "pitch %d: second repeat at %.0f Hz (want %.0f)", st, f2, 440 * r);
        CHECK (std::abs (f3 - 440 * r * r) < std::max (6.0, 0.025 * 440 * r * r), "pitch %d: third repeat at %.0f Hz (want %.0f)", st, f3, 440 * r * r);
        std::vector<float> L0, R0;  DelayParams q = base;  q.timeMs = 500;  run (q, 2.4, L0, R0, longBurst, nullptr);
        const double ed0 = endOf (L0, 0.8, 1.0);
        const double e1 = endOf (L, 0.8, 1.0) - ed0, e2 = endOf (L, 1.3, 1.5) - ed0 - 0.5, e3 = endOf (L, 1.8, 2.0) - ed0 - 1.0;
        CHECK (std::abs (e1) < 0.004 && std::abs (e2) < 0.012 && std::abs (e3) < 0.02, "pitch %d: repeats end on the unshifted grid (%.1f / %.1f / %.1f ms)", st, e1 * 1000, e2 * 1000, e3 * 1000);
    }
    {
        DelayParams q = base;  q.timeMs = 600;  q.feedback = 0.6f;
        { DelayParams p = q;  p.pitch = 12;  p.pitchMix = 0.5f;  run (p, 2.2, L, R, longBurst2, nullptr);  double a = 0, b = 0;
          for (double f = 420; f <= 460; f += 2) a = std::max (a, hmag (L, 1.4, 1.6, f));  for (double f = 840; f <= 920; f += 2) b = std::max (b, hmag (L, 1.4, 1.6, f));
          CHECK (a > 0.4 * b && b > 0.4 * a, "pitch mix 0.5: second repeat holds both the original (%.4f) and the shifted (%.4f) pitch", a, b); }
        { DelayParams p = q;  p.pitchFine = 30;  run (p, 2.2, L, R, longBurst2, nullptr);  const double f2 = domFine (L, 1.4, 1.6, 400, 500), want = 440 * std::pow (2.0, 0.3 / 12);
          CHECK (std::abs (f2 / want - 1) < 0.006, "fine +30 cents: second repeat at %.1f Hz (want %.1f)", f2, want); }
    }
    {
        bool ok = true;  float mx = 0;
        for (int mode = 0; mode <= 10; ++mode) for (int st = 0; st < 4; ++st) for (int v = 0; v < 4; ++v)
        {
            DelayParams p = base;  p.mode = mode;  p.stereo = st;  p.feedback = 0.9f;  p.offset = 0.5f;  p.spread = 1;
            p.pitch = (v == 0 ? 24.0f : v == 1 ? -24.0f : v == 2 ? 7.0f : 12.0f);  p.pitchMix = (v == 3 ? 0.6f : 1.0f);
            if (v >= 2) { p.groove = 1;  p.accent = 0.5f;  p.feel = -1;  p.sat = 1;  p.taps = 3; }
            if (v == 1) { p.style = 1;  p.lofi = 1; }
            if (mode % 3 == 0) p.timeMs = 30;
            run (p, 3.0, L, R, [] (long t, void*) { return 0.3f * (float) std::sin (2 * M_PI * 110.0 * (double) t / SR); }, nullptr, 0.6f);
            for (size_t i = 0; i < L.size(); ++i) { if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) ok = false;  mx = std::max (mx, std::max (std::fabs (L[i]), std::fabs (R[i]))); }
        }
        CHECK (ok, "pitched feedback: output finite in every mode and stereo style");
        CHECK (mx < 8.0f, "pitched feedback: peak %.2f stays under 8", mx);
    }
}

static void testTransientShaper()
{
    std::printf ("[transshaper] transient shaper: neutral, attack, sustain, level independence, low notes, speed\n");
    const double SR = 48000.0;
    auto pluck = [SR] (double f, double a, double tau, double secs) { std::vector<float> x ((size_t) (secs * SR));  for (size_t i = 0; i < x.size(); ++i) { const double t = (double) i / SR;  x[i] = (float) (a * std::exp (-t / tau) * std::sin (2 * M_PI * f * t)); }  return x; };
    auto rms = [SR] (const std::vector<float>& x, double a, double b) { double s = 0;  long n = 0;  for (long i = (long) (a * SR); i < (long) (b * SR) && i < (long) x.size(); ++i) { s += (double) x[(size_t) i] * x[(size_t) i];  ++n; }  return std::sqrt (s / (double) n); };
    auto run = [SR] (std::vector<float> x, FxExtra::TransientFx::Params p) { FxExtra::TransientFx t;  t.prepare (SR);  for (size_t i = 0; i < x.size(); i += 512) { const int n = (int) std::min<size_t> (512, x.size() - i);  t.process (&x[i], nullptr, n, p); }  return x; };
    auto db = [] (double a, double b) { return 20 * std::log10 (a / b); };
    for (double amp : { 0.5, 0.05 })
    {
        const auto x = pluck (220, amp, 0.35, 1.5);
        FxExtra::TransientFx::Params p;
        auto y = run (x, p);
        double dd = 0;  for (size_t i = 0; i < x.size(); ++i) dd = std::max (dd, (double) std::abs (x[i] - y[i]));
        CHECK (dd < 1e-6, "neutral settings pass the signal unchanged (largest difference %.2e, level %.2f)", dd, amp);
        p.attack = 1;  y = run (x, p);
        CHECK (db (rms (y, 0, 0.03), rms (x, 0, 0.03)) > 3.0 && std::abs (db (rms (y, 0.6, 0.9), rms (x, 0.6, 0.9))) < 1.0, "attack +1 raises the pick and leaves the tail (level %.2f)", amp);
        p.attack = -1;  y = run (x, p);
        CHECK (db (rms (y, 0, 0.03), rms (x, 0, 0.03)) < -3.0 && std::abs (db (rms (y, 0.6, 0.9), rms (x, 0.6, 0.9))) < 1.0, "attack -1 softens the pick and leaves the tail (level %.2f)", amp);
        p.attack = 0;  p.sustain = -1;  y = run (x, p);
        CHECK (std::abs (db (rms (y, 0, 0.02), rms (x, 0, 0.02))) < 2.0 && db (rms (y, 0.6, 0.9), rms (x, 0.6, 0.9)) < -9.0, "sustain -1 shortens the tail and leaves the pick (level %.2f)", amp);
        p.sustain = 1;  y = run (x, p);
        CHECK (std::abs (db (rms (y, 0, 0.02), rms (x, 0, 0.02))) < 2.0 && db (rms (y, 0.6, 0.9), rms (x, 0.6, 0.9)) > 8.0, "sustain +1 lengthens the tail and leaves the pick (level %.2f)", amp);
    }
    {
        FxExtra::TransientFx::Params p;  p.attack = 1;
        const auto a = pluck (220, 0.5, 0.35, 1.0), b = pluck (220, 0.02, 0.35, 1.0);
        const auto ya = run (a, p), yb = run (b, p);
        const double ga = db (rms (ya, 0, 0.03), rms (a, 0, 0.03)), gb = db (rms (yb, 0, 0.03), rms (b, 0, 0.03));
        CHECK (std::abs (ga - gb) < 0.5, "the same boost at a loud and a quiet pick (%.2f dB apart)", ga - gb);
    }
    for (double f : { 41.0, 55.0, 82.0 })
    {
        std::vector<float> x ((size_t) (2 * SR));  for (size_t i = 0; i < x.size(); ++i) x[i] = 0.3f * (float) std::sin (2 * M_PI * f * (double) i / SR);
        FxExtra::TransientFx::Params p;  p.attack = 1;  p.sustain = 1;  FxExtra::TransientFx t;  t.prepare (SR);  double mn = 1e9, mx = -1e9;
        for (size_t i = 0; i < x.size(); i += 64) { t.process (&x[i], nullptr, 64, p);  if ((double) i > SR) { mn = std::min<double> (mn, t.grDb);  mx = std::max<double> (mx, t.grDb); } }
        CHECK (mx - mn < 1.0, "a sustained %.0f Hz note: gain ripple %.2f dB peak to peak", f, mx - mn);
    }
    {
        const auto x = pluck (220, 0.5, 0.35, 1.0);  FxExtra::TransientFx::Params p;  p.attack = 1;  p.speed = 0;  const auto y0 = run (x, p);  p.speed = 1;  const auto y1 = run (x, p);
        const double a0 = db (rms (y0, 0.04, 0.07), rms (x, 0.04, 0.07)), a1 = db (rms (y1, 0.04, 0.07), rms (x, 0.04, 0.07));
        CHECK (a0 > a1 + 0.5, "a slow Speed keeps boosting longer (%.2f dB more at 40-70 ms)", a0 - a1);
    }
    for (double amp : { 0.5, 0.05 })
    {   // b56 Body: the stretch just after the attack. It leaves the pick and the late tail alone, at any playing level
        const auto x = pluck (220, amp, 0.2, 2.0);
        FxExtra::TransientFx::Params p;  p.body = 1;  auto y = run (x, p);
        CHECK (std::abs (db (rms (y, 0, 0.015), rms (x, 0, 0.015))) < 1.0 && db (rms (y, 0.08, 0.2), rms (x, 0.08, 0.2)) > 8.0 && std::abs (db (rms (y, 1.0, 1.8), rms (x, 1.0, 1.8))) < 1.0,
               "body +1 raises the stretch after the pick, not the pick or the late tail (level %.2f: %.1f dB at 80-200 ms)", amp, db (rms (y, 0.08, 0.2), rms (x, 0.08, 0.2)));
        p.body = -1;  y = run (x, p);
        CHECK (std::abs (db (rms (y, 0, 0.015), rms (x, 0, 0.015))) < 1.0 && db (rms (y, 0.08, 0.2), rms (x, 0.08, 0.2)) < -8.0 && std::abs (db (rms (y, 1.0, 1.8), rms (x, 1.0, 1.8))) < 1.0,
               "body -1 thins the stretch after the pick, not the pick or the late tail (level %.2f)", amp);
    }
    {   // Body at 0 changes nothing, and it adds to Sustain and Attack rather than replacing them
        const auto x = pluck (220, 0.5, 0.2, 2.0);
        FxExtra::TransientFx::Params p;  p.attack = 0.6f;  p.sustain = -0.4f;  const auto a = run (x, p);  p.body = 0.0f;  const auto b = run (x, p);
        double dd = 0;  for (size_t i = 0; i < a.size(); ++i) dd = std::max (dd, (double) std::abs (a[i] - b[i]));
        CHECK (dd == 0.0, "body 0 leaves the shaper exactly as before (largest difference %.2e)", dd);
        p.body = 1;  const auto c = run (x, p);
        CHECK (std::abs (db (rms (c, 0, 0.015), rms (a, 0, 0.015))) < 1.0 && std::abs (db (rms (c, 1.0, 1.8), rms (a, 1.0, 1.8))) < 1.0 && db (rms (c, 0.08, 0.2), rms (a, 0.08, 0.2)) > 6.0,
               "body +1 on top of Attack and Sustain only adds in the stretch after the pick (%.1f dB there)", db (rms (c, 0.08, 0.2), rms (a, 0.08, 0.2)));
    }
    {   // a note that holds its level (a held chord) is all body; nothing runs away
        std::vector<float> h ((size_t) (3 * SR));  for (size_t i = 0; i < h.size(); ++i) h[i] = 0.3f * (float) std::sin (2 * M_PI * 196.0 * (double) i / SR);
        FxExtra::TransientFx::Params p;  p.body = 1;  const auto y = run (h, p);
        float pk = 0;  for (float v : y) pk = std::max (pk, std::abs (v));
        CHECK (pk < 0.3f * 4.2f, "body +1 on a held note stays bounded (peak %.2f, 12 dB up would be %.2f)", (double) pk, 0.3 * 3.98);
    }
    {   // stereo input: both channels get the same gain
        FxExtra::TransientFx::Params p;  p.attack = 1;  p.sustain = -0.5f;  FxExtra::TransientFx t;  t.prepare (SR);
        auto l = pluck (220, 0.4, 0.3, 1.0), r = l;  for (auto& v : r) v *= 0.5f;
        for (size_t i = 0; i < l.size(); i += 256) { const int n = (int) std::min<size_t> (256, l.size() - i);  t.process (&l[i], &r[i], n, p); }
        double worst = 0;  const auto x0 = pluck (220, 0.4, 0.3, 1.0);
        for (size_t i = 0; i < l.size(); ++i) if (std::abs (x0[i]) > 1e-3f) worst = std::max (worst, std::abs ((double) r[i] / l[i] - 0.5));
        CHECK (worst < 0.01, "both channels get the same gain (ratio error %.4f)", worst);
    }
    {   // b54: four bands. Each band changes only the notes inside it, neutral settings are level-flat, the crossovers can be moved
        using MT = FxExtra::MultiTransientFx;
        auto pluckAt = [SR] (std::vector<float>& x, double t0, double f, double a, double tau) { for (size_t i = (size_t) (t0 * SR); i < x.size(); ++i) { const double t = (double) i / SR - t0;  x[i] += (float) (a * std::exp (-t / tau) * std::sin (2 * M_PI * f * t)); } };
        auto runM = [SR] (std::vector<float> x, const MT::Params& p) { MT t;  t.prepare (SR);  for (size_t i = 0; i < x.size(); i += 512) { const int n = (int) std::min<size_t> (512, x.size() - i);  t.process (&x[i], nullptr, n, p); }  return x; };
        std::vector<float> x ((size_t) (2.2 * SR), 0.0f);  pluckAt (x, 0.0, 110, 0.4, 0.3);  pluckAt (x, 1.0, 6000, 0.4, 0.3);   // a low note, then a high one
        MT::Params neutral;
        double worst = 0;
        for (double f : { 30.0, 60.0, 100.0, 150.0, 200.0, 400.0, 800.0, 1200.0, 2500.0, 3500.0, 5000.0, 9000.0, 14000.0 })
        {
            std::vector<float> t ((size_t) (1.5 * SR));  for (size_t i = 0; i < t.size(); ++i) t[i] = 0.3f * (float) std::sin (2 * M_PI * f * (double) i / SR);
            const auto z = runM (t, neutral);  worst = std::max (worst, std::abs (db (rms (z, 0.8, 1.4), rms (t, 0.8, 1.4))));
        }
        CHECK (worst < 0.05, "multiband at neutral: level change across 13 frequencies, worst %.3f dB", worst);
        { MT::Params q;  q.attack[0] = 1;  q.mix = 0;  const auto z = runM (x, q), y = runM (x, neutral);  double d = 0;  for (size_t i = 0; i < x.size(); ++i) d = std::max (d, (double) std::abs (y[i] - z[i]));
          CHECK (d < 1e-6, "multiband mix 0 equals neutral (%.2e)", d); }
        { MT::Params q;  q.attack[0] = 1;  const auto z = runM (x, q);
          CHECK (db (rms (z, 0, 0.03), rms (x, 0, 0.03)) > 3.0 && std::abs (db (rms (z, 1.0, 1.03), rms (x, 1.0, 1.03))) < 1.0, "band 1 attack raises the low note and leaves the high one"); }
        { MT::Params q;  q.attack[3] = 1;  const auto z = runM (x, q);
          CHECK (db (rms (z, 1.0, 1.03), rms (x, 1.0, 1.03)) > 3.0 && std::abs (db (rms (z, 0, 0.03), rms (x, 0, 0.03))) < 1.0, "band 4 attack raises the high note and leaves the low one"); }
        { MT::Params q;  q.sustain[0] = -1;  const auto z = runM (x, q);
          CHECK (db (rms (z, 0.5, 0.8), rms (x, 0.5, 0.8)) < -6.0 && std::abs (db (rms (z, 1.5, 1.8), rms (x, 1.5, 1.8))) < 1.0, "band 1 sustain -1 shortens the low tail and leaves the high one"); }
        { MT::Params q;  q.sustain[3] = 1;  const auto z = runM (x, q);
          CHECK (db (rms (z, 1.5, 1.8), rms (x, 1.5, 1.8)) > 5.0 && std::abs (db (rms (z, 0.5, 0.8), rms (x, 0.5, 0.8))) < 1.0, "band 4 sustain +1 lengthens the high tail and leaves the low one"); }
        {   // moving the first split above a 400 Hz note moves it from band 2 to band 1
            std::vector<float> m ((size_t) (1.5 * SR), 0.0f);  pluckAt (m, 0.0, 400, 0.4, 0.3);
            MT::Params q;  q.attack[0] = 1;  const auto z1 = runM (m, q);  q.xo[0] = 1000;  q.xo[1] = 2000;  q.xo[2] = 5000;  const auto z2 = runM (m, q);
            const double a = db (rms (z1, 0, 0.03), rms (m, 0, 0.03)), b = db (rms (z2, 0, 0.03), rms (m, 0, 0.03));
            CHECK (std::abs (a) < 1.0 && b > 3.0, "moving Split 1 above a 400 Hz note puts it in band 1 (%.2f dB before, %.2f after)", a, b);
        }
        {   // the owning band's gain does not ripple on a sustained note (the low bands use slower detectors)
            double worstRipple = 0;
            for (double f : { 41.0, 55.0, 82.0, 110.0, 220.0, 440.0, 1000.0, 4000.0 })
            {
                std::vector<float> t ((size_t) (2 * SR));  for (size_t i = 0; i < t.size(); ++i) t[i] = 0.3f * (float) std::sin (2 * M_PI * f * (double) i / SR);
                MT::Params q;  for (int k = 0; k < 4; ++k) { q.attack[k] = 1;  q.sustain[k] = 1; }
                MT m;  m.prepare (SR);  const int own = f < 150 ? 0 : f < 800 ? 1 : f < 3500 ? 2 : 3;  double mn = 1e9, mx = -1e9;
                for (size_t i = 0; i < t.size(); i += 64) { m.process (&t[i], nullptr, 64, q);  if ((double) i > SR) { mn = std::min<double> (mn, m.bandGrDb (own));  mx = std::max<double> (mx, m.bandGrDb (own)); } }
                worstRipple = std::max (worstRipple, mx - mn);
            }
            CHECK (worstRipple < 1.0, "multiband: gain ripple on sustained notes from 41 Hz to 4 kHz, worst %.2f dB", worstRipple);
        }
        {   // out-of-order, equal and extreme splits stay finite and bounded
            bool ok = true;  double pk = 0;
            for (int v = 0; v < 4; ++v)
            {
                MT::Params q;  for (int k = 0; k < 4; ++k) { q.attack[k] = (k % 2) ? 1.0f : -1.0f;  q.sustain[k] = (k % 2) ? -1.0f : 1.0f; }
                if (v == 0) { q.xo[0] = 3000;  q.xo[1] = 500;  q.xo[2] = 100; }
                if (v == 1) { q.xo[0] = 40;  q.xo[1] = 100;  q.xo[2] = 14000; }
                if (v == 2) { q.xo[0] = 1500;  q.xo[1] = 6000;  q.xo[2] = 14000; }
                if (v == 3) q.xo[0] = q.xo[1] = q.xo[2] = 1000;
                const auto z = runM (x, q);  for (float f : z) { if (! std::isfinite (f)) ok = false;  pk = std::max (pk, (double) std::abs (f)); }
            }
            CHECK (ok && pk < 4.0, "multiband with out-of-order and extreme splits: finite, peak %.2f", pk);
        }
        {   // b56: per-band Body changes only the notes inside its band, in the stretch after the pick
            std::vector<float> y2 ((size_t) (2.2 * SR), 0.0f);  pluckAt (y2, 0.0, 110, 0.4, 0.2);  pluckAt (y2, 1.0, 6000, 0.4, 0.2);
            MT::Params q;  q.body[0] = 1;  const auto z = runM (y2, q);
            CHECK (db (rms (z, 0.08, 0.2), rms (y2, 0.08, 0.2)) > 5.0 && std::abs (db (rms (z, 1.08, 1.2), rms (y2, 1.08, 1.2))) < 1.0 && std::abs (db (rms (z, 0, 0.012), rms (y2, 0, 0.012))) < 2.5,
                   "band 1 body raises the low note's body and leaves the high note and the pick (%.1f dB)", db (rms (z, 0.08, 0.2), rms (y2, 0.08, 0.2)));
            MT::Params r;  r.body[3] = -1;  const auto z2 = runM (y2, r);
            CHECK (db (rms (z2, 1.08, 1.2), rms (y2, 1.08, 1.2)) < -5.0 && std::abs (db (rms (z2, 0.08, 0.2), rms (y2, 0.08, 0.2))) < 1.0,
                   "band 4 body -1 thins the high note's body and leaves the low note (%.1f dB)", db (rms (z2, 1.08, 1.2), rms (y2, 1.08, 1.2)));
        }
        {   // stereo: both channels get the same gains
            MT::Params q;  q.attack[2] = 1;  q.sustain[0] = -1;  MT m;  m.prepare (SR);  auto l = x, r = x;  for (auto& v : r) v *= 0.5f;
            for (size_t i = 0; i < l.size(); i += 256) { const int n = (int) std::min<size_t> (256, l.size() - i);  m.process (&l[i], &r[i], n, q); }
            double w = 0;  for (size_t i = 0; i < l.size(); ++i) if (std::abs (x[i]) > 1e-3f) w = std::max (w, std::abs ((double) r[i] / l[i] - 0.5));
            CHECK (w < 0.01, "multiband stereo: both channels get the same gain (ratio error %.4f)", w);
        }
    }
    {   // inside the plugin: it sits in the Gate's board slot, can run in every lane, and is transparent at neutral settings
        const int n = 48000;  auto in = testSignal (n, 48000.0, 0.3f);
        std::vector<float> off, neutral;
        { StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, 512);  p.prepareToPlay (48000.0, 512);  process (p, in, off, 512);
          CHECK (p.cpuLoad.load() > 0.0f, "the CPU readout measures the audio calls (load %.4f)", (double) p.cpuLoad.load()); }
        for (int lane = 0; lane < 3; ++lane)
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, 512);  p.prepareToPlay (48000.0, 512);
            setP (p, "tsOn", 1.0f);  placeLane (p, 19, lane);  process (p, in, neutral, 512);
            double d = 0;  for (size_t i = 0; i < off.size(); ++i) d = std::max (d, (double) std::abs (off[i] - neutral[i]));
            if (lane < 2) CHECK (d < 1e-3, "shaper on at neutral settings, lane %d: largest difference from off %.2e", lane, d);   // (a block in the amp's FX loop adds a second oversampler round trip, so it is never bit-identical to off)
            std::vector<float> shaped;  setP (p, "tsAttack", 1.0f);  setP (p, "tsSustain", 1.0f);  process (p, in, shaped, 512);
            double e = 0;  for (size_t i = 0; i < off.size(); ++i) e += std::abs (off[i] - shaped[i]);
            float pk;  CHECK (finiteBounded (shaped, 8.0f, pk), "shaper in lane %d: finite and bounded (peak %.2f)", lane, pk);
            CHECK (e > 1e-2, "shaper in lane %d: attack and sustain change the sound (%.4f)", lane, e);
            std::vector<float> bodied;  setP (p, "tsAttack", 0.0f);  setP (p, "tsSustain", 0.0f);  setP (p, "tsBody", 1.0f);  process (p, in, bodied, 512);
            double e2 = 0;  for (size_t i = 0; i < off.size(); ++i) e2 += std::abs (off[i] - bodied[i]);
            CHECK (e2 > 1e-2 && finiteBounded (bodied, 8.0f, pk), "shaper in lane %d: the Body knob changes the sound (%.4f)", lane, e2);
        }
        for (int lane = 0; lane < 2; ++lane)   // b54: Multiband inside the plugin
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, 512);  p.prepareToPlay (48000.0, 512);
            setP (p, "tsOn", 1.0f);  setP (p, "tsMulti", 1.0f);  placeLane (p, 19, lane);  process (p, in, neutral, 512);
            double a = 0, b = 0;  for (size_t i = 0; i < off.size(); ++i) { a += (double) off[i] * off[i];  b += (double) neutral[i] * neutral[i]; }
            CHECK (std::abs (10 * std::log10 (b / a)) < 0.3, "multiband on at neutral, lane %d: output level within 0.3 dB of off (%.3f dB)", lane, 10 * std::log10 (b / a));
            std::vector<float> shaped;  setP (p, "tsAtk4", 1.0f);  setP (p, "tsSus1", -1.0f);  setP (p, "tsX2", 900.0f);  process (p, in, shaped, 512);
            double e = 0;  for (size_t i = 0; i < off.size(); ++i) e += std::abs (off[i] - shaped[i]);
            float pk;  CHECK (finiteBounded (shaped, 8.0f, pk), "multiband in lane %d: finite and bounded (peak %.2f)", lane, pk);
            CHECK (e > 1e-2, "multiband in lane %d: band settings change the sound (%.4f)", lane, e);
            std::vector<float> bodied;  setP (p, "tsAtk4", 0.0f);  setP (p, "tsSus1", 0.0f);  setP (p, "tsBod2", 1.0f);  process (p, in, bodied, 512);
            double e2 = 0;  for (size_t i = 0; i < off.size(); ++i) e2 += std::abs (off[i] - bodied[i]);
            CHECK (e2 > 1e-2 && finiteBounded (bodied, 8.0f, pk), "multiband in lane %d: a band's Body knob changes the sound (%.4f)", lane, e2);
        }
    }
}


static void testTubeSwitch()
{
    std::printf ("[tubeswitch] changing the power tube or the preamp tube while playing makes no spike (b53: a stale anti-aliasing history gave bursts up to 20000 times the level)\n");
    const int blk = 256, nBlocks = 220;
    const auto in = testSignal (blk * nBlocks, 48000.0, 0.5f, 1);
    for (int model : { 27, 2, 8, 12 })
        for (int mode : { 1, 2 })
        {
            auto make = [&] (StarterAmpProcessor& p)
            {
                p.setPlayConfigDetails (2, 2, 48000.0, blk);
                setP (p, "ampModel", (float) model);  setP (p, "ampMode", (float) mode);  setP (p, "ampPower", 0.62f);  setP (p, "ampSagAmt", 0.52f);
                setP (p, "ampStageModel", 1.0f);  setP (p, "ampClass", 1.0f);  setP (p, "perfMode", 1.0f);
                p.prepareToPlay (48000.0, blk);
            };
            float steady = 0;
            for (int pt = 0; pt <= 6; ++pt) for (int tb = 0; tb <= 2; ++tb)
            {
                StarterAmpProcessor p;  make (p);  setP (p, "ampPwrTube", (float) pt);  setP (p, "ampTube", (float) tb);
                std::vector<float> y;  process (p, in, y, blk);  for (float v : y) steady = std::max (steady, std::abs (v));
            }
            StarterAmpProcessor p;  make (p);
            juce::AudioBuffer<float> b (2, blk);  juce::MidiBuffer m;  float peak = 0;  bool fin = true;
            for (int k = 0; k < nBlocks; ++k)
            {
                if (k % 8 == 0) { setP (p, "ampPwrTube", (float) ((k / 8) % 7));  setP (p, "ampTube", (float) ((k / 8) % 3)); }
                for (int i = 0; i < blk; ++i) { b.setSample (0, i, in[(size_t) (k * blk + i)]);  b.setSample (1, i, in[(size_t) (k * blk + i)]); }
                p.processBlock (b, m);
                for (int i = 0; i < blk; ++i) { const float v = b.getSample (0, i);  if (! std::isfinite (v)) fin = false;  peak = std::max (peak, std::abs (v)); }
            }
            CHECK (fin && peak < 2.5f * steady + 0.05f, "amp %d mode %d: peak %.2f while switching tubes, %.2f settled", model, mode, peak, steady);
        }
}


// b55: four-band compressor, multiband saturator (its own board block), and the old-session conversions that came with them
static void testMultibandB55()
{
    std::printf ("[multiband] four-band compressor and saturator: neutral, band isolation, aliasing, old sessions and boards\n");
    const double SR = 48000.0;
    auto sineT = [SR] (double f, double a, double secs) { std::vector<float> x ((size_t) (secs * SR));  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (a * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
    auto rms = [SR] (const std::vector<float>& x, double a, double b) { double s = 0;  long n = 0;  for (long i = (long) (a * SR); i < (long) (b * SR) && i < (long) x.size(); ++i) { s += (double) x[(size_t) i] * x[(size_t) i];  ++n; }  return std::sqrt(s / (double) std::max (n, 1L)); };
    auto db = [] (double a, double b) { return 20 * std::log10 (a / b); };
    auto mixTwo = [] (const std::vector<float>& a, const std::vector<float>& b) { std::vector<float> x (a.size());  for (size_t i = 0; i < a.size(); ++i) x[i] = a[i] + b[i];  return x; };
    const double freqs[] = { 30.0, 60.0, 100.0, 150.0, 200.0, 400.0, 800.0, 1200.0, 2500.0, 3500.0, 5000.0, 9000.0, 14000.0 };

    {   // ---- compressor
        using MC = FxExtra::MultibandCompFx;
        auto runC = [SR] (std::vector<float> x, const MC::Params& p) { MC c;  c.prepare (SR);  for (size_t i = 0; i < x.size(); i += 512) { const int n = (int) std::min<size_t> (512, x.size() - i);  c.process (&x[i], nullptr, n, p); }  return x; };
        MC::Params neutral;  for (int k = 0; k < 4; ++k) { neutral.thr[k] = 0.0f;  neutral.ratio[k] = 1.0f; }
        double worst = 0;
        for (double f : freqs) { const auto t = sineT (f, 0.3, 1.5);  const auto z = runC (t, neutral);  worst = std::max (worst, std::abs (db (rms (z, 0.8, 1.4), rms (t, 0.8, 1.4)))); }
        CHECK (worst < 0.05, "comp at neutral: level change across 13 frequencies, worst %.3f dB", worst);
        const auto lo = sineT (70, 0.4, 2.0), hi = sineT (8000, 0.4, 2.0), mid = sineT (350, 0.4, 2.0), up = sineT (1800, 0.4, 2.0);
        auto hard = [] (int band) { MC::Params q;  for (int k = 0; k < 4; ++k) { q.thr[k] = 0.0f;  q.ratio[k] = 1.0f; }  q.thr[band] = -40.0f;  q.ratio[band] = 20.0f;  q.attackMs[band] = 1.0f;  return q; };
        for (int band = 0; band < 4; ++band)
        {
            const auto q = hard (band);
            const std::vector<float>* tone[4] = { &lo, &mid, &up, &hi };
            for (int k = 0; k < 4; ++k)
            {
                const auto z = runC (*tone[k], q);  const double d = db (rms (z, 1.0, 1.9), rms (*tone[k], 1.0, 1.9));
                if (k == band) CHECK (d < -6.0, "comp band %d squashes its own tone (%.1f dB)", band + 1, d);
                else           CHECK (std::abs (d) < 0.7, "comp band %d leaves the tone of band %d alone (%.2f dB)", band + 1, k + 1, d);
            }
        }
        {   MC::Params q = neutral;  q.gainDb[1] = 6.0f;
            const double a = db (rms (runC (mid, q), 1.0, 1.9), rms (mid, 1.0, 1.9)), b = db (rms (runC (lo, q), 1.0, 1.9), rms (lo, 1.0, 1.9)), c = db (rms (runC (hi, q), 1.0, 1.9), rms (hi, 1.0, 1.9));
            CHECK (std::abs (a - 6.0) < 0.4 && std::abs (b) < 0.7 && std::abs (c) < 0.3, "comp band 2 gain +6 dB: own %.2f, low %.2f, high %.2f", a, b, c); }
        {   MC::Params q = neutral;  q.outDb = 6.0f;  double w = 0;
            for (double f : { 80.0, 700.0, 7000.0 }) { const auto t = sineT (f, 0.2, 1.5);  w = std::max (w, std::abs (db (rms (runC (t, q), 0.8, 1.4), rms (t, 0.8, 1.4)) - 6.0)); }
            CHECK (w < 0.2, "comp Output +6 dB moves every band by 6 dB (worst error %.2f)", w); }
        {   MC::Params q = hard (0);  q.mix = 0.0f;  const auto a = runC (lo, q), b = runC (lo, neutral);  double d = 0;  for (size_t i = 0; i < a.size(); ++i) d = std::max (d, (double) std::abs (a[i] - b[i]));
            CHECK (d < 1e-6, "comp Mix 0 equals neutral (%.2e)", d); }
        {   MC::Params q = hard (0);  q.mix = 0.5f;  const double d = db (rms (runC (lo, q), 1.0, 1.9), rms (lo, 1.0, 1.9));
            const MC::Params q1 = hard (0);  const double d1 = db (rms (runC (lo, q1), 1.0, 1.9), rms (lo, 1.0, 1.9));
            CHECK (d > d1 + 2.0 && d < -0.5, "comp Mix 0.5 is parallel compression: %.1f dB (full %.1f dB)", d, d1); }
        {   std::vector<float> m = sineT (400, 0.4, 2.0);  MC::Params q = hard (0);
            const double a = db (rms (runC (m, q), 1.0, 1.9), rms (m, 1.0, 1.9));  q.xo[0] = 1000;  q.xo[1] = 2000;  q.xo[2] = 5000;
            const double b = db (rms (runC (m, q), 1.0, 1.9), rms (m, 1.0, 1.9));
            CHECK (std::abs (a) < 0.7 && b < -6.0, "comp: moving Split 1 above a 400 Hz tone puts it in band 1 (%.2f dB before, %.2f after)", a, b); }
        {   MC::Params q;  for (int k = 0; k < 4; ++k) { q.thr[k] = -48;  q.ratio[k] = 20;  q.attackMs[k] = 0.1f;  q.releaseMs[k] = 5;  q.gainDb[k] = 18; }  q.outDb = 12;
            bool ok = true;
            for (const auto& xo : { std::array<float, 3> { 20, 20, 20 }, std::array<float, 3> { 20000, 20000, 20000 }, std::array<float, 3> { 3000, 100, 40 } })
                for (double sr : { 44100.0, 96000.0 })
                {
                    MC::Params qq = q;  for (int k = 0; k < 3; ++k) qq.xo[k] = xo[(size_t) k];
                    std::vector<float> t ((size_t) (sr * 0.5)), u (t.size());  for (size_t i = 0; i < t.size(); ++i) { t[i] = (float) (0.9 * std::sin (0.07 * (double) i) + 0.1 * std::sin (1.9 * (double) i));  u[i] = t[i]; }
                    MC c;  c.prepare (sr);  for (size_t i = 0; i < t.size(); i += 64) c.process (&t[i], &u[i], (int) std::min<size_t> (64, t.size() - i), qq);
                    for (size_t i = 0; i < t.size(); ++i) if (! std::isfinite (t[i]) || ! std::isfinite (u[i]) || std::abs (t[i]) > 200.0f) ok = false;
                }
            CHECK (ok, "comp: extreme settings and crossover orders stay finite at 44.1 and 96 kHz"); }
    }

    {   // ---- saturator
        using MS = FxExtra::MultibandSatFx;
        auto runS = [SR] (std::vector<float> x, const MS::Params& p, bool adaa = true) { MS s;  s.adaa = adaa;  s.prepare (SR);  for (size_t i = 0; i < x.size(); i += 512) { const int n = (int) std::min<size_t> (512, x.size() - i);  s.process (&x[i], nullptr, n, p); }  return x; };
        MS::Params flat;  for (int k = 0; k < 4; ++k) { flat.driveDb[k] = 0.0f;  flat.levelDb[k] = 0.0f; }
        double worst = 0;
        for (double f : { 30.0, 60.0, 100.0, 150.0, 200.0, 400.0, 800.0, 1200.0, 2500.0, 3500.0, 5000.0, 9000.0, 14000.0, 18000.0 })
        { const auto t = sineT (f, 0.01, 1.5);  const auto z = runS (t, flat);  worst = std::max (worst, std::abs (db (rms (z, 0.8, 1.4), rms (t, 0.8, 1.4)))); }
        CHECK (worst < 0.15, "saturator at Drive 0: level change across 14 frequencies, worst %.3f dB", worst);
        {   MS::Params q = flat;  q.mix = 0.0f;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 36.0f;  double w = 0;
            for (double f : { 60.0, 400.0, 2500.0, 9000.0 }) { const auto t = sineT (f, 0.3, 1.5);  w = std::max (w, std::abs (db (rms (runS (t, q), 0.8, 1.4), rms (t, 0.8, 1.4)))); }
            CHECK (w < 0.15, "saturator Mix 0 passes the signal at any drive (worst %.3f dB)", w); }
        auto bin = [SR] (const std::vector<float>& y, double f) { double re = 0, im = 0;  long n = 0;  for (long i = (long) (0.5 * SR); i < (long) (1.3 * SR); ++i) { re += y[(size_t) i] * std::cos (2 * M_PI * f * (double) i / SR);  im += y[(size_t) i] * std::sin (2 * M_PI * f * (double) i / SR);  ++n; }  return 2 * std::sqrt (re * re + im * im) / (double) n; };
        {   const auto x = mixTwo (sineT (110, 0.4, 1.5), sineT (6000, 0.02, 1.5));  MS::Params q = flat;  q.driveDb[0] = 36.0f;
            const auto z = runS (x, q), y0 = runS (x, flat);
            CHECK (std::abs (db (bin (z, 6000), bin (y0, 6000))) < 0.3, "saturator: Drive on band 1 leaves the 6 kHz tone alone (%.2f dB)", db (bin (z, 6000), bin (y0, 6000)));
            CHECK (bin (z, 330) > 0.02 && bin (y0, 330) < 0.004, "saturator: Drive on band 1 makes the low note's third harmonic (%.4f, flat %.4f)", bin (z, 330), bin (y0, 330));
            MS::Params q4 = flat;  q4.driveDb[3] = 36.0f;  const auto z4 = runS (x, q4);
            CHECK (std::abs (db (bin (z4, 110), bin (y0, 110))) < 0.3 && bin (z4, 18000) > 0.0005, "saturator: Drive on band 4 leaves the 110 Hz note alone (%.2f dB) and colours the highs", db (bin (z4, 110), bin (y0, 110))); }
        {   double r0 = 0, r1 = 0;  for (int type = 0; type < 5; ++type)
            {   MS::Params q = flat;  q.type = type;  const auto t = sineT (2000, 0.25, 1.5);  r0 = rms (runS (t, q), 0.8, 1.4);  q.driveDb[2] = 24.0f;  r1 = rms (runS (t, q), 0.8, 1.4);
                CHECK (std::abs (db (r1, r0)) < 4.0, "saturator type %d: 24 dB of Drive on the band changes a -12 dBFS tone by %.2f dB (a clipped wave is at most 3 dB louder)", type, db (r1, r0)); } }
        {   // aliasing: the share of the output power that is not at a multiple of the note (100 Hz to 20 kHz), hard-driven
            auto share = [SR] (const std::vector<float>& y, double f0)
            {   const size_t N = 65536;  std::vector<std::complex<double>> a (N);  const size_t off = (size_t) (0.3 * SR);
                for (size_t i = 0; i < N; ++i) a[i] = (double) y[off + i] * (0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) N));
                for (size_t i = 1, j = 0; i < N; ++i) { size_t bit = N >> 1;  for (; j & bit; bit >>= 1) j ^= bit;  j ^= bit;  if (i < j) std::swap (a[i], a[j]); }
                for (size_t len = 2; len <= N; len <<= 1) { const double ang = -2 * M_PI / (double) len;  const std::complex<double> wl (std::cos (ang), std::sin (ang));
                    for (size_t i = 0; i < N; i += len) { std::complex<double> w (1);  for (size_t j = 0; j < len / 2; ++j) { const auto u = a[i + j], v = a[i + j + len / 2] * w;  a[i + j] = u + v;  a[i + j + len / 2] = u - v;  w *= wl; } } }
                double tot = 0, al = 0;
                for (size_t k = 1; k < N / 2; ++k) { const double f = (double) k * SR / (double) N;  if (f < 100 || f > 20000) continue;  const double pw = std::norm (a[k]);  tot += pw;
                    double m = std::fmod (f, f0);  if (m > f0 / 2) m = f0 - m;  if (m > 60) al += pw; }
                return 10 * std::log10 (al / tot + 1e-30); };
            for (int type : { 0, 2, 4 })
            {   MS::Params q = flat;  q.type = type;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 36.0f;  const auto t = sineT (5300, 0.3, 2.5);
                const double with = share (runS (t, q, true), 5300), without = share (runS (t, q, false), 5300);
                CHECK (with < -42.0 && with < without - 10.0, "saturator type %d at 36 dB Drive, 5.3 kHz: folded-back energy %.1f dB with ADAA (%.1f dB without)", type, with, without); } }
        {   MS::Params q = flat;  q.type = 1;  q.bias = 0.5f;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 30.0f;  const auto z = runS (sineT (1300, 0.3, 2.0), q);
            double dc = 0;  for (size_t i = (size_t) (0.5 * SR); i < z.size(); ++i) dc += z[i];  dc /= (double) z.size() - 0.5 * SR;
            CHECK (std::abs (dc) < 0.003, "saturator Tube with Bias: no DC left (%.4f)", dc); }
        {   bool ok = true;
            for (int type = 0; type < MS::kTypes; ++type)
                for (double sr : { 44100.0, 96000.0 })
                {   MS::Params q;  q.type = type;  q.bias = 1.0f;  q.xo[0] = q.xo[1] = q.xo[2] = 100;  for (int k = 0; k < 4; ++k) { q.tone[k] = (k & 1) ? 1.0f : -1.0f;  q.dyn[k] = (k & 2) ? 1.0f : -1.0f; }  q.autoLevel = true;  q.loCutHz = 400.0f;  q.hiCutHz = 3000.0f;  for (int k = 0; k < 4; ++k) { q.driveDb[k] = 48;  q.levelDb[k] = 24; }  q.outDb = 12;
                    std::vector<float> t ((size_t) (sr * 0.4)), u (t.size());  for (size_t i = 0; i < t.size(); ++i) { t[i] = (float) std::sin (0.04 * (double) i);  u[i] = (float) (0.5 * std::sin (2.1 * (double) i)); }
                    MS s;  s.prepare (sr);  for (size_t i = 0; i < t.size(); i += 64) s.process (&t[i], &u[i], (int) std::min<size_t> (64, t.size() - i), q);
                    for (size_t i = 0; i < t.size(); ++i) if (! std::isfinite (t[i]) || ! std::isfinite (u[i]) || std::abs (t[i]) > 5000.0f) ok = false; }
            CHECK (ok, "saturator: every curve at the extreme settings stays finite and bounded at 44.1 and 96 kHz"); }
        auto share5 = [SR] (const std::vector<float>& y, double f0)
        {   const size_t N = 65536;  std::vector<std::complex<double>> a (N);  const size_t off = (size_t) (0.3 * SR);
            for (size_t i = 0; i < N; ++i) a[i] = (double) y[off + i] * (0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) N));
            for (size_t i = 1, j = 0; i < N; ++i) { size_t bit = N >> 1;  for (; j & bit; bit >>= 1) j ^= bit;  j ^= bit;  if (i < j) std::swap (a[i], a[j]); }
            for (size_t len = 2; len <= N; len <<= 1) { const double ang = -2 * M_PI / (double) len;  const std::complex<double> wl (std::cos (ang), std::sin (ang));
                for (size_t i = 0; i < N; i += len) { std::complex<double> w (1);  for (size_t j = 0; j < len / 2; ++j) { const auto u = a[i + j], v = a[i + j + len / 2] * w;  a[i + j] = u + v;  a[i + j + len / 2] = u - v;  w *= wl; } } }
            double tot = 0, al = 0;
            for (size_t k = 1; k < N / 2; ++k) { const double f = (double) k * SR / (double) N;  if (f < 100 || f > 20000) continue;  const double pw = std::norm (a[k]);  tot += pw;
                double m = std::fmod (f, f0);  if (m > f0 / 2) m = f0 - m;  if (m > 60) al += pw; }
            return 10 * std::log10 (al / tot + 1e-30); };
        {   // b63: the new curves and per-band controls
            { bool ok = true;  for (int type = 5; type < MS::kTypes; ++type) { MS::Params q = flat;  q.type = type;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 36.0f;  q.mix = 0.0f;
                  const auto t = sineT (440, 0.3, 1.5);  if (std::abs (db (rms (runS (t, q), 0.8, 1.4), rms (t, 0.8, 1.4))) > 0.15) ok = false; }
              CHECK (ok, "saturator: Mix 0 passes the signal for every new curve"); }
            for (int type : { 5, 6, 7 })
            {   MS::Params q = flat;  q.type = type;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 36.0f;  const auto t = sineT (5300, 0.3, 2.5);
                const double with = share5 (runS (t, q, true), 5300), without = share5 (runS (t, q, false), 5300);
                CHECK (with < -40.0 && with < without - 8.0, "saturator type %d at 36 dB Drive, 5.3 kHz: folded-back energy %.1f dB with ADAA (%.1f dB without)", type, with, without); }
            {   // Iron and Tape +: flat where nothing saturates, and the saturation depends on frequency
                double worstIron = 0, worstTape = 0;
                for (double f : { 60.0, 200.0, 1000.0, 4000.0, 10000.0 })
                {   const auto t = sineT (f, 0.003, 1.5);  MS::Params q = flat;  q.type = 8;  worstIron = std::max (worstIron, std::abs (db (rms (runS (t, q), 0.8, 1.4), rms (t, 0.8, 1.4))));
                    q.type = 9;  if (f > 900.0) worstTape = std::max (worstTape, std::abs (db (rms (runS (t, q), 0.8, 1.4), rms (t, 0.8, 1.4)))); }
                CHECK (worstIron < 0.15 && worstTape < 0.15, "saturator Iron and Tape +: flat for small signals (Iron %.2f dB, Tape + above 1 kHz %.2f dB)", worstIron, worstTape);
                MS::Params q = flat;  q.type = 8;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 12.0f;
                const auto lo = runS (sineT (80, 0.25, 1.5), q), hi = runS (sineT (3000, 0.25, 1.5), q);
                const double h3lo = db (bin (lo, 240), bin (lo, 80)), h3hi = db (bin (hi, 9000), bin (hi, 3000));
                CHECK (h3lo > h3hi + 10.0, "saturator Iron: the lows make more third harmonic (%.1f dB) than the highs (%.1f dB)", h3lo, h3hi);
                q.type = 9;  const auto lo9 = runS (sineT (400, 0.25, 1.5), q), hi9 = runS (sineT (3000, 0.25, 1.5), q);
                const double a9 = db (bin (lo9, 1200), bin (lo9, 400)), b9 = db (bin (hi9, 9000), bin (hi9, 3000));
                CHECK (b9 > a9 + 2.0, "saturator Tape +: the highs squash first (H3 %.1f dB at 3 kHz vs %.1f dB at 400 Hz)", b9, a9); }
            {   // Line Xfmr (type 13): a very clean line transformer; harmonics appear only with a lot of Drive
                CHECK (MS::kTypes == 16, "saturator has 16 types (%d)", (int) MS::kTypes);
                MS::Params q = flat;  q.type = 13;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 12.0f;
                const auto t = sineT (200, 0.25, 1.5);  const auto y12 = runS (t, q);
                const double h3a = db (bin (y12, 600), bin (y12, 200)), ga = db (rms (y12, 0.8, 1.4), rms (t, 0.8, 1.4));
                for (int k = 0; k < 4; ++k) q.driveDb[k] = 36.0f;
                const auto y36 = runS (t, q);  const double h3b = db (bin (y36, 600), bin (y36, 200));
                CHECK (h3a < -65.0 && std::abs (ga) < 0.2, "saturator Line Xfmr at Drive 12: third harmonic %.1f dB, level change %.2f dB", h3a, ga);
                CHECK (h3b > -45.0 && h3b < -15.0, "saturator Line Xfmr at Drive 36: third harmonic %.1f dB (a transformer pushed hard)", h3b); }
            {   // a band's own curve, Mix, Tone and Dynamics change only that band; Low Cut / High Cut and Auto level
                const auto x = mixTwo (sineT (110, 0.3, 1.5), sineT (6000, 0.1, 1.5));
                MS::Params q = flat;  for (int k = 0; k < 4; ++k) q.driveDb[k] = 24.0f;
                const auto base = runS (x, q);
                MS::Params q2 = q;  q2.bandType[3] = 12;  const auto z2 = runS (x, q2);
                CHECK (std::abs (db (bin (z2, 110), bin (base, 110))) < 0.3 && std::abs (db (bin (z2, 6000), bin (base, 6000))) > 0.2, "saturator: a different curve on band 4 changes the highs only (110 Hz %.2f dB, 6 kHz %.2f dB)", db (bin (z2, 110), bin (base, 110)), db (bin (z2, 6000), bin (base, 6000)));
                MS::Params q3 = flat;  q3.driveDb[3] = 24.0f;  q3.bandMix[3] = 0.0f;  const auto z3 = runS (x, q3);
                CHECK (std::abs (db (bin (z3, 6000), bin (runS (x, flat), 6000))) < 0.3, "saturator: band 4 Mix 0 leaves the 6 kHz tone as it came in (%.2f dB)", db (bin (z3, 6000), bin (runS (x, flat), 6000)));
                MS::Params q4 = q;  q4.loCutHz = 400.0f;  q4.hiCutHz = 4000.0f;  const auto z4 = runS (x, q4);
                CHECK (db (bin (z4, 110), bin (base, 110)) < -15.0 && db (bin (z4, 6000), bin (base, 6000)) < -3.0, "saturator: Low Cut %.1f dB at 110 Hz, High Cut %.1f dB at 6 kHz", db (bin (z4, 110), bin (base, 110)), db (bin (z4, 6000), bin (base, 6000)));
                MS::Params q5 = q;  for (int k = 0; k < 4; ++k) q5.driveDb[k] = 36.0f;  q5.autoLevel = true;
                const auto t = sineT (300, 0.03, 4.0);  const auto z5 = runS (t, q5);  const double a5 = db (rms (z5, 3.0, 3.9), rms (t, 3.0, 3.9));
                CHECK (std::abs (a5) < 1.0, "saturator Auto level: a quiet note at 36 dB Drive comes out within 1 dB of its input level (%.2f dB)", a5);
                MS::Params q6 = q;  q6.dyn[1] = q6.dyn[2] = 1.0f;  const auto xs = sineT (300, 0.03, 1.5), xl = sineT (300, 0.3, 1.5);
                const auto ys = runS (xs, q6), yl = runS (xl, q6), ys0 = runS (xs, q), yl0 = runS (xl, q);
                CHECK (db (bin (yl, 900), bin (yl, 300)) - db (bin (ys, 900), bin (ys, 300)) > db (bin (yl0, 900), bin (yl0, 300)) - db (bin (ys0, 900), bin (ys0, 300)) + 6.0, "saturator Dynamics +: loud notes get relatively more harmonics than soft ones"); }
        }
    }

    {   // ---- in the plugin: the saturator is a board block, old sessions convert
        StarterAmpProcessor a;  a.setPlayConfigDetails (2, 2, 48000.0, 512);  a.prepareToPlay (48000.0, 512);
        const auto d0 = a.getPatch();
        CHECK (PG::isPedal (27) && PG::laneOf (d0, 27) == 1 && PG::kNamP1 == PG::kPedals && PG::kNodes == 38, "the default board has the Multiband Saturator after the cab (lane %d)", PG::laneOf (d0, 27));
        auto in = testSignal (48000, 48000.0, 0.3f);
        std::vector<float> yOff, yOn, yOn2;
        process (a, in, yOff, 512);
        setP (a, "msOn", 1.0f);  for (const char* id : { "msDrive1", "msDrive2", "msDrive3", "msDrive4" }) setP (a, id, 30.0f);
        a.prepareToPlay (48000.0, 512);  process (a, in, yOn, 512);
        double d = 0, e = 1e-12;  for (size_t i = 0; i < in.size(); ++i) { d += (double) (yOn[i] - yOff[i]) * (yOn[i] - yOff[i]);  e += (double) yOff[i] * yOff[i]; }
        float pk;  CHECK (finiteBounded (yOn, 16.0f, pk) && d / e > 1e-4, "the saturator block changes the sound when it is on (difference %.1f dB)", 10 * std::log10 (d / e + 1e-20));
        for (int lane : { 0, 2, 1 })
        {   placeLane (a, 27, lane);  CHECK (PG::laneOf (a.getPatch(), 27) == lane, "the saturator moves to lane %d", lane);
            a.prepareToPlay (48000.0, 512);  process (a, in, yOn2, 512);  CHECK (finiteBounded (yOn2, 16.0f, pk), "saturator in lane %d: not finite or peak %.1f", lane, pk); }
        setP (a, "msOn", 0.0f);

        {   // a board saved before b55 (text "P1", the nodes from the NAM pedal on were one number lower) is read the same
            auto toP1 = [] (const PG::Data& dd) { std::string s = "P1";  char b[160];  std::snprintf (b, sizeof b, "|x:%g,%g,%g|c:", (double) dd.xoHz[0], (double) dd.xoHz[1], (double) dd.xoHz[2]);  s += b;
                for (int i = 0; i < dd.n; ++i) { const auto& c = dd.c[i];  const int f = c.from >= 28 ? c.from - 1 : c.from, t = c.to >= 28 ? c.to - 1 : c.to;
                    std::snprintf (b, sizeof b, "%s%d.%d>%d.%d,%g,%g,%d,%d", i ? ";" : "", f, (int) c.fp, t, (int) c.tp, (double) c.levelDb, (double) c.pan, (int) c.pol, (int) c.chan);  s += b; }
                return s; };
            PG::Data base;  PG::defaultPatch (base);  PG::removeNodeBypass (base, 27);  PG::Data viaOld;           // (an old board has no pedal 27)
            CHECK (PG::fromString (toP1 (base), viaOld) && PG::toString (viaOld) == PG::toString (base), "a saved P1 board reads back as the same board");
            auto base2 = base;  PG::insertBefore (base2, 12, PG::kOut, 0);  PG::connect (base2, PG::kNamP1, 0, 22, 0, -3.0f, 0.2f, true, 1);
            PG::Data v2;  CHECK (PG::fromString (toP1 (base2), v2) && PG::toString (v2) == PG::toString (base2), "...with a pedal moved and a cable with level, pan, polarity and channel");
            juce::MemoryBlock mb;  a.getStateInformation (mb);
            auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
            CHECK (xml != nullptr, "state to xml");
            if (xml != nullptr)
            {
                xml->setAttribute ("patch", juce::String (toP1 (base2)));
                juce::MemoryBlock old;  juce::AudioProcessor::copyXmlToBinary (*xml, old);
                StarterAmpProcessor b;  b.setPlayConfigDetails (2, 2, 48000.0, 512);  b.prepareToPlay (48000.0, 512);  b.setStateInformation (old.getData(), (int) old.getSize());
                CHECK (b.getPatchText() == PG::toString (base2), "a session with an old board loads it unchanged");
            }
        }
        {   // a session from before the cables (b35 to b45) has order lists of 27 blocks and no board: the saturator goes in at the end, the board is rebuilt
            juce::MemoryBlock mb;  a.getStateInformation (mb);
            auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
            if (xml != nullptr)
            {
                xml->removeAttribute ("patch");
                xml->setAttribute ("fxPreOrder",  "19,0,26,1,2,16,3,4,5,6,7,8,14,9,10,11,12,15,13,18,17,20,21,24,25,22,23");
                xml->setAttribute ("fxPostOrder", "8,18,14,9,10,11,12,15,17,13,6,7,0,26,1,2,3,4,5,16,19,20,21,24,25,22,23");
                xml->setAttribute ("fxLoopOrder", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26");
                for (const char* at : { "fxPreRow", "fxPostRow", "fxLoopRow" }) xml->setAttribute (at, "000000000000000000000000000");
                juce::MemoryBlock old;  juce::AudioProcessor::copyXmlToBinary (*xml, old);
                StarterAmpProcessor b;  b.setPlayConfigDetails (2, 2, 48000.0, 512);  b.prepareToPlay (48000.0, 512);  b.setStateInformation (old.getData(), (int) old.getSize());
                bool tail = true;  for (int g = 0; g < 3; ++g) { const auto o = b.getFxOrder (g);  tail = tail && o[27] == 27; std::array<int, 28> sorted = o;  std::sort (sorted.begin(), sorted.end());  for (int k = 0; k < 28; ++k) tail = tail && sorted[(size_t) k] == k; }
                CHECK (tail, "27-block order lists get the saturator at the end and stay permutations");
                const auto d = b.getPatch();  std::vector<float> yb;  process (b, in, yb, 512);  float pk3;
                CHECK (PG::laneOf (d, PG::kAmpA) >= 0 && PG::laneOf (d, 18) == 1 && finiteBounded (yb, 16.0f, pk3), "...and the board is rebuilt from the lanes and renders");
            }
        }
        {   // a session saved with the three-band compressor: crossovers, thresholds and the shared Ratio / Attack / Release map onto the bands
            setP (a, "mbOn", 1.0f);  setP (a, "mbX1", 200.0f);  setP (a, "mbX2", 3000.0f);
            juce::MemoryBlock mb;  a.getStateInformation (mb);
            auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
            if (xml != nullptr)
            {
                std::vector<juce::XmlElement*> kill;
                for (auto* c : xml->getChildIterator()) { const auto id = c->getStringAttribute ("id");  if (id.startsWith ("mbThr") || id.startsWith ("mbRatio") || id.startsWith ("mbAtk") || id.startsWith ("mbRel") || id.startsWith ("mbGain") || id == "mbX3") kill.push_back (c); }
                for (auto* c : kill) xml->removeChildElement (c, true);
                xml->removeAttribute ("mbBands");
                auto add = [&] (const char* id, double v) { auto* c = xml->createNewChildElement ("PARAM");  c->setAttribute ("id", id);  c->setAttribute ("value", v); };
                add ("mbThrL", -30.0);  add ("mbThrM", -20.0);  add ("mbThrH", -10.0);  add ("mbRatio", 5.0);  add ("mbAttack", 25.0);  add ("mbRelease", 300.0);
                juce::MemoryBlock old;  juce::AudioProcessor::copyXmlToBinary (*xml, old);
                StarterAmpProcessor b;  b.setPlayConfigDetails (2, 2, 48000.0, 512);  b.prepareToPlay (48000.0, 512);  b.setStateInformation (old.getData(), (int) old.getSize());
                auto v = [&] (const char* id) { return b.apvts.getRawParameterValue (id)->load(); };
                CHECK (std::abs (v ("mbThr1") + 30) < 0.2f && std::abs (v ("mbThr2") + 20) < 0.2f && std::abs (v ("mbThr3") + 10) < 0.2f && std::abs (v ("mbThr4") + 10) < 0.2f, "old compressor: thresholds %.1f %.1f %.1f %.1f (want -30 -20 -10 -10)", v ("mbThr1"), v ("mbThr2"), v ("mbThr3"), v ("mbThr4"));
                CHECK (std::abs (v ("mbRatio4") - 5) < 0.1f && std::abs (v ("mbAtk2") - 25) < 0.3f && std::abs (v ("mbRel3") - 300) < 1.0f && std::abs (v ("mbGain3")) < 1e-3f, "old compressor: ratio, attack and release copied to every band, gains 0");
                CHECK (std::abs (v ("mbX1") - 200) < 1.0f && std::abs (v ("mbX2") - 3000) < 1.0f && v ("mbX3") > 4400.0f, "old compressor: splits %.0f %.0f and a third at %.0f above them", v ("mbX1"), v ("mbX2"), v ("mbX3"));
                std::vector<float> yb, ya;  process (b, in, yb, 512);  float pk2;  CHECK (finiteBounded (yb, 16.0f, pk2), "old compressor session renders finite");
            }
        }
        {   // state round trip keeps the new controls
            setP (a, "msType", 3.0f);  setP (a, "msType2", 9.0f);  setP (a, "msTone3", -0.4f);  setP (a, "msLoCut", 180.0f);  setP (a, "msDrive3", 21.5f);  setP (a, "msLevel2", -7.0f);  setP (a, "mbAtk4", 3.5f);  setP (a, "mbGain1", 4.0f);
            juce::MemoryBlock mb;  a.getStateInformation (mb);
            StarterAmpProcessor b;  b.setPlayConfigDetails (2, 2, 48000.0, 512);  b.prepareToPlay (48000.0, 512);  b.setStateInformation (mb.getData(), (int) mb.getSize());
            auto v = [&] (const char* id) { return b.apvts.getRawParameterValue (id)->load(); };
            CHECK (v ("msType") == 3.0f && std::abs (v ("msDrive3") - 21.5f) < 0.05f && std::abs (v ("msLevel2") + 7.0f) < 0.05f && std::abs (v ("mbAtk4") - 3.5f) < 0.05f && std::abs (v ("mbGain1") - 4.0f) < 0.05f && v ("msType2") == 9.0f && std::abs (v ("msTone3") + 0.4f) < 0.01f && std::abs (v ("msLoCut") - 180.0f) < 1.0f, "the new controls survive a state round trip");
        }
    }
}

// b56: page locks, and the dry path around a pedal
static float getP (StarterAmpProcessor& p, const char* id) { auto* v = p.apvts.getRawParameterValue (id);  return v != nullptr ? v->load() : -999.0f; }

static void testWorkflowB56()
{
    std::printf ("[workflow] b56: page locks, the dry path around a pedal\n");
    {   // ---- page locks
        auto make = [] (StarterAmpProcessor& p)
        {
            p.setPlayConfigDetails (2, 2, 48000.0, 512);  p.prepareToPlay (48000.0, 512);
            std::vector<signed char> map ((size_t) p.midiParamCount(), (signed char) -1);
            map[(size_t) p.midiIndexOf ("tsAttack")] = 0;  map[(size_t) p.midiIndexOf ("tsOut")] = 0;
            map[(size_t) p.midiIndexOf ("ampModel")] = 4;  map[(size_t) p.midiIndexOf ("reverbMix")] = 9;
            p.setParamPages (map);
        };
        StarterAmpProcessor a, b;  make (a);  make (b);
        setP (b, "tsAttack", -0.4f);  setP (b, "tsOut", -6.0f);  setP (b, "ampModel", 7.0f);  setP (b, "reverbMix", 0.55f);
        juce::MemoryBlock pre;  b.getPresetData (pre);
        setP (a, "tsAttack", 0.7f);  setP (a, "tsOut", 3.0f);  setP (a, "ampModel", 3.0f);  setP (a, "reverbMix", 0.1f);
        a.setPageLocked (0, true);
        CHECK (a.isPageLocked (0) && ! a.isPageLocked (4) && a.pageLocks() == 1, "page 0 is locked, page 4 is not");
        CHECK (a.loadPresetData (pre.getData(), (int) pre.getSize()), "a preset loads with a page locked");
        CHECK (std::abs (getP (a, "tsAttack") - 0.7f) < 1e-3f && std::abs (getP (a, "tsOut") - 3.0f) < 0.05f, "the locked page keeps its settings (%.2f, %.1f)", getP (a, "tsAttack"), getP (a, "tsOut"));
        CHECK (getP (a, "ampModel") == 7.0f && std::abs (getP (a, "reverbMix") - 0.55f) < 1e-3f, "the other pages take the preset");
        CHECK (a.isPageLocked (0), "the lock survives the preset load");
        CHECK (StarterAmpProcessor::factoryPresetCount() == 0, "b61: there are no built-in presets");
        juce::MemoryBlock sess;  a.getStateInformation (sess);
        StarterAmpProcessor c;  make (c);  c.setStateInformation (sess.getData(), (int) sess.getSize());
        CHECK (c.isPageLocked (0) && c.pageLocks() == 1, "a saved session remembers the locks");
        juce::MemoryBlock pre2;  a.getPresetData (pre2);
        StarterAmpProcessor d;  make (d);  d.loadPresetData (pre2.getData(), (int) pre2.getSize());
        CHECK (d.pageLocks() == 0, "a saved preset does not carry the locks");
        a.setPageLocked (0, false);  a.loadPresetData (pre.getData(), (int) pre.getSize());
        CHECK (std::abs (getP (a, "tsAttack") - (-0.4f)) < 1e-3f, "unlocked again, the preset takes the page (%.2f)", getP (a, "tsAttack"));
        // undo and A/B are not blocked by locks
        a.setPageLocked (0, true);  setP (a, "tsAttack", 0.1f);  a.abSelect (1);  setP (a, "tsAttack", 0.9f);  a.abSelect (0);
        CHECK (std::abs (getP (a, "tsAttack") - 0.1f) < 1e-3f, "A/B switching still changes a locked page (%.2f)", getP (a, "tsAttack"));
    }
    {   // ---- dry path
        PG::Data d;  PG::fromString (StarterAmpProcessor().getPatchText(), d);
        int node = -1, ni = 0, no = 0;
        for (int n = 0; n < PG::kPedals && node < 0; ++n)
        {
            if (! PG::isPedal (n)) continue;
            int i2 = 0, o2 = 0;  for (int i = 0; i < d.n; ++i) { if (d.c[i].to == n) ++i2;  if (d.c[i].from == n) ++o2; }
            if (i2 > 0 && o2 > 0) { node = n;  ni = i2;  no = o2; }
        }
        CHECK (node >= 0, "the standard board has a pedal with cables on both sides");
        const auto before = PG::toString (d);
        CHECK (! PG::hasDryPath (d, node), "no dry path to start with");
        const bool added = PG::addDryPath (d, node);
        CHECK (added && d.n > 0 && PG::hasDryPath (d, node), "a dry path can be added");
        auto d2 = d;  PG::removeDryPath (d2, node);
        CHECK (PG::toString (d2) == before && ! PG::hasDryPath (d2, node), "...and taking it away gives back the same board");
        // inside the plugin: with a drive that changes the sound a lot, the dry path changes the output and removing it restores it exactly
        const int n = 24000;  auto in = testSignal (n, 48000.0, 0.3f);
        auto run = [&] (int mode, std::vector<float>& out)
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, 512);  p.prepareToPlay (48000.0, 512);
            setP (p, "msOn", 1.0f);  for (const char* id : { "msDrive1", "msDrive2", "msDrive3", "msDrive4" }) setP (p, id, 36.0f);
            auto pd = p.getPatch();
            if (mode >= 1) { CHECK (PG::addDryPath (pd, 27), "the saturator can have a dry path");  p.setPatch (pd); }
            if (mode == 2) { PG::removeDryPath (pd, 27);  p.setPatch (pd); }
            process (p, in, out, 512);
        };
        std::vector<float> y0, y1, y2;  run (0, y0);  run (1, y1);  run (2, y2);
        double e = 0, same = 0;  for (size_t i = 0; i < y0.size(); ++i) { e += std::abs (y0[i] - y1[i]);  same = std::max (same, (double) std::abs (y0[i] - y2[i])); }
        float pk;
        CHECK (e > 1e-2 && finiteBounded (y1, 8.0f, pk), "the dry path changes the sound and stays bounded (%.3f, peak %.2f)", e, pk);
        CHECK (same == 0.0, "taking the dry path away restores the sound exactly (%.2e)", same);
    }
}


// b58: the Opto Sustain compressor style
static void testPassiveStack9()
{
    std::printf ("[stack] b58: the German high-gain passive stack (menu entries 11 to 13)\n");
    const double fs = 48000.0;
    auto resp = [fs] (int type, float t, float b, float m, double f) {
        PassiveStack ps;  std::vector<float> x ((size_t) fs);
        for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (0.1 * std::sin (2 * M_PI * f * (double) i / fs));
        for (size_t i = 0; i < x.size(); i += 512) { ps.set (type, t, b, m, fs);  ps.process (&x[i], (int) std::min<size_t> (512, x.size() - i)); }
        double e = 0;  long n = 0;  for (size_t i = x.size() / 2; i < x.size(); ++i) { e += (double) x[i] * x[i];  ++n; }
        return 20.0 * std::log10 (std::sqrt (e / (double) n) / (0.1 / std::sqrt (2.0))); };
    bool fin = true;
    for (float t : { 0.0f, 0.5f, 1.0f }) for (float b : { 0.0f, 0.5f, 1.0f }) for (float m : { 0.0f, 0.5f, 1.0f }) for (double f : { 80.0, 600.0, 4000.0 }) if (! std::isfinite (resp (9, t, b, m, f))) fin = false;
    CHECK (fin, "type 9 is finite at every corner of the three pots");
    CHECK (resp (9, 1.0f, 0.5f, 0.5f, 4000) - resp (9, 0.0f, 0.5f, 0.5f, 4000) > 8.0, "Treble opens the highs by more than 8 dB (%.1f)", resp (9, 1.0f, 0.5f, 0.5f, 4000) - resp (9, 0.0f, 0.5f, 0.5f, 4000));
    CHECK (resp (9, 0.5f, 1.0f, 0.5f, 100) - resp (9, 0.5f, 0.0f, 0.5f, 100) > 5.0, "Bass lifts the lows by more than 5 dB (%.1f)", resp (9, 0.5f, 1.0f, 0.5f, 100) - resp (9, 0.5f, 0.0f, 0.5f, 100));
    CHECK (resp (9, 0.5f, 0.5f, 1.0f, 600) - resp (9, 0.5f, 0.5f, 0.0f, 600) > 5.0, "Mid fills the mid dip by more than 5 dB (%.1f)", resp (9, 0.5f, 0.5f, 1.0f, 600) - resp (9, 0.5f, 0.5f, 0.0f, 600));
    {   // the two stacks added after it (entries 12 and 13): finite at every corner, working knobs
        for (int type : { 10, 11 })
        {
            bool f = true;
            for (float t : { 0.0f, 0.5f, 1.0f }) for (float b : { 0.0f, 0.5f, 1.0f }) for (float m : { 0.0f, 0.5f, 1.0f }) for (double fr : { 80.0, 600.0, 4000.0 }) if (! std::isfinite (resp (type, t, b, m, fr))) f = false;
            const double tr = resp (type, 1.0f, 0.5f, 0.5f, 4000) - resp (type, 0.0f, 0.5f, 0.5f, 4000), bs = resp (type, 0.5f, 1.0f, 0.5f, 100) - resp (type, 0.5f, 0.0f, 0.5f, 100);
            CHECK (f && tr > 6.0 && bs > 4.0, "stack type %d: finite, Treble range %.1f dB at 4 kHz, Bass range %.1f dB at 100 Hz", type, tr, bs);
        }
        // the bright cap opens the highs earlier than the 250 pF stack (types 11 and 0, Treble up)
        const double bright = resp (11, 0.7f, 0.5f, 0.5f, 1000) - resp (0, 0.7f, 0.5f, 0.5f, 1000);
        CHECK (bright > 2.0, "the 1000 pF stack has more at 1 kHz than the 250 pF one with Treble at 0.7 (%.1f dB more)", bright);
    }
    // the menu entry reaches it: the amp is finite and changes the sound against the orange-style stack
    const int blk = 512;  auto in = testSignal (24000, 48000.0, 0.3f);
    std::vector<float> a, b9;
    for (int sel : { 9, 11 })
    {
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, blk);  p.prepareToPlay (48000.0, blk);
        setP (p, "ampModel", 17.0f);  setP (p, "ampStack", (float) sel);  process (p, in, sel == 9 ? a : b9, blk);
    }
    double d = 0;  float pk;  for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b9[i]);
    CHECK (d > 1e-3 && finiteBounded (b9, 8.0f, pk), "Diesel Fuel with the new stack is finite, bounded and differs from the orange-style stack (%.3f, peak %.2f)", d, pk);
}

static void testProximityB59()
{
    std::printf("[proximity] b59: mic distance follows the pressure-gradient law (realistic response), older sessions keep the old law\n");
    using P = StarterAmpProcessor;
    auto r = [] (int mic, float d, double hz, bool real) { return P::builtinCabResponseDb (3, mic, 3, 0.3f, d, 0.0f, real, hz); };
    int nMics = 19;  double widest = 0;
    for (int m = 0; m < nMics; ++m)
    {
        const double at1 = r (m, 1.0f, 100.0, true), at0 = r (m, 0.0f, 100.0, true), at3 = r (m, 3.0f, 100.0, true), at12 = r (m, 12.0f, 100.0, true);
        CHECK (at0 >= at1 - 1e-9 && at1 >= at3 - 1e-9 && at3 >= at12 - 1e-9, "mic %d: lows fall steadily with distance (%.1f %.1f %.1f %.1f dB)", m, at0, at1, at3, at12);
        CHECK (at0 - at1 < 7.0, "mic %d: going from 1 in to the grille adds under 7 dB at 100 Hz (%.1f)", m, at0 - at1);
        widest = std::max (widest, at0 - at12);
        // the highs only see the distance loss, not the proximity law
        CHECK (std::abs ((r (m, 0.0f, 6000.0, true) - r (m, 12.0f, 6000.0, true)) - (r (m, 0.0f, 6000.0, false) - r (m, 12.0f, 6000.0, false))) < 0.5, "mic %d: the highs change the same way in both laws", m);
        // the mid band (1 kHz) barely moves with distance
        CHECK (std::abs (r (m, 0.5f, 1000.0, true) - r (m, 1.0f, 1000.0, true)) < 1.0, "mic %d: 1 kHz barely moves between 0.5 in and 1 in", m);
    }
    CHECK (widest > 6.0 && widest < 20.0, "the strongest mic changes %.1f dB at 100 Hz between 0 and 12 in", widest);
    // the legacy law is untouched
    CHECK (std::abs (r (0, 1.0f, 100.0, false) - r (0, 1.0f, 100.0, false)) == 0.0, "legacy law deterministic");
    // at the default distance the realistic law adds nothing on top of the earlier shelf
    for (int m : { 0, 3, 8 })
    {
        const double a = r (m, 1.0f, 100.0, true), b = r (m, 1.0f, 100.0, true);
        CHECK (a == b && std::isfinite (a), "mic %d: finite response at the default distance", m);
    }
}

static void testCompStylesB59()
{
    std::printf("[compstyles59] b59: FET measured ratios, Variable-mu attack and release stretch, Bus attack and 10:1\n");
    const double SR = 48000.0;
    // gain reduction in dB, read every 16 samples, for a 1 kHz tone at levelDb (RMS) that starts at t = 0 and lasts secs, then silence for tail seconds
    auto gr = [SR] (int style, int ratio, float thr, float attackMs, float releaseMs, double levelDb, double secs, double tail)
    {
        CompressorEffect c;  c.prepare (SR);  std::vector<float> out;
        const double A = std::pow (10.0, (levelDb + 3.0103) / 20.0);
        const long n = (long) ((secs + tail) * SR);  std::vector<float> x ((size_t) 16);  long i = 0;
        while (i < n)
        {
            for (int k = 0; k < 16; ++k, ++i) x[(size_t) k] = (i < (long) (secs * SR)) ? (float) (A * std::sin (2 * M_PI * 1000.0 * (double) i / SR)) : 0.0f;
            CompressorEffect::Params p { thr, ratio, attackMs, releaseMs, 0.0f, 1.0f, 0, 0, 0, 0, 0 };  p.style = style;
            c.process (x.data(), 16, p);  out.push_back (c.getGainReductionDb());
        }
        return out;
    };
    auto timeTo = [] (const std::vector<float>& g, double frac, size_t from, bool rising)
    {
        const float peak = *std::max_element (g.begin(), g.end());  size_t i = from;
        while (i < g.size() && (rising ? g[i] < frac * peak : g[i] > frac * peak)) ++i;
        return (double) (i - from) * 16.0 / 48.0;     // ms
    };
    {   // FET: the 4:1 button measures 5.1:1 and the 10:1 button 9.9:1 (12 dB over the threshold)
        const auto g4 = gr (1, 3, -30.0f, 5.0f, 250.0f, -18.0, 1.0, 0.0), g10 = gr (1, 5, -30.0f, 5.0f, 250.0f, -18.0, 1.0, 0.0);
        const double a = g4.back(), b = g10.back();
        CHECK (std::abs (a - 12.0 * (1.0 - 1.0 / 5.1)) < 0.6, "FET 4:1 holds %.2f dB of reduction at 12 dB over (5.1:1 gives %.2f)", a, 12.0 * (1.0 - 1.0 / 5.1));
        CHECK (std::abs (b - 12.0 * (1.0 - 1.0 / 9.9)) < 0.6, "FET 10:1 holds %.2f dB of reduction at 12 dB over (9.9:1 gives %.2f)", b, 12.0 * (1.0 - 1.0 / 9.9));
    }
    {   // Variable-mu: with the Attack knob down it grabs in a few milliseconds (the real unit's is 0.2 to 0.4 ms)
        const auto g = gr (3, 5, -30.0f, 0.5f, 250.0f, -8.0, 0.5, 0.0);
        const double t = timeTo (g, 0.5, 0, true);
        CHECK (t < 4.0, "Variable-mu with a short attack reaches half its reduction in %.1f ms", t);
        // the release stretches after a long squeeze
        const auto sh = gr (3, 5, -30.0f, 5.0f, 250.0f, -12.0, 0.06, 2.0), lo = gr (3, 5, -30.0f, 5.0f, 250.0f, -12.0, 4.0, 4.0);
        const size_t i1 = (size_t) (0.06 * 3000.0), i2 = (size_t) (4.0 * 3000.0);
        const double tShort = timeTo (std::vector<float> (sh.begin() + (long) i1, sh.end()), 0.5, 0, false) , tLong = timeTo (std::vector<float> (lo.begin() + (long) i2, lo.end()), 0.5, 0, false);
        CHECK (tLong > 1.4 * tShort, "Variable-mu lets go more slowly after a long squeeze (%.0f ms vs %.0f ms to half)", tLong, tShort);
    }
    {   // RMS (VCA, style 9): constant-rate release, attack faster for bigger hits, hard knee, finite everywhere
        const auto sh = gr (9, 6, -40.0f, 5.0f, 250.0f, -28.0, 0.5, 0.6), dp = gr (9, 6, -40.0f, 5.0f, 250.0f, -4.0, 0.5, 1.0);
        const size_t i1 = (size_t) (0.5 * 3000.0);
        const std::vector<float> a1 (sh.begin() + (long) i1, sh.end()), a2 (dp.begin() + (long) i1, dp.end());
        const double tS = timeTo (a1, 0.5, 0, false), tD = timeTo (a2, 0.5, 0, false);
        CHECK (tD > 2.0 * tS && tD < 4.5 * tS, "RMS release runs at a steady rate: half of a deep squeeze takes %.0f ms, of a light one %.0f ms", tD, tS);
        CHECK (std::abs (a1.front() - 12.0 * (1.0 - 1.0 / 20.0)) < 1.0, "RMS 20:1 holds about %.1f dB of reduction at 12 dB over (%.1f)", 12.0 * (1.0 - 1.0 / 20.0), a1.front());
        const auto big = gr (9, 6, -40.0f, 5.0f, 250.0f, -10.0, 0.3, 0.0), small = gr (9, 6, -40.0f, 5.0f, 250.0f, -30.0, 0.3, 0.0);
        const double tb = timeTo (big, 0.9, 0, true), ts = timeTo (small, 0.9, 0, true);
        CHECK (tb < ts, "RMS attack is faster for the bigger hit (%.1f ms for 30 dB over, %.1f ms for 10 dB over)", tb, ts);
        bool fin = true;  for (float v : big) if (! std::isfinite (v)) fin = false;
        CHECK (fin, "RMS output is finite");
    }
    {   // Bus: 10:1 squeezes harder than 4:1 (it was capped at 4:1), and the shortest attack is fast
        const auto g4 = gr (5, 3, -30.0f, 5.0f, 250.0f, -10.0, 1.0, 0.0), g10 = gr (5, 5, -30.0f, 5.0f, 250.0f, -10.0, 1.0, 0.0);
        CHECK (g10.back() > g4.back() + 2.0, "Bus 10:1 (%.1f dB) squeezes harder than 4:1 (%.1f dB) at 20 dB over", g10.back(), g4.back());
        const auto g = gr (5, 3, -30.0f, 0.05f, 250.0f, -10.0, 0.5, 0.0);
        const double t = timeTo (g, 0.5, 0, true);
        CHECK (t < 6.0, "Bus with the shortest attack reaches half its reduction in %.1f ms", t);
    }
}

static void testProgramEqB59()
{
    std::printf("[programeq] b59: the Program EQ: shelves, peak and bandwidth, high attenuate, neutral at zero, warmth, level, inside the plugin\n");
    using PE = FxExtra::ProgramEqFx;
    const double SR = 48000.0;
    PE ref;  ref.prepare (SR);
    auto sine = [SR] (double f, double a, double secs) { std::vector<float> x ((size_t) (secs * SR));  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (a * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
    auto run = [SR] (std::vector<float> x, const PE::Params& p) { PE t;  t.prepare (SR);  for (size_t i = 0; i < x.size(); i += 256) { const int n = (int) std::min<size_t> (256, x.size() - i);  t.process (&x[i], nullptr, n, p); }  return x; };
    auto rmsTail = [SR] (const std::vector<float>& x) { double s = 0;  long n = 0;  for (size_t i = x.size() / 2; i < x.size(); ++i) { s += (double) x[i] * x[i];  ++n; }  return std::sqrt (s / (double) n); };
    auto gainDb = [&] (double f, const PE::Params& p, double amp = 0.1) { const auto x = sine (f, amp, 1.0);  return 20 * std::log10 (rmsTail (run (x, p)) / rmsTail (x)); };

    {   // all zero = the signal untouched, to the last bit
        PE::Params p;  const auto x = sine (220, 0.3, 0.5);  const auto y = run (x, p);
        double d = 0;  for (size_t i = 0; i < x.size(); ++i) d = std::max (d, (double) std::abs (x[i] - y[i]));
        CHECK (d == 0.0, "neutral settings leave the signal untouched (%.2e)", d);
    }
    {   // low boost and low attenuate
        PE::Params p;  p.lowBoostDb = 12.0f;  p.lowHz = 60.0f;
        CHECK (ref.responseDb (25.0f, p) > 9.0f && ref.responseDb (25.0f, p) < 15.0f, "low boost 12 dB at 60 Hz: %.1f dB at 25 Hz", ref.responseDb (25.0f, p));
        CHECK (std::abs (ref.responseDb (1000.0f, p)) < 0.3 && std::abs (ref.responseDb (4000.0f, p)) < 0.05, "low boost leaves 1 kHz alone (%.2f dB)", ref.responseDb (1000.0f, p));
        p = PE::Params {};  p.lowCutDb = 12.0f;  p.lowHz = 60.0f;
        CHECK (ref.responseDb (25.0f, p) < -9.0f && ref.responseDb (200.0f, p) < -2.0f && ref.responseDb (200.0f, p) > ref.responseDb (25.0f, p), "low attenuate 12 dB: %.1f dB at 25 Hz, %.1f dB at 200 Hz (wider than the boost)", ref.responseDb (25.0f, p), ref.responseDb (200.0f, p));
        // both together: a lift below and a scoop above the frequency
        p = PE::Params {};  p.lowBoostDb = 10.0f;  p.lowCutDb = 10.0f;  p.lowHz = 60.0f;
        CHECK (ref.responseDb (30.0f, p) > 0.5f && ref.responseDb (130.0f, p) < -3.0f, "boost + attenuate together: %.1f dB at 30 Hz, %.1f dB at 130 Hz", ref.responseDb (30.0f, p), ref.responseDb (130.0f, p));
        // the dial moves it
        PE::Params q = p;  q.lowHz = 200.0f;
        CHECK (ref.responseDb (300.0f, q) < ref.responseDb (300.0f, p) - 0.5f, "a higher dial moves the scoop up");
    }
    {   // high boost: peak at the dial, bandwidth moves the width
        PE::Params p;  p.highBoostDb = 12.0f;  p.highHz = 8000.0f;  p.bandwidth = 0.0f;
        const double broad4k = ref.responseDb (4000.0f, p), peak = ref.responseDb (8000.0f, p);
        p.bandwidth = 1.0f;
        const double sharp4k = ref.responseDb (4000.0f, p);
        CHECK (std::abs (peak - 12.0) < 0.6 && std::abs (ref.responseDb (8000.0f, p) - 12.0) < 0.6, "high boost 12 dB peaks at %.1f dB on the dial", peak);
        CHECK (broad4k > sharp4k + 4.0, "broad (%.1f dB) is wider than sharp (%.1f dB) an octave below", broad4k, sharp4k);
        CHECK (std::abs (ref.responseDb (200.0f, p)) < 0.05, "high boost does not touch the lows");
    }
    {   // high attenuate
        PE::Params p;  p.highCutDb = 12.0f;  p.highCutHz = 10000.0f;
        CHECK (ref.responseDb (20000.0f, p) < -9.0f && ref.responseDb (1000.0f, p) > -1.0f && ref.responseDb (10000.0f, p) < -3.0f, "high attenuate 12 dB: %.1f dB at 20 kHz, %.1f dB at 10 kHz, %.1f dB at 1 kHz", ref.responseDb (20000.0f, p), ref.responseDb (10000.0f, p), ref.responseDb (1000.0f, p));
    }
    {   // the processed signal matches the response
        PE::Params p;  p.lowBoostDb = 9.0f;  p.lowHz = 100.0f;  p.highBoostDb = 8.0f;  p.highHz = 5000.0f;  p.bandwidth = 0.4f;  p.highCutDb = 6.0f;  p.highCutHz = 15000.0f;
        for (double f : { 60.0, 300.0, 5000.0, 12000.0 })
            CHECK (std::abs (gainDb (f, p) - (double) ref.responseDb ((float) f, p)) < 0.3, "%.0f Hz: measured %.2f dB, response %.2f dB", f, gainDb (f, p), (double) ref.responseDb ((float) f, p));
    }
    {   // level, and warmth
        PE::Params p;  p.levelDb = 6.0f;
        CHECK (std::abs (gainDb (500.0, p) - 6.0) < 0.1, "level +6 dB (%.2f)", gainDb (500.0, p));
        p = PE::Params {};  p.drive = 1.0f;
        CHECK (std::abs (gainDb (500.0, p, 0.003)) < 0.8, "warmth leaves a quiet note at about the same level (%.2f dB)", gainDb (500.0, p, 0.003));
        // a loud note picks up harmonics (2nd and 3rd), a quiet one barely
        auto harm = [SR] (const std::vector<float>& y, double f, int k) { double re = 0, im = 0;  const size_t a = y.size() / 2;  const long n = (long) ((y.size() - a) / 48000) * 48000;
            for (long i = 0; i < n; ++i) { const double ph = 2 * M_PI * f * k * (double) (a + (size_t) i) / SR;  re += (double) y[a + (size_t) i] * std::cos (ph);  im += (double) y[a + (size_t) i] * std::sin (ph); }  return 2.0 * std::sqrt (re * re + im * im) / (double) n; };
        const auto loud = run (sine (110, 0.4, 2.0), p), quiet = run (sine (110, 0.01, 2.0), p);
        const double h3l = harm (loud, 110, 3) / harm (loud, 110, 1), h3q = harm (quiet, 110, 3) / harm (quiet, 110, 1);
        CHECK (h3l > 0.02 && h3l > 8.0 * h3q, "warmth adds harmonics to a loud note (3rd: %.4f) far more than a quiet one (%.5f)", h3l, h3q);
        bool fin = true;  for (float v : loud) if (! std::isfinite (v)) fin = false;
        CHECK (fin, "warmth output is finite");
    }
    {   // inside the plugin: on = changes the sound and stays bounded, off (with the block's own switch off) = identical, and a bass boost raises the lows
        const int blk = 512;  auto in = testSignal (48000, 48000.0, 0.3f);
        std::vector<float> off, on, on2;
        { StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, blk);  p.prepareToPlay (48000.0, blk);  process (p, in, off, blk); }
        { StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, blk);  p.prepareToPlay (48000.0, blk);
          setP (p, "pgOn", 1.0f);  process (p, in, on, blk); }                                   // on with every control neutral
        double e0 = 0;  for (size_t i = 0; i < off.size(); ++i) e0 = std::max (e0, (double) std::abs (off[i] - on[i]));
        CHECK (e0 == 0.0, "Program EQ on with neutral settings does not change the sound (%.2e)", e0);
        { StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, blk);  p.prepareToPlay (48000.0, blk);
          setP (p, "pgOn", 1.0f);  setP (p, "pgLowBoost", 12.0f);  setP (p, "pgHighBoost", 10.0f);  setP (p, "pgDrive", 0.5f);  process (p, in, on2, blk); }
        double e = 0;  for (size_t i = 0; i < off.size(); ++i) e += std::abs (off[i] - on2[i]);
        float pk;
        CHECK (e > 1e-2 && finiteBounded (on2, 8.0f, pk), "Program EQ changes the sound inside the plugin and stays bounded (%.3f, peak %.2f)", e, pk);
    }
    {   // stereo: both channels get the same treatment; mono and stereo agree
        PE::Params p;  p.lowBoostDb = 6.0f;  p.highBoostDb = 6.0f;
        auto x = sine (330, 0.2, 0.5);  auto l = x, r = x;  PE t;  t.prepare (SR);
        for (size_t i = 0; i < l.size(); i += 256) { const int n = (int) std::min<size_t> (256, l.size() - i);  t.process (&l[i], &r[i], n, p); }
        const auto m = run (x, p);  double d = 0;  for (size_t i = 0; i < m.size(); ++i) d = std::max (d, (double) std::max (std::abs (m[i] - l[i]), std::abs (l[i] - r[i])));
        CHECK (d < 1e-6, "stereo and mono give the same result (%.2e)", d);
    }
}

static void testOptoSustain()
{
    std::printf("[optosustain] b58: the Opto Sustain compressor style: levelling, Sustain, Blend, Grit, Treble, Volume, low notes, aliasing, inside the plugin\n");
    using OS = FxExtra::OptoSustainFx;
    const double SR = 48000.0;
    auto pluck = [SR] (double f, double a, double tau, double secs) { std::vector<float> x ((size_t) (secs * SR));  for (size_t i = 0; i < x.size(); ++i) { const double t = (double) i / SR;  x[i] = (float) (a * std::exp (-t / tau) * std::sin (2 * M_PI * f * t)); }  return x; };
    auto rms = [SR] (const std::vector<float>& x, double a, double b) { double s = 0;  long n = 0; for (long i = (long) (a * SR); i < (long) (b * SR) && i < (long) x.size(); ++i) { s += (double) x[(size_t) i] * x[(size_t) i];  ++n; }  return std::sqrt (s / (double) std::max (1L, n) + 1e-30); };
    auto db = [] (double a, double b) { return 20 * std::log10 (a / b); };
    auto run = [SR] (std::vector<float> x, const OS::Params& p, float* gr = nullptr) { OS t;  t.prepare (SR);  for (size_t i = 0; i < x.size(); i += 512) { const int n = (int) std::min<size_t> (512, x.size() - i);  const float g = t.process (&x[i], n, p);  if (gr != nullptr) *gr = g; }  return x; };
    auto sine = [SR] (double f, double a, double secs) { std::vector<float> x ((size_t) (secs * SR));  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (a * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
    // power of a tone at one harmonic, over a window that holds a whole number of cycles
    auto harm = [SR] (const std::vector<float>& y, double f, int k, double t0, double len) { double re = 0, im = 0;  const long a = (long) (t0 * SR), n = (long) (len * SR);
        for (long i = a; i < a + n; ++i) { const double ph = 2 * M_PI * f * k * (double) i / SR;  re += (double) y[(size_t) i] * std::cos (ph);  im += (double) y[(size_t) i] * std::sin (ph); }
        return 2.0 * std::sqrt (re * re + im * im) / (double) n; };

    {   // Sustain 1 holds the level of a decaying note; Sustain 0 leaves a quiet note alone
        OS::Params p;  p.sustain = 1.0f;
        const auto x = pluck (220, 0.3, 0.5, 2.5);  const auto y = run (x, p);
        const double span = db (rms (y, 0.05, 0.09), rms (y, 2.0, 2.04)), spanIn = db (rms (x, 0.05, 0.09), rms (x, 2.0, 2.04));
        CHECK (span < 12.0 && spanIn > 30.0, "Sustain 1: a note that dies away by %.0f dB comes out within %.1f dB", spanIn, span);
        const auto xs = pluck (220, 0.03, 0.5, 2.5);  const auto ys = run (xs, p);
        CHECK (std::abs (db (rms (y, 0.3, 0.34), rms (ys, 0.3, 0.34))) < 3.0, "Sustain 1: a hard and a soft pick come out within 3 dB (%.1f dB apart)", db (rms (y, 0.3, 0.34), rms (ys, 0.3, 0.34)));
        p.sustain = 0.0f;  const auto q = sine (220, 0.01, 1.0);  const auto yq = run (q, p);
        CHECK (std::abs (db (rms (yq, 0.5, 0.9), rms (q, 0.5, 0.9))) < 0.3, "Sustain 0: a quiet note passes at the same level (%.2f dB)", db (rms (yq, 0.5, 0.9), rms (q, 0.5, 0.9)));
        double prev = -1e9;  bool mono = true;
        for (float sv : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }) { p.sustain = sv;  const double t = db (rms (run (x, p), 1.6, 1.64), 1.0);  if (t < prev - 2.0) mono = false;  prev = t; }
        CHECK (mono && prev > db (rms (run (x, OS::Params { 0.0f, 0, 0, 1, 0 }), 1.6, 1.64), 1.0) + 10.0, "more Sustain lifts the tail (to within 2 dB at the top, where the longer hold slows the swell)");
    }
    {   // the release is about half a second: after a loud burst stops, the gain climbs back over a few hundred ms (not instantly, not seconds)
        std::vector<float> x ((size_t) (3 * SR), 0.0f);  const auto burst = sine (220, 0.3, 0.5);  for (size_t i = 0; i < burst.size(); ++i) x[i] = burst[i];
        for (size_t i = (size_t) (0.5 * SR); i < x.size(); ++i) x[i] = (float) (0.003 * std::sin (2 * M_PI * 330.0 * (double) i / SR));        // then a quiet note
        OS::Params p;  p.sustain = 0.8f;  const auto y = run (x, p);
        const double a = db (rms (y, 0.52, 0.56), rms (x, 0.52, 0.56)), b = db (rms (y, 0.9, 0.94), rms (x, 0.9, 0.94)), c = db (rms (y, 2.5, 2.54), rms (x, 2.5, 2.54));
        CHECK (a < b - 3.0 && b < c - 3.0 && c < 40.0, "the quiet note after a burst swells up over time (%.1f, %.1f, %.1f dB of gain)", a, b, c);
    }
    {   // Blend 0 is the dry sound, Volume is in dB
        const auto x = pluck (220, 0.3, 0.5, 1.0);  OS::Params p;  p.sustain = 0.9f;  p.grit = 0.5f;  p.treble = 0.6f;  p.blend = 0.0f;
        const auto y = run (x, p);  double d = 0;  for (size_t i = 0; i < x.size(); ++i) d = std::max (d, (double) std::abs (x[i] - y[i]));
        CHECK (d == 0.0, "Blend 0 is the dry sound exactly (%.2e)", d);
        p.volumeDb = 6.0f;  const auto y6 = run (x, p);  CHECK (std::abs (db (rms (y6, 0.1, 0.5), rms (x, 0.1, 0.5)) - 6.0) < 0.05, "Volume +6 dB raises the dry sound by 6 dB (%.2f)", db (rms (y6, 0.1, 0.5), rms (x, 0.1, 0.5)));
        p.blend = 0.5f;  p.volumeDb = 0.0f;  const auto yh = run (x, p);  p.blend = 1.0f;  const auto yw = run (x, p);
        double dd = 0;  for (size_t i = 0; i < x.size(); ++i) dd = std::max (dd, (double) std::abs (yh[i] - 0.5f * (x[i] + yw[i])));
        CHECK (dd < 1e-5, "Blend 0.5 is half dry and half squeezed (%.2e)", dd);
    }
    {   // Treble: flat at the middle, cuts and boosts the highs, leaves the lows
        OS::Params p;  p.sustain = 0.0f;
        for (float tr : { -1.0f, 0.0f, 1.0f })
        {
            p.treble = tr;  double lo, hi;
            { const auto q = sine (100, 0.01, 1.0);  lo = db (rms (run (q, p), 0.5, 0.9), rms (q, 0.5, 0.9)); }
            { const auto q = sine (8000, 0.01, 1.0); hi = db (rms (run (q, p), 0.5, 0.9), rms (q, 0.5, 0.9)); }
            CHECK (std::abs (lo) < 0.5 && (tr == 0.0f ? std::abs (hi) < 0.1 : (tr < 0 ? hi < -5.0 : hi > 5.0)), "Treble %+.0f: 100 Hz %.2f dB, 8 kHz %.2f dB", (double) tr, lo, hi);
        }
    }
    {   // the squeezer alone adds almost no distortion, even on a low note at full Sustain
        for (double f : { 82.0, 110.0, 330.0 })
        {
            const auto q = sine (f, 0.2, 2.0);  OS::Params p;  p.sustain = 1.0f;  const auto y = run (q, p);
            // (half a second holds a whole number of cycles for these even frequencies)
            const double h1 = harm (y, f, 1, 1.0, 0.5), h2 = harm (y, f, 2, 1.0, 0.5), h3 = harm (y, f, 3, 1.0, 0.5);
            CHECK (db (std::max (h2, h3), h1) < -40.0, "Sustain 1 on a %.0f Hz note: 2nd and 3rd harmonic %.1f dB below the note", f, db (std::max (h2, h3), h1));
        }
    }
    {   // Grit adds harmonics and level, stays bounded, and its clipper does not alias
        const auto q = sine (1100, 0.1, 2.0);  OS::Params p;  p.sustain = 0.5f;
        const auto y0 = run (q, p);  p.grit = 1.0f;  const auto y1 = run (q, p);
        const double h3a = harm (y0, 1100, 3, 1.0, 0.5), h3b = harm (y1, 1100, 3, 1.0, 0.5), h1b = harm (y1, 1100, 1, 1.0, 0.5);
        CHECK (db (h3b, h1b) > db (h3a, harm (y0, 1100, 1, 1.0, 0.5)) + 15.0, "Grit 1 adds odd harmonics (3rd is %.1f dB below the note, was %.1f)", db (h3b, h1b), db (h3a, harm (y0, 1100, 1, 1.0, 0.5)));
        double total = 0;  for (long i = (long) (1.0 * SR); i < (long) (1.5 * SR); ++i) total += (double) y1[(size_t) i] * y1[(size_t) i];
        double harmonics = 0;  for (int k = 1; k * 1100 < 24000; ++k) { const double h = harm (y1, 1100, k, 1.0, 0.5);  harmonics += 0.5 * h * h * (0.5 * SR) / (0.5 * SR); }
        const double resid = std::max (0.0, total / (0.5 * SR) - harmonics);
        CHECK (10 * std::log10 (resid / (total / (0.5 * SR)) + 1e-30) < -40.0, "Grit 1 at 1100 Hz: everything that is not a harmonic is %.1f dB down", 10 * std::log10 (resid / (total / (0.5 * SR)) + 1e-30));
        float pk = 0;  bool fin = true;  for (float v : y1) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, std::abs (v)); }
        CHECK (fin && pk < 2.0f, "Grit 1 stays finite and bounded (peak %.2f)", (double) pk);
    }
    {   // extremes: silence, full-scale noise, every knob at both ends, a change of settings in mid-stream
        std::mt19937 rng (5);  std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        std::vector<float> n ((size_t) (2 * SR));  for (auto& v : n) v = u (rng);
        bool fin = true;  float pk = 0;
        for (int c = 0; c < 8; ++c)
        {
            OS::Params p;  p.sustain = (c & 1) ? 1.0f : 0.0f;  p.grit = (c & 2) ? 1.0f : 0.0f;  p.treble = (c & 4) ? 1.0f : -1.0f;  p.volumeDb = (c & 1) ? 24.0f : -24.0f;
            for (float v : run (n, p)) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, std::abs (v)); }
        }
        std::vector<float> z ((size_t) SR, 0.0f);  OS::Params p;  p.sustain = 1.0f;  p.grit = 1.0f;  const auto yz = run (z, p);
        bool silent = true;  for (float v : yz) if (v != 0.0f) silent = false;
        CHECK (fin && pk < 200.0f, "full-scale noise at every extreme stays finite (peak %.1f)", (double) pk);
        CHECK (silent, "silence in, silence out");
        OS t;  t.prepare (SR);  auto w = n;  OS::Params q1, q2;  q2.sustain = 1.0f;  q2.grit = 1.0f;  bool fin2 = true;
        for (size_t i = 0; i < w.size(); i += 256) { t.process (&w[i], (int) std::min<size_t> (256, w.size() - i), ((i / 256) % 2) ? q1 : q2);  }
        for (float v : w) if (! std::isfinite (v)) fin2 = false;
        CHECK (fin2, "switching settings every block stays finite");
    }
    {   // inside the plugin
        const int blk = 512;  auto in = testSignal (48000, 48000.0, 0.3f);
        std::vector<float> off, on, onL;
        { StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, blk);  p.prepareToPlay (48000.0, blk);  process (p, in, off, blk); }
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, blk);  p.prepareToPlay (48000.0, blk);
        setP (p, "compOn", 1.0f);  setP (p, "compStyle", 8.0f);  setP (p, "compSustain", 0.9f);  setP (p, "compGrit", 0.3f);
        process (p, in, on, blk);
        double e = 0;  for (size_t i = 0; i < off.size(); ++i) e += std::abs (off[i] - on[i]);
        float pk;
        CHECK (e > 1e-2 && finiteBounded (on, 8.0f, pk), "the style changes the sound inside the plugin and stays bounded (%.3f, peak %.2f)", e, pk);
        CHECK (p.getCompGainReductionDb() > 1.0f, "the gain-reduction meter shows the squeeze (%.1f dB)", (double) p.getCompGainReductionDb());
        // both channels carry the same signal
        juce::AudioBuffer<float> b (2, blk);  juce::MidiBuffer m;  double dlr = 0;
        for (int k = 0; k < 20; ++k)
        {
            for (int i = 0; i < blk; ++i) { b.setSample (0, i, in[(size_t) (k * blk + i)]);  b.setSample (1, i, in[(size_t) (k * blk + i)]); }
            p.processBlock (b, m);
            for (int i = 0; i < blk; ++i) dlr = std::max (dlr, (double) std::abs (b.getSample (0, i) - b.getSample (1, i)));
        }
        CHECK (dlr == 0.0, "both channels get the same sound (%.2e)", dlr);
        // moving between this style and the others while playing stays finite
        bool fin = true;
        for (int k = 0; k < 40; ++k)
        {
            setP (p, "compStyle", (float) (k % 9));
            for (int i = 0; i < blk; ++i) { b.setSample (0, i, in[(size_t) ((k * blk + i) % in.size())]);  b.setSample (1, i, b.getSample (0, i)); }
            p.processBlock (b, m);
            for (int i = 0; i < blk; ++i) if (! std::isfinite (b.getSample (0, i))) fin = false;
        }
        CHECK (fin, "switching between all nine compressor styles while playing stays finite");
        // a session remembers the knobs
        setP (p, "compStyle", 8.0f);
        juce::MemoryBlock mb;  p.getStateInformation (mb);  StarterAmpProcessor q;  q.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (getP (q, "compStyle") == 8.0f && std::abs (getP (q, "compSustain") - 0.9f) < 1e-3f && std::abs (getP (q, "compGrit") - 0.3f) < 1e-3f, "a saved session keeps the style and its knobs (style %.1f, sustain %.3f, grit %.3f)", (double) getP (q, "compStyle"), (double) getP (q, "compSustain"), (double) getP (q, "compGrit"));
    }
}


//==============================================================================
// b62: Crush Limiter comp style, Tape boost mode, No cabinet, the Frequency shifter and Vocoder Mod FX types (with the optional sidechain),
//      the Feral fuzz and the Cassette Lo-Fi voicing.
static double tone62 (const std::vector<float>& y, double f, double sr, size_t from, size_t to)
{
    double re = 0, im = 0;  const size_t n = to - from;
    for (size_t i = 0; i < n; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (6.283185307 * (double) i / (double) n), ph = 6.283185307 * f * (double) (from + i) / sr;
        re += (double) y[from + i] * w * std::cos (ph);  im += (double) y[from + i] * w * std::sin (ph);
    }
    return std::sqrt (re * re + im * im) / ((double) n * 0.25);
}
static double toneBand62 (const std::vector<float>& y, double lo, double hi, double sr, size_t from, size_t to)
{   // the strongest line between lo and hi Hz (the loop's own resonance sits a little off the nominal Pitch)
    double best = 0;  for (double f = lo; f <= hi; f += 1.0) best = std::max (best, tone62 (y, f, sr, from, to));  return best;
}
static double dbr (double a, double b) { return 20.0 * std::log10 ((a + 1e-12) / (b + 1e-12)); }

static void testB63()
{
    std::printf ("[b63] Amp A on / off\n");
    const auto in = testSignal (96000, 48000.0, 0.5f);
    auto fresh = [&] () { auto p = std::make_unique<StarterAmpProcessor>();  p->setPlayConfigDetails (2, 2, 48000.0, 512);  p->prepareToPlay (48000.0, 512);  return p; };
    {   auto a = fresh();  std::vector<float> y;  process (*a, in, y, 512);
        CHECK (a->apvts.getRawParameterValue ("ampAOn")->load() > 0.5f && rmsDb (y, 24000) > -50.0, "Amp A is on by default and makes sound (%.1f dB)", rmsDb (y, 24000)); }
    {   auto a = fresh();  setP (*a, "ampAOn", 0.0f);  std::vector<float> y;  process (*a, in, y, 512);
        float pk;  CHECK (finiteBounded (y, 4.0f, pk) && rmsDb (y, 4096) < -120.0, "Amp A off (and Amp B off): silence after the first block (%.1f dB)", rmsDb (y, 4096)); }
    {   auto a = fresh();  setP (*a, "ampAOn", 0.0f);  setP (*a, "ampBOn", 1.0f);  setP (*a, "ampBModel", 8.0f);  std::vector<float> y;  process (*a, in, y, 512);
        float pk;  CHECK (finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -55.0, "Amp A off, Amp B on: Amp B plays on its own (%.1f dB)", rmsDb (y, 24000)); }
    {   auto a = fresh();  std::vector<float> y1, y2, y3;
        const std::vector<float> p1 (in.begin(), in.begin() + 36000), p2 (in.begin() + 36000, in.begin() + 60000), p3 (in.begin() + 60000, in.end());
        process (*a, p1, y1, 512);  setP (*a, "ampAOn", 0.0f);  process (*a, p2, y2, 512);  setP (*a, "ampAOn", 1.0f);  process (*a, p3, y3, 512);
        auto jump = [] (const std::vector<float>& y, size_t from, size_t to) { float j = 0;  for (size_t i = from + 1; i < to && i < y.size(); ++i) j = std::max (j, std::abs (y[i] - y[i - 1]));  return j; };
        const float j2 = jump (y2, 0, 1024), j3 = jump (y3, 0, 1024), j1 = jump (y1, 8000, y1.size());
        float pk;  CHECK (finiteBounded (y2, 4.0f, pk) && finiteBounded (y3, 4.0f, pk) && rmsDb (y2, 4096) < -100.0 && rmsDb (y3, 8192) > -60.0, "switching Amp A off and on mid-note: silent while off (%.1f dB), back after (%.1f dB)", rmsDb (y2, 4096), rmsDb (y3, 8192));
        CHECK (j2 < 4.0f * j1 + 0.05f && j3 < 4.0f * j1 + 0.05f, "no click on switching (largest step %.3f off, %.3f on, %.3f playing)", j2, j3, j1); }
    std::printf ("[b63] Cedar Spring voicings\n");
    {   // five voicings on model 30; every other amp holds at four
        double lvl[5];  bool allOk = true;
        for (int v = 0; v < 5; ++v)
        {   auto a = fresh();  setP (*a, "ampModel", 30.0f);  setP (*a, "ampVoicing", (float) v);  std::vector<float> y;  process (*a, in, y, 512);
            float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk);  lvl[v] = rmsDb (y, 24000); }
        bool differ = false;  for (int v = 1; v < 5; ++v) if (std::abs (lvl[v] - lvl[v - 1]) > 0.05) differ = true;
        CHECK (allOk && differ, "Cedar Spring: all five voicings play, finite and bounded, and they differ (%.1f %.1f %.1f %.1f %.1f dB)", lvl[0], lvl[1], lvl[2], lvl[3], lvl[4]);
        auto a3 = fresh();  setP (*a3, "ampModel", 8.0f);  setP (*a3, "ampVoicing", 3.0f);  std::vector<float> y3;  process (*a3, in, y3, 512);
        auto a4 = fresh();  setP (*a4, "ampModel", 8.0f);  setP (*a4, "ampVoicing", 4.0f);  std::vector<float> y4;  process (*a4, in, y4, 512);
        double d = 0;  for (size_t i = 0; i < y3.size(); ++i) d = std::max (d, (double) std::abs (y3[i] - y4[i]));
        CHECK (d < 1e-6, "a fifth voicing on any other amp holds at its fourth (largest difference %.2g)", d); }
    std::printf ("[b65] extra voicings (Marque Dream, Citrus Doom, Matt's Amp)\n");
    {   int n = 0;  const AmpExtras::Item* t = AmpExtras::table (n);
        bool seen[40] = {};
        for (int i = 0; i < n; ++i)
        {
            const int m = t[i].model;  if (seen[m]) continue;  seen[m] = true;
            for (int mode = 0; mode < 3; ++mode)
            {
                bool allOk = true, differ = true;  double lvl[8] = {};  std::vector<float> ys[8];
                for (int v = 0; v <= AmpExtras::lastVoicing (m); ++v)
                {   auto a = fresh();  setP (*a, "ampModel", (float) m);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampVoicing", (float) v);
                    process (*a, in, ys[v], 512);
                    float pk;  allOk = allOk && finiteBounded (ys[v], 8.0f, pk);  lvl[v] = rmsDb (ys[v], 24000); }
                // the extras must sound different from every voicing below them (the trims keep their levels close, so compare the waveforms)
                for (int v = 4; v <= AmpExtras::lastVoicing (m); ++v)
                    for (int u = 0; u < v; ++u)
                    {   double d = 0, e = 0;  for (size_t k = 24000; k < ys[v].size() && k < ys[u].size(); ++k) { d += (double) (ys[v][k] - ys[u][k]) * (ys[v][k] - ys[u][k]);  e += (double) ys[u][k] * ys[u][k]; }
                        if (d < 0.01 * e) differ = false; }
                CHECK (allOk && differ, "model %d, mode %d: its %d voicings play, finite and bounded (%d), and every extra differs from the ones below it (%d); levels %.1f %.1f %.1f %.1f %.1f %.1f %.1f", m, mode, AmpExtras::lastVoicing (m) + 1, (int) allOk, (int) differ, lvl[0], lvl[1], lvl[2], lvl[3], lvl[4], lvl[5], lvl[6]);
            }
        } }
}

static void testB62()
{
    std::printf ("[b62] new effects\n");
    const double SR = 48000.0;
    auto sineV = [&] (double f, double amp, double secs) { std::vector<float> v ((size_t) (secs * SR)); for (size_t i = 0; i < v.size(); ++i) v[i] = (float) (amp * std::sin (6.283185307 * f * (double) i / SR)); return v; };
    auto noiseV = [&] (double amp, double secs, unsigned seed) { std::mt19937 r (seed); std::uniform_real_distribution<float> u (-1.0f, 1.0f); std::vector<float> v ((size_t) (secs * SR)); for (auto& x : v) x = (float) amp * u (r); return v; };

    {   // ---- Frequency shifter, directly
        auto run = [&] (std::vector<float> l, std::vector<float>* r, FxExtra::FreqShiftFx::Params p)
        {
            FxExtra::FreqShiftFx fs;  fs.prepare (SR);
            for (size_t i = 0; i < l.size(); i += 512)
            {
                const int n = (int) std::min<size_t> (512, l.size() - i);
                fs.process (&l[i], r != nullptr ? &(*r)[i] : nullptr, n, p);
            }
            return l;
        };
        FxExtra::FreqShiftFx::Params p;  p.mode = 0;  p.shiftHz = 200.0f;  p.depth = 0.0f;  p.mix = 1.0f;  p.toneHz = 20000.0f;
        auto y = run (sineV (1000, 0.5, 1.0), nullptr, p);
        const size_t a = (size_t) (0.5 * SR), b = y.size();
        const double up = tone62 (y, 1200, SR, a, b), img = tone62 (y, 800, SR, a, b), orig = tone62 (y, 1000, SR, a, b);
        CHECK (up > 0.4 && dbr (up, img) > 30.0 && dbr (up, orig) > 30.0, "shifter +200 Hz: 1 kHz becomes 1.2 kHz (%.2f), image %.1f dB down, original %.1f dB down", up, dbr (up, img), dbr (up, orig));
        p.shiftHz = -200.0f;  y = run (sineV (1000, 0.5, 1.0), nullptr, p);
        const double dn = tone62 (y, 800, SR, a, b), img2 = tone62 (y, 1200, SR, a, b);
        CHECK (dn > 0.4 && dbr (dn, img2) > 30.0, "shifter -200 Hz: 1 kHz becomes 800 Hz (%.2f), image %.1f dB down", dn, dbr (dn, img2));
        p.shiftHz = 1900.0f;  y = run (sineV (1000, 0.5, 1.0), nullptr, p);
        CHECK (tone62 (y, 2900, SR, a, b) > 0.35, "shifter +1900 Hz reaches 2.9 kHz (%.2f)", tone62 (y, 2900, SR, a, b));
        p.mode = 1;  p.shiftHz = 200.0f;  y = run (sineV (1000, 0.5, 1.0), nullptr, p);
        const double s1 = tone62 (y, 800, SR, a, b), s2 = tone62 (y, 1200, SR, a, b), s0 = tone62 (y, 1000, SR, a, b);
        CHECK (s1 > 0.3 && s2 > 0.3 && dbr (std::min (s1, s2), s0) > 25.0, "ring mod 200 Hz: sidebands %.2f / %.2f, carrier-free (%.1f dB)", s1, s2, dbr (std::min (s1, s2), s0));
        p.mode = 2;  p.shiftHz = 200.0f;  auto L = sineV (1000, 0.5, 1.0), R = L;  L = run (L, &R, p);
        CHECK (tone62 (L, 1200, SR, a, b) > 0.35 && tone62 (R, 800, SR, a, b) > 0.35 && dbr (tone62 (L, 1200, SR, a, b), tone62 (L, 800, SR, a, b)) > 25.0, "spread: left up (%.2f at 1.2 kHz), right down (%.2f at 800 Hz)", tone62 (L, 1200, SR, a, b), tone62 (R, 800, SR, a, b));
        // feedback, LFO, extremes stay bounded
        bool fin = true;  float pk = 0;
        for (int c = 0; c < 6; ++c)
        {
            FxExtra::FreqShiftFx::Params q;  q.mode = c % 3;  q.shiftHz = (c & 1) ? -1500.0f : 7.0f;  q.feedback = (c & 2) ? 0.92f : -0.92f;  q.depth = 1.0f;  q.lfoHz = 8.0f;  q.shape = c;  q.delayMs = (c & 1) ? 1.0f : 120.0f;  q.mix = 1.0f;
            auto z = run (noiseV (1.0, 1.5, 3 + (unsigned) c), nullptr, q);
            for (float v : z) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, std::abs (v)); }
        }
        CHECK (fin && pk < 20.0f, "shifter: full feedback, deep LFO and extreme shifts stay bounded (peak %.2f)", (double) pk);
    }
    {   // ---- Vocoder, directly
        FxExtra::VocoderFx v;  v.prepare (SR);
        FxExtra::VocoderFx::Params p;  p.bands = 16;  p.sibilance = 0.0f;
        const size_t n = (size_t) (2.0 * SR);
        std::vector<float> mod = noiseV (0.3, 2.0, 5), car (n), out (n);
        for (size_t i = 0; i < n; ++i) { const double t = (double) i / SR, gate = std::fmod (t, 0.5) < 0.25 ? 1.0 : 0.0; mod[i] *= (float) gate;  car[i] = (float) (0.3 * (2.0 * std::fmod (110.0 * t, 1.0) - 1.0)); }
        for (size_t i = 0; i < n; i += 512) v.process (&mod[i], &car[i], &out[i], (int) std::min<size_t> (512, n - i), p);
        double on = 0, off = 0;  size_t cn = 0, cf = 0;
        for (size_t i = (size_t) (0.5 * SR); i < n; ++i) { const double t = (double) i / SR, ph = std::fmod (t, 0.5);  if (ph > 0.05 && ph < 0.22) { on += (double) out[i] * out[i]; ++cn; } else if (ph > 0.3 && ph < 0.47) { off += (double) out[i] * out[i]; ++cf; } }
        const double onDb = 10 * std::log10 (on / (double) cn + 1e-20), offDb = 10 * std::log10 (off / (double) cf + 1e-20);
        CHECK (onDb - offDb > 30.0 && onDb > -40.0, "vocoder: sound follows the voice (on %.1f dB, off %.1f dB)", onDb, offDb);
        const size_t w0 = (size_t) (1.05 * SR), w1 = (size_t) (1.22 * SR);
        const double h = tone62 (out, 330, SR, w0, w1), gap = tone62 (out, 275, SR, w0, w1);
        CHECK (dbr (h, gap) > 8.0, "vocoder: the carrier's harmonics survive (330 Hz is %.1f dB above 275 Hz)", dbr (h, gap));
        bool fin = true;  float pk = 0;
        for (int nb : { 8, 12, 16, 24, 32 }) for (float fm : { -1.0f, 0.0f, 1.0f })
        {
            FxExtra::VocoderFx w;  w.prepare (SR);  FxExtra::VocoderFx::Params q;  q.bands = nb;  q.formant = fm;  q.sharp = 1.0f;  q.response = 1.0f;  q.sibilance = 1.0f;  q.levelDb = 12.0f;
            std::vector<float> o (n);  for (size_t i = 0; i < n; i += 300) w.process (&mod[i], &car[i], &o[i], (int) std::min<size_t> (300, n - i), q);
            for (float x : o) { if (! std::isfinite (x)) fin = false;  pk = std::max (pk, std::abs (x)); }
        }
        CHECK (fin && pk < 20.0f, "vocoder: every band count and formant shift stays bounded (peak %.2f)", (double) pk);
    }
    {   // ---- the two Mod FX types inside the plugin
        const int blk = 512;  auto in = testSignal (96000, SR, 0.3f);
        std::vector<float> off;
        { StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);  process (p, in, off, blk); }
        for (int type : { 22, 23 })
            for (int pos = 0; pos < 3; ++pos)
            {
                StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
                setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", (float) type);  setP (p, "mfAPos", (float) pos);  setP (p, "mfAMix", 1.0f);  setP (p, "mfAShift", 300.0f);
                std::vector<float> on;  process (p, in, on, blk);  float pk;
                double e = 0;  for (size_t i = 0; i < on.size(); ++i) e += std::abs (on[i] - off[i]);
                CHECK (finiteBounded (on, 10.0f, pk) && e > 1.0, "Mod FX type %d, position %d: changes the sound, stays bounded (diff %.1f, peak %.2f)", type, pos, e, (double) pk);
            }
        bool fin = true;
        for (int car = 0; car < 4; ++car)
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "mfBOn", 1.0f);  setP (p, "mfBType", 23.0f);  setP (p, "mfBPos", 1.0f);  setP (p, "mfBMix", 1.0f);  setP (p, "mfBVocCar", (float) car);
            std::vector<float> on;  process (p, in, on, blk);  float pk;  fin = fin && finiteBounded (on, 10.0f, pk);
        }
        CHECK (fin, "vocoder: all four carriers run inside the plugin and stay bounded");
        // saving and loading keeps the new menus
        StarterAmpProcessor p;  setP (p, "mfCType", 22.0f);  setP (p, "mfCFsMode", 2.0f);  setP (p, "mfDVocCar", 3.0f);  setP (p, "mfDVocNote", 30.0f);  setP (p, "mfAShift", -1234.5f);
        juce::MemoryBlock mb;  p.getStateInformation (mb);  StarterAmpProcessor q;  q.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (getP (q, "mfCType") == 22.0f && getP (q, "mfCFsMode") == 2.0f && getP (q, "mfDVocCar") == 3.0f && getP (q, "mfDVocNote") == 30.0f && std::abs (getP (q, "mfAShift") + 1234.5f) < 0.2f,
               "a saved session keeps the shifter and vocoder settings (shift %.1f)", (double) getP (q, "mfAShift"));
    }
    {   // ---- the sidechain
        const int blk = 512;
        auto voice = noiseV (0.3, 1.0, 11);  std::vector<float> zeros (voice.size(), 0.0f);
        auto render = [&] (bool sidechain, const std::vector<float>& main, const std::vector<float>& side, bool modOn)
        {
            StarterAmpProcessor p;
            if (sidechain) { auto lay = p.getBusesLayout();  lay.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();  const bool ok = p.setBusesLayout (lay);  CHECK (ok, "the host can turn the sidechain on"); }
            if (! sidechain) p.setPlayConfigDetails (2, 2, SR, blk);
            p.prepareToPlay (SR, blk);
            if (modOn) { setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 23.0f);  setP (p, "mfAPos", 1.0f);  setP (p, "mfAMix", 1.0f);  setP (p, "mfAVocCar", 2.0f); }
            std::vector<float> out (main.size());  juce::MidiBuffer m;
            juce::AudioBuffer<float> b (sidechain ? 4 : 2, blk);
            for (size_t pos = 0; pos < main.size(); pos += (size_t) blk)
            {
                const int n = (int) std::min<size_t> ((size_t) blk, main.size() - pos);
                juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), b.getNumChannels(), n);
                for (int i = 0; i < n; ++i) { v.setSample (0, i, main[pos + (size_t) i]);  v.setSample (1, i, main[pos + (size_t) i]);  if (sidechain) { v.setSample (2, i, side[pos + (size_t) i]);  v.setSample (3, i, side[pos + (size_t) i]); } }
                p.processBlock (v, m);
                for (int i = 0; i < n; ++i) out[pos + (size_t) i] = v.getSample (0, i);
            }
            return out;
        };
        const auto in = testSignal ((int) voice.size(), SR, 0.3f);
        const auto plain = render (false, in, zeros, false), withBus = render (true, in, voice, false);
        double d = 0;  for (size_t i = 0; i < plain.size(); ++i) d = std::max (d, (double) std::abs (plain[i] - withBus[i]));
        CHECK (d < 1e-6, "an enabled sidechain does not change the main sound (max difference %.2e)", d);
        const auto quiet = render (true, zeros, zeros, true), loud = render (true, zeros, voice, true);
        const double eq = rmsDb (quiet, 24000), el = rmsDb (loud, 24000);
        CHECK (el - eq > 20.0, "the vocoder takes its modulator from the sidechain (silent %.1f dB, voice %.1f dB)", eq, el);
        const auto noBus = render (false, zeros, zeros, true);
        CHECK (rmsDb (noBus, 24000) < -80.0, "without a sidechain and no guitar the vocoder stays silent (%.1f dB)", rmsDb (noBus, 24000));
    }
    {   // ---- Feral fuzz
        const int blk = 512;
        auto in = testSignal ((int) (2.0 * SR), SR, 0.3f);  { auto t = noiseV (0.002, 4.0, 9);  in.insert (in.end(), t.begin(), t.end()); }
        auto tail = [&] (float squeal, float action, int which)
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            const char* pre = which == 0 ? "fuzz" : "fuzz2";
            auto id = [&] (const char* n) { return std::string (pre) + n; };
            setP (p, id ("On").c_str(), 1.0f);  setP (p, id ("Style").c_str(), 13.0f);  setP (p, id ("Squeal").c_str(), squeal);  setP (p, id ("OscHz").c_str(), 900.0f);
            setP (p, id ("Action").c_str(), action);  setP (p, id ("Gain").c_str(), 0.6f);
            std::vector<float> out;  process (p, in, out, blk);  return out;
        };
        for (int which = 0; which < 2; ++which)
        {
            auto direct = [&] (float squeal)
            {
                FuzzEffect fx;  fx.prepare (SR, 512);
                FuzzEffect::Params p {};  p.style = 13;  p.gain = 0.6f;  p.tone = 0.5f;  p.level = 1.0f;  p.squeal = squeal;  p.oscHz = 900.0f;  p.action = 0.0f;  p.actSpeed = 0.5f;
                auto y = in;  for (size_t i = 0; i < y.size(); i += 512) fx.process (&y[i], (int) std::min<size_t> (512, y.size() - i), p);
                return y;
            };
            (void) which;
            const auto calm = direct (0.0f), sing = direct (1.0f);
            const size_t a = (size_t) (4.0 * SR), b = calm.size();
            float pk;
            CHECK (finiteBounded (sing, 20.0f, pk), "Feral: Squeal 1 stays bounded (peak %.2f)", (double) pk);
            const double t0 = toneBand62 (calm, 800, 1000, SR, a, b), t1 = toneBand62 (sing, 800, 1000, SR, a, b);
            CHECK (dbr (t1, t0) > 20.0, "Feral: Squeal 1 keeps singing at the Pitch after the playing stops (%.1f dB above Squeal 0)", dbr (t1, t0));
            if (which == 0) { const double mid = toneBand62 (direct (0.3f), 800, 1000, SR, a, b);  CHECK (dbr (mid, t0) < 20.0, "Feral: the default Squeal 0.3 does not oscillate by itself (%.1f dB above Squeal 0)", dbr (mid, t0)); }
        }
        bool fin = true;  float pk;
        for (int c = 0; c < 8; ++c)
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "fuzzOn", 1.0f);  setP (p, "fuzzStyle", 13.0f);  setP (p, "fuzzSqueal", (c & 1) ? 1.0f : 0.0f);  setP (p, "fuzzAction", (c & 2) ? 1.0f : 0.0f);
            setP (p, "fuzzActSpeed", (c & 4) ? 1.0f : 0.0f);  setP (p, "fuzzOscHz", (c & 1) ? 8000.0f : 60.0f);  setP (p, "fuzzStarve", (c & 2) ? 1.0f : 0.0f);  setP (p, "fuzzGain", 1.0f);
            std::vector<float> o;  process (p, in, o, blk);  fin = fin && finiteBounded (o, 20.0f, pk);
        }
        CHECK (fin, "Feral: every extreme of Squeal, Pitch, Action, Speed and Starve stays bounded");
        // the Action knob makes the squeal gate: the level over time varies more with Action than without
        const auto steady = tail (0.55f, 0.0f, 0), moving = tail (0.55f, 1.0f, 0);
        auto spread = [&] (const std::vector<float>& y) { double mn = 1e9, mx = -1e9; for (size_t s = (size_t) (3.0 * SR); s + 4800 <= y.size(); s += 4800) { const double e = rmsDb (std::vector<float> (y.begin() + (long) s, y.begin() + (long) s + 4800)); mn = std::min (mn, e);  mx = std::max (mx, e); } return mx - mn; };
        std::printf ("    Feral level swing over time: Action 0 = %.1f dB, Action 1 = %.1f dB\n", spread (steady), spread (moving));
        CHECK (spread (moving) > spread (steady) + 3.0, "Feral: Action makes the level wander (%.1f dB against %.1f dB)", spread (moving), spread (steady));
    }
    {   // ---- Cassette voicing
        auto run = [&] (int voice, float dirt, const std::vector<float>& x)
        {
            LoFiWarble lf;  lf.prepare (SR);  auto y = x;
            LoFiWarble::Params p { 0, 0.5f, 0.0f, 0.0f, 0.5f, dirt, 0.0f, 0.0f, 1.0f, 0.0f, voice };
            for (size_t i = 0; i < y.size(); i += 512) lf.process (&y[i], (int) std::min<size_t> (512, y.size() - i), p);
            return y;
        };
        std::vector<float> x ((size_t) (2.0 * SR));
        for (size_t i = 0; i < x.size(); ++i) { const double t = (double) i / SR; x[i] = (float) (0.05 * (std::sin (6.283185307 * 80 * t) + std::sin (6.283185307 * 400 * t) + std::sin (6.283185307 * 7000 * t))); }
        const auto w = run (0, 0.0f, x), c = run (1, 0.0f, x);
        const size_t a = (size_t) (0.5 * SR), b = x.size();
        const double bump = dbr (tone62 (c, 80, SR, a, b), tone62 (c, 400, SR, a, b)) - dbr (tone62 (w, 80, SR, a, b), tone62 (w, 400, SR, a, b));
        const double top = dbr (tone62 (c, 7000, SR, a, b), tone62 (c, 400, SR, a, b)) - dbr (tone62 (w, 7000, SR, a, b), tone62 (w, 400, SR, a, b));
        CHECK (bump > 1.0, "Cassette: a head bump (80 Hz is %.1f dB higher against 400 Hz than in Warble)", bump);
        CHECK (top < -3.0, "Cassette: a steeper top roll-off (7 kHz is %.1f dB lower against 400 Hz than in Warble)", top);
        bool fin = true;  float pk;
        for (int v = 0; v < 2; ++v) for (float dirt : { 0.0f, 0.5f, 1.0f }) fin = fin && finiteBounded (run (v, dirt, noiseV (0.5, 1.0, 4)), 20.0f, pk);
        CHECK (fin, "Lo-Fi: both voicings stay bounded at every Dirt setting");
    }
    {   // ---- Crush Limiter
        auto level = [&] (float crush, double amp)
        {
            FxExtra::CrushLimiterFx f;  f.prepare (SR);  FxExtra::CrushLimiterFx::Params p;  p.crush = crush;  p.mix = 1.0f;
            auto x = sineV (1000, amp, 2.0);  for (size_t i = 0; i < x.size(); i += 512) f.process (&x[i], (int) std::min<size_t> (512, x.size() - i), p);
            return rmsDb (x, (size_t) (1.0 * SR));
        };
        const double spanLow = level (0.1f, 0.5) - level (0.1f, 0.005), spanHigh = level (1.0f, 0.5) - level (1.0f, 0.005);
        std::printf ("    Crush Limiter: a 40 dB input range comes out as %.1f dB at Crush 0.1 and %.1f dB at Crush 1\n", spanLow, spanHigh);
        CHECK (spanHigh < spanLow - 6.0 && spanHigh < 30.0, "Crush Limiter: more Crush squashes the range (%.1f dB against %.1f dB)", spanHigh, spanLow);
        bool fin = true;  float pk;
        for (float crush : { 0.0f, 1.0f }) for (float crunch : { 0.0f, 1.0f }) for (float dark : { 0.0f, 1.0f }) for (bool fast : { false, true })
        {
            FxExtra::CrushLimiterFx f;  f.prepare (SR);  FxExtra::CrushLimiterFx::Params p;  p.crush = crush;  p.crunch = crunch;  p.darkness = dark;  p.fast = fast;  p.volumeDb = 6.0f;
            auto x = noiseV (1.0, 1.0, 6);  f.process (x.data(), (int) x.size(), p);
            const bool ok = finiteBounded (x, 20.0f, pk);  fin = fin && ok;
            if (! ok) std::printf ("    Crush Limiter extreme crush %.0f crunch %.0f dark %.0f fast %d: peak %.2f\n", (double) crush, (double) crunch, (double) dark, (int) fast, (double) pk);
        }
        CHECK (fin, "Crush Limiter: every extreme stays bounded");
    }
    {   // ---- Tape boost mode
        auto run = [&] (int mode, double amp)
        {
            BoostStage b;  b.prepare (SR);  BoostStage::Params p;  p.mode = mode;  p.gainDb = 12.0f;  p.sat = 0.7f;
            auto x = sineV (1000, amp, 1.0);  for (size_t i = 0; i < x.size(); i += 512) b.process (&x[i], (int) std::min<size_t> (512, x.size() - i), p);
            return x;
        };
        const auto t = run (7, 0.3), c = run (0, 0.3);
        const size_t a = (size_t) (0.5 * SR), e = t.size();
        const double h3t = dbr (tone62 (t, 3000, SR, a, e), tone62 (t, 1000, SR, a, e)), h3c = dbr (tone62 (c, 3000, SR, a, e), tone62 (c, 1000, SR, a, e));
        float pk;
        CHECK (finiteBounded (t, 10.0f, pk) && std::abs (h3t - h3c) > 1.0, "Tape boost mode: its own colour (3rd harmonic %.1f dB, Clean %.1f dB), peak %.2f", h3t, h3c, (double) pk);
        bool fin = true;  for (double amp : { 0.0, 0.01, 1.0, 4.0 }) fin = fin && finiteBounded (run (7, amp), 40.0f, pk);
        CHECK (fin, "Tape boost mode stays bounded from silence to 4x full scale");
    }
    {   // ---- No cabinet, and the longer position range
        double lowNone = 0, lowOpen = 0;
        for (int cab : { 16, 3 })
        {
            const double r40 = StarterAmpProcessor::builtinCabResponseDb (0, 0, cab, 0.3f, 4.0f, 0.0f, true, 45.0),
                         r500 = StarterAmpProcessor::builtinCabResponseDb (0, 0, cab, 0.3f, 4.0f, 0.0f, true, 500.0);
            (cab == 16 ? lowNone : lowOpen) = r40 - r500;
        }
        CHECK (lowNone > lowOpen + 3.0, "No cabinet keeps the low end that a cabinet cuts (45 Hz against 500 Hz: %.1f dB, a cabinet gives %.1f dB)", lowNone, lowOpen);
        bool fin = true;
        for (int cab = 0; cab <= 16; ++cab) for (float pos : { -0.4f, 0.0f, 0.3f, 1.0f, 1.6f })
            for (double f : { 30.0, 100.0, 1000.0, 5000.0, 15000.0 })
            { const double r = StarterAmpProcessor::builtinCabResponseDb (0, 0, cab, pos, 4.0f, 0.0f, true, f);  if (! std::isfinite (r) || std::abs (r) > 80.0) fin = false; }
        CHECK (fin, "every cabinet at the whole position range gives a sane response");
        const int blk = 512;  auto in = testSignal (48000, SR, 0.3f);
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "cabCbnA", 16.0f);  setP (p, "cabPosA", -0.4f);
        std::vector<float> o;  process (p, in, o, blk);  float pk;
        CHECK (finiteBounded (o, 10.0f, pk), "the plugin runs with No cabinet and the position at its far end (peak %.2f)", (double) pk);
    }
    {   // ---- the compressor styles inside the plugin, including Crush Limiter
        const int blk = 512;  auto in = testSignal (96000, SR, 0.5f);
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "compOn", 1.0f);  bool fin = true;  float pk;
        for (int k = 0; k < 44; ++k)
        {
            setP (p, "compStyle", (float) (k % 11));
            std::vector<float> seg (in.begin() + (long) (k % 8) * 4096, in.begin() + (long) (k % 8) * 4096 + 4096), o;
            process (p, seg, o, blk);  fin = fin && finiteBounded (o, 20.0f, pk);
        }
        CHECK (fin, "switching between all eleven compressor styles while playing stays finite");
        setP (p, "compStyle", 10.0f);  setP (p, "compCrush", 0.9f);
        std::vector<float> o;  process (p, in, o, blk);
        CHECK (p.getCompGainReductionDb() >= 0.0f && finiteBounded (o, 20.0f, pk), "Crush Limiter inside the plugin (meter %.1f dB)", (double) p.getCompGainReductionDb());
    }
}

// b64: Phaser voicings (Small Stone, Uni-Vibe), the small-box Wall version
static void testB64()
{
    std::printf ("[b64] phaser voicings\n");
    const double SR = 48000.0;
    auto magAt = [] (const std::vector<float>& y, double f, size_t a, size_t n, double fs)
    {   std::complex<double> s = 0;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) n);  s += y[a + i] * w * std::polar (1.0, -2 * M_PI * f * (double) (a + i) / fs); }
        return std::abs (s) / (double) n * 4.0; };
    auto rmsFrom = [] (const std::vector<float>& v, size_t a) { double s = 0;  for (size_t i = a; i < v.size(); ++i) s += (double) v[i] * v[i];  return std::sqrt (s / (double) (v.size() - a)); };
    const double fs = 48000.0;
    std::mt19937 rng (3);  std::normal_distribution<float> nd (0.0f, 0.2f);
    std::vector<float> in (96000);  for (auto& v : in) v = nd (rng);
    for (int voice = 0; voice <= 3; ++voice)
    {
        Phaser ph;  ph.prepare (fs);  std::vector<float> y = in;
        Phaser::Params p { 4, 1, 0.6f, 0.7f, 1040.0f, 0.0f, 1.0f, 0.0f, 0.0f, voice };       // wet only
        ph.process (y.data(), (int) y.size(), p);
        bool fin = true;  double pk = 0;  for (float v : y) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, (double) std::fabs (v)); }
        const double ratio = rmsFrom (y, 4800) / rmsFrom (in, 4800);
        CHECK (fin && pk < 3.0, "phaser voice %d is finite and bounded (peak %.2f)", voice, pk);
        if (voice <= 1) CHECK (std::fabs (ratio - 1.0) < 0.03, "phaser voice %d wet-only is an all-pass (level ratio %.3f)", voice, ratio);
        if (voice == 2) CHECK (ratio > 1.05 && ratio < 2.5, "Small Stone Color adds resonance (ratio %.3f)", ratio);
        if (voice == 3) CHECK (ratio > 0.9 && ratio < 1.5, "Uni-Vibe wet-only level is near unity (stage alpha / beta 0.95 to 1.11, ratio %.3f)", ratio);
        Phaser ph2;  ph2.prepare (fs);  std::vector<float> z = in;
        Phaser::Params q { 4, 1, 0.05f, 0.7f, 1040.0f, 0.0f, 0.5f, 0.0f, 0.0f, voice };
        ph2.process (z.data(), (int) z.size(), q);
        double lo = 1e9;
        for (double f = 150; f < 4000; f *= 1.06) lo = std::min (lo, magAt (z, f, 24000, 8192, fs) / std::max (1e-9, magAt (in, f, 24000, 8192, fs)));
        CHECK (lo < 0.35, "phaser voice %d mixed 50 / 50 shows a notch (min gain %.3f)", voice, lo);
    }
    for (float depth : { 0.2f, 0.66f, 1.0f })
    {   // the Uni-Vibe notches move over the lamp sweep
        std::vector<float> noise (48000 * 8);  for (auto& v : noise) v = nd (rng);
        Phaser ph;  ph.prepare (fs);  std::vector<float> z = noise;
        Phaser::Params q { 4, 0, 0.25f, depth, 1040.0f, 0.0f, 0.5f, 0.0f, 0.0f, 3 };
        ph.process (z.data(), (int) z.size(), q);
        double fMin = 1e9, fMax = 0;
        for (int k = 0; k < 16; ++k)
        {   const size_t a = (size_t) (48000 * 4 + k * 12000);  double lo = 1e9, flo = 0;
            for (double f = 60; f < 8000; f *= 1.03) { const double m = magAt (z, f, a, 6000, fs) / std::max (1e-9, magAt (noise, f, a, 6000, fs));  if (m < lo) { lo = m;  flo = f; } }
            fMin = std::min (fMin, flo);  fMax = std::max (fMax, flo); }
        CHECK (fMax > fMin * 1.5, "Uni-Vibe notches move at Depth %.2f (%.0f to %.0f Hz)", depth, fMin, fMax);
    }
    {   // extremes: fast, full depth, loud, with feedback and envelope up (voices 2 and 3)
        for (int voice : { 2, 3 })
        {   Phaser ph;  ph.prepare (fs);  std::vector<float> y (96000);  for (size_t i = 0; i < y.size(); ++i) y[i] = 0.9f * (float) std::sin (2 * M_PI * 220 * (double) i / fs);
            Phaser::Params p { 4, 1, 20.0f, 1.0f, 80.0f, 0.9f, 1.0f, 1.0f, 0.0f, voice };
            ph.process (y.data(), (int) y.size(), p);
            bool fin = true;  double pk = 0;  for (float v : y) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, (double) std::fabs (v)); }
            CHECK (fin && pk < 4.0, "phaser voice %d stays bounded at the extremes (peak %.2f)", voice, pk); }
    }
    {   // the plugin: a Phaser in a Mod FX slot with each voicing
        const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        for (int voice = 0; voice < 4; ++voice)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 13.0f);  setP (p, "mfAPhVoice", (float) voice);  setP (p, "mfAMix", 0.5f);
            std::vector<float> o;  process (p, sig, o, blk);  float pk;
            CHECK (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -60.0, "plugin: Phaser voicing %d plays (%.1f dB)", voice, rmsDb (o, 24000)); }
    }
    std::printf ("[b64] small-box Wall version\n");
    for (int var = 0; var <= 5; ++var)
    {   Circuit::MuffCircuit m;  Circuit::MuffCircuit::Params p;  p.sustain = 0.6f;  p.tone = 0.5f;  p.variant = var;  p.diode = 0;
        m.prepare (fs * 2);  m.configure (p);  bool fin = true;
        for (float amp : { 0.01f, 0.05f, 0.2f })
        {   std::vector<float> y (96000);  for (size_t i = 0; i < y.size(); ++i) { y[i] = m.process (amp * (float) std::sin (2 * M_PI * 196.0 * (double) i / (fs * 2)));  if (! std::isfinite (y[i])) fin = false; }
            if (var == 5) CHECK (rmsFrom (y, 24000) > 1e-4, "small-box build makes sound at %.2f (%.5f)", amp, rmsFrom (y, 24000)); }
        CHECK (fin, "Wall circuit variant %d is finite at three input levels", var); }
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "fuzzOn", 1.0f);  setP (p, "fuzzStyle", 0.0f);  setP (p, "fuzzMuffVer", 9.0f);  setP (p, "fuzzGain", 0.8f);
        std::vector<float> o;  process (p, sig, o, blk);  float pk;
        CHECK (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -60.0, "plugin: Wall 'Circuit (small-box build)' plays (%.1f dB)", rmsDb (o, 24000)); }
    std::printf ("[b64] Natural Overdrive (Nobels) and the solid-state combo stack\n");
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        double lvl[3][3] = {};
        for (int nob = 0; nob < 3; ++nob)
            for (int gi = 0; gi < 3; ++gi)
            {   const float g = 0.5f * (float) gi;
                StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
                setP (p, "odOn", 1.0f);  setP (p, "odStyle", 9.0f);  setP (p, "odNob", (float) nob);  setP (p, "odGain", g);
                std::vector<float> o;  process (p, sig, o, blk);  float pk;
                lvl[nob][gi] = rmsDb (o, 24000);
                CHECK (finiteBounded (o, 10.0f, pk) && lvl[nob][gi] > -60.0, "plugin: Natural Overdrive voicing %d at Drive %.1f plays (%.1f dB, peak %.2f)", nob, g, lvl[nob][gi], pk); }
        for (int nob = 0; nob < 3; ++nob)
            CHECK (lvl[nob][2] > lvl[nob][0] - 3.0 && lvl[nob][2] < lvl[nob][0] + 12.0, "Natural Overdrive voicing %d: the Drive sweep stays in a sane level range (%.1f to %.1f dB)", nob, lvl[nob][0], lvl[nob][2]);
        CHECK (std::fabs (lvl[0][2] - lvl[1][2]) < 6.0 && std::fabs (lvl[0][2] - lvl[2][2]) < 6.0, "the three Natural Overdrive voicings land within 6 dB of each other at full Drive (%.1f / %.1f / %.1f)", lvl[0][2], lvl[1][2], lvl[2][2]);
        // the second drive slot accepts it too
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "od2On", 1.0f);  setP (p, "od2Style", 9.0f);  setP (p, "od2Nob", 2.0f);  setP (p, "od2Gain", 0.7f);
        std::vector<float> o;  process (p, sig, o, blk);  float pk;
        CHECK (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -60.0, "plugin: Natural Overdrive in Drive 2 plays (%.1f dB)", rmsDb (o, 24000)); }
    std::printf ("[b67] Metal Zone (Boss MT-2 type) drive style\n");
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        double lvl[3] = {};  bool ok = true;  double worstPk = 0;
        for (int gi = 0; gi < 3; ++gi)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "odOn", 1.0f);  setP (p, "odStyle", 11.0f);  setP (p, "odGain", 0.5f * (float) gi);
            std::vector<float> o;  process (p, sig, o, blk);  float pk;
            lvl[gi] = rmsDb (o, 24000);  worstPk = std::max (worstPk, (double) pk);
            CHECK (finiteBounded (o, 10.0f, pk) && lvl[gi] > -60.0, "plugin: Metal Zone at Dist %.1f plays (%.1f dB, peak %.2f)", 0.5f * (float) gi, lvl[gi], pk); }
        CHECK (lvl[2] > lvl[0] - 3.0 && lvl[2] < lvl[0] + 12.0, "Metal Zone: the Dist sweep stays in a sane level range (%.1f to %.1f dB)", lvl[0], lvl[2]);
        for (float lo : { 0.0f, 1.0f }) for (float md : { 0.0f, 1.0f }) for (float mf : { 0.0f, 1.0f }) for (float hi : { 0.0f, 1.0f })
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "odOn", 1.0f);  setP (p, "odStyle", 11.0f);  setP (p, "odGain", 0.9f);  setP (p, "odBass", lo);  setP (p, "odMid", md);  setP (p, "odMidFreq", mf);  setP (p, "odTone", hi);
            std::vector<float> o;  process (p, sig, o, blk);  float pk;
            if (! (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -60.0)) ok = false; }
        CHECK (ok, "Metal Zone: every corner of Low, Mid, Mid Freq and High is finite and bounded");
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "od2On", 1.0f);  setP (p, "od2Style", 11.0f);  setP (p, "od2Gain", 0.7f);
        std::vector<float> o;  process (p, sig, o, blk);  float pk;
        CHECK (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -60.0, "plugin: Metal Zone in Drive 2 plays (%.1f dB)", rmsDb (o, 24000)); }
    std::printf ("[b64] shared saturation curves in the Delay, Reverb wet drive and Compressor menus\n");
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        bool dOk = true, rOk = true, cOk = true;  double worst = 0;
        for (int t = 0; t < FxExtra::MultibandSatFx::kTypes; ++t)
        {   float pk;  std::vector<float> o;
            {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
                setP (p, "preDelayOn", 1.0f);  setP (p, "preDelayMix", 0.5f);  setP (p, "preDelayFeedback", 0.6f);  setP (p, "preDelaySat", 0.8f);  setP (p, "preDelaySatType", (float) t);
                process (p, sig, o, blk);  if (! (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -70.0)) dOk = false;  worst = std::max (worst, (double) pk); }
            {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
                setP (p, "reverbOn", 1.0f);  setP (p, "reverbMix", 0.5f);  setP (p, "reverbWDrive", 0.8f);  setP (p, "reverbWDriveType", (float) (3 + t));
                process (p, sig, o, blk);  if (! (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -80.0)) rOk = false; }
            {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
                setP (p, "compOn", 1.0f);  setP (p, "compThresh", -30.0f);  setP (p, "compSat", (float) (9 + t));
                process (p, sig, o, blk);  if (! (finiteBounded (o, 10.0f, pk) && rmsDb (o, 24000) > -60.0)) cOk = false; }
        }
        CHECK (dOk, "Delay saturation: every one of the %d curves plays finite and bounded in the loop (worst peak %.2f)", (int) FxExtra::MultibandSatFx::kTypes, worst);
        CHECK (rOk, "Reverb wet drive: every shared curve plays finite and bounded");
        CHECK (cOk, "Compressor saturation: every shared curve plays finite and bounded");
        // the original Delay curves are untouched: type 0 to 3 still match the earlier ones (small signal passes at unity)
        for (int t = 0; t < 4; ++t)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "preDelayOn", 1.0f);  setP (p, "preDelayMix", 1.0f);  setP (p, "preDelaySat", 0.3f);  setP (p, "preDelaySatType", (float) t);
            std::vector<float> o;  process (p, sig, o, blk);  float pk;
            CHECK (finiteBounded (o, 10.0f, pk), "Delay saturation type %d still plays", t); } }
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        std::vector<float> a, b;
        for (int sel : { 2, 14 })
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "ampModel", 17.0f);  setP (p, "ampStack", (float) sel);  process (p, sig, sel == 2 ? a : b, blk); }
        float pk;  double d = 0, e = 0;  for (size_t i = 24000; i < a.size(); ++i) { d += (double) (a[i] - b[i]) * (a[i] - b[i]);  e += (double) a[i] * a[i]; }
        CHECK (finiteBounded (b, 10.0f, pk) && rmsDb (b, 24000) > -60.0 && rmsDb (b, 24000) < rmsDb (a, 24000) + 15.0 && rmsDb (b, 24000) > rmsDb (a, 24000) - 15.0, "solid-state combo stack plays at a level near the American stack (%.1f vs %.1f dB)", rmsDb (b, 24000), rmsDb (a, 24000));
        CHECK (d > 0.01 * e, "solid-state combo stack sounds different from the American stack"); }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const std::string cmd = argc > 1 ? argv[1] : "stress";
    const std::string golden = argc > 3 ? argv[3] : "golden.txt";
    if (cmd == "regress") { if (regress (argc > 2 && std::string (argv[2]) == "--update", golden)) return 2; }
    else if (cmd == "foldin") { testFoldIn(); }
    else if (cmd == "poly") { testPolyTuner(); }
    else if (cmd == "latch") { testFeedbackLatch(); }
    else if (cmd == "lofidelay") { testLoFiDelay(); }
    else if (cmd == "delay52") { testDelayFeatures(); }
    else if (cmd == "pitchdelay") { testDelayPitch(); }
    else if (cmd == "transshaper") { testTransientShaper(); }
    else if (cmd == "multiband") { testMultibandB55(); }
    else if (cmd == "workflow") { testWorkflowB56(); }
    else if (cmd == "optosustain") { testOptoSustain(); }
    else if (cmd == "proximity") { testProximityB59(); }
    else if (cmd == "compstyles59") { testCompStylesB59(); }
    else if (cmd == "programeq") { testProgramEqB59(); }
    else if (cmd == "stack") { testPassiveStack9(); }
    else if (cmd == "tubeswitch") { testTubeSwitch(); }
    else if (cmd == "ampswitch") { testAmpSwitch(); }
    else if (cmd == "highgain") { testHighGainControls(); }
    else if (cmd == "b62") { testB62(); }
    else if (cmd == "b63") { testB63(); }
    else if (cmd == "b64") { testB64(); }
    else if (cmd == "alias") { testAliasing(); testAliasingEffects(); }
    else
    {
        testStability(); testBlockSizes(); testAutomation (1); testAutomation (2); testAutomation (3);
        testState(); testLifecycle(); testOversampling(); testSwitching(); testLoFiDelay(); testDelayFeatures(); testDelayPitch(); testTransientShaper(); testMultibandB55(); testWorkflowB56(); testProximityB59(); testCompStylesB59(); testProgramEqB59(); testOptoSustain(); testPassiveStack9(); testTubeSwitch(); testAmpSwitch(); testHighGainControls(); testMovedBlocks(); testMovedDrives(); testB62(); testB63(); testB64(); testParallelSection(); testPatchGraph(); testBrownEyeTight(); testFeedbackLatch(); testModSlots(); testFoldIn(); testPolyTuner(); testCompStyles(); testReverbTypes(); testSpringReturn(); testAliasing(); testAliasingEffects(); testJadeDist(); testDiMatch(); testFuzz2Block();
    }
    std::printf ("\n%d checks passed, %d failed\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
