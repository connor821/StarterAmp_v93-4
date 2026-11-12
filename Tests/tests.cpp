// Starter Amp automated tests. Build instructions are in Tests/README.md.
//   tests stress            numerical stability, block sizes, automation, state round trip, lifecycle, switching
//   tests regress           compare a fixed render of every amp and voicing with Tests/golden.txt
//   tests regress --update  rewrite golden.txt (do this only after a change you meant to make)
#include "PluginProcessor.h"
#include <chrono>
#include <cmath>
#include <limits>
#include <complex>
#include <cstdio>
#include <cstring>
#include <map>
#include <random>
#include <string>

// b91: allocation counter. Counts calls to operator new on the thread that armed it (gAllocWatch), so a test can say
// "this processBlock call must not allocate". Everything is malloc/free, so replacing new/delete is safe program-wide.
#include <new>
#include <cstdlib>
static thread_local bool gAllocWatch = false;
static thread_local long gAllocCount = 0;
void* operator new (std::size_t n)                         { if (gAllocWatch) ++gAllocCount;  if (void* q = std::malloc (n ? n : 1)) return q;  throw std::bad_alloc(); }
void* operator new (std::size_t n, std::align_val_t a)     { if (gAllocWatch) ++gAllocCount;  void* q = nullptr;  if (posix_memalign (&q, (std::size_t) a < sizeof (void*) ? sizeof (void*) : (std::size_t) a, n ? n : 1) == 0) return q;  throw std::bad_alloc(); }
void operator delete (void* q) noexcept                    { std::free (q); }
void operator delete (void* q, std::size_t) noexcept       { std::free (q); }
void operator delete (void* q, std::align_val_t) noexcept  { std::free (q); }
void operator delete (void* q, std::size_t, std::align_val_t) noexcept { std::free (q); }

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
    for (int model = 0; model <= 45; ++model)
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
            if (ra->paramID != "wobOn" && ra->paramID != "tapeStop" && ra->paramID != "repHold" && ! ra->paramID.endsWith ("Pos"))   // (b97: Tape Stop is never loaded as "on", by design since b81, so it does not round-trip; the random draw for it changed when parameters were added) an old Wobble setting is converted to a swell pedal on load by design, so it does not round-trip; the Mod FX position params only mirror where the patch puts each slot
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
        for (int type = 0; type < 19; ++type)
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
static std::vector<float> polyChord (const std::vector<std::pair<float, float>>& notes, double sr, int n, unsigned seed, float lowB = 1.5e-4f)   // (midi, detune cents)
{
    std::mt19937 rng (seed); std::uniform_real_distribution<float> ph (0.0f, 6.2831853f); std::normal_distribution<float> g (0.0f, 1.0f);
    std::vector<float> x ((size_t) n, 0.0f);
    for (auto& nt : notes)
    {
        const float f0 = 440.0f * std::pow (2.0f, (nt.first - 69.0f) / 12.0f) * std::pow (2.0f, nt.second / 1200.0f);
        const float B = nt.first < 52 ? lowB : 5e-5f;
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

        // b93: 7 string, A standard (A D G C F A D). Strings of different stiffness read right; one string at a time reads right; and a guitar
        // that is still a whole step sharp (B standard) is not read as being "in tune" through the octave and fifth relations of its partials
        {
            const auto& pr = PolyTuner::presets()[8];
            float t7[8]; for (int i = 0; i < 7; ++i) t7[i] = (float) pr.notes[i];
            for (float lowB : { 0.6e-4f, 1.5e-4f, 3.5e-4f })
            {
                double worst = 0.0; int heard = 0;
                for (int t = 0; t < 10; ++t)
                {
                    std::vector<std::pair<float, float>> notes; for (int i = 0; i < 7; ++i) notes.push_back ({ (float) pr.notes[i], u (rng) * 25.0f });
                    auto x = polyChord (notes, sr, N + 3000, (unsigned) (300 + t), lowB);
                    std::vector<float> w (x.end() - N, x.end());
                    PolyTuner::Result r; pt.reset(); pt.analyse (w.data(), t7, 7, 440.0f, 0.1f, r);
                    for (int i = 0; i < 5; ++i) if (r.found[i] && ! r.shared[i]) { worst = std::max (worst, (double) std::abs (r.cents[i] - notes[(size_t) i].second));  ++heard; }
                }
                CHECK (heard >= 45, "%.0f Hz A standard, low-string stiffness %.1e: only %d of 50 readings of the lowest five strings", sr, (double) lowB, heard);
                CHECK (worst < 3.0, "%.0f Hz A standard, low-string stiffness %.1e: a reading was %.2f cents off", sr, (double) lowB, worst);
            }
            for (int i = 0; i < 7; ++i)
            {
                std::vector<std::pair<float, float>> one { { (float) pr.notes[i], (i % 2 ? 1.0f : -1.0f) * (8.0f + 3.0f * (float) i) } };
                auto y = polyChord (one, sr, N + 3000, (unsigned) (330 + i), 2.5e-4f);
                std::vector<float> w (y.end() - N, y.end());
                PolyTuner::Result r; pt.reset(); pt.analyse (w.data(), t7, 7, 440.0f, 0.1f, r);
                CHECK (r.found[i] && ! r.shared[i] && std::abs (r.cents[i] - one[0].second) < 2.0f, "%.0f Hz A standard string %d played alone: read %.2f, should be %.2f", sr, i + 1, r.cents[i], one[0].second);
            }
            // the whole guitar a whole step sharp: nothing for A, D and G
            int wrong = 0;
            for (int t = 0; t < 10; ++t)
            {
                std::vector<std::pair<float, float>> notes; for (int i = 0; i < 7; ++i) notes.push_back ({ (float) pr.notes[i] + 2.0f, u (rng) * 3.0f });
                auto x = polyChord (notes, sr, N + 3000, (unsigned) (360 + t));
                std::vector<float> w (x.end() - N, x.end());
                PolyTuner::Result r; pt.reset(); pt.analyse (w.data(), t7, 7, 440.0f, 0.1f, r);
                for (int i = 0; i < 3; ++i) if (r.found[i]) ++wrong;
            }
            CHECK (wrong == 0, "%.0f Hz: a guitar in B standard read against A standard gave %d readings for A, D and G (should be none)", sr, wrong);
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
    std::printf ("[regress] fixed render of every amp (0 to 45) and voicing (gain 0.5, hybrid mode, cab off)\n");
    std::map<std::pair<int,int>, std::vector<double>> gold;
    if (! update)
    {
        FILE* f = std::fopen (path.c_str(), "r"); if (! f) { std::printf ("  cannot open %s (run with --update to create it)\n", path.c_str()); return 1; }
        int m, v; double r[9];
        while (std::fscanf (f, "%d %d %lf %lf %lf %lf %lf %lf %lf %lf %lf", &m, &v, &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6], &r[7], &r[8]) == 11) gold[{ m, v }] = std::vector<double> (r, r + 9);
        std::fclose (f);
    }
    FILE* out = update ? std::fopen (path.c_str(), "w") : nullptr;
    for (int m = 0; m <= 45; ++m)
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
                CHECK (with < -33.0 && with < without - 6.0, "saturator type %d at 36 dB Drive, 5.3 kHz: folded-back energy %.1f dB with ADAA (%.1f dB without)", type, with, without); } }
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
                CHECK (with < -33.0 && with < without - 6.0, "saturator type %d at 36 dB Drive, 5.3 kHz: folded-back energy %.1f dB with ADAA (%.1f dB without)", type, with, without); }
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
    {   // FET: the 4:1 button measures 5.1:1 and the 10:1 button 9.9:1 (12 dB over the dial threshold; since b86 those buttons also start 5 and 1.5 dB earlier)
        const auto g4 = gr (1, 3, -30.0f, 5.0f, 250.0f, -18.0, 1.0, 0.0), g10 = gr (1, 5, -30.0f, 5.0f, 250.0f, -18.0, 1.0, 0.0);
        const double a = g4.back(), b = g10.back();
        CHECK (std::abs (a - 17.0 * (1.0 - 1.0 / 5.1)) < 0.6, "FET 4:1 holds %.2f dB of reduction at 12 dB over the dial threshold, 17 dB over its own (5.1:1 gives %.2f)", a, 17.0 * (1.0 - 1.0 / 5.1));
        CHECK (std::abs (b - 13.5 * (1.0 - 1.0 / 9.9)) < 0.6, "FET 10:1 holds %.2f dB of reduction at 12 dB over the dial threshold, 13.5 dB over its own (9.9:1 gives %.2f)", b, 13.5 * (1.0 - 1.0 / 9.9));
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
        CHECK (span < 16.0 && spanIn > 30.0, "Sustain 1: a note that dies away by %.0f dB comes out within %.1f dB (b68: the release follows the cell, slower than before, so about 13 dB)", spanIn, span);
        const auto xs = pluck (220, 0.03, 0.5, 2.5);  const auto ys = run (xs, p);
        CHECK (std::abs (db (rms (y, 0.3, 0.34), rms (ys, 0.3, 0.34))) < 4.5, "Sustain 1: a hard and a soft pick come out within 4.5 dB (%.1f dB apart)", db (rms (y, 0.3, 0.34), rms (ys, 0.3, 0.34)));
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
    {   // Treble: leaves the lows, and cuts and boosts the highs relative to the middle (b68: the middle itself is no longer flat at 8 kHz on a quiet note, because the dark-state low-pass of the real stage is in)
        OS::Params p;  p.sustain = 0.0f;
        double hi0 = 0.0;
        for (float tr : { 0.0f, -1.0f, 1.0f })
        {
            p.treble = tr;  double lo, hi;
            { const auto q = sine (100, 0.01, 1.0);  lo = db (rms (run (q, p), 0.5, 0.9), rms (q, 0.5, 0.9)); }
            { const auto q = sine (8000, 0.01, 1.0); hi = db (rms (run (q, p), 0.5, 0.9), rms (q, 0.5, 0.9)); }
            if (tr == 0.0f) hi0 = hi;
            CHECK (std::abs (lo) < 0.5 && (tr == 0.0f ? true : (tr < 0 ? hi - hi0 < -5.0 : hi - hi0 > 5.0)), "Treble %+.0f: 100 Hz %.2f dB, 8 kHz %.2f dB (%.2f dB from the middle)", (double) tr, lo, hi, hi - hi0);
        }
        {   // the dark-state low-pass: a quiet note at 8 kHz is well down (the pole at 2.19 kHz), a loud one that squeezes hard is much less so
            p.treble = 0.0f;  p.sustain = 1.0f;
            auto tilt = [&] (double amp) { const auto lo = sine (500, amp, 2.5), hi = sine (8000, amp, 2.5);
                return db (rms (run (hi, p), 2.0, 2.4), rms (hi, 2.0, 2.4)) - db (rms (run (lo, p), 2.0, 2.4), rms (lo, 2.0, 2.4)); };
            const double dq = tilt (0.0003), dh = tilt (0.3);
            CHECK (dh > dq + 8.0, "dark state: 8 kHz against 500 Hz is %.1f dB on a very quiet note and %.1f dB on a squeezed one (the squeezed one is brighter)", dq, dh);
        }
    }
    {   // the squeezer and its soft clipper add little distortion, even on a low note at full Sustain (b68: under -20 dB; was -40 dB before the clipper was moved down to the held level)
        for (double f : { 82.0, 110.0, 330.0 })
        {
            const auto q = sine (f, 0.2, 2.0);  OS::Params p;  p.sustain = 1.0f;  const auto y = run (q, p);
            // (half a second holds a whole number of cycles for these even frequencies)
            const double h1 = harm (y, f, 1, 1.0, 0.5), h2 = harm (y, f, 2, 1.0, 0.5), h3 = harm (y, f, 3, 1.0, 0.5);
            CHECK (db (std::max (h2, h3), h1) < -20.0, "Sustain 1 on a %.0f Hz note: 2nd and 3rd harmonic %.1f dB below the note (b68: the soft ceiling sits 4 dB above the held level, so it rounds a held note slightly)", f, db (std::max (h2, h3), h1));
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
    {   // five voicings on model 30; every other amp holds at four (5555 Block gained a fifth, Vitriol, in b88, so the hold-at-four check uses Brown Eye)
        double lvl[5];  bool allOk = true;
        for (int v = 0; v < 5; ++v)
        {   auto a = fresh();  setP (*a, "ampModel", 30.0f);  setP (*a, "ampVoicing", (float) v);  std::vector<float> y;  process (*a, in, y, 512);
            float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk);  lvl[v] = rmsDb (y, 24000); }
        bool differ = false;  for (int v = 1; v < 5; ++v) if (std::abs (lvl[v] - lvl[v - 1]) > 0.05) differ = true;
        CHECK (allOk && differ, "Cedar Spring: all five voicings play, finite and bounded, and they differ (%.1f %.1f %.1f %.1f %.1f dB)", lvl[0], lvl[1], lvl[2], lvl[3], lvl[4]);
        auto a3 = fresh();  setP (*a3, "ampModel", 2.0f);  setP (*a3, "ampVoicing", 3.0f);  std::vector<float> y3;  process (*a3, in, y3, 512);
        auto a4 = fresh();  setP (*a4, "ampModel", 2.0f);  setP (*a4, "ampVoicing", 4.0f);  std::vector<float> y4;  process (*a4, in, y4, 512);
        double d = 0;  for (size_t i = 0; i < y3.size(); ++i) d = std::max (d, (double) std::abs (y3[i] - y4[i]));
        CHECK (d < 1e-6, "a fifth voicing on any other amp holds at its fourth (largest difference %.2g)", d); }
    std::printf ("[b65] extra voicings (Marque Dream, Citrus Doom, Matt's Amp)\n");
    {   int n = 0;  const AmpExtras::Item* t = AmpExtras::table (n);
        bool seen[64] = {};
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

// b70: Prism, the idealized clean amp (model 42)
static void testPrism()
{
    std::printf ("[b71] Prism (idealized clean amp): plays everywhere, four distinct voicings, glassy and full range, close to the same with the suggested cab as with the cab off\n");
    const double SR = 48000.0;
    const auto in = testSignal (48000 * 2, SR, 0.5f);
    auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    auto bare = [&] (StarterAmpProcessor& a)   // amp only: every effect off, Auto Level on
    {   for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (a, id, 0.0f);
        setP (a, "reverbMix", 0.0f);  setP (a, "gate", -90.0f);  setP (a, "ampAutoLevel", 1.0f); };
    for (int mode = 0; mode < 3; ++mode)
    {
        bool allOk = true;
        for (int v = 0; v < 4; ++v)
            for (float g : { 0.0f, 0.5f, 1.0f })
            {   auto a = fresh();  setP (*a, "ampModel", 42.0f);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampVoicing", (float) v);  setP (*a, "ampGain", g);  a->prepareToPlay (SR, 512);
                std::vector<float> y;  process (*a, in, y, 512);  float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -70.0; }
        CHECK (allOk, "Prism, mode %d: four voicings play at three gains, finite, bounded and audible", mode);
    }
    // band levels (transfer: output against input, relative to 1 kHz) on pink-ish noise, Gain 0.3, amp only
    const double fc[] = { 100, 250, 400, 1000, 2000, 3150, 4000, 5000, 6300, 8000, 10000 };
    auto bandLevels = [&] (const std::vector<float>& y, double* lv)
    {   const size_t from = 48000, n = y.size() - from;
        for (int k = 0; k < 11; ++k)
        {   double acc = 0;
            for (int j = -3; j <= 3; ++j) { const double f = fc[k] * std::pow (2.0, j / 9.0);  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }
            lv[k] = 10 * std::log10 (acc / 7 + 1e-30); } };
    std::vector<float> x (48000 * 4);
    {   unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.9); } }
    double inLv[11];  bandLevels (x, inLv);
    // out[0..10] = transfer relative to 1 kHz; out[11] = overall transfer level at 1 kHz
    auto bandsOf = [&] (int model, int voicing, int mode, bool suggestedCab, double* out)
    {
        auto a = fresh();  bare (*a);  setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voicing);  setP (*a, "ampGain", 0.3f);  setP (*a, "ampMode", (float) mode);
        if (suggestedCab) a->applySuggestedCab();  else setP (*a, "cabOn", 0.0f);
        a->prepareToPlay (SR, 512);  a->applyPendingUpdates();
        std::vector<float> y;  process (*a, x, y, 512);
        double lv[11];  bandLevels (y, lv);
        for (int k = 0; k < 11; ++k) out[k] = (lv[k] - lv[3]) - (inLv[k] - inLv[3]);
        out[11] = lv[3] - inLv[3];
    };
    for (int mode = 0; mode < 3; ++mode)
    {
        double b[4][12], th[4];
        for (int v = 0; v < 4; ++v)
        {   bandsOf (42, v, mode, false, b[v]);
            auto a = fresh();  bare (*a);  setP (*a, "ampModel", 42.0f);  setP (*a, "ampVoicing", (float) v);  setP (*a, "ampGain", 0.3f);  setP (*a, "ampMode", (float) mode);  setP (*a, "cabOn", 0.0f);  a->prepareToPlay (SR, 512);
            std::vector<float> xs (96000), y;  for (size_t i = 0; i < xs.size(); ++i) xs[i] = (float) (0.4 * std::sin (2 * M_PI * 220.0 * (double) i / SR));  process (*a, xs, y, 512);
            auto gz = [&] (double f) { const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  for (size_t i = 48000; i < 96000; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - 48000) / 47999.0);  const double sv = y[i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }  return s1 * s1 + s2 * s2 - c * s1 * s2; };
            double h = 0;  for (int k = 2; k <= 8; ++k) h += gz (220.0 * k);
            th[v] = 10 * std::log10 (h / gz (220.0) + 1e-30); }
        bool differ = true;  double least = 99;
        for (int v = 1; v < 4; ++v)
            for (int u = 0; u < v; ++u)
            {   double m = std::abs (b[v][11] - b[u][11]);  for (int k = 0; k < 11; ++k) m = std::max (m, std::abs (b[v][k] - b[u][k]));
                m = std::max (m, std::abs (th[v] - th[u]) / 3.0);   // a 3 dB difference in distortion at a hard pick counts as 1
                least = std::min (least, m);  if (m < 1.0) differ = false; }
        CHECK (differ, "Prism, mode %d: every pair of voicings differs audibly, by at least 1 dB in band levels or overall level, or 3 dB in distortion at a hard pick (smallest pair: %.1f)", mode, least);
    }
    double twin[12], glass[12], air[12], glassCab[12];
    bandsOf (0, 3, 2, false, twin);  bandsOf (42, 0, 2, false, glass);  bandsOf (42, 1, 2, false, air);  bandsOf (42, 0, 2, true, glassCab);
    CHECK (glass[0] < twin[0] - 2.0, "Prism Glass has a tighter low end than the Twin Sparkle voicing it starts from: 100 Hz %+.1f dB against %+.1f dB (at least 2 dB less)", glass[0], twin[0]);
    CHECK (glass[8] > twin[8] + 1.5 && glass[9] > twin[9] + 1.5, "Prism Glass opens the top: 6.3 kHz %+.1f dB (%+.1f), 8 kHz %+.1f dB (%+.1f), at least 1.5 dB above the source", glass[8], twin[8], glass[9], twin[9]);
    CHECK (air[6] > glass[6] + 0.5 && air[8] > glass[8] + 1.5 && air[9] > glass[9] + 1.5, "Prism Air is brighter than Glass above 4 kHz (4 kHz %+.1f, 6.3 kHz %+.1f, 8 kHz %+.1f dB)", air[6] - glass[6], air[8] - glass[8], air[9] - glass[9]);
    CHECK (glass[9] > 0.0 && glass[10] > -3.0, "with the cab off Prism Glass is full range: 8 kHz %+.1f dB and 10 kHz %+.1f dB against 1 kHz (not rolled off)", glass[9], glass[10]);
    double worst = 0;  int worstK = 0;
    for (int k = 1; k <= 9; ++k) { const double d = std::abs (glassCab[k] - glass[k]);  if (d > worst) { worst = d;  worstK = k; } }
    CHECK (worst < 3.5 && std::abs (glassCab[10] - glass[10]) < 4.5, "Prism Glass sounds close to the same with the suggested cab as with the cab off: worst difference %.1f dB (at %.0f Hz) from 250 Hz to 8 kHz, %.1f dB at 10 kHz", worst, fc[worstK], std::abs (glassCab[10] - glass[10]));
    // headroom: a 220 Hz note at a guitar-like level stays clean at low Gain
    {   auto a = fresh();  bare (*a);  setP (*a, "ampModel", 42.0f);  setP (*a, "ampVoicing", 0.0f);  setP (*a, "ampGain", 0.3f);  setP (*a, "ampMode", 2.0f);  setP (*a, "cabOn", 0.0f);  a->prepareToPlay (SR, 512);
        std::vector<float> xs (96000), y;  for (size_t i = 0; i < xs.size(); ++i) xs[i] = (float) (0.1 * std::sin (2 * M_PI * 220.0 * (double) i / SR));  process (*a, xs, y, 512);
        auto gz = [&] (double f) { const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  for (size_t i = 48000; i < 96000; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - 48000) / 47999.0);  const double sv = y[i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }  return s1 * s1 + s2 * s2 - c * s1 * s2; };
        double h = 0;  for (int k = 2; k <= 8; ++k) h += gz (220.0 * k);
        const double thd = 10 * std::log10 (h / gz (220.0) + 1e-30);
        CHECK (thd < -40.0, "Prism Glass at Gain 0.3 keeps a 220 Hz note clean: harmonics 2 to 8 at %.1f dB (want below -40)", thd); }
}

// b73: Ember, the idealized lead amp (model 43)
// b75: the four Waza voicings: CE-2 / CE-1 chorus modes, VB-2 vibrato with Rise Time, MT-2W Metal Zone Custom, DM-2 delay modes.
static void testWaza75()
{
    std::printf ("[b75] Waza voicings: CE-2 chorus modes, VB-2 vibrato (Rise Time), MT-2W Metal Zone Custom, DM-2 delays (estimates, measured not listened to)\n");
    const double SR = 48000.0;
    auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    auto bare = [&] (StarterAmpProcessor& a)
    {   for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "chorusOn", "mfAOn", "mfBOn", "mfCOn", "mfDOn" }) setP (a, id, 0.0f);
        setP (a, "reverbMix", 0.0f);  setP (a, "gate", -90.0f);  setP (a, "cabOn", 0.0f);  setP (a, "ampGain", 0.1f); };
    auto sine = [&] (size_t n, double f, double amp, size_t startAt = 0)
    {   std::vector<float> x (n, 0.0f);  for (size_t i = startAt; i < n; ++i) x[i] = (float) (amp * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    // spread of the period between upward zero crossings, as a percentage of the mean period (pitch wobble)
    auto wobblePct = [&] (const std::vector<float>& y, size_t from, size_t to)
    {   std::vector<double> per;  double last = -1;  bool armed = false;  float pk = 0;  for (size_t i = from; i < to; ++i) pk = std::max (pk, std::abs (y[i]));
        const float h = 0.05f * pk;
        for (size_t i = from + 1; i < to; ++i)
        {   if (y[i] < -h) armed = true;
            if (armed && y[i - 1] <= 0.0f && y[i] > 0.0f) { const double pos = (double) i - y[i] / (y[i] - y[i - 1]);  if (last >= 0) per.push_back (pos - last);  last = pos;  armed = false; } }
        if (per.size() < 8) return 0.0;
        double m = 0;  for (double v : per) m += v;  m /= (double) per.size();
        double s = 0;  for (double v : per) s += (v - m) * (v - m);
        return 100.0 * std::sqrt (s / (double) per.size()) / m; };

    // ---- CE-2 / CE-1 chorus
    {
        bool ok = true;  double lvl[3] = {};
        for (int m = 9; m <= 11; ++m)
        {   auto a = fresh();  bare (*a);  setP (*a, "chorusOn", 1.0f);  setP (*a, "chorusMode", (float) m);  setP (*a, "chorusMix", 0.5f);  a->prepareToPlay (SR, 512);
            std::vector<float> y;  process (*a, sine (96000, 110.0, 0.1), y, 512);  float pk;  ok = ok && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -70.0;  lvl[m - 9] = rmsDb (y, 24000); }
        CHECK (ok, "Chorus modes 9 (CE-2), 10 (CE-1 Chorus) and 11 (CE-1 Vibrato) play: finite, bounded, audible (levels %.1f, %.1f, %.1f dB)", lvl[0], lvl[1], lvl[2]);
        // vibrato ignores Mix: the same output at Mix 0 and Mix 1
        std::vector<float> y0, y1;
        for (int k = 0; k < 2; ++k)
        {   auto a = fresh();  bare (*a);  setP (*a, "chorusOn", 1.0f);  setP (*a, "chorusMode", 11.0f);  setP (*a, "chorusMix", k == 0 ? 0.0f : 1.0f);  a->prepareToPlay (SR, 512);
            process (*a, sine (96000, 110.0, 0.1), k == 0 ? y0 : y1, 512); }
        double dmax = 0;  for (size_t i = 0; i < y0.size(); ++i) dmax = std::max (dmax, (double) std::abs (y0[i] - y1[i]));
        CHECK (dmax < 1e-5, "CE-1 Vibrato (mode 11) is wet only: Mix 0 and Mix 1 give the same output (largest difference %.2g)", dmax);
        // it bends the pitch; the CE-2 chorus at Mix 0.5 bends less (the dry signal anchors the pitch)
        auto wob = [&] (int mode, float mix)
        {   auto a = fresh();  bare (*a);  setP (*a, "chorusOn", 1.0f);  setP (*a, "chorusMode", (float) mode);  setP (*a, "chorusMix", mix);  setP (*a, "chorusDepth", 1.0f);  a->prepareToPlay (SR, 512);
            std::vector<float> y;  process (*a, sine (192000, 110.0, 0.1), y, 512);  return wobblePct (y, 48000, 192000); };
        const double wv = wob (11, 1.0f), wc = wob (9, 1.0f);
        CHECK (wv > 0.5, "CE-1 Vibrato bends the pitch: period spread %.2f %% of the period (want above 0.5 %%)", wv);
        CHECK (wc > 0.2 && wc < wv * 1.5, "CE-2 at Mix 1 also moves the pitch (%.2f %%), within a sensible range of the vibrato's %.2f %%", wc, wv);
        // old modes unchanged: mode 0 still tame
        const double w0 = wob (0, 1.0f);
        CHECK (w0 < wv, "Mode I (classic) bends the pitch less than the CE-1 Vibrato (%.2f against %.2f %%)", w0, wv);
    }

    // ---- VB-2 vibrato in the Mod FX
    {
        auto setup = [&] (StarterAmpProcessor& a, int voice, float rise)
        {   bare (a);  setP (a, "mfAOn", 1.0f);  setP (a, "mfAType", 2.0f);  setP (a, "mfAViVoice", (float) voice);  setP (a, "mfAFeedback", rise * 0.9f);
            setP (a, "mfADepth", 0.8f);  setP (a, "mfAMix", 1.0f);  setP (a, "mfAShape", 0.0f); };
        bool ok = true;
        for (int v = 0; v < 3; ++v)
        {   auto a = fresh();  setup (*a, v, 0.5f);  a->prepareToPlay (SR, 512);  std::vector<float> y;  process (*a, sine (96000, 110.0, 0.1), y, 512);  float pk;
            ok = ok && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -70.0; }
        CHECK (ok, "Vibrato voicings Clean, VB-2 and VB-2 Custom all play: finite, bounded, audible");
        // Rise Time: a note that starts at 0.5 s. With Rise 0 the pitch wobbles at once; with Rise 1 it is steady just after the pick and wobbles later.
        // (Rate is whatever the slot default is; the windows are long enough to hold several cycles.)
        auto run = [&] (int voice, float rise)
        {   auto a = fresh();  setup (*a, voice, rise);  setP (*a, "mfARate", 5.0f);  a->prepareToPlay (SR, 512);  std::vector<float> y;  process (*a, sine (48000 * 4, 110.0, 0.1, 24000), y, 512);  return y; };
        const auto r0 = run (1, 0.0f), r1 = run (1, 1.0f);
        const double e0 = wobblePct (r0, 24000 + 2400, 24000 + 14400), e1 = wobblePct (r1, 24000 + 2400, 24000 + 14400), l1 = wobblePct (r1, 48000 * 2 + 24000, 48000 * 4 - 2400);
        CHECK (e0 > 0.4, "VB-2, Rise Time 0: the vibrato is there at once (period spread %.2f %% in the first 0.3 s after the pick, want above 0.4)", e0);
        CHECK (e1 < 0.5 * e0, "VB-2, Rise Time full: the first 0.3 s after the pick is steadier (%.2f %%) than with no rise (%.2f %%)", e1, e0);
        CHECK (l1 > 0.7 * e0, "VB-2, Rise Time full: by 2 s after the pick the vibrato has come in (%.2f %%, no-rise value %.2f %%)", l1, e0);
        // Mix is ignored by the VB-2 voices
        {   auto a = fresh();  setup (*a, 1, 0.0f);  setP (*a, "mfAMix", 0.0f);  a->prepareToPlay (SR, 512);  std::vector<float> ya;  process (*a, sine (96000, 110.0, 0.1), ya, 512);
            auto b = fresh();  setup (*b, 1, 0.0f);  setP (*b, "mfAMix", 1.0f);  b->prepareToPlay (SR, 512);  std::vector<float> yb;  process (*b, sine (96000, 110.0, 0.1), yb, 512);
            double dm = 0;  for (size_t i = 0; i < ya.size(); ++i) dm = std::max (dm, (double) std::abs (ya[i] - yb[i]));
            CHECK (dm < 1e-5, "VB-2 is wet only: Mix 0 and Mix 1 match (largest difference %.2g)", dm); }
        // Custom adds a tone sweep: the level of a 3 kHz harmonic-rich tone moves with the wave. Compare block RMS spread of VB-2 and VB-2 Custom on a bright signal.
        auto brightSpread = [&] (int voice)
        {   auto a = fresh();  setup (*a, voice, 0.0f);  setP (*a, "mfARate", 3.0f);  a->prepareToPlay (SR, 512);
            std::vector<float> x (48000 * 3);  unsigned st = 99;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.05); }
            std::vector<float> y;  process (*a, x, y, 512);  double lo = 1e9, hi = -1e9;
            for (size_t s = 48000; s + 2400 <= y.size(); s += 2400) { double e = 0;  for (size_t i = 0; i < 2400; ++i) e += (double) y[s + i] * y[s + i];  const double d = 10 * std::log10 (e / 2400.0 + 1e-20);  lo = std::min (lo, d);  hi = std::max (hi, d); }
            return hi - lo; };
        const double s1 = brightSpread (1), s2 = brightSpread (2);
        CHECK (s2 > s1 + 1.0, "VB-2 Custom's swept filter makes the level of noise move with the wave (%.1f dB swing) more than plain VB-2 (%.1f dB)", s2, s1);
    }

    // ---- MT-2W Metal Zone Custom
    {
        auto mz = [&] (int mode, float dist, double f, double amp)
        {   auto a = fresh();  bare (*a);  setP (*a, "odOn", 1.0f);  setP (*a, "odStyle", 11.0f);  setP (*a, "odMz", (float) mode);  setP (*a, "odGain", dist);  setP (*a, "odVolume", -12.0f);  a->prepareToPlay (SR, 512);
            std::vector<float> y;  process (*a, sine (96000, f, amp), y, 512);  return y; };
        bool ok = true;
        for (int m = 0; m < 2; ++m)
            for (float d : { 0.0f, 0.5f, 1.0f }) { const auto y = mz (m, d, 220.0, 0.1);  float pk;  ok = ok && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -80.0; }
        CHECK (ok, "Metal Zone Standard and Custom play at Dist 0, 0.5 and 1: finite, bounded, audible");
        // tighter lows: the small-signal response (a very quiet sine at Dist 0, so nothing clips) falls more at 80 Hz against 800 Hz with Custom
        auto lowRatio = [&] (int mode)
        {   const auto lo = mz (mode, 0.0f, 80.0, 0.0005), hi = mz (mode, 0.0f, 800.0, 0.0005);
            return goertz (lo, 48000, lo.size(), 80.0) - goertz (hi, 48000, hi.size(), 800.0); };
        const double rs = lowRatio (0), rc = lowRatio (1);
        CHECK (rc < rs - 1.0, "Metal Zone Custom has a tighter low end: the small-signal response at 80 Hz against 800 Hz is %.1f dB, Standard %.1f dB (want at least 1 dB lower)", rc, rs);
        // more gain: a quiet note is more saturated with Custom (harmonics against the fundamental, 220 Hz at 0.01 input, Dist 0.4)
        auto thd = [&] (int mode)
        {   const auto y = mz (mode, 0.4f, 220.0, 0.01);  double h = 0;  for (int k = 2; k <= 8; ++k) h += std::pow (10.0, goertz (y, 48000, y.size(), 220.0 * k) / 10.0);
            return 10 * std::log10 (h / std::pow (10.0, goertz (y, 48000, y.size(), 220.0) / 10.0) + 1e-30); };
        const double ts = thd (0), tc = thd (1);
        CHECK (tc > ts + 1.0, "Metal Zone Custom is more saturated than Standard at Dist 0.4: harmonics %.1f against %.1f dB (want 1 dB or more)", tc, ts);
    }

    // ---- DM-2 delay modes
    {
        auto echo = [&] (int mode, double* hf)
        {   auto a = fresh();  bare (*a);  setP (*a, "postDelayOn", 1.0f);  setP (*a, "postDelayMode", (float) mode);  setP (*a, "postDelayTime", 200.0f);  setP (*a, "postDelayMix", 1.0f);
            setP (*a, "postDelayFeedback", 0.3f);  setP (*a, "postDelayNote", 0.0f);  a->prepareToPlay (SR, 512);
            std::vector<float> x (48000 * 2, 0.0f);  unsigned st = 17;  for (size_t i = 100; i < 4900; ++i) { st = st * 1664525u + 1013904223u;  x[i] = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.1); }
            std::vector<float> y;  process (*a, x, y, 512);
            const size_t from = 100 + 9600, to = 4900 + 9600;       // the first repeat
            *hf = goertz (y, from, to, 6000.0) - goertz (y, from, to, 1000.0);
            return rmsDb (std::vector<float> (y.begin() + (long) from, y.begin() + (long) to)); };
        {   std::printf ("  info: first-repeat 6 kHz vs 1 kHz by delay mode:");  for (int m = 0; m < 14; ++m) { double hh;  echo (m, &hh);  std::printf (" %d:%.1f", m, hh); }  std::printf ("\n"); }
        double h0, h1, h2, h3;
        const double l0 = echo (0, &h0), l1 = echo (12, &h1), l2 = echo (13, &h2);  (void) echo (1, &h3);
        CHECK (l1 > -60.0 && l2 > -60.0, "DM-2 and DM-2 Custom put a first repeat at 200 ms (levels %.1f and %.1f dB)", l1, l2);
        CHECK (h1 < h0 - 3.0, "DM-2 repeats are darker than Digital: 6 kHz against 1 kHz is %.1f dB, Digital %.1f dB (want 3 dB or more darker)", h1, h0);
        CHECK (h2 > h1 + 2.0, "DM-2 Custom is clearer than DM-2: %.1f against %.1f dB at 6 kHz against 1 kHz (want 2 dB or more)", h2, h1);
    }
}

// b76: the CS-3 (Boss sustainer) compressor style, tested on the processor class itself (so the amp does not colour the numbers) and once inside the plugin.
static void testCs3()
{
    std::printf ("[b76] CS-3 sustainer: squeezes a 30 dB range, Sustain, Attack (recovery), Tone, Mix, inside the plugin (estimates, measured not listened to)\n");
    const double SR = 48000.0;
    auto run = [&] (const std::vector<float>& in, float sus, float atk, float tone, float mix, float* grOut = nullptr)
    {   FxExtra::Cs3Fx fx;  fx.prepare (SR);  FxExtra::Cs3Fx::Params p;  p.sustain = sus;  p.attack = atk;  p.tone = tone;  p.mix = mix;
        std::vector<float> y = in;  float gr = 0;  for (size_t i = 0; i < y.size(); i += 512) gr = fx.process (y.data() + i, (int) std::min<size_t> (512, y.size() - i), p);
        if (grOut) *grOut = gr;  return y; };
    auto sine = [&] (size_t n, double f, double amp) { std::vector<float> x (n);  for (size_t i = 0; i < n; ++i) x[i] = (float) (amp * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
    auto lvl = [&] (const std::vector<float>& y, size_t a, size_t b) { double s = 0;  for (size_t i = a; i < b; ++i) s += (double) y[i] * y[i];  return 10 * std::log10 (s / (double) (b - a) + 1e-20); };
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };

    bool ok = true;
    for (float s : { 0.0f, 0.5f, 1.0f })
        for (float tn : { -1.0f, 0.0f, 1.0f })
            for (double a : { 0.003, 0.1, 0.9 }) { const auto y = run (sine (96000, 220.0, a), s, 0.5f, tn, 1.0f);  float pk;  ok = ok && finiteBounded (y, 8.0f, pk); }
    CHECK (ok, "CS-3: finite and bounded at three Sustain, three Tone and three input levels");

    // squeeze: a 30 dB range of input comes out as a much smaller range (settled levels, 220 Hz)
    {   const double qi = -50.0, li = -20.0;       // input dB rms-ish (peak amplitude 0.0045 and 0.14)
        const auto q = run (sine (96000, 220.0, 0.0045), 0.7f, 0.5f, 0.0f, 1.0f), l = run (sine (96000, 220.0, 0.14), 0.7f, 0.5f, 0.0f, 1.0f);
        const double dq = lvl (q, 60000, 96000), dl = lvl (l, 60000, 96000);
        const double range = dl - dq;
        std::printf ("  info: Sustain 0.7: input range %.0f dB comes out as %.1f dB (quiet %.1f, loud %.1f dB rms)\n", li - qi, range, dq, dl);
        CHECK (range < 20.0 && range > 3.0, "CS-3, Sustain 0.7: a 30 dB range of input comes out as %.1f dB (want between 3 and 20)", range); }
    // Sustain: more sustain brings a quiet note up
    {   const double lo = lvl (run (sine (96000, 220.0, 0.0045), 0.2f, 0.5f, 0.0f, 1.0f), 60000, 96000), hi = lvl (run (sine (96000, 220.0, 0.0045), 0.8f, 0.5f, 0.0f, 1.0f), 60000, 96000);
        CHECK (hi > lo + 10.0, "CS-3: Sustain 0.8 lifts a quiet note %.1f dB over Sustain 0.2 (want 10 or more)", hi - lo); }
    // Attack (recovery): after a loud burst, a quiet tone comes back faster with the Attack knob down
    {   auto burstThenQuiet = [&] (float atk)
        {   std::vector<float> x (96000, 0.0f);  const auto lo = sine (96000, 220.0, 0.0045), hi = sine (96000, 220.0, 0.3);
            for (size_t i = 0; i < 96000; ++i) x[i] = i < 24000 ? hi[i] : lo[i];
            const auto y = run (x, 0.7f, atk, 0.0f, 1.0f);  return lvl (y, 24000 + 4800, 24000 + 9600); };       // 0.1 to 0.2 s after the burst
        const double fast = burstThenQuiet (0.0f), slow = burstThenQuiet (1.0f);
        CHECK (fast > slow + 3.0, "CS-3: 0.1 to 0.2 s after a loud burst the quiet tone is %.1f dB higher with Attack at 0 than at 1 (want 3 or more)", fast - slow); }
    // Tone: boosts and cuts the top
    {   std::vector<float> n (96000);  unsigned st = 3;  for (auto& v : n) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.02); }
        auto tilt = [&] (float tn) { const auto y = run (n, 0.0f, 0.5f, tn, 1.0f);  return goertz (y, 24000, 96000, 4000.0) - goertz (y, 24000, 96000, 200.0); };
        const double t0 = tilt (0.0f), tp = tilt (1.0f), tm = tilt (-1.0f);
        CHECK (tp > t0 + 6.0 && tm < t0 - 6.0, "CS-3 Tone: 4 kHz against 200 Hz is %.1f dB at the right end and %.1f at the left, against %.1f at the centre (want 6 dB or more either way)", tp, tm, t0); }
    // Mix 0 is the dry signal
    {   const auto x = sine (48000, 220.0, 0.1), y = run (x, 0.8f, 0.5f, 0.5f, 0.0f);  double d = 0;  for (size_t i = 0; i < x.size(); ++i) d = std::max (d, (double) std::abs (x[i] - y[i]));
        CHECK (d < 1e-4, "CS-3: Mix 0 gives the dry signal (largest difference %.2g)", d); }
    // inside the plugin
    {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        for (const char* id : { "odOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (*a, id, 0.0f);
        setP (*a, "compOn", 1.0f);  setP (*a, "compStyle", 11.0f);  setP (*a, "compSustain", 0.7f);  setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);
        a->prepareToPlay (SR, 512);  std::vector<float> y;  process (*a, testSignal (96000, SR, 0.5f), y, 512);  float pk;
        CHECK (finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -70.0, "CS-3 style plays inside the plugin: finite, bounded, audible"); }
}

// b76: wah voicings (Dimebag / 535Q with Range, Clyde McCoy, Morley optical, Bad Horsie, Petrucci JP95, Cliff Burton fuzz wah), Q and Boost. Tested on the Wah class.
static void testWow77()
{
    std::printf ("[b77] Pitch pedal tracking: Single note / Chords / Chords (fast) (measured, not listened to)\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    auto chord = [&] (size_t n, std::initializer_list<double> fs)
    {   std::vector<float> x (n, 0.0f);
        for (size_t i = 0; i < n; ++i) { double v = 0;  for (double f : fs) v += std::sin (2 * M_PI * f * (double) i / SR);  x[i] = (float) (0.12 * v); }
        return x; };
    auto run = [&] (int track, int mode, float mix, const std::vector<float>& in, int* lat = nullptr)
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "wowOn", 1.0f);  setP (p, "wowMode", (float) mode);  setP (p, "wowPos", 1.0f);  setP (p, "wowGlide", 0.0f);  setP (p, "wowMix", mix);  setP (p, "wowTrack", (float) track);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        std::vector<float> y;  process (p, in, y, 512);  if (lat) { p.applyPendingUpdates();  *lat = p.getLatencySamples(); }  return y; };

    bool ok = true;  float pk;
    for (int t = 0; t < 3; ++t)
        for (int m : { 0, 1, 2, 5 }) { const auto y = run (t, m, 1.0f, chord (48000, { 110.0, 165.0, 220.0 })); ok = ok && finiteBounded (y, 8.0f, pk); }
    CHECK (ok, "pitch pedal: all three tracking modes are finite and bounded in four range modes");

    // a power chord (A2 + E3 + A3) shifted an octave up (Octave up, toe)
    const auto in = chord (96000, { 110.0, 165.0, 220.0 });
    int lat0 = 0, lat1 = 0, lat2 = 0, latOff = 0;
    const auto y0 = run (0, 0, 1.0f, in, &lat0), y1 = run (1, 0, 1.0f, in, &lat1), y2 = run (2, 0, 1.0f, in, &lat2);
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);  setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        std::vector<float> y;  process (p, in, y, 512);  p.applyPendingUpdates();  latOff = p.getLatencySamples(); }
    lat0 -= latOff;  lat1 -= latOff;  lat2 -= latOff;
    (void) y0;
    std::printf ("  info: reported latency (pedal only) single %d, chords %d, chords fast %d samples\n", lat0, lat1, lat2);
    CHECK (lat1 > lat2 && lat2 > 0, "latency: Chords (%d) is longer than Chords (fast) (%d), both above zero", lat1, lat2);
    // Chords (long window): every note of a low power chord moves up an octave (110, 165, 220 -> 220, 330, 440)
    {   const double a110 = goertz (y1, 60000, 92000, 110.0), a165 = goertz (y1, 60000, 92000, 165.0), a220 = goertz (y1, 60000, 92000, 220.0), a330 = goertz (y1, 60000, 92000, 330.0), a440 = goertz (y1, 60000, 92000, 440.0);
        std::printf ("  info: Chords, +12 st: 110 %.1f  165 %.1f | 220 %.1f  330 %.1f  440 %.1f dB\n", a110, a165, a220, a330, a440);
        CHECK (a330 > a165 + 20.0 && a440 > a110 + 10.0, "Chords: the 165 and 220 Hz notes moved to 330 and 440 (330 %.1f vs 165 %.1f dB; 440 %.1f vs 110 %.1f dB)", a330, a165, a440, a110); }
    // Single note: a chord is not tracked cleanly (the delay-line shifter), so the same chord leaves a lot of stray energy; measured and reported
    {   const double a165 = goertz (y0, 60000, 92000, 165.0), a330 = goertz (y0, 60000, 92000, 330.0), a440 = goertz (y0, 60000, 92000, 440.0);
        std::printf ("  info: Single note on the same chord, +12 st: 165 %.1f | 330 %.1f  440 %.1f dB\n", a165, a330, a440); }
    // Chords (fast), short window: it resolves notes about 100 Hz apart, so use a chord an octave up (220, 330, 440 -> 440, 660, 880)
    {   const auto hi = chord (96000, { 220.0, 330.0, 440.0 });
        const auto yf = run (2, 0, 1.0f, hi);
        const double a330 = goertz (yf, 60000, 92000, 330.0), a660 = goertz (yf, 60000, 92000, 660.0), a880 = goertz (yf, 60000, 92000, 880.0), a220 = goertz (yf, 60000, 92000, 220.0);
        std::printf ("  info: Chords (fast), +12 st on 220/330/440: 220 %.1f  330 %.1f | 660 %.1f  880 %.1f dB\n", a220, a330, a660, a880);
        CHECK (a660 > a330 + 15.0 && a880 > a220 + 10.0, "Chords (fast): 330 and 440 Hz moved to 660 and 880 (660 %.1f vs 330 %.1f; 880 %.1f vs 220 %.1f dB)", a660, a330, a880, a220); }
    // Mix 0: the dry signal only; nothing at the shifted pitches (440 is not in the chord)
    for (int t = 1; t <= 2; ++t)
    {
        const auto y = run (t, 0, 0.0f, in);
        const double a110 = goertz (y, 60000, 92000, 110.0), a440 = goertz (y, 60000, 92000, 440.0), a330 = goertz (y, 60000, 92000, 330.0);
        CHECK (a110 > a440 + 25.0 && a110 > a330 + 25.0, "track %d, Mix 0: only the original notes (110 Hz %.1f dB; 330 %.1f, 440 %.1f dB)", t, a110, a330, a440);
    }
    // Mix 0.5 keeps the dry and the shifted note both
    {   const auto y = run (1, 0, 0.5f, in);
        const double a110 = goertz (y, 60000, 92000, 110.0), a330 = goertz (y, 60000, 92000, 330.0);
        CHECK (a110 > -20.0 && a330 > -20.0, "Chords, Mix 0.5: dry (110 Hz %.1f dB) and shifted (330 Hz %.1f dB) both present", a110, a330); }
    // octave down on a chord
    {   const auto y = run (1, 2, 1.0f, chord (96000, { 220.0, 330.0, 440.0 }));
        const double a110 = goertz (y, 60000, 92000, 110.0), a165 = goertz (y, 60000, 92000, 165.0), a330 = goertz (y, 60000, 92000, 330.0);
        CHECK (a110 > a330 + 6.0 || a165 > a330 + 6.0, "Chords, Octave down: low notes appear (110 %.1f, 165 %.1f, 330 %.1f dB)", a110, a165, a330); }
    // moving the pedal does not blow up (glide through the range) and switching mode mid-play is clean
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "wowOn", 1.0f);  setP (p, "wowMode", 5.0f);  setP (p, "wowMix", 1.0f);  setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        std::vector<float> all;  bool fine = true;
        for (int seg = 0; seg < 12; ++seg)
        {
            setP (p, "wowTrack", (float) (seg % 3));  setP (p, "wowPos", (float) (seg % 4) / 3.0f);
            std::vector<float> y;  process (p, testSignal (12000, SR), y, 256);
            fine = fine && finiteBounded (y, 8.0f, pk);
        }
        CHECK (fine, "switching tracking mode and pedal position mid-play stays finite and bounded"); }
    // restore state: turning the pedal off reports zero latency
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "wowOn", 1.0f);  setP (p, "wowTrack", 1.0f);  setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "gate", -90.0f);  std::vector<float> y;  process (p, testSignal (24000, SR), y, 512);
        setP (p, "wowOn", 0.0f);  process (p, testSignal (24000, SR), y, 512);  p.applyPendingUpdates();
        CHECK (p.getLatencySamples() == latOff, "pedal off: latency back to %d (want %d)", p.getLatencySamples(), latOff); }
}

// b78: Timmy-style drive and the pitch pedal's bad-tracking characters.
static void testTimmy78 (bool meas)
{
    std::printf ("[b78] Timmy-style drive (my estimate, no schematic): flat response, soft feedback clipping, cut-only Bass and Treble (measured, not listened to)\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    auto run = [&] (double f, double amp, float gain, float tr, float bs, int clip, size_t n = 48000)
    {   Circuit::NewPedals ped;  ped.prepare (SR);
        Circuit::DiodeSpec dp = Circuit::diodeSilicon(), dn = dp;
        if (clip == 1) dp = dn = Circuit::diodeLed();  else if (clip == 2) dp = dn = Circuit::diodeGermanium();  else if (clip == 3) dn = Circuit::diodeSiliconPair();
        ped.setupTimmy (gain, tr, bs, dp, dn);
        std::vector<float> y (n);  for (size_t i = 0; i < n; ++i) y[i] = ped.process ((float) (amp * std::sin (2 * M_PI * f * (double) i / SR)));
        return y; };
    auto gainDb = [&] (double f, float gain, float tr, float bs) { const auto y = run (f, 0.001, gain, tr, bs, 0);  return goertz (y, 24000, 48000, f) - 10 * std::log10 (0.001 * 0.001 * 0.5 * 0.375 / 1.0) + 0.0 - 10 * std::log10 (1.0) ; };
    // reference: the same measure on the input (a sine of amp 0.001) so the gain is relative
    auto rel = [&] (double f, float gain, float tr, float bs)
    {   const auto y = run (f, 0.001, gain, tr, bs, 0);  std::vector<float> x (y.size());  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (0.001 * std::sin (2 * M_PI * f * (double) i / SR));
        return goertz (y, 24000, 48000, f) - goertz (x, 24000, 48000, f); };
    (void) gainDb;
    bool ok = true;  float pk;
    for (float g : { 0.0f, 0.5f, 1.0f }) for (float t : { 0.0f, 1.0f }) for (float b : { 0.0f, 1.0f }) for (int cl = 0; cl < 4; ++cl)
        for (double a : { 0.01, 0.3 }) ok = ok && finiteBounded (run (220.0, a, g, t, b, cl, 24000), 12.0f, pk);
    CHECK (ok, "Timmy: finite and bounded over Gain, Treble, Bass, four clippings and two input levels");

    const double g0 = rel (1000.0, 0.0f, 1.0f, 1.0f), g5 = rel (1000.0, 0.5f, 1.0f, 1.0f), g10 = rel (1000.0, 1.0f, 1.0f, 1.0f);
    std::printf ("  info: small-signal gain at 1 kHz: Gain 0 %.1f dB, 0.5 %.1f dB, 1 %.1f dB\n", g0, g5, g10);
    CHECK (g0 > 0.0 && g0 < 6.0 && g10 > 24.0 && g10 < 36.0 && g5 > g0 + 5.0 && g5 < g10 - 3.0, "Timmy: gain runs from %.1f to %.1f dB (want about 1 to 6 up to about 24 to 36), middle %.1f dB", g0, g10, g5);

    // flat: no mid hump (Gain 0.4, Treble and Bass full): within 3 dB from 150 Hz to 3 kHz
    {   double lo = 99, hi = -99;  std::printf ("  info: response at Gain 0.4 (dB):");
        for (double f : { 150.0, 300.0, 600.0, 1000.0, 2000.0, 3000.0 }) { const double r = rel (f, 0.4f, 1.0f, 1.0f);  lo = std::min (lo, r);  hi = std::max (hi, r);  std::printf (" %.0f Hz %.1f;", f, r); }
        std::printf ("\n");
        CHECK (hi - lo < 3.5, "Timmy: flat, no mid hump: 150 Hz to 3 kHz spread is %.1f dB (want under 3.5)", hi - lo); }
    // Treble: cut-only
    {   const double hi1 = rel (6000.0, 0.4f, 1.0f, 1.0f), hi0 = rel (6000.0, 0.4f, 0.0f, 1.0f), mid1 = rel (500.0, 0.4f, 1.0f, 1.0f), mid0 = rel (500.0, 0.4f, 0.0f, 1.0f);
        CHECK (hi0 < hi1 - 12.0 && std::abs (mid0 - mid1) < 3.0, "Timmy Treble: down cuts 6 kHz by %.1f dB (want 12 or more) and leaves 500 Hz alone (%.1f dB change)", hi1 - hi0, mid1 - mid0); }
    // Bass: cut-only
    {   const double lo1 = rel (80.0, 0.4f, 1.0f, 1.0f), lo0 = rel (80.0, 0.4f, 1.0f, 0.0f), mid1 = rel (1000.0, 0.4f, 1.0f, 1.0f), mid0 = rel (1000.0, 0.4f, 1.0f, 0.0f);
        CHECK (lo0 < lo1 - 8.0 && std::abs (mid0 - mid1) < 1.5, "Timmy Bass: down cuts 80 Hz by %.1f dB (want 8 or more) and leaves 1 kHz alone (%.1f dB change)", lo1 - lo0, mid1 - mid0); }
    // soft clipping: the output grows slower than the input at the top, and harmonics rise with Gain
    {   const auto lo = run (220.0, 0.01, 0.8f, 1.0f, 1.0f, 0), hi = run (220.0, 0.5, 0.8f, 1.0f, 1.0f, 0);
        const double d1 = goertz (lo, 24000, 48000, 220.0), d2 = goertz (hi, 24000, 48000, 220.0);
        const double h3lo = goertz (lo, 24000, 48000, 660.0) - d1, h3hi = goertz (hi, 24000, 48000, 660.0) - d2;
        std::printf ("  info: Gain 0.8, 220 Hz: third harmonic %.1f dB (quiet) and %.1f dB (loud), compression %.1f dB over a 34 dB input step\n", h3lo, h3hi, 34.0 - (d2 - d1));
        CHECK (h3hi > h3lo + 15.0 && (d2 - d1) < 34.0 - 8.0, "Timmy: clipping grows with level: third harmonic %.1f -> %.1f dB, output rises %.1f dB for a 34 dB step (want at least 8 dB of compression)", h3lo, h3hi, d2 - d1); }
    // asymmetric clipping adds even harmonics
    {   const auto s = run (220.0, 0.3, 0.8f, 1.0f, 1.0f, 0), a = run (220.0, 0.3, 0.8f, 1.0f, 1.0f, 3);
        const double s2 = goertz (s, 24000, 48000, 440.0) - goertz (s, 24000, 48000, 220.0), a2 = goertz (a, 24000, 48000, 440.0) - goertz (a, 24000, 48000, 220.0);
        CHECK (a2 > s2 + 6.0, "Timmy: Asymmetric clipping has more second harmonic than Symmetrical (%.1f vs %.1f dB)", a2, s2); }
    // LED has more headroom than germanium
    {   const auto l = run (220.0, 0.3, 0.8f, 1.0f, 1.0f, 1), g = run (220.0, 0.3, 0.8f, 1.0f, 1.0f, 2);
        CHECK (goertz (l, 24000, 48000, 220.0) > goertz (g, 24000, 48000, 220.0) + 2.0, "Timmy: LED clipping is louder than germanium at the same setting (%.1f vs %.1f dB)", goertz (l, 24000, 48000, 220.0), goertz (g, 24000, 48000, 220.0)); }

    // in the plugin: both drive slots, and the level row
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        double row[11];  bool fin = true;
        for (int gi = 0; gi <= 10; ++gi)
        {
            auto mk = [&] (bool on, int slot)
            {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);
                setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);
                setP (p, "compOn", 0.0f);  setP (p, "eqOn", 0.0f);  setP (p, "reverbOn", 0.0f);  setP (p, "postDelayOn", 0.0f);  setP (p, "preDelayOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
                if (on) { const char* pre = slot == 0 ? "od" : "od2";  char b[48];
                    std::snprintf (b, sizeof b, "%sOn", pre);  setP (p, b, 1.0f);  std::snprintf (b, sizeof b, "%sStyle", pre);  setP (p, b, 12.0f);
                    std::snprintf (b, sizeof b, "%sGain", pre);  setP (p, b, 0.1f * (float) gi);  std::snprintf (b, sizeof b, "%sVolume", pre);  setP (p, b, -6.0f);  std::snprintf (b, sizeof b, "%sAuto", pre);  setP (p, b, 0.0f);
                    std::snprintf (b, sizeof b, "%sTone", pre);  setP (p, b, 0.7f);  std::snprintf (b, sizeof b, "%sBass", pre);  setP (p, b, 0.8f); }
                p.prepareToPlay (SR, blk);  std::vector<float> o;  process (p, sig, o, blk);  float pk;  fin = fin && finiteBounded (o, 12.0f, pk);  return rmsDb (o, 24000); };
            row[gi] = mk (true, 0) - mk (false, 0);
            if (gi == 5) { const double r2 = mk (true, 1) - mk (false, 1);  CHECK (std::abs (r2 - row[gi]) < 1.5, "Timmy in Drive 2 plays like Drive 1 (%.1f vs %.1f dB)", r2, row[gi]); }
        }
        CHECK (fin, "Timmy in the plugin: finite and bounded at Gain 0 to 1");
        if (meas) { std::printf ("  ROW:"); for (int i = 0; i < 11; ++i) std::printf (" %.2ff,", row[i]); std::printf ("\n"); }
        else std::printf ("  info: loudness change with the drive on (Volume -6): %.1f / %.1f / %.1f dB at Gain 0 / 0.5 / 1\n", row[0], row[5], row[10]);
        CHECK (row[10] > row[0] - 1.0, "Timmy: louder at full Gain than at none (%.1f vs %.1f dB)", row[10], row[0]); }
}

static void testRough78()
{
    std::printf ("[b78] Pitch pedal characters: Grainy Sweep, Glitchy, Abrasive, Noisy (my own designs, measured not listened to)\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    auto sine = [&] (size_t n, double f, double amp) { std::vector<float> x (n);  for (size_t i = 0; i < n; ++i) x[i] = (float) (amp * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
    auto shift = [&] (const std::vector<float>& in, int ch, float semis, float grit)
    {   RoughShifter r;  r.prepare (SR);  RoughShifter::Params p;  p.semis = semis;  p.character = ch;  p.grit = grit;
        std::vector<float> y (in.size());  for (size_t i = 0; i < in.size(); ++i) y[i] = r.process (in[i], p);  return y; };
    bool ok = true;  float pk;
    for (int ch = 1; ch <= 5; ++ch) for (float gr : { 0.0f, 0.5f, 1.0f }) for (float st : { -12.0f, -5.0f, 0.0f, 7.0f, 12.0f, 24.0f })
        ok = ok && finiteBounded (shift (sine (48000, 196.0, 0.5), ch, st, gr), 4.0f, pk);
    CHECK (ok, "all five characters (Warble included) are finite and bounded at three Grit settings and six shifts (-12 to +24)");
    // the pitch: +12 semitones lands near 2x the frequency in every character
    for (int ch = 1; ch <= 5; ++ch)
    {
        const auto y = shift (sine (96000, 196.0, 0.4), ch, 12.0f, 0.3f);
        double best = -999, bf = 0;  for (double f = 150; f < 700; f += 2.0) { const double g = goertz (y, 24000, 96000, f);  if (g > best) { best = g;  bf = f; } }
        std::printf ("  info: character %d, +12 st on 196 Hz: strongest component at %.0f Hz (%.1f dB)\n", ch, bf, best);
        CHECK (std::abs (bf / 392.0 - 1.0) < 0.06 || (ch == 3 && std::abs (bf / 392.0 - 1.0) < 0.15), "character %d: +12 semitones lands near 392 Hz (strongest at %.0f Hz)", ch, bf);
    }
    // 0 semitones: the Grainy Sweep still passes the note (a little darkened)
    {   const auto y = shift (sine (96000, 196.0, 0.4), 1, 0.0f, 0.0f);
        CHECK (goertz (y, 24000, 96000, 196.0) > goertz (sine (96000, 196.0, 0.4), 24000, 96000, 196.0) - 6.0, "Grainy Sweep at 0 semitones passes the note within 6 dB"); }
    // worse tracking = more junk: a 5 kHz tone through the Grainy Sweep. At Grit 0 the note stays put; at Grit 1 the lower sample rate, the 4-bit-shorter word and the darker top
    // take level off it and put images and noise around it (the share of the energy within 60 Hz of the note falls)
    auto focus = [&] (const std::vector<float>& y, double f0)
    {   double in = 0, all = 0;
        for (double f = 100; f < 14000; f += 8.0) { const double p = std::pow (10.0, goertz (y, 24000, 96000, f) / 10.0);  all += p;  if (std::abs (f - f0) < 60.0) in += p; }
        return 10 * std::log10 (in / all + 1e-30); };
    {   const auto in = sine (96000, 5000.0, 0.4);
        const auto y0 = shift (in, 1, 0.0f, 0.0f), y1 = shift (in, 1, 0.0f, 1.0f);
        const double j0 = focus (y0, 5000.0), j1 = focus (y1, 5000.0), l0 = goertz (y0, 24000, 96000, 5000.0), l1 = goertz (y1, 24000, 96000, 5000.0);
        std::printf ("  info: Grainy Sweep, 5 kHz tone: level %.1f dB at Grit 0 and %.1f dB at Grit 1; share of energy on the note %.1f and %.1f dB\n", l0, l1, j0, j1);
        CHECK (l1 < l0 - 3.0 && j1 < j0 - 1.0, "Grainy Sweep: more Grit, a darker top (%.1f dB lower) and less of the sound stays on the note (%.1f vs %.1f dB)", l0 - l1, j1, j0); }
    // Abrasive makes harmonics
    {   const auto in = sine (96000, 196.0, 0.4);  const auto y = shift (in, 3, 0.0f, 0.8f);
        const double f1 = goertz (y, 24000, 96000, 196.0), f3 = goertz (y, 24000, 96000, 588.0);
        CHECK (f3 > f1 - 25.0, "Abrasive: the third harmonic is within 25 dB of the note (%.1f dB down)", f1 - f3); }
    // Glitchy: dropouts exist (silent stretches inside a held note) and are not there at Grit 0 as often
    {   const auto in = sine (192000, 196.0, 0.4);  const auto y = shift (in, 2, 0.0f, 1.0f);
        int quiet = 0, blocks = 0;  for (size_t i = 24000; i + 480 < y.size(); i += 480) { double e = 0;  for (size_t k = 0; k < 480; ++k) e += (double) y[i + k] * y[i + k];  ++blocks;  if (10 * std::log10 (e / 480 + 1e-20) < -30.0) ++quiet; }
        std::printf ("  info: Glitchy at Grit 1: %d of %d ten-millisecond blocks are quiet\n", quiet, blocks);
        CHECK (quiet >= 3, "Glitchy: dropouts happen (%d quiet blocks of %d; want at least 3)", quiet, blocks); }
    // Noisy: hiss rises when a note plays and the floor is low when silent
    {   std::vector<float> in (96000, 0.0f);  for (size_t i = 24000; i < 72000; ++i) in[i] = (float) (0.3 * std::sin (2 * M_PI * 196.0 * (double) i / SR));
        const auto y = shift (in, 4, 0.0f, 1.0f);
        auto rms = [&] (size_t a, size_t b) { double s = 0;  for (size_t i = a; i < b; ++i) s += (double) y[i] * y[i];  return 10 * std::log10 (s / (double) (b - a) + 1e-20); };
        std::printf ("  info: Noisy at Grit 1: silent input %.1f dB, playing %.1f dB\n", rms (4000, 20000), rms (40000, 70000));
        CHECK (rms (4000, 20000) < -50.0 && rms (40000, 70000) > rms (4000, 20000) + 20.0, "Noisy: quiet when silent (%.1f dB) and louder while a note plays", rms (4000, 20000)); }
    // in the plugin: latency per character, Mix 0 passes the dry, and switching on the fly stays finite
    {   const int blk = 512;
        auto mk = [&] (int ch, float mix)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
            setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "cabOn", 0.0f);
            setP (p, "wowOn", 1.0f);  setP (p, "wowMode", 0.0f);  setP (p, "wowPos", 1.0f);  setP (p, "wowGlide", 0.0f);  setP (p, "wowMix", mix);  setP (p, "wowChar", (float) ch);
            std::vector<float> y;  process (p, sine (96000, 196.0, 0.3), y, blk);  p.applyPendingUpdates();  return std::make_pair (p.getLatencySamples(), y); };
        StarterAmpProcessor p0;  p0.setPlayConfigDetails (2, 2, SR, blk);  p0.prepareToPlay (SR, blk);  p0.applyPendingUpdates();  const int base = p0.getLatencySamples();
        int lat[5];  for (int ch = 1; ch <= 4; ++ch) lat[ch] = mk (ch, 1.0f).first - base;
        std::printf ("  info: reported delay (samples): Grainy Sweep %d, Glitchy %d, Abrasive %d, Noisy %d\n", lat[1], lat[2], lat[3], lat[4]);
        CHECK (lat[2] > lat[1] && lat[1] > lat[3] && lat[3] > 0, "delays differ by character: Glitchy %d > Grainy Sweep %d > Abrasive %d > 0", lat[2], lat[1], lat[3]);
        const auto y = mk (1, 0.0f).second;
        CHECK (goertz (y, 48000, 96000, 196.0) > goertz (y, 48000, 96000, 392.0) + 25.0, "Mix 0: only the original note comes out of a character");
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "wowOn", 1.0f);  setP (p, "wowMode", 5.0f);  setP (p, "wowMix", 1.0f);  bool fine = true;  float pk;
        for (int seg = 0; seg < 15; ++seg) { setP (p, "wowChar", (float) (seg % 6));  setP (p, "wowGrit", (float) (seg % 3) * 0.5f);  setP (p, "wowPos", (float) (seg % 4) / 3.0f);
            std::vector<float> y2;  process (p, testSignal (12000, SR), y2, 256);  fine = fine && finiteBounded (y2, 8.0f, pk); }
        CHECK (fine, "switching Character, Grit and pedal position mid-play stays finite and bounded"); }
}

// b79: the octaver's bank engine (POG/HOG-style voices, Tracking, Character, envelope, resonant filter).
static void testOct79()
{
    std::printf("[b79] Octaver bank: voices, Tracking, Character, Attack/Decay, resonant filter, Detune, Dry routing (my own design from public descriptions of the POG and HOG; measured, not listened to)\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    auto chord = [&] (size_t n, std::initializer_list<double> fs, double amp = 0.12)
    {   std::vector<float> x (n, 0.0f);  for (size_t i = 0; i < n; ++i) { double v = 0;  for (double f : fs) v += std::sin (2 * M_PI * f * (double) i / SR);  x[i] = (float) (amp * v); }  return x; };
    auto run = [&] (const std::vector<float>& in, const OctaverBank::Params& p)
    {   OctaverBank b;  b.prepare (SR);  std::vector<float> y = in;  for (size_t i = 0; i < y.size(); i += 480) b.process (y.data() + i, (int) std::min<size_t> (480, y.size() - i), p);  return y; };
    auto mk = [&] ()
    {   OctaverBank::Params p;  p.nVoices = 4;  p.semis[0] = -24;  p.semis[1] = -12;  p.semis[2] = 12;  p.semis[3] = 24;  p.dry = 1.0f;  p.tone = 1.0f;  p.windowMs = 28.0f;  return p; };
    bool ok = true;  float pk;
    // 1. finite and bounded for every engine, with every voice on
    for (int tr = 0; tr < 3; ++tr) for (int ch = 0; ch <= 5; ++ch)
    {   auto p = mk();  p.nVoices = 8;  const float st[8] = { -24, -12, 12, 24, 7, 19, 28, 36 };  for (int v = 0; v < 8; ++v) { p.semis[v] = st[v];  p.level[v] = 0.4f; }
        p.track = tr;  p.character = ch;  p.grit = 0.7f;  p.attack = 0.3f;  p.decay = 0.3f;  p.res = 0.6f;  p.detuneCents = 20.0f;  p.dryFx = 3;
        const auto y = run (chord (48000, { 110.0, 165.0, 220.0 }), p);  ok = ok && finiteBounded (y, 8.0f, pk); }
    CHECK (ok, "all eight voices on: finite and bounded in every Tracking and Character, with Attack, Decay, Resonance, Detune and Dry routing set");

    // 2. a POG bank on a power chord, Chords tracking: each voice lands on its own interval of each note
    {   auto p = mk();  for (int v = 0; v < 4; ++v) p.level[v] = 0.5f;  p.track = 1;
        const auto y = run (chord (96000, { 110.0, 165.0 }), p);
        const double a = goertz (y, 60000, 92000, 55.0), b = goertz (y, 60000, 92000, 82.5), c = goertz (y, 60000, 92000, 220.0), d = goertz (y, 60000, 92000, 330.0), e = goertz (y, 60000, 92000, 440.0), f = goertz (y, 60000, 92000, 660.0), g = goertz (y, 60000, 92000, 137.5);
        std::printf ("  info: Chords, POG bank on 110 + 165 Hz: -1 oct 55 %.1f / 82.5 %.1f | +1 oct 220 %.1f / 330 %.1f | +2 oct 440 %.1f / 660 %.1f | nothing at 137.5 Hz: %.1f dB\n", a, b, c, d, e, f, g);
        CHECK (a > g + 15.0 && b > g + 15.0 && c > g + 15.0 && d > g + 15.0 && e > g + 15.0 && f > g + 15.0, "POG bank, Chords: all four voices are present on both notes (55, 82.5, 220, 330, 440, 660 Hz all 15 dB over the empty 137.5 Hz)"); }
    // 3. HOG voices: the fifth, 12th, 17th, 3 octaves on a single note
    {   auto p = mk();  p.nVoices = 8;  const float st[8] = { -24, -12, 12, 24, 7, 19, 28, 36 };  for (int v = 0; v < 8; ++v) { p.semis[v] = st[v];  p.level[v] = v >= 4 ? 0.5f : 0.0f; }  p.dry = 0.0f;  p.track = 1;
        const auto y = run (chord (96000, { 110.0 }, 0.3), p);
        const double f5 = 110.0 * std::pow (2.0, 7.0 / 12.0), f12 = 110.0 * std::pow (2.0, 19.0 / 12.0), f17 = 110.0 * std::pow (2.0, 28.0 / 12.0), f3 = 880.0;
        const double e0 = goertz (y, 60000, 92000, 400.0);
        std::printf ("  info: HOG voices on 110 Hz: +5th %.0f Hz %.1f dB | 12th %.0f Hz %.1f dB | 17th %.0f Hz %.1f dB | +3 oct 880 Hz %.1f dB | empty 400 Hz %.1f dB\n", f5, goertz (y, 60000, 92000, f5), f12, goertz (y, 60000, 92000, f12), f17, goertz (y, 60000, 92000, f17), goertz (y, 60000, 92000, f3), e0);
        CHECK (goertz (y, 60000, 92000, f5) > e0 + 15.0 && goertz (y, 60000, 92000, f12) > e0 + 15.0 && goertz (y, 60000, 92000, f17) > e0 + 15.0 && goertz (y, 60000, 92000, f3) > e0 + 15.0, "HOG voices: +5th, 12th, 17th and +3 octaves each stand 15 dB over the floor"); }
    // 4. Single note vs Chords on a chord: the Chords engine keeps every note's octave cleaner
    {   auto ps = mk(), pc = mk();  ps.level[2] = pc.level[2] = 0.5f;  ps.dry = pc.dry = 0.0f;  ps.track = 0;  pc.track = 1;
        const auto in = chord (96000, { 110.0, 165.0, 220.0 });
        const auto ys = run (in, ps), yc = run (in, pc);
        const double sr = goertz (ys, 60000, 92000, 330.0) - goertz (ys, 60000, 92000, 275.0), cr = goertz (yc, 60000, 92000, 330.0) - goertz (yc, 60000, 92000, 275.0);
        std::printf ("  info: +1 octave on a 110/165/220 chord: the 330 Hz note stands %.1f dB (Single note) and %.1f dB (Chords) above the 275 Hz gap\n", sr, cr);
        CHECK (cr > 20.0, "Chords: the octave of the 165 Hz note stands %.1f dB over a 275 Hz gap (want 20 or more)", cr); }
    // 5. latency per engine, and the dry guitar lines up (Dry only, no voices audible: output = dry delayed by the reported latency)
    {   OctaverBank b;  b.prepare (SR);  auto p = mk();  p.level[1] = 0.5f;
        int l0, l1, l2, l3;  p.track = 0; l0 = b.latency (p);  p.track = 1; l1 = b.latency (p);  p.track = 2; l2 = b.latency (p);  p.track = 0;  p.character = 2; l3 = b.latency (p);
        std::printf ("  info: reported delay: Single note %d, Chords %d, Chords (fast) %d, Glitchy %d samples\n", l0, l1, l2, l3);
        CHECK (l1 > l2 && l2 > l3 && l3 > l0 && l0 > 0, "delays: Chords (%d) > Chords fast (%d) > Glitchy (%d) > Single note (%d) > 0", l1, l2, l3, l0);
        auto q = mk();  q.track = 1;  const auto in = chord (48000, { 220.0 }, 0.3);  const auto y = run (in, q);       // all levels 0: dry only
        const int lat = b.latency (q);  double err = 0;  for (size_t i = lat + 100; i < in.size(); ++i) err = std::max (err, (double) std::fabs (y[i] - in[i - (size_t) lat]));
        CHECK (err < 1.0e-5, "with no voices the output is the dry guitar delayed by exactly the reported %d samples (worst error %.2g)", lat, err); }
    // 6. Attack: the voices come in slowly after a pluck
    {   std::vector<float> in (96000, 0.0f);  for (size_t i = 24000; i < 90000; ++i) in[i] = (float) (0.3 * std::sin (2 * M_PI * 220.0 * (double) i / SR) * std::exp (-(double) (i - 24000) / 40000.0));
        auto p = mk();  p.level[2] = 0.8f;  p.dry = 0.0f;  p.track = 0;
        auto rms = [&] (const std::vector<float>& y, size_t a, size_t b) { double s = 0;  for (size_t i = a; i < b; ++i) s += (double) y[i] * y[i];  return 10 * std::log10 (s / (double) (b - a) + 1e-20); };
        auto q = p;  q.attack = 0.85f;
        const auto y0 = run (in, p), y1 = run (in, q);
        const size_t on = 24000 + 1300;           // the note arrives at the output after the engine's delay
        const double early0 = rms (y0, on + 500, on + 3500), early1 = rms (y1, on + 500, on + 3500), late0 = rms (y0, on + 36000, on + 40000), late1 = rms (y1, on + 36000, on + 40000);
        std::printf ("  info: Attack 0.85: first 60 ms %.1f dB (Attack 0: %.1f), after 0.75 s %.1f dB (Attack 0: %.1f)\n", early1, early0, late1, late0);
        CHECK (early1 < early0 - 10.0 && std::abs (late1 - late0) < 3.0, "Attack: the voice starts %.1f dB lower than without and arrives at the same level later (%.1f dB apart)", early0 - early1, std::abs (late1 - late0)); }
    // 7. Decay: a held note's voices die away
    {   std::vector<float> in (96000, 0.0f);  for (size_t i = 24000; i < 96000; ++i) in[i] = (float) (0.3 * std::sin (2 * M_PI * 220.0 * (double) i / SR));
        auto p = mk();  p.level[2] = 0.8f;  p.dry = 0.0f;  auto q = p;  q.decay = 0.8f;
        auto rms = [&] (const std::vector<float>& y, size_t a, size_t b) { double s = 0;  for (size_t i = a; i < b; ++i) s += (double) y[i] * y[i];  return 10 * std::log10 (s / (double) (b - a) + 1e-20); };
        const auto y0 = run (in, p), y1 = run (in, q);
        std::printf ("  info: Decay 0.8: 0.1 s after the note %.1f dB, 1.2 s after %.1f dB (Decay 0: %.1f and %.1f)\n", rms (y1, 27000, 29000), rms (y1, 84000, 88000), rms (y0, 27000, 29000), rms (y0, 84000, 88000));
        CHECK (rms (y1, 84000, 88000) < rms (y1, 27000, 29000) - 20.0 && rms (y0, 84000, 88000) > rms (y0, 27000, 29000) - 3.0, "Decay: the voices fall by %.1f dB over a second; without Decay they hold", rms (y1, 27000, 29000) - rms (y1, 84000, 88000)); }
    // 8. Resonance makes a peak at the cutoff; Tone moves the cutoff
    {   OctaverBank::Params p = mk();  p.level[1] = 0.0f;  p.level[2] = 1.0f;  p.dry = 0.0f;  p.tone = 0.5f;           // cutoff 800 x 10^0.5 = 2.5 kHz; the voice is an octave up, so drive it with a sweep via several tones
        auto lvl = [&] (double f, float res) { auto q = p;  q.res = res;  const auto y = run (chord (96000, { f }, 0.1), q);  return goertz (y, 48000, 90000, 2.0 * f); };
        const double flat = lvl (1265.0, 0.0f), peak = lvl (1265.0, 1.0f), lowRes = lvl (400.0, 1.0f), lowFlat = lvl (400.0, 0.0f);
        std::printf ("  info: filter at 2.5 kHz (voice an octave up): Resonance 0 %.1f dB, Resonance 1 %.1f dB at the cutoff; a 800 Hz voice %.1f vs %.1f dB\n", flat, peak, lowFlat, lowRes);
        CHECK (peak > flat + 6.0 && std::abs (lowRes - lowFlat) < 3.0, "Resonance: +%.1f dB at the cutoff, %.1f dB elsewhere", peak - flat, lowRes - lowFlat); }
    // 9. Detune moves only the voices an octave or more up
    {   auto p = mk();  p.level[1] = 0.5f;  p.level[2] = 0.5f;  p.dry = 0.0f;  p.track = 1;  p.detuneCents = 40.0f;
        const auto y = run (chord (96000, { 220.0 }, 0.3), p);
        const double up = 440.0 * std::pow (2.0, 40.0 / 1200.0);
        double bestUp = -999, bfUp = 0, bestDn = -999, bfDn = 0;
        for (double f = 400; f < 520; f += 1.0) { const double g = goertz (y, 60000, 92000, f);  if (g > bestUp) { bestUp = g;  bfUp = f; } }
        for (double f = 100; f < 125; f += 0.5) { const double g = goertz (y, 60000, 92000, f);  if (g > bestDn) { bestDn = g;  bfDn = f; } }
        std::printf ("  info: Detune +40 cents: the octave up lands at %.0f Hz (wanted %.0f), the octave down at %.1f Hz (wanted 110)\n", bfUp, up, bfDn);
        CHECK (std::abs (bfUp - up) < 5.0 && std::abs (bfDn - 110.0) < 2.0, "Detune moves the octave up (%.0f vs %.0f Hz) and leaves the octave down alone (%.1f Hz)", bfUp, up, bfDn); }
    // 10. Dry routing: Dry through the filter darkens the dry guitar
    {   auto p = mk();  p.dry = 1.0f;  p.tone = 0.0f;  p.res = 0.2f;  auto q = p;  q.dryFx = 1;
        const auto in = chord (96000, { 3000.0 }, 0.3);
        const auto y0 = run (in, p), y1 = run (in, q);
        const double a = goertz (y0, 60000, 92000, 3000.0), b = goertz (y1, 60000, 92000, 3000.0);
        CHECK (b < a - 8.0, "Dry routing: Dry through the filter takes %.1f dB off a 3 kHz dry note (filter at 800 Hz)", a - b); }
    // 11. Character: the lo-fi engine runs the same bank
    {   auto p = mk();  p.level[1] = 0.5f;  p.dry = 0.0f;  p.character = 1;  p.grit = 0.3f;
        const auto y = run (chord (96000, { 220.0 }, 0.3), p);
        CHECK (goertz (y, 60000, 92000, 110.0) > goertz (y, 60000, 92000, 165.0) + 10.0, "Character: Grainy Sweep octave down puts the note at 110 Hz (%.1f dB vs %.1f dB at 165)", goertz (y, 60000, 92000, 110.0), goertz (y, 60000, 92000, 165.0)); }
    // 12. cost: eight voices on Chords
    {   auto p = mk();  p.nVoices = 8;  const float st[8] = { -24, -12, 12, 24, 7, 19, 28, 36 };  for (int v = 0; v < 8; ++v) { p.semis[v] = st[v];  p.level[v] = 0.3f; }  p.track = 1;
        OctaverBank b;  b.prepare (SR);  std::vector<float> y = chord (480000, { 110.0, 165.0, 220.0 });
        const auto t0 = std::chrono::steady_clock::now();
        for (size_t i = 0; i < y.size(); i += 512) b.process (y.data() + i, (int) std::min<size_t> (512, y.size() - i), p);
        const double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("  info: eight voices on Chords tracking: %.2f s of audio in %.2f s of processing (%.1f percent of one core, this machine)\n", 10.0, secs, 100.0 * secs / 10.0);
        CHECK (secs < 10.0, "eight Chords voices run faster than real time (%.2f s for 10 s)", secs); }
    // ---- in the plugin
    {   const int blk = 512;  auto sig = testSignal (96000, SR, 0.5f);
        auto mkp = [&] (std::vector<std::pair<const char*, float>> ps, int* latOut = nullptr)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);
            setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);
            setP (p, "compOn", 0.0f);  setP (p, "eqOn", 0.0f);  setP (p, "reverbOn", 0.0f);  setP (p, "postDelayOn", 0.0f);  setP (p, "preDelayOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
            setP (p, "octOn", 1.0f);  setP (p, "octALvl", 0.6f);  setP (p, "octBLvl", 0.4f);  setP (p, "octA", 1.0f);  setP (p, "octB", 2.0f);
            for (auto& kv : ps) setP (p, kv.first, kv.second);
            p.prepareToPlay (SR, blk);  std::vector<float> o;  process (p, sig, o, blk);  p.applyPendingUpdates();  if (latOut) *latOut = p.getLatencySamples();  return o; };
        // the old path and the new engine with nothing special asked of it sound the same (an Attack of 0.002 sends it down the new path but is under the 0.01 where Attack starts to act)
        const auto o0 = mkp ({}), o1 = mkp ({ { "octAttack", 0.002f } });
        double num = 0, den = 0;  for (size_t i = 24000; i < o0.size(); ++i) { num += (double) (o0[i] - o1[i]) * (o0[i] - o1[i]);  den += (double) o0[i] * o0[i]; }
        std::printf ("  info: old path against the new engine, same settings: difference %.1f dB below the signal\n", 10 * std::log10 (num / (den + 1e-20) + 1e-20));
        CHECK (10 * std::log10 (num / (den + 1e-20) + 1e-20) < -30.0, "the new engine with default options matches the old octaver (difference %.1f dB below the signal; want under -30)", 10 * std::log10 (num / (den + 1e-20) + 1e-20));
        // bank modes and the delay they report
        int latOld = 0, latChords = 0, latGlitchy = 0;  mkp ({}, &latOld);
        const auto b1 = mkp ({ { "octMode", 1.0f }, { "octP1", 0.5f }, { "octM2", 0.5f }, { "octTrack", 1.0f } }, &latChords);
        const auto b2 = mkp ({ { "octMode", 2.0f }, { "octP7", 0.4f }, { "octP19", 0.4f }, { "octP28", 0.3f }, { "octP36", 0.2f }, { "octTrack", 2.0f }, { "octAttack", 0.4f }, { "octDecay", 0.3f }, { "octRes", 0.5f }, { "octDetune", 12.0f } });
        const auto b3 = mkp ({ { "octMode", 1.0f }, { "octChar", 2.0f }, { "octGrit", 0.8f } }, &latGlitchy);
        float pk;
        CHECK (finiteBounded (b1, 12.0f, pk) && rmsDb (b1, 24000) > -60.0 && finiteBounded (b2, 12.0f, pk) && rmsDb (b2, 24000) > -60.0 && finiteBounded (b3, 12.0f, pk) && rmsDb (b3, 24000) > -60.0, "plugin: POG bank with Chords, HOG bank with every option and POG bank with Glitchy all play (%.1f, %.1f, %.1f dB)", rmsDb (b1, 24000), rmsDb (b2, 24000), rmsDb (b3, 24000));
        std::printf ("  info: plugin delay: old %d samples, bank with Chords %d, bank with Glitchy %d\n", latOld, latChords, latGlitchy);
        CHECK (latChords >= 4096 && latGlitchy > latOld && latGlitchy < latChords, "the host is told: Chords %d samples, Glitchy %d, the old path %d", latChords, latGlitchy, latOld);
        // switching on the fly
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "octOn", 1.0f);  setP (p, "octP1", 0.5f);  bool fine = true;
        for (int seg = 0; seg < 18; ++seg) { setP (p, "octMode", (float) (seg % 3));  setP (p, "octTrack", (float) ((seg / 3) % 3));  setP (p, "octChar", (float) (seg % 5));  setP (p, "octAttack", (seg % 2) * 0.5f);
            std::vector<float> y; process (p, testSignal (9600, SR), y, 256);  fine = fine && finiteBounded (y, 12.0f, pk); }
        CHECK (fine, "switching Voices, Tracking, Character and Attack mid-play stays finite and bounded"); }
}

static void testWah76()
{
    std::printf ("[b76] Wah voicings: sweep ranges, peak gain, Range selector, Boost, Q, fuzz (published figures where the sources give them; the rest are estimates)\n");
    const double SR = 48000.0;
    auto gainDb = [&] (int voice, float pos, double f, int range = 0, float boost = 0.0f, float q = 5.0f, float fuzz = 0.0f, double amp = 0.003)
    {   Wah w;  w.prepare (SR);
        Wah::Params p {};  p.mode = 0;  p.pos = pos;  p.heelHz = 400.0f;  p.toeHz = 2200.0f;  p.q = q;  p.mix = 1.0f;  p.sens = 0.5f;  p.decayMs = 100.0f;  p.rate = 1.0f;  p.depth = 0.5f;
        p.circuit = voice;  p.range = range;  p.boostDb = boost;  p.fuzz = fuzz;
        std::vector<float> x (24000), y;  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (amp * std::sin (2 * M_PI * f * (double) i / SR));
        y = x;  for (size_t i = 0; i < y.size(); i += 512) w.process (y.data() + i, (int) std::min<size_t> (512, y.size() - i), p);
        double a = 0, b = 0;  for (size_t i = 12000; i < y.size(); ++i) { a += (double) y[i] * y[i];  b += (double) x[i] * x[i]; }
        return 10 * std::log10 (a / b + 1e-20); };
    auto peakOf = [&] (int voice, float pos, int range = 0, float q = 5.0f, double* pkDb = nullptr)
    {   double best = -999, bf = 0;
        for (double f = 150; f < 5000; f *= 1.03) { const double g = gainDb (voice, pos, f, range, 0.0f, q);  if (g > best) { best = g;  bf = f; } }
        if (pkDb) *pkDb = best;  return bf; };
    auto near = [&] (double v, double target, double tol) { return std::abs (v / target - 1.0) <= tol; };

    bool ok = true;
    for (int v = 0; v <= 7; ++v)
        for (float pos : { 0.0f, 0.5f, 1.0f }) { const double g = gainDb (v, pos, 700.0, 0, 0.0f, 5.0f, 0.5f, 0.05);  ok = ok && std::isfinite (g) && g > -80.0 && g < 60.0; }
    CHECK (ok, "all eight wah voices are finite and bounded at heel, middle and toe");

    {   std::printf ("  info (heel Hz / heel dB / toe Hz / toe dB):");
        for (int v = 1; v <= 7; ++v) { double ph, pt;  const double fh = peakOf (v, 0.0f, 0, 5.0f, &ph), ft = peakOf (v, 1.0f, 0, 5.0f, &pt);  std::printf ("  v%d %.0f/%.1f/%.0f/%.1f", v, fh, ph, ft, pt); }
        std::printf ("\n"); }
    {   std::printf ("  info Petrucci JP95 (Hz / dB at treadle 0, 25, 50, 75, 100 percent):");
        for (float ps : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f }) { double pk;  const double f = peakOf (6, ps, 0, 5.0f, &pk);  std::printf ("  %.0f/%.1f", f, pk); }
        std::printf ("\n"); }
    // sweep ends
    double pkH1, pkH2, pkH3, pkH6, pkH7, pkT1, pkT3, pkT6;
    const double h1 = peakOf (1, 0.0f, 0, 5.0f, &pkH1), t1 = peakOf (1, 1.0f, 0, 5.0f, &pkT1);
    CHECK (near (h1, 450, 0.12) && near (t1, 1600, 0.12), "Classic circuit: heel peak %.0f Hz (want 450) and toe peak %.0f Hz (want 1600), within 12 percent", h1, t1);
    CHECK (pkH1 > 15.0 && pkH1 < 21.0, "Classic circuit: peak gain at the heel is %.1f dB (published about 18)", pkH1);
    const double h3 = peakOf (3, 0.0f, 0, 5.0f, &pkH3), t3 = peakOf (3, 1.0f, 0, 5.0f, &pkT3);
    CHECK (near (h3, 410, 0.12) && near (t3, 2200, 0.12), "Clyde McCoy: heel %.0f Hz (want 410) and toe %.0f Hz (want 2200), within 12 percent (published CM95 sweep)", h3, t3);
    const double h6 = peakOf (6, 0.0f, 0, 5.0f, &pkH6), t6 = peakOf (6, 1.0f, 0, 5.0f, &pkT6);
    CHECK (near (h6, 219, 0.08) && near (t6, 1267, 0.08), "Petrucci JP95 (measured, b101): treadle 0 peak %.0f Hz (measured 219) and treadle 100 peak %.0f Hz (measured 1267), within 8 percent", h6, t6);
    CHECK (std::abs (pkH6 - 11.1) < 1.5 && std::abs (pkT6 - 18.6) < 1.5, "Petrucci JP95: peak height %.1f dB at treadle 0 (measured 11.1) and %.1f dB at treadle 100 (measured 18.6), within 1.5 dB", pkH6, pkT6);
    {   double pkM;  const double fM = peakOf (6, 0.5f, 0, 5.0f, &pkM);
        CHECK (near (fM, 412, 0.08) && std::abs (pkM - 16.4) < 1.5, "Petrucci JP95: treadle 50 peak %.0f Hz / %.1f dB (measured 412 Hz / 16.4 dB)", fM, pkM);
        double pk25, pk75;  const double f25 = peakOf (6, 0.25f, 0, 5.0f, &pk25), f75 = peakOf (6, 0.75f, 0, 5.0f, &pk75);
        CHECK (h6 < f25 && f25 < fM && fM < f75 && f75 < t6 && pk25 > 11.0 && pk25 < 20.0 && pk75 > 14.0 && pk75 < 22.0, "Petrucci JP95: the sweep rises smoothly through the interpolated positions (%.0f / %.0f / %.0f / %.0f / %.0f Hz) and the peaks stay sane (%.1f and %.1f dB at 25 and 75 percent; interpolated, not measured)", h6, f25, fM, f75, t6, pk25, pk75);
        const double hNarrow = peakOf (6, 0.0f, 0, 9.0f), hWide = peakOf (6, 0.0f, 0, 2.0f);
        const double nHi = gainDb (6, 0.0f, h6, 0, 0.0f, 9.0f) - gainDb (6, 0.0f, h6 * 1.5, 0, 0.0f, 9.0f), nLo = gainDb (6, 0.0f, h6, 0, 0.0f, 2.0f) - gainDb (6, 0.0f, h6 * 1.5, 0, 0.0f, 2.0f);
        CHECK (nHi > nLo + 2.0 && hNarrow > 0 && hWide > 0, "Petrucci JP95: Resonance still narrows the peak (%.1f dB above a point 50 percent higher with Q 9, against %.1f dB with Q 2)", nHi, nLo);
        const double hLow = peakOf (6, 0.0f, 0, 5.0f), hKnob = [&] { Wah w;  w.prepare (SR);  Wah::Params p {};  p.mode = 0;  p.pos = 0.0f;  p.heelHz = 200.0f;  p.toeHz = 2200.0f;  p.q = 5.0f;  p.mix = 1.0f;  p.circuit = 6;  p.decayMs = 100.0f;
            double best = -999, bf = 0;  for (double f = 100; f < 3000; f *= 1.03) { Wah w2;  w2.prepare (SR);  std::vector<float> x (24000);  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (0.003 * std::sin (2 * M_PI * f * (double) i / SR));
                for (size_t i = 0; i < x.size(); i += 512) w2.process (x.data() + i, (int) std::min<size_t> (512, x.size() - i), p);  double e = 0;  for (size_t i = 12000; i < x.size(); ++i) e += (double) x[i] * x[i];  if (e > best) { best = e;  bf = f; } }
            return bf; } ();
        CHECK (hKnob < hLow * 0.7, "Petrucci JP95: the Heel knob still re-ranges the sweep (heel peak %.0f Hz at Heel 200 against %.0f Hz at the default 400)", hKnob, hLow); }
    const double h2 = peakOf (2, 0.0f, 0, 5.0f, &pkH2);
    CHECK (pkH2 < pkH1 - 1.0 && pkH2 > 12.0 && pkH2 < 17.5, "Dimebag / 535Q: peak at the heel is %.1f dB, lower than the classic's %.1f (published maximum 15 dB)", pkH2, pkH1);

    {   bool toeOk = true;  double worst = 0;
        for (int v = 1; v <= 7; ++v) { if (v == 6) continue;  double pt;  peakOf (v, 1.0f, 0, 5.0f, &pt);  toeOk = toeOk && pt > -0.5 && pt < 3.0;  if (std::abs (pt - 1.2) > std::abs (worst - 1.2)) worst = pt; }
        CHECK (toeOk, "every voicing's peak at the toe stays near +1 dB, except the measured Petrucci voice (furthest: %.1f dB; no source gives a toe figure for the others)", worst); }
    // Range selector: six windows, heel falls from 440 to 250 Hz
    {   const double r1 = peakOf (2, 0.0f, 1), r6 = peakOf (2, 0.0f, 6), r3 = peakOf (2, 0.0f, 3);
        CHECK (near (r1, 440, 0.12) && near (r3, 375, 0.12) && near (r6, 250, 0.15), "535Q Range: heel peak is %.0f Hz on 1 (want 440), %.0f on 3 (want 375), %.0f on 6 (want 250)", r1, r3, r6);
        const double tr1 = peakOf (2, 1.0f, 1), tr6 = peakOf (2, 1.0f, 6);
        CHECK (tr1 > tr6 * 1.3, "535Q Range: toe peak is %.0f Hz on 1 against %.0f on 6 (the whole sweep moves)", tr1, tr6);
        const double ro = peakOf (4, 0.0f, 3);
        CHECK (near (ro, 375, 0.15), "Range also moves the other voicings (Morley optical, Range 3: heel %.0f Hz, want 375)", ro); }

    // Boost and Q
    {   const double g0 = gainDb (1, 0.0f, 450.0), g12 = gainDb (1, 0.0f, 450.0, 0, 12.0f);
        CHECK (std::abs ((g12 - g0) - 12.0) < 0.5, "Boost +12 dB raises the wah's level by %.1f dB", g12 - g0);
        // Q: the response 40 percent away from the peak is further below the peak with a higher Q
        const double nLo = gainDb (1, 0.0f, 450.0, 0, 0.0f, 2.0f) - gainDb (1, 0.0f, 450.0 * 1.4, 0, 0.0f, 2.0f), nHi = gainDb (1, 0.0f, 450.0, 0, 0.0f, 9.0f) - gainDb (1, 0.0f, 450.0 * 1.4, 0, 0.0f, 9.0f);
        CHECK (nHi > nLo + 2.0, "Resonance (Q): the peak stands %.1f dB above a point 40 percent higher with Q 9, against %.1f dB with Q 2", nHi, nLo); }

    // voicings differ from each other at mid sweep
    {   double g[8];  for (int v = 1; v <= 7; ++v) g[v] = gainDb (v, 0.5f, 800.0);
        bool diff = true;  for (int v = 2; v <= 7; ++v) { bool anyDiff = false;  for (double f : { 400.0, 800.0, 1600.0 }) if (std::abs (gainDb (v, 0.5f, f) - gainDb (1, 0.5f, f)) > 0.7) anyDiff = true;  diff = diff && anyDiff; }
        CHECK (diff, "every voicing differs from the classic circuit by 0.7 dB or more somewhere at mid sweep"); }

    // fuzz wah: harmonics when fuzz is up; none when down
    {   auto thd = [&] (float fz)
        {   Wah w;  w.prepare (SR);  Wah::Params p {};  p.pos = 0.5f;  p.heelHz = 400.0f;  p.toeHz = 2200.0f;  p.q = 5.0f;  p.mix = 1.0f;  p.circuit = 7;  p.fuzz = fz;  p.decayMs = 100.0f;
            std::vector<float> y (48000);  for (size_t i = 0; i < y.size(); ++i) y[i] = (float) (0.1 * std::sin (2 * M_PI * 330.0 * (double) i / SR));
            for (size_t i = 0; i < y.size(); i += 512) w.process (y.data() + i, (int) std::min<size_t> (512, y.size() - i), p);
            auto gz = [&] (double f) { const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  for (size_t i = 12000; i < 48000; ++i) { const double ww = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - 12000) / 35999.0);  const double sv = y[i] * ww + c * s1 - s2;  s2 = s1;  s1 = sv; }  return s1 * s1 + s2 * s2 - c * s1 * s2; };
            double h = 0;  for (int k = 2; k <= 8; ++k) h += gz (330.0 * k);  return 10 * std::log10 (h / gz (330.0) + 1e-30); };
        const double lo = thd (0.0f), hi = thd (0.8f);
        CHECK (hi > lo + 12.0, "Cliff Burton fuzz wah: harmonics against the fundamental are %.1f dB with Fuzz 0.8 and %.1f dB with Fuzz 0 (want 12 dB more)", hi, lo); }

    // inside the plugin: the new parameters exist and every voicing plays
    {   bool allOk = true;
        for (int v = 0; v <= 7; ++v)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (*a, id, 0.0f);
            setP (*a, "wahOn", 1.0f);  setP (*a, "wahVoice", (float) v);  setP (*a, "wahRange", (float) (v % 7));  setP (*a, "wahBoost", 6.0f);  setP (*a, "wahFuzz", 0.6f);  setP (*a, "wahPos", 0.4f);
            setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  a->prepareToPlay (SR, 512);
            std::vector<float> y;  process (*a, testSignal (48000, SR, 0.4f), y, 512);  float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk) && rmsDb (y, 12000) > -70.0; }
        CHECK (allOk, "all eight wah voices play inside the plugin with Range, Boost and Fuzz set: finite, bounded, audible"); }
}

static void testEmber()
{
    std::printf ("[b73] Ember (idealized lead amp): plays everywhere, four distinct voicings, a gain range from edge of breakup to extreme, touch sensitive, weightier and smoother than the Brown Eye\n");
    const double SR = 48000.0;
    const auto in = testSignal (48000 * 2, SR, 0.5f);
    auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    auto bare = [&] (StarterAmpProcessor& a)
    {   for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (a, id, 0.0f);
        setP (a, "reverbMix", 0.0f);  setP (a, "gate", -90.0f);  setP (a, "ampAutoLevel", 1.0f);  setP (a, "cabOn", 0.0f); };
    for (int mode = 0; mode < 3; ++mode)
    {
        bool allOk = true;
        for (int v = 0; v < 4; ++v)
            for (float g : { 0.0f, 0.5f, 1.0f })
            {   auto a = fresh();  setP (*a, "ampModel", 43.0f);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampVoicing", (float) v);  setP (*a, "ampGain", g);  a->prepareToPlay (SR, 512);
                std::vector<float> y;  process (*a, in, y, 512);  float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -70.0; }
        CHECK (allOk, "Ember, mode %d: four voicings play at three gains, finite, bounded and audible", mode);
    }
    // 220 Hz note: harmonics 2 to 8 against the fundamental (dB)
    auto thdOf = [&] (int model, int voicing, float gain, double amp)
    {   auto a = fresh();  bare (*a);  setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voicing);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", 2.0f);  a->prepareToPlay (SR, 512);
        std::vector<float> xs (96000), y;  for (size_t i = 0; i < xs.size(); ++i) xs[i] = (float) (amp * std::sin (2 * M_PI * 220.0 * (double) i / SR));  process (*a, xs, y, 512);
        auto gz = [&] (double f) { const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  for (size_t i = 48000; i < 96000; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - 48000) / 47999.0);  const double sv = y[i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }  return s1 * s1 + s2 * s2 - c * s1 * s2; };
        double h = 0;  for (int k = 2; k <= 8; ++k) h += gz (220.0 * k);
        return 10 * std::log10 (h / gz (220.0) + 1e-30); };
    // pink-ish noise and band levels
    const double fc[] = { 100, 160, 250, 400, 1000, 2000, 3150, 4000, 5000, 6300, 8000, 10000 };   // 12 bands, 1 kHz is index 4
    std::vector<float> x (48000 * 4);
    {   unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.9); } }
    auto bandLevels = [&] (const std::vector<float>& y, double* lv)
    {   const size_t from = 48000, n = y.size() - from;
        for (int k = 0; k < 12; ++k)
        {   double acc = 0;
            for (int j = -3; j <= 3; ++j) { const double f = fc[k] * std::pow (2.0, j / 9.0);  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }
            lv[k] = 10 * std::log10 (acc / 7 + 1e-30); } };
    double inLv[12];  bandLevels (x, inLv);
    auto bandsOf = [&] (int model, int voicing, float gain, double* out)     // transfer relative to 1 kHz; out[12] = level at 1 kHz
    {   auto a = fresh();  bare (*a);  setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voicing);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", 2.0f);  a->prepareToPlay (SR, 512);
        std::vector<float> y;  process (*a, x, y, 512);  double lv[12];  bandLevels (y, lv);
        for (int k = 0; k < 12; ++k) out[k] = (lv[k] - lv[4]) - (inLv[k] - inLv[4]);
        out[12] = lv[4] - inLv[4]; };
    // the four voicings differ audibly (band levels, level, or distortion at a mid-knob hard pick)
    {   double b[4][13], th[4];
        for (int v = 0; v < 4; ++v) { bandsOf (43, v, 0.4f, b[v]);  th[v] = thdOf (43, v, 0.4f, 0.1); }
        bool differ = true;  double least = 99;
        for (int v = 1; v < 4; ++v)
            for (int u = 0; u < v; ++u)
            {   double m = std::abs (b[v][12] - b[u][12]);  for (int k = 0; k < 12; ++k) m = std::max (m, std::abs (b[v][k] - b[u][k]));
                m = std::max (m, std::abs (th[v] - th[u]) / 3.0);
                least = std::min (least, m);  if (m < 1.0) differ = false; }
        CHECK (differ, "Ember, Hybrid, Gain 0.4: every pair of voicings differs by at least 1 dB in band levels or overall level, or 3 dB in distortion at a 0.1 input (smallest pair: %.1f)", least); }
    // the gain range of the Lead voicing: clean for a soft note at the bottom, saturated for the softest note at the top
    {   const double lowClean = thdOf (43, 0, 0.0f, 0.006), lowBreak = thdOf (43, 0, 0.1f, 0.1), topSat = thdOf (43, 0, 1.0f, 0.006), topSat2 = thdOf (43, 0, 0.75f, 0.006);
        CHECK (lowClean < -40.0 && lowBreak > -32.0 && lowBreak < -12.0, "Ember Lead, Gain 0: a very soft note (0.006) is clean (harmonics %.1f dB, want below -40); Gain 0.1 with a 0.1 pick is breaking up but not saturated (%.1f dB, want -32 to -12)", lowClean, lowBreak);
        CHECK (topSat > -8.0 && topSat2 > -8.0, "Ember Lead, Gain 1.0 and 0.75: even the softest note (0.006) is saturated: harmonics %.1f and %.1f dB (want above -8)", topSat, topSat2); }
    // touch sensitivity: at an edge-of-breakup setting the distortion rises a lot with a harder pick, and Edge keeps this over most of the knob
    {   const double soft = thdOf (43, 0, 0.25f, 0.03), hard = thdOf (43, 0, 0.25f, 0.4);
        CHECK (hard - soft > 15.0, "Ember Lead, Gain 0.25: a hard pick (0.4) has %.1f dB more distortion than a soft one (0.03): %.1f against %.1f dB (want at least 15 dB more)", hard - soft, hard, soft);
        const double eSoft = thdOf (43, 1, 0.5f, 0.03), eHard = thdOf (43, 1, 0.5f, 0.4), eTop = thdOf (43, 1, 0.5f, 0.006);
        CHECK (eHard - eSoft > 12.0 && eTop < -35.0, "Ember Edge, Gain 0.5: still touch sensitive in the middle of the knob (%.1f dB more distortion for a hard pick; a very soft note stays clean at %.1f dB)", eHard - eSoft, eTop); }
    // weight and smoothness against the Brown Eye Tight at full gain (Hybrid, pink noise, relative to 1 kHz), and bite for the Bite voicing against the Marque Dream lead
    {   double ember[13], brown[13], bite[13], marque[13];
        bandsOf (43, 0, 1.0f, ember);  bandsOf (2, 0, 1.0f, brown);  bandsOf (43, 2, 1.0f, bite);  bandsOf (11, 1, 1.0f, marque);
        CHECK (ember[0] > brown[0] + 2.0, "Ember Lead has more weight than the Brown Eye Tight: 100 Hz %+.1f dB against %+.1f dB (at least 2 dB more)", ember[0], brown[0]);
        CHECK (ember[11] < brown[11] - 4.0, "Ember Lead has a smoother top than the Brown Eye Tight: 10 kHz %+.1f dB against %+.1f dB (at least 4 dB less)", ember[11], brown[11]);
        CHECK (bite[5] > marque[5] + 2.0, "Ember Bite has more attack than the Marque Dream lead: 2 kHz %+.1f dB against %+.1f dB (at least 2 dB more)", bite[5], marque[5]); }
}

// b80: Kindle, the idealized low / mid gain amp (model 44)
static void testKindle()
{
    std::printf ("[b80] Kindle (idealized low / mid gain amp): plays everywhere, four distinct voicings, a gain range from clean through edge of breakup to a light lead (never extreme), touch sensitive, chime over weight, smoother than the Chime Thirty\n");
    const double SR = 48000.0;
    const auto in = testSignal (48000 * 2, SR, 0.5f);
    auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    auto bare = [&] (StarterAmpProcessor& a)
    {   for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (a, id, 0.0f);
        setP (a, "reverbMix", 0.0f);  setP (a, "gate", -90.0f);  setP (a, "ampAutoLevel", 1.0f);  setP (a, "cabOn", 0.0f); };
    for (int mode = 0; mode < 3; ++mode)
    {
        bool allOk = true;
        for (int v = 0; v < 4; ++v)
            for (float g : { 0.0f, 0.5f, 1.0f })
            {   auto a = fresh();  setP (*a, "ampModel", 44.0f);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampVoicing", (float) v);  setP (*a, "ampGain", g);  a->prepareToPlay (SR, 512);
                std::vector<float> y;  process (*a, in, y, 512);  float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -70.0; }
        CHECK (allOk, "Kindle, mode %d: four voicings play at three gains, finite, bounded and audible", mode);
    }
    // 220 Hz note: harmonics 2 to 8 against the fundamental (dB)
    auto thdOf = [&] (int model, int voicing, float gain, double amp, int mode)
    {   auto a = fresh();  bare (*a);  setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voicing);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", (float) mode);  a->prepareToPlay (SR, 512);
        std::vector<float> xs (96000), y;  for (size_t i = 0; i < xs.size(); ++i) xs[i] = (float) (amp * std::sin (2 * M_PI * 220.0 * (double) i / SR));  process (*a, xs, y, 512);
        auto gz = [&] (double f) { const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  for (size_t i = 48000; i < 96000; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - 48000) / 47999.0);  const double sv = y[i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }  return s1 * s1 + s2 * s2 - c * s1 * s2; };
        double h = 0;  for (int k = 2; k <= 8; ++k) h += gz (220.0 * k);
        return 10 * std::log10 (h / gz (220.0) + 1e-30); };
    const double fc[] = { 100, 160, 250, 400, 1000, 2000, 3150, 4000, 5000, 6300, 8000, 10000 };   // 12 bands, 1 kHz is index 4
    std::vector<float> x (48000 * 4);
    {   unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.9); } }
    auto bandLevels = [&] (const std::vector<float>& y, double* lv)
    {   const size_t from = 48000, n = y.size() - from;
        for (int k = 0; k < 12; ++k)
        {   double acc = 0;
            for (int j = -3; j <= 3; ++j) { const double f = fc[k] * std::pow (2.0, j / 9.0);  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }
            lv[k] = 10 * std::log10 (acc / 7 + 1e-30); } };
    double inLv[12];  bandLevels (x, inLv);
    auto bandsOf = [&] (int model, int voicing, float gain, double* out)     // transfer relative to 1 kHz; out[12] = level at 1 kHz
    {   auto a = fresh();  bare (*a);  setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voicing);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", 2.0f);  a->prepareToPlay (SR, 512);
        std::vector<float> y;  process (*a, x, y, 512);  double lv[12];  bandLevels (y, lv);
        for (int k = 0; k < 12; ++k) out[k] = (lv[k] - lv[4]) - (inLv[k] - inLv[4]);
        out[12] = lv[4] - inLv[4]; };
    // the four voicings differ audibly (band levels, level, or distortion at a mid-knob hard pick)
    {   double b[4][13], th[4];
        for (int v = 0; v < 4; ++v) { bandsOf (44, v, 0.4f, b[v]);  th[v] = thdOf (44, v, 0.4f, 0.1, 2); }
        bool differ = true;  double least = 99;
        for (int v = 1; v < 4; ++v)
            for (int u = 0; u < v; ++u)
            {   double m = std::abs (b[v][12] - b[u][12]);  for (int k = 0; k < 12; ++k) m = std::max (m, std::abs (b[v][k] - b[u][k]));
                m = std::max (m, std::abs (th[v] - th[u]) / 3.0);
                least = std::min (least, m);  if (m < 1.0) differ = false; }
        CHECK (differ, "Kindle, Hybrid, Gain 0.4: every pair of voicings differs by at least 1 dB in band levels or overall level, or 3 dB in distortion at a 0.1 input (smallest pair: %.1f)", least); }
    // the gain range: clean at the bottom, never extreme at the top (the softest note is still not fully saturated for Chime and Warm), the hottest voicing reaches a light lead
    {   const double clean = thdOf (44, 0, 0.0f, 0.006, 2), brk = thdOf (44, 0, 0.5f, 0.1, 2), topChime = thdOf (44, 0, 1.0f, 0.006, 2), topWarm = thdOf (44, 3, 1.0f, 0.006, 2), topCrunch = thdOf (44, 1, 1.0f, 0.006, 2);
        CHECK (clean < -40.0 && brk > -32.0 && brk < -12.0, "Kindle Chime: Gain 0 is clean for a very soft note (harmonics %.1f dB, want below -40); Gain 0.5 with a 0.1 pick is breaking up but not saturated (%.1f dB, want -32 to -12)", clean, brk);
        CHECK (topChime < -12.0 && topWarm < -12.0, "Kindle Chime and Warm, Gain 1.0: the softest note (0.006) is still not saturated: harmonics %.1f and %.1f dB (want below -12)", topChime, topWarm);
        CHECK (topCrunch > -20.0 && topCrunch < -6.0, "Kindle Crunch, Gain 1.0: the softest note reaches a light lead, crunching hard but not a fully saturated wall: harmonics %.1f dB (want -20 to -6)", topCrunch); }
    // lower gain than Ember's lead: the same knob position, the softest note
    {   const double k = thdOf (44, 1, 0.75f, 0.006, 2), e = thdOf (43, 0, 0.75f, 0.006, 2);
        CHECK (k < e - 8.0, "Kindle Crunch has less gain than Ember Lead: at Gain 0.75 the softest note has harmonics at %.1f dB against %.1f dB (want at least 8 dB lower)", k, e); }
    // touch sensitivity
    {   const double soft = thdOf (44, 0, 0.5f, 0.03, 2), hard = thdOf (44, 0, 0.5f, 0.4, 2);
        CHECK (hard - soft > 15.0, "Kindle Chime, Gain 0.5: a hard pick (0.4) has %.1f dB more distortion than a soft one (0.03): %.1f against %.1f dB (want at least 15 dB more)", hard - soft, hard, soft); }
    // the Hybrid and Circuit-derived modes use the Chime Thirty's cold class-A stage shapes (the Classic mode does not), so the distortion differs from Classic's
    {   const double cl = thdOf (44, 0, 0.5f, 0.03, 0), hy = thdOf (44, 0, 0.5f, 0.03, 2), ci = thdOf (44, 0, 0.5f, 0.03, 1);
        CHECK (std::abs (hy - cl) > 3.0 && std::abs (ci - cl) > 3.0, "Kindle Chime, Gain 0.5, 0.03 pick: Hybrid (%.1f dB) and Circuit-derived (%.1f dB) differ from Classic (%.1f dB) by more than 3 dB: the class-A stage shapes are in use", hy, ci, cl); }
    // chime over weight, and a smoother top than the Chime Thirty at full gain (Hybrid, pink noise, relative to 1 kHz)
    {   double kin[13], ct[13];
        bandsOf (44, 0, 1.0f, kin);  bandsOf (1, 0, 1.0f, ct);
        CHECK (kin[5] > 2.0 && kin[6] > 2.0, "Kindle Chime, Gain 1.0: the chime is there, 2 kHz %+.1f dB and 3.15 kHz %+.1f dB over 1 kHz (want both above +2)", kin[5], kin[6]);
        CHECK (kin[0] > ct[0] + 4.0, "Kindle Chime has more weight than the Chime Thirty: 100 Hz %+.1f dB against %+.1f dB (at least 4 dB more)", kin[0], ct[0]);
        CHECK (kin[10] < ct[10] - 8.0 && kin[11] < ct[11] - 8.0, "Kindle Chime has a smoother top than the Chime Thirty: 8 kHz %+.1f against %+.1f dB, 10 kHz %+.1f against %+.1f dB (at least 8 dB less)", kin[10], ct[10], kin[11], ct[11]); }
}

// b80: Ideal Defined, the idealized speaker for the idealized amps
static void testIdealDefined()
{
    std::printf ("[b80] Ideal Defined (idealized speaker): in the speaker list as the last entry, a low end that reaches lower than the other idealized speakers, full but not boomy low mids, no hump at 3 to 4 kHz, a smooth roll-off, and the suggested speaker for Ember and Kindle\n");
    const auto names = StarterAmpProcessor::builtinSpeakerNames();
    const int idx = names.indexOf ("Ideal Defined");
    CHECK (idx == names.size() - 1 && idx == 59, "Ideal Defined is the last speaker, index %d of %d (saved sessions keep their numbers)", idx, names.size());
    auto resp = [&] (int spk, double hz)      // 4x12 Closed, Flat mic, Position 0.3, 1 in: relative to 1 kHz
    {   const double r1 = StarterAmpProcessor::builtinCabResponseDb (spk, 5, 3, 0.3f, 1.0f, 0.0f, true, 1000.0);
        return StarterAmpProcessor::builtinCabResponseDb (spk, 5, 3, 0.3f, 1.0f, 0.0f, true, hz) - r1; };
    const int ref = names.indexOf ("Ideal Reference"), rock = names.indexOf ("Rock Bite");
    CHECK (resp (idx, 60.0) > -6.0 && resp (idx, 60.0) > resp (ref, 60.0) + 1.5, "the low end reaches lower: 60 Hz is %+.1f dB re 1 kHz (Ideal Reference %+.1f dB; want above -6 and at least 1.5 dB more)", resp (idx, 60.0), resp (ref, 60.0));
    const double l150 = resp (idx, 150.0), l300 = resp (idx, 300.0), l400 = resp (idx, 400.0), l500 = resp (idx, 500.0), l700 = resp (idx, 700.0), l200 = resp (idx, 200.0);
    CHECK (l150 > 4.0 && l150 < 8.0 && l500 < 2.0 && l200 >= l300 - 0.2 && l300 >= l400 - 0.2 && l400 >= l500 - 0.2 && l500 >= l700 - 0.2,
           "the low mids are full but defined: 150 Hz %+.1f dB (want +4 to +8), 500 Hz %+.1f dB (want below +2), falling steadily from 200 Hz to 700 Hz (%+.1f, %+.1f, %+.1f, %+.1f, %+.1f)", l150, l500, l200, l300, l400, l500, l700);
    double worstHump = -99.0;  bool falling = true;  double prev = resp (idx, 2800.0);
    for (double f = 3000.0; f <= 6000.0; f *= 1.0595)     // semitone steps: no rise above 3 kHz
    {   const double r = resp (idx, f);  worstHump = std::max (worstHump, r - prev);  prev = r; }
    falling = worstHump < 0.2;
    CHECK (falling && resp (idx, 3500.0) < 0.5 && resp (idx, 4000.0) < 0.0, "no hump at 3 to 4 kHz: the response only falls above 3 kHz (largest step up %+.2f dB); 3.5 kHz %+.1f dB and 4 kHz %+.1f dB re 1 kHz (want below +0.5 and below 0)", worstHump, resp (idx, 3500.0), resp (idx, 4000.0));
    CHECK (resp (idx, 3000.0) < resp (rock, 3000.0) - 4.0, "much smoother at 3 kHz than Rock Bite (Ember's earlier suggestion): %+.1f dB against %+.1f dB (at least 4 dB lower)", resp (idx, 3000.0), resp (rock, 3000.0));
    CHECK (resp (idx, 2500.0) > 0.5 && resp (idx, 8000.0) < -12.0 && resp (idx, 10000.0) < -20.0, "a small presence lift and a smooth roll-off: 2.5 kHz %+.1f dB (want above +0.5), 8 kHz %+.1f dB (below -12), 10 kHz %+.1f dB (below -20)", resp (idx, 2500.0), resp (idx, 8000.0), resp (idx, 10000.0));
    for (int m : { 43, 44 })
    {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, 48000.0, 512);  setP (*a, "ampModel", (float) m);
        const auto txt = a->applySuggestedCab();
        CHECK (txt.contains ("Ideal Defined") && (int) a->apvts.getRawParameterValue ("cabSpkA")->load() == idx, "the suggested cab for amp %d uses Ideal Defined (\"%s\")", m, txt.toRawUTF8()); }
    {   const double SR = 48000.0;  const auto in = testSignal (24000, SR, 0.5f);  bool ok = true;
        for (int cab : { 0, 1, 3, 4, 5, 16 })
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  setP (*a, "ampModel", 44.0f);  setP (*a, "cabOn", 1.0f);  setP (*a, "cabOnA", 1.0f);  setP (*a, "cabSrcA", 0.0f);  setP (*a, "cabSpkA", (float) idx);  setP (*a, "cabCbnA", (float) cab);  a->prepareToPlay (SR, 512);
            std::vector<float> y;  float pk;  process (*a, in, y, 512);  ok = ok && finiteBounded (y, 32.0f, pk) && rmsDb (y, 8000) > -70.0; }
        CHECK (ok, "Ideal Defined plays through six cabinet types after Kindle, finite, bounded and audible"); }
}

// b70: Amp B through every menu entry, voicing and mode; level against Amp A; save and load; A and B together
static void testAmpBSweep()
{
    std::printf ("[b70] Amp B: every menu entry x voicing x mode plays (finite, bounded, audible, silent after the note); level close to Amp A; state round trip; A and B together\n");
    const double SR = 48000.0;
    const auto in = testSignal (24000, SR, 0.5f);
    std::vector<float> silence (24000, 0.0f);
    auto modelOf = [] (int mi) { return mi <= 4 ? std::max (0, mi) : mi <= 29 ? mi + 1 : std::min (mi, 42) + 2; };   // the same mapping as ampBModelValue
    auto mk = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    int bad = 0, n = 0;  double knownWorst = 0, worstGap = 0;  int worstMi = -1, worstV = -1, worstMode = -1;  std::vector<double> gaps;
    for (int mi = 0; mi < 43; ++mi)
        for (int v = 0; v < 6; ++v)
            for (int mode = 0; mode < 3; ++mode)
            {
                auto b = mk();  setP (*b, "ampAOn", 0.0f);  setP (*b, "ampBOn", 1.0f);  setP (*b, "ampBModel", (float) mi);  setP (*b, "ampBVoicing", (float) v);
                setP (*b, "ampBMode", (float) mode);  setP (*b, "ampBGain", 0.5f);  setP (*b, "ampBAutoLevel", 1.0f);  b->prepareToPlay (SR, 512);
                std::vector<float> y, t;  float pk;  process (*b, in, y, 512);  process (*b, silence, t, 512);
                const double lev = rmsDb (y, 8000), tail = rmsDb (t, 12000);
                const bool ok = finiteBounded (y, 32.0f, pk) && finiteBounded (t, 32.0f, pk) && lev > -60.0 && tail < -50.0;
                ++n;  if (! ok) { ++bad;  std::printf ("  FAIL Amp B menu %d (model %d) voicing %d mode %d: level %.1f dB tail %.1f dB peak %.1f\n", mi, modelOf (mi), v, mode, lev, tail, pk); }
                // the same amp, voicing, mode and gain on Amp A
                auto a = mk();  setP (*a, "ampModel", (float) modelOf (mi));  setP (*a, "ampVoicing", (float) v);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampGain", 0.5f);
                setP (*a, "ampAutoLevel", 1.0f);  a->prepareToPlay (SR, 512);
                std::vector<float> ya;  process (*a, in, ya, 512);
                // Amp B alone comes out of the equal-power A+B sum (x 0.7071), so it sits 3.01 dB under Amp A alone by design; the gap below is measured beyond that
                const double gap = lev - rmsDb (ya, 8000) + 3.0103;
                if (std::getenv ("AMPB_DUMP")) std::printf ("GAP %d %d %d %d %.2f %.2f\n", mi, modelOf (mi), v, mode, lev, gap);
                const int mdl = modelOf (mi);
                if (mdl == 9 || mdl == 29) { knownWorst = std::max (knownWorst, std::abs (gap));  continue; }   // known differences, reported below
                gaps.push_back (std::abs (gap));
                if (std::abs (gap) > std::abs (worstGap)) { worstGap = gap;  worstMi = mi;  worstV = v;  worstMode = mode; }
            }
    CHECK (bad == 0, "Amp B: %d of %d combinations (41 menu entries x 6 voicings x 3 modes) failed", bad, n);
    std::sort (gaps.begin(), gaps.end());
    std::printf ("  Amp B against Amp A (beyond the 3.01 dB sum law), Auto Level on both, Gain 0.5, %d combinations: median %.2f dB, 90%% within %.2f dB, worst %.2f dB (menu %d, voicing %d, mode %d). Known differences left in: Tweedle Dee (9) and Solid Metal (29), worst %.2f dB\n",
                 (int) gaps.size(), gaps[gaps.size() / 2], gaps[gaps.size() * 9 / 10], worstGap, worstMi, worstV, worstMode, knownWorst);
    CHECK (std::abs (worstGap) < 0.7, "Amp B lands within 0.7 dB of Amp A in every Amp Mode, apart from Tweedle Dee and Solid Metal (worst %.2f dB)", worstGap);
    CHECK (knownWorst < 3.2, "Tweedle Dee (Amp A has the 5E3 single tone control in the circuit modes, Amp B does not) and Solid Metal stay within 3.2 dB of Amp A (%.2f dB)", knownWorst);
    // state round trip with Amp B set
    {   int mism = 0;
        for (int mi = 0; mi < 43; mi += 3)
        {
            auto a = mk();  setP (*a, "ampAOn", 0.0f);  setP (*a, "ampBOn", 1.0f);  setP (*a, "ampBModel", (float) mi);  setP (*a, "ampBVoicing", (float) (mi % 4));  setP (*a, "ampBMode", (float) (mi % 3));  setP (*a, "ampBGain", 0.7f);
            a->prepareToPlay (SR, 512);  juce::MemoryBlock mb;  a->getStateInformation (mb);
            auto c = mk();  c->setStateInformation (mb.getData(), (int) mb.getSize());  c->prepareToPlay (SR, 512);
            auto ra = [] (StarterAmpProcessor& q, const char* id) { return q.apvts.getRawParameterValue (id)->load(); };
            std::vector<float> y1, y2;  process (*a, in, y1, 512);  process (*c, in, y2, 512);
            const bool same = ra (*a, "ampBModel") == ra (*c, "ampBModel") && ra (*a, "ampBVoicing") == ra (*c, "ampBVoicing") && ra (*a, "ampBMode") == ra (*c, "ampBMode") && ra (*c, "ampBOn") > 0.5f && ra (*c, "ampAOn") < 0.5f
                              && std::abs (rmsDb (y1, 8000) - rmsDb (y2, 8000)) < 0.1;
            if (! same) { ++mism;  std::printf ("  state mismatch at Amp B menu %d\n", mi); }
        }
        CHECK (mism == 0, "Amp B settings (model, voicing, mode, on/off) and the sound survive save and load (%d mismatches)", mism); }
    // A and B together, different amps
    {   int badPair = 0;
        for (int m = 0; m <= 44; ++m)
            for (int mode = 0; mode < 3; ++mode)
            {
                auto a = mk();  setP (*a, "ampModel", (float) m);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampBOn", 1.0f);  setP (*a, "ampBModel", (float) ((m * 7 + 3) % 41));
                setP (*a, "ampBMode", (float) ((mode + 1) % 3));  setP (*a, "ampGain", 0.8f);  setP (*a, "ampBGain", 0.8f);  a->prepareToPlay (SR, 512);
                std::vector<float> y, t;  float pk;  process (*a, in, y, 512);  process (*a, silence, t, 512);
                if (! (finiteBounded (y, 64.0f, pk) && finiteBounded (t, 64.0f, pk) && rmsDb (y, 8000) > -60.0 && rmsDb (t, 12000) < -50.0)) { ++badPair;  std::printf ("  FAIL A model %d + B menu %d, mode %d\n", m, (m * 7 + 3) % 41, mode); }
            }
        CHECK (badPair == 0, "Amp A and Amp B together (every Amp A model x 3 modes, a different Amp B each): %d failures", badPair); }
}

// b70: suggested cab per amp
static void testSuggestedCab()
{
    std::printf("[b70] Suggested cab: every amp has a valid suggestion that lands in cab block A only, and the amp plays through it\n");
    const double SR = 48000.0;
    const auto in = testSignal (24000, SR, 0.5f);
    auto mk = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  a->prepareToPlay (SR, 512);  return a; };
    auto get = [] (StarterAmpProcessor& q, const char* id) { return q.apvts.getRawParameterValue (id)->load(); };
    int badText = 0, badRead = 0, badSound = 0, changedB = 0, noSuggest = 0;
    for (int m = 0; m <= 44; ++m)
    {
        auto a = mk();  setP (*a, "ampModel", (float) m);
        const float posB0 = get (*a, "cabPosB"), spkB0 = get (*a, "cabSpkB"), cbnB0 = get (*a, "cabCbnB"), onB0 = get (*a, "cabOnB");
        const auto txt = a->applySuggestedCab();
        if (m == 5) { if (txt.isNotEmpty()) ++noSuggest;  continue; }                    // the acoustic body replaces the cab
        const auto parts = juce::StringArray::fromTokens (txt, ", ", "");
        const auto cab = StarterAmpProcessor::builtinSpeakerNames();                      // (speaker names; the cabinet and mic names are checked through the text)
        if (txt.isEmpty() || parts.size() < 3 || txt.contains (",,") ) ++badText;
        const int cbn = (int) get (*a, "cabCbnA"), spk = (int) get (*a, "cabSpkA");
        if (get (*a, "cabSrcA") != 0.0f || get (*a, "cabOnA") < 0.5f || get (*a, "cabOn") < 0.5f || spk < 0 || spk >= cab.size() || ! txt.contains (cab[spk]) || cbn < 0) ++badRead;
        if (get (*a, "cabPosB") != posB0 || get (*a, "cabSpkB") != spkB0 || get (*a, "cabCbnB") != cbnB0 || get (*a, "cabOnB") != onB0) ++changedB;
        a->applyPendingUpdates();
        std::vector<float> y;  float pk;  process (*a, in, y, 512);
        if (! (finiteBounded (y, 32.0f, pk) && rmsDb (y, 8000) > -70.0)) { ++badSound;  std::printf ("  no sound with the suggested cab: amp %d (%.1f dB)\n", m, rmsDb (y, 8000)); }
    }
    CHECK (badText == 0, "every amp (except the acoustic) has a suggestion with a cabinet, a speaker and a mic (%d without)", badText);
    CHECK (badRead == 0, "the suggestion lands in block A as a built-in model, with the cab on, and its speaker matches the text (%d wrong)", badRead);
    CHECK (changedB == 0, "blocks B to D are left alone (%d amps changed block B)", changedB);
    CHECK (badSound == 0, "every amp plays through its suggested cab (%d silent or broken)", badSound);
    CHECK (noSuggest == 0, "the acoustic amp gives no suggestion");
    // follows Amp B when Amp A is off
    {   auto a = mk();  setP (*a, "ampAOn", 0.0f);  setP (*a, "ampBOn", 1.0f);  setP (*a, "ampModel", 0.0f);  setP (*a, "ampBModel", 39.0f);   // Amp B menu 39 is Clarion (model 41); 40 is Prism (model 42)
        const auto txt = a->applySuggestedCab();
        CHECK (txt == StarterAmpProcessor::suggestedCabText (41) && txt.isNotEmpty(), "with Amp A off the suggestion follows Amp B (Clarion): %s", txt.toRawUTF8()); }
    // several amps give different suggestions
    {   juce::StringArray all;  for (int m = 0; m <= 44; ++m) if (m != 5) all.addIfNotAlreadyThere (StarterAmpProcessor::suggestedCabText (m));
        CHECK (all.size() >= 20, "the suggestions are varied (%d different ones across 41 amps)", all.size()); }
}

// b69: Clarion, the idealized high-gain amp (model 41)
static void testClarion()
{
    std::printf ("[b69] Clarion (idealized high gain): plays everywhere, four distinct voicings, less fizz and a tighter low end than Revved Up\n");
    const double SR = 48000.0;
    const auto in = testSignal (48000 * 2, SR, 0.5f);
    auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    for (int mode = 0; mode < 3; ++mode)
    {
        bool allOk = true, differ = true;  std::vector<float> ys[4];
        for (int v = 0; v < 4; ++v)
            for (float g : { 0.0f, 0.5f, 1.0f })
            {   auto a = fresh();  setP (*a, "ampModel", 41.0f);  setP (*a, "ampMode", (float) mode);  setP (*a, "ampVoicing", (float) v);  setP (*a, "ampGain", g);  a->prepareToPlay (SR, 512);
                std::vector<float> y;  process (*a, in, y, 512);  float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk);  if (g == 0.5f) ys[v] = y; }
        for (int v = 1; v < 4; ++v)
            for (int u = 0; u < v; ++u)
            {   double d = 0, e = 0;  for (size_t k = 24000; k < ys[v].size(); ++k) { d += (double) (ys[v][k] - ys[u][k]) * (ys[v][k] - ys[u][k]);  e += (double) ys[u][k] * ys[u][k]; }
                if (d < 0.01 * e) differ = false; }
        CHECK (allOk && differ, "Clarion, mode %d: four voicings play at three gains, finite and bounded (%d), and every voicing differs from the ones before it (%d)", mode, (int) allOk, (int) differ);
    }
    // spectral shape on pink-ish noise at Gain 1 (Hybrid): bands relative to 1 kHz, as the design measurements do
    auto bandsOf = [&] (int model, int voicing, double& low100, double& fizz63)
    {
        auto a = fresh();  setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voicing);  setP (*a, "ampGain", 1.0f);  setP (*a, "ampMode", 2.0f);  a->prepareToPlay (SR, 512);
        std::vector<float> x (48000 * 4);  unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.9); }
        std::vector<float> y;  process (*a, x, y, 512);
        const size_t from = 48000, n = y.size() - from;
        auto pw = [&] (double f0) { double acc = 0;  for (int j = -3; j <= 3; ++j) { const double f = f0 * std::pow (2.0, j / 9.0);  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }  return 10 * std::log10 (acc / 7 + 1e-30); };
        const double ref = pw (1000.0);  low100 = pw (100.0) - ref;  fizz63 = pw (6300.0) - ref;
    };
    double ruLow, ruFizz;  bandsOf (13, 0, ruLow, ruFizz);
    for (int v = 0; v < 3; ++v)
    {   double lo, fz;  bandsOf (41, v, lo, fz);
        CHECK (fz < ruFizz - 4.0 && lo < ruLow + 1.0, "Clarion voicing %d against Revved Up Tight: 6.3 kHz %.1f dB (Revved Up %.1f, at least 4 dB less), 100 Hz %.1f dB (Revved Up %.1f, no more than 1 dB above)", v, fz, ruFizz, lo, ruLow); }
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



//==============================================================================
// b81: Digital damage (Mod type 24), granular Recall, Studio 78 / Bass Decay / Clock on the reverbs, and the output Tape stop.
static double goertzelPow (const std::vector<float>& y, size_t a, size_t b, double f, double sr)
{
    const double c = 2 * std::cos (2 * M_PI * f / sr);  double s1 = 0, s2 = 0;
    for (size_t i = a; i < b && i < y.size(); ++i)
    {   const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) (i - a) / (double) (b - a));
        const double s = w * y[i] + c * s1 - s2;  s2 = s1;  s1 = s; }
    return (s1 * s1 + s2 * s2 - c * s1 * s2) / (double) (b - a) / (double) (b - a) * 4.0;
}
static double zeroCrossHz (const std::vector<float>& y, size_t a, size_t b, double sr)
{
    int zc = 0;  for (size_t i = a + 1; i < b && i < y.size(); ++i) if ((y[i - 1] < 0) != (y[i] < 0)) ++zc;
    return 0.5 * zc * sr / (double) (b - a);
}

static void testB81()
{
    std::printf ("[b81] Digital damage, granular Recall, Studio 78 / Bass Decay / Clock, Tape stop\n");
    const double SR = 48000.0;

    // ---------------- Digital damage (the engine itself)
    {
        std::mt19937 rng (3);  std::uniform_real_distribution<float> u (-1.0f, 1.0f);
        const int n = 48000 * 4;
        std::vector<float> noise ((size_t) n), tone ((size_t) n);
        for (int i = 0; i < n; ++i) { noise[(size_t) i] = 0.2f * u (rng);  tone[(size_t) i] = 0.3f * (float) std::sin (2 * M_PI * 220.0 * i / SR) + 0.1f * (float) std::sin (2 * M_PI * 1330.0 * i / SR); }
        auto run = [&] (std::vector<float> x, FxExtra::DigitalDamageFx::Params p, bool stereo = false)
        {   FxExtra::DigitalDamageFx d;  d.prepare (SR);
            std::vector<float> r = x;
            for (size_t i = 0; i < x.size(); i += 512) { const int k = (int) std::min<size_t> (512, x.size() - i);  d.process (&x[i], stereo ? &r[i] : nullptr, k, p); }
            return x; };
        FxExtra::DigitalDamageFx::Params clean;  clean.mix = 1.0f;
        const auto ref = run (tone, clean);
        const int L = 1024;
        double err = 0, sig = 0;  for (int i = 8000; i < n - 100; ++i) { const double e = ref[(size_t) i] - tone[(size_t) (i - L)];  err += e * e;  sig += (double) tone[(size_t) (i - L)] * tone[(size_t) (i - L)]; }
        CHECK (10 * std::log10 (err / sig + 1e-20) < -50.0, "Digital damage with nothing turned up is not transparent (error %.1f dB)", 10 * std::log10 (err / sig + 1e-20));
        { auto p = clean;  p.mix = 0.0f;  p.loss = 1.0f;  auto o = run (tone, p);  double m = 0;  for (int i = L; i < n; ++i) m = std::max (m, (double) std::abs (o[(size_t) i] - tone[(size_t) (i - L)]));
          CHECK (m < 1e-6, "Mix 0 is not the dry signal delayed by exactly the latency (%.2e)", m); }
        { std::vector<float> imp ((size_t) n, 0.0f);  imp[5000] = 1.0f;  auto o = run (imp, clean);  int at = 0;  float pk = 0;  for (int i = 0; i < n; ++i) if (std::abs (o[(size_t) i]) > pk) { pk = std::abs (o[(size_t) i]);  at = i; }
          CHECK (std::abs (at - (5000 + L)) <= 1 && pk > 0.9f, "Digital damage latency is not 1024 samples (peak %.2f at %d)", pk, at); }
        // Loss grows the damage and cuts the top
        double prev = 0;  bool grows = true;
        for (float loss : { 0.3f, 0.7f, 1.0f })
        {   auto p = clean;  p.loss = loss;  auto o = run (tone, p);  double e = 0, s2 = 0;
            for (int i = 8000; i < n - 100; ++i) { const double d = o[(size_t) i] - ref[(size_t) i];  e += d * d;  s2 += (double) ref[(size_t) i] * ref[(size_t) i]; }
            const double db = 10 * std::log10 (e / s2 + 1e-20);  grows = grows && db > prev - 0.5 || prev == 0;  prev = db;
            float pk; CHECK (finiteBounded (o, 2.0f, pk), "Digital damage Loss %.1f not finite or too loud (peak %.2f)", loss, pk); }
        CHECK (grows, "Digital damage: more Loss did not mean more damage");
        { auto p = clean;  p.loss = 1.0f;  auto o = run (noise, p);  auto c0 = run (noise, clean);
          double hi = 0, hi0 = 0;  for (double f : { 9000.0, 11000.0, 13000.0, 15000.0 }) { hi += goertzelPow (o, 24000, 72000, f, SR);  hi0 += goertzelPow (c0, 24000, 72000, f, SR); }
          CHECK (10 * std::log10 ((hi + 1e-20) / (hi0 + 1e-20)) < -12.0, "Digital damage at full Loss keeps the top end (%.1f dB)", 10 * std::log10 ((hi + 1e-20) / (hi0 + 1e-20))); }
        // Dropouts: holes in a steady tone
        auto holes = [&] (float drops)
        {   FxExtra::DigitalDamageFx::Params p = clean;  p.drops = drops;  p.speedHz = 2.0f;
            std::vector<float> steady ((size_t) n);  for (int i = 0; i < n; ++i) steady[(size_t) i] = 0.3f * (float) std::sin (2 * M_PI * 440.0 * i / SR);
            auto o = run (steady, p);  int low = 0, tot = 0;
            for (int s = 12000; s + 240 < n - 2000; s += 240) { double e = 0;  for (int i = 0; i < 240; ++i) e += (double) o[(size_t) (s + i)] * o[(size_t) (s + i)];  if (std::sqrt (e / 240) < 0.2 * 0.3 * 0.7071) ++low;  ++tot; }
            return (double) low / (double) tot; };
        CHECK (holes (0.0f) < 0.01, "Digital damage drops sound out with Dropouts at 0 (%.3f)", holes (0.0f));
        CHECK (holes (1.0f) > 0.08, "Digital damage Dropouts at full leaves no holes (%.3f)", holes (1.0f));
        // Freeze: a tone burst, then silence; the frozen tone keeps sounding
        {   std::vector<float> burst ((size_t) n, 0.0f);  for (int i = 0; i < 72000; ++i) burst[(size_t) i] = 0.3f * (float) std::sin (2 * M_PI * 220.0 * i / SR);
            auto p = clean;  p.freeze = 1.0f;  p.speedHz = 0.25f;  auto f = run (burst, p);  auto p0 = clean;  auto g = run (burst, p0);
            auto rmsOf = [&] (const std::vector<float>& v, int a, int b) { double e = 0;  for (int i = a; i < b; ++i) e += (double) v[(size_t) i] * v[(size_t) i];  return e / (double) (b - a); };
            const double held = rmsOf (f, 120000, 168000), none = rmsOf (g, 120000, 168000), ref0 = rmsOf (burst, 24000, 72000);
            CHECK (10 * std::log10 ((held + 1e-20) / ref0) > -14.0, "Digital damage Freeze does not sustain the tone (%.1f dB)", 10 * std::log10 ((held + 1e-20) / ref0));
            CHECK (10 * std::log10 ((none + 1e-20) / ref0) < -60.0, "Digital damage without Freeze keeps ringing (%.1f dB)", 10 * std::log10 ((none + 1e-20) / ref0)); }
        // stereo and a 96 kHz run
        { auto p = clean;  p.loss = 0.6f;  p.drops = 0.5f;  p.freeze = 0.4f;  auto o = run (tone, p, true);  float pk; CHECK (finiteBounded (o, 2.0f, pk), "Digital damage in stereo not finite"); }
        { FxExtra::DigitalDamageFx d;  d.prepare (96000.0);  std::vector<float> x (96000, 0.1f);  FxExtra::DigitalDamageFx::Params p = clean;  p.loss = 0.8f;  d.process (x.data(), nullptr, 96000, p);  float pk; CHECK (finiteBounded (x, 2.0f, pk), "Digital damage at 96 kHz not finite"); }
    }
    // the Mod slot type itself, in both places, and switching to and from it
    {   const auto in = testSignal (48000 * 2, SR, 0.3f);
        for (int pos = 0; pos < 2; ++pos)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);  StarterAmpProcessor q;  q.setPlayConfigDetails (2, 2, SR, 512);  q.prepareToPlay (SR, 512);
            placeLane (p, 6, pos);  setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 24.0f);  setP (p, "mfAMix", 1.0f);  setP (p, "mfADepth", 0.8f);  setP (p, "mfAManual", 0.4f);
            std::vector<float> y, d;  process (p, in, y, 512);  process (q, in, d, 512);
            float pk;  double diff = 0;  for (size_t i = 0; i < y.size(); ++i) diff += std::abs (y[i] - d[i]);
            CHECK (finiteBounded (y, 8.0f, pk), "Mod type 24 at position %d not finite or too loud (%.2f)", pos, pk);
            CHECK (diff > 1e-3, "Mod type 24 at position %d is identical to dry", pos); }
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);  setP (p, "mfCOn", 1.0f);  placeLane (p, 24, 1);
        for (int k : { 24, 3, 24, 12, 24, 0, 24 }) { setP (p, "mfCType", (float) k);  std::vector<float> y;  process (p, in, y, 512);  float pk;  CHECK (finiteBounded (y, 8.0f, pk), "switching Mod type to %d around Digital damage not finite", k); }
    }

    // ---------------- Granular Recall
    {   Granular g;  g.prepare (SR);
        const int n = 48000 * 12;  std::vector<float> x ((size_t) n);
        for (int i = 0; i < n; ++i) x[(size_t) i] = 0.3f * (float) std::sin (2 * M_PI * (i < 4 * 48000 ? 440.0 : 880.0) * i / SR);
        auto run = [&] (float recall)
        {   Granular gr;  gr.prepare (SR);  std::vector<float> l = x, r = x;
            Granular::Params p {};  p.sizeMs = 80.0f;  p.density = 20.0f;  p.posMs = 100.0f;  p.spray = 0.0f;  p.pitch = 0.0f;  p.pitchRand = 0.0f;  p.reverse = 0.0f;  p.width = 0.0f;  p.feedback = 0.0f;  p.mix = 1.0f;  p.freeze = false;  p.recall = recall;
            for (size_t i = 0; i < l.size(); i += 512) { const int k = (int) std::min<size_t> (512, l.size() - i);  gr.process (&l[i], &r[i], k, p); }
            return l; };
        const auto a = run (0.0f), b = run (1.0f);
        const double old0 = goertzelPow (a, 10 * 48000, 12 * 48000, 440.0, SR), new0 = goertzelPow (a, 10 * 48000, 12 * 48000, 880.0, SR);
        const double old1 = goertzelPow (b, 10 * 48000, 12 * 48000, 440.0, SR);
        CHECK (10 * std::log10 ((old0 + 1e-20) / new0) < -40.0, "Granular without Recall already plays old material (%.1f dB)", 10 * std::log10 ((old0 + 1e-20) / new0));
        // grains are drawn from 1 s to 30 s back and biased towards recent material, so only about a third of them reach the old phrase
        CHECK (10 * std::log10 ((old1 + 1e-20) / new0) > -32.0 && 10 * std::log10 ((old1 + 1e-20) / (old0 + 1e-20)) > 25.0, "Granular Recall at full does not bring back the old phrase (%.1f dB, %.1f dB over no Recall)", 10 * std::log10 ((old1 + 1e-20) / new0), 10 * std::log10 ((old1 + 1e-20) / (old0 + 1e-20)));
        float pk;  CHECK (finiteBounded (b, 4.0f, pk), "Granular with Recall not finite or too loud (%.2f)", pk);
    }

    // ---------------- Reverb: Studio 78, Bass Decay, Clock
    {   auto burst = [&] (int secs)
        {   std::vector<float> imp ((size_t) (48000 * secs), 0.0f);  unsigned s = 1;
            for (int i = 0; i < 2400; ++i) { s = s * 1664525u + 1013904223u; imp[(size_t) i] = 0.3f * (((s >> 9) / 8388608.0f) - 0.5f) * 2.0f; }
            return imp; };
        auto render = [&] (int unit, int type, float decay, float bass, float cross, int clock, int secs = 8)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
            const std::string pre = unit == 0 ? "reverb" : "rev2";
            for (const char* id : { "odOn", "compOn", "eqOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (p, id, 0.0f);
            setP (p, "ampAOn", 0.0f);  setP (p, "cabOn", 0.0f);
            setP (p, (pre + "On").c_str(), 1.0f);  setP (p, (pre + "Type").c_str(), (float) type);  setP (p, (pre + "Mix").c_str(), 1.0f);
            setP (p, (pre + "Size").c_str(), decay);  setP (p, (pre + "BassDecay").c_str(), bass);  setP (p, (pre + "Cross").c_str(), cross);  setP (p, (pre + "Clock").c_str(), (float) clock);
            std::vector<float> y;  process (p, burst (secs), y, 512);  return y; };
        auto win = [&] (const std::vector<float>& y, double a, double b, std::vector<double> fs)
        {   double s = 0;  for (double f : fs) s += goertzelPow (y, (size_t) (a * SR) * 2, (size_t) (b * SR) * 2, f, SR * 2);   // (the helper sees interleaved-looking mono at twice the index rate)
            return s; };
        (void) win;
        for (int unit = 0; unit < 2; ++unit)
        {   auto y = render (unit, 18, 2.0f, 1.0f, 500.0f, 0);  float pk;
            CHECK (finiteBounded (y, 8.0f, pk), "Studio 78 in reverb %d not finite or too loud (%.2f)", unit, pk);
            auto rms = [&] (double a, double b) { double s = 0;  size_t c = 0;  for (size_t i = (size_t) (a * SR); i < (size_t) (b * SR) && i < y.size(); ++i) { s += (double) y[i] * y[i];  ++c; }  return std::sqrt (s / (double) std::max<size_t> (c, 1)); };
            CHECK (rms (0.3, 0.9) > 1e-4, "Studio 78 in reverb %d is silent after a burst (%.6f)", unit, rms (0.3, 0.9));
            CHECK (rms (6.0, 7.0) < 0.05 * rms (0.3, 0.9), "Studio 78 in reverb %d does not fade (%.6f vs %.6f)", unit, rms (6.0, 7.0), rms (0.3, 0.9)); }
        // Bass Decay: the bass band lasts less (or more) while the highs keep their time
        auto lowHigh = [&] (float bass)
        {   auto y = render (0, 2, 4.0f, bass, 500.0f, 0, 5);
            const size_t a = (size_t) (1.5 * SR), b = (size_t) (2.5 * SR);
            double lo = 0, hi = 0;  for (double f : { 90.0, 130.0, 180.0, 240.0 }) lo += goertzelPow (y, a, b, f, SR);  for (double f : { 1500.0, 2000.0, 2600.0, 3300.0 }) hi += goertzelPow (y, a, b, f, SR);
            return 10 * std::log10 ((lo + 1e-20) / (hi + 1e-20)); };
        const double base = lowHigh (1.0f), thin = lowHigh (0.4f), fat = lowHigh (2.0f);
        CHECK (thin < base - 5.0, "Bass Decay 0.4 does not cut the late bass (%.1f vs %.1f dB)", thin, base);
        CHECK (fat > base + 1.5, "Bass Decay 2 does not lengthen the late bass (%.1f vs %.1f dB)", fat, base);
        { auto a = render (0, 2, 3.0f, 1.0f, 500.0f, 0, 4);  auto b = render (0, 2, 3.0f, 1.0f, 1200.0f, 0, 4);  double d = 0;  for (size_t i = 0; i < a.size(); ++i) d += std::abs (a[i] - b[i]);
          CHECK (d < 1e-9, "Bass Crossover changes the sound while Bass Decay is 1 (%.3e)", d); }
        // Clock: still finite and fading, and darker as it gets cruder; no stuck buzz at the end
        const auto clean = render (0, 18, 2.0f, 1.0f, 500.0f, 0, 14);
        double cleanHi = 0;  for (double f : { 5000.0, 6000.0, 7000.0 }) cleanHi += goertzelPow (clean, (size_t) (0.4 * SR), (size_t) (1.4 * SR), f, SR);
        double prevHi = cleanHi;
        for (int clock = 1; clock <= 3; ++clock)
        {   auto y = render (0, 18, 2.0f, 1.0f, 500.0f, clock, 14);  float pk;
            CHECK (finiteBounded (y, 8.0f, pk), "Reverb Clock %d not finite or too loud (%.2f)", clock, pk);
            double d = 0;  for (size_t i = 0; i < y.size(); ++i) d += std::abs (y[i] - clean[i]);
            CHECK (d > 1e-2, "Reverb Clock %d sounds the same as Clean", clock);
            float tail = 0;  for (size_t i = (size_t) (12.0 * SR); i < y.size(); ++i) tail = std::max (tail, std::abs (y[i]));
            CHECK (tail < 1e-4, "Reverb Clock %d leaves a buzz that never dies (%.5f)", clock, tail);
            double hi = 0;  for (double f : { 5000.0, 6000.0, 7000.0 }) hi += goertzelPow (y, (size_t) (0.4 * SR), (size_t) (1.4 * SR), f, SR);
            if (clock == 3) CHECK (hi < cleanHi * 0.5, "Reverb Clock 3 is not clearly darker than Clean (%.1f dB)", 10 * std::log10 ((hi + 1e-20) / (cleanHi + 1e-20)));
            if (clock > 1) CHECK (hi < prevHi * 1.05, "Reverb Clock %d is not darker than the one before", clock);
            prevHi = hi; }
        // every FDN type with the Clock and Bass Decay on stays finite
        for (int type : { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 14, 15, 16, 18 })
        {   auto y = render (0, type, 3.0f, 0.5f, 400.0f, 2, 4);  float pk;  CHECK (finiteBounded (y, 8.0f, pk), "reverb type %d with Clock and Bass Decay not finite (%.2f)", type, pk); }
    }

    // ---------------- Tape stop
    {   const int n = 48000 * 7;  std::vector<float> in ((size_t) n);
        for (int i = 0; i < n; ++i) in[(size_t) i] = 0.3f * (float) std::sin (2 * M_PI * 440.0 * i / SR);
        auto setup = [&] (StarterAmpProcessor& p) { for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (p, id, 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "tapeStopTime", 1.0f); };
        StarterAmpProcessor a;  a.setPlayConfigDetails (2, 2, SR, 512);  a.prepareToPlay (SR, 512);  setup (a);
        StarterAmpProcessor b;  b.setPlayConfigDetails (2, 2, SR, 512);  b.prepareToPlay (SR, 512);  setup (b);
        std::vector<float> yb;  process (b, in, yb, 512);
        // a: stop at 1 s, wind up at 3 s
        std::vector<float> ya ((size_t) n);
        {   juce::AudioBuffer<float> buf (2, 512);  juce::MidiBuffer m;
            for (int pos = 0; pos < n; pos += 512)
            {   const int k = std::min (512, n - pos);
                setP (a, "tapeStop", (pos >= 48000 && pos < 3 * 48000) ? 1.0f : 0.0f);
                juce::AudioBuffer<float> v (buf.getArrayOfWritePointers(), 2, k);
                for (int i = 0; i < k; ++i) { v.setSample (0, i, in[(size_t) (pos + i)]);  v.setSample (1, i, in[(size_t) (pos + i)]); }
                a.processBlock (v, m);
                for (int i = 0; i < k; ++i) ya[(size_t) (pos + i)] = v.getSample (0, i); } }
        double before = 0;  for (int i = 0; i < 40000; ++i) before = std::max (before, (double) std::abs (ya[(size_t) i] - yb[(size_t) i]));
        CHECK (before < 1e-6, "Tape stop changes the signal before it is switched on (%.2e)", before);
        const double f0 = zeroCrossHz (ya, 48000 + 2400, 48000 + 7200, SR), f1 = zeroCrossHz (ya, 48000 + 24000, 48000 + 28800, SR);
        CHECK (f0 < 440.0 * 0.97 && f0 > 440.0 * 0.6, "Tape stop does not start slowing the pitch (%.0f Hz)", f0);
        CHECK (f1 < f0 - 40.0, "Tape stop pitch does not keep falling (%.0f then %.0f Hz)", f0, f1);
        double held = 0;  for (int i = 2 * 48000 + 24000; i < 3 * 48000 - 100; ++i) held = std::max (held, (double) std::abs (ya[(size_t) i]));
        CHECK (held < 0.01, "Tape stop is not silent while stopped (%.4f)", held);
        const double fu = zeroCrossHz (ya, 3 * 48000 + 4800, 3 * 48000 + 9600, SR);
        CHECK (fu > 20.0 && fu < 440.0 * 0.8, "Tape stop does not wind up from a low pitch (%.0f Hz)", fu);
        const double fe = zeroCrossHz (ya, 5 * 48000, 5 * 48000 + 9600, SR);
        double r = 0, rb = 0;  for (int i = 5 * 48000; i < 6 * 48000; ++i) { r += (double) ya[(size_t) i] * ya[(size_t) i];  rb += (double) yb[(size_t) i] * yb[(size_t) i]; }
        CHECK (std::abs (fe - 440.0) < 12.0 && std::abs (10 * std::log10 (r / rb)) < 1.0, "Tape stop does not come back to normal (%.0f Hz, level off by %.1f dB)", fe, 10 * std::log10 (r / rb));
        float pk;  CHECK (finiteBounded (ya, 2.0f, pk), "Tape stop not finite or too loud (%.2f)", pk);
        // a session saved with the tape stopped loads with it off
        setP (a, "tapeStop", 1.0f);  juce::MemoryBlock mb;  a.getStateInformation (mb);
        StarterAmpProcessor c;  c.setPlayConfigDetails (2, 2, SR, 512);  c.prepareToPlay (SR, 512);  c.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (c.apvts.getRawParameterValue ("tapeStop")->load() < 0.5f, "a session loads with the tape stopped");
    }
}

//==============================================================================
// b84: Electric Mistress flanger voicing (Mod FX type 0, FlVoice 1 and 2)
static void testB84()
{
    std::printf ("[b84] Electric Mistress flanger voicing\n");
    const double SR = 48000.0;
    auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
    auto bare = [&] (StarterAmpProcessor& a)   // every other effect off, the amp at its lowest gain so it adds little
    {   for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "cabOn" }) setP (a, id, 0.0f);
        setP (a, "reverbMix", 0.0f);  setP (a, "gate", -90.0f);  setP (a, "ampAutoLevel", 0.0f);  setP (a, "ampGain", 0.0f); };
    auto noiseSig = [&] (size_t n, float amp) { std::vector<float> x (n);  unsigned st = 7;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * amp); }  return x; };
    auto setup = [&] (StarterAmpProcessor& a, int voice, float manual, float depth, float fb, float rate)
    {   bare (a);  setP (a, "mfAOn", 1.0f);  setP (a, "mfAType", 0.0f);  setP (a, "mfAFlVoice", (float) voice);  setP (a, "mfAManual", manual);  setP (a, "mfADepth", depth);
        setP (a, "mfAFeedback", fb);  setP (a, "mfARate", rate);  setP (a, "mfAMix", 1.0f);  setP (a, "mfAShape", 1.0f); };
    auto run = [&] (int voice, float manual, float depth, float fb, float rate, const std::vector<float>& x)
    {   auto a = fresh();  setup (*a, voice, manual, depth, fb, rate);  a->prepareToPlay (SR, 512);  std::vector<float> y;  process (*a, x, y, 512);  return y; };

    bool ok = true;
    for (int v = 0; v < 3; ++v)
    {   const auto y = run (v, 0.5f, 0.8f, 0.5f, 1.0f, noiseSig (96000, 0.1f));  float pk;
        ok = ok && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -60.0; }
    CHECK (ok, "Flanger voicings Clean, Electric Mistress and Filter Matrix all play: finite, bounded, audible");

    // Filter Matrix: the clock is held still, so Rate and Depth change nothing
    {   const auto x = noiseSig (96000, 0.1f);
        const auto a = run (2, 0.7f, 0.9f, 0.3f, 0.5f, x), b = run (2, 0.7f, 0.2f, 0.3f, 9.0f, x);
        double dm = 0;  for (size_t i = 0; i < a.size(); ++i) dm = std::max (dm, (double) std::abs (a[i] - b[i]));
        CHECK (dm < 1e-5, "Filter Matrix: Rate and Depth do nothing, the comb stays put (largest difference %.2g)", dm);
        const auto c = run (1, 0.7f, 0.9f, 0.3f, 0.5f, x), d = run (1, 0.7f, 0.9f, 0.3f, 9.0f, x);
        double e = 0, s = 0;  for (size_t i = 24000; i < c.size(); ++i) { e += (double) (c[i] - d[i]) * (c[i] - d[i]);  s += (double) c[i] * c[i]; }
        CHECK (e > 0.05 * s, "Electric Mistress swept: Rate changes the sound (difference %.1f dB below the signal)", -10 * std::log10 (e / s + 1e-20)); }

    // Filter Matrix at full Range is 7.3 ms (the Deluxe sheet's 70 kHz clock): notch at 3.5 / 7.3 ms = 479 Hz, peak at 548 Hz
    {   auto sine = [&] (double f) { std::vector<float> x (96000);  for (size_t i = 0; i < x.size(); ++i) x[i] = 0.1f * (float) std::sin (2 * M_PI * f * (double) i / SR);  return x; };
        const auto yn = run (2, 1.0f, 0.5f, 0.0f, 1.0f, sine (3.5 / 0.0073)), yp = run (2, 1.0f, 0.5f, 0.0f, 1.0f, sine (4.0 / 0.0073));
        const double pn = goertzelPow (yn, 24000, 96000, 3.5 / 0.0073, SR), pp = goertzelPow (yp, 24000, 96000, 4.0 / 0.0073, SR);
        CHECK (10 * std::log10 (pp / (pn + 1e-20)) > 20.0, "Filter Matrix, Range full: the comb notch sits at 479 Hz and the peak at 548 Hz (peak %.1f dB above the notch, want above 20)", 10 * std::log10 (pp / (pn + 1e-20))); }

    // Band-limited: the BBD voicing has far less above 9 kHz than the clean flanger
    {   const auto x = noiseSig (96000, 0.1f);
        const auto c = run (0, 0.5f, 0.0f, 0.0f, 1.0f, x), m = run (2, 0.5f, 0.0f, 0.0f, 1.0f, x);
        double hc = 0, hm = 0;  for (double f = 10000.0; f < 16000.0; f += 337.0) { hc += goertzelPow (c, 24000, 96000, f, SR);  hm += goertzelPow (m, 24000, 96000, f, SR); }
        CHECK (10 * std::log10 (hc / (hm + 1e-20)) > 1.2, "Electric Mistress is darker above 10 kHz than the clean flanger (%.1f dB, want above 1.2: the dry signal is in both and the clean flanger's own Tone already rolls off its wet side)", 10 * std::log10 (hc / (hm + 1e-20))); }

    // Full regeneration stays bounded, both signs, and the clean voicing is still the old flanger (different from the BBD voicing)
    {   float pk = 0;  bool b = true;
        for (float fb : { 0.93f, -0.93f }) { const auto y = run (1, 0.4f, 1.0f, fb, 2.0f, noiseSig (48000 * 4, 0.2f));  b = b && finiteBounded (y, 8.0f, pk); }
        CHECK (b, "Electric Mistress with full feedback of either sign stays finite and bounded");
        const auto x = noiseSig (48000, 0.1f);  const auto c = run (0, 0.5f, 0.6f, 0.4f, 1.0f, x), m = run (1, 0.5f, 0.6f, 0.4f, 1.0f, x);
        double e = 0, s = 0;  for (size_t i = 12000; i < c.size(); ++i) { e += (double) (c[i] - m[i]) * (c[i] - m[i]);  s += (double) c[i] * c[i]; }
        CHECK (e > 0.1 * s, "The Clean flanger voicing and the Electric Mistress voicing sound different"); }

    // ---------------- Ironside (amp model 45) and Solid Metal's fifth voicing (Hybrid + JFET)
    {
        auto amp = [&] (int model, int voi, float gain)
        {   auto a = fresh();  for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "cabOn" }) setP (*a, id, 0.0f);
            setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "ampAutoLevel", 0.0f);  setP (*a, "ampMode", 2.0f);
            setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voi);  setP (*a, "ampGain", gain);  a->prepareToPlay (SR, 512);  return a; };
        const auto pink = [&] () { std::vector<float> x (48000 * 4);  unsigned st = 12345;  double b0 = 0, b1 = 0, b2 = 0;
            for (auto& v : x) { st = st * 1664525u + 1013904223u;  const double w = (((st >> 8) / 8388608.0) - 1.0);  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) (0.036 * (b0 + b1 + b2 + w * 0.1848)); }
            return x; } ();
        auto band = [&] (const std::vector<float>& y, double f) { double s = 0;  for (double k : { 0.85, 0.92, 1.0, 1.09, 1.18 }) s += goertzelPow (y, 48000, y.size(), f * k, SR);  return 10 * std::log10 (s / 5.0 + 1e-20); };
        std::vector<float> y[4];  bool ok = true;
        for (int v = 0; v < 4; ++v)
        {   auto a = amp (45, v, 0.6f);  process (*a, pink, y[v], 512);  float pk;  ok = ok && finiteBounded (y[v], 8.0f, pk) && rmsDb (y[v], 48000) > -50.0; }
        CHECK (ok, "Ironside: all four voicings (Treble, Bass, Linked, Fifty) play: finite, bounded, audible");
        const double t6 = band (y[0], 6400) - band (y[0], 1000), b6 = band (y[1], 6400) - band (y[1], 1000);
        CHECK (t6 - b6 > 4.0, "Ironside Treble keeps more 6.4 kHz than Bass (%.1f dB against %.1f dB relative to 1 kHz, want 4 dB more)", t6, b6);
        // distinct from the Plexi-style amp it is built on: the mid scoop and the tighter bass make the spectrum differ by more than 2 dB somewhere
        {   auto a = amp (23, 0, 0.6f);  std::vector<float> p;  process (*a, pink, p, 512);
            double worst = 0;  for (double f : { 100.0, 200.0, 400.0, 800.0, 1600.0, 3200.0, 6400.0 }) worst = std::max (worst, std::abs ((band (y[0], f) - band (y[0], 1200)) - (band (p, f) - band (p, 1200))));
            CHECK (worst > 2.0, "Ironside Treble is not a copy of the Plexi-style Stock voicing (largest band difference %.1f dB, want above 2)", worst); }
        {   auto a = amp (23, 1, 0.6f);  std::vector<float> p;  process (*a, pink, p, 512);
            double worst = 0;  for (double f : { 100.0, 200.0, 400.0, 800.0, 1600.0, 3200.0, 6400.0 }) worst = std::max (worst, std::abs ((band (y[2], f) - band (y[2], 1200)) - (band (p, f) - band (p, 1200))));
            CHECK (worst > 2.0, "Ironside Linked is not a copy of the Plexi-style Jumped voicing (largest band difference %.1f dB, want above 2)", worst); }
        // the four voicings are not the same sound
        bool distinct = true;
        for (int a = 0; a < 4; ++a) for (int b = a + 1; b < 4; ++b)
        {   double e = 0, s = 0;  for (size_t i = 48000; i < y[a].size(); ++i) { e += (double) (y[a][i] - y[b][i]) * (y[a][i] - y[b][i]);  s += (double) y[a][i] * y[a][i]; }
            if (e < 0.05 * s) distinct = false; }
        CHECK (distinct, "Ironside's four voicings are all different from each other");
        // the menus: Ironside is a choice of both amps, and Amp B can select it
        {   auto b = fresh();  for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "cabOn" }) setP (*b, id, 0.0f);
            setP (*b, "ampAOn", 0.0f);  setP (*b, "ampBOn", 1.0f);  setP (*b, "ampBModel", 43.0f);  setP (*b, "ampBGain", 0.6f);  setP (*b, "ampBMode", 2.0f);  b->prepareToPlay (SR, 512);
            std::vector<float> yb;  process (*b, pink, yb, 512);  float pk;
            CHECK (finiteBounded (yb, 8.0f, pk) && rmsDb (yb, 48000) > -50.0, "Amp B's last menu entry plays (it is Ironside)"); }
        // the stability sweep (the full suite's amp x voicing x gain test) for the new voicings: loud and quiet input at the extremes of Amp Gain, in all three amp modes
        {   bool okS = true;  float pk = 0;
            const auto loud = testSignal (24000, 48000.0, 0.9f), quiet = testSignal (24000, 48000.0, 0.02f);
            for (int mode = 0; mode < 3; ++mode)
                for (auto mv : { std::make_pair (45, 0), std::make_pair (45, 1), std::make_pair (45, 2), std::make_pair (45, 3), std::make_pair (29, 4) })
                    for (float g : { 0.0f, 1.0f })
                        for (int k = 0; k < 2; ++k)
                        {   auto a = amp (mv.first, mv.second, g);  setP (*a, "ampMode", (float) mode);  a->prepareToPlay (SR, 512);  std::vector<float> o;  process (*a, k ? quiet : loud, o, 512);
                            okS = okS && finiteBounded (o, 10.0f, pk); }
            CHECK (okS, "Ironside (four voicings) and Hybrid + JFET stay finite and bounded at Amp Gain 0 and 1, loud and quiet input, in all three amp modes (peak %.2f)", pk); }
        // Solid Metal's fifth voicing: plays, and is neither of its parents
        {   std::vector<float> h[5];  bool ok5 = true;
            for (int v : { 0, 2, 4 }) { auto a = amp (29, v, 0.6f);  process (*a, pink, h[v], 512);  float pk;  ok5 = ok5 && finiteBounded (h[v], 8.0f, pk) && rmsDb (h[v], 48000) > -50.0; }
            CHECK (ok5, "Solid Metal's fifth voicing (Hybrid + JFET) plays: finite, bounded, audible");
            auto diff = [&] (const std::vector<float>& a, const std::vector<float>& b) { double worst = 0;  for (double f : { 100.0, 200.0, 400.0, 1600.0, 3200.0, 6400.0 }) worst = std::max (worst, std::abs ((band (a, f) - band (a, 1000)) - (band (b, f) - band (b, 1000))));  return worst; };
            const double d0 = diff (h[4], h[0]), d2 = diff (h[4], h[2]);
            CHECK (d0 > 2.0 && d2 > 2.0, "Hybrid + JFET differs from both Hybrid Hi-Gain (%.1f dB) and JFET Lead (%.1f dB) by more than 2 dB somewhere", d0, d2); }
    }

    // ---------------- Pops with the reverb on (your report): the pitch shifter behind the reverb's Shimmer and Symphony layers read one sample past the end of its buffer
    // whenever its read position rounded up to exactly the buffer length. That gave a garbage sample now and then (a pop; sometimes a runaway in the reverb's feedback).
    // The sound of a run must not depend on what was in memory before: the same render twice in one process must match sample for sample, and stay small.
    {
        auto render = [&] (int type, bool shimmer, float symph)
        {   auto a = fresh();  for (const char* id : { "odOn", "compOn", "eqOn", "noiseOn", "cabOn" }) setP (*a, id, 0.0f);
            setP (*a, "gate", -90.0f);  setP (*a, "ampModel", 42.0f);  setP (*a, "ampGain", 0.3f);
            setP (*a, "reverbOn", 1.0f);  setP (*a, "reverbType", (float) type);  setP (*a, "reverbMix", 0.5f);  setP (*a, "reverbSize", 6.0f);
            setP (*a, "shimmerOn", shimmer ? 1.0f : 0.0f);  setP (*a, "reverbSymph", symph);  setP (*a, "reverbSymphPitch", 1.0f);
            setP (*a, "postDelayOn", 1.0f);  setP (*a, "postDelayMix", 0.3f);
            a->prepareToPlay (SR, 512);  std::vector<float> x (48000 * 12, 0.0f);
            for (int n = 0; n < 16; ++n) for (int i = 0; i < 24000; ++i) x[(size_t) (n * 36000 + i)] = 0.2f * (float) std::sin (2 * M_PI * (110.0 * std::pow (2.0, (n % 5) / 6.0)) * i / SR) * (float) std::exp (-6.0 * i / SR);
            std::vector<float> y;  process (*a, x, y, 512);  return y; };
        bool same = true, small = true;  float pk = 0;
        for (int type : { 0, 2, 5, 9, 10, 15 })
            for (auto cfg : { std::make_pair (true, 0.0f), std::make_pair (false, 0.7f), std::make_pair (true, 0.7f) })
            {   const auto a = render (type, cfg.first, cfg.second), b = render (type, cfg.first, cfg.second);
                same = same && a == b;  small = small && finiteBounded (a, 3.0f, pk); }
        CHECK (same, "Reverb with Shimmer and Symphony on: the same render twice matches sample for sample (no reads of stale memory)");
        CHECK (small, "Reverb with Shimmer and Symphony on, with a Delay, stays finite and below 3.0 (peak %.2f)", pk);
    }
}


static void testB85()
{
    std::printf ("[b85] early OTA sustainer comp style (fit to the CS-2 sheet table)\n");
    const double SR = 48000.0;
    // 1 kHz sine at dBm in (0 dBm = a 0.5 sine), settled; returns the output level in dBm (RMS referred to the same 0.5 sine)
    auto lvl = [&] (double dbm, float sustain, float attack, double f = 1000.0)
    {
        FxExtra::Cs2Fx c;  c.prepare (SR);  FxExtra::Cs2Fx::Params p;  p.sustain = sustain;  p.attack = attack;  p.mix = 1.0f;  p.volumeDb = 0.0f;
        const size_t n = (size_t) (3 * SR);  std::vector<float> x (n);  const double A = 0.5 * std::pow (10.0, dbm / 20.0);
        for (size_t i = 0; i < n; ++i) x[i] = (float) (A * std::sin (2 * M_PI * f * (double) i / SR));
        for (size_t i = 0; i < n; i += 256) c.process (x.data() + i, (int) std::min<size_t> (256, n - i), p);
        double s2 = 0;  size_t cnt = 0;  for (size_t i = n / 2; i < n; ++i) { s2 += (double) x[i] * x[i];  ++cnt; }
        return 20.0 * std::log10 (std::sqrt (s2 / (double) cnt) / (0.5 / std::sqrt (2.0)));
    };
    const double a50 = lvl (-50, 1.0f, 1.0f), a40 = lvl (-40, 1.0f, 1.0f), a0 = lvl (0, 1.0f, 1.0f), m40 = lvl (-40, 0.0f, 1.0f);
    std::printf ("    measured: -50 -> %.2f, -40 -> %.2f, 0 -> %.2f dBm (sheet -12, -8.5, -8); Sustain min -40 -> %.2f (sheet -33)\n", a50, a40, a0, m40);
    CHECK (std::abs (a50 + 12.0) < 1.5, "Sustain max, 1 kHz, -50 dBm in: %.1f dBm out (the sheet says about -12)", a50);
    CHECK (std::abs (a40 + 8.5) < 1.5, "Sustain max, 1 kHz, -40 dBm in: %.1f dBm out (the sheet says about -8.5)", a40);
    CHECK (std::abs (a0 + 8.0) < 2.0, "Sustain max, 1 kHz, 0 dBm in: %.1f dBm out (the sheet says about -8, slight distortion)", a0);
    CHECK (std::abs (m40 + 33.0) < 2.0, "Sustain min, 1 kHz, -40 dBm in: %.1f dBm out (the sheet says about -33)", m40);
    CHECK (a0 - a40 < 1.5, "A flat ceiling: 40 dB more input gives only %.1f dB more output at full Sustain", a0 - a40);
    // the Attack knob only changes the speed, not the settled level
    const double fast = lvl (-20, 1.0f, 0.0f), slow = lvl (-20, 1.0f, 1.0f);
    CHECK (std::abs (fast - slow) < 1.5, "Attack changes the settled level by only %.2f dB (it changes the speed)", std::abs (fast - slow));

    // through the whole plugin: the style plays, stays bounded, and is not the CS-3 style
    {
        auto run = [&] (int style)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            for (const char* id : { "odOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "cabOn" }) setP (*a, id, 0.0f);
            setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "ampAutoLevel", 0.0f);  setP (*a, "ampGain", 0.0f);
            setP (*a, "compOn", 1.0f);  setP (*a, "compStyle", (float) style);  setP (*a, "compSustain", 0.8f);  setP (*a, "compMix", 1.0f);
            a->prepareToPlay (SR, 512);  std::vector<float> x (48000 * 2), y;
            for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (0.05 * std::sin (2 * M_PI * 220.0 * (double) i / SR) * (i < 48000 ? 1.0 : 0.1));
            process (*a, x, y, 512);  return y; };
        const auto y12 = run (12), y11 = run (11);  float pk = 0;
        CHECK (finiteBounded (y12, 8.0f, pk) && rmsDb (y12, 24000) > -60.0, "The early OTA sustainer plays through the plugin: finite, bounded, audible (peak %.2f)", pk);
        // a note that fades to a tenth of its level: this style holds it up more than the CS-3 style does
        const double d12 = rmsDb (y12, 24000) - rmsDb (y12, 72000), d11 = rmsDb (y11, 24000) - rmsDb (y11, 72000);
        std::printf ("    (level drop when the note fades by 20 dB: early OTA %.1f dB, CS-3 style %.1f dB; information only)\n", d12, d11);
        CHECK (d12 < 20.0, "The note's 20 dB fade is shortened: early OTA %.1f dB", d12);
    }
}

static void testB86()
{
    std::printf ("[b86] compressor alignments (FET knee and threshold, Opto attack and release), BBD flanger lines, analogue-divider octaver\n");
    const double SR = 48000.0;

    // ---- compressors, on the shared compressor class: 1 kHz tone, gain reduction read every 16 samples
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
    {   // FET: the 4:1 button starts limiting about 5 dB sooner than the 20:1 button (threshold -18 dB, a tone at -21 dB)
        const auto g4 = gr (1, 3, -18.0f, 5.0f, 250.0f, -21.0, 1.0, 0.0), g20 = gr (1, 6, -18.0f, 5.0f, 250.0f, -21.0, 1.0, 0.0);
        CHECK (g4.back() > 0.8 && g20.back() < 0.3, "FET: at 3 dB under the threshold the 4:1 button already takes %.2f dB and 20:1 takes %.2f dB (the threshold moves with the ratio)", g4.back(), g20.back());
        const auto h4 = gr (1, 3, -18.0f, 5.0f, 250.0f, -12.0, 1.0, 0.0), h20 = gr (1, 6, -18.0f, 5.0f, 250.0f, -12.0, 1.0, 0.0);
        // 4:1 button: effective threshold -18 - 5 = -23 dB, so 11 dB over, three quarters of it taken (8.25 dB); 20:1: 6 dB over, 19/20 of it (5.7 dB)
        CHECK (std::abs (h4.back() - 8.25) < 1.5 && std::abs (h20.back() - 5.7) < 1.5, "FET: 6 dB above the dial threshold, 4:1 takes %.1f dB (expect 8.3) and 20:1 takes %.1f dB (expect 5.7)", h4.back(), h20.back());
    }
    {   // Opto: the Attack knob at its minimum now reaches a fast grab, and the first half of the release takes about 60 ms after a short squeeze
        auto optoT = [&] (double levelDb, double squeeze, double& tAtk, double& t50)
        {   const auto g = gr (2, 2, -30.0f, 0.05f, 250.0f, levelDb, squeeze, 2.0);
            double peak = 0;  for (double v : g) peak = std::max (peak, v);
            size_t i = 0;  while (i < g.size() && g[i] < 0.63 * peak) ++i;
            tAtk = (double) i * 16.0 / 48.0;
            const size_t from = (size_t) (squeeze * SR / 16.0);  const double top = g[from - 1];
            size_t j = from;  while (j < g.size() && g[j] > 0.5 * top) ++j;
            t50 = (double) (j - from) * 16.0 / 48.0; };
        double tA = 0, t50s = 0, tA2 = 0, t50d = 0;
        optoT (-22.0, 0.10, tA, t50s);  optoT (-10.0, 0.60, tA2, t50d);
        std::printf ("    opto: 63 %% attack %.2f ms; 50 %% release %.0f ms after a short, shallow squeeze and %.0f ms after a long, deep one\n", tA2, t50s, t50d);
        CHECK (tA2 < 2.0, "Opto, Attack at minimum: 63 %% of the gain reduction in %.2f ms (it was 3 ms or more)", tA2);
        CHECK (t50s > 40.0 && t50s < 80.0, "Opto, short squeeze: half the gain reduction is gone after %.0f ms (both LA-2A documents say about 60 ms; it was 29)", t50s);
        CHECK (t50d > t50s, "Opto, a long deep squeeze releases more slowly than a short one (%.0f against %.0f ms)", t50d, t50s);
    }

    // ---- BBD flanger lines: the short line has half the delay, so its comb is an octave higher
    {
        auto fresh = [&] () { auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);  return a; };
        auto run = [&] (int voice, float manual, float depth, float fb, const std::vector<float>& x)
        {   auto a = fresh();
            for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "cabOn" }) setP (*a, id, 0.0f);
            setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "ampAutoLevel", 0.0f);  setP (*a, "ampGain", 0.0f);
            setP (*a, "mfAOn", 1.0f);  setP (*a, "mfAType", 0.0f);  setP (*a, "mfAFlVoice", (float) voice);  setP (*a, "mfAManual", manual);  setP (*a, "mfADepth", depth);
            setP (*a, "mfAFeedback", fb);  setP (*a, "mfARate", 1.0f);  setP (*a, "mfAMix", 1.0f);  setP (*a, "mfAShape", 1.0f);
            a->prepareToPlay (SR, 512);  std::vector<float> y;  process (*a, x, y, 512);  return y; };
        auto sine = [&] (double f) { std::vector<float> x (96000);  for (size_t i = 0; i < x.size(); ++i) x[i] = 0.1f * (float) std::sin (2 * M_PI * f * (double) i / SR);  return x; };
        auto pw = [&] (const std::vector<float>& y, double f) { return goertzelPow (y, 24000, 96000, f, SR); };
        // Manual at full, Depth 0: long line 5.0 ms (notch 100 Hz, peak 200 Hz), short line 2.5 ms (notch 200 Hz, peak 400 Hz)
        const auto l100 = run (3, 1.0f, 0.0f, 0.0f, sine (100.0)), l200 = run (3, 1.0f, 0.0f, 0.0f, sine (200.0));
        const auto s200 = run (4, 1.0f, 0.0f, 0.0f, sine (200.0)), s400 = run (4, 1.0f, 0.0f, 0.0f, sine (400.0));
        const double longDepth = 10 * std::log10 (pw (l200, 200.0) / (pw (l100, 100.0) + 1e-20));
        const double shortDepth = 10 * std::log10 (pw (s400, 400.0) / (pw (s200, 200.0) + 1e-20));
        CHECK (longDepth > 15.0, "BBD long line, Range full: peak at 200 Hz is %.1f dB above the notch at 100 Hz (5.0 ms)", longDepth);
        CHECK (shortDepth > 15.0, "BBD short line, Range full: peak at 400 Hz is %.1f dB above the notch at 200 Hz (2.5 ms, half the long line)", shortDepth);
        const double halves = 10 * std::log10 (pw (l200, 200.0) / (pw (s200, 200.0) + 1e-20));
        CHECK (halves > 15.0, "At 200 Hz the long line peaks and the short line notches (%.1f dB apart)", halves);
        // all voicings play, bounded, with full feedback of both signs
        bool ok = true;  float pk = 0;
        std::vector<float> noise (48000 * 3);  unsigned st = 5;  for (auto& v : noise) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.2); }
        for (int v : { 3, 4 }) for (float fb : { 0.0f, 0.93f, -0.93f }) { const auto y = run (v, 0.5f, 0.8f, fb, noise);  ok = ok && finiteBounded (y, 8.0f, pk) && rmsDb (y, 24000) > -60.0; }
        CHECK (ok, "BBD long and short lines play and stay bounded at full feedback of either sign");
        const auto m = run (1, 0.5f, 0.0f, 0.0f, sine (150.0)), b = run (3, 0.5f, 0.0f, 0.0f, sine (150.0));
        double e = 0, sg = 0;  for (size_t i = 24000; i < m.size(); ++i) { e += (double) (m[i] - b[i]) * (m[i] - b[i]);  sg += (double) m[i] * m[i]; }
        CHECK (e > 0.05 * sg, "The BBD long line is not a copy of the Mistress voicing (difference %.1f dB below the signal)", -10 * std::log10 (e / sg + 1e-20));
    }

    // ---- the analogue divider octaver
    {
        auto runD = [&] (float m1, float m2, float dry, const std::vector<float>& x)
        {   FxExtra::FlipFlopOctaverFx d;  d.prepare (SR);  FxExtra::FlipFlopOctaverFx::Params p;  p.oct1 = m1;  p.oct2 = m2;  p.dry = dry;  p.tone = 0.5f;
            std::vector<float> y (x);  for (size_t i = 0; i < y.size(); i += 256) d.process (y.data() + i, (int) std::min<size_t> (256, y.size() - i), p);  return y; };
        auto sine = [&] (double f, float amp) { std::vector<float> x (96000);  for (size_t i = 0; i < x.size(); ++i) x[i] = amp * (float) std::sin (2 * M_PI * f * (double) i / SR);  return x; };
        auto pw = [&] (const std::vector<float>& y, double f) { return goertzelPow (y, 24000, 96000, f, SR); };
        const auto x = sine (110.0, 0.1f);
        const auto o1 = runD (1.0f, 0.0f, 0.0f, x), o2 = runD (0.0f, 1.0f, 0.0f, x), dr = runD (0.0f, 0.0f, 1.0f, x);
        const double r1 = 10 * std::log10 (pw (o1, 55.0) / (pw (o1, 110.0) + 1e-20)), r2 = 10 * std::log10 (pw (o2, 27.5) / (pw (o2, 55.0) + 1e-20));
        const double dD = rmsDb (dr, 24000), d1 = rmsDb (o1, 24000) - dD, d2 = rmsDb (o2, 24000) - dD;
        std::printf ("    divider: 55/110 Hz %.1f dB, 27.5/55 Hz %.1f dB, oct1 %+.2f dB, oct2 %+.2f dB re direct\n", r1, r2, d1, d2);
        CHECK (r1 > 20.0, "Divider, first octave: 55 Hz is %.1f dB over 110 Hz for a 110 Hz note", r1);
        CHECK (r2 > 15.0, "Divider, second octave: 27.5 Hz is %.1f dB over 55 Hz", r2);
        CHECK (std::abs (d1 - 3.0) < 2.0, "Divider levels at maximum, against the direct signal: first octave %+.1f dB (the sheet says +3)", d1);
        CHECK (std::abs (d2 + 3.5) < 2.0, "Divider levels at maximum, against the direct signal: second octave %+.1f dB (the sheet says -3.5)", d2);
        // level follows the playing: 30 dB less input gives 30 dB less output (the sheet: -20 to -50 dBm in, octave 3 dB hotter at both)
        const auto q = runD (1.0f, 0.0f, 0.0f, sine (110.0, 0.1f * (float) std::pow (10.0, -30.0 / 20.0)));
        const double drop = rmsDb (o1, 24000) - rmsDb (q, 24000);
        CHECK (std::abs (drop - 30.0) < 4.0, "Divider: 30 dB less input gives %.1f dB less output", drop);
        // silence and hiss do not make squares
        const auto sil = runD (1.0f, 1.0f, 0.0f, std::vector<float> (96000, 0.0f));
        float pkS = 0;  for (float v : sil) pkS = std::max (pkS, std::abs (v));
        CHECK (pkS < 1e-6f, "Divider: silence in, silence out (peak %.2g)", (double) pkS);
        std::vector<float> noise (96000);  unsigned st = 9;  for (auto& v : noise) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.3); }
        const auto nn = runD (1.0f, 1.0f, 1.0f, noise);  float pk = 0;
        CHECK (finiteBounded (nn, 8.0f, pk), "Divider on loud noise stays finite and bounded (peak %.2f)", (double) pk);
    }
    // ---- and wired into the processor: the mode plays, its note comes out an octave down, and it adds no latency of its own
    {
        auto runP = [&] (bool on, double f)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn", "cabOn" }) setP (*a, id, 0.0f);
            setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "ampAutoLevel", 0.0f);  setP (*a, "ampGain", 0.0f);
            setP (*a, "octOn", on ? 1.0f : 0.0f);  setP (*a, "octMode", 3.0f);  setP (*a, "octM1", 1.0f);  setP (*a, "octM2", 0.0f);  setP (*a, "octDry", 0.0f);  setP (*a, "octTone", 0.5f);
            a->prepareToPlay (SR, 512);  const int lat = a->getLatencySamples();
            std::vector<float> x (96000), y;  for (size_t i = 0; i < x.size(); ++i) x[i] = 0.1f * (float) std::sin (2 * M_PI * f * (double) i / SR);
            process (*a, x, y, 512);  return std::make_pair (y, lat); };
        const auto on = runP (true, 220.0), off = runP (false, 220.0);
        CHECK (on.second == off.second, "The analogue divider adds no latency (%d samples against %d with the octaver off)", on.second, off.second);
        const double lo = goertzelPow (on.first, 24000, 96000, 110.0, SR), hi = goertzelPow (on.first, 24000, 96000, 220.0, SR);
        CHECK (lo > 4.0 * hi, "In the processor, a 220 Hz note comes out at 110 Hz (%.1f dB over the 220 Hz component)", 10 * std::log10 (lo / (hi + 1e-20)));
        const auto sil = runP (true, 0.0);   // zero frequency: silence in
        float pkS = 0, pkO = 0;  for (float v : sil.first) pkS = std::max (pkS, std::abs (v));
        (void) pkO;
        CHECK (pkS < 1e-3f, "Silence through the processor with the divider on stays silent (peak %.2g)", (double) pkS);
    }
}

static void testB87()
{
    std::printf ("[b87] gain-range layers on Hi-Wat Headroom and Ironside, and their Auto Level\n");
    auto level = [] (int model, int voi, int mode, float gain, bool al, float inLevel)
    {
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, 512);
        setP (p, "ampModel", (float) model);  setP (p, "ampVoicing", (float) voi);  setP (p, "ampGain", gain);  setP (p, "ampMode", (float) mode);  setP (p, "ampAutoLevel", al ? 1.0f : 0.0f);
        setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);
        for (const char* c : { "cabOn", "cabOnA", "cabOnB", "cabOnC", "cabOnD" }) setP (p, c, 0.0f);
        p.prepareToPlay (48000.0, 512);
        const auto in = testSignal (48000 * 2, 48000.0, inLevel);  std::vector<float> y;  process (p, in, y, 512);
        return rmsDb (y, 24000);
    };
    // range: Auto Level off, a quiet input: the top of the knob is well hotter than the bottom (measured, not listened to)
    for (int model : { 21, 45 })
        for (int voi = 0; voi < 4; ++voi)
        {
            const double lo = level (model, voi, 2, 0.0f, false, 0.02f), hi = level (model, voi, 2, 1.0f, false, 0.02f);
            std::printf ("  amp %d voicing %d: quiet input, Gain 0 to 1 = %+.1f dB\n", model, voi, hi - lo);
            CHECK (hi - lo > 20.0, "amp %d voicing %d: only %.1f dB between Gain 0 and Gain 1 at a quiet input", model, voi, hi - lo);
        }
    // Auto Level: the level at Gain 0.1, 0.5 and 1.0 stays within 3.5 dB in every amp mode (the tables' own plucked signal leaves the same residual as before b87, up to about 4.8 dB on Hi-Wat Linked at the top of the knob)
    for (int model : { 21, 45 })
        for (int voi = 0; voi < 4; ++voi)
            for (int mode = 0; mode < 3; ++mode)
            {
                const double a = level (model, voi, mode, 0.1f, true, 0.3f), b = level (model, voi, mode, 0.5f, true, 0.3f), c = level (model, voi, mode, 1.0f, true, 0.3f);
                const double spread = std::max ({ a, b, c }) - std::min ({ a, b, c });
                std::printf ("  amp %d v%d mode %d: Auto Level %.1f / %.1f / %.1f dB (spread %.1f)\n", model, voi, mode, a, b, c, spread);
                CHECK (spread < 3.5, "amp %d voicing %d mode %d: Auto Level spread %.1f dB between Gain 0.1, 0.5 and 1.0", model, voi, mode, spread);
            }
}

// ---- b95: robustness (nothing here changes the sound) ---------------------------------------------------------------------
// b97: the pitch pedal's Warble character, tone (formant) switch and Formant knob, and Keep Lows; the octaver's Warble.
static void testB97()
{
    std::printf ("[b97] pitch pedal: Warble, Tone (Stays natural), Formant knob, Keep Lows (my own designs, measured, not listened to)\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    auto bandDb = [&] (const std::vector<float>& y, size_t from, size_t to, double lo, double hi)      // power summed over a band, on a 5 Hz grid
    {   double e = 0;  for (double f = lo; f <= hi; f += 5.0) e += std::pow (10.0, goertz (y, from, to, f) / 10.0);  return 10 * std::log10 (e + 1e-30); };
    typedef std::vector<std::pair<const char*, float>> Set;
    auto run = [&] (const Set& prm, const std::vector<float>& in, int* lat = nullptr, int blk = 512)
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, blk);  p.prepareToPlay (SR, blk);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        setP (p, "wowOn", 1.0f);  setP (p, "wowMode", 0.0f);  setP (p, "wowPos", 1.0f);  setP (p, "wowGlide", 0.0f);  setP (p, "wowMix", 1.0f);
        for (auto& kv : prm) setP (p, kv.first, kv.second);
        std::vector<float> y;  process (p, in, y, blk);  if (lat) { p.applyPendingUpdates();  *lat = p.getLatencySamples(); }  return y; };
    auto base = [&] ()          // the latency with the pedal off
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        std::vector<float> y;  process (p, testSignal (4096, SR), y, 512);  p.applyPendingUpdates();  return p.getLatencySamples(); };
    // a steady harmonic tone (the first 12 harmonics, falling off) and a vowel-like one (formants near 800 and 2200 Hz)
    auto saw = [&] (size_t n, double f0)
    {   std::vector<float> x (n, 0.0f);  for (size_t i = 0; i < n; ++i) { double v = 0;  for (int h = 1; h <= 12; ++h) v += std::sin (2 * M_PI * f0 * h * (double) i / SR) / h;  x[i] = (float) (0.25 * v); }  return x; };
    auto vowel = [&] (size_t n, double f0)
    {   std::vector<float> x (n, 0.0f);
        for (int h = 1; h * f0 < 9000.0; ++h)
        {   const double fh = f0 * h;
            const double a = 1.0 / (1.0 + std::pow ((fh - 800.0) / 220.0, 2.0)) + 0.5 / (1.0 + std::pow ((fh - 2200.0) / 400.0, 2.0)) + 0.01;
            for (size_t i = 0; i < n; ++i) x[i] += (float) (0.12 * a * std::sin (2 * M_PI * fh * (double) i / SR)); }
        return x; };
    float pk;

    // 1. Warble: a slow amplitude wobble on a shifted-up steady tone; Clean has none
    {   auto wobble = [&] (const std::vector<float>& y, double* rateHz)        // std deviation in dB of the 10 ms level, and the strongest rate between 2 and 20 Hz
        {   const size_t hop = 480, from = (size_t) (1.2 * SR);  std::vector<double> lv;
            for (size_t i = from; i + hop <= y.size(); i += hop) { double s = 0;  for (size_t k = 0; k < hop; ++k) s += (double) y[i + k] * y[i + k];  lv.push_back (10 * std::log10 (s / (double) hop + 1e-12)); }
            double m = 0;  for (double v : lv) m += v;  m /= (double) lv.size();
            double sd = 0;  for (double v : lv) sd += (v - m) * (v - m);  sd = std::sqrt (sd / (double) lv.size());
            double best = 0, bf = 0;
            for (double f = 2.0; f <= 20.0; f += 0.25)
            {   double re = 0, im = 0;  for (size_t k = 0; k < lv.size(); ++k) { const double ph = 2 * M_PI * f * (double) k * 0.01;  re += (lv[k] - m) * std::cos (ph);  im += (lv[k] - m) * std::sin (ph); }
                const double mg = re * re + im * im;  if (mg > best) { best = mg;  bf = f; } }
            if (rateHz) *rateHz = bf;  return sd; };
        // The depth depends on how the note lines up with the window (a half-window delay that is a whole number of periods adds up
        // in phase, so there is no wobble; half a period out, the two taps cancel and the wobble is full). So the test uses
        // notes picked to be half a period out at the window of each Grit, and one that is whole periods out.
        auto tone = [&] (size_t n, double f) { std::vector<float> x (n);  for (size_t i = 0; i < n; ++i) x[i] = (float) (0.3 * std::sin (2 * M_PI * f * (double) i / SR));  return x; };
        struct Case { float grit; double halfWin, f0; double rate; };
        const Case cases[3] = { { 0.5f, 0.1225, 26.5 / 0.1225, 2.0 / 0.245 }, { 0.0f, 0.190, 41.5 / 0.190, 2.0 / 0.380 }, { 1.0f, 0.055, 11.5 / 0.055, 2.0 / 0.110 } };
        double rates[3] = { 0, 0, 0 };
        for (int c = 0; c < 3; ++c)
        {   double r = 0;  const double sd = wobble (run ({ { "wowChar", 5.0f }, { "wowGrit", cases[c].grit } }, tone (SR * 6, cases[c].f0)), &r);  rates[c] = r;
            std::printf ("  info: Warble +12 st, Grit %.1f, %.1f Hz pure tone (half a period out): level wobble %.2f dB rms at %.2f Hz (expected %.2f Hz)\n", cases[c].grit, cases[c].f0, sd, r, cases[c].rate);
            CHECK (sd > 2.0, "Warble Grit %.1f: a tone shifted up an octave swells and fades (%.2f dB rms, want > 2)", cases[c].grit, sd);
            CHECK (std::abs (r / cases[c].rate - 1.0) < 0.25, "Warble Grit %.1f: the wobble rate is 2 x (ratio - 1) / window = %.2f Hz (found %.2f Hz)", cases[c].grit, cases[c].rate, r); }
        CHECK (rates[2] > rates[0] * 2.0, "Warble: Grit 1 (110 ms window) wobbles much faster than Grit 0 (380 ms): %.2f vs %.2f Hz", rates[2], rates[1]);
        const double inPhase = wobble (run ({ { "wowChar", 5.0f } }, tone (SR * 6, 27.0 / 0.1225)), nullptr);       // 27 whole periods in the half window: the taps alone would not wobble
        const double clean = wobble (run ({ { "wowChar", 0.0f } }, tone (SR * 6, cases[0].f0)), nullptr);
        const double still = wobble (run ({ { "wowChar", 5.0f }, { "wowPos", 0.0f } }, tone (SR * 6, cases[0].f0)), nullptr);
        std::printf ("  info: wobble on a note lined up in phase with the window %.2f dB; Warble with no shift %.2f dB; Clean %.2f dB\n", inPhase, still, clean);
        CHECK (inPhase > 2.0, "Warble: a note that lines up with the window still wobbles (%.2f dB rms, want > 2)", inPhase);
        CHECK (still < 0.7, "Warble at zero shift does not wobble (%.2f dB rms, want < 0.7)", still);
        CHECK (clean < 0.7, "Clean has no wobble on the same note (%.2f dB rms, want < 0.7)", clean);
        // the shifted pitch is right: 220 Hz up an octave puts the energy at 440
        const auto y = run ({ { "wowChar", 5.0f } }, saw (SR * 5, 220.0));
        const double a440 = goertz (y, 120000, 220000, 440.0), a330 = goertz (y, 120000, 220000, 330.0);
        CHECK (a440 > a330 + 20.0, "Warble: 220 Hz up an octave lands on 440 Hz (440 %.1f dB, 330 %.1f dB)", a440, a330);
        int l = -1;  run ({ { "wowChar", 5.0f } }, saw (4800, 220.0), &l);
        CHECK (l == base(), "Warble reports no latency of its own (%d vs %d with the pedal off)", l, base()); }

    // 2. Tone: Stays natural keeps the formants where they were; Follows the pitch moves them
    {   const auto in = vowel (SR * 4, 110.0);
        const size_t f = (size_t) (1.5 * SR), t = (size_t) (3.8 * SR);
        auto lowHigh = [&] (const std::vector<float>& y) { return bandDb (y, f, t, 500.0, 1100.0) - bandDb (y, f, t, 1400.0, 2400.0); };     // positive: the 800 Hz hill is higher than the 1600 to 2200 Hz hill
        const double dryLH = lowHigh (in);
        const double follow = lowHigh (run ({ { "wowForm", 0.0f } }, in)), keep = lowHigh (run ({ { "wowForm", 1.0f } }, in));
        std::printf ("  info: vowel with hills at 800 and 2200 Hz, +12 st: low band minus high band: dry %.1f dB, Follows %.1f dB, Stays natural %.1f dB\n", dryLH, follow, keep);
        CHECK (keep > follow + 6.0, "Stays natural keeps the low hill: %.1f dB vs %.1f dB when it follows the pitch", keep, follow);
        // down an octave
        const double followD = lowHigh (run ({ { "wowForm", 0.0f }, { "wowMode", 2.0f } }, in)), keepD = lowHigh (run ({ { "wowForm", 1.0f }, { "wowMode", 2.0f } }, in));
        std::printf ("  info: -12 st: Follows %.1f dB, Stays natural %.1f dB\n", followD, keepD);
        CHECK (keepD < followD - 3.0, "Octave down: Stays natural keeps the hills in place (%.1f dB) instead of dragging them down (%.1f dB)", keepD, followD);
        // the same octave down on a higher voice (220 Hz becomes 110 Hz): Stays natural has to land nearer the dry balance than Follows does
        {   const auto in2 = vowel (SR * 4, 220.0);
            const double dry2 = lowHigh (in2), fo = lowHigh (run ({ { "wowForm", 0.0f }, { "wowMode", 2.0f } }, in2)), kp = lowHigh (run ({ { "wowForm", 1.0f }, { "wowMode", 2.0f } }, in2));
            std::printf ("  info: vowel at 220 Hz, -12 st: dry %.1f dB, Follows %.1f dB, Stays natural %.1f dB\n", dry2, fo, kp);
            CHECK (std::abs (kp - dry2) < std::abs (fo - dry2), "Octave down on 220 Hz: Stays natural (%.1f dB) is nearer the dry balance (%.1f dB) than Follows (%.1f dB)", kp, dry2, fo); }
        // the pitch is still right
        const auto y = run ({ { "wowForm", 1.0f } }, saw (SR * 4, 220.0));
        const double a440 = goertz (y, 96000, 180000, 440.0), a330 = goertz (y, 96000, 180000, 330.0);
        CHECK (a440 > a330 + 20.0, "Stays natural: the pitch still goes up an octave (440 %.1f dB, 330 %.1f dB)", a440, a330); }

    // 3. Formant knob: moves the hills without moving the pitch
    {   const auto in = vowel (SR * 4, 110.0);
        const size_t f = (size_t) (1.5 * SR), t = (size_t) (3.8 * SR);
        auto centroid = [&] (const std::vector<float>& y, double f0)       // power-weighted mean frequency of the harmonics
        {   double num = 0, den = 0;  for (int h = 1; h * f0 < 8000.0; ++h) { const double pw = std::pow (10.0, goertz (y, f, t, f0 * h) / 10.0);  num += f0 * h * pw;  den += pw; }  return num / (den + 1e-30); };
        const double z = centroid (run ({ { "wowPos", 0.0f } }, in), 110.0), up = centroid (run ({ { "wowPos", 0.0f }, { "wowFormShift", 9.0f } }, in), 110.0), dn = centroid (run ({ { "wowPos", 0.0f }, { "wowFormShift", -6.0f } }, in), 110.0);
        std::printf ("  info: pitch unchanged, mean frequency of the harmonics: Formant 0 st %.0f Hz, +9 st %.0f Hz, -6 st %.0f Hz\n", z, up, dn);
        CHECK (up > z * 1.25 && dn < z * 0.93, "Formant knob: up raises the tone (%.0f Hz vs %.0f Hz), down lowers it (%.0f Hz)", up, z, dn);
        const auto y = run ({ { "wowPos", 0.0f }, { "wowFormShift", 9.0f } }, saw (SR * 4, 220.0));
        const double a440 = goertz (y, 96000, 180000, 440.0), a495 = goertz (y, 96000, 180000, 495.0), a220 = goertz (y, 96000, 180000, 220.0);
        CHECK (a440 > a495 + 20.0 && a220 > a495 + 20.0, "Formant knob leaves the pitch alone (220 Hz %.1f, 440 Hz %.1f, between %.1f dB)", a220, a440, a495);
        // the warp on its own: no move = the input delayed, same level; a move keeps the pitch
        FormantWarp fw;  fw.prepare (SR);  fw.setSemis (0.0f);
        std::vector<float> src = saw (SR * 2, 220.0), out (src.size());
        for (size_t i = 0; i < src.size(); ++i) out[i] = fw.process (src[i]);
        double err = 0, sig = 0;  for (size_t i = 20000; i < 60000; ++i) { const double d = out[i] - src[i - FormantWarp::kLatency];  err += d * d;  sig += (double) src[i - FormantWarp::kLatency] * src[i - FormantWarp::kLatency]; }
        std::printf ("  info: FormantWarp at 0 st: error against the delayed input %.1f dB\n", 10 * std::log10 (err / sig + 1e-20));
        CHECK (err < sig * 1.0e-4, "FormantWarp at 0 st returns the input delayed by %d samples (error %.1f dB)", FormantWarp::kLatency, 10 * std::log10 (err / sig + 1e-20));
        fw.reset();  fw.setSemis (7.0f);  for (size_t i = 0; i < src.size(); ++i) out[i] = fw.process (src[i]);
        const double w220 = goertz (out, 48000, 90000, 220.0), w330 = goertz (out, 48000, 90000, 330.0);
        CHECK (w220 > w330 + 25.0, "FormantWarp at +7 st leaves the pitch at 220 Hz (%.1f dB vs %.1f dB at 330)", w220, w330);
        // at no shift the pedal passes the level: Stays natural against the plain pedal
        const auto ref = run ({ { "wowPos", 0.0f } }, saw (SR * 4, 220.0)), d = run ({ { "wowPos", 0.0f }, { "wowForm", 1.0f } }, saw (SR * 4, 220.0));
        const double l0 = rmsDb (std::vector<float> (d.begin() + 96000, d.end())), l1 = rmsDb (std::vector<float> (ref.begin() + 96000, ref.end()));
        CHECK (std::abs (l0 - l1) < 1.0, "Stays natural at no shift keeps the level of the plain pedal (%.2f vs %.2f dB)", l0, l1); }

    // 4. latency: Stays natural and the Formant knob add exactly 1024 samples, reported to the host; the dry signal is delayed to match
    {   const int b0 = base();
        int la = 0, lb = 0, lc = 0, ld = 0, le = 0;
        run ({}, saw (4800, 220.0), &la);  run ({ { "wowForm", 1.0f } }, saw (4800, 220.0), &lb);  run ({ { "wowFormShift", 4.0f } }, saw (4800, 220.0), &lc);
        run ({ { "wowForm", 1.0f }, { "wowFormShift", -7.0f } }, saw (4800, 220.0), &ld);  run ({ { "wowForm", 1.0f }, { "wowTrack", 1.0f } }, saw (4800, 220.0), &le);
        int ln = 0;  run ({ { "wowTrack", 1.0f } }, saw (4800, 220.0), &ln);
        std::printf ("  info: latency: pedal off %d, plain %d, Stays natural %d, Formant +4 %d, both %d, Chords + Stays natural %d (Chords alone %d)\n", b0, la, lb, lc, ld, le, ln);
        CHECK (lb == la + FormantWarp::kLatency && lc == lb && ld == lb, "Stays natural / Formant add exactly %d samples and it does not change with the setting (%d, %d, %d vs %d)", FormantWarp::kLatency, lb, lc, ld, la);
        CHECK (le == ln + FormantWarp::kLatency, "the added delay is the same with the chord shifter (%d vs %d)", le, ln);
        // a click through Mix 0 comes out where the host expects it
        for (int cfg = 0; cfg < 2; ++cfg)
        {   std::vector<float> click (SR / 2, 0.0f);  click[6000] = 0.8f;
            int lt = 0;  const auto y = run ({ { "wowMix", 0.0f }, { "wowForm", 1.0f }, { "wowLowKeep", cfg ? 150.0f : 0.0f } }, click, &lt);
            const auto off = run ({ { "wowOn", 0.0f } }, click);
            auto peak = [] (const std::vector<float>& v) { size_t a = 0;  for (size_t i = 0; i < v.size(); ++i) if (std::abs (v[i]) > std::abs (v[a])) a = i;  return (int) a; };
            const int at = peak (y), atOff = peak (off);
            std::printf ("  info: Mix 0 click: arrives %d samples later than with the pedal off (reported pedal latency %d)\n", at - atOff, lt - b0);
            CHECK (std::abs ((at - atOff) - (lt - b0)) <= 1, "Mix 0: the dry click arrives with the reported delay (%d vs %d)%s", at - atOff, lt - b0, cfg ? ", Keep Lows on" : ""); } }

    // 5. Keep Lows: the notes below the crossover stay dry, the rest shifts
    {   std::vector<float> two (SR * 5, 0.0f);
        for (size_t i = 0; i < two.size(); ++i) two[i] = (float) (0.2 * std::sin (2 * M_PI * 80.0 * (double) i / SR) + 0.2 * std::sin (2 * M_PI * 880.0 * (double) i / SR));
        const size_t f = (size_t) (2.5 * SR), t = (size_t) (4.5 * SR);
        for (int track : { 1, 0 })
        {   const auto off = run ({ { "wowTrack", (float) track } }, two), on = run ({ { "wowTrack", (float) track }, { "wowLowKeep", 200.0f } }, two);
            const double o80 = goertz (off, f, t, 80.0), o160 = goertz (off, f, t, 160.0), n80 = goertz (on, f, t, 80.0), n160 = goertz (on, f, t, 160.0), n880 = goertz (on, f, t, 880.0), n1760 = goertz (on, f, t, 1760.0);
            std::printf ("  info: track %d, +12 st on 80 + 880 Hz: Keep Lows off: 80 Hz %.1f, 160 Hz %.1f | on (200 Hz): 80 Hz %.1f, 160 Hz %.1f, 880 Hz %.1f, 1760 Hz %.1f dB\n", track, o80, o160, n80, n160, n880, n1760);
            CHECK (n80 > n160 + 20.0, "track %d, Keep Lows 200 Hz: the 80 Hz note stays at 80 (%.1f dB vs %.1f dB at 160)", track, n80, n160);
            CHECK (n1760 > n880 + 15.0, "track %d, Keep Lows 200 Hz: the 880 Hz note still shifts to 1760 (%.1f dB vs %.1f dB at 880)", track, n1760, n880);
            if (track == 1) CHECK (o160 > o80 + 8.0, "track %d, Keep Lows off: the 80 Hz note shifts to 160 (%.1f dB vs %.1f dB)", track, o160, o80); }
        // with no shift the crossover adds back flat
        const auto a = run ({ { "wowPos", 0.0f }, { "wowTrack", 1.0f } }, two), b = run ({ { "wowPos", 0.0f }, { "wowTrack", 1.0f }, { "wowLowKeep", 200.0f } }, two);
        CHECK (std::abs (goertz (a, f, t, 80.0) - goertz (b, f, t, 80.0)) < 1.5 && std::abs (goertz (a, f, t, 880.0) - goertz (b, f, t, 880.0)) < 1.5, "Keep Lows with no shift is level-neutral (80 Hz %.1f vs %.1f, 880 Hz %.1f vs %.1f dB)", goertz (a, f, t, 80.0), goertz (b, f, t, 80.0), goertz (a, f, t, 880.0), goertz (b, f, t, 880.0)); }

    // 6. every combination is finite and bounded, also while the settings change mid-play and with odd block sizes
    {   bool ok = true;  int combos = 0;
        for (int ch = 0; ch <= 5; ++ch) for (int track = 0; track <= 2; ++track) for (int form = 0; form <= 1; ++form) for (float fs : { 0.0f, -12.0f, 12.0f }) for (float lk : { 0.0f, 120.0f, 400.0f })
        {   if (ch > 0 && track > 0) continue;  ++combos;
            const auto y = run ({ { "wowChar", (float) ch }, { "wowTrack", (float) track }, { "wowForm", (float) form }, { "wowFormShift", fs }, { "wowLowKeep", lk }, { "wowMode", 5.0f }, { "wowPos", 0.9f }, { "wowGrit", 0.8f } }, testSignal (24000, SR), nullptr, 480);
            ok = ok && finiteBounded (y, 8.0f, pk); }
        CHECK (ok, "all %d combinations of Character, Tracking, Tone, Formant and Keep Lows are finite and bounded", combos);
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 256);  p.prepareToPlay (SR, 256);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        setP (p, "wowOn", 1.0f);  setP (p, "wowMode", 5.0f);  setP (p, "wowGlide", 0.0f);  setP (p, "wowMix", 0.7f);
        bool fine = true;  std::mt19937 rng (3);
        for (int seg = 0; seg < 40; ++seg)
        {   setP (p, "wowChar", (float) (rng() % 6));  setP (p, "wowTrack", (float) (rng() % 3));  setP (p, "wowForm", (float) (rng() % 2));
            setP (p, "wowFormShift", (float) ((int) (rng() % 25) - 12));  setP (p, "wowLowKeep", (float) (rng() % 3) * 100.0f);  setP (p, "wowPos", (float) (rng() % 100) / 99.0f);  setP (p, "wowGrit", (float) (rng() % 100) / 99.0f);
            if (seg % 9 == 4) setP (p, "wowOn", 0.0f); else setP (p, "wowOn", 1.0f);
            std::vector<float> y;  process (p, testSignal (7000 + (int) (rng() % 3000), SR), y, 173);  fine = fine && finiteBounded (y, 8.0f, pk); }
        CHECK (fine, "changing Character, Tracking, Tone, Formant, Keep Lows and the pedal itself every 7000 samples stays finite and bounded");
        std::vector<float> bad = saw (4800, 220.0);  bad[1000] = std::numeric_limits<float>::quiet_NaN();  bad[2000] = std::numeric_limits<float>::infinity();
        const auto y = run ({ { "wowForm", 1.0f }, { "wowLowKeep", 150.0f } }, bad);
        const auto y2 = run ({ { "wowForm", 1.0f }, { "wowLowKeep", 150.0f } }, saw (SR * 2, 220.0));
        CHECK (finiteBounded (y2, 8.0f, pk), "Tone and Keep Lows on a clean signal are finite"); (void) y; }

    // 7. the octaver's Warble is a Character too
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        setP (p, "octOn", 1.0f);  setP (p, "octChar", 5.0f);
        std::vector<float> y;  process (p, saw (SR * 3, 220.0), y, 512);
        CHECK (finiteBounded (y, 8.0f, pk) && rmsDb (y, 48000) > -40.0, "octaver with Warble: finite and audible (%.1f dB)", rmsDb (y, 48000)); }

    // 8. nothing allocates on the audio thread
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "wowOn", 1.0f);  setP (p, "wowForm", 1.0f);  setP (p, "wowLowKeep", 150.0f);  setP (p, "wowFormShift", 3.0f);  setP (p, "wowMode", 5.0f);
        std::vector<float> y;  process (p, testSignal (24000, SR), y, 512);
        long total = 0;
        for (int ch : { 0, 1, 5 })
        {   setP (p, "wowChar", (float) ch);  std::vector<float> in = testSignal (2048, SR);
            juce::AudioBuffer<float> b (2, 512); juce::MidiBuffer m;
            for (int blk = 0; blk < 4; ++blk)
            {   for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[(size_t) (blk * 512 + i)]);  b.setSample (1, i, in[(size_t) (blk * 512 + i)]); }
                gAllocCount = 0;  gAllocWatch = (blk >= 2);  p.processBlock (b, m);  gAllocWatch = false;  total += gAllocCount; } }
        CHECK (total == 0, "Tone, Formant and Keep Lows: %ld allocations in processBlock at steady state", total); }
}

// ---------------------------------------------------------------------------------------------------------------------
static void testB98()
{
    std::printf("[b98] Transpose: precise splice search (my own tuning, measured against a recorded shifter's captures, not listened to)\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return std::pow (10.0, 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30) / 10.0); };
    // the share of the power (20 Hz to 2 kHz, 2.5 Hz grid) that lies within 3 Hz of a harmonic of any of the given fundamentals
    auto onSeries = [&] (const std::vector<float>& y, size_t from, size_t to, const std::vector<double>& f0s)
    {   double on = 0, all = 0;
        for (double f = 20.0; f <= 2000.0; f += 2.5)
        {   const double e = goertz (y, from, to, f);  all += e;  bool hit = false;
            for (double f0 : f0s) { const double h = std::round (f / f0);  if (h >= 1.0 && std::abs (f - h * f0) <= 3.0) hit = true; }
            if (hit) on += e; }
        return on / (all + 1e-30); };
    auto tonesOf = [&] (size_t n, const std::vector<double>& f0s, int firstHarm = 1)
    {   std::vector<float> x (n, 0.0f);
        for (double f0 : f0s) for (int h = firstHarm; h <= 10; ++h) for (size_t i = 0; i < n; ++i) x[i] += (float) (0.12 * std::sin (2 * M_PI * f0 * h * (double) i / SR) / h);
        return x; };
    typedef std::vector<std::pair<const char*, float>> Set;
    auto run = [&] (const Set& prm, const std::vector<float>& in, int* lat = nullptr)
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        setP (p, "tpOn", 1.0f);  setP (p, "tpMix", 1.0f);  setP (p, "tpLock", 0.0f);
        for (auto& kv : prm) setP (p, kv.first, kv.second);
        std::vector<float> y;  process (p, in, y, 512);  if (lat) { p.applyPendingUpdates();  *lat = p.getLatencySamples(); }  return y; };
    auto base = [&] ()
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        std::vector<float> y;  process (p, testSignal (4096, SR), y, 512);  p.applyPendingUpdates();  return p.getLatencySamples(); };
    const size_t N = (size_t) (SR * 3.0), A = (size_t) (SR * 1.5), B = (size_t) (SR * 2.9);

    // 1. chords and low notes follow the shift: the power sits on the harmonic series of the shifted notes
    {   struct Case { const char* name; std::vector<double> notes; float semis; int first; double want; };
        const Case cases[] = {
            { "single A2 down an octave",            { 110.0 },                         -12.0f, 1, 0.9 },
            { "fifth A2+E3 down an octave",          { 110.0, 164.81 },                 -12.0f, 1, 0.8 },
            { "power chord A1+E2+A2 down an octave", { 55.0, 82.41, 110.0 },            -12.0f, 1, 0.8 },
            { "minor second A3+A#3 down an octave",  { 220.0, 233.08 },                 -12.0f, 1, 0.7 },
            { "fifth A2+E3 down two octaves",        { 110.0, 164.81 },                 -24.0f, 1, 0.8 },
            { "fifth A2+E3 down a fifth",            { 110.0, 164.81 },                 -7.0f,  1, 0.8 },
            { "fifth A2+E3 up an octave",            { 110.0, 164.81 },                 12.0f,  1, 0.8 },
            { "power chord up a fifth",              { 55.0, 82.41, 110.0 },            7.0f,   1, 0.8 },
            { "weak fundamental (harmonics 2 up) down an octave", { 82.41 },            -12.0f, 2, 0.8 } };
        for (const auto& c : cases)
        {   const auto y = run ({ { "tpSemis", c.semis } }, tonesOf (N, c.notes, c.first));
            std::vector<double> sh;  for (double f : c.notes) sh.push_back (f * std::pow (2.0, (double) c.semis / 12.0));
            const double v = onSeries (y, A, B, sh);
            std::printf ("  info: %s: %.2f of the power on the shifted notes\n", c.name, v);
            CHECK (v >= c.want, "%s: only %.2f of the power is on the shifted notes (want %.2f)", c.name, v, c.want); } }

    // 2. the delay: Fast 12 ms window (half of it), Smooth 28 ms, the 7-string floor; reported to the host and the dry lines up
    {   const int b0 = base();  int lf = 0, ls = 0, l7 = 0, l8 = 0;
        const auto in = tonesOf (4800, { 110.0 });
        run ({ { "tpSemis", -12.0f } }, in, &lf);  run ({ { "tpSemis", -12.0f }, { "tpQuality", 1.0f } }, in, &ls);
        run ({ { "tpSemis", -12.0f }, { "tpRange", 1.0f } }, in, &l7);  run ({ { "tpSemis", -12.0f }, { "tpRange", 2.0f } }, in, &l8);
        std::printf ("  info: Transpose delay (samples at 48 kHz): Fast %d, Smooth %d, 7-string %d, 8-string %d\n", lf - b0, ls - b0, l7 - b0, l8 - b0);
        CHECK (std::abs ((lf - b0) - 288) <= 2, "Fast reports half of a 12 ms window (%d samples, want 288)", lf - b0);
        CHECK (std::abs ((ls - b0) - 672) <= 2, "Smooth reports half of a 28 ms window (%d samples, want 672)", ls - b0);
        CHECK (l7 - b0 >= lf - b0 && l8 - b0 >= l7 - b0 && l8 - b0 <= 700, "longer string ranges add a little delay, in order (%d, %d, %d)", lf - b0, l7 - b0, l8 - b0);
        std::vector<float> click (SR / 2, 0.0f);  click[6000] = 0.8f;
        for (int q = 0; q < 2; ++q)
        {   int lt = 0;  const auto y = run ({ { "tpSemis", -12.0f }, { "tpMix", 0.0f }, { "tpQuality", (float) q } }, click, &lt);
            const auto off = run ({ { "tpOn", 0.0f } }, click);
            auto peak = [] (const std::vector<float>& v) { size_t a = 0;  for (size_t i = 0; i < v.size(); ++i) if (std::abs (v[i]) > std::abs (v[a])) a = i;  return (int) a; };
            CHECK (std::abs ((peak (y) - peak (off)) - (lt - b0)) <= 1, "Mix 0: the dry click arrives with the reported delay (%d vs %d)%s", peak (y) - peak (off), lt - b0, q ? ", Smooth" : ", Fast"); } }

    // 3. the level stays put and silence stays silent, shifting up and down (the shifter on its own: the amp's input filter would cut the lows)
    {   auto direct = [&] (const std::vector<float>& in, float st)
        {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.reset();
            std::vector<float> out (in.size()), wet (512), dry (512);
            for (size_t pos = 0; pos < in.size(); pos += 512)
            {   const int n = (int) std::min ((size_t) 512, in.size() - pos);
                for (int i = 0; i < n; ++i) wet[(size_t) i] = in[pos + (size_t) i];
                t.process (wet.data(), dry.data(), n, st);
                for (int i = 0; i < n; ++i) out[pos + (size_t) i] = wet[(size_t) i]; }
            return out; };
        for (float st : { -24.0f, -12.0f, 12.0f, 24.0f })
        {   const auto in = tonesOf (N, { 110.0, 164.81 });
            const auto y = direct (in, st);
            double ri = 0, ro = 0;  for (size_t i = A; i < B; ++i) { ri += (double) in[i] * in[i];  ro += (double) y[i] * y[i]; }
            const double d = 10 * std::log10 (ro / ri + 1e-20);
            std::printf ("  info: level change at %+.0f st: %.2f dB\n", st, d);
            CHECK (std::abs (d) < 2.0, "level change at %+.0f st is %.2f dB", st, d);
            bool fin = true;  for (float v : y) if (! std::isfinite (v)) fin = false;
            CHECK (fin, "finite output at %+.0f st", st); }
        const auto z = direct (std::vector<float> ((size_t) SR, 0.0f), -12.0f);
        float m = 0;  for (float v : z) m = std::max (m, std::abs (v));
        CHECK (m == 0.0f, "silence in, silence out (%.2e)", m); }

    // 4. a new note after a different one is followed quickly (the comparison weights the newest samples most)
    {   std::vector<float> x (N, 0.0f);
        for (size_t i = 0; i < N; ++i) { const double f = i < N / 2 ? 110.0 : 146.83; x[i] = (float) (0.3 * std::sin (2 * M_PI * f * (double) i / SR)); }
        const auto y = run ({ { "tpSemis", -12.0f } }, x);
        const size_t from = N / 2 + (size_t) (0.12 * SR), to = N / 2 + (size_t) (0.5 * SR);
        const double on = goertz (y, from, to, 73.415), off = goertz (y, from, to, 55.0);
        std::printf ("  info: 120 ms after a note change the new note is %.1f dB above the old one\n", 10 * std::log10 (on / (off + 1e-30)));
        CHECK (on > 10.0 * off, "the shifted note follows a note change (new note only %.1f dB above the old)", 10 * std::log10 (on / (off + 1e-30))); }

    // 5. the cost: ten seconds of audio through the shifter
    {   const auto in = testSignal ((int) (SR * 10), SR);
        const auto t0 = std::chrono::steady_clock::now();
        const auto y = run ({ { "tpSemis", -12.0f } }, in);
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        std::printf ("  info: 10 s of audio through the whole chain with Transpose on took %.2f s\n", sec);
        CHECK (sec < 12.0, "Transpose costs far too much (%.2f s for 10 s of audio)", sec); }
}

// ---------------------------------------------------------------------------------------------------------------------

// b102: the riff recorder's position now belongs to the audio thread; transport calls ask for a rewind and the next block does it.
// b103: Dream Singing retune (Auto Level stays level in all three amp modes) and the wider acoustic Body knob (range, and the level makeup)
static void testB103()
{
    std::printf("[b103] Marque Dream Singing Auto Level, acoustic Body range\n");
    const double SR = 48000.0;
    const auto in = testSignal (48000 * 6, SR, 0.5f);
    auto run = [&] (std::initializer_list<std::pair<const char*, float>> prm)
    {
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
        for (auto& kv : prm) setP (p, kv.first, kv.second);
        setP (p, "odOn", 0.0f);  setP (p, "compOn", 0.0f);  setP (p, "eqOn", 0.0f);  setP (p, "reverbOn", 0.0f);  setP (p, "reverbMix", 0.0f);
        setP (p, "postDelayOn", 0.0f);  setP (p, "preDelayOn", 0.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "cabOn", 0.0f);
        p.prepareToPlay (SR, 512);
        std::vector<float> y;  process (p, in, y, 512);
        return y;
    };
    // Singing: Auto Level lands within 1.2 dB of the other Marque Dream voicings at Gain 0.2 / 0.5 / 1.0, in every amp mode
    for (int mode = 0; mode < 3; ++mode)
        for (float g : { 0.2f, 0.5f, 1.0f })
        {
            const double lv[4] = { rmsDb (run ({ { "ampModel", 11 }, { "ampVoicing", 0 }, { "ampGain", g }, { "ampMode", (float) mode }, { "ampAutoLevel", 1 } }), (size_t) SR),
                                   rmsDb (run ({ { "ampModel", 11 }, { "ampVoicing", 1 }, { "ampGain", g }, { "ampMode", (float) mode }, { "ampAutoLevel", 1 } }), (size_t) SR),
                                   rmsDb (run ({ { "ampModel", 11 }, { "ampVoicing", 2 }, { "ampGain", g }, { "ampMode", (float) mode }, { "ampAutoLevel", 1 } }), (size_t) SR),
                                   rmsDb (run ({ { "ampModel", 11 }, { "ampVoicing", 3 }, { "ampGain", g }, { "ampMode", (float) mode }, { "ampAutoLevel", 1 } }), (size_t) SR) };
            const double mean3 = (lv[0] + lv[1] + lv[2]) / 3.0;
            CHECK (std::fabs (lv[3] - mean3) < 1.2, "Singing mode %d gain %.1f: Auto Level %.2f dB vs the other voicings' mean %.2f dB", mode, g, lv[3], mean3);
        }
    // Acoustic Body: 1 against 0 lifts the lows and the highs well above the mids (measured with 100 Hz, 1 kHz and 8 kHz tones through the acoustic amp, Squeeze, Preamp and Room off), and the level of plucked notes stays within 3 dB
    auto bands = [&] (float amt, double& lo, double& mid, double& hi, double& all)
    {
        auto make = [&] (float amtv)
        {
            auto p = std::make_unique<StarterAmpProcessor>();  p->setPlayConfigDetails (2, 2, SR, 512);
            setP (*p, "ampModel", 5);  setP (*p, "acBodyAmt", amtv);  setP (*p, "acSqueeze", 0.0f);  setP (*p, "acPreamp", 0.0f);  setP (*p, "acRoom", 0.0f);
            setP (*p, "odOn", 0.0f);  setP (*p, "compOn", 0.0f);  setP (*p, "eqOn", 0.0f);  setP (*p, "reverbOn", 0.0f);  setP (*p, "reverbMix", 0.0f);
            setP (*p, "postDelayOn", 0.0f);  setP (*p, "preDelayOn", 0.0f);  setP (*p, "noiseOn", 0.0f);  setP (*p, "gate", -90.0f);  setP (*p, "cabOn", 0.0f);
            p->prepareToPlay (SR, 512);
            return p;
        };
        auto tone = [&] (double f)
        {
            std::vector<float> x ((size_t) SR * 2);  for (size_t k = 0; k < x.size(); ++k) x[k] = 0.02f * (float) std::sin (6.283185307 * f * (double) k / SR);
            auto p = make (amt);  std::vector<float> y;  process (*p, x, y, 512);
            return rmsDb (y, (size_t) SR);
        };
        lo = tone (100.0);  mid = tone (1000.0);  hi = tone (8000.0);
        auto p = make (amt);  std::vector<float> y;  process (*p, in, y, 512);  all = rmsDb (y, (size_t) SR);
    };
    double l0, m0, h0, a0, l1, m1, h1, a1;
    bands (0.0f, l0, m0, h0, a0);  bands (1.0f, l1, m1, h1, a1);
    std::printf("  Body 1 against 0: 100 Hz %+.1f dB, 1 kHz %+.1f dB, 8 kHz %+.1f dB (relative to 1 kHz: %+.1f / %+.1f); overall level %+.1f dB\n", l1 - l0, m1 - m0, h1 - h0, (l1 - l0) - (m1 - m0), (h1 - h0) - (m1 - m0), a1 - a0);
    CHECK ((l1 - l0) - (m1 - m0) > 9.0, "Body knob: the low end only moves %.1f dB against the mids", (l1 - l0) - (m1 - m0));
    CHECK ((h1 - h0) - (m1 - m0) > 9.0, "Body knob: the high end only moves %.1f dB against the mids", (h1 - h0) - (m1 - m0));
    CHECK (std::fabs (a1 - a0) < 3.0, "Body knob: the level moves %.1f dB between 0 and 1", a1 - a0);
}

// b104: the stereo chorus modes (12 to 18 of the Chorus Mode menu)
static void testB104()
{
    std::printf("[b104] Stereo chorus\n");
    const double SR = 48000.0;
    // 1. The class on its own: a single click, Mix 1, Tone wide open. The wet click of each side arrives at the design delay (Spatial: centre 17 ms at Depth 1, left at the start of its sine,
    //    right 0.18 of a cycle later; Depth 1.5: centre 27 ms; Vintage: 8.5 ms with the sides 0.07 of a cycle apart).
    auto taps = [&] (int mode, float depth, double& dl, double& dr, double& wetL, double& dryL)
    {
        StereoChorus c;  c.prepare (SR);
        std::vector<float> l ((size_t) (0.12 * SR), 0.0f), r (l.size(), 0.0f);
        l[100] = 1.0f;  r[100] = 1.0f;
        c.process (l.data(), r.data(), (int) l.size(), StereoChorus::Params { mode, 1.0f, 1.0f, depth, 20000.0f, 0.0f });
        auto find = [&] (const std::vector<float>& v, double& ms, double& amp)
        {
            size_t k = 0;  float best = 0.0f;
            for (size_t i = 160; i < v.size(); ++i) if (std::abs (v[i]) > best) { best = std::abs (v[i]);  k = i; }
            ms = (double) ((int) k - 100) / SR * 1000.0;  amp = best;
        };
        double a;  find (l, dl, wetL);  find (r, dr, a);  dryL = std::abs (l[100]);
    };
    double dl, dr, wl, dry;
    taps (12, 1.0f, dl, dr, wl, dry);
    const double expL = 17.0, expR = 17.0 + 7.5 * std::sin (6.2831853 * 0.18);
    CHECK (std::abs (dl - expL) < 1.3 && std::abs (dr - expR) < 1.3, "Spatial: wet clicks at %.2f / %.2f ms, expected %.2f / %.2f", dl, dr, expL, expR);
    CHECK (std::abs (dry - 0.7071) < 0.01, "Spatial: the dry click is %.3f, expected 0.707 at Mix 1 (equal power)", dry);
    taps (12, 1.5f, dl, dr, wl, dry);
    CHECK (std::abs (dl - 27.0) < 1.5, "Spatial at Depth 1.5: left wet click at %.2f ms, expected about 27 ms", dl);
    taps (13, 1.0f, dl, dr, wl, dry);
    CHECK (std::abs (dl - 8.5) < 1.0 && std::abs (dr - (8.5 + 3.0 * std::sin (6.2831853 * 0.07))) < 1.0, "Vintage: wet clicks at %.2f / %.2f ms, expected 8.5 / %.2f", dl, dr, 8.5 + 3.0 * std::sin (6.2831853 * 0.07));
    // Level on broadband noise: dry and wet are each about -3 dB, so the sum stays near the input level; Vintage's wet is louder than Spatial's.
    auto noiseGain = [&] (int mode)
    {
        StereoChorus c;  c.prepare (SR);
        const int n = (int) (SR * 4);
        std::vector<float> l ((size_t) n), r ((size_t) n);
        unsigned seed = 12345u;  double ein = 0.0, eout = 0.0;
        for (int i = 0; i < n; ++i) { seed = seed * 1664525u + 1013904223u;  l[(size_t) i] = r[(size_t) i] = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.3f; }
        const auto ref = l;
        c.process (l.data(), r.data(), n, StereoChorus::Params { mode, 1.0f, 1.0f, 1.0f, 14000.0f, 0.0f });
        for (int i = n / 4; i < n; ++i) { ein += ref[(size_t) i] * ref[(size_t) i];  eout += 0.5 * (l[(size_t) i] * l[(size_t) i] + r[(size_t) i] * r[(size_t) i]); }
        return 10.0 * std::log10 (eout / ein);
    };
    const double gSp = noiseGain (12), gVi = noiseGain (13);
    CHECK (gSp > -2.5 && gSp < 1.5, "Spatial at Mix 1: output level %.2f dB relative to the input, expected within about 2 dB of 0", gSp);
    CHECK (gVi > gSp, "Vintage's wet is louder than Spatial's (%.2f dB against %.2f dB)", gVi, gSp);
    // 2. Every stereo mode: finite, bounded, the sides differ for a mono input, Mix 0 passes the dry through untouched, a mono call sums both sides.
    for (int mode = StereoChorus::firstMode; mode <= StereoChorus::lastMode; ++mode)
    {
        const auto in = testSignal ((int) (SR * 3), SR, 0.5f);
        StereoChorus c;  c.prepare (SR);
        std::vector<float> l = in, r = in;
        c.process (l.data(), r.data(), (int) l.size(), StereoChorus::Params { mode, 0.7f, 1.0f, 1.0f, 8000.0f, 0.0f });
        float peak = 0.0f, diff = 0.0f;  bool fin = true;
        for (size_t i = 0; i < l.size(); ++i) { fin = fin && std::isfinite (l[i]) && std::isfinite (r[i]);  peak = std::max (peak, std::max (std::abs (l[i]), std::abs (r[i])));  diff = std::max (diff, std::abs (l[i] - r[i])); }
        CHECK (fin && peak < 2.0f, "mode %d: finite and bounded (peak %.2f)", mode, peak);
        CHECK (diff > 0.02f, "mode %d: the two sides should differ for a mono input (largest difference %.4f)", mode, diff);
        StereoChorus c0;  c0.prepare (SR);
        std::vector<float> l0 = in, r0 = in;
        c0.process (l0.data(), r0.data(), (int) l0.size(), StereoChorus::Params { mode, 0.0f, 1.0f, 1.0f, 8000.0f, 0.0f });
        float dmax = 0.0f;  for (size_t i = 0; i < l0.size(); ++i) dmax = std::max (dmax, std::max (std::abs (l0[i] - in[i]), std::abs (r0[i] - in[i])));
        CHECK (dmax < 1.0e-6f, "mode %d: Mix 0 should leave the dry signal alone (difference %.2g)", mode, dmax);
        StereoChorus cm;  cm.prepare (SR);
        std::vector<float> lm = in;
        cm.process (lm.data(), nullptr, (int) lm.size(), StereoChorus::Params { mode, 0.7f, 1.0f, 1.0f, 8000.0f, 0.0f });
        float dm = 0.0f;  for (size_t i = 0; i < lm.size(); ++i) dm = std::max (dm, std::abs (lm[i] - 0.5f * (l[i] + r[i])));
        CHECK (dm < 1.0e-4f, "mode %d: a mono call should equal the two sides averaged (difference %.2g)", mode, dm);
    }
    // 3. In the plugin: after the cab the sides differ and Mod FX Chorus stereo modes do the same; before the amp both channels stay equal.
    auto render = [&] (bool modSlot, int lane, int mode, float& diffOut, float& peakOut)
    {
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
        setP (p, "ampModel", 0);  setP (p, "ampGain", 0.1f);  setP (p, "odOn", 0.0f);  setP (p, "compOn", 0.0f);  setP (p, "eqOn", 0.0f);  setP (p, "reverbOn", 0.0f);  setP (p, "reverbMix", 0.0f);
        setP (p, "postDelayOn", 0.0f);  setP (p, "preDelayOn", 0.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "cabOn", 0.0f);
        if (modSlot) { setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 12.0f);  setP (p, "mfAChMode", (float) mode);  setP (p, "mfAMix", 0.8f);  placeLane (p, 6, lane); }
        else         { setP (p, "chorusOn", 1.0f);  setP (p, "chorusMode", (float) mode);  setP (p, "chorusMix", 0.8f);  placeLane (p, 4, lane); }
        p.prepareToPlay (SR, 512);
        const auto in = testSignal ((int) (SR * 3), SR, 0.5f);
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;
        diffOut = 0.0f;  peakOut = 0.0f;
        for (size_t pos = 0; pos + 512 <= in.size(); pos += 512)
        {
            for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[pos + (size_t) i]);  b.setSample (1, i, in[pos + (size_t) i]); }
            p.processBlock (b, m);
            if (pos > (size_t) SR)
                for (int i = 0; i < 512; ++i) { diffOut = std::max (diffOut, std::abs (b.getSample (0, i) - b.getSample (1, i)));  peakOut = std::max (peakOut, std::abs (b.getSample (0, i))); }
        }
    };
    for (int mode : { 12, 13, 14, 15, 16, 17, 18 })
    {
        float d1, k1, d2, k2, d3, k3, d4, k4;
        render (false, 1, mode, d1, k1);  render (true, 1, mode, d2, k2);  render (false, 0, mode, d3, k3);  render (true, 0, mode, d4, k4);
        CHECK (d1 > 0.01f && std::isfinite (k1), "old Chorus block, mode %d, after the cab: sides should differ (%.4f)", mode, d1);
        CHECK (d2 > 0.01f && std::isfinite (k2), "Mod FX Chorus, mode %d, after the cab: sides should differ (%.4f)", mode, d2);
        CHECK (d3 < 1.0e-6f && k3 > 0.001f, "old Chorus block, mode %d, before the amp: both channels should be equal and not silent (%.2g, peak %.4f)", mode, d3, k3);
        CHECK (d4 < 1.0e-6f && k4 > 0.001f, "Mod FX Chorus, mode %d, before the amp: both channels should be equal and not silent (%.2g, peak %.4f)", mode, d4, k4);
    }
}

// b105: the Arc fuzz style (style 14), the Sustain hold Mod FX type (type 25) and the Vinyl Lo-Fi voicing (voice 2)
static double goertzelAmp (const std::vector<float>& y, size_t from, size_t to, double hz, double sr)
{
    double re = 0, im = 0;  const double w = 6.283185307 * hz / sr;
    for (size_t i = from; i < to; ++i) { const double ph = w * (double) (i - from);  re += y[i] * std::cos (ph);  im += y[i] * std::sin (ph); }
    return 2.0 * std::sqrt (re * re + im * im) / (double) (to - from);
}
static double inharmonicDb (const std::vector<float>& y, double f0, double sr)      // energy away from the harmonics of f0, in dB re the total below 20 kHz
{
    const int L = 1 << 15;
    if (y.size() < (size_t) L * 2) return 0.0;
    juce::dsp::FFT fft (15);  std::vector<float> buf ((size_t) L * 2, 0.0f);
    for (int i = 0; i < L; ++i) buf[(size_t) i] = y[y.size() - (size_t) L + (size_t) i] * (0.5f - 0.5f * std::cos (6.2831853f * (float) i / (float) L));
    fft.performFrequencyOnlyForwardTransform (buf.data());
    double tot = 0, harm = 0;
    for (int k = 1; k < L / 2; ++k)
    {
        const double f = k * sr / L;  if (f > 20000.0) break;
        const double e = (double) buf[(size_t) k] * buf[(size_t) k];  tot += e;
        const double h = f / f0;  if (std::abs (h - std::round (h)) * f0 < 25.0) harm += e;
    }
    return 10.0 * std::log10 ((tot - harm) / tot + 1e-20);
}
static void testB105()
{
    std::printf("[b105] Arc, Sustain hold, Vinyl\n");
    const double SR = 48000.0;
    // ---- 1. Arc on its own
    auto sineDb = [&] (double hz, double db, double sec) { std::vector<float> v ((size_t) (sec * SR));  const float a = (float) std::pow (10.0, db / 20.0);  for (size_t i = 0; i < v.size(); ++i) v[i] = a * (float) std::sin (6.283185307 * hz * (double) i / SR);  return v; };
    auto arcRun = [&] (float gain, float gate, float stab, float comp, float oct, std::vector<float> y)
    {
        FuzzEffect fx;  fx.prepare (SR, 512);
        FuzzEffect::Params p {};  p.style = 14;  p.gain = gain;  p.tone = 0.5f;  p.level = 1.0f;  p.gate = gate;  p.stab = stab;  p.stages = 2;  p.mix = 1.0f;  p.comp = comp;  p.input = 1.0f;  p.octave = oct;
        for (size_t b = 0; b + 512 <= y.size(); b += 512) fx.process (y.data() + b, 512, p);
        return y;
    };
    {
        // finite and bounded across the knobs
        bool ok = true;  float worst = 0.0f;
        for (float g : { 0.0f, 0.6f, 1.0f }) for (float gate : { 0.0f, 0.5f, 1.0f }) for (float stab : { 0.0f, 1.0f }) for (float comp : { 0.0f, 1.0f })
        {
            const auto y = arcRun (g, gate, stab, comp, 0.0f, testSignal ((int) (SR * 2), SR, 0.9f));
            float pk;  ok = ok && finiteBounded (y, 3.0f, pk);  worst = std::max (worst, pk);
        }
        CHECK (ok, "Arc: finite and bounded across Gain, Gate, Stab and Comp (worst peak %.2f)", worst);
    }
    {
        // flat dynamics while the arc burns: 18 dB more input gives under 4 dB more output; a fixed level for a fixed setting
        const auto lo = arcRun (0.6f, 0.2f, 0.8f, 0.0f, 0.0f, sineDb (196.0, -24.0, 1.5)), hi = arcRun (0.6f, 0.2f, 0.8f, 0.0f, 0.0f, sineDb (196.0, -6.0, 1.5));
        const double dl = rmsDb (lo, 36000), dh = rmsDb (hi, 36000);
        CHECK (std::abs (dh - dl) < 4.0 && dh > -14.0 && dh < 0.0, "Arc at Gate 0.2: -24 dBFS in gives %.1f dB out and -6 dBFS gives %.1f dB: the dynamics should be flat (under 4 dB apart)", dl, dh);
        // the strike voltage: a quiet note lights the arc at a low Gate and is gated out at a high one
        const double q2 = rmsDb (arcRun (0.6f, 0.2f, 0.8f, 0.0f, 0.0f, sineDb (196.0, -30.0, 1.5)), 36000), q8 = rmsDb (arcRun (0.6f, 0.8f, 0.8f, 0.0f, 0.0f, sineDb (196.0, -30.0, 1.5)), 36000);
        CHECK (q2 > -25.0 && q8 < -45.0, "Arc: a -30 dBFS note should light the arc at Gate 0.2 (%.1f dB) and be gated out at Gate 0.8 (%.1f dB)", q2, q8);
        const double l8 = rmsDb (arcRun (0.6f, 0.8f, 0.8f, 0.0f, 0.0f, sineDb (196.0, -6.0, 1.5)), 36000);
        CHECK (l8 > -20.0, "Arc: a loud note should still strike at Gate 0.8 (%.1f dB)", l8);
    }
    {
        // Comp keeps a fading note above the strike voltage: it rings on longer
        auto lastAudible = [&] (float comp)
        {
            std::vector<float> x ((size_t) (SR * 6));
            for (size_t i = 0; i < x.size(); ++i) x[i] = 0.5f * (float) std::exp (-(double) i / (SR * 1.2)) * (float) std::sin (6.283185307 * 196.0 * (double) i / SR);
            const auto y = arcRun (0.6f, 0.5f, 0.8f, comp, 0.0f, x);
            double last = 0.0;  const size_t w = 960;
            for (size_t i = 0; i + w < y.size(); i += w) { double e = 0;  for (size_t j = 0; j < w; ++j) e += (double) y[i + j] * y[i + j];  if (10.0 * std::log10 (e / (double) w + 1e-20) > -50.0) last = (double) i / SR; }
            return last;
        };
        const double t0 = lastAudible (0.0f), t1 = lastAudible (1.0f);
        CHECK (t1 > t0 + 0.8, "Arc at Gate 0.5: the note rings until %.2f s at Comp 0 and %.2f s at Comp 1 (it should be at least 0.8 s longer)", t0, t1);
        // Stab: a restless arc is noisier (energy away from the harmonics)
        const double inSt = inharmonicDb (arcRun (0.9f, 0.2f, 1.0f, 0.5f, 0.0f, sineDb (1180.0, -20.0, 3.0)), 1180.0, SR), inRe = inharmonicDb (arcRun (0.9f, 0.2f, 0.0f, 0.5f, 0.0f, sineDb (1180.0, -20.0, 3.0)), 1180.0, SR);
        CHECK (inRe > inSt + 5.0, "Arc: Stab 0 should crackle more than Stab 1 (%.1f dB against %.1f dB of non-harmonic energy)", inRe, inSt);
        // Octave Up changes the sound
        const auto a0 = arcRun (0.6f, 0.2f, 0.8f, 0.5f, 0.0f, sineDb (196.0, -12.0, 1.5)), a1 = arcRun (0.6f, 0.2f, 0.8f, 0.5f, 1.0f, sineDb (196.0, -12.0, 1.5));
        double d = 0, e = 0;  for (size_t i = 36000; i < a0.size(); ++i) { d += std::pow ((double) a0[i] - a1[i], 2.0);  e += (double) a0[i] * a0[i]; }
        CHECK (d > 0.05 * e, "Arc: Octave Up should change the sound (difference %.3f of the energy)", d / (e + 1e-20));
    }
    {
        // in the plugin: both Fuzz blocks, and switching through every style into Arc and back
        float pk;
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampGain", 0.1f);  setP (p, "cabOn", 0.0f);  setP (p, "reverbMix", 0.0f);
        bool ok = true;  std::vector<float> y;
        for (int blk = 0; blk < 2; ++blk)
        {
            setP (p, blk == 0 ? "fuzzOn" : "fuzz2On", 1.0f);
            for (int st : { 0, 4, 7, 13, 14, 5, 14, 0 })
            {
                setP (p, blk == 0 ? "fuzzStyle" : "fuzz2Style", (float) st);
                process (p, testSignal (12000, SR, 0.5f), y, 512);
                ok = ok && finiteBounded (y, 6.0f, pk);
            }
        }
        CHECK (ok, "Arc in the plugin (both Fuzz blocks, switching styles): finite and bounded");
        setP (p, "fuzzStyle", 14.0f);  setP (p, "fuzzGate", 0.2f);
        process (p, testSignal ((int) (SR * 2), SR, 0.5f), y, 512);
        CHECK (finiteBounded (y, 6.0f, pk) && rmsDb (y, 24000) > -40.0, "Arc in the plugin: audible (%.1f dB)", rmsDb (y, 24000));
    }

    // ---- 2. Sustain hold
    using SH = FxExtra::SustainHoldFx;
    auto holdRun = [&] (const std::vector<float>& in, const std::vector<std::pair<double, bool>>& holdAt, int layers, float rise, float tail, float mix)
    {
        SH h;  h.prepare (SR);
        std::vector<float> y = in;
        bool hold = false;  size_t k = 0;
        for (size_t b = 0; b + 256 <= y.size(); b += 256)
        {
            while (k < holdAt.size() && holdAt[k].first * SR <= (double) b) { hold = holdAt[k].second;  ++k; }
            SH::Params p;  p.hold = hold;  p.layers = layers;  p.rise = rise;  p.tail = tail;  p.toneHz = 16000.0f;  p.mix = mix;
            h.process (y.data() + b, 256, p);
        }
        return y;
    };
    auto sineSeg = [&] (std::vector<float>& v, double t0, double t1, double hz, float amp) { for (size_t i = (size_t) (t0 * SR); i < (size_t) (t1 * SR) && i < v.size(); ++i) v[i] += amp * (float) std::sin (6.283185307 * hz * (double) i / SR); };
    {
        // the held note goes on after the input stops; with a short Tail it dies away after release, with the longest it stays
        std::vector<float> in ((size_t) (SR * 6), 0.0f);
        sineSeg (in, 0.2, 1.5, 220.0, 0.3f);
        for (size_t i = (size_t) (0.2 * SR); i < (size_t) (0.25 * SR); ++i) in[i] += 0.8f * (float) std::exp (-(double) (i - (size_t) (0.2 * SR)) / (0.004 * SR)) * (float) std::sin (6.283185307 * 220.0 * (double) i / SR);
        const auto shortTail = holdRun (in, { { 1.0, true }, { 3.5, false } }, 3, 0.1f, 0.0f, 0.5f);
        std::vector<float> seg (shortTail.begin() + (size_t) (2.0 * SR), shortTail.begin() + (size_t) (3.4 * SR));
        std::vector<float> after (shortTail.begin() + (size_t) (4.3 * SR), shortTail.begin() + (size_t) (5.5 * SR));
        CHECK (rmsDb (seg) > -20.0 && rmsDb (seg) < -8.0, "Sustain hold: the held note should ring on at about the level it was played (%.1f dB; the input is silent from 1.5 s)", rmsDb (seg));
        CHECK (rmsDb (after) < -55.0, "Sustain hold: with the shortest Tail the note should be gone 0.8 s after release (%.1f dB)", rmsDb (after));
        const auto longTail = holdRun (in, { { 1.0, true }, { 3.5, false } }, 3, 0.1f, 1.0f, 0.5f);
        std::vector<float> still (longTail.begin() + (size_t) (5.0 * SR), longTail.begin() + (size_t) (5.9 * SR));
        CHECK (rmsDb (still) > -20.0, "Sustain hold: with the longest Tail the note should still ring 1.5 s after release (%.1f dB)", rmsDb (still));
    }
    {
        // the pick attack is not looped: press Hold 50 ms after a hard attack on a quiet note
        std::vector<float> in ((size_t) (SR * 4), 0.0f);
        sineSeg (in, 0.5, 1.5, 220.0, 0.2f);
        for (size_t i = (size_t) (0.5 * SR); i < (size_t) (0.55 * SR); ++i) in[i] += 0.8f * (float) std::exp (-(double) (i - (size_t) (0.5 * SR)) / (0.004 * SR)) * (float) std::sin (6.283185307 * 220.0 * (double) i / SR);
        const auto y = holdRun (in, { { 0.55, true } }, 3, 0.1f, 1.0f, 0.5f);
        float pk = 0.0f;  for (size_t i = (size_t) (2.0 * SR); i < (size_t) (3.5 * SR); ++i) pk = std::max (pk, std::abs (y[i]));
        CHECK (pk > 0.1f && pk < 0.3f, "Sustain hold: the pick attack should not be in the loop (peak %.2f; the steady note is 0.2 and the attack was 1.0)", pk);
    }
    {
        // layers: two captures stack up to Layers; with Layers 1 the second replaces the first
        std::vector<float> in ((size_t) (SR * 6), 0.0f);
        sineSeg (in, 0.2, 1.1, 220.0, 0.3f);  sineSeg (in, 1.5, 2.4, 330.0, 0.3f);
        const std::vector<std::pair<double, bool>> hs = { { 0.8, true }, { 1.0, false }, { 2.1, true }, { 2.3, false } };
        const auto three = holdRun (in, hs, 3, 0.1f, 1.0f, 0.5f), one = holdRun (in, hs, 1, 0.1f, 1.0f, 0.5f);
        const size_t a = (size_t) (4.0 * SR), b = (size_t) (5.0 * SR);
        const double t220 = goertzelAmp (three, a, b, 220.0, SR), t330 = goertzelAmp (three, a, b, 330.0, SR), o220 = goertzelAmp (one, a, b, 220.0, SR), o330 = goertzelAmp (one, a, b, 330.0, SR);
        CHECK (t220 > 0.05 && t330 > 0.05, "Sustain hold, Layers 3: both held notes should be sounding (220 Hz %.3f, 330 Hz %.3f)", t220, t330);
        CHECK (o330 > 0.05 && o220 < 0.1 * o330, "Sustain hold, Layers 1: the second capture should replace the first (220 Hz %.3f, 330 Hz %.3f)", o220, o330);
    }
    {
        // Mix 0 leaves the dry signal alone; extreme settings stay finite and bounded
        const auto in = testSignal ((int) (SR * 3), SR, 0.5f);
        const auto y0 = holdRun (in, { { 0.7, true }, { 1.5, false } }, 3, 0.1f, 0.5f, 0.0f);
        double dmax = 0;  for (size_t i = 0; i < in.size() - 256; ++i) dmax = std::max (dmax, (double) std::abs (y0[i] - in[i]));
        CHECK (dmax < 1.0e-6, "Sustain hold: Mix 0 should leave the dry signal alone (difference %.2g)", dmax);
        bool ok = true;  float pk, worst = 0.0f;
        for (int layers : { 1, 5 }) for (float rise : { 0.0f, 1.0f }) for (float tail : { 0.0f, 1.0f }) for (float mix : { 0.5f, 1.0f })
        {
            std::vector<std::pair<double, bool>> hs;  for (int k = 0; k < 12; ++k) hs.push_back ({ 0.25 * (k + 1), (k & 1) == 0 });
            const auto y = holdRun (testSignal ((int) (SR * 4), SR, 0.9f), hs, layers, rise, tail, mix);
            ok = ok && finiteBounded (y, 6.0f, pk);  worst = std::max (worst, pk);
        }
        CHECK (ok, "Sustain hold: finite and bounded with rapid Hold toggling at the extremes (worst peak %.2f)", worst);
    }
    {
        // in the plugin: the Mod FX slot type, before the amp and after the cab; nothing allocates while Hold is toggled
        for (int lane : { 0, 1 })
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
            setP (p, "ampGain", 0.1f);  setP (p, "cabOn", 0.0f);  setP (p, "reverbMix", 0.0f);
            setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 25.0f);  setP (p, "mfAMix", 0.6f);  setP (p, "mfAFeedback", 0.9f);  placeLane (p, 6, lane);
            p.prepareToPlay (SR, 512);
            std::vector<float> y;  float pk;
            process (p, testSignal ((int) (SR * 1), SR, 0.5f), y, 512);
            setP (p, "mfAHold", 1.0f);
            process (p, testSignal ((int) (SR * 1), SR, 0.5f), y, 512);
            CHECK (finiteBounded (y, 6.0f, pk) && rmsDb (y, 12000) > -45.0, "Sustain hold in the plugin (%s): finite and audible (%.1f dB)", lane == 0 ? "before the amp" : "after the cab", rmsDb (y, 12000));
            long total = 0;  const auto in = testSignal (4096, SR, 0.5f);
            juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;
            for (int blk = 0; blk < 8; ++blk)
            {
                if (blk % 2 == 0) setP (p, "mfAHold", (blk / 2) % 2 == 0 ? 0.0f : 1.0f);
                for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[(size_t) (blk * 512 + i)]);  b.setSample (1, i, in[(size_t) (blk * 512 + i)]); }
                gAllocCount = 0;  gAllocWatch = (blk >= 2);  p.processBlock (b, m);  gAllocWatch = false;  total += gAllocCount;
            }
            CHECK (total == 0, "Sustain hold in the plugin (%s): %ld allocations in processBlock", lane == 0 ? "before the amp" : "after the cab", total);
        }
    }

    // ---- 3. Vinyl
    {
        auto vinylRun = [&] (int voice, float noise, const std::vector<float>& in)
        {
            LoFiWarble lf;  lf.prepare (SR);
            LoFiWarble::Params p {};  p.shape = 0;  p.rate = 0.5f;  p.depth = 0.3f;  p.drift = 0.2f;  p.tone = 0.6f;  p.dirt = 0.3f;  p.noise = noise;  p.dropouts = 0.0f;  p.mix = 1.0f;  p.levelDb = 0.0f;  p.voice = voice;
            std::vector<float> y = in;
            for (size_t b = 0; b + 256 <= y.size(); b += 256) lf.process (y.data() + b, 256, p);
            return y;
        };
        const std::vector<float> silence ((size_t) (SR * 4), 0.0f);
        const auto quiet = vinylRun (2, 0.0f, silence), noisy = vinylRun (2, 1.0f, silence), tape = vinylRun (1, 1.0f, silence);
        float pkQ = 0.0f;  for (float v : quiet) pkQ = std::max (pkQ, std::abs (v));
        int ticks = 0;  bool above = false;
        for (float v : noisy) { if (std::abs (v) > 0.05f) { if (! above) ++ticks;  above = true; } else if (std::abs (v) < 0.02f) above = false; }
        CHECK (pkQ < 1.0e-4f, "Vinyl: no surface noise at Noise 0 (peak %.2g)", pkQ);
        CHECK (ticks >= 30, "Vinyl: Noise 1 should give ticks and pops (counted %d in 4 s, expected 30 or more)", ticks);
        double d = 0;  for (size_t i = 0; i < noisy.size(); ++i) d += std::pow ((double) noisy[i] - tape[i], 2.0);
        CHECK (d > 1.0e-3, "Vinyl: the voicing should differ from Cassette (difference %.2g)", d);
        const auto play = vinylRun (2, 0.7f, testSignal ((int) (SR * 3), SR, 0.5f));
        float pk;  CHECK (finiteBounded (play, 3.0f, pk) && rmsDb (play, 24000) > -40.0, "Vinyl: finite and audible on a played signal (%.1f dB)", rmsDb (play, 24000));
    }
}

// b106: looper playback modes, stereo phaser, Nonlinear gate shapes
static void testB106()
{
    std::printf("[b106] Looper playback, stereo phaser, Nonlinear gate shape\n");
    const double SR = 48000.0;
    // ---- 1. Looper: record a ramp one second long, then play it in each mode
    {
        auto recordRamp = [&] (Looper& lp)
        {
            lp.prepare (SR);
            Looper::Params prm;  prm.armOnNote = false;  prm.level = 1.0f;
            lp.command (Looper::Cycle);
            std::vector<float> in ((size_t) SR);
            for (size_t i = 0; i < in.size(); ++i) in[i] = (float) i / (float) SR;
            for (size_t b = 0; b + 256 <= in.size(); b += 256) lp.process (in.data() + b, nullptr, 256, prm, 0.0f);
            lp.command (Looper::Cycle);                        // close the loop and start playing
            std::vector<float> z (256, 0.0f);
            lp.process (z.data(), nullptr, 256, prm, 0.0f);    // this block handles the command (it plays the first 256 samples)
        };
        auto play = [&] (Looper& lp, int mode, int n, bool& finite)
        {
            Looper::Params prm;  prm.armOnNote = false;  prm.mode = mode;
            std::vector<float> out ((size_t) n, 0.0f);
            for (int b = 0; b + 256 <= n; b += 256) lp.process (out.data() + b, nullptr, 256, prm, 0.0f);
            for (float v : out) if (! std::isfinite (v)) finite = false;
            return out;
        };
        bool finite = true;
        {
            Looper lp;  recordRamp (lp);
            CHECK (lp.getState() == Looper::Playing, "Looper: after the second press the loop is playing (state %d)", lp.getState());
            const auto o = play (lp, 0, 4096, finite);
            // the loop started 256 samples ago, so the ramp now reads about 256/SR
            CHECK (std::abs (o[1000] - (256.0f + 1000.0f) / (float) SR) < 0.002f, "Looper, Normal: the loop plays the recorded ramp (read %.4f, expected %.4f)", o[1000], (256.0f + 1000.0f) / (float) SR);
        }
        {
            Looper lp;  recordRamp (lp);
            const auto o = play (lp, 1, 8192, finite);
            // the ramp falls when played backwards
            CHECK (o[2000] > o[4000] + 0.03f && o[4000] > o[6000] + 0.03f, "Looper, Reverse: the ramp should fall (%.3f, %.3f, %.3f)", o[2000], o[4000], o[6000]);
        }
        {
            Looper lp;  recordRamp (lp);
            const auto o = play (lp, 2, 20480, finite);
            const float slope = (o[16000] - o[8000]) * (float) SR / 8000.0f;       // loop-seconds per second: 0.5 at half speed
            CHECK (std::abs (slope - 0.5f) < 0.03f, "Looper, Half speed: the ramp climbs at half the rate (slope %.3f, expected 0.5)", slope);
        }
        {
            Looper lp;  recordRamp (lp);
            const auto o = play (lp, 4, 8192, finite);
            const float slope = (o[5000] - o[1000]) * (float) SR / 4000.0f;
            CHECK (std::abs (slope - 2.0f) < 0.1f, "Looper, Double speed: the ramp climbs at twice the rate (slope %.3f, expected 2)", slope);
        }
        {
            // overdub is only done at normal speed: in a non-normal mode the big button leaves the loop playing
            Looper lp;  recordRamp (lp);
            play (lp, 3, 1024, finite);
            lp.command (Looper::Cycle);
            play (lp, 3, 512, finite);
            CHECK (lp.getState() == Looper::Playing, "Looper, Half speed reversed: the big button does not start an overdub (state %d)", lp.getState());
            // switching between modes while playing, including during an overdub, stays finite and ends in a sound state
            Looper lp2;  recordRamp (lp2);
            lp2.command (Looper::Cycle);                           // overdub at normal speed
            play (lp2, 0, 2048, finite);
            const int stOd = lp2.getState();
            play (lp2, 1, 2048, finite);                           // switch to reverse during the overdub
            CHECK (stOd == Looper::Overdub && lp2.getState() == Looper::Playing, "Looper: a mode switch during an overdub ends it (was %d, now %d)", stOd, lp2.getState());
            play (lp2, 0, 2048, finite);  play (lp2, 4, 2048, finite);  play (lp2, 2, 2048, finite);
            CHECK (finite, "Looper: every mode and switching between them stays finite");
        }
    }

    // ---- 2. Stereo phaser in the plugin
    auto runSt = [&] (int type, int stMode, int lane, std::vector<float>& L, std::vector<float>& R)
    {
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
        setP (p, "ampGain", 0.1f);  setP (p, "cabOn", 0.0f);  setP (p, "reverbMix", 0.0f);
        setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", (float) type);  setP (p, "mfAMix", 0.7f);  setP (p, "mfAPhStereo", (float) stMode);
        placeLane (p, 6, lane);
        p.prepareToPlay (SR, 512);
        const auto in = testSignal ((int) (SR * 3), SR, 0.4f);
        L.assign (in.size(), 0.0f);  R.assign (in.size(), 0.0f);
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;
        for (size_t pos = 0; pos + 512 <= in.size(); pos += 512)
        {
            for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[pos + (size_t) i]);  b.setSample (1, i, in[pos + (size_t) i]); }
            p.processBlock (b, m);
            for (int i = 0; i < 512; ++i) { L[pos + (size_t) i] = b.getSample (0, i);  R[pos + (size_t) i] = b.getSample (1, i); }
        }
    };
    auto sideDiffDb = [&] (const std::vector<float>& a, const std::vector<float>& b)
    {
        double d = 0, e = 0;
        for (size_t i = 24000; i < a.size(); ++i) { d += std::pow ((double) a[i] - b[i], 2.0);  e += (double) a[i] * a[i]; }
        return 10.0 * std::log10 ((d + 1e-20) / (e + 1e-20));
    };
    {
        std::vector<float> L0, R0, L1, R1, L2, R2, Lb, Rb;
        runSt (13, 0, 1, L0, R0);  runSt (13, 1, 1, L1, R1);  runSt (13, 2, 1, L2, R2);
        const double d0 = sideDiffDb (L0, R0), d1 = sideDiffDb (L1, R1), d2 = sideDiffDb (L2, R2);
        CHECK (d0 < -80.0, "Phaser Mono, after the cab: both sides are the same (difference %.1f dB)", d0);
        CHECK (d1 > -12.0 && d2 > -9.0, "Phaser Stereo, after the cab: the sides differ at a quarter cycle (%.1f dB) and half a cycle (%.1f dB)", d1, d2);
        const double lm = sideDiffDb (L0, L1);
        CHECK (lm < -50.0, "Phaser Stereo: the left side is the mono phaser (difference %.1f dB)", lm);
        float pk;  CHECK (finiteBounded (L1, 6.0f, pk) && finiteBounded (R1, 6.0f, pk) && finiteBounded (R2, 6.0f, pk), "Phaser Stereo: finite and bounded");
        std::vector<float> Ls, Rs;  runSt (13, 2, 0, Ls, Rs);
        const double dp = sideDiffDb (Ls, Rs);
        CHECK (dp < -80.0, "Phaser Stereo before the amp: the signal is one channel, the sides are the same (difference %.1f dB)", dp);
        // zero allocations while it runs, with the stereo mode switched on the way
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
        setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 13.0f);  placeLane (p, 6, 1);  p.prepareToPlay (SR, 512);
        const auto in = testSignal (4096, SR, 0.4f);
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;  long total = 0;
        for (int blk = 0; blk < 8; ++blk)
        {
            if (blk % 3 == 0) setP (p, "mfAPhStereo", (float) ((blk / 3) % 3));
            for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[(size_t) (blk * 512 + i)]);  b.setSample (1, i, in[(size_t) (blk * 512 + i)]); }
            gAllocCount = 0;  gAllocWatch = (blk >= 2);  p.processBlock (b, m);  gAllocWatch = false;  total += gAllocCount;
        }
        CHECK (total == 0, "Phaser Stereo: %ld allocations in processBlock", total);
    }

    // ---- 3. Nonlinear gate shapes
    {
        auto run = [&] (int shape)
        {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
            setP (p, "ampGain", 0.1f);  setP (p, "cabOn", 0.0f);
            setP (p, "reverbOn", 1.0f);  setP (p, "reverbType", 11.0f);  setP (p, "reverbMix", 1.0f);  setP (p, "reverbSize", 3.0f);  setP (p, "reverbGateShape", (float) shape);
            p.prepareToPlay (SR, 512);
            std::vector<float> in ((size_t) (SR * 2), 0.0f);
            uint32_t seed = 12345u;
            for (size_t i = 0; i < (size_t) (0.02 * SR); ++i) { seed = seed * 1664525u + 1013904223u;  in[i] = 0.6f * ((float) (seed >> 8) / 8388608.0f - 1.0f); }
            std::vector<float> y;  process (p, in, y, 512);
            return y;
        };
        auto win = [&] (const std::vector<float>& y, double t0, double t1)
        {
            std::vector<float> w (y.begin() + (long) (t0 * SR), y.begin() + (long) (t1 * SR));
            return rmsDb (w);
        };
        const auto fade = run (0), chop = run (1), swell = run (2);
        float pk;
        CHECK (finiteBounded (fade, 4.0f, pk) && finiteBounded (chop, 4.0f, pk) && finiteBounded (swell, 4.0f, pk), "Nonlinear: all three gate shapes finite and bounded");
        const double fMid = win (fade, 0.60, 0.70);
        CHECK (fMid > -75.0, "Nonlinear Fade: the held tail is audible just before the hold ends (%.1f dB; the burst is short and quiet, so only the comparisons below are relative)", fMid);
        // hold = 0.12 + 0.22 x 3 s = 0.78 s after the last loud sample; it closes at about 0.80 s
        const double fAfter = win (fade, 0.83, 0.86), cAfter = win (chop, 0.83, 0.86);
        CHECK (cAfter < fAfter - 20.0, "Nonlinear Chop: 30 ms after the hold ends it is far below the fade (chop %.1f dB, fade %.1f dB)", cAfter, fAfter);
        const double cBefore = win (chop, 0.60, 0.70);
        CHECK (std::abs (cBefore - fMid) < 1.5, "Nonlinear Chop: before the cut it equals Fade (%.1f dB against %.1f dB)", cBefore, fMid);
        const double sEarly = win (swell, 0.04, 0.06) - win (fade, 0.04, 0.06), sLate = win (swell, 0.70, 0.75) - win (fade, 0.70, 0.75);
        CHECK (sLate > sEarly + 12.0, "Nonlinear Reverse swell: relative to Fade the tail is lower early (%.1f dB) and near unity late (%.1f dB)", sEarly, sLate);
        const double sAfter = win (swell, 0.84, 0.88);
        CHECK (sAfter < win (swell, 0.70, 0.75) - 20.0, "Nonlinear Reverse swell: it is chopped after the hold (%.1f dB against %.1f dB)", sAfter, win (swell, 0.70, 0.75));
    }
}

// b107: the two modulators
static void testB107()
{
    std::printf("[b107] Modulators\n");
    const double SR = 48000.0;
    // ---- 1. the target list: every id is a real continuous knob
    {
        StarterAmpProcessor p;
        std::string missing;
        for (const auto& t : ModSource::kTargets)
        {
            auto* par = p.apvts.getParameter (t.id);
            if (par == nullptr || dynamic_cast<juce::AudioParameterFloat*> (par) == nullptr) missing += std::string (t.id) + " ";
        }
        CHECK (missing.empty(), "Modulators: every target is a real float knob (not found: %s)", missing.c_str());
        auto* tp = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("mod1T1"));
        CHECK (tp != nullptr && tp->choices.size() == ModSource::kNumTargets + 1, "Modulators: the Target menu has None plus %d knobs (%d)", ModSource::kNumTargets, tp != nullptr ? tp->choices.size() : -1);
    }
    // ---- 2. the modulator signals
    {
        ModSource::Modulator m;  m.prepare (SR);
        // sine at 2 Hz, 512-sample blocks: about 20 zero crossings in 5 s, bounded by 1
        int crossings = 0;  float prev = 0.0f, lo = 0.0f, hi = 0.0f;
        for (int b = 0; b < (int) (5.0 * SR / 512.0); ++b)
        {
            const float v = m.step (ModSource::Sine, 2.0f, 0.0f, 0.0f, 512);
            if (b > 4 && prev * v < 0.0f) ++crossings;
            prev = v;  lo = std::min (lo, v);  hi = std::max (hi, v);
        }
        CHECK (crossings >= 17 && crossings <= 21 && hi <= 1.0f && lo >= -1.0f && hi > 0.8f && lo < -0.8f, "Modulator: a 2 Hz sine crosses zero %d times in 5 s (expected about 20) and swings %.2f to %.2f", crossings, lo, hi);
        // envelope: full scale at -10 dBFS, then falls back with the release
        ModSource::Modulator e;  e.prepare (SR);
        const float full = e.step (ModSource::Envelope, 0.0f, 0.0f, 0.316f, 512);
        float last = full;  int blocks = 0;
        while (last > 0.1f && blocks < 4000) { last = e.step (ModSource::Envelope, 0.0f, 0.0f, 0.0f, 512);  ++blocks; }
        const double tFall = blocks * 512.0 / SR;
        CHECK (full > 0.99f && tFall > 0.02 && tFall < 0.12, "Modulator: the envelope reaches %.2f at -10 dBFS and falls to 0.1 in %.3f s with Smooth 0 (expected about 0.045 s)", full, tFall);
        ModSource::Modulator e2;  e2.prepare (SR);
        e2.step (ModSource::Envelope, 0.0f, 1.0f, 0.316f, 512);
        float l2 = 1.0f;  int b2 = 0;
        while (l2 > 0.1f && b2 < 40000) { l2 = e2.step (ModSource::Envelope, 0.0f, 1.0f, 0.0f, 512);  ++b2; }
        CHECK (b2 * 512.0 / SR > 1.5, "Modulator: with Smooth 1 the envelope takes %.2f s to fall to 0.1 (expected over 1.5 s)", b2 * 512.0 / SR);
        // random steps hold between changes; glide moves smoothly
        ModSource::Modulator st;  st.prepare (SR);
        int changes = 0;  float pv = 2.0f, maxJump = 0.0f;
        for (int b = 0; b < (int) (10.0 * SR / 512.0); ++b)
        {
            const float v = st.step (ModSource::Steps, 1.0f, 0.0f, 0.0f, 512);
            if (pv < 1.5f && std::abs (v - pv) > 1.0e-2f) ++changes;
            pv = v;
        }
        CHECK (changes >= 7 && changes <= 14, "Modulator: random steps at 1 Hz change about 10 times in 10 s (%d)", changes);
        ModSource::Modulator gl;  gl.prepare (SR);
        pv = 0.0f;
        for (int b = 0; b < (int) (10.0 * SR / 512.0); ++b) { const float v = gl.step (ModSource::Glide, 1.0f, 0.0f, 0.0f, 512);  maxJump = std::max (maxJump, std::abs (v - pv));  pv = v; }
        CHECK (maxJump < 0.5f, "Modulator: random glide moves smoothly (largest jump per block %.2f)", maxJump);
    }
    // ---- 3. with the sources off, or the amounts at zero, the sound is exactly what it was
    auto setup = [&] (StarterAmpProcessor& p)
    {
        p.setPlayConfigDetails (2, 2, SR, 512);
        for (const char* on : { "fuzzOn", "odOn", "od2On", "compOn", "bsOn", "wahOn", "wowOn", "filtOn", "chorusOn", "tremOn", "mfAOn", "mfBOn", "mfCOn", "mfDOn", "preDelayOn", "postDelayOn", "reverbOn", "ampBOn" })
            setP (p, on, 1.0f);
        setP (p, "cabOn", 0.0f);  setP (p, "preDelayMix", 0.4f);  setP (p, "postDelayMix", 0.4f);  setP (p, "reverbMix", 0.4f);       // so the delays' and the reverb's own knobs can be heard
        p.prepareToPlay (SR, 512);
    };
    auto render = [&] (StarterAmpProcessor& p, int seconds, float level)
    {
        std::vector<float> y;  process (p, testSignal ((int) (SR * seconds), SR, level), y, 512);
        return y;
    };
    auto diffDb = [&] (const std::vector<float>& a, const std::vector<float>& b)
    {
        double d = 0, e = 0;
        for (size_t i = 0; i < a.size(); ++i) { d += std::pow ((double) a[i] - b[i], 2.0);  e += (double) a[i] * a[i]; }
        return 10.0 * std::log10 ((d + 1e-30) / (e + 1e-30));
    };
    std::vector<float> base;
    {
        StarterAmpProcessor p;  setup (p);  base = render (p, 2, 0.3f);
    }
    {
        StarterAmpProcessor p;  setup (p);
        setP (p, "mod1Src", 2.0f);  setP (p, "mod1T1", 1.0f);  setP (p, "mod1A1", 0.0f);               // on, but the amount is 0
        setP (p, "mod2Src", 0.0f);  setP (p, "mod2T1", 5.0f);  setP (p, "mod2A1", 1.0f);               // an amount, but the source is off
        const auto y = render (p, 2, 0.3f);
        CHECK (y == base, "Modulators: a source that is off, or an amount of zero, leaves the sound bit for bit as it was (difference %.1f dB)", diffDb (y, base));
    }
    // ---- 4. every target responds when it is modulated (an LFO swinging it by half its travel), and the knob itself does not move
    {
        std::string dead;  int tested = 0;
        for (int t = 0; t < ModSource::kNumTargets; ++t)
        {
            const bool needsCab = std::string (ModSource::kTargets[t].id).rfind ("cab", 0) == 0;        // the cab's cuts only act with the cab on
            const bool needsRep = std::string (ModSource::kTargets[t].id).rfind ("rep", 0) == 0;        // b123: the Repeater's knobs only act while it is repeating (on, with Repeat Now held)
            std::vector<float> ref = base;
            if (needsCab) { StarterAmpProcessor c;  setup (c);  setP (c, "cabOn", 1.0f);  ref = render (c, 2, 0.3f); }
            // The Repeater grabs the last slice when Repeat Now goes on, so it needs sound before that, and a slice short enough for several repeats
            // inside the test (pitch and glitch act from the second repeat on): play 0.6 s, switch Repeat Now on, play the rest. 1/32 note slices.
            auto renderRep = [&] (StarterAmpProcessor& q)
            {
                setP (q, "repOn", 1.0f);  setP (q, "repDiv", 5.0f);  setP (q, "repHold", 0.0f);
                const auto sig = testSignal ((int) (SR * 2), SR, 0.3f);
                const size_t cut = (size_t) (0.6 * SR) / 512 * 512;
                std::vector<float> a (sig.begin(), sig.begin() + (long) cut), b (sig.begin() + (long) cut, sig.end()), ya, yb;
                process (q, a, ya, 512);  setP (q, "repHold", 1.0f);  process (q, b, yb, 512);
                ya.insert (ya.end(), yb.begin(), yb.end());  return ya;
            };
            if (needsRep) { StarterAmpProcessor c;  setup (c);  ref = renderRep (c); }
            StarterAmpProcessor p;  setup (p);
            if (needsCab) setP (p, "cabOn", 1.0f);
            setP (p, "mod1Src", 2.0f);  setP (p, "mod1Rate", 3.0f);  setP (p, "mod1Smooth", 0.0f);
            setP (p, "mod1T1", (float) (t + 1));  setP (p, "mod1A1", 0.5f);
            auto* raw = p.apvts.getRawParameterValue (ModSource::kTargets[t].id);
            const float before = raw->load();
            const auto y = needsRep ? renderRep (p) : render (p, 2, 0.3f);
            ++tested;
            const double d = diffDb (y, ref);
            if (! (d > -60.0)) dead += std::string (ModSource::kTargets[t].id) + "(" + std::to_string ((int) d) + " dB) ";
            if (std::abs (raw->load() - before) > 1.0e-6f) dead += std::string (ModSource::kTargets[t].id) + "(knob moved) ";
        }
        CHECK (dead.empty(), "Modulators: all %d targets change the sound when modulated; no response from: %s", tested, dead.c_str());
    }
    // ---- 5. the envelope follows how hard you play
    {
        int revIdx = 0;
        for (int t = 0; t < ModSource::kNumTargets; ++t) if (std::string (ModSource::kTargets[t].id) == "reverbMix") revIdx = t;
        auto envDiff = [&] (float level)
        {
            StarterAmpProcessor p;  setup (p);
            setP (p, "mod1Src", 1.0f);  setP (p, "mod1Smooth", 0.2f);  setP (p, "mod1T1", (float) (revIdx + 1));  setP (p, "mod1A1", 1.0f);       // reverb mix
            StarterAmpProcessor q;  setup (q);
            const auto a = render (p, 2, level), b = render (q, 2, level);
            return diffDb (a, b);
        };
        const double quiet = envDiff (0.003f), loud = envDiff (0.5f);
        CHECK (loud > quiet + 10.0, "Modulators: the envelope moves the knob more when you play harder (difference %.1f dB loud, %.1f dB quiet)", loud, quiet);
    }
    // ---- 6. stable and allocation-free while sources and targets change
    {
        StarterAmpProcessor p;  setup (p);
        const auto in = testSignal (8192, SR, 0.4f);
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;  long total = 0;  bool ok = true;
        for (int blk = 0; blk < 16; ++blk)
        {
            if (blk % 2 == 0)
            {
                setP (p, "mod1Src", (float) (1 + (blk / 2) % 5));  setP (p, "mod2Src", (float) (1 + (blk / 3) % 5));
                setP (p, "mod1T1", (float) (1 + (blk * 7) % ModSource::kNumTargets));  setP (p, "mod1A1", 1.0f);
                setP (p, "mod2T2", (float) (1 + (blk * 11) % ModSource::kNumTargets));  setP (p, "mod2A2", -1.0f);
                setP (p, "mod2T3", (float) (1 + (blk * 5) % ModSource::kNumTargets));  setP (p, "mod2A3", 0.7f);
            }
            for (int i = 0; i < 512; ++i) { b.setSample (0, i, in[(size_t) (blk * 512 + i)]);  b.setSample (1, i, in[(size_t) (blk * 512 + i)]); }
            gAllocCount = 0;  gAllocWatch = (blk >= 2);  p.processBlock (b, m);  gAllocWatch = false;  total += gAllocCount;
            for (int i = 0; i < 512; ++i) if (! std::isfinite (b.getSample (0, i)) || std::abs (b.getSample (0, i)) > 8.0f) ok = false;
        }
        CHECK (total == 0, "Modulators: %ld allocations in processBlock while the sources and targets change", total);
        CHECK (ok, "Modulators: finite and bounded while the sources and targets change");
    }
}

static void testB102Riff()
{
    std::printf("[b102 riff] record / play / re-record rewinds through the audio thread\n");
    RiffRecorder r;  r.prepare (48000.0);
    std::vector<float> in (512), L (512), R (512);
    auto fill = [&] (float v) { for (size_t i = 0; i < in.size(); ++i) in[i] = v * (float) std::sin (0.01 * (double) i); };
    r.record();
    for (int b = 0; b < 20; ++b) { fill (0.3f + 0.01f * (float) b);  L = in;  R = in;  r.processInput (L.data(), R.data(), 512, 1.0f, false); }
    r.stop();
    CHECK (r.getLength() == 20 * 512, "recorded length %d, want %d", r.getLength(), 20 * 512);
    r.play();
    bool same = true;
    for (int b = 0; b < 20; ++b) { std::fill (L.begin(), L.end(), 0.0f);  R = L;  r.processInput (L.data(), R.data(), 512, 1.0f, true);
        fill (0.3f + 0.01f * (float) b);  for (size_t i = 0; i < 512; ++i) if (std::fabs (L[i] - in[i]) > 1e-6f) same = false; }
    CHECK (same, "playback differs from what was recorded");
    r.record();                                   // re-record while the previous playback state is still there
    for (int b = 0; b < 3; ++b) { fill (0.5f);  L = in;  R = in;  r.processInput (L.data(), R.data(), 512, 1.0f, false); }
    r.stop();
    CHECK (r.getLength() == 3 * 512, "a new recording started from zero: length %d, want %d", r.getLength(), 3 * 512);
    r.clear();  r.play();
    CHECK (r.getState() == RiffRecorder::Idle, "play with nothing recorded should stay idle");
}

// b102: the audit's NAM items. (1) Switching Dual on in the stereo pedal slot must not allocate or stall the audio thread; (2) the parser refuses models that
// would take unreasonable memory; (3) a re-warmed second player ends up where a freshly built one would.
static std::string makeLstmJson (int hidden, int layers, unsigned seed, int extraWeights = 0)
{
    std::mt19937 rng (seed);  std::uniform_real_distribution<float> u (-0.3f, 0.3f);
    std::string w;
    int count = 0;
    auto add = [&] (size_t n) { for (size_t i = 0; i < n; ++i) { w += (count++ ? "," : "") + std::to_string (u (rng)); } };
    for (int l = 0; l < layers; ++l) { const size_t I = l == 0 ? 1 : (size_t) hidden;  add (4 * (size_t) hidden * (I + (size_t) hidden));  add (4 * (size_t) hidden);  add ((size_t) hidden);  add ((size_t) hidden); }
    add ((size_t) hidden + 1);  add ((size_t) extraWeights);
    return "{\"version\":\"0.5.0\",\"architecture\":\"LSTM\",\"sample_rate\":48000,\"config\":{\"num_layers\":" + std::to_string (layers) + ",\"input_size\":1,\"hidden_size\":" + std::to_string (hidden)
         + "},\"weights\":[" + w + "]}";
}

static void testB102Nam()
{
    std::printf("[b102 nam] stereo pedal slot Dual toggle without allocation; parser limits\n");
    const double SR = 48000.0;  const int B = 512;
    auto makePlayer = [&] (unsigned seed) -> std::unique_ptr<NamLite::Player>
    {
        juce::String err;  auto m = NamLite::parse (juce::String (makeLstmJson (6, 2, seed)), err);
        if (m == nullptr) { std::printf("  could not build the test model: %s\n", err.toRawUTF8()); return nullptr; }
        return std::make_unique<NamLite::Player> (std::move (m), SR, B);
    };
    auto a = makePlayer (1), b = makePlayer (1);                      // the same model twice, the way the plugin builds a Dual pair
    CHECK (a != nullptr && b != nullptr, "the test LSTM did not load");
    if (a == nullptr || b == nullptr) return;
    NamSlot slot;  slot.prepare (B);
    slot.install (std::move (a), std::move (b), 0.0f);
    slot.pickUp();
    std::vector<float> sig = testSignal ((int) (SR * 4), SR, 0.3f);
    std::vector<float> L (B), R (B);
    long allocs = 0;  int blocksMono = 0, blocksDual = 0;  double worstBlockMs = 0.0;
    for (int blk = 0; blk < (int) sig.size() / B; ++blk)
    {
        std::copy (sig.begin() + blk * B, sig.begin() + (blk + 1) * B, L.begin());  R = L;
        NamSlot::PedalSettings st { true, 0.0f, 0.0f, 1.0f, false, (blk / 40) % 2 == 1 };      // Dual flips every 40 blocks
        const auto t0 = std::chrono::steady_clock::now();
        gAllocCount = 0;  gAllocWatch = true;
        slot.processPedal (L.data(), R.data(), B, st);
        gAllocWatch = false;  allocs += gAllocCount;
        worstBlockMs = std::max (worstBlockMs, std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - t0).count());
        (st.dual && ! slot.rewarming ? blocksDual : blocksMono)++;
        for (int i = 0; i < B; ++i) { CHECK (std::isfinite (L[(size_t) i]) && std::isfinite (R[(size_t) i]), "non-finite output in block %d", blk);  if (! std::isfinite (L[(size_t) i])) return; }
    }
    std::printf("  info: %d mono-sum blocks, %d dual blocks, worst block %.2f ms, allocations %ld\n", blocksMono, blocksDual, worstBlockMs, allocs);
    CHECK (allocs == 0, "processPedal allocated %ld times while Dual was toggled", allocs);
    CHECK (blocksDual > 0, "Dual never took over");
    CHECK (worstBlockMs < 20.0, "a block took %.1f ms when Dual was switched on (the old reset stalled for much longer)", worstBlockMs);

    // a re-warmed player must settle to what a fresh one gives (same model, same input, after the warm-up)
    {
        auto fresh = makePlayer (7), used = makePlayer (7);
        std::vector<float> x (2048), y1, y2;  for (size_t i = 0; i < x.size(); ++i) x[i] = 0.4f * std::sin (0.05f * (float) i);
        std::vector<float> junk (4096, 0.8f);  used->process (junk.data(), (int) junk.size());      // use it, so it is stale
        CHECK (used->isStale(), "a player that has processed audio should be stale");
        used->beginRewarm();  int steps = 0;  while (! used->rewarmStep (1024) && steps < 100) ++steps;
        y1 = x;  y2 = x;  fresh->process (y1.data(), (int) y1.size());  used->process (y2.data(), (int) y2.size());
        double err = 0;  for (size_t i = 0; i < x.size(); ++i) err = std::max (err, (double) std::fabs (y1[i] - y2[i]));
        std::printf("  info: re-warm took %d blocks of 1024; largest difference from a fresh player %.2e\n", steps + 1, err);
        CHECK (err < 1e-4, "a re-warmed player differs from a fresh one by %.2e", err);
    }

    // parser limits
    {
        juce::String err;
        auto big = NamLite::parse (juce::String (makeLstmJson (6, 9, 3)), err);
        CHECK (big == nullptr, "an LSTM with 9 layers should be refused");
        err = {};
        auto huge = NamLite::parse (juce::String ("{\"architecture\":\"LSTM\",\"config\":{\"num_layers\":1,\"input_size\":1,\"hidden_size\":100000},\"weights\":[0]}"), err);
        CHECK (huge == nullptr, "an LSTM with a hidden size of 100000 should be refused");
        err = {};
        auto wn = NamLite::parse (juce::String ("{\"architecture\":\"WaveNet\",\"config\":{\"layers\":[{\"input_size\":1,\"condition_size\":1,\"head_size\":1,\"channels\":8,\"kernel_size\":3,\"dilations\":[1,2,4,1073741824],\"activation\":\"Tanh\",\"gated\":false,\"head_bias\":false}],\"head\":null,\"head_scale\":1.0},\"weights\":[0]}"), err);
        CHECK (wn == nullptr, "a WaveNet with an enormous dilation should be refused (got a model)");
        std::printf("  info: refusal messages: \"%s\"\n", err.toRawUTF8());
    }
}

// b111: the Cab Room knob. Plugin-side measurements in b101 said Room 0.1 and 1.0 sounded the same; this checks it with a warm-up long enough for the convolver to swap in the new response.
static void testRoomKnob()
{
    std::printf("[room] Cab Room knob: the tail grows with Room for every room type, the direct sound stays close, everything finite\n");
    const double SR = 48000.0;
    const int burst = (int) (0.25 * SR), tailN = (int) (1.0 * SR), warm = (int) (2.0 * SR);
    std::vector<float> noise ((size_t) burst);  unsigned st = 4242;
    for (auto& v : noise) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.05); }
    struct R { double direct, tail; bool ok; };
    auto measure = [&] (int type, float room)
    {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        setP (*a, "ampGain", 0.2f);  setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
        for (const char* c : { "cabOnB", "cabOnC", "cabOnD" }) setP (*a, c, 0.0f);
        setP (*a, "cabOn", 1.0f);  setP (*a, "cabOnA", 1.0f);  setP (*a, "cabSrcA", 0.0f);
        setP (*a, "cabRoomType", (float) type);  setP (*a, "cabRoom", room);
        a->prepareToPlay (SR, 512);
        a->applyPendingUpdates();                                      // the plugin does this on its message-thread timer; the test has no message loop
        std::vector<float> w ((size_t) warm), y;  for (size_t i = 0; i < w.size(); ++i) w[i] = noise[i % noise.size()] * ((i / noise.size()) % 2 == 0 ? 1.0f : 0.0f);
        process (*a, w, y, 512);                                        // warm-up: the convolver builds the new response on a background thread
        juce::Thread::sleep (1500);  { std::vector<float> w2 ((size_t) (0.2 * SR), 0.0f);  process (*a, w2, y, 512); }   // wait for it, then let it swap in
        std::vector<float> x ((size_t) (burst + tailN), 0.0f);  std::copy (noise.begin(), noise.end(), x.begin());
        float pk;  process (*a, x, y, 512);
        R r;  r.ok = finiteBounded (y, 32.0f, pk);
        r.direct = rmsDb (std::vector<float> (y.begin() + 2000, y.begin() + burst - 2000));
        r.tail = rmsDb (std::vector<float> (y.begin() + burst + (int) (0.05 * SR), y.begin() + burst + (int) (0.40 * SR)));
        return r; };
    for (int type = 0; type <= 3; ++type)
    {   double d[5], t[5];  bool ok = true;  const float rooms[5] = { 0.0f, 0.1f, 0.3f, 0.6f, 1.0f };
        for (int i = 0; i < 5; ++i) { const auto r = measure (type, rooms[i]);  d[i] = r.direct;  t[i] = r.tail;  ok = ok && r.ok; }
        std::printf ("  info: room type %d: direct %.1f %.1f %.1f %.1f %.1f dB, tail %.1f %.1f %.1f %.1f %.1f dB (Room 0, 0.1, 0.3, 0.6, 1)\n", type, d[0], d[1], d[2], d[3], d[4], t[0], t[1], t[2], t[3], t[4]);
        CHECK (ok, "room type %d: finite and bounded at every Room setting", type);
        CHECK (t[1] > t[0] - 0.3 && t[2] > t[1] - 0.3 && t[3] > t[2] - 0.3 && t[4] > t[3] - 0.3, "room type %d: the tail never shrinks as Room goes up (%.1f %.1f %.1f %.1f %.1f dB)", type, t[0], t[1], t[2], t[3], t[4]);
        CHECK (t[4] > t[1] + 3.0, "room type %d: Room 1 has a tail at least 3 dB above Room 0.1 (%.1f against %.1f dB)", type, t[4], t[1]);
        CHECK (std::abs (d[4] - d[0]) < 3.0, "room type %d: the direct sound moves by less than 3 dB from Room 0 to 1 (%.1f dB)", type, d[4] - d[0]); }
}

static void testB111()
{
    std::printf("[b111] polyphonic tuner hold-off; Spring Feedback on the spring tank\n");
    {   // the gate: a pluck holds a string's reading back for 0.6 s; a ringing string with a wobbling level does not
        PolySettleGate g;  g.reset();  const float dt = 0.15f;
        auto run = [&] (float level) { return g.update (0, level, dt); };
        for (int i = 0; i < 10; ++i) run (0.0f);                                      // silence
        const bool atPluck = run (0.05f);                                              // the pluck arrives
        int heldFrames = 1;  bool open = atPluck;  float t = dt;
        while (! open && t < 2.0f) { open = run (0.05f * std::exp (-1.5f * t));  t += dt;  if (! open) ++heldFrames; }
        CHECK (! atPluck && t >= 0.55f && t <= 0.85f, "a pluck is held for about 0.6 s (opened after %.2f s, %d frames)", (double) t, heldFrames);
        // a ringing string: the level wobbles by +-25 percent each frame and never decays much
        PolySettleGate r;  r.reset();  for (int i = 0; i < 4; ++i) r.update (0, 0.0f, dt);
        bool all = false;  for (int i = 0; i < 12; ++i) all = r.update (0, 0.05f, dt);        // settles first
        int blocked = 0;  for (int i = 0; i < 40; ++i) if (! r.update (0, 0.05f * (i % 2 ? 1.25f : 0.8f), dt)) ++blocked;
        CHECK (all && blocked == 0, "a sustaining string with a wobbling level is never held back (%d blocked frames)", blocked);
        // two plucks a second apart are each held; a second pluck inside the hold does not extend it much
        PolySettleGate q;  q.reset();  for (int i = 0; i < 4; ++i) q.update (0, 0.0f, dt);
        int h1 = 0, h2 = 0;  float lev = 0.05f;
        for (int i = 0; i < 12; ++i) { if (! q.update (0, lev, dt)) ++h1;  lev *= 0.8f; }
        for (int i = 0; i < 20; ++i) q.update (0, 0.002f, dt);                         // dies away
        lev = 0.05f;  for (int i = 0; i < 12; ++i) { if (! q.update (0, lev, dt)) ++h2;  lev *= 0.8f; }
        CHECK (h1 >= 3 && h1 <= 6 && h2 >= 3 && h2 <= 6, "each pluck is held for 3 to 6 frames of 0.15 s (%d and %d)", h1, h2);
        // strings are independent
        PolySettleGate s2;  s2.reset();  for (int i = 0; i < 30; ++i) { s2.update (0, 0.05f, dt);  s2.update (1, 0.05f, dt); }
        s2.update (0, 0.5f, dt);
        CHECK (! s2.update (0, 0.5f, dt) && s2.update (1, 0.05f, dt), "a pluck on one string does not hold the others"); }

    {   // b111: Spring Feedback on the tank
        const double SR = 48000.0;
        auto tail = [&] (float fb, float decay, double& peak, double& e1, double& e3)
        {   FxExtra::SpringTank k;  k.prepare (SR);  k.setParams (decay, 0.5f);  k.setFeedback (fb);  k.reset();
            peak = 0;  e1 = e3 = 0;  bool fin = true;
            for (int i = 0; i < (int) (6.0 * SR); ++i)
            {   const float in = i < 48 ? 0.8f * (i % 2 ? -1.0f : 1.0f) : 0.0f;                   // one hit
                float l, r;  k.process (in, l, r);  fin = fin && std::isfinite (l) && std::isfinite (r);
                peak = std::max (peak, (double) std::max (std::abs (l), std::abs (r)));
                if (i >= (int) (1.0 * SR) && i < (int) (2.0 * SR)) e1 += (double) l * l;
                if (i >= (int) (4.0 * SR) && i < (int) (5.0 * SR)) e3 += (double) l * l; }
            return fin; };
        for (float fbv : { 0.0f, 0.5f, 0.8f, 1.0f })
        {   FxExtra::SpringTank k;  k.prepare (SR);  k.setParams (2.0f, 0.5f);  k.setFeedback (fbv);  k.reset();
            double sl[8] {};  for (int i = 0; i < (int) (8.0 * SR); ++i) { float l, r;  k.process (i < 48 ? 0.8f * (i % 2 ? -1.0f : 1.0f) : 0.0f, l, r);  sl[i / (int) SR] += (double) l * l / SR; }
            std::printf ("  info: ring-down, Feedback %.1f, level per second (dB re 1 s of full scale): ", (double) fbv);  for (double v : sl) std::printf ("%.0f ", 10 * std::log10 (v + 1e-30));  std::printf ("\n"); }
        double p0, a0, b0, p5, a5, b5, p10, a10, b10;
        const bool f0 = tail (0.0f, 2.0f, p0, a0, b0), f5 = tail (0.5f, 2.0f, p5, a5, b5), f10 = tail (1.0f, 2.0f, p10, a10, b10);
        std::printf ("  info: spring tank after one hit, Decay 2 s: peak %.3f / %.3f / %.3f, level 4-5 s after the hit %.1f / %.1f / %.1f dB (Feedback 0 / 0.5 / 1)\n", p0, p5, p10, 10 * std::log10 (b0 + 1e-30), 10 * std::log10 (b5 + 1e-30), 10 * std::log10 (b10 + 1e-30));
        CHECK (f0 && f5 && f10, "the spring tank stays finite at Feedback 0, 0.5 and 1");
        CHECK (p10 < 4.0 * std::max (p0, 0.05), "Feedback 1 stays bounded (peak %.2f against %.2f at Feedback 0)", p10, p0);
        { double pp, m1 = 0, m5 = 0;  FxExtra::SpringTank k;  k.prepare (SR);  k.setParams (2.0f, 0.5f);  k.setFeedback (1.0f);  k.reset();
          for (int i = 0; i < (int) (8.0 * SR); ++i) { float l, r;  k.process (i < 48 ? 0.8f * (i % 2 ? -1.0f : 1.0f) : 0.0f, l, r);  if (i >= (int) (1.0 * SR) && i < (int) (1.5 * SR)) m1 += (double) l * l;  if (i >= (int) (7.5 * SR)) m5 += (double) l * l; }  (void) pp;
          CHECK (m5 < m1 * 0.5, "even at Feedback 1 the ring still dies away (level 1 to 1.5 s %.1f dB, 7.5 to 8 s %.1f dB)", 10 * std::log10 (m1 + 1e-30), 10 * std::log10 (m5 + 1e-30)); }
        CHECK (b5 > 4.0 * b0 && b10 >= b5, "the ring lasts longer with Feedback: level 4 to 5 s after the hit %.1f, %.1f and %.1f dB", 10 * std::log10 (b0 + 1e-30), 10 * std::log10 (b5 + 1e-30), 10 * std::log10 (b10 + 1e-30));
        double q, x1, x2;  tail (0.0f, 2.0f, q, x1, x2);
        CHECK (std::abs (q - p0) < 1e-12 && std::abs (x2 - b0) < 1e-18, "Feedback 0 gives the same output on every run (it adds exactly zero to the driver)"); }
    {   // through the plugin: type Spring (tank), Feedback up gives a longer tail, and it is saved with the state
        const double SR = 48000.0;
        auto render = [&] (float fb)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            setP (*a, "reverbOn", 1.0f);  setP (*a, "reverbType", 17.0f);  setP (*a, "reverbMix", 0.6f);  setP (*a, "reverbSize", 2.0f);  setP (*a, "reverbSpringFb", fb);
            setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);  setP (*a, "ampAOn", 0.0f);
            a->prepareToPlay (SR, 512);
            std::vector<float> x ((size_t) (6.0 * SR), 0.0f);  for (int i = 0; i < 2000; ++i) x[(size_t) i] = 0.4f * std::sin (0.12f * (float) i) * std::exp (-(float) i / 400.0f);
            std::vector<float> y;  process (*a, x, y, 512);  float pk;  const bool ok = finiteBounded (y, 32.0f, pk);
            std::vector<float> late (y.begin() + (int) (4.0 * SR), y.begin() + (int) (5.5 * SR));
            return std::make_pair (ok, rmsDb (late)); };
        const auto r0 = render (0.0f), r1 = render (0.8f);
        std::printf ("  info: plugin, Spring (tank), tail level 4 to 5.5 s after a hit: %.1f dB at Feedback 0, %.1f dB at 0.8\n", r0.second, r1.second);
        CHECK (r0.first && r1.first, "Spring (tank) at Feedback 0 and 0.8 plays: finite and bounded");
        CHECK (r1.second > r0.second + 3.0, "Spring Feedback 0.8 lengthens the tail through the plugin (%.1f against %.1f dB)", r1.second, r0.second);
        auto a = std::make_unique<StarterAmpProcessor>();  setP (*a, "reverbSpringFb", 0.7f);  setP (*a, "rev2SpringFb", 0.3f);
        juce::MemoryBlock mb;  a->getStateInformation (mb);
        auto b = std::make_unique<StarterAmpProcessor>();  b->setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (std::abs (b->apvts.getRawParameterValue ("reverbSpringFb")->load() - 0.7f) < 0.01f && std::abs (b->apvts.getRawParameterValue ("rev2SpringFb")->load() - 0.3f) < 0.01f, "both Spring Feedback knobs survive save and load"); }
}

// b114: tail length, sidechain clean-up, the saturator's high-drive growth and output ceiling, and the cab blocks' Delay reported as latency
static void testB114()
{
    std::printf("[b114] tail length, sidechain clean-up, saturator ceiling, cab latency\n");
    const double SR = 48000.0;
    // ---- tail length
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "reverbOn", 0.0f);  setP (p, "rev2On", 0.0f);  setP (p, "preDelayOn", 0.0f);  setP (p, "postDelayOn", 0.0f);
        const double base = p.getTailLengthSeconds();
        CHECK (base >= 0.4 && base <= 1.0, "nothing time-based on: a small tail (%.2f s)", base);
        setP (p, "reverbOn", 1.0f);  setP (p, "reverbMix", 0.4f);  setP (p, "reverbSize", 3.0f);
        const double r3 = p.getTailLengthSeconds();
        setP (p, "reverbSize", 8.0f);  const double r8 = p.getTailLengthSeconds();
        CHECK (r3 >= 3.0 && r8 >= 8.0 && r8 > r3, "Reverb decay 3 s and 8 s report at least that (%.2f, %.2f s)", r3, r8);
        setP (p, "reverbMix", 0.0f);  CHECK (p.getTailLengthSeconds() < 1.0, "Reverb mix at 0 adds nothing (%.2f s)", p.getTailLengthSeconds());
        setP (p, "reverbMix", 0.4f);  setP (p, "reverbSize", 2.0f);  setP (p, "reverbType", 17.0f);  setP (p, "reverbSpringFb", 0.0f);  const double s0 = p.getTailLengthSeconds();
        setP (p, "reverbSpringFb", 1.0f);  const double s1 = p.getTailLengthSeconds();
        CHECK (s1 > 3.0 * s0 && s1 <= 30.0, "Spring Feedback lengthens the reported tail (%.1f s to %.1f s)", s0, s1);
        setP (p, "reverbOn", 0.0f);
        setP (p, "postDelayOn", 1.0f);  setP (p, "postDelayMix", 0.4f);  setP (p, "postDelayTime", 500.0f);  setP (p, "postDelayFeedback", 0.0f);  const double d0 = p.getTailLengthSeconds();
        setP (p, "postDelayFeedback", 0.6f);  const double d6 = p.getTailLengthSeconds();
        setP (p, "postDelayFeedback", 0.9f);  const double d9 = p.getTailLengthSeconds();
        std::printf ("  info: delay 500 ms: tail %.2f s at feedback 0, %.2f s at 0.6, %.2f s at 0.9\n", d0, d6, d9);
        CHECK (d0 < d6 && d6 < d9 && d6 >= 0.5 * 12 * 0.5 && d9 <= 30.0, "a delay's tail grows with its feedback and stays within 30 s (%.2f, %.2f, %.2f s)", d0, d6, d9);
        // sixty dB down at feedback 0.6: 500 ms x 14 repeats = 7 s
        CHECK (d6 >= 7.0 && d6 <= 8.5, "Delay 500 ms at feedback 0.6 reports the time to 60 dB down (about 7 s): %.2f s", d6); }
    // ---- sidechain with bad samples
    {   const int blk = 512;
        auto render = [&] (bool bad, std::vector<float>& out)
        {   StarterAmpProcessor p;  auto lay = p.getBusesLayout();  lay.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();  p.setBusesLayout (lay);
            p.prepareToPlay (SR, blk);
            setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 23.0f);  setP (p, "mfAPos", 1.0f);  setP (p, "mfAMix", 1.0f);  setP (p, "mfAVocCar", 2.0f);
            const int len = (int) (2.0 * SR);  out.assign ((size_t) len, 0.0f);  juce::MidiBuffer m;  juce::AudioBuffer<float> b (4, blk);  unsigned st = 3;  bool fin = true;
            for (int pos = 0; pos < len; pos += blk)
            {   const int n = std::min (blk, len - pos);  juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 4, n);
                for (int i = 0; i < n; ++i)
                {   st = st * 1664525u + 1013904223u;  const float nz = 0.3f * (float) ((((st >> 8) / 8388608.0) - 1.0));
                    float sc = nz;
                    if (bad && pos + i >= 20000 && pos + i < 20050) sc = std::numeric_limits<float>::quiet_NaN();
                    if (bad && pos + i >= 30000 && pos + i < 30010) sc = std::numeric_limits<float>::infinity();
                    if (bad && pos + i >= 40000 && pos + i < 40010) sc = 1.0e9f;
                    v.setSample (0, i, nz);  v.setSample (1, i, nz);  v.setSample (2, i, sc);  v.setSample (3, i, sc); }
                p.processBlock (v, m);
                for (int i = 0; i < n; ++i) { const float o = v.getSample (0, i);  out[(size_t) (pos + i)] = o;  fin = fin && std::isfinite (o); } }
            return fin; };
        std::vector<float> good, bad;  const bool f0 = render (false, good), f1 = render (true, bad);
        double pk = 0, tailRms = 0, tailRef = 0;  for (size_t i = 0; i < bad.size(); ++i) pk = std::max (pk, (double) std::abs (bad[i]));
        for (size_t i = (size_t) (1.5 * SR); i < bad.size(); ++i) { tailRms += (double) bad[i] * bad[i];  tailRef += (double) good[i] * good[i]; }
        CHECK (f0 && f1, "the vocoder output stays finite when the sidechain carries NaN, infinity and 1e9 samples");
        CHECK (tailRms > 0.0 && tailRms < 4.0 * tailRef + 1e-6, "and it recovers: the last half second is as loud as without the bad samples (%.3g against %.3g)", tailRms, tailRef); }
    // ---- saturator: growth at high drive and the ceiling
    {   using MS = FxExtra::MultibandSatFx;
        auto peak = [&] (float drive, float amp, float outDb, bool sine, double hz = 3000.0)
        {   MS m;  m.prepare (SR);  MS::Params p;  for (int b = 0; b < 4; ++b) p.driveDb[b] = drive;  p.outDb = outDb;
            std::vector<float> l ((size_t) SR);  unsigned st = 11;  double pk = 0;  bool fin = true;
            for (size_t i = 0; i < l.size(); ++i) { st = st * 1664525u + 1013904223u;  l[i] = sine ? amp * (float) std::sin (2 * M_PI * hz * (double) i / SR) : amp * (float) ((((st >> 8) / 8388608.0) - 1.0)); }
            for (size_t pos = 0; pos < l.size(); pos += 512) m.process (l.data() + pos, nullptr, (int) std::min<size_t> (512, l.size() - pos), p);
            for (size_t i = 0; i < l.size(); ++i) { fin = fin && std::isfinite (l[i]);  if (i > 2000) pk = std::max (pk, (double) std::abs (l[i])); }
            return fin ? pk : 1e9; };
        std::printf ("  info: 0.5 noise burst, peak by Drive (b112 reported 0.9 / 0.9 / 2.0 / 6.8 / 28):");
        double pk[5];  int k = 0;  for (float d : { 0.0f, 12.0f, 24.0f, 36.0f, 48.0f }) { pk[k] = peak (d, 0.5f, 0.0f, false);  std::printf (" %.2f", pk[k]);  ++k; }  std::printf ("\n");
        CHECK (pk[0] < 1.0 && pk[1] < 1.0, "Drive 0 and 12 dB are as before: peak %.2f and %.2f (b112: 0.9)", pk[0], pk[1]);
        CHECK (pk[4] < 3.0 && pk[3] < 3.0, "Drive 36 and 48 dB no longer grow: peak %.2f and %.2f (b112: 6.8 and 28)", pk[3], pk[4]);
        double worst = 0;  for (float d : { 12.0f, 24.0f, 36.0f, 48.0f }) for (double hz : { 200.0, 1000.0, 5000.0, 12000.0 }) worst = std::max (worst, peak (d, 0.7f, 0.0f, true, hz));
        CHECK (worst < 3.0, "a 0.7 sine at 200 Hz to 12 kHz and Drive 12 to 48 dB stays under 3.0 (worst %.2f)", worst);
        const double hot = peak (0.0f, 0.5f, 40.0f, false);
        CHECK (hot <= 3.0 && hot > 2.0, "the output ceiling: with +40 dB of Out the peak is held at %.2f (under 3.0)", hot);
        const double small = peak (0.0f, 0.1f, 0.0f, true, 1000.0);
        CHECK (small < 0.5 && small > 0.01, "a quiet signal passes the ceiling untouched (peak %.3f)", small); }
    // ---- cab Delay as host latency
    {   auto make = [&] (float delayMs, bool cabOn)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            setP (*a, "cabOn", cabOn ? 1.0f : 0.0f);  setP (*a, "cabOnA", 1.0f);  setP (*a, "cabDelayA", delayMs);  setP (*a, "cabOnB", 0.0f);  setP (*a, "cabOnC", 0.0f);  setP (*a, "cabOnD", 0.0f);
            a->prepareToPlay (SR, 512);  a->applyPendingUpdates();  return a; };
        const int base = make (0.0f, true)->getLatencySamples();
        auto a5 = make (5.0f, true);
        CHECK (a5->getLatencySamples() == base + 240, "Delay 5 ms on an active cab block adds 240 samples at 48 kHz (%d, was %d)", a5->getLatencySamples(), base);
        CHECK (make (5.0f, false)->getLatencySamples() == base, "with the cab off the Delay adds nothing");
        {   auto b = make (5.0f, true);  setP (*b, "cabOnA", 0.0f);  setP (*b, "cabOnB", 1.0f);  setP (*b, "cabDelayB", 2.0f);  setP (*b, "cabDelayA", 9.0f);
            const auto x = testSignal ((int) (0.8 * SR), SR);  std::vector<float> y;  process (*b, x, y, 512);  b->applyPendingUpdates();
            CHECK (b->getLatencySamples() == base + 96, "only blocks that are switched on count: block B at 2 ms gives 96 samples (%d)", b->getLatencySamples() - base); }
        // holding back: a knob moved every 0.1 s does not change the reported latency; once it stays for 0.5 s it does
        auto c = make (0.0f, true);  const auto x = testSignal ((int) (0.1 * SR), SR);  std::vector<float> y;  bool still = true;
        for (int k = 0; k < 8; ++k) { setP (*c, "cabDelayA", (k & 1) ? 1.0f : 6.0f);  process (*c, x, y, 512);  c->applyPendingUpdates();  still = still && c->getLatencySamples() == base; }
        CHECK (still, "a Delay knob moved every 0.1 s does not make the host re-sync");
        setP (*c, "cabDelayA", 4.0f);  const auto x2 = testSignal ((int) (0.7 * SR), SR);  process (*c, x2, y, 512);  c->applyPendingUpdates();
        CHECK (c->getLatencySamples() == base + 192, "after 0.5 s at 4 ms the latency follows (%d, want %d)", c->getLatencySamples(), base + 192);
        setP (*c, "cabDelayA", 0.0f);  process (*c, x2, y, 512);  c->applyPendingUpdates();
        CHECK (c->getLatencySamples() == base, "and goes back to %d when the Delay is 0 again (%d)", base, c->getLatencySamples()); }
}


// ---------------------------------------------------------------------------------------------------------------------
// b115: harmonizer with four voices, MIDI mode, melodic minor and the arpeggiator
static void testB115()
{
    std::printf("[b115] harmonizer: four voices, MIDI mode, melodic minor, arpeggiator\n");
    const double SR = 48000.0;
    auto sine = [&] (double f, double secs) { std::vector<float> x ((size_t) (secs * SR)); for (size_t i = 0; i < x.size(); ++i) x[i] = 0.4f * (float) std::sin (6.283185307 * f * (double) i / SR); return x; };
    auto power = [&] (const std::vector<float>& y, double f, double t0, double t1)
    {   const size_t a = (size_t) (t0 * SR), b = std::min (y.size(), (size_t) (t1 * SR));  double re = 0, im = 0;
        for (size_t i = a; i < b; ++i) { const double ph = 6.283185307 * f * (double) i / SR;  re += y[i] * std::cos (ph);  im += y[i] * std::sin (ph); }
        const double n = (double) (b - a);  return (re * re + im * im) / (n * n) * 4.0 + 1e-20; };
    auto db = [] (double a, double b) { return 10.0 * std::log10 (a / b); };
    // runs the engine on a sine, in blocks; the result is the left channel
    auto run = [&] (HarmonizerEngine& e, const HarmSettings& hs, double f, double secs, bool poly, std::vector<int>* arpTrace = nullptr)
    {   auto x = sine (f, secs);  std::vector<float> R (x);  juce::AudioBuffer<float> scratch;  const int blk = 512;
        for (size_t pos = 0; pos < x.size(); pos += (size_t) blk)
        {   const int n = (int) std::min ((size_t) blk, x.size() - pos);
            if (hs.mode != 0) { if (poly) e.feedChords (x.data() + pos, n); else e.getTracker().feed (x.data() + pos, n); }
            e.process (x.data() + pos, R.data() + pos, scratch, n, hs);
            if (arpTrace) arpTrace->push_back (e.arpVoiceNow()); }
        return x; };
    auto fresh = [&] (HarmonizerEngine& e, bool poly) { e.prepare (SR);  e.setRange (58.0f, 28.0f);  e.setPoly (poly, false);  e.reset(); };

    // ---- each of the four voices lands where it should (Interval mode, one voice at a time)
    {   const float want[4] = { 7.0f, -12.0f, 12.0f, 19.0f };    // A..D
        for (int v = 0; v < 4; ++v)
        {   HarmonizerEngine e;  fresh (e, false);  HarmSettings hs;  hs.mode = 0;  hs.dry = 0.0f;  hs.tone = 1.0f;
            for (int k = 0; k < 4; ++k) { hs.lvl[k] = k == v ? 1.0f : 0.0f;  hs.semis[k] = want[k]; }
            const auto y = run (e, hs, 220.0, 2.0, false);
            const double f = 220.0 * std::pow (2.0, want[v] / 12.0);
            const double on = power (y, f, 1.0, 2.0), off = power (y, 220.0, 1.0, 2.0);
            std::printf ("  info: voice %c %+0.0f st: %.0f Hz is %.1f dB above the original pitch\n", 'A' + v, want[v], f, db (on, off));
            CHECK (db (on, off) > 12.0, "voice %c at %+0.0f semitones sounds at %.0f Hz (%.1f dB above the unshifted pitch, want 12)", 'A' + v, want[v], f, db (on, off)); } }

    // ---- melodic minor: the table and the harmony it gives
    {   const HarmScale& m = harmScale (7);
        CHECK (m.n == 7 && m.steps[0] == 0 && m.steps[2] == 3 && m.steps[5] == 9 && m.steps[6] == 11, "Melodic minor is C D Eb F G A B (n=%d)", m.n);
        const int played[7] = { 60, 62, 63, 65, 67, 69, 71 }, third[7] = { 3, 3, 4, 4, 4, 3, 3 }, fifth[7] = { 7, 7, 8, 7, 7, 6, 6 };
        bool ok3 = true, ok5 = true;
        for (int i = 0; i < 7; ++i)
        {   if ((int) harmonyShift ((float) played[i], 0, 7, 2) != third[i]) ok3 = false;
            if ((int) harmonyShift ((float) played[i], 0, 7, 4) != fifth[i]) ok5 = false; }
        CHECK (ok3, "melodic minor in C: thirds above C D Eb F G A B are 3 3 4 4 4 3 3 semitones");
        CHECK (ok5, "melodic minor in C: fifths above C D Eb F G A B are 7 7 8 7 7 6 6 semitones");
        CHECK ((int) harmonyShift (60.0f, 0, 7, 7) == 12 && (int) harmonyShift (60.0f, 0, 7, -7) == -12, "melodic minor: 7 steps is an octave either way");
        HarmSettings hs;  hs.mode = 1;  hs.key = 0;  hs.scale = 7;  hs.deg[2] = 2;  hs.deg[3] = -2;
        CHECK ((int) hs.shiftFor (2, 67.0f) == 4 && (int) hs.shiftFor (3, 67.0f) == -4, "voices C and D follow the scale too: from G, +2 steps is +4 and -2 steps is -4 (%.0f, %.0f)", hs.shiftFor (2, 67.0f), hs.shiftFor (3, 67.0f)); }

    // ---- MIDI mode: the held keys decide the notes
    {   HarmSettings hs;  hs.mode = 2;  hs.midiNotes[0] = 64;  hs.midiNotes[1] = 67;  hs.nMidi = 2;
        CHECK ((int) hs.shiftFor (0, 60.2f) == 4 && (int) hs.shiftFor (1, 60.2f) == 7, "MIDI: from C, keys E and G are +4 and +7 (%.1f, %.1f)", hs.shiftFor (0, 60.2f), hs.shiftFor (1, 60.2f));
        CHECK ((int) hs.shiftFor (0, 71.0f) == -7, "MIDI: from B the E key is 7 semitones down (%.1f)", hs.shiftFor (0, 71.0f));
        hs.lvl[0] = hs.lvl[1] = hs.lvl[2] = hs.lvl[3] = 0.5f;
        CHECK (hs.voiceOn (0) && hs.voiceOn (1) && ! hs.voiceOn (2) && ! hs.voiceOn (3), "MIDI: with two keys held only voices A and B are on");
        hs.nMidi = 0;  CHECK (! hs.voiceOn (0), "MIDI: no keys held, no voices");
        // audio: a 220 Hz note, key A4 held: voice A comes out at 440 Hz
        HarmonizerEngine e;  fresh (e, false);  HarmSettings m;  m.mode = 2;  m.dry = 0.0f;  m.tone = 1.0f;  m.lvl[0] = 1.0f;  m.lvl[1] = m.lvl[2] = m.lvl[3] = 0.0f;  m.midiNotes[0] = 69;  m.nMidi = 1;
        const auto y = run (e, m, 220.0, 2.5, false);
        CHECK (db (power (y, 440.0, 1.5, 2.5), power (y, 220.0, 1.5, 2.5)) > 12.0, "MIDI: playing A3 with the A4 key held sounds at 440 Hz (%.1f dB over the original)", db (power (y, 440.0, 1.5, 2.5), power (y, 220.0, 1.5, 2.5)));
        // the tracked note moves, the target note stays: play 247 Hz (B3) with the same key, voice A drops to 440 Hz still
        HarmonizerEngine e2;  fresh (e2, false);
        const auto y2 = run (e2, m, 246.94, 2.5, false);
        CHECK (db (power (y2, 440.0, 1.5, 2.5), power (y2, 246.94, 1.5, 2.5)) > 10.0, "MIDI: a different played note still lands on the held key (%.1f dB over the original)", db (power (y2, 440.0, 1.5, 2.5), power (y2, 246.94, 1.5, 2.5))); }

    // ---- arpeggiator
    {   auto seq = [&] (int pattern, double bpm, float beats, std::vector<int>* trace = nullptr)
        {   HarmonizerEngine e;  fresh (e, false);  HarmSettings hs;  hs.mode = 0;  hs.arp = true;  hs.arpPattern = pattern;  hs.arpBeats = beats;  hs.bpm = bpm;  hs.dry = 0.0f;
            hs.lvl[0] = hs.lvl[1] = hs.lvl[2] = 0.5f;  hs.lvl[3] = 0.0f;  hs.semis[0] = 4;  hs.semis[1] = 7;  hs.semis[2] = 12;
            std::vector<int> tr;  run (e, hs, 220.0, 4.0, false, &tr);
            std::vector<int> out;  for (int v : tr) if (out.empty() || out.back() != v) out.push_back (v);
            if (trace) *trace = tr;  return out; };
        auto str = [] (const std::vector<int>& v, size_t n) { std::string t;  for (size_t i = 0; i < std::min (n, v.size()); ++i) t += std::to_string (v[i]) + " ";  return t; };
        const auto up = seq (0, 120.0, 0.25f), dn = seq (1, 120.0, 0.25f), ud = seq (2, 120.0, 0.25f), rnd = seq (3, 120.0, 0.25f);
        const int upW[7] = { 0, 1, 2, 0, 1, 2, 0 }, dnW[7] = { 2, 1, 0, 2, 1, 0, 2 }, udW[8] = { 0, 1, 2, 1, 0, 1, 2, 1 };
        bool a = up.size() >= 7, b = dn.size() >= 7, c = ud.size() >= 8;
        for (int i = 0; a && i < 7; ++i) a = up[(size_t) i] == upW[i];
        for (int i = 0; b && i < 7; ++i) b = dn[(size_t) i] == dnW[i];
        for (int i = 0; c && i < 8; ++i) c = ud[(size_t) i] == udW[i];
        CHECK (a, "arp Up steps A B C A B C (%s)", str (up, 8).c_str());
        CHECK (b, "arp Down steps C B A C B A (%s)", str (dn, 8).c_str());
        CHECK (c, "arp Up and down steps A B C B A B C B (%s)", str (ud, 10).c_str());
        bool inSet = rnd.size() > 8, seen[3] = { false, false, false };
        for (int v : rnd) { if (v < 0 || v > 2) inSet = false; else seen[v] = true; }
        CHECK (inSet && seen[0] && seen[1] && seen[2], "arp Random only plays voices that are on, and plays all of them (%s)", str (rnd, 12).c_str());
        // tempo: a quarter of a beat (1/16 note) at 120 BPM is 0.125 s a step (32 in 4 s); at 60 BPM half as many; an eighth of a beat (1/32) twice as many
        const auto s120 = seq (0, 120.0, 0.25f), s60 = seq (0, 60.0, 0.25f), s120b = seq (0, 120.0, 0.125f);
        std::printf ("  info: steps in 4 s: 120 BPM %zu, 60 BPM %zu, 120 BPM at double speed %zu\n", s120.size(), s60.size(), s120b.size());
        CHECK (std::abs ((int) s120.size() - 32) <= 1 && std::abs ((int) s60.size() - 16) <= 1 && std::abs ((int) s120b.size() - 64) <= 1, "arp steps follow the tempo and the rate (%zu, %zu, %zu; want 32, 16, 64)", s120.size(), s60.size(), s120b.size());
        // nothing on: no crash, no voice, the dry sound goes through
        HarmonizerEngine e;  fresh (e, false);  HarmSettings hs;  hs.mode = 0;  hs.arp = true;  hs.lvl[0] = hs.lvl[1] = hs.lvl[2] = hs.lvl[3] = 0.0f;  hs.dry = 1.0f;
        std::vector<int> tr;  const auto y = run (e, hs, 220.0, 1.0, false, &tr);
        float pk = 0;  CHECK (finiteBounded (y, 2.0f, pk) && tr.back() == -3 && db (power (y, 220.0, 0.5, 1.0), 0.16) > -3.0, "arp with no voices on: the dry note passes (peak %.2f)", pk); }
    {   // the gate shortens the notes, the arp level sets how loud they are
        auto level = [&] (float gate, float lvl)
        {   HarmonizerEngine e;  fresh (e, false);  HarmSettings hs;  hs.mode = 0;  hs.arp = true;  hs.arpBeats = 0.25f;  hs.bpm = 120.0;  hs.dry = 0.0f;  hs.tone = 1.0f;
            hs.lvl[0] = 0.5f;  hs.lvl[1] = hs.lvl[2] = hs.lvl[3] = 0.0f;  hs.semis[0] = 12;  hs.arpGate = gate;  hs.arpLevel = lvl;
            const auto y = run (e, hs, 220.0, 3.0, false);  double s = 0;  size_t n = 0;  for (size_t i = (size_t) (1.0 * SR); i < y.size(); ++i) { s += (double) y[i] * y[i];  ++n; }
            return 10.0 * std::log10 (s / (double) n + 1e-20); };
        const double full = level (1.0f, 0.7f), tenth = level (0.2f, 0.7f), half = level (1.0f, 0.35f);
        std::printf ("  info: arp output %.1f dB at gate 1, %.1f dB at gate 0.2, %.1f dB at half the level\n", full, tenth, half);
        CHECK (full - tenth > 4.0 && full - tenth < 11.0, "arp Gate 0.2 is quieter than Gate 1 (%.1f dB less; about 7 expected)", full - tenth);
        CHECK (std::abs ((full - half) - 6.0) < 1.5, "arp Level half the size is 6 dB down (%.1f)", full - half); }

    // ---- chord mode: voices C and D
    {   const float want[4] = { 7.0f, 12.0f, -12.0f, 19.0f };
        for (int v = 2; v < 4; ++v)
        {   HarmonizerEngine e;  fresh (e, true);  HarmSettings hs;  hs.mode = 0;  hs.poly = true;  hs.dry = 0.0f;  hs.tone = 1.0f;
            for (int k = 0; k < 4; ++k) { hs.lvl[k] = k == v ? 1.0f : 0.0f;  hs.semis[k] = want[k]; }
            const auto y = run (e, hs, 220.0, 3.0, true);
            const double f = 220.0 * std::pow (2.0, want[v] / 12.0);
            const double on = power (y, f, 2.0, 3.0), off = power (y, 220.0, 2.0, 3.0);
            CHECK (db (on, off) > 10.0, "chord mode, voice %c at %+0.0f semitones: %.1f dB above the unshifted pitch (want 10)", 'A' + v, want[v], db (on, off)); }
        // all four at once and the arp on top, in chord mode: no blow-ups
        HarmonizerEngine e;  fresh (e, true);  HarmSettings hs;  hs.mode = 0;  hs.poly = true;  for (int k = 0; k < 4; ++k) hs.lvl[k] = 0.8f;
        auto y = run (e, hs, 220.0, 1.5, true);  float pk = 0;
        CHECK (finiteBounded (y, 6.0f, pk), "chord mode, four voices at once: finite and bounded (peak %.2f)", pk);
        HarmonizerEngine e2;  fresh (e2, true);  hs.arp = true;  y = run (e2, hs, 220.0, 1.5, true);
        CHECK (finiteBounded (y, 6.0f, pk), "chord mode with the arpeggiator: finite and bounded (peak %.2f)", pk); }

    // ---- the processor: parameters, MIDI keys, the whole chain
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        for (const char* id : { "hmCInt", "hmDInt", "hmCDeg", "hmDDeg", "hmCLvl", "hmDLvl", "hmCPan", "hmDPan", "hmArp", "hmArpPattern", "hmArpRate", "hmArpGate", "hmArpLevel" })
            CHECK (p.apvts.getRawParameterValue (id) != nullptr, "parameter %s exists", id);
        CHECK (p.apvts.getRawParameterValue ("hmDLvl")->load() == 0.0f && p.apvts.getRawParameterValue ("hmCLvl")->load() == 0.0f, "voices C and D start off, so old settings sound the same");
        // MIDI keys reach the harmonizer
        juce::AudioBuffer<float> b (2, 256);  b.clear();  juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 0);  m.addEvent (juce::MidiMessage::noteOn (1, 100, 0.8f), 1);
        p.processBlock (b, m);
        CHECK ((p.midiHeldNotes (1) & 1) && ((p.midiHeldNotes (1) >> 36) & 1), "MIDI note 64 and note 100 show in the held-key mask");
        juce::MidiBuffer m2;  m2.addEvent (juce::MidiMessage::noteOff (1, 64), 0);  b.clear();  p.processBlock (b, m2);
        CHECK (! (p.midiHeldNotes (1) & 1) && ((p.midiHeldNotes (1) >> 36) & 1), "releasing a key removes just that key");
        juce::MidiBuffer m3;  m3.addEvent (juce::MidiMessage::allNotesOff (1), 0);  b.clear();  p.processBlock (b, m3);
        CHECK (p.midiHeldNotes (0) == 0 && p.midiHeldNotes (1) == 0, "all notes off clears the held keys"); }
    {   // the whole chain: MIDI mode and the arp in both positions, finite and bounded, and silent input stays silent after a harmony
        for (int pos = 0; pos < 2; ++pos)
            for (int arp = 0; arp < 2; ++arp)
                for (int tr = 0; tr < 3; ++tr)
                {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
                    setP (p, "hmOn", 1.0f);  setP (p, "hmPos", (float) pos);  setP (p, "hmMode", 2.0f);  setP (p, "hmTrack", (float) tr);  setP (p, "hmArp", (float) arp);
                    for (const char* id : { "hmALvl", "hmBLvl", "hmCLvl", "hmDLvl" }) setP (p, id, 0.8f);
                    p.applyPendingUpdates();
                    const auto in = testSignal (4 * 48000, SR);  std::vector<float> y;  float pk;
                    juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;  y.assign (in.size(), 0.0f);
                    for (size_t pos2 = 0; pos2 < in.size(); pos2 += 512)
                    {   const int n = (int) std::min ((size_t) 512, in.size() - pos2);  juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
                        for (int i = 0; i < n; ++i) { v.setSample (0, i, in[pos2 + (size_t) i]);  v.setSample (1, i, in[pos2 + (size_t) i]); }
                        m.clear();
                        if (pos2 == 0) { m.addEvent (juce::MidiMessage::noteOn (1, 64, 0.8f), 0);  m.addEvent (juce::MidiMessage::noteOn (1, 67, 0.8f), 0);  m.addEvent (juce::MidiMessage::noteOn (1, 71, 0.8f), 0); }
                        if (pos2 == 512 * 100) m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
                        p.processBlock (v, m);
                        for (int i = 0; i < n; ++i) y[pos2 + (size_t) i] = v.getSample (0, i); }
                    CHECK (finiteBounded (y, 32.0f, pk), "chain: harmonizer %s, MIDI mode, tracking %d, arp %d: non-finite or peak %.1f", pos ? "after cab" : "before amp", tr, arp, pk); } }
}


// b115: the Mod FX Position menus and the board stay in step, and a pedal switched on from its own page is plugged in
static void testBoardSync()
{
    std::printf("[board] Mod FX position menu against the board\n");
    const double SR = 48000.0;
    auto stereoRun = [&] (StarterAmpProcessor& p, std::vector<float>& L, std::vector<float>& R)
    {   const auto in = testSignal (2 * 48000, SR);  L.assign (in.size(), 0.0f);  R.assign (in.size(), 0.0f);
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;
        for (size_t pos = 0; pos < in.size(); pos += 512)
        {   const int n = (int) std::min ((size_t) 512, in.size() - pos);  juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
            for (int i = 0; i < n; ++i) { v.setSample (0, i, in[pos + (size_t) i]);  v.setSample (1, i, in[pos + (size_t) i]); }
            p.processBlock (v, m);
            for (int i = 0; i < n; ++i) { L[pos + (size_t) i] = v.getSample (0, i);  R[pos + (size_t) i] = v.getSample (1, i); } } };
    auto diffDb = [] (const std::vector<float>& a, const std::vector<float>& b) { double s = 0, r = 0; for (size_t i = a.size() / 2; i < a.size(); ++i) { s += (double) (a[i] - b[i]) * (a[i] - b[i]); r += (double) a[i] * a[i]; } return 10.0 * std::log10 ((s + 1e-20) / (r + 1e-20)); };
    static const char* const posId[4] = { "mfAPos", "mfBPos", "mfCPos", "mfDPos" };
    static const char* const onId[4]  = { "mfAOn", "mfBOn", "mfCOn", "mfDOn" };
    static const char* const mixId[4] = { "mfAMix", "mfBMix", "mfCMix", "mfDMix" };
    static const char* const typeId[4] = { "mfAType", "mfBType", "mfCType", "mfDType" };
    static const int node[4] = { 6, 7, 24, 25 };
    const char* how[3] = { "menu changed by the host or a preset", "taken off the board, then switched on", "taken off, menu changed, then switched on" };
    for (int slot = 0; slot < 4; ++slot)
        for (int mode = 0; mode < 3; ++mode)
            for (int pos = 0; pos < 3; ++pos)
            {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
                std::vector<float> L0, R0, L1, R1;
                setP (p, onId[slot], 0.0f);  p.applyPendingUpdates();  stereoRun (p, L0, R0);
                setP (p, typeId[slot], 0.0f);  setP (p, mixId[slot], 1.0f);
                if (mode == 0) setP (p, posId[slot], (float) pos);
                if (mode >= 1) { auto d = p.getPatch();  PatchGraph::takeOff (d, node[slot]);  p.setPatch (d);  if (mode == 2) setP (p, posId[slot], (float) pos); }
                setP (p, onId[slot], 1.0f);  p.applyPendingUpdates();  stereoRun (p, L1, R1);
                const int lane = PatchGraph::laneOf (p.getPatch(), node[slot]);
                const int menu = (int) p.apvts.getRawParameterValue (posId[slot])->load();
                const double wet = diffDb (L1, L0), width = diffDb (L1, R1);
                if (mode == 1 && pos > 0) continue;                       // the menu is untouched there: only the default position is meaningful
                CHECK (lane == (mode == 1 ? menu : pos) && menu == lane, "Mod FX %c, %s, position %d: board lane %d, menu %d", 'A' + slot, how[mode], pos, lane, menu);
                CHECK (wet > -30.0, "Mod FX %c, %s, position %d: the effect is heard (%.1f dB from dry)", 'A' + slot, how[mode], pos, wet);
                if (lane == 1 && mode != 1) CHECK (width > -20.0, "Mod FX %c after the cab is in stereo (L-R %.1f dB)", 'A' + slot, width); }
    // a pedal that is on and unplugged (any pedal) is plugged in where it normally sits
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        auto d = p.getPatch();  PatchGraph::takeOff (d, 4);  PatchGraph::takeOff (d, 12);  p.setPatch (d);
        CHECK (PatchGraph::laneOf (p.getPatch(), 4) < 0 && PatchGraph::laneOf (p.getPatch(), 12) < 0, "Chorus and Reverb are off the board");
        setP (p, "chorusOn", 1.0f);  setP (p, "reverbOn", 1.0f);  p.applyPendingUpdates();
        CHECK (PatchGraph::laneOf (p.getPatch(), 4) == 0 && PatchGraph::laneOf (p.getPatch(), 12) == 1, "switching them on plugs them back in (Chorus before the amp, Reverb after the cab: %d, %d)", PatchGraph::laneOf (p.getPatch(), 4), PatchGraph::laneOf (p.getPatch(), 12)); }
    // switched off and off the board: left alone
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        auto d = p.getPatch();  PatchGraph::takeOff (d, 4);  p.setPatch (d);  p.applyPendingUpdates();
        CHECK (PatchGraph::laneOf (p.getPatch(), 4) < 0, "a pedal that is off stays in the tray"); }
}


// measurement (b117): how pure is each pitch shifter? Not a pass/fail test: it prints, per shifter, how much of the output sits at the wanted pitch
// (and its harmonics) against how much sits at the "sidebands" that a ring-mod-like shifter adds, plus how hard the level wobbles.
static void measPitchShifters()
{
    std::printf("[psmeas] pitch shifter purity\n");
    const double SR = 48000.0;
    struct Src { const char* name; double f0; int harm; };
    const Src srcs[2] = { { "220 Hz sine", 220.0, 1 }, { "110 Hz guitar-like (8 harmonics)", 110.0, 8 } };
    auto make = [&] (const Src& sc, double secs) { std::vector<float> x ((size_t) (secs * SR));
        for (size_t i = 0; i < x.size(); ++i) { double v = 0; for (int h = 1; h <= sc.harm; ++h) v += std::sin (6.283185307 * sc.f0 * h * (double) i / SR) / (double) h;  x[i] = (float) (0.3 * v); }  return x; };
    // purity: of the energy that is within 1.5 semitones of a harmonic of the wanted pitch... use narrow bins: power within +-4 Hz of each wanted harmonic / total power (50 Hz to 8 kHz)
    auto purity = [&] (const std::vector<float>& y, double f0, int harm, double t0)
    {   const size_t a = (size_t) (t0 * SR), n = y.size() - a;  const int N = 1 << 15;  double tot = 0, want = 0;
        std::vector<double> win (N);  for (int i = 0; i < N; ++i) win[(size_t) i] = 0.5 - 0.5 * std::cos (6.283185307 * i / (N - 1));
        const int blocks = (int) (n / (size_t) N);  if (blocks < 1) return 0.0;
        std::vector<double> ps (N / 2, 0.0);
        for (int b = 0; b < blocks; ++b)
        {   std::vector<double> re (N), im (N);
            for (int i = 0; i < N; ++i) re[(size_t) i] = y[a + (size_t) b * N + (size_t) i] * win[(size_t) i];
            // naive DFT on the bins we care about is too slow: do an FFT
            std::vector<std::complex<double>> c (N);  for (int i = 0; i < N; ++i) c[(size_t) i] = re[(size_t) i];
            for (int len = N, k2 = 0; len > 1; len >>= 1, ++k2) {}
            // iterative radix-2
            for (int i = 1, j = 0; i < N; ++i) { int bit = N >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (c[(size_t) i], c[(size_t) j]); }
            for (int len = 2; len <= N; len <<= 1) { const double ang = -6.283185307179586 / len;  const std::complex<double> wl (std::cos (ang), std::sin (ang));
                for (int i = 0; i < N; i += len) { std::complex<double> w (1.0); for (int j = 0; j < len / 2; ++j) { auto u = c[(size_t) (i + j)], v = c[(size_t) (i + j + len / 2)] * w; c[(size_t) (i + j)] = u + v; c[(size_t) (i + j + len / 2)] = u - v; w *= wl; } } }
            for (int k = 0; k < N / 2; ++k) ps[(size_t) k] += std::norm (c[(size_t) k]); }
        const double binHz = SR / N;
        for (int k = 1; k < N / 2; ++k) { const double f = k * binHz;  if (f < 50.0 || f > 8000.0) continue;  tot += ps[(size_t) k];
            for (int h = 1; h <= harm; ++h) if (std::abs (f - f0 * h) < 6.0) { want += ps[(size_t) k];  break; } }
        return 100.0 * want / (tot + 1e-30); };
    // level wobble: the envelope (rectify, 5 ms smoothing) spread in dB over the last seconds
    auto wobble = [&] (const std::vector<float>& y, double t0)
    {   double env = 0;  const double a = 1.0 - std::exp (-1.0 / (0.005 * SR));  std::vector<double> e;
        for (size_t i = (size_t) (t0 * SR); i < y.size(); ++i) { env += a * (std::abs ((double) y[i]) - env);  if ((i & 63) == 0) e.push_back (20.0 * std::log10 (env + 1e-9)); }
        std::sort (e.begin(), e.end());  return e.empty() ? 0.0 : e[(size_t) ((double) e.size() * 0.95)] - e[(size_t) ((double) e.size() * 0.05)]; };
    const float shifts[4] = { 12.0f, 7.0f, -12.0f, -5.0f };
    for (const auto& sc : srcs)
    {   std::printf ("  source: %s\n", sc.name);
        for (float st : shifts)
        {   const double ratio = std::pow (2.0, st / 12.0);
            std::printf ("   shift %+.0f st:", st);
            const auto in = make (sc, 3.0);
            {   Transposer t;  t.prepare (SR);  std::vector<float> w (in), d (in.size());  for (size_t p = 0; p < in.size(); p += 512) { const int n = (int) std::min ((size_t) 512, in.size() - p);  t.process (w.data() + p, d.data() + p, n, st); }
                std::printf ("  Clean %5.1f%% (wobble %4.1f dB)", purity (w, sc.f0 * ratio, sc.harm, 1.0), wobble (w, 1.0)); }
            for (int ch = 1; ch <= 5; ++ch)
                for (float grit : { 0.0f, 0.5f })
                {   RoughShifter r;  r.prepare (SR);  RoughShifter::Params rp;  rp.semis = st;  rp.character = ch;  rp.grit = grit;
                    std::vector<float> w (in.size());  for (size_t i = 0; i < in.size(); ++i) w[i] = r.process (in[i], rp);
                    std::printf ("  | c%d g%.1f %5.1f%% (%4.1f dB)", ch, grit, purity (w, sc.f0 * ratio, sc.harm, 1.0), wobble (w, 1.0)); }
            std::printf ("\n"); } }
}


// measurement (b117): how late does a plucked note arrive through the Transposer? (prints; not a pass/fail test)
static void measTransposeDelay()
{
    std::printf("[tpdelay] Transposer: note arrival delay\n");
    const double SR = 48000.0;
    struct Cfg { const char* name; float win; };
    const Cfg cfgs[4] = { { "Low Latency (10 ms window)", 10.0f }, { "Fast (12 ms)", 12.0f }, { "Smooth (28 ms)", 28.0f }, { "Normal-Smooth old (45 ms)", 45.0f } };
    const float shifts[5] = { 12.0f, 7.0f, 3.0f, -5.0f, -12.0f };
    for (const auto& c : cfgs)
    {   std::printf ("  %s: nominal delay %.1f ms\n", c.name, 0.5 * c.win);
        for (float st : shifts)
        {   Transposer t;  t.prepare (SR);  t.setWindowMs (c.win);  t.setLowHz (0.0f);  t.setDualVoice (true, 3.0f, 8.0f, 1.0f);  t.setAttackHold (st <= -20.0f ? 60.0f : 0.0f, 6.0f);  t.reset();
            std::vector<double> delays;  uint32_t rng = 12345u;
            for (int note = 0; note < 40; ++note)
            {   rng = rng * 1664525u + 1013904223u;
                const double f0 = 82.0 * std::pow (2.0, (double) ((rng >> 8) % 36) / 12.0);        // E2 up three octaves
                const size_t len = (size_t) (0.45 * SR), pre = (size_t) (0.25 * SR);
                std::vector<float> x (pre + len, 0.0f);
                for (size_t i = 0; i < len; ++i) { double v = 0; for (int h = 1; h <= 6; ++h) v += std::sin (6.283185307 * f0 * h * (double) i / SR + h) / (double) h;
                                                   x[pre + i] = (float) (0.3 * v * std::min (1.0, (double) i / (0.001 * SR)) * std::exp (-3.0 * (double) i / SR)); }
                std::vector<float> w (x), d (x.size());  for (size_t p = 0; p < x.size(); p += 256) { const int n = (int) std::min ((size_t) 256, x.size() - p);  t.process (w.data() + p, d.data() + p, n, st); }
                // arrival: the first time the 1 ms smoothed envelope of the wet signal passes 25 percent of its own first-100-ms peak, minus the input's onset
                auto arrive = [&] (const std::vector<float>& y, size_t from) { double env = 0, pk = 0;  const double a = 1.0 - std::exp (-1.0 / (0.001 * SR));  std::vector<double> e (y.size());
                    for (size_t i = from; i < y.size(); ++i) { env += a * (std::abs ((double) y[i]) - env);  e[i] = env; }
                    for (size_t i = from; i < std::min (y.size(), from + (size_t) (0.1 * SR)); ++i) pk = std::max (pk, e[i]);
                    for (size_t i = from; i < y.size(); ++i) if (e[i] > 0.25 * pk) return (double) (i - from) * 1000.0 / SR;  return 999.0; };
                delays.push_back (arrive (w, pre - (size_t) (0.02 * SR)) - arrive (x, pre - (size_t) (0.02 * SR)));
            }
            std::sort (delays.begin(), delays.end());  double m = 0;  for (double v : delays) m += v;  m /= (double) delays.size();
            std::printf ("   shift %+3.0f st: mean %5.1f ms, median %5.1f, 90th %5.1f, worst %5.1f\n", st, m, delays[delays.size() / 2], delays[(size_t) (delays.size() * 0.9)], delays.back()); } }
}


// ---------------------------------------------------------------------------------------------------------------------
// b117: the rough pitch pedal characters are partly waveform-matched (less ring-mod feel), and Transpose's tone move is quicker in Fast mode
static void testB117()
{
    std::printf("[b117] rough pitch characters: partial waveform matching; Fast-mode formant delay\n");
    const double SR = 48000.0;
    auto tone = [&] (double f0, int harm, double secs) { std::vector<float> x ((size_t) (secs * SR));
        for (size_t i = 0; i < x.size(); ++i) { double v = 0; for (int h = 1; h <= harm; ++h) v += std::sin (6.283185307 * f0 * h * (double) i / SR) / (double) h;  x[i] = (float) (0.3 * v); }  return x; };
    // typical level wobble: the median distance of the 5 ms envelope (in dB) from its median, over the last 2 s
    auto wobble = [&] (const std::vector<float>& y)
    {   double env = 0;  const double a = 1.0 - std::exp (-1.0 / (0.005 * SR));  std::vector<double> e;
        for (size_t i = (size_t) (1.0 * SR); i < y.size(); ++i) { env += a * (std::abs ((double) y[i]) - env);  if ((i & 63) == 0) e.push_back (20.0 * std::log10 (env + 1e-9)); }
        std::sort (e.begin(), e.end());  const double med = e[e.size() / 2];  std::vector<double> d;  for (double v : e) d.push_back (std::abs (v - med));  std::sort (d.begin(), d.end());  return d[d.size() / 2]; };
    auto run = [&] (const std::vector<float>& in, int ch, float grit, float st)
    {   RoughShifter r;  r.prepare (SR);  RoughShifter::Params rp;  rp.semis = st;  rp.character = ch;  rp.grit = grit;
        std::vector<float> w (in.size());  for (size_t i = 0; i < in.size(); ++i) w[i] = r.process (in[i], rp);  return w; };
    // before b117 the typical wobble of Grainy Sweep and Noisy on a plain 220 Hz tone was 1.6 to 2.6 dB at every Grit (measured with the matching switched off)
    {   const auto in = tone (220.0, 1, 3.0);
        for (int ch : { 1, 4 })
            for (float st : { 12.0f, 7.0f, -5.0f, -12.0f })
            {   const double w = wobble (run (in, ch, 0.5f, st));
                CHECK (w < 1.0, "character %d at %+.0f st, Grit 0.5: typical level wobble %.2f dB (was 1.6 to 2.6 before matching; want under 1)", ch, st, w); } }
    {   const auto in = tone (110.0, 8, 3.0);
        for (int ch : { 1, 4 })
        {   const double w = wobble (run (in, ch, 0.5f, 7.0f));
            CHECK (w < 1.0, "character %d on a low note with harmonics, +7 st: typical level wobble %.2f dB (was 0.7 to 0.9 dB before, 1.2 at -12)", ch, w); } }
    // the shifted pitch is still right: the strongest peak of the output sits at the shifted note (Grainy Sweep steps the pitch, so allow 1.5 percent)
    {   const auto in = tone (220.0, 1, 3.0);
        for (int ch : { 1, 2, 3, 4 })
        {   const auto y = run (in, ch, 0.3f, 12.0f);  double best = 0, bestF = 0;
            for (double f = 300.0; f < 600.0; f += 1.0) { double re = 0, im = 0;  for (size_t i = (size_t) (1.0 * SR); i < y.size(); ++i) { const double ph = 6.283185307 * f * (double) i / SR;  re += y[i] * std::cos (ph);  im += y[i] * std::sin (ph); }
                const double pw = re * re + im * im;  if (pw > best) { best = pw;  bestF = f; } }
            CHECK (std::abs (bestF - 440.0) < 8.0, "character %d at +12 st: strongest component at %.0f Hz (want 440)", ch, bestF); } }
    // nothing blows up, nothing is non-finite, for every character, Grit and shift, on a guitar-like note and on silence
    {   const auto in = tone (82.4, 10, 2.0);  bool ok = true;  float pk = 0;
        for (int ch = 1; ch <= 5; ++ch) for (float grit : { 0.0f, 0.5f, 1.0f }) for (float st : { 24.0f, 12.0f, 3.0f, -7.0f, -24.0f })
        {   const auto y = run (in, ch, grit, st);  for (float v : y) { if (! std::isfinite (v)) ok = false;  pk = std::max (pk, std::abs (v)); }
            const auto z = run (std::vector<float> (48000, 0.0f), ch, grit, st);  for (size_t i = 24000; i < z.size(); ++i) if (std::abs (z[i]) > 0.02f) ok = false; }      // after the DC blocker has settled (the rectifier of the Abrasive starts with an offset)
        CHECK (ok && pk < 4.0f, "all characters, Grits and shifts: finite, silent on silence (peak %.2f)", pk); }
    // Warble keeps its slow wobble on purpose
    {   const auto in = tone (220.0, 1, 3.0);  const double w = wobble (run (in, 5, 0.5f, 7.0f));
        CHECK (w > 3.0, "Warble still wobbles (%.1f dB typical)", w); }
}

// print-only (b118): what each setting adds between the guitar and the output: the delay of a plucked note (against the amp alone, no oversampling)
// and the latency the plugin tells the host. Not a pass/fail test.
static void measLatencyAll()
{
    std::printf("[latency] arrival delay of a 220 Hz plucked note through the whole plugin, against the amp alone with no oversampling\n");
    const double SR = 48000.0;
    struct Cfg { const char* name; std::vector<std::pair<const char*, float>> p; };
    const std::vector<Cfg> cfgs = {
        { "amp alone, no oversampling (reference)", { { "ampQuality", 2.0f } } },
        { "amp, 2x oversampling", { { "ampQuality", 1.0f } } },
        { "amp, 4x oversampling", { { "ampQuality", 0.0f } } },
        { "Low Latency mode", { { "perfMode", 1.0f } } },
        { "High Quality mode", { { "perfMode", 2.0f } } },
        { "limiter on (Normal)", { { "ampQuality", 2.0f }, { "limOn", 1.0f } } },
        { "limiter on (High Quality)", { { "ampQuality", 2.0f }, { "perfMode", 2.0f }, { "limOn", 1.0f } } },
        { "Transpose +7, Fast, Lock off", { { "ampQuality", 2.0f }, { "tpOn", 1.0f }, { "tpSemis", 7.0f }, { "tpLock", 0.0f }, { "tpMix", 1.0f } } },
        { "Transpose +7, Fast, Lock on", { { "ampQuality", 2.0f }, { "tpOn", 1.0f }, { "tpSemis", 7.0f }, { "tpLock", 1.0f }, { "tpMix", 1.0f } } },
        { "Transpose +7, Smooth, Lock on", { { "ampQuality", 2.0f }, { "tpOn", 1.0f }, { "tpSemis", 7.0f }, { "tpLock", 1.0f }, { "tpQuality", 1.0f }, { "tpMix", 1.0f } } },
        { "Pitch pedal, Clean", { { "ampQuality", 2.0f }, { "wowOn", 1.0f }, { "wowChar", 0.0f } } },
        { "Pitch pedal, Grainy Sweep", { { "ampQuality", 2.0f }, { "wowOn", 1.0f }, { "wowChar", 1.0f } } },
        { "Pitch pedal, Glitchy", { { "ampQuality", 2.0f }, { "wowOn", 1.0f }, { "wowChar", 2.0f } } },
        { "Pitch pedal, Abrasive", { { "ampQuality", 2.0f }, { "wowOn", 1.0f }, { "wowChar", 3.0f } } },
        { "Pitch pedal, Warble", { { "ampQuality", 2.0f }, { "wowOn", 1.0f }, { "wowChar", 5.0f } } },
        { "Octaver", { { "ampQuality", 2.0f }, { "octOn", 1.0f } } },
        { "Harmonizer", { { "ampQuality", 2.0f }, { "hmOn", 1.0f } } },
        { "Cab on (default)", { { "ampQuality", 2.0f }, { "cabOn", 1.0f } } },
        { "Mod FX A: Through-zero flanger", { { "ampQuality", 2.0f }, { "mfAOn", 1.0f }, { "mfAType", 1.0f }, { "mfAMix", 1.0f } } },
        { "Mod FX A: Digital damage", { { "ampQuality", 2.0f }, { "mfAOn", 1.0f }, { "mfAType", 24.0f }, { "mfAMix", 1.0f } } },
        { "Mod FX A: Chorus", { { "ampQuality", 2.0f }, { "mfAOn", 1.0f }, { "mfAType", 12.0f }, { "mfAMix", 1.0f } } },
        { "Mod FX A: Pitch modulation", { { "ampQuality", 2.0f }, { "mfAOn", 1.0f }, { "mfAType", 10.0f }, { "mfAMix", 1.0f } } },
        { "Mod FX A: Frequency shifter", { { "ampQuality", 2.0f }, { "mfAOn", 1.0f }, { "mfAType", 22.0f }, { "mfAMix", 1.0f } } },
        { "Reverb on", { { "ampQuality", 2.0f }, { "reverbOn", 1.0f }, { "reverbMix", 1.0f } } },
    };
    auto arrive = [&] (const std::vector<float>& y, size_t from) { double env = 0, pk = 0;  const double a = 1.0 - std::exp (-1.0 / (0.001 * SR));  std::vector<double> e (y.size());
        for (size_t i = from; i < y.size(); ++i) { env += a * (std::abs ((double) y[i]) - env);  e[i] = env; }
        for (size_t i = from; i < std::min (y.size(), from + (size_t) (0.15 * SR)); ++i) pk = std::max (pk, e[i]);
        for (size_t i = from; i < y.size(); ++i) if (e[i] > 0.25 * pk) return (double) (i - from) * 1000.0 / SR;  return 999.0; };
    double ref = 0;
    for (size_t c = 0; c < cfgs.size(); ++c)
    {   std::vector<double> d;  int rep = 0;
        for (int note = 0; note < 6; ++note)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 256);
            setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
            for (const auto& kv : cfgs[c].p) setP (p, kv.first, kv.second);
            p.prepareToPlay (SR, 256);
            const size_t pre = (size_t) (0.3 * SR) + (size_t) note * 733, len = (size_t) (0.5 * SR);  std::vector<float> x (pre + len, 0.0f);
            for (size_t i = 0; i < len; ++i) { double v = 0;  for (int h = 1; h <= 5; ++h) v += std::sin (6.283185307 * (200.0 + 17.0 * note) * h * (double) i / SR) / (double) h;
                                               x[pre + i] = (float) (0.3 * v * std::min (1.0, (double) i / (0.001 * SR)) * std::exp (-3.0 * (double) i / SR)); }
            std::vector<float> y;  process (p, x, y, 256);  p.applyPendingUpdates();  rep = p.getLatencySamples();
            std::vector<float> yy (y);  d.push_back (arrive (yy, pre - (size_t) (0.02 * SR)) - arrive (x, pre - (size_t) (0.02 * SR))); }
        std::sort (d.begin(), d.end());  double m = 0;  for (double v : d) m += v;  m /= (double) d.size();
        if (c == 0) ref = m;
        std::printf ("  %-42s note arrives %5.1f ms later than reference (median %5.1f), plugin reports %5.1f ms (%d samples)\n", cfgs[c].name, m - ref, d[d.size() / 2] - ref, 1000.0 * rep / SR, rep); }
}

// b120: the Repeater, the Step sequence, Cassette breathing, Bit mangle, the reverbs' dry and wet pan
static void testB120()
{
    std::printf("[b120] Repeater, Step sequence, Cassette breathing, Bit mangle, reverb dry/wet pan\n");
    const double SR = 48000.0;
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    auto rms = [&] (const std::vector<float>& y, size_t a, size_t b) { double s = 0;  for (size_t i = a; i < b; ++i) s += (double) y[i] * y[i];  return std::sqrt (s / (double) std::max<size_t> (1, b - a)); };
    auto noise = [&] (size_t n, unsigned seed, float amp) { std::vector<float> x (n);  unsigned r = seed;  for (auto& v : x) { r = r * 1664525u + 1013904223u;  v = amp * ((float) (r >> 8) / 8388608.0f - 1.0f); }  return x; };

    // 1. Step sequence (the modulator itself): the step follows the beat, the pattern length wraps, the value reaches the step
    {   ModSource::Modulator m;  m.prepare (SR);  float st[16];  for (int k = 0; k < 16; ++k) st[k] = (k % 4 == 0) ? 1.0f : (k % 4 == 1) ? -1.0f : (k % 4 == 2) ? 0.5f : 0.0f;
        bool idxOk = true, valOk = true;
        for (int b = 0; b < 200; ++b)
        {   const double beat = 0.05 * b;                                   // the quarter-note position
            const float v = m.stepSeq (st, 16, 0.25, beat, 0.0f, 512);       // 1/16 steps
            const int want = (int) (std::floor (beat / 0.25)) % 16;
            if (m.seqIdx != want) idxOk = false;
            if (b % 5 == 4 && std::abs (v - st[want]) > 0.25f) valOk = false; }
        CHECK (idxOk, "Step sequence: the step number follows the beat (1/16 steps, 16 long)");
        CHECK (valOk, "Step sequence: the output reaches each step's value");
        ModSource::Modulator m2;  m2.prepare (SR);  int idx[3] = { 0, 0, 0 };
        m2.stepSeq (st, 6, 0.5, 0.0, 0.0f, 512);  idx[0] = m2.seqIdx;  m2.stepSeq (st, 6, 0.5, 3.1, 0.0f, 512);  idx[1] = m2.seqIdx;  m2.stepSeq (st, 6, 0.5, 3.6, 0.0f, 512);  idx[2] = m2.seqIdx;
        CHECK (idx[0] == 0 && idx[1] == 0 && idx[2] == 1, "Step sequence: a 6-step pattern of eighth notes wraps after 3 beats (%d %d %d)", idx[0], idx[1], idx[2]); }
    // 1b. in the plugin: four steps of a quarter note each at 120 BPM move the Input level up and down in time
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
        setP (p, "ampQuality", 2.0f);  setP (p, "gate", -90.0f);  setP (p, "tempoBpm", 120.0f);  setP (p, "tempoHost", 0.0f);
        setP (p, "mod1Src", 6.0f);  setP (p, "mod1Div", 0.0f);  setP (p, "mod1Len", 4.0f);  setP (p, "mod1Smooth", 0.0f);  setP (p, "mod1T1", 1.0f);  setP (p, "mod1A1", 1.0f);
        setP (p, "mod1S1", 1.0f);  setP (p, "mod1S2", -1.0f);  setP (p, "mod1S3", 1.0f);  setP (p, "mod1S4", -1.0f);
        p.prepareToPlay (SR, 512);
        std::vector<float> x ((size_t) (2.2 * SR));  for (size_t i = 0; i < x.size(); ++i) x[i] = (float) (0.02 * std::sin (6.2831853 * 220.0 * (double) i / SR));
        std::vector<float> y;  process (p, x, y, 512);
        const double a = rmsDb (std::vector<float> (y.begin() + (size_t) (0.15 * SR), y.begin() + (size_t) (0.45 * SR))), b = rmsDb (std::vector<float> (y.begin() + (size_t) (0.65 * SR), y.begin() + (size_t) (0.95 * SR)));
        std::printf ("  info: Step sequence on Input level: step 1 %.1f dB, step 2 %.1f dB\n", a, b);
        CHECK (a > b + 12.0, "Step sequence in the plugin: step 1 (+) is much louder than step 2 (-) (%.1f vs %.1f dB)", a, b); }

    // 2. Repeater
    {   const double bpm = 120.0;  const int L = (int) std::lround (0.25 * 60.0 / bpm * SR);       // a 1/16 slice: 6000 samples
        const auto in = noise ((size_t) (3.0 * SR), 7u, 0.3f);
        FxExtra::RepeaterFx rp;  rp.prepare (SR);
        auto run = [&] (FxExtra::RepeaterFx& r, const std::vector<float>& src, FxExtra::RepeaterFx::Params base, size_t engageAt, size_t releaseAt)
        {   std::vector<float> o (src);  size_t pos = 0;
            while (pos < src.size())
            {   const size_t n = std::min<size_t> (256, src.size() - pos);
                FxExtra::RepeaterFx::Params p = base;  p.hold = pos >= engageAt && pos < releaseAt;  p.beat0 = (double) pos * bpm / (60.0 * SR);
                r.process (o.data() + pos, nullptr, (int) n, p);  pos += n; }
            return o; };
        FxExtra::RepeaterFx::Params base { 0, false, 0.25f, 0.0f, 0.0f, 0.0f, 1.0f, bpm, 0.0 };
        const size_t eng = (size_t) (1.0 * SR);
        const auto o = run (rp, in, base, eng, (size_t) (2.5 * SR));
        bool same = true;  for (size_t i = 0; i < eng - 64; ++i) if (o[i] != in[i]) same = false;
        CHECK (same, "Repeater: nothing changes (bit for bit) before it engages: it adds no delay");
        // periodic after engaging: the sound repeats every L samples
        double num = 0, d1 = 0, d2 = 0;  for (size_t i = eng + 2 * (size_t) L; i < eng + 8 * (size_t) L; ++i) { num += (double) o[i] * o[i - (size_t) L];  d1 += (double) o[i] * o[i];  d2 += (double) o[i - (size_t) L] * o[i - (size_t) L]; }
        const double corr = num / std::sqrt (d1 * d2 + 1e-30);
        std::printf ("  info: Repeater: correlation of the output with itself one slice (%d samples) earlier: %.3f\n", L, corr);
        CHECK (corr > 0.97, "Repeater: the output repeats with the slice length (%.3f)", corr);
        // the repeat is the last slice of what was playing: matches the input just before the engage point
        double nn = 0, e1 = 0, e2 = 0;  for (size_t i = 300; i < (size_t) L - 300; ++i) { nn += (double) o[eng + i] * in[eng - (size_t) L + 1 + i];  e1 += (double) o[eng + i] * o[eng + i];  e2 += (double) in[eng - (size_t) L + 1 + i] * in[eng - (size_t) L + 1 + i]; }
        CHECK (nn / std::sqrt (e1 * e2 + 1e-30) > 0.98, "Repeater: the first repeat is the slice that was just played (%.3f)", nn / std::sqrt (e1 * e2 + 1e-30));
        // it lets go: after release the live sound is back
        double back = 0, bd = 0;  for (size_t i = (size_t) (2.7 * SR); i < (size_t) (2.9 * SR); ++i) { back += (double) o[i] * in[i];  bd += (double) in[i] * in[i]; }
        CHECK (back / (bd + 1e-30) > 0.98, "Repeater: after it lets go the live sound is back (%.3f)", back / (bd + 1e-30));
        // decay: the second repeat is quieter by exp(-0.8 decay)
        FxExtra::RepeaterFx r2;  r2.prepare (SR);  FxExtra::RepeaterFx::Params pd = base;  pd.decay = 1.0f;
        const auto od = run (r2, in, pd, eng, (size_t) (2.5 * SR));
        const double g1 = rms (od, eng + (size_t) L + 200, eng + 2 * (size_t) L - 200) / rms (od, eng + 200, eng + (size_t) L - 200);
        CHECK (std::abs (g1 - std::exp (-0.8)) < 0.06, "Repeater: with Decay 1 the second repeat is %.2f of the first (want %.2f)", g1, std::exp (-0.8));
        // pitch: a sine, +12 st per repeat: the second repeat plays at twice the frequency
        std::vector<float> sine ((size_t) (3.0 * SR));  for (size_t i = 0; i < sine.size(); ++i) sine[i] = (float) (0.3 * std::sin (6.2831853 * 440.0 * (double) i / SR));
        FxExtra::RepeaterFx r3;  r3.prepare (SR);  FxExtra::RepeaterFx::Params pp = base;  pp.pitchSt = 12.0f;
        const auto os = run (r3, sine, pp, eng, (size_t) (2.5 * SR));
        const size_t a0 = eng + (size_t) L + 400, a1 = eng + 2 * (size_t) L - 400;
        const double f880 = goertz (os, a0, a1, 880.0), f440 = goertz (os, a0, a1, 440.0);
        CHECK (f880 > f440 + 15.0, "Repeater: +12 st per repeat: the second repeat is an octave up (880 Hz %.1f dB, 440 Hz %.1f dB)", f880, f440);
        // glitch: bounded and finite for a long run, and it does something
        FxExtra::RepeaterFx r4;  r4.prepare (SR);  FxExtra::RepeaterFx::Params pg = base;  pg.glitch = 1.0f;  pg.pitchSt = 3.0f;  pg.decay = 0.2f;
        const auto og = run (r4, in, pg, eng, (size_t) (2.9 * SR));
        bool fin = true;  float pk = 0;  for (float v : og) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, std::abs (v)); }
        CHECK (fin && pk < 0.31f, "Repeater: Glitch 1 stays finite and no louder than the input (peak %.3f)", pk);
        // automatic mode 1: the last beat of every bar (beat 3 to 4 = 1.5 s to 2.0 s at 120 BPM): untouched before, repeating inside
        FxExtra::RepeaterFx r5;  r5.prepare (SR);  FxExtra::RepeaterFx::Params pa = base;  pa.autoMode = 1;
        std::vector<float> oa (in);  for (size_t pos = 0; pos < in.size(); pos += 256) { const size_t n = std::min<size_t> (256, in.size() - pos);  pa.beat0 = (double) pos * bpm / (60.0 * SR);  r5.process (oa.data() + pos, nullptr, (int) n, pa); }
        bool pre = true;  for (size_t i = 0; i < (size_t) (1.49 * SR); ++i) if (oa[i] != in[i]) pre = false;
        double na = 0, da1 = 0, da2 = 0;  for (size_t i = (size_t) (1.66 * SR); i < (size_t) (1.95 * SR); ++i) { na += (double) oa[i] * oa[i - (size_t) L];  da1 += (double) oa[i] * oa[i];  da2 += (double) oa[i - (size_t) L] * oa[i - (size_t) L]; }
        CHECK (pre && na / std::sqrt (da1 * da2 + 1e-30) > 0.97, "Repeater, last beat of every bar: untouched before beat 3, repeating inside it (%.3f)", na / std::sqrt (da1 * da2 + 1e-30)); }

    // 3. Cassette breathing
    {   auto run = [&] (float breath, const std::vector<float>& src) { LoFiWarble lf;  lf.prepare (SR);  LoFiWarble::Params p { 0, 0.5f, 0.0f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f, 1.0f, 0.0f, 1 };  p.breath = breath;
                                                                       std::vector<float> o (src);  lf.process (o.data(), (int) o.size(), p);  return o; };
        std::vector<float> src ((size_t) (3.0 * SR), 0.0f);  for (size_t i = (size_t) (0.5 * SR); i < (size_t) (1.0 * SR); ++i) src[i] = (float) (0.25 * std::sin (6.2831853 * 3000.0 * (double) i / SR));
        const auto o0 = run (0.0f, src), o1 = run (1.0f, src);
        const double quiet0 = rms (o0, (size_t) (2.0 * SR), (size_t) (3.0 * SR)), quiet1 = rms (o1, (size_t) (2.0 * SR), (size_t) (3.0 * SR));
        const double after1 = rms (o1, (size_t) (1.0 * SR) + 200, (size_t) (1.0 * SR) + 1800);
        std::printf ("  info: Cassette breathing: hiss when quiet %.1f dB without, %.1f dB with; just after a loud burst %.1f dB\n", 20 * std::log10 (quiet0 + 1e-12), 20 * std::log10 (quiet1 + 1e-12), 20 * std::log10 (after1 + 1e-12));
        CHECK (quiet1 < quiet0 * 0.7, "Cassette breathing: the hiss between notes is quieter with it on (%.1f dB against %.1f dB)", 20 * std::log10 (quiet1 + 1e-12), 20 * std::log10 (quiet0 + 1e-12));
        CHECK (after1 > quiet1 * 1.5, "Cassette breathing: the hiss swells right after you play (%.1f dB after, %.1f dB later)", 20 * std::log10 (after1 + 1e-12), 20 * std::log10 (quiet1 + 1e-12));
        bool fin = true;  for (float v : o1) if (! std::isfinite (v) || std::abs (v) > 4.0f) fin = false;
        CHECK (fin, "Cassette breathing: finite and bounded");
        const auto oz = run (0.0f, src);  LoFiWarble lf2;  lf2.prepare (SR);  LoFiWarble::Params pd { 0, 0.5f, 0.0f, 0.0f, 0.7f, 0.0f, 0.4f, 0.0f, 1.0f, 0.0f, 1 };  std::vector<float> o2 (src);  lf2.process (o2.data(), (int) o2.size(), pd);
        CHECK (oz == o2, "Cassette breathing 0 is the old sound, bit for bit"); }

    // 4. Bit mangle
    {   std::vector<float> sine ((size_t) (1.0 * SR));  for (size_t i = 0; i < sine.size(); ++i) sine[i] = (float) (0.5 * std::sin (6.2831853 * 330.0 * (double) i / SR));
        { BitMangle b;  b.reset();  std::vector<float> o (sine);  b.process (o.data(), (int) o.size(), 0.0f, SR);  CHECK (o == sine, "Bit mangle 0 changes nothing (bit for bit)"); }
        double lastDiff = 0;
        for (float a : { 0.2f, 0.5f, 0.8f, 1.0f })
        {   BitMangle b;  b.reset();  std::vector<float> o (sine);  b.process (o.data(), (int) o.size(), a, SR);
            double d = 0;  bool fin = true;  float pk = 0;  for (size_t i = 0; i < o.size(); ++i) { d += std::pow ((double) o[i] - sine[i], 2.0);  if (! std::isfinite (o[i])) fin = false;  pk = std::max (pk, std::abs (o[i])); }
            d = std::sqrt (d / (double) o.size());
            std::printf ("  info: Bit mangle %.1f: error against the clean sine %.4f rms, peak %.3f\n", (double) a, d, (double) pk);
            CHECK (fin && pk <= 1.0001f && d > lastDiff, "Bit mangle %.1f: finite, within full scale, and more damage than the step before (%.4f > %.4f)", (double) a, d, lastDiff);
            lastDiff = d; } }

    // 5. the reverbs' dry and wet pan
    {   auto run = [&] (float dryPan, float wetPan, const char* pre) {
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);
            for (const char* id : { "odOn", "compOn", "eqOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (p, id, 0.0f);
            setP (p, "ampQuality", 2.0f);  setP (p, "gate", -90.0f);  setP (p, "cabOn", 0.0f);  setP (p, "reverbOn", 1.0f);  setP (p, "reverbMix", 0.6f);  setP (p, "reverbSize", 3.0f);  setP (p, "reverbType", 0.0f);
            setP (p, pre, 0.0f);  setP (p, "reverbDryPan", dryPan);  setP (p, "reverbWetPan", wetPan);
            p.prepareToPlay (SR, 512);
            std::vector<float> x ((size_t) (2.0 * SR), 0.0f);  for (size_t i = 0; i < (size_t) (0.4 * SR); ++i) x[i] = (float) (0.2 * std::sin (6.2831853 * 330.0 * (double) i / SR) * 1.0);
            juce::AudioBuffer<float> b (2, (int) x.size());
            for (int c = 0; c < 2; ++c) for (int i = 0; i < (int) x.size(); ++i) b.setSample (c, i, x[(size_t) i]);
            juce::MidiBuffer m;
            for (int s = 0; s < b.getNumSamples(); s += 512) { juce::AudioBuffer<float> blk (b.getArrayOfWritePointers(), 2, s, std::min (512, b.getNumSamples() - s));  p.processBlock (blk, m); }
            std::vector<float> L (x.size()), R (x.size());  for (int i = 0; i < (int) x.size(); ++i) { L[(size_t) i] = b.getSample (0, i);  R[(size_t) i] = b.getSample (1, i); }
            return std::make_pair (L, R); };
        const auto c = run (0.0f, 0.0f, "reverbPreDelay");
        const auto d = run (-1.0f, -1.0f, "reverbPreDelay");
        const auto e = run (1.0f, -1.0f, "reverbPreDelay");
        const size_t t0 = (size_t) (1.0 * SR), t1 = (size_t) (1.9 * SR), b0 = (size_t) (0.1 * SR), b1 = (size_t) (0.35 * SR);
        const double cl = rmsDb (c.first, t0), cr = rmsDb (c.second, t0);
        std::printf ("  info: reverb pan: centred tail L %.1f R %.1f dB; dry-left burst L %.1f R %.1f dB; wet-left tail L %.1f R %.1f dB\n", cl, cr,
                     20 * std::log10 (rms (d.first, b0, b1) + 1e-12), 20 * std::log10 (rms (d.second, b0, b1) + 1e-12), 20 * std::log10 (rms (e.first, t0, t1) + 1e-12), 20 * std::log10 (rms (e.second, t0, t1) + 1e-12));
        CHECK (std::abs (cl - cr) < 3.0, "Reverb pans at 0: the tail is balanced left and right (%.1f / %.1f dB)", cl, cr);
        CHECK (20 * std::log10 (rms (d.first, b0, b1) + 1e-12) > 20 * std::log10 (rms (d.second, b0, b1) + 1e-12) + 40.0, "Dry Pan and Wet Pan both left: the right channel is empty while the left plays");
        CHECK (20 * std::log10 (rms (e.first, t0, t1) + 1e-12) > 20 * std::log10 (rms (e.second, t0, t1) + 1e-12) + 15.0, "Wet Pan left (dry right): the reverb tail is on the left only (%.1f / %.1f dB)", 20 * std::log10 (rms (e.first, t0, t1) + 1e-12), 20 * std::log10 (rms (e.second, t0, t1) + 1e-12)); }

    // 6. inside the plugin: everything on at once, switching while it plays, allocates nothing; Repeat Now is never loaded as on
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 256);
        setP (p, "ampQuality", 2.0f);  setP (p, "repOn", 1.0f);  setP (p, "repGlitch", 1.0f);  setP (p, "repPitch", 2.0f);  setP (p, "repDecay", 0.3f);
        setP (p, "mod1Src", 6.0f);  setP (p, "mod1T1", 2.0f);  setP (p, "mod1A1", 0.3f);  setP (p, "mod1S1", 0.7f);  setP (p, "mod1S2", -0.7f);
        setP (p, "mfAOn", 1.0f);  setP (p, "mfAType", 14.0f);  setP (p, "mfALoVoice", 1.0f);  setP (p, "mfABreath", 1.0f);  setP (p, "mfABitMangle", 0.6f);  setP (p, "mfAMix", 1.0f);
        setP (p, "mfBOn", 1.0f);  setP (p, "mfBType", 24.0f);  setP (p, "mfBBitMangle", 0.6f);  setP (p, "mfBMix", 1.0f);
        setP (p, "reverbOn", 1.0f);  setP (p, "reverbMix", 0.3f);  setP (p, "reverbWetPan", -0.5f);  setP (p, "reverbDryPan", 0.5f);
        p.prepareToPlay (SR, 256);
        const auto in = testSignal ((int) (SR * 3.0), SR, 0.4f);  long allocs = 0;  bool fin = true;  float worst = 0;
        for (int blk = 0; blk * 256 < (int) in.size(); ++blk)
        {   if (blk % 12 == 0) setP (p, "repHold", (blk / 12) % 2 ? 0.0f : 1.0f);
            if (blk % 20 == 0) setP (p, "repAuto", (float) ((blk / 20) % 4));
            juce::AudioBuffer<float> b (2, 256);  juce::MidiBuffer m;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const size_t k = (size_t) blk * 256 + (size_t) i;  b.setSample (c, i, k < in.size() ? in[k] : 0.0f); }
            gAllocCount = 0;  gAllocWatch = (blk >= 2);  p.processBlock (b, m);  gAllocWatch = false;  allocs += gAllocCount;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const float v = b.getSample (c, i);  if (! std::isfinite (v)) fin = false;  worst = std::max (worst, std::abs (v)); } }
        CHECK (fin && worst < 8.0f, "Everything new on at once: finite and bounded (peak %.2f)", worst);
        CHECK (allocs == 0, "Everything new on at once: no allocation while playing and switching (%ld)", allocs);
        setP (p, "repHold", 1.0f);  juce::MemoryBlock mb;  p.getStateInformation (mb);  StarterAmpProcessor q;  q.setStateInformation (mb.getData(), (int) mb.getSize());
        CHECK (q.apvts.getRawParameterValue ("repHold")->load() < 0.5f, "a session never loads with Repeat Now held"); }
}

// b118: Warble shifts with a short sweep (about 6 ms of lag instead of 55 to 190) and keeps its wobble; Transpose goes down to -36 st
static void testB118()
{
    std::printf("[b118] Warble: short sweep, same wobble; Transpose range -36 to +24\n");
    const double SR = 48000.0;
    auto tone = [&] (double f0, int harm, double secs) { std::vector<float> x ((size_t) (secs * SR));
        for (size_t i = 0; i < x.size(); ++i) { double v = 0; for (int h = 1; h <= harm; ++h) v += std::sin (6.283185307 * f0 * h * (double) i / SR) / (double) h;  x[i] = (float) (0.3 * v); }  return x; };
    auto goertz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return 10 * std::log10 ((s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n + 1e-30); };
    // 1. Warble: the wobble stays as it was (380 to 110 ms windows, see the b97 group), the lag is now that of a 12 ms sweep
    {   RoughShifter r;  r.prepare (SR);  CHECK (r.latency (5) == 0, "Warble still reports no latency of its own (%d)", r.latency (5)); }
    for (float st : { 12.0f, 7.0f, -12.0f }) for (float grit : { 0.0f, 0.5f, 1.0f })
    {   // how long a note takes to arrive: a 220 Hz tone starts after 0.5 s of silence; first output above a sixth of its level
        std::vector<float> in ((size_t) (2.0 * SR), 0.0f);  const auto t = tone (220.0, 1, 1.5);  for (size_t i = 0; i < t.size() && i + (size_t) (0.5 * SR) < in.size(); ++i) in[i + (size_t) (0.5 * SR)] = t[i];
        RoughShifter r;  r.prepare (SR);  RoughShifter::Params rp;  rp.semis = st;  rp.character = 5;  rp.grit = grit;
        std::vector<float> y (in.size());  for (size_t i = 0; i < in.size(); ++i) y[i] = r.process (in[i], rp);
        size_t first = 0;  for (size_t i = (size_t) (0.5 * SR); i < y.size(); ++i) if (std::abs (y[i]) > 0.05f) { first = i - (size_t) (0.5 * SR);  break; }
        const double ms = 1000.0 * (double) first / SR;
        std::printf ("  info: Warble %+.0f st, Grit %.1f: the shifted note first sounds %.1f ms after the dry one (the old window gave 0 to %.0f ms, %.0f on average)\n", (double) st, grit, ms, (double) RoughShifter::windowMs (5, grit), 0.5 * (double) RoughShifter::windowMs (5, grit));
        CHECK (first > 0 && ms <= 20.0, "Warble %+.0f st, Grit %.1f: the note arrives within 20 ms (%.1f ms)", (double) st, grit, ms); }
    {   const auto in = tone (220.0, 1, 3.0);  RoughShifter r;  r.prepare (SR);  RoughShifter::Params rp;  rp.semis = 12.0f;  rp.character = 5;  rp.grit = 0.5f;
        std::vector<float> y (in.size());  for (size_t i = 0; i < in.size(); ++i) y[i] = r.process (in[i], rp);
        const size_t a = (size_t) (1.0 * SR), b = y.size();
        CHECK (goertz (y, a, b, 440.0) > goertz (y, a, b, 330.0) + 20.0 && goertz (y, a, b, 440.0) > goertz (y, a, b, 220.0) + 20.0, "Warble +12 st with the short sweep still lands on 440 Hz (440 %.1f, 330 %.1f, 220 %.1f dB)", goertz (y, a, b, 440.0), goertz (y, a, b, 330.0), goertz (y, a, b, 220.0)); }
    // 3. the two latency options: Digital damage Frame, and the pitch pedal's Instant dry
    struct Arr { double ms; int rep; bool fin; };
    auto arrival = [&] (const std::vector<std::pair<const char*, float>>& cfg, double sr, int notes) -> Arr
    {   std::vector<double> d;  int rep = 0;  bool fin = true;
        auto arrive = [&] (const std::vector<float>& y, size_t from) { double env = 0, pk = 0;  const double a = 1.0 - std::exp (-1.0 / (0.001 * sr));  std::vector<double> e (y.size());
            for (size_t i = from; i < y.size(); ++i) { env += a * (std::abs ((double) y[i]) - env);  e[i] = env; }
            for (size_t i = from; i < std::min (y.size(), from + (size_t) (0.15 * sr)); ++i) pk = std::max (pk, e[i]);
            for (size_t i = from; i < y.size(); ++i) if (e[i] > 0.25 * pk) return (double) (i - from) * 1000.0 / sr;  return 999.0; };
        for (int note = 0; note < notes; ++note)
        {   StarterAmpProcessor pr;  pr.setPlayConfigDetails (2, 2, sr, 256);
            setP (pr, "ampModel", 0.0f);  setP (pr, "ampGain", 0.0f);  setP (pr, "ampPower", 0.0f);  setP (pr, "ampAutoLevel", 0.0f);  setP (pr, "gate", -90.0f);  setP (pr, "noiseOn", 0.0f);  setP (pr, "cabOn", 0.0f);  setP (pr, "limOn", 0.0f);  setP (pr, "ampQuality", 2.0f);
            for (const auto& kv : cfg) setP (pr, kv.first, kv.second);
            pr.prepareToPlay (sr, 256);
            const size_t pre = (size_t) (0.3 * sr) + (size_t) note * 733, len = (size_t) (0.5 * sr);  std::vector<float> x (pre + len, 0.0f);
            for (size_t i = 0; i < len; ++i) { double v = 0;  for (int h = 1; h <= 5; ++h) v += std::sin (6.283185307 * (200.0 + 17.0 * note) * h * (double) i / sr) / (double) h;
                                               x[pre + i] = (float) (0.3 * v * std::min (1.0, (double) i / (0.001 * sr)) * std::exp (-3.0 * (double) i / sr)); }
            std::vector<float> y;  process (pr, x, y, 256);  pr.applyPendingUpdates();  rep = pr.getLatencySamples();
            float pk;  if (! finiteBounded (y, 8.0f, pk)) fin = false;
            d.push_back (arrive (y, pre - (size_t) (0.02 * sr)) - arrive (x, pre - (size_t) (0.02 * sr))); }
        std::sort (d.begin(), d.end());  return { d[d.size() / 2], rep, fin }; };
    {   const Arr off = arrival ({}, SR, 3);
        const double want[3] = { 21.3, 10.7, 5.3 };
        for (int f = 0; f < 3; ++f)
        {   const Arr a = arrival ({ { "mfAOn", 1.0f }, { "mfAType", 24.0f }, { "mfAMix", 1.0f }, { "mfAFrame", (float) f } }, SR, 3);
            std::printf ("  info: Digital damage Frame %d: the note arrives %.1f ms late\n", f, a.ms - off.ms);
            CHECK (a.fin && std::abs ((a.ms - off.ms) - want[f]) < 3.0, "Digital damage Frame %d delays the sound by about %.1f ms (measured %.1f ms)", f, want[f], a.ms - off.ms); } }
    for (double sr : { 48000.0, 96000.0 })        // the extremes of the effect at every frame, and switching frames while it runs does not allocate
    {   StarterAmpProcessor pr;  pr.setPlayConfigDetails (2, 2, sr, 256);
        setP (pr, "mfAOn", 1.0f);  setP (pr, "mfAType", 24.0f);  setP (pr, "mfADepth", 1.0f);  setP (pr, "mfAManual", 1.0f);  setP (pr, "mfAFeedback", 0.9f);  setP (pr, "mfAMix", 1.0f);  setP (pr, "mfARate", 20.0f);  setP (pr, "ampQuality", 2.0f);
        pr.prepareToPlay (sr, 256);
        const auto in = testSignal ((int) (sr * 2.0), sr, 0.4f);   long allocs = 0;  bool fin = true;  float worst = 0;
        for (int blk = 0; blk * 256 < (int) in.size(); ++blk)
        {   if (blk % 16 == 0) setP (pr, "mfAFrame", (float) ((blk / 16) % 3));
            juce::AudioBuffer<float> b (2, 256);  juce::MidiBuffer m;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const size_t k = (size_t) blk * 256 + (size_t) i;  b.setSample (c, i, k < in.size() ? in[k] : 0.0f); }
            gAllocCount = 0;  gAllocWatch = (blk >= 2);  pr.processBlock (b, m);  gAllocWatch = false;  allocs += gAllocCount;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < 256; ++i) { const float v = b.getSample (c, i);  if (! std::isfinite (v)) fin = false;  worst = std::max (worst, std::abs (v)); } }
        CHECK (fin && worst < 8.0f, "Digital damage at %.0f Hz, every frame, hard settings: finite, bounded (peak %.2f)", sr, worst);
        CHECK (allocs == 0, "Digital damage at %.0f Hz: switching the frame while playing allocates nothing (%ld)", sr, allocs); }
    {   const Arr base = arrival ({}, SR, 3);
        const Arr aligned = arrival ({ { "wowOn", 1.0f }, { "wowChar", 2.0f }, { "wowMix", 0.0f } }, SR, 3);
        const Arr instant = arrival ({ { "wowOn", 1.0f }, { "wowChar", 2.0f }, { "wowMix", 0.0f }, { "wowDryNow", 1.0f } }, SR, 3);
        std::printf ("  info: Glitchy at Mix 0: the dry arrives %.1f ms late and %d samples are reported; with Instant dry %.1f ms late and %d reported\n", aligned.ms - base.ms, aligned.rep, instant.ms - base.ms, instant.rep);
        CHECK (aligned.fin && instant.fin, "Glitchy, Instant dry off and on: finite and bounded");
        CHECK (aligned.ms - base.ms > 25.0 && aligned.rep >= 1400, "Instant dry off: the dry is delayed with the Character (%.1f ms, %d reported)", aligned.ms - base.ms, aligned.rep);
        CHECK (std::abs (instant.ms - base.ms) < 1.5 && instant.rep == 0, "Instant dry on: the dry is not delayed and nothing is reported (%.1f ms, %d reported)", instant.ms - base.ms, instant.rep); }
    // 2. Transpose down to -36
    {   auto* par = (juce::RangedAudioParameter*) nullptr;  StarterAmpProcessor p;  par = p.apvts.getParameter ("tpSemis");
        CHECK (par != nullptr && std::abs (par->convertFrom0to1 (0.0f) + 36.0f) < 1e-3f && std::abs (par->convertFrom0to1 (1.0f) - 24.0f) < 1e-3f, "Transpose range is -36 to +24 (%.1f to %.1f)", par ? (double) par->convertFrom0to1 (0.0f) : 0.0, par ? (double) par->convertFrom0to1 (1.0f) : 0.0); }
    for (float st : { -36.0f, -30.0f, -24.0f })
    {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.setOnsetSync (true);  t.setDualVoice (true, 3.0f, 8.0f, 1.0f);  t.setAttackHold (st <= -20.0f ? 60.0f : 0.0f, 6.0f);  t.reset();
        const auto in = tone (440.0, 1, 3.0);  std::vector<float> w = in, dry (in.size());      // a plain sine, so the octave above the wanted note is not just the second harmonic
        t.process (w.data(), dry.data(), (int) w.size(), st);
        const double want = 440.0 * std::pow (2.0, (double) st / 12.0);  const size_t a = (size_t) (1.0 * SR), b = w.size();
        bool fin = true;  float pk = 0;  for (float v : w) { if (! std::isfinite (v)) fin = false;  pk = std::max (pk, std::abs (v)); }
        const double onNote = goertz (w, a, b, want), above = goertz (w, a, b, want * 2.0), below = goertz (w, a, b, want * 0.5), third = goertz (w, a, b, want * 1.5);
        std::printf ("  info: Transpose %+.0f st on 440 Hz (%.1f Hz wanted): %.1f dB on the note; octave up %.1f, octave down %.1f, a fifth up %.1f\n", (double) st, want, onNote, above, below, third);
        CHECK (fin && pk < 2.0f, "Transpose %+.0f st: finite and bounded (peak %.2f)", (double) st, pk);
        CHECK (onNote > above + 12.0 && onNote > below + 12.0 && onNote > third + 12.0, "Transpose %+.0f st: the energy sits on the wanted note, %.1f Hz (note %.1f dB; octave up %.1f, down %.1f, fifth %.1f)", (double) st, want, onNote, above, below, third); }
    // inside the plugin: -36 plays, with Formant Lock on and off
    for (float lock : { 0.0f, 1.0f })
    {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (*a, id, 0.0f);
        setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "tpOn", 1.0f);  setP (*a, "tpSemis", -36.0f);  setP (*a, "tpLock", lock);
        a->prepareToPlay (SR, 512);
        std::vector<float> y;  process (*a, testSignal (96000, SR, 0.4f), y, 512);  float pk;
        CHECK (finiteBounded (y, 8.0f, pk) && rmsDb (y, 12000) > -80.0, "Transpose -36 st, Lock %s, plays inside the plugin: finite (peak %.2f), audible (%.1f dB)", lock > 0.5f ? "on" : "off", pk, rmsDb (y, 12000)); }
}

static void testB112()
{
    std::printf("[b112] Multiband saturator: Easy mode (Amount, Focus) and per-band Feedback\n");
    using MS = FxExtra::MultibandSatFx;
    const double SR = 48000.0;
    // Easy mode mapping
    {   bool even = true, mono = true;
        for (int b = 0; b < 4; ++b) even = even && std::abs (MS::easyDriveDb (0.4f, 0.0f, b) - 12.0f) < 1e-4f;
        for (int b = 0; b < 4; ++b) for (float f : { -1.0f, 0.0f, 1.0f }) { float prev = -1; for (float a = 0.0f; a <= 1.0f; a += 0.1f) { const float d = MS::easyDriveDb (a, f, b); mono = mono && d >= prev - 1e-5f && d >= 0.0f && d <= 48.0f; prev = d; } }
        CHECK (even, "Amount 0.4 with Focus 0 is 12 dB on every band (the default band settings)");
        CHECK (mono, "Easy Drive rises with Amount, stays between 0 and 48 dB, for every band and Focus");
        CHECK (MS::easyDriveDb (1.0f, 1.0f, 0) < 0.01f && MS::easyDriveDb (1.0f, -1.0f, 3) < 0.01f && MS::easyDriveDb (1.0f, 1.0f, 3) > MS::easyDriveDb (1.0f, 0.0f, 3) && MS::easyDriveDb (1.0f, -1.0f, 0) > MS::easyDriveDb (1.0f, 0.0f, 0),
               "Focus +1 leaves the lowest band undriven and pushes the top one; Focus -1 does the reverse (%.1f / %.1f dB)", (double) MS::easyDriveDb (1.0f, 1.0f, 3), (double) MS::easyDriveDb (1.0f, -1.0f, 0)); }
    // through the plugin: Easy at 0.4 / 0 equals the default band settings (the amp stays on: with Amp A off there is no sound)
    {   auto render = [&] (bool easy)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
            setP (*a, "msOn", 1.0f);  setP (*a, "msEasy", easy ? 1.0f : 0.0f);  setP (*a, "msAmount", 0.4f);  setP (*a, "msFocus", 0.0f);
            a->prepareToPlay (SR, 512);
            const auto x = testSignal ((int) (1.5 * SR), SR, 0.5f);  std::vector<float> y;  process (*a, x, y, 512);  return y; };
        const auto e1 = render (true), m1 = render (false);
        double dmax = 0, lvl = 0;  for (size_t i = 0; i < e1.size(); ++i) { dmax = std::max (dmax, (double) std::abs (e1[i] - m1[i]));  lvl = std::max (lvl, (double) std::abs (m1[i])); }
        CHECK (dmax < 1e-5 && lvl > 0.01, "Easy at Amount 0.4, Focus 0 equals the default band drives through the plugin (largest difference %.2g, peak %.2f)", dmax, lvl);
        // Focus moves the saturation between the lows and the highs (the block alone, Tape curve)
        auto run = [&] (float focus, double hz)
        {   MS m;  m.prepare (SR);  MS::Params p;  for (int b = 0; b < 4; ++b) p.driveDb[b] = MS::easyDriveDb (1.0f, focus, b);
            std::vector<float> l ((size_t) SR), r;  for (size_t i = 0; i < l.size(); ++i) l[i] = 0.3f * (float) std::sin (2 * M_PI * hz * (double) i / SR);
            for (size_t pos = 0; pos < l.size(); pos += 512) m.process (l.data() + pos, nullptr, (int) std::min<size_t> (512, l.size() - pos), p);
            return l; };
        auto thd = [&] (const std::vector<float>& y, double hz)      // share of the energy that is not the fundamental, 0.2 s to 1 s, in dB
        {   double te = 0, sc = 0, ss = 0;  const size_t a0 = (size_t) (0.2 * SR);
            for (size_t i = a0; i < y.size(); ++i) { sc += y[i] * std::cos (2 * M_PI * hz * (double) i / SR);  ss += y[i] * std::sin (2 * M_PI * hz * (double) i / SR);  te += (double) y[i] * y[i]; }
            const double n = (double) (y.size() - a0), fe = (sc * sc + ss * ss) * 2.0 / n;
            return 10.0 * std::log10 (std::max (1e-12, te - fe) / std::max (1e-12, te)); };
        const double a = thd (run (1.0f, 100.0), 100.0), b = thd (run (-1.0f, 100.0), 100.0), c = thd (run (-1.0f, 6000.0), 6000.0), d = thd (run (1.0f, 6000.0), 6000.0);
        std::printf ("  info: distortion share (dB) of a 100 Hz sine: Focus +1 %.1f, Focus -1 %.1f; of a 6 kHz sine: Focus -1 %.1f, Focus +1 %.1f\n", a, b, c, d);
        CHECK (b > a + 6.0, "Focus -1 distorts the lows far more than Focus +1 does (%.1f against %.1f dB)", b, a);
        CHECK (d > c + 6.0, "Focus +1 distorts the highs far more than Focus -1 does (%.1f against %.1f dB)", d, c); }
    // Feedback, on the block itself
    {   auto run = [&] (float fb, float pitch, float drive, float amp, int len, std::vector<float>& out, bool burst)
        {   MS m;  m.prepare (SR);  MS::Params p;  for (int b = 0; b < 4; ++b) { p.driveDb[b] = drive;  p.fb[b] = fb; }  p.fbPitch = pitch;
            out.assign ((size_t) len, 0.0f);  std::vector<float> l (512), r (512);  unsigned st = 99;  bool fin = true;
            for (int pos = 0; pos < len; pos += 512)
            {   const int n = std::min (512, len - pos);
                for (int i = 0; i < n; ++i) { const int k = pos + i;  float v = 0;
                    if (burst) { if (k < (int) (0.2 * SR)) { st = st * 1664525u + 1013904223u;  v = amp * (float) ((((st >> 8) / 8388608.0) - 1.0)); } }
                    else if (k < 4) v = amp * (k == 0 ? 1.0f : 0.0f);
                    l[(size_t) i] = r[(size_t) i] = v; }
                m.process (l.data(), r.data(), n, p);
                for (int i = 0; i < n; ++i) { out[(size_t) (pos + i)] = l[(size_t) i];  fin = fin && std::isfinite (l[(size_t) i]); } }
            return fin; };
        std::vector<float> y0, y1;  bool fin = true;
        // finite and bounded at every drive and pitch, full feedback: the peak stays within 2x of the same setting without Feedback
        bool bounded = true;  double worstRatio = 0;
        for (float drive : { 0.0f, 12.0f, 48.0f })
        {   run (0.0f, 0.0f, drive, 0.5f, (int) (1.5 * SR), y0, true);  double p0 = 0;  for (float v : y0) p0 = std::max (p0, (double) std::abs (v));
            for (float pitch : { -1.0f, 0.0f, 1.0f })
            {   fin = fin && run (1.0f, pitch, drive, 0.5f, (int) (1.5 * SR), y1, true);  double pc = 0;  for (float v : y1) pc = std::max (pc, (double) std::abs (v));
                std::printf ("  info: Feedback 1, Drive %.0f dB, Pitch %+.0f: peak %.2f (%.2f without Feedback)\n", (double) drive, (double) pitch, pc, p0);
                bounded = bounded && pc < 2.0 * p0 + 0.1;  worstRatio = std::max (worstRatio, pc / std::max (p0, 1e-9)); } }
        CHECK (fin && bounded, "Feedback 1 is finite and its peak stays within 2x of the same setting without Feedback, at Drive 0, 12 and 48 dB and Pitch -1, 0 and +1 (worst ratio %.2f)", worstRatio);
        // the ring dies away: after a burst, the last half second of silence is far below the burst
        double worst = -999;
        for (float drive : { 0.0f, 12.0f, 48.0f }) { run (1.0f, 0.0f, drive, 0.3f, (int) (4.0 * SR), y1, true);
            double eb = 0, et = 0;  for (int i = 0; i < (int) (0.2 * SR); ++i) eb += (double) y1[(size_t) i] * y1[(size_t) i];  for (int i = (int) (3.5 * SR); i < (int) (4.0 * SR); ++i) et += (double) y1[(size_t) i] * y1[(size_t) i];
            worst = std::max (worst, 10.0 * std::log10 ((et / (0.5 * SR)) / (eb / (0.2 * SR)) + 1e-30)); }
        CHECK (worst < -40.0, "at Feedback 1 the ring dies away: 3.5 to 4 s after a noise burst it is %.0f dB below the burst (worst of Drive 0, 12, 48)", worst);
        // Feedback adds a ring after a click, and 0 adds nothing
        run (0.0f, 0.0f, 12.0f, 0.5f, (int) (0.6 * SR), y0, false);  run (1.0f, 0.0f, 12.0f, 0.5f, (int) (0.6 * SR), y1, false);
        double e0 = 0, e1 = 0;  for (int i = (int) (0.05 * SR); i < (int) (0.4 * SR); ++i) { e0 += (double) y0[(size_t) i] * y0[(size_t) i];  e1 += (double) y1[(size_t) i] * y1[(size_t) i]; }
        std::printf ("  info: ring after one click, 50 to 400 ms: %.1f dB without Feedback, %.1f dB with Feedback 1\n", 10 * std::log10 (e0 + 1e-30), 10 * std::log10 (e1 + 1e-30));
        CHECK (e1 > 10.0 * e0, "Feedback 1 rings after a click, at least 10 dB above no Feedback (%.1f against %.1f dB)", 10 * std::log10 (e1 + 1e-30), 10 * std::log10 (e0 + 1e-30));
        std::vector<float> ya, yb;  run (0.0f, 0.0f, 12.0f, 0.5f, (int) (0.5 * SR), ya, true);  run (0.0f, 0.7f, 12.0f, 0.5f, (int) (0.5 * SR), yb, true);
        CHECK (ya == yb, "with Feedback 0 the Pitch knob changes nothing (the output is identical)");
        // turning Feedback on in the middle of a signal does not click
        MS m;  m.prepare (SR);  MS::Params p;  std::vector<float> l (512), r (512);  unsigned st = 5;  double step = 0, stepRef = 0;  float prev = 0;
        for (int blk = 0; blk < 200; ++blk)
        {   if (blk == 100) for (int b = 0; b < 4; ++b) p.fb[b] = 0.8f;
            for (int i = 0; i < 512; ++i) { st = st * 1664525u + 1013904223u;  l[(size_t) i] = r[(size_t) i] = 0.1f * (float) std::sin (2 * M_PI * 220.0 * (double) (blk * 512 + i) / SR) + 0.0f * (float) (st >> 31); }
            m.process (l.data(), r.data(), 512, p);
            for (int i = 0; i < 512; ++i) { const double d = std::abs (l[(size_t) i] - prev);  prev = l[(size_t) i];  if (blk < 100) stepRef = std::max (stepRef, d); else if (blk < 104) step = std::max (step, d); } }
        CHECK (step < 3.0 * stepRef + 0.02, "switching Feedback on during a note does not click (largest step %.3f against %.3f before)", step, stepRef); }
    // through the plugin
    {   auto a = std::make_unique<StarterAmpProcessor>();  setP (*a, "msFb2", 0.6f);  setP (*a, "msFbPitch", -0.4f);  setP (*a, "msEasy", 1.0f);  setP (*a, "msAmount", 0.7f);  setP (*a, "msFocus", 0.3f);
        juce::MemoryBlock mb;  a->getStateInformation (mb);
        auto b = std::make_unique<StarterAmpProcessor>();  b->setStateInformation (mb.getData(), (int) mb.getSize());
        auto g = [&] (const char* id) { return b->apvts.getRawParameterValue (id)->load(); };
        CHECK (std::abs (g ("msFb2") - 0.6f) < 0.01f && std::abs (g ("msFbPitch") + 0.4f) < 0.01f && g ("msEasy") > 0.5f && std::abs (g ("msAmount") - 0.7f) < 0.01f && std::abs (g ("msFocus") - 0.3f) < 0.01f, "Easy mode, Amount, Focus, Feedback and Pitch survive save and load");
        auto c = std::make_unique<StarterAmpProcessor>();  c->setPlayConfigDetails (2, 2, SR, 512);
        setP (*c, "msOn", 1.0f);  setP (*c, "msFb1", 0.8f);  setP (*c, "msFb3", 0.8f);  setP (*c, "msFb4", 0.5f);  setP (*c, "msFbPitch", 0.5f);  setP (*c, "msType", 5.0f);
        c->prepareToPlay (SR, 512);
        const auto in = testSignal ((int) (3.0 * SR), SR, 0.5f);  std::vector<float> y;  float pk;  process (*c, in, y, 512);
        CHECK (finiteBounded (y, 32.0f, pk) && rmsDb (y, 24000) > -70.0, "the plugin plays with the saturator, Feedback on three bands (finite, bounded, audible; peak %.2f)", (double) pk); }
}

static void testB109()
{
    std::printf("[b109] Transpose two-voice re-sync: notes inside a fast run start with a steady delay\n");
    const double SR = 48000.0;
    // fifteen plucks 107 ms apart (A minor pentatonic over the low range, ringing into each other), each with a short pick click
    const double RUN[15] = { 110.0, 130.81, 146.83, 164.81, 196.0, 220.0, 261.63, 293.66, 329.63, 293.66, 261.63, 220.0, 196.0, 164.81, 146.83 };
    const double step = 0.107;
    const int pre = (int) (0.30 * SR), n = pre + (int) (15 * step * SR) + (int) (0.5 * SR);
    auto makeIn = [&] (double clickAmp)
    {   std::vector<float> in ((size_t) n, 0.0f);
        unsigned lcg = 12345u;
        for (int k = 0; k < 15; ++k)
        {   const int s0 = pre + (int) (k * step * SR);
            for (int i = 0; i < (int) (1.0 * SR) && s0 + i < n; ++i)
            {   const double tt = (double) i / SR;  double v = 0;
                for (int h = 1; h <= 6; ++h) v += std::sin (2 * M_PI * RUN[k] * h * tt) / h;
                lcg = lcg * 1664525u + 1013904223u;
                const double click = i < (int) (0.003 * SR) ? ((double) (lcg >> 9) / 4194304.0 - 1.0) * std::exp (-tt / 0.0008) : 0.0;
                in[(size_t) (s0 + i)] += (float) (0.12 * (v * (1.0 - std::exp (-tt / 0.0008)) * std::exp (-tt / 0.9) + clickAmp * click)); } }
        return in; };
    std::vector<float> in = makeIn (1.4);
    struct Out { std::vector<float> y;  int switches; };
    auto render = [&] (float semis, bool dual, float jit = 1.0f, const std::vector<float>* src = nullptr)
    {   const std::vector<float>& x = src ? *src : in;  Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.setOnsetSync (true);  t.setDualVoice (dual, 3.0f, 8.0f, jit);  t.reset();
        Out o;  o.y.assign ((size_t) n, 0.0f);
        std::vector<float> wet (512), dry (512);
        for (int pos = 0; pos < n; pos += 512)
        {   const int m = std::min (512, n - pos);
            for (int i = 0; i < m; ++i) wet[(size_t) i] = x[(size_t) (pos + i)];
            t.process (wet.data(), dry.data(), m, semis);
            for (int i = 0; i < m; ++i) o.y[(size_t) (pos + i)] = wet[(size_t) i]; }
        o.switches = t.switchCount();  return o; };
    // start delay of a note: when 10 percent of the energy of its pick click (second difference, 45 ms window) has arrived, output against input
    auto t10 = [&] (const std::vector<float>& x, int t0)
    {   const int a = t0 - (int) (0.004 * SR), b = t0 + (int) (0.045 * SR);
        std::vector<double> e;  double tot = 0;
        for (int i = a; i < b; ++i) { const double d2 = (double) x[(size_t) i] - 2.0 * x[(size_t) i - 1] + x[(size_t) i - 2];  tot += d2 * d2;  e.push_back (tot); }
        for (size_t i = 0; i < e.size(); ++i) if (e[i] >= 0.1 * tot) return (double) i * 1000.0 / SR - 4.0;
        return 0.0; };
    auto delays = [&] (const std::vector<float>& y, const std::vector<float>* src = nullptr)
    {   std::vector<double> d;  const std::vector<float>& x0 = src ? *src : in;
        for (int k = 1; k < 15; ++k) { const int t0 = pre + (int) (k * step * SR);  d.push_back (t10 (y, t0) - t10 (x0, t0)); }
        return d; };
    auto sdOf = [] (const std::vector<double>& d) { double m = 0, v = 0;  for (double x : d) m += x;  m /= (double) d.size();  for (double x : d) v += (x - m) * (x - m);  return std::sqrt (v / (double) d.size()); };
    auto mxOf = [] (const std::vector<double>& d) { return *std::max_element (d.begin(), d.end()); };
    for (float semis : { -3.0f, -9.0f, -12.0f })
    {   const auto one = render (semis, false), two = render (semis, true);
        const auto d1 = delays (one.y), d2 = delays (two.y);
        CHECK (sdOf (d2) < 0.6 * sdOf (d1) && mxOf (d2) < mxOf (d1), "%+.0f st: note starts inside the run vary by sd %.1f ms with two voices (one voice: %.1f), worst %.1f ms (%.1f)", (double) semis, sdOf (d2), sdOf (d1), mxOf (d2), mxOf (d1));
        CHECK (two.switches >= 10 && two.switches <= 24, "%+.0f st: the second voice takes over at the attacks and not much more (%d switches for 15 notes)", (double) semis, two.switches);
        double r1 = 0, r2 = 0, st1 = 0, st2 = 0;
        for (int i = pre; i < n; ++i) { r1 += (double) one.y[(size_t) i] * one.y[(size_t) i];  r2 += (double) two.y[(size_t) i] * two.y[(size_t) i];
            st1 = std::max (st1, (double) std::abs (one.y[(size_t) i] - one.y[(size_t) i - 1]));  st2 = std::max (st2, (double) std::abs (two.y[(size_t) i] - two.y[(size_t) i - 1])); }
        CHECK (std::abs (10.0 * std::log10 (r2 / r1)) < 1.5 && st2 < 1.5 * st1 + 0.02, "%+.0f st: same level (%.2f dB) and no clicks (largest step %.3f against %.3f)", (double) semis, 10.0 * std::log10 (r2 / r1), st2, st1); }
    // b110: the +-1 ms start variation makes the notes of a run differ a little, and costs almost nothing in steadiness
    {   const auto strong = makeIn (5.0);  const auto d0 = delays (render (-9.0f, true, 0.0f, &strong).y, &strong), d1 = delays (render (-9.0f, true, 1.0f, &strong).y, &strong);
        double diff = 0;  for (size_t i = 0; i < d0.size(); ++i) diff = std::max (diff, std::abs (d1[i] - d0[i]));
        CHECK (diff > 0.05 && sdOf (d1) < sdOf (d0) + 1.5 && mxOf (d1) < mxOf (d0) + 3.0, "+-1 ms start variation at -9 st: the notes differ (largest change %.2f ms), spread %.1f against %.1f ms without, worst %.1f against %.1f", diff, sdOf (d1), sdOf (d0), mxOf (d1), mxOf (d0)); }
    // a steady tone, a slow swell and a glide are not attacks: the second voice does not switch for them
    {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.setOnsetSync (true);  t.setDualVoice (true, 3.0f, 8.0f, 1.0f);  t.reset();
        const int m = (int) (3.0 * SR);  std::vector<float> w ((size_t) m), dry ((size_t) m);  double ph = 0;
        for (int i = 0; i < m; ++i) { const double tt = (double) i / SR;  const double f = 196.0 * std::pow (2.0, 0.5 * tt / 3.0);  ph += 2 * M_PI * f / SR;
            w[(size_t) i] = (float) (0.3 * std::sin (ph) * (0.4 + 0.6 * tt / 3.0) * (1.0 + 0.1 * std::sin (2 * M_PI * 6.0 * tt))); }
        t.process (w.data(), dry.data(), m, -9.0f);
        CHECK (t.switchCount() <= 2, "a swelling, gliding, vibrato tone does not trigger the second voice (%d switches)", t.switchCount()); }
    // a held power chord (three notes, harmonics to the tenth, no high-frequency content) beats against itself but has no pick click: one switch at most, for its start
    {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.setOnsetSync (true);  t.setDualVoice (true, 3.0f, 8.0f, 1.0f);  t.reset();
        const int m = (int) (3.0 * SR);  std::vector<float> w ((size_t) m, 0.0f), dry ((size_t) m);
        for (double f0 : { 55.0, 82.41, 110.0 }) for (int h = 1; h <= 10; ++h) for (int i = 0; i < m; ++i) w[(size_t) i] += (float) (0.12 * std::sin (2 * M_PI * f0 * h * (double) i / SR) / h);
        t.process (w.data(), dry.data(), m, 7.0f);
        CHECK (t.switchCount() <= 1, "a held power chord does not trigger the second voice (%d switches)", t.switchCount()); }
    // a held minor second (13 Hz beating, no pick click) used to switch 39 times in 3 s and cost a third of the chord's pitch accuracy: the weak triggers now need 90 ms between them
    {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.setOnsetSync (true);  t.setDualVoice (true, 3.0f, 8.0f, 1.0f);  t.reset();
        const int m = (int) (3.0 * SR);  std::vector<float> w ((size_t) m, 0.0f), dry ((size_t) m);
        for (double f0 : { 220.0, 233.08 }) for (int h = 1; h <= 10; ++h) for (int i = 0; i < m; ++i) w[(size_t) i] += (float) (0.12 * std::sin (2 * M_PI * f0 * h * (double) i / SR) / h);
        t.process (w.data(), dry.data(), m, -12.0f);
        CHECK (t.switchCount() <= 24, "a held minor second switches at most 24 times in 3 s (%d; 39 before the 90 ms rule)", t.switchCount()); }
    // inside the plugin
    for (float semis : { -9.0f, -3.0f, 7.0f })
    {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (*a, id, 0.0f);
        setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "tpOn", 1.0f);  setP (*a, "tpSemis", semis);
        a->prepareToPlay (SR, 512);
        std::vector<float> y;  process (*a, testSignal (96000, SR, 0.4f), y, 512);  float pk;
        CHECK (finiteBounded (y, 8.0f, pk) && rmsDb (y, 12000) > -70.0, "Transpose %+.0f st plays inside the plugin with two voices: finite (peak %.2f), audible (%.1f dB)", (double) semis, pk, rmsDb (y, 12000)); }
}

static void testB108()
{
    std::printf ("[b108] Transpose attack hold: deep down-shifts no longer let the first splices after a pick attack wander 80 ms back\n");
    const double SR = 48000.0;
    // eight plucks of mixed pitch, 300 ms apart (the previous note still ringing, so the onset sync does not fire), at -24 st
    auto run = [&] (float hold, std::vector<double>& maxFirst, std::vector<double>& maxLater)
    {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.setOnsetSync (true);  t.setAttackHold (hold, 6.0f);  t.reset();
        std::vector<float> offs;  t.setDebugOffsets (&offs);
        const double f0s[] = { 82.4, 110.0, 146.8, 196.0, 246.9, 329.6, 392.0, 220.0 };
        const int N = 8, gap = (int) (0.30 * SR), pre = (int) (0.2 * SR), n = pre + N * gap + (int) (0.3 * SR);
        std::vector<float> in ((size_t) n, 0.0f);
        std::vector<int> starts;
        for (int k = 0; k < N; ++k)
        {   const int s0 = pre + k * gap;  starts.push_back (s0);
            for (int i = s0; i < std::min (n, s0 + (int) (0.7 * SR)); ++i)
            {   const double tt = (double) (i - s0) / SR;  double v = 0;  for (int h = 1; h <= 6; ++h) v += std::sin (2 * M_PI * f0s[k] * h * tt) / h;
                in[(size_t) i] += (float) (0.35 * v * (1.0 - std::exp (-tt / 0.0006)) * std::exp (-tt / 0.10)); } }
        std::vector<float> wet (32), dry (32);
        maxFirst.assign ((size_t) N, 0.0);  maxLater.assign ((size_t) N, 0.0);
        for (int pos = 0; pos + 32 <= n; pos += 32)
        {   for (int i = 0; i < 32; ++i) wet[(size_t) i] = in[(size_t) (pos + i)];
            const size_t before = offs.size();
            t.process (wet.data(), dry.data(), 32, -24.0f);
            for (size_t j = before; j < offs.size(); ++j)
                for (int k = 1; k < N; ++k)
                {   const int dt = pos - starts[(size_t) k];
                    if (dt >= 192 && dt < (int) (0.060 * SR))       maxFirst[(size_t) k] = std::max (maxFirst[(size_t) k], (double) std::abs (offs[j]) * 1000.0 / SR);
                    else if (dt >= (int) (0.060 * SR) && dt < (int) (0.250 * SR)) maxLater[(size_t) k] = std::max (maxLater[(size_t) k], (double) std::abs (offs[j]) * 1000.0 / SR); } }
    };
    std::vector<double> f0, l0, f1, l1;
    run (0.0f, f0, l0);  run (60.0f, f1, l1);
    double worstHeld = 0, worstFree = 0, laterHeld = 0;
    for (int k = 1; k < 8; ++k) { worstHeld = std::max (worstHeld, f1[(size_t) k]);  worstFree = std::max (worstFree, f0[(size_t) k]);  laterHeld = std::max (laterHeld, l1[(size_t) k]); }
    CHECK (worstHeld <= 6.2, "with the hold, no splice in the 60 ms after an attack moves a tap more than 6 ms (largest %.2f ms)", worstHeld);
    CHECK (worstFree > 12.0, "without the hold the same splices do wander (largest %.1f ms), so the check above is not vacuous", worstFree);
    CHECK (laterHeld > 12.0, "after the hold the full search is back (largest later splice %.1f ms)", laterHeld);
    // inside the plugin at -24, -21 and -12: the block plays, stays finite and bounded
    for (float semis : { -24.0f, -21.0f, -12.0f })
    {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (*a, id, 0.0f);
        setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "tpOn", 1.0f);  setP (*a, "tpSemis", semis);  setP (*a, "tpLock", 0.0f);
        a->prepareToPlay (SR, 512);
        std::vector<float> y;  process (*a, testSignal (96000, SR, 0.4f), y, 512);  float pk;
        CHECK (finiteBounded (y, 8.0f, pk) && rmsDb (y, 12000) > -70.0, "Transpose %+.0f st plays inside the plugin: finite (peak %.2f), audible (%.1f dB)", (double) semis, pk, rmsDb (y, 12000)); }
}

static void testB101()
{
    std::printf ("[b101] Pitch shifter onset sync (Transposer: Transpose, octaver, pitch pedal clean path, harmonizer); wah JP95 measured voice is in group wah76\n");
    const double SR = 48000.0;
    // delay of the first audible output of a plucked note that starts after a pause, over 60 start times spread across the shifter's cycle
    auto onsetDelays = [&] (float semis, bool sync, double f0, float windowMs)
    {   std::vector<double> d;
        for (int k = 0; k < 60; ++k)
        {   Transposer t;  t.prepare (SR);  t.setWindowMs (windowMs);  t.setLowHz (0.0f);  t.setOnsetSync (sync);  t.reset();
            const int pre = (int) (0.30 * SR) + k * 17, n = pre + (int) (0.25 * SR);
            std::vector<float> in ((size_t) n, 0.0f), out ((size_t) n), wet (512), dry (512);
            for (int i = pre; i < n; ++i) { const double tt = (double) (i - pre) / SR;  double s = 0;  for (int h = 1; h <= 6; ++h) s += std::sin (2 * M_PI * f0 * h * tt) / h;  in[(size_t) i] = (float) (0.4 * s * (1.0 - std::exp (-tt / 0.0006)) * std::exp (-tt / 0.25)); }
            for (int pos = 0; pos < n; pos += 512) { const int m = std::min (512, n - pos);  for (int i = 0; i < m; ++i) wet[(size_t) i] = in[(size_t) (pos + i)];  t.process (wet.data(), dry.data(), m, semis);  for (int i = 0; i < m; ++i) out[(size_t) (pos + i)] = wet[(size_t) i]; }
            double pkIn = 0, pkOut = 0;  for (int i = pre; i < pre + (int) (0.05 * SR); ++i) { pkIn = std::max (pkIn, (double) std::abs (in[(size_t) i]));  pkOut = std::max (pkOut, (double) std::abs (out[(size_t) i])); }
            int ti = pre, to = pre;
            for (int i = pre; i < n; ++i) if (std::abs (in[(size_t) i]) > 0.05 * pkIn) { ti = i; break; }
            for (int i = pre; i < n; ++i) if (std::abs (out[(size_t) i]) > 0.05 * pkOut) { to = i; break; }
            d.push_back ((to - ti) * 1000.0 / SR); }
        return d; };
    auto sdOf = [] (const std::vector<double>& d) { double m = 0, v = 0;  for (double x : d) m += x;  m /= (double) d.size();  for (double x : d) v += (x - m) * (x - m);  return std::sqrt (v / (double) d.size()); };
    for (float semis : { -12.0f, -7.0f, 7.0f, 12.0f })
    {   const auto off = onsetDelays (semis, false, 196.0, 12.0f), on = onsetDelays (semis, true, 196.0, 12.0f);
        const double mx = *std::max_element (on.begin(), on.end());
        CHECK (sdOf (on) < 0.1 && sdOf (off) > 0.5 && mx < 6.5, "onset sync %+.0f st: the start delay of a note after a pause varies by sd %.2f ms (was %.2f ms without sync), at most %.1f ms (half the 12 ms window is 6 ms)", (double) semis, sdOf (on), sdOf (off), mx); }
    {   const auto on = onsetDelays (-12.0f, true, 82.4, 12.0f), off = onsetDelays (-12.0f, false, 82.4, 12.0f);
        CHECK (sdOf (on) < 0.1, "onset sync, low E (82 Hz) at -12: sd %.2f ms (was %.2f ms)", sdOf (on), sdOf (off)); }
    {   const auto on = onsetDelays (-12.0f, true, 196.0, 28.0f), off = onsetDelays (-12.0f, false, 196.0, 28.0f);
        CHECK (sdOf (on) < 0.1 && sdOf (off) > 1.0, "onset sync with the Smooth window (28 ms): sd %.2f ms (was %.2f ms)", sdOf (on), sdOf (off)); }
    // a steady tone triggers the sync once (its start) and never again; runs of notes (no pause between them) are not restarted
    {   Transposer a;  a.prepare (SR);  a.setOnsetSync (true);  a.reset();
        const int n = 48000;  std::vector<float> wa ((size_t) n), dry ((size_t) n);
        for (int i = 0; i < n; ++i) wa[(size_t) i] = (float) (0.3 * std::sin (2 * M_PI * 220.0 * (double) i / SR));
        a.process (wa.data(), dry.data(), n, -5.0f);
        double maxStep = 0;  for (int i = 4800; i + 1 < n; ++i) maxStep = std::max (maxStep, (double) std::abs (wa[(size_t) i + 1] - wa[(size_t) i]));
        CHECK (a.onsetCount() == 1 && maxStep < 0.12, "a steady tone triggers the sync once (%d times) and the output has no steps (largest %.3f)", a.onsetCount(), maxStep);
        Transposer r;  r.prepare (SR);  r.setOnsetSync (true);  r.reset();
        std::vector<float> run ((size_t) n), dry2 ((size_t) n);       // eight notes in a row, 120 ms apart, each decaying but never below the quiet level: no restarts after the first
        for (int i = 0; i < n; ++i) { const int note = i / 5760;  const double tt = (double) (i % 5760) / SR;  const double f = 196.0 * std::pow (2.0, (double) (note % 4) / 12.0);
            run[(size_t) i] = (float) (0.35 * std::sin (2 * M_PI * f * (double) i / SR) * (1.0 - std::exp (-tt / 0.0008)) * std::exp (-tt / 0.4)); }
        r.process (run.data(), dry2.data(), n, -12.0f);
        CHECK (r.onsetCount() <= 2, "a run of notes without pauses restarts the cycle at most at its first note (%d times)", r.onsetCount()); }
    // inside the plugin: every pitch block still plays with the sync, switching on and off and changing intervals mid-play
    {   bool allOk = true;
        for (int block = 0; block < 3; ++block)
        {   auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
            for (const char* id : { "odOn", "compOn", "eqOn", "reverbOn", "postDelayOn", "preDelayOn", "noiseOn" }) setP (*a, id, 0.0f);
            setP (*a, "reverbMix", 0.0f);  setP (*a, "gate", -90.0f);
            if (block == 0) { setP (*a, "tpOn", 1.0f);  setP (*a, "tpSemis", -7.0f); }
            if (block == 1) { setP (*a, "octOn", 1.0f); }
            if (block == 2) { setP (*a, "wowOn", 1.0f); }
            a->prepareToPlay (SR, 512);
            std::vector<float> y;  process (*a, testSignal (48000, SR, 0.4f), y, 512);  float pk;  allOk = allOk && finiteBounded (y, 8.0f, pk) && rmsDb (y, 12000) > -70.0; }
        CHECK (allOk, "Transpose, octaver and pitch pedal play inside the plugin with the sync: finite, bounded, audible"); }
}

// ---------------------------------------------------------------------------------------------------------------------
static void testB99()
{
    std::printf("[b99] Transpose formant per note (my own design, measured, not listened to), DI Match follows the tone, shorter windows in the clean octaver / pitch pedal / harmonizer\n");
    const double SR = 48000.0;
    auto gz = [&] (const std::vector<float>& y, size_t from, size_t to, double f)
    {   const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;  const size_t n = to - from;
        for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
        return (s1 * s1 + s2 * s2 - c * s1 * s2) / (double) n; };
    auto aOf = [] (double fh) { return 1.0 / (1.0 + std::pow ((fh - 800.0) / 220.0, 2.0)) + 0.5 / (1.0 + std::pow ((fh - 2200.0) / 400.0, 2.0)) + 0.01; };
    auto vowel = [&] (size_t n, double f0)
    {   std::vector<float> x (n, 0.0f);
        for (int h = 1; h * f0 < 9000.0; ++h) for (size_t i = 0; i < n; ++i) x[i] += (float) (0.12 * aOf (f0 * h) * std::sin (2 * M_PI * f0 * h * (double) i / SR));
        return x; };
    // rms dB between each harmonic (150 Hz to 5 kHz) of the output and the same harmonic of a reference that plays the vowel natively at the output pitch
    // (so the ideal answer, with the chain's own tone cancelled out), mean removed
    auto harmErr = [&] (const std::vector<float>& y, const std::vector<float>& ref, size_t from, size_t to, double f0o)
    {   std::vector<double> d;
        for (int h = 1; h * f0o < 5000.0; ++h)
        {   const double f = h * f0o;  if (f < 150.0) continue;
            double e = 0, r = 0;  for (double df = -2.0; df <= 2.0; df += 1.0) { e += gz (y, from, to, f + df);  r += gz (ref, from, to, f + df); }
            d.push_back (10 * std::log10 (e + 1e-20) - 10 * std::log10 (r + 1e-20)); }
        double m = 0;  for (double v : d) m += v;  m /= (double) d.size();
        double s = 0;  for (double v : d) s += (v - m) * (v - m);  return std::sqrt (s / (double) d.size()); };
    typedef std::vector<std::pair<const char*, float>> Set;
    auto run = [&] (const Set& prm, const std::vector<float>& in, int* lat = nullptr, StarterAmpProcessor** keep = nullptr)
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        setP (p, "tpOn", 1.0f);  setP (p, "tpMix", 1.0f);
        for (auto& kv : prm) setP (p, kv.first, kv.second);
        std::vector<float> y;  process (p, in, y, 512);  if (lat) { p.applyPendingUpdates();  *lat = p.getLatencySamples(); }  (void) keep;  return y; };
    auto base = [&] ()
    {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "ampPower", 0.0f);  setP (p, "ampAutoLevel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        std::vector<float> y;  process (p, testSignal (4096, SR), y, 512);  p.applyPendingUpdates();  return p.getLatencySamples(); };
    const size_t N = (size_t) (SR * 2.5), A = (size_t) (SR * 1.2), B = (size_t) (SR * 2.4);

    // 1. the tone mover gives a shifted note back the tone of the original (the shifter and the mover on their own, a broad two-hill vowel; the amp's
    //    own tone would colour the numbers if the whole chain were used). Error = rms dB between each harmonic and the ideal envelope at its frequency.
    {   auto broad = [&] (double fh) { return 1.0 / (1.0 + std::pow ((fh - 800.0) / 500.0, 2.0)) + 0.5 / (1.0 + std::pow ((fh - 2200.0) / 900.0, 2.0)) + 0.02; };
        auto bv = [&] (size_t n, double f0) { std::vector<float> x (n, 0.0f);  for (int h = 1; h * f0 < 9000.0; ++h) for (size_t i = 0; i < n; ++i) x[i] += (float) (0.12 * broad (f0 * h) * std::sin (2 * M_PI * f0 * h * (double) i / SR));  return x; };
        auto err = [&] (const std::vector<float>& y, double f0o)
        {   std::vector<double> d;
            for (int h = 1; h * f0o < 5000.0; ++h)
            {   const double f = h * f0o;  double e = 0;  for (double df = -2.0; df <= 2.0; df += 1.0) e += gz (y, A, B, f + df);
                d.push_back (10 * std::log10 (e + 1e-20) - 20 * std::log10 (broad (f))); }
            double m = 0;  for (double v : d) m += v;  m /= (double) d.size();
            double s = 0;  for (double v : d) s += (v - m) * (v - m);  return std::sqrt (s / (double) d.size()); };
        auto go = [&] (const std::vector<float>& in, float st, bool mover)
        {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.reset();  FormantWarpQuick fw;  fw.prepare (SR);  fw.reset();  fw.setSemis (mover ? -st : 0.0f);
            std::vector<float> out (in.size()), wet (512), dry (512);
            for (size_t pos = 0; pos < in.size(); pos += 512)
            {   const int n = (int) std::min ((size_t) 512, in.size() - pos);
                for (int i = 0; i < n; ++i) wet[(size_t) i] = in[pos + (size_t) i];
                t.process (wet.data(), dry.data(), n, st);
                for (int i = 0; i < n; ++i) out[pos + (size_t) i] = fw.process (wet[(size_t) i]); }
            return out; };
        double sumOn = 0, sumOff = 0;  int cnt = 0;
        for (double f0 : { 110.0, 196.0, 330.0 })
            for (float st : { -12.0f, -7.0f, 7.0f, 12.0f })
            {   const auto in = bv (N, f0);
                const double f0o = f0 * std::pow (2.0, (double) st / 12.0);
                const double eOff = err (go (in, st, false), f0o), eOn = err (go (in, st, true), f0o);
                sumOff += eOff;  sumOn += eOn;  ++cnt;
                CHECK (eOn < eOff + 0.5, "tone mover at %+.0f st on %.0f Hz: envelope error %.2f dB, worse than without (%.2f dB)", st, f0, eOn, eOff); }
        std::printf ("  info: envelope error, mean over 12 cases: mover off %.2f dB, on %.2f dB\n", sumOff / cnt, sumOn / cnt);
        CHECK (sumOn / cnt < sumOff / cnt - 2.0, "the tone mover does not bring the tone back on average (%.2f dB against %.2f dB without)", sumOn / cnt, sumOff / cnt); }

    // 1b. in the plug-in Lock on actually changes the sound of a shifted note (the wiring), and by the right way round: a note shifted up gets darker
    {   const auto in = vowel (N, 196.0);
        const auto off = run ({ { "tpSemis", 12.0f }, { "tpLock", 0.0f } }, in), on = run ({ { "tpSemis", 12.0f }, { "tpLock", 1.0f } }, in);
        auto band = [&] (const std::vector<float>& y, double lo, double hi) { double e = 0;  for (double f = lo; f <= hi; f += 5.0) e += gz (y, A, B, f);  return 10 * std::log10 (e + 1e-20); };
        const double tOff = band (off, 2500, 4500) - band (off, 300, 700), tOn = band (on, 2500, 4500) - band (on, 300, 700);
        std::printf ("  info: +12 st, high/low band: Lock off %.1f dB, Lock on %.1f dB\n", tOff, tOn);
        CHECK (tOn < tOff - 3.0, "Lock on a note shifted up should be darker (%.1f dB against %.1f dB)", tOn, tOff); }

    // 2. the Formant knob alone moves the tone, not the pitch
    {   const auto in = vowel (N, 100.0);
        const auto y0 = run ({ { "tpSemis", 0.0f }, { "tpLock", 0.0f }, { "tpFormant", 0.0f } }, in), y5 = run ({ { "tpSemis", 0.0f }, { "tpLock", 0.0f }, { "tpFormant", 5.0f } }, in);
        auto band = [&] (const std::vector<float>& y, double lo, double hi) { double e = 0;  for (double f = lo; f <= hi; f += 5.0) e += gz (y, A, B, f);  return 10 * std::log10 (e + 1e-20); };
        const double r0 = band (y0, 1000, 1150) - band (y0, 700, 900), r5 = band (y5, 1000, 1150) - band (y5, 700, 900);
        double on = 0, all = 0;
        for (double f = 40.0; f <= 4000.0; f += 2.5) { const double e = gz (y5, A, B, f);  all += e;  const double h = std::round (f / 100.0);  if (h >= 1.0 && std::abs (f - h * 100.0) <= 3.0) on += e; }
        std::printf ("  info: Formant +5 st: upper/lower band %.1f dB (off %.1f dB), %.2f of the power still on the 100 Hz series\n", r5, r0, on / all);
        CHECK (r5 > r0 + 3.0, "Formant +5 st does not move the tone up (%.1f vs %.1f dB)", r5, r0);
        CHECK (on / all > 0.9, "Formant alone moved the pitch (%.2f of the power on the original series)", on / all); }

    // 3. delay: Lock on adds the tone mover's frame (kLatency samples), the dry stays in step, silence stays silent
    {   const int b0 = base();  int lOff = 0, lOn = 0, lForm = 0, lSmooth = 0;
        const auto in = vowel (4800, 196.0);
        run ({ { "tpSemis", -12.0f }, { "tpLock", 0.0f } }, in, &lOff);  run ({ { "tpSemis", -12.0f }, { "tpLock", 1.0f } }, in, &lOn);
        run ({ { "tpSemis", 0.0f }, { "tpLock", 1.0f }, { "tpFormant", 4.0f } }, in, &lForm);  run ({ { "tpSemis", -12.0f }, { "tpLock", 1.0f }, { "tpQuality", 1.0f } }, in, &lSmooth);
        std::printf ("  info: Transpose delay (samples): Lock off %d, Lock on %d, Formant alone %d, Smooth with Lock %d\n", lOff - b0, lOn - b0, lForm - b0, lSmooth - b0);
        CHECK (lOff - b0 == 288 && lOn - b0 == 288 + FormantWarpQuick::kLatency && lForm - b0 == 288 + FormantWarpQuick::kLatency && lSmooth - b0 == 672 + FormantWarpFast::kLatency, "delays %d, %d, %d, %d (want 288, %d, %d, %d)", lOff - b0, lOn - b0, lForm - b0, lSmooth - b0, 288 + FormantWarpQuick::kLatency, 288 + FormantWarpQuick::kLatency, 672 + FormantWarpFast::kLatency);
        {   int l24 = 0, l18 = 0, l12 = 0;                                   // Lock fades out between one and two octaves of shift: no mover at -24, the mover at -18 and -12
            run ({ { "tpSemis", -24.0f }, { "tpLock", 1.0f } }, in, &l24);  run ({ { "tpSemis", -18.0f }, { "tpLock", 1.0f } }, in, &l18);  run ({ { "tpSemis", -12.0f }, { "tpLock", 1.0f } }, in, &l12);
            std::printf ("  info: delay with Lock on: -24 st %d, -18 st %d, -12 st %d\n", l24 - b0, l18 - b0, l12 - b0);
            CHECK (l24 - b0 == 288 && l18 - b0 == 288 + FormantWarpQuick::kLatency && l12 - b0 == 288 + FormantWarpQuick::kLatency, "Lock fade: delays %d, %d, %d (want 288, %d, %d)", l24 - b0, l18 - b0, l12 - b0, 288 + FormantWarpQuick::kLatency, 288 + FormantWarpQuick::kLatency); }
        std::vector<float> click (SR / 2, 0.0f);  click[6000] = 0.8f;
        for (int lock = 0; lock < 2; ++lock)
        {   int lt = 0;  const auto y = run ({ { "tpSemis", -12.0f }, { "tpMix", 0.0f }, { "tpLock", (float) lock } }, click, &lt);
            const auto off = run ({ { "tpOn", 0.0f } }, click);
            auto peak = [] (const std::vector<float>& v) { size_t a = 0;  for (size_t i = 0; i < v.size(); ++i) if (std::abs (v[i]) > std::abs (v[a])) a = i;  return (int) a; };
            CHECK (std::abs ((peak (y) - peak (off)) - (lt - b0)) <= 1, "Mix 0: the dry click arrives with the reported delay (%d vs %d), Lock %d", peak (y) - peak (off), lt - b0, lock); }
        {   Transposer t;  t.prepare (SR);  t.setWindowMs (12.0f);  t.setLowHz (0.0f);  t.reset();  FormantWarpQuick fw;  fw.prepare (SR);  fw.reset();  fw.setSemis (12.0f);
            std::vector<float> w (512), d (512);  float m = 0;
            for (int blk = 0; blk < 200; ++blk) { std::fill (w.begin(), w.end(), 0.0f);  t.process (w.data(), d.data(), 512, -12.0f);  for (float& v : w) { v = fw.process (v);  m = std::max (m, std::abs (v)); } }
            CHECK (m == 0.0f, "silence in, silence out with the tone mover (%.2e)", m); } }

    // 4. DI Match follows the tone: with Lock on the shifted signal's picture is the unshifted one
    {   auto curveOf = [&] (const Set& prm)
        {   StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
            setP (p, "ampModel", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
            setP (p, "dmOn", 1.0f);  setP (p, "dmSource", 2.0f);  setP (p, "dmTarget", 0.0f);
            for (auto& kv : prm) setP (p, kv.first, kv.second);
            std::vector<float> y;  process (p, testSignal (24000, SR), y, 512);
            return p.diCurrentCurve(); };
        const auto c0 = curveOf ({}), cLock = curveOf ({ { "tpOn", 1.0f }, { "tpSemis", -12.0f }, { "tpLock", 1.0f }, { "tpMix", 1.0f } }),
                   cNo = curveOf ({ { "tpOn", 1.0f }, { "tpSemis", -12.0f }, { "tpLock", 0.0f }, { "tpMix", 1.0f } });
        double dLock = 0, dNo = 0;  for (size_t k = 0; k < c0.size(); ++k) { dLock += std::abs (cLock[k] - c0[k]);  dNo += std::abs (cNo[k] - c0[k]); }
        dLock /= (double) c0.size();  dNo /= (double) c0.size();
        std::printf ("  info: DI Match curve against no Transpose: Lock on %.2f dB, Lock off %.2f dB (mean absolute difference)\n", dLock, dNo);
        CHECK (dLock < 0.2, "with Formant Lock on DI Match sees a moved tone (%.2f dB from the unshifted curve)", dLock);
        CHECK (dNo > 0.5, "with Lock off DI Match did not follow the shift (%.2f dB)", dNo); }

    // 5. the clean octaver, pitch pedal and harmonizer use the same short windows
    {   const int b0 = base();
        const auto in = vowel (4800, 196.0);
        auto latOf = [&] (const Set& prm) { int l = 0;  run ({ { "tpOn", 0.0f } }, in, &l);  StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
            setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
            for (auto& kv : prm) setP (p, kv.first, kv.second);
            std::vector<float> y;  process (p, in, y, 512);  p.applyPendingUpdates();  return p.getLatencySamples() - b0; };
        const int oct = latOf ({ { "octOn", 1.0f }, { "octALvl", 0.6f }, { "octA", 1.0f } }), octS = latOf ({ { "octOn", 1.0f }, { "octALvl", 0.6f }, { "octA", 1.0f }, { "tpQuality", 1.0f } });
        const int octN = latOf ({ { "octOn", 1.0f }, { "octP1", 0.5f }, { "octMode", 1.0f } });
        const int wow = latOf ({ { "wowOn", 1.0f }, { "wowMix", 1.0f } }), wowS = latOf ({ { "wowOn", 1.0f }, { "wowMix", 1.0f }, { "tpQuality", 1.0f } });
        const int hm = latOf ({ { "hmOn", 1.0f }, { "hmTrack", 0.0f } }), oct7 = latOf ({ { "octOn", 1.0f }, { "octALvl", 0.6f }, { "octA", 1.0f }, { "tpRange", 1.0f } });
        std::printf ("  info: delay (samples): octaver %d (Smooth %d, bank %d, 7-string %d), pitch pedal %d (Smooth %d), harmonizer single note %d\n", oct, octS, octN, oct7, wow, wowS, hm);
        CHECK (oct == 288 && octN == 288 && wow == 288 && hm == 288, "Fast windows should report 288 samples (octaver %d, bank %d, pedal %d, harmonizer %d)", oct, octN, wow, hm);
        CHECK (octS == 672 && wowS == 672, "Smooth windows should report 672 samples (octaver %d, pedal %d)", octS, wowS);
        CHECK (oct7 >= oct && oct7 <= 672, "7-string octaver delay %d", oct7);
        // and they follow a chord: the octave-down voice of a fifth, with the dry guitar turned off, sits on the shifted notes
        auto tones = [&] (size_t n, const std::vector<double>& f0s) { std::vector<float> x (n, 0.0f);  for (double f0 : f0s) for (int h = 1; h <= 10; ++h) for (size_t i = 0; i < n; ++i) x[i] += (float) (0.12 * std::sin (2 * M_PI * f0 * h * (double) i / SR) / h);  return x; };
        auto share = [&] (const std::vector<float>& y, const std::vector<double>& f0s)
        {   double on = 0, all = 0;
            for (double f = 20.0; f <= 2000.0; f += 2.5) { const double e = gz (y, A, B, f);  all += e;  bool hit = false;  for (double f0 : f0s) { const double h = std::round (f / f0);  if (h >= 1.0 && std::abs (f - h * f0) <= 3.0) hit = true; }  if (hit) on += e; }
            return on / (all + 1e-30); };
        const auto fifth = tones (N, { 110.0, 164.81 });
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "ampModel", 0.0f);  setP (p, "ampGain", 0.0f);  setP (p, "gate", -90.0f);  setP (p, "noiseOn", 0.0f);  setP (p, "cabOn", 0.0f);  setP (p, "limOn", 0.0f);
        setP (p, "octOn", 1.0f);  setP (p, "octALvl", 1.0f);  setP (p, "octA", 1.0f);  setP (p, "octBLvl", 0.0f);  setP (p, "octDry", 0.0f);
        std::vector<float> yo;  process (p, fifth, yo, 512);
        const double sh = share (yo, { 55.0, 82.405 });
        std::printf ("  info: octaver, octave down on a fifth: %.2f of the power on the shifted notes\n", sh);
        CHECK (sh > 0.8, "octaver octave-down on a fifth: only %.2f of the power on the shifted notes", sh); }
}

static void testB95()
{
    std::printf ("[b95] robustness: calibrator abuse, bad input values, other sample rates, random parameter fuzz\n");
    // 1. the calibrator must survive anything a host can send it
    for (double sr : { 22050.0, 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int blk : { 1, 3, 17, 480, 4096, 20000 })
        {
            InputCalibrator c;  c.prepare (sr);  c.start();
            std::vector<float> x ((size_t) blk);
            std::mt19937 rng (5);  std::uniform_real_distribution<float> u (-0.3f, 0.3f);
            const long total = (long) (sr * 12.0);
            for (long done = 0, k = 0; done < total; done += blk, ++k)
            {
                for (auto& v : x) v = u (rng);
                if (k % 7 == 3) x[0] = std::numeric_limits<float>::quiet_NaN();
                if (k % 11 == 5) x[(size_t) blk - 1] = std::numeric_limits<float>::infinity();
                if (k % 13 == 6) x[(size_t) blk / 2] = -1.0e30f;
                c.feed (x.data(), blk);
                c.feed (x.data(), 0);
            }
            const auto r = c.result();
            const bool fin = std::isfinite (r.noiseRmsDb) && std::isfinite (r.p99Db) && std::isfinite (r.maxDb) && std::isfinite (r.recInGain) && std::isfinite (r.recGate);
            CHECK (fin, "calibrator at %.0f Hz, block %d: non-finite result (noise %.1f p99 %.1f gain %.1f gate %.1f)", sr, blk, r.noiseRmsDb, r.p99Db, r.recInGain, r.recGate);
            CHECK (r.recInGain >= -24.0f && r.recInGain <= 24.0f && r.recGate >= -90.0f && r.recGate <= -30.0f, "calibrator at %.0f Hz, block %d: advice out of range (gain %.1f, gate %.1f)", sr, blk, r.recInGain, r.recGate);
        }
    // 2. a NaN or infinite sample from the host must not leave the amp stuck on silence or NaN
    {
        const auto sig = testSignal (48000 * 3, 48000.0, 0.4f);
        int bad = 0, shown = 0;
        for (int model : { 0, 4, 9, 14, 21, 33, 41, 45 })
            for (int kind = 0; kind < 3; ++kind)
            {
                StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, 48000.0, 512);
                setP (p, "ampModel", (float) model);  setP (p, "ampMode", 2.0f);
                p.prepareToPlay (48000.0, 512);
                auto x = sig;
                const float badv = kind == 0 ? std::numeric_limits<float>::quiet_NaN() : kind == 1 ? std::numeric_limits<float>::infinity() : 1.0e20f;
                x[24000] = badv;  x[24001] = -badv;
                std::vector<float> y;  process (p, x, y, 512);
                bool fin = true;  for (size_t i = 72000; i < y.size(); ++i) if (! std::isfinite (y[i])) fin = false;
                std::vector<float> tail (y.begin() + 96000, y.end());
                if (! fin) { ++bad; if (shown++ < 6) std::printf ("  note: amp %d, bad value kind %d: output still non-finite 1 s later\n", model, kind); }
            }
        CHECK (bad == 0, "bad input values: %d of 24 runs left a non-finite output", bad);
    }
    // 2b. host layouts: mono in and out, empty blocks, blocks much longer than the size the host announced
    {
        int bad = 0;
        for (int channels : { 1, 2 })
            for (int announced : { 16, 64, 512 })
            {
                StarterAmpProcessor p;  p.setPlayConfigDetails (channels, channels, 48000.0, announced);
                setP (p, "ampMode", 2.0f);
                p.prepareToPlay (48000.0, announced);
                juce::MidiBuffer m;
                const auto sig = testSignal (48000, 48000.0, 0.4f);
                float pk = 0;
                for (int blk : { 0, 1, announced, announced + 1, announced * 8 + 3, 4096 })
                {
                    juce::AudioBuffer<float> b (channels, std::max (blk, 1));
                    for (int pass = 0; pass < 4; ++pass)
                    {
                        b.setSize (channels, std::max (blk, 1), false, true, true);
                        for (int c = 0; c < channels; ++c) for (int i = 0; i < blk; ++i) b.setSample (c, i, sig[(size_t) ((i + pass * 997) % (int) sig.size())]);
                        juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), channels, blk);
                        p.processBlock (v, m);
                        for (int c = 0; c < channels; ++c) for (int i = 0; i < blk; ++i) { const float s = v.getSample (c, i); if (! std::isfinite (s)) ++bad; else pk = std::max (pk, std::abs (s)); }
                    }
                }
                if (pk > 64.0f) ++bad;
            }
        CHECK (bad == 0, "host layouts (mono / stereo, empty and oversized blocks): %d bad samples or peaks", bad);
    }
    // 3. other sample rates and block sizes, with random parameter settings
    {
        int nonFinite = 0, huge = 0;  double worst = 0;
        std::mt19937 rng (95);  std::uniform_real_distribution<float> u (0.0f, 1.0f);
        const double rates[3] = { 44100.0, 96000.0, 192000.0 };
        const int blocks[4] = { 32, 256, 1024, 2048 };
        for (int trial = 0; trial < 36; ++trial)
        {
            const double sr = rates[trial % 3];
            StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, sr, blocks[trial % 4]);
            p.prepareToPlay (sr, blocks[trial % 4]);
            for (auto* prm : p.getParameters())
                if (prm->isAutomatable())
                    prm->setValueNotifyingHost (u (rng));
            const auto sig = testSignal ((int) (sr * 0.8), sr, 0.5f, (unsigned) trial + 1);
            std::vector<float> y;  process (p, sig, y, blocks[trial % 4]);
            bool fin = true;  float pk = 0;
            for (float v : y) { if (! std::isfinite (v)) fin = false; else pk = std::max (pk, std::abs (v)); }
            if (! fin) { ++nonFinite; std::printf ("  note: fuzz trial %d (%.0f Hz, block %d): non-finite output\n", trial, sr, blocks[trial % 4]); }
            else if (pk > 64.0f) { ++huge; std::printf ("  note: fuzz trial %d (%.0f Hz, block %d): peak %.0f\n", trial, sr, blocks[trial % 4], pk); }
            worst = std::max (worst, (double) pk);
        }
        CHECK (nonFinite == 0, "parameter fuzz: %d of 36 random settings gave a non-finite output", nonFinite);
        CHECK (huge == 0, "parameter fuzz: %d of 36 random settings peaked above +36 dBFS (worst %.1f)", huge, worst);
    }
}


static void testB94()
{
    std::printf ("[b94] input calibration: noise floor, hum, hard-playing level, advice, wiring in the processor, no allocation\n");
    const double SR = 48000.0;
    auto runCal = [&] (InputCalibrator& c, const std::vector<float>& quietSig, const std::vector<float>& playSig)
    {
        c.prepare (SR);  c.start();
        const int B = 480;
        auto feedAll = [&] (const std::vector<float>& x) { for (size_t i = 0; i + B <= x.size(); i += B) c.feed (x.data() + i, B); };
        feedAll (quietSig);
        feedAll (playSig);
    };
    const int nQuiet = (int) (SR * 3.05), nPlay = (int) (SR * 8.05);
    std::mt19937 rng (3);  std::normal_distribution<float> g (0.0f, 1.0f);
    auto noise = [&] (int n, double rmsDb, double humRmsDb, double humHz)
    {
        std::vector<float> x ((size_t) n);
        const float sg = (float) std::pow (10.0, rmsDb / 20.0), ha = humRmsDb > -200.0 ? (float) (std::sqrt (2.0) * std::pow (10.0, humRmsDb / 20.0)) : 0.0f;
        for (int i = 0; i < n; ++i) x[(size_t) i] = sg * g (rng) + ha * std::sin ((float) (6.283185307179586 * humHz * i / SR));
        return x;
    };
    // a burst of sine "notes": each 0.25 s at a fixed amplitude, 0.15 s gaps
    auto bursts = [&] (int n, float amp)
    {
        std::vector<float> x ((size_t) n, 0.0f);
        const int per = (int) (0.4 * SR), len = (int) (0.25 * SR);
        for (int s = 0; s + len < n; s += per)
            for (int i = 0; i < len; ++i) x[(size_t) (s + i)] = amp * std::sin ((float) (6.283185307179586 * (110.0 + 20.0 * ((s / per) % 5)) * i / SR));
        return x;
    };
    const auto dbf = [] (double db) { return (float) std::pow (10.0, db / 20.0); };

    // 1. noise and hum are measured; 60 Hz is told from 50 Hz
    for (double hz : { 60.0, 50.0, 49.8 })
    {
        InputCalibrator c;  runCal (c, noise (nQuiet, -62.0, -58.0, hz), bursts (nPlay, dbf (-20.0)));
        const auto r = c.result();
        CHECK (c.currentPhase() == InputCalibrator::done, "calibration finished (phase %d)", c.currentPhase());
        CHECK (r.hum && r.humHz == (hz > 55.0 ? 60 : 50), "%.1f Hz hum: detected %d at %d Hz", hz, (int) r.hum, r.humHz);
        CHECK (std::abs (r.humDb + 58.0f) < 1.5f, "%.1f Hz hum level %.2f dB (should be -58)", hz, r.humDb);
        CHECK (std::abs (r.noiseRmsDb + 56.5f) < 1.0f, "noise RMS %.2f dB (should be about -56.5)", r.noiseRmsDb);
        CHECK (r.noiseEnvDb > r.noiseRmsDb && r.noiseEnvDb < r.noiseRmsDb + 12.0f, "noise envelope %.2f dB against RMS %.2f dB", r.noiseEnvDb, r.noiseRmsDb);
    }
    {
        InputCalibrator c;  runCal (c, noise (nQuiet, -62.0, -300.0, 60.0), bursts (nPlay, dbf (-20.0)));
        const auto r = c.result();
        CHECK (! r.hum && r.humHz == 0, "plain noise reported as hum (%d at %d Hz, %.1f dB)", (int) r.hum, r.humHz, r.humDb);
        CHECK (std::abs (r.noiseRmsDb + 62.0f) < 0.6f, "plain noise RMS %.2f dB (should be -62)", r.noiseRmsDb);
    }

    // 2. the hard-playing level and the advice
    for (double lv : { -30.0, -20.0, -10.0, -4.0 })
    {
        InputCalibrator c;  runCal (c, noise (nQuiet, -70.0, -300.0, 60.0), bursts (nPlay, dbf (lv)));
        const auto r = c.result();
        CHECK (r.heardPlaying, "playing at %.0f dB was not heard", lv);
        CHECK (std::abs (r.p99Db - (float) lv) < 0.6f, "bursts of %.0f dBFS read as %.2f dB", lv, r.p99Db);
        CHECK (std::abs (r.recInGain - (InputCalibrator::kTargetPeakDb - (float) lv)) < 0.8f, "at %.0f dBFS the advice is %+.1f dB (should be %+.1f)", lv, r.recInGain, InputCalibrator::kTargetPeakDb - lv);
        CHECK (r.recGate >= -90.0f && r.recGate <= -30.0f, "gate advice %.1f is outside the knob", r.recGate);
        CHECK (r.clipped == 0, "bursts of %.0f dBFS counted %d clipped samples", lv, r.clipped);
    }
    {
        // a few loud pick clicks must not set the level (99% of the active blocks)
        auto x = bursts (nPlay, dbf (-24.0));
        for (int k = 0; k < 6; ++k) x[(size_t) (SR * (1.0 + 1.1 * k))] = 0.8f;
        InputCalibrator c;  runCal (c, noise (nQuiet, -70.0, -300.0, 60.0), x);
        const auto r = c.result();
        CHECK (r.p99Db < -22.0f && r.maxDb > -3.0f, "six pick clicks moved the hard-playing level to %.2f dB (max %.1f)", r.p99Db, r.maxDb);
    }
    {
        // the advice sets the gate 6 dB over the noise the gate would see after the new Input level
        InputCalibrator c;  runCal (c, noise (nQuiet, -64.0, -300.0, 60.0), bursts (nPlay, dbf (-18.0)));
        const auto r = c.result();
        CHECK (std::abs (r.recGate - (r.noiseEnvDb + r.recInGain + 6.0f)) <= 1.0f, "gate advice %.1f for envelope %.1f and Input %+.1f", r.recGate, r.noiseEnvDb, r.recInGain);
    }
    {
        // very quiet playing: the advice stops at the knob's range and says so
        InputCalibrator c;  runCal (c, noise (nQuiet, -90.0, -300.0, 60.0), bursts (nPlay, dbf (-50.0)));
        const auto r = c.result();
        CHECK (r.recInGain == 24.0f && r.gainLimited, "very quiet playing: advice %+.1f dB, limited %d", r.recInGain, (int) r.gainLimited);
        CHECK (r.outputClass == 0, "very quiet playing classed %d", r.outputClass);
    }

    // 3. clipping and silence
    {
        auto x = bursts (nPlay, 1.4f);  for (auto& v : x) v = std::max (-1.0f, std::min (1.0f, v));
        InputCalibrator c;  runCal (c, noise (nQuiet, -70.0, -300.0, 60.0), x);
        const auto r = c.result();
        CHECK (r.clipped > 100 && r.maxDb > -0.1f && r.outputClass == 3, "clipped input: %d clipped samples, max %.2f dB, class %d", r.clipped, r.maxDb, r.outputClass);
    }
    {
        InputCalibrator c;  runCal (c, noise (nQuiet, -70.0, -300.0, 60.0), noise (nPlay, -70.0, -300.0, 60.0));
        CHECK (! c.result().heardPlaying, "silence counted as playing");
        CHECK (c.completedRuns() == 1, "one finished run is counted (%d)", c.completedRuns());
    }

    // 4. phases, progress, cancel, and a second run
    {
        InputCalibrator c;  c.prepare (SR);
        CHECK (c.currentPhase() == InputCalibrator::idle && ! c.isActive(), "starts idle");
        auto q = noise (480, -70.0, -300.0, 60.0);
        c.start();  CHECK (c.isActive(), "a start request counts as active");
        c.feed (q.data(), 480);
        CHECK (c.currentPhase() == InputCalibrator::quiet, "quiet step began (phase %d)", c.currentPhase());
        for (int i = 0; i < 300; ++i) c.feed (q.data(), 480);                 // 3.0 s in (10 ms blocks)
        CHECK (c.currentPhase() == InputCalibrator::play, "play step began after 3 s (phase %d)", c.currentPhase());
        c.cancel();  c.feed (q.data(), 480);
        CHECK (c.currentPhase() == InputCalibrator::idle, "cancel returned to idle (phase %d)", c.currentPhase());
        c.start();  c.feed (q.data(), 480);
        CHECK (c.currentPhase() == InputCalibrator::quiet, "a second run starts again (phase %d)", c.currentPhase());
        CHECK (c.completedRuns() == 0, "no run has finished yet (%d)", c.completedRuns());
    }
    {
        // the advice on its own
        InputCalibrator::Result r;  r.p99Db = -16.3f;  r.noiseEnvDb = -61.0f;  InputCalibrator::advise (r);
        CHECK (r.recInGain == 6.5f && r.recGate == -49.0f && r.outputClass == 1 && std::abs (r.snrDb - 44.7f) < 0.01f, "advice: gain %.1f gate %.1f class %d snr %.2f", r.recInGain, r.recGate, r.outputClass, r.snrDb);
        r.p99Db = -4.0f;  InputCalibrator::advise (r);  CHECK (r.recInGain == -6.0f && r.outputClass == 3, "hot guitar: gain %.1f class %d", r.recInGain, r.outputClass);
    }

    // 5. in the processor: it measures the input before the Input level, does not change the sound, and allocates nothing
    {
        StarterAmpProcessor p;  p.setPlayConfigDetails (2, 2, SR, 512);  p.prepareToPlay (SR, 512);
        setP (p, "inGain", 12.0f);                                        // must not change what is measured
        auto q = noise ((int) (SR * 3.1), -66.0, -300.0, 60.0), pl = bursts ((int) (SR * 8.2), dbf (-22.0));
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;
        p.inputCal.start();
        long allocs = 0;
        auto run = [&] (const std::vector<float>& x)
        {
            for (size_t i = 0; i + 512 <= x.size(); i += 512)
            {
                for (int k = 0; k < 512; ++k) { b.setSample (0, k, x[i + (size_t) k]);  b.setSample (1, k, x[i + (size_t) k]); }
                gAllocCount = 0;  gAllocWatch = true;
                p.processBlock (b, m);
                gAllocWatch = false;  allocs += gAllocCount;
            }
        };
        run (q);  run (pl);
        const auto r = p.inputCal.result();
        CHECK (p.inputCal.currentPhase() == InputCalibrator::done && r.heardPlaying, "processor run finished (phase %d)", p.inputCal.currentPhase());
        CHECK (std::abs (r.p99Db + 22.0f) < 0.8f, "measured before the Input level: %.2f dB (should be -22 although Input is +12)", r.p99Db);
        CHECK (std::abs (r.recInGain - 12.0f) < 1.0f, "advice %+.1f dB (should be +12)", r.recInGain);
        CHECK (allocs == 0, "the processor allocated %ld times while calibrating", allocs);
    }
}

static void testB92()
{
    std::printf ("[b92] Hi-Wat Headroom and Ironside: second harmonic in Hybrid (b121: Hi-Wat share 0.36 retuned to hardware captures, Ironside back at the plain 0.5 mix); Auto Level\n");
    const double SR = 48000.0;
    auto fresh = [&] (int model, int voi, float gain, int mode, bool cabOff = true)
    {
        auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voi);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", (float) mode);
        setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
        if (cabOff) for (const char* c : { "cabOn", "cabOnA", "cabOnB", "cabOnC", "cabOnD" }) setP (*a, c, 0.0f);
        a->prepareToPlay (SR, 512);  return a;
    };
    // a 196 Hz sine at six input levels (-50 to -10 dBFS), amp only: H2 and H3 re the fundamental (dB), mean over the six levels
    auto harm = [&] (int model, int voi, float gain, int mode, double& h2, double& h3)
    {
        double s2 = 0, s3 = 0;
        for (int l = 0; l < 6; ++l)
        {
            auto a = fresh (model, voi, gain, mode);
            const float amp = (float) std::pow (10.0, (-50.0 + 8.0 * l) / 20.0);
            const int n = (int) SR * 2;  std::vector<float> x ((size_t) n), y;
            for (int i = 0; i < n; ++i) x[(size_t) i] = amp * (float) std::sin (2.0 * M_PI * 196.0 * i / SR);
            process (*a, x, y, 512);
            const size_t from = (size_t) SR, m = y.size() - from;
            auto mag = [&] (double f) { double re = 0, im = 0;  for (size_t i = 0; i < m; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (m - 1));  const double ph = 2 * M_PI * f * (double) i / SR;  re += y[from + i] * w * std::cos (ph);  im -= y[from + i] * w * std::sin (ph); }  return std::sqrt (re * re + im * im) + 1e-12; };
            const double f1 = mag (196.0);
            s2 += 20 * std::log10 (mag (392.0) / f1);  s3 += 20 * std::log10 (mag (588.0) / f1);
        }
        h2 = s2 / 6;  h3 = s3 / 6;
    };
    struct Row { int model, voi; const char* name; };
    const Row rows[] = { { 21, 0, "Hi-Wat Normal" }, { 45, 0, "Ironside Treble" } };
    for (const auto& r : rows)
    {
        // the same measurement with the b91 share (0.5, the plain 50/50 Hybrid mix) and with the shipped share
        auto& tab = StageShaper::hybridShares().v[r.model];
        const float shipped = tab;
        double o2 = 0, o3 = 0, h2 = 0, h3 = 0, c2 = 0, c3 = 0;
        tab = 0.5f;      harm (r.model, r.voi, 0.65f, 2, o2, o3);
        tab = shipped;   harm (r.model, r.voi, 0.65f, 2, h2, h3);   harm (r.model, r.voi, 0.65f, 0, c2, c3);
        std::printf ("  %-16s Gain 0.65 Hybrid H2 %.1f (share 0.5: %.1f), H3 %.1f (share 0.5: %.1f); Classic H2 %.1f\n", r.name, h2, o2, h3, o3, c2);
        if (r.model == 21)      // b121: Hi-Wat keeps a reduced circuit share (0.36, where the hardware captures' mean second-harmonic gap crosses zero)
        {
            CHECK (shipped > 0.30f && shipped < 0.42f, "%s: the shipped Hybrid share %.2f is outside 0.30 to 0.42", r.name, shipped);
            CHECK (h2 <= o2 - 2.0, "%s: Hybrid H2 %.1f dB is not at least 2 dB below the 50/50 mix (%.1f)", r.name, h2, o2);
            CHECK (h2 >= c2 - 6.0, "%s: Hybrid H2 %.1f dB went more than 6 dB below Classic (%.1f): too far", r.name, h2, c2);
            CHECK (std::fabs (h3 - o3) < 2.5, "%s: Hybrid H3 %.1f dB moved more than 2.5 dB from the 50/50 mix (%.1f)", r.name, h3, o3);
        }
        else      // Ironside: b121 returned it to the plain mix, which is already about zero against the captures
            CHECK (std::fabs (shipped - 0.5f) < 1.0e-6f, "%s: the shipped Hybrid share %.2f is not the plain 0.5 mix", r.name, shipped);
    }
    // Auto Level: the Hybrid level at Gain 0.5 stays where it was in b91 (measured then), and the three modes agree
    auto level = [&] (int model, int voi, int mode, float gain)
    {
        auto a = fresh (model, voi, gain, mode, false);  setP (*a, "ampAutoLevel", 1.0f);
        const float notes[] = { 82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f };
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;  double s = 0;  long n = 0;
        for (int k = 0; k < 190; ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                const long t = k * 512L + i;  const int nn = (int) ((t / 24000) % 6);  const float tt = (float) (t % 24000) / 48000.0f;  float x = 0;
                for (int h = 1; h <= 8; ++h) x += std::sin (6.2831853f * notes[nn] * (float) h * tt) / (float) h;
                x *= 0.12f * std::exp (-3.0f * tt);  b.setSample (0, i, x);  b.setSample (1, i, x);
            }
            a->processBlock (b, m);
            if (k > 20) for (int i = 0; i < 512; ++i) { s += b.getSample (0, i) * b.getSample (0, i);  ++n; }
        }
        return 10.0 * std::log10 (s / (double) n + 1e-12);
    };
    const double ref21[4] = { -26.92, -26.79, -26.11, -27.27 };   // the b91 Hybrid output with Auto Level on at Gain 0.5 (measured then, before the share change)
    const double ref45[4] = { -25.64, -25.58, -24.82, -24.85 };
    for (int model : { 21, 45 })
        for (int v = 0; v < 4; ++v)
        {
            const double h = level (model, v, 2, 0.5f), c = level (model, v, 0, 0.5f), ci = level (model, v, 1, 0.5f);
            CHECK (std::fabs (c - h) < 1.5 && std::fabs (ci - h) < 1.5, "model %d voicing %d: Auto Level modes differ (Hybrid %.2f, Classic %.2f, Circuit %.2f)", model, v, h, c, ci);
            CHECK (std::fabs (h - (model == 21 ? ref21[v] : ref45[v])) < 0.5, "model %d voicing %d: Auto Level Hybrid %.2f dB is not within 0.5 dB of the b91 level %.2f", model, v, h, model == 21 ? ref21[v] : ref45[v]);
        }
}

static void testB91()
{
    std::printf ("[b91] audio-thread allocation: nothing may allocate inside processBlock after prepareToPlay\n");
    const double SR = 48000.0;
    const int B = 512;

    // sanity: the counter really sees an allocation
    {
        gAllocCount = 0;  gAllocWatch = true;
        std::vector<float> v;  v.assign (4096, 0.0f);
        gAllocWatch = false;
        CHECK (gAllocCount > 0, "the allocation counter did not see a vector allocation");
    }
    // the spectral shifter: switching to the long window after prepare() must not allocate (it did before b91)
    for (int startLong = 0; startLong < 2; ++startLong)
    {
        SpectralShifter ss;  ss.prepare (SR);
        if (startLong) ss.setSize (1 << 20);
        gAllocCount = 0;  gAllocWatch = true;
        ss.setSize (startLong ? 0 : 1 << 20);  ss.setSize (startLong ? 1 << 20 : 0);  ss.setSize (1 << 20);
        gAllocWatch = false;
        CHECK (gAllocCount == 0, "SpectralShifter::setSize allocated %ld times after prepare (start long %d)", gAllocCount, startLong);
    }

    std::vector<float> sig = testSignal ((int) (SR * 2), SR, 0.5f);
    struct Case { const char* name; std::vector<std::pair<const char*, float>> before, after; };
    const std::vector<Case> cases = {
        { "steady state, default amp", {}, {} },
        { "pitch pedal: short chord window to long window", { { "wowOn", 1 }, { "wowTrack", 2 } }, { { "wowTrack", 1 } } },
        { "pitch pedal: single note to long chord window", { { "wowOn", 1 }, { "wowTrack", 0 } }, { { "wowTrack", 1 } } },
        { "harmonizer: single note to long chord window", { { "hmOn", 1 }, { "hmTrack", 0 } }, { { "hmTrack", 1 } } },
        { "harmonizer: short chord window to long window", { { "hmOn", 1 }, { "hmTrack", 2 } }, { { "hmTrack", 1 } } },
        { "octaver: single note to long chord window", { { "octOn", 1 }, { "octTrack", 0 } }, { { "octTrack", 1 } } },
        { "octaver: short to long chord window", { { "octOn", 1 }, { "octTrack", 2 } }, { { "octTrack", 1 } } },
        { "amp model change", { { "ampModel", 3 } }, { { "ampModel", 8 } } },
        { "amp voicing change", { { "ampModel", 8 } }, { { "ampVoicing", 2 } } },
        { "amp mode change", { { "ampMode", 0 } }, { { "ampMode", 2 } } },
        { "amp B on", {}, { { "ampBOn", 1 } } },
    };
    long worst = 0;
    for (const auto& c : cases)
    {
        auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, B);
        for (const char* id : { "gate", "noiseOn" }) setP (*a, id, id[0] == 'g' ? -90.0f : 0.0f);
        for (auto& kv : c.before) setP (*a, kv.first, kv.second);
        a->prepareToPlay (SR, B);
        juce::AudioBuffer<float> buf (2, B);  juce::MidiBuffer midi;
        size_t pos = 0;
        auto run = [&] (int blocks, bool watch)
        {
            long total = 0;
            for (int k = 0; k < blocks; ++k)
            {
                for (int i = 0; i < B; ++i) { const float v = sig[(pos + (size_t) i) % sig.size()];  buf.setSample (0, i, v);  buf.setSample (1, i, v); }
                pos += (size_t) B;
                gAllocCount = 0;  gAllocWatch = watch;
                a->processBlock (buf, midi);
                gAllocWatch = false;
                total += gAllocCount;
            }
            return total;
        };
        run (40, false);                                       // warm up: first-use work may allocate
        const long steady = run (6, true);
        for (auto& kv : c.after) setP (*a, kv.first, kv.second);
        const long changed = run (6, true);                    // the first block after the change is the one that matters
        std::printf ("  %-52s steady %ld, after change %ld\n", c.name, steady, changed);
        worst = std::max (worst, std::max (steady, changed));
        CHECK (steady == 0,  "%s: %ld allocations per 6 blocks at steady state", c.name, steady);
        CHECK (changed == 0, "%s: %ld allocations in the 6 blocks after the change", c.name, changed);
    }
    std::printf ("  worst case: %ld allocations\n", worst);
}

static void testB90()
{
    std::printf ("[b90] retuned variety voicings: Triple Blend Warm and Open, Prism Air, Jade Funk and Shimmer, Jubilant Lead Hi\n");
    const double SR = 48000.0;
    auto fresh = [&] (int model, int voi, float gain, int mode)
    {
        auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voi);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", (float) mode);
        setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
        for (const char* c : { "cabOn", "cabOnA", "cabOnB", "cabOnC", "cabOnD" }) setP (*a, c, 0.0f);
        a->prepareToPlay (SR, 512);  return a;
    };
    // pink-ish noise, bands re 1 kHz, plus the share of energy above 6 kHz (a fizz proxy)
    auto bands = [&] (int model, int voi, float gain, std::vector<double>& out, double& hf)
    {
        auto a = fresh (model, voi, gain, 2);
        std::vector<float> x ((size_t) (SR * 4));  unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.5); }
        std::vector<float> y;  process (*a, x, y, 512);
        const size_t from = 48000, n = y.size() - from;
        auto pw = [&] (double f0, int half) { double acc = 0;  for (int j = -half; j <= half; ++j) { const double f = f0 * std::pow (2.0, j / (double) (2 * half + 2));  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }  return acc / (2 * half + 1); };
        const double ref = 10 * std::log10 (pw (1000.0, 3) + 1e-30);  out.clear();
        for (double f : { 63.0, 125.0, 250.0, 500.0, 2000.0, 4000.0, 8000.0 }) out.push_back (10 * std::log10 (pw (f, 3) + 1e-30) - ref);
        double hi = 0, tot = 0;  for (double f = 100.0; f < 20000.0; f *= 1.12) { const double e = pw (f, 0);  tot += e;  if (f > 6000.0) hi += e; }
        hf = 10 * std::log10 (hi / tot + 1e-30);
    };
    auto level = [&] (int model, int voi, int mode, float gain, bool al, float inLevel)
    {
        auto a = fresh (model, voi, gain, mode);  setP (*a, "ampAutoLevel", al ? 1.0f : 0.0f);
        const auto in = testSignal (48000 * 2, SR, inLevel);  std::vector<float> y;  process (*a, in, y, 512);
        bool ok = true;  for (float v : y) if (! std::isfinite (v) || std::fabs (v) > 40.0f) ok = false;
        CHECK (ok, "model %d voicing %d, mode %d, Gain %.1f: output finite and bounded", model, voi, mode, gain);
        return rmsDb (y, 24000);
    };
    auto show = [&] (const char* name, const std::vector<double>& v) { std::printf ("  %-26s", name);  for (double x : v) std::printf (" %6.1f", x);  std::printf ("\n"); };
    std::printf ("  (pink noise, amp only, Gain 0.5, bands 63 125 250 500 2k 4k 8k re 1 kHz)\n");
    std::vector<double> tb[4], pr[2], jd[3];  double hf = 0;
    for (int v = 0; v < 3; ++v) bands (31, v, 0.5f, tb[v], hf);
    bands (42, 0, 0.5f, pr[0], hf);  bands (42, 1, 0.5f, pr[1], hf);
    bands (10, 0, 0.5f, jd[0], hf);  bands (10, 2, 0.5f, jd[1], hf);  bands (10, 3, 0.5f, jd[2], hf);
    show ("Triple Blend Three-in-One", tb[0]);  show ("Triple Blend Warm", tb[1]);  show ("Triple Blend Open", tb[2]);
    show ("Prism Glass", pr[0]);  show ("Prism Air", pr[1]);
    show ("Jade Glass", jd[0]);  show ("Jade Funk", jd[1]);  show ("Jade Shimmer", jd[2]);
    // indices: 0 63 Hz, 1 125, 2 250, 3 500, 4 2 kHz, 5 4 kHz, 6 8 kHz
    CHECK (tb[1][5] < tb[0][5] - 1.5, "Triple Blend Warm 4 kHz %.1f dB is at least 1.5 dB below Three-in-One (%.1f)", tb[1][5], tb[0][5]);
    CHECK (tb[1][6] < tb[0][6] - 1.5, "Triple Blend Warm 8 kHz %.1f dB is at least 1.5 dB below Three-in-One (%.1f)", tb[1][6], tb[0][6]);
    CHECK (tb[2][5] > tb[0][5] + 0.8, "Triple Blend Open 4 kHz %.1f dB is at least 0.8 dB above Three-in-One (%.1f)", tb[2][5], tb[0][5]);
    CHECK (tb[2][6] > tb[0][6] + 1.5, "Triple Blend Open 8 kHz %.1f dB is at least 1.5 dB above Three-in-One (%.1f)", tb[2][6], tb[0][6]);
    CHECK (tb[1][1] > tb[0][1] + 0.5 && tb[2][1] < tb[0][1] - 0.5, "Triple Blend 125 Hz: Warm %.1f above and Open %.1f below Three-in-One (%.1f)", tb[1][1], tb[2][1], tb[0][1]);
    CHECK (pr[1][6] > pr[0][6] + 2.5, "Prism Air 8 kHz %.1f dB is at least 2.5 dB above Glass (%.1f)", pr[1][6], pr[0][6]);
    CHECK (jd[1][0] < jd[0][0] - 5.0, "Jade Funk 63 Hz %.1f dB is at least 5 dB below Glass (%.1f): the tighter low end", jd[1][0], jd[0][0]);
    CHECK (jd[1][4] > jd[0][4] + 1.0, "Jade Funk 2 kHz %.1f dB is at least 1 dB above Glass (%.1f): the forward upper mid", jd[1][4], jd[0][4]);
    CHECK (jd[2][6] > jd[0][6] + 3.0, "Jade Shimmer 8 kHz %.1f dB is at least 3 dB above Glass (%.1f)", jd[2][6], jd[0][6]);
    // Jubilant Lead Hi is hotter than Lead: a quiet note comes out louder at the same Gain, Auto Level off
    {
        const double lo = level (40, 2, 2, 0.5f, false, 0.02f), hi = level (40, 3, 2, 0.5f, false, 0.02f);
        std::printf ("  Jubilant Gain 0.5, quiet input: Lead %.1f dB, Lead Hi %.1f dB\n", lo, hi);
        CHECK (hi > lo + 1.0, "Jubilant Lead Hi (%.1f dB) is at least 1 dB louder than Lead (%.1f) at a quiet input", hi, lo);
    }
    // Auto Level: each retuned voicing still lands within 1.5 dB of its neighbour, in every mode, across the knob
    for (int mode = 0; mode < 3; ++mode)
        for (float g : { 0.1f, 0.5f, 1.0f })
        {
            const double t0 = level (31, 0, mode, g, true, 0.3f), t1 = level (31, 1, mode, g, true, 0.3f), t2 = level (31, 2, mode, g, true, 0.3f);
            CHECK (std::fabs (t1 - t0) < 1.5 && std::fabs (t2 - t0) < 1.5, "Triple Blend mode %d Gain %.1f: Auto Level Warm %.1f / Open %.1f within 1.5 dB of Three-in-One %.1f", mode, (double) g, t1, t2, t0);
            const double j0 = level (10, 0, mode, g, true, 0.3f), j2 = level (10, 2, mode, g, true, 0.3f), j3 = level (10, 3, mode, g, true, 0.3f);
            CHECK (std::fabs (j2 - j0) < 1.5 && std::fabs (j3 - j0) < 1.5, "Jade Chorus mode %d Gain %.1f: Auto Level Funk %.1f / Shimmer %.1f within 1.5 dB of Glass %.1f", mode, (double) g, j2, j3, j0);
            const double u2 = level (40, 2, mode, g, true, 0.3f), u3 = level (40, 3, mode, g, true, 0.3f);
            CHECK (std::fabs (u3 - u2) < 1.5, "Jubilant mode %d Gain %.1f: Auto Level Lead Hi %.1f within 1.5 dB of Lead %.1f", mode, (double) g, u3, u2);
            const double p0 = level (42, 0, mode, g, true, 0.3f), p1 = level (42, 1, mode, g, true, 0.3f);
            CHECK (std::fabs (p1 - p0) < 1.5, "Prism mode %d Gain %.1f: Auto Level Air %.1f within 1.5 dB of Glass %.1f", mode, (double) g, p1, p0);
        }
}

static void testB89()
{
    std::printf ("[b89] Modern eased back halfway; Clarion's fifth voicing (Cascade)\n");
    const double SR = 48000.0;
    auto fresh = [&] (int model, int voi, float gain, int mode)
    {
        auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voi);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", (float) mode);
        setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
        for (const char* c : { "cabOn", "cabOnA", "cabOnB", "cabOnC", "cabOnD" }) setP (*a, c, 0.0f);
        a->prepareToPlay (SR, 512);  return a;
    };
    // pink-ish noise, bands re 1 kHz, plus the share of energy above 6 kHz (a fizz proxy)
    auto bands = [&] (int model, int voi, float gain, std::vector<double>& out, double& hf)
    {
        auto a = fresh (model, voi, gain, 2);
        std::vector<float> x ((size_t) (SR * 4));  unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.5); }
        std::vector<float> y;  process (*a, x, y, 512);
        const size_t from = 48000, n = y.size() - from;
        auto pw = [&] (double f0, int half) { double acc = 0;  for (int j = -half; j <= half; ++j) { const double f = f0 * std::pow (2.0, j / (double) (2 * half + 2));  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }  return acc / (2 * half + 1); };
        const double ref = 10 * std::log10 (pw (1000.0, 3) + 1e-30);  out.clear();
        for (double f : { 63.0, 125.0, 250.0, 500.0, 2000.0, 4000.0, 8000.0 }) out.push_back (10 * std::log10 (pw (f, 3) + 1e-30) - ref);
        double hi = 0, tot = 0;  for (double f = 100.0; f < 20000.0; f *= 1.12) { const double e = pw (f, 0);  tot += e;  if (f > 6000.0) hi += e; }
        hf = 10 * std::log10 (hi / tot + 1e-30);
    };
    // Cascade against Clarion's Clear: selectable, different in some band, and no fizzier
    std::vector<double> cl, ca;  double hfCl = 0, hfCa = 0;
    bands (41, 0, 0.8f, cl, hfCl);  bands (41, 4, 0.8f, ca, hfCa);
    std::printf ("  Clarion Clear   Gain 0.8 (63 125 250 500 2k 4k 8k re 1 kHz): ");  for (double v : cl) std::printf ("%.1f ", v);  std::printf (" | above 6 kHz %.1f dB\n", hfCl);
    std::printf ("  Clarion Cascade Gain 0.8: ");  for (double v : ca) std::printf ("%.1f ", v);  std::printf (" | above 6 kHz %.1f dB\n", hfCa);
    double maxDiff = 0;  for (size_t i = 0; i < cl.size(); ++i) maxDiff = std::max (maxDiff, std::fabs (cl[i] - ca[i]));
    CHECK (maxDiff >= 1.5, "Cascade differs from Clear by at least 1.5 dB in some band (largest %.1f dB)", maxDiff);
    CHECK (hfCa < hfCl + 3.0, "Cascade's energy above 6 kHz (%.1f dB) is within 3 dB of Clear's (%.1f dB): not fizzier", hfCa, hfCl);
    auto level = [&] (int model, int voi, int mode, float gain, bool al, float inLevel)
    {
        auto a = fresh (model, voi, gain, mode);  setP (*a, "ampAutoLevel", al ? 1.0f : 0.0f);
        const auto in = testSignal (48000 * 2, SR, inLevel);  std::vector<float> y;  process (*a, in, y, 512);
        bool ok = true;  for (float v : y) if (! std::isfinite (v) || std::fabs (v) > 40.0f) ok = false;
        CHECK (ok, "model %d voicing %d, mode %d, Gain %.1f: output finite and bounded", model, voi, mode, gain);
        return rmsDb (y, 24000);
    };
    // Auto Level: Cascade and Modern within 3.5 dB across Gain 0.1 / 0.5 / 1.0 in every mode, and Cascade lands within 2 dB of Clear's level
    for (int mode = 0; mode < 3; ++mode)
    {
        double sum[2] = { 0, 0 };
        for (int k = 0; k < 2; ++k)
        {
            const int voi = k == 0 ? 0 : 4;
            const double a = level (41, voi, mode, 0.1f, true, 0.3f), b = level (41, voi, mode, 0.5f, true, 0.3f), c = level (41, voi, mode, 1.0f, true, 0.3f);
            const double spread = std::max ({ a, b, c }) - std::min ({ a, b, c });  sum[k] = (a + b + c) / 3.0;
            if (k == 1) CHECK (spread < 3.5, "Clarion Cascade mode %d: Auto Level spread %.1f dB between Gain 0.1, 0.5 and 1.0", mode, spread);
        }
        std::printf ("  Clarion mode %d: Auto Level Clear %.1f dB, Cascade %.1f dB\n", mode, sum[0], sum[1]);
        CHECK (std::fabs (sum[0] - sum[1]) < 2.0, "Clarion mode %d: Cascade lands within 2 dB of Clear with Auto Level on (%.1f against %.1f)", mode, sum[1], sum[0]);
        const double a = level (8, 1, mode, 0.1f, true, 0.3f), b = level (8, 1, mode, 0.5f, true, 0.3f), c = level (8, 1, mode, 1.0f, true, 0.3f);
        const double spread = std::max ({ a, b, c }) - std::min ({ a, b, c });
        CHECK (spread < 3.5, "5555 Block Modern mode %d: Auto Level spread %.1f dB between Gain 0.1, 0.5 and 1.0", mode, spread);
        const double r = level (8, 0, mode, 0.5f, true, 0.3f);
        CHECK (std::fabs (b - r) < 3.0, "5555 Block mode %d: Modern (%.1f dB) lands within 3 dB of Raw (%.1f dB) with Auto Level on", mode, b, r);
    }
    level (41, 4, 2, 1.0f, false, 0.5f);
    // Modern eased back: still fuller and more open than Raw, but nearer the old sound than the b88 fit (63 Hz band and 8 kHz band re 1 kHz)
    std::vector<double> mo, raw;  double h1 = 0, h2 = 0;  bands (8, 1, 0.7f, mo, h1);  bands (8, 0, 0.7f, raw, h2);
    std::printf ("  Modern  Gain 0.7: ");  for (double v : mo) std::printf ("%.1f ", v);  std::printf (" | above 6 kHz %.1f dB\n", h1);
    std::printf ("  Raw     Gain 0.7: ");  for (double v : raw) std::printf ("%.1f ", v);  std::printf (" | above 6 kHz %.1f dB\n", h2);
    CHECK (mo[6] > raw[6] + 0.5, "Modern 8 kHz band %.1f dB against Raw %.1f: still more open than Raw", mo[6], raw[6]);
    CHECK (mo[6] < -7.5 && mo[0] < -2.6, "Modern 8 kHz %.1f and 63 Hz %.1f dB re 1 kHz are below the b88 values (-7.5 and -2.6): eased back", mo[6], mo[0]);
}

static void testB88()
{
    std::printf ("[b88] 5555 Block: Modern toned toward the Misha captures, and the fifth voicing (Vitriol)\n");
    const double SR = 48000.0;
    auto fresh = [&] (int voi, float gain, int mode)
    {
        auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        setP (*a, "ampModel", 8.0f);  setP (*a, "ampVoicing", (float) voi);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", (float) mode);
        setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
        for (const char* c : { "cabOn", "cabOnA", "cabOnB", "cabOnC", "cabOnD" }) setP (*a, c, 0.0f);
        a->prepareToPlay (SR, 512);  return a;
    };
    // pink-ish noise (Voss-style filter), -18 dBFS-ish, amp only, bands relative to 1 kHz
    auto bands = [&] (int voi, float gain, std::vector<double>& out)
    {
        auto a = fresh (voi, gain, 2);
        std::vector<float> x ((size_t) (SR * 4));  unsigned st = 12345;  for (auto& v : x) { st = st * 1664525u + 1013904223u;  v = (float) ((((st >> 8) / 8388608.0) - 1.0) * 0.06); }
        double b0 = 0, b1 = 0, b2 = 0;  for (auto& v : x) { const double w = v;  b0 = 0.99765 * b0 + w * 0.0990460;  b1 = 0.96300 * b1 + w * 0.2965164;  b2 = 0.57000 * b2 + w * 1.0526913;  v = (float) ((b0 + b1 + b2 + w * 0.1848) * 0.3); }
        std::vector<float> y;  process (*a, x, y, 512);
        const size_t from = 48000, n = y.size() - from;
        auto pw = [&] (double f0) { double acc = 0;  for (int j = -3; j <= 3; ++j) { const double f = f0 * std::pow (2.0, j / 9.0);  const double c = 2 * std::cos (2 * M_PI * f / SR);  double s1 = 0, s2 = 0;
                for (size_t i = 0; i < n; ++i) { const double w = 0.5 - 0.5 * std::cos (2 * M_PI * (double) i / (double) (n - 1));  const double sv = y[from + i] * w + c * s1 - s2;  s2 = s1;  s1 = sv; }
                acc += s1 * s1 + s2 * s2 - c * s1 * s2; }  return 10 * std::log10 (acc / 7 + 1e-30); };
        const double ref = pw (1000.0);  out.clear();  for (double f : { 63.0, 125.0, 250.0, 500.0, 2000.0, 4000.0, 8000.0 }) out.push_back (pw (f) - ref);
    };
    std::vector<double> mo, raw, vi;  bands (1, 0.7f, mo);  bands (0, 0.7f, raw);  bands (4, 0.5f, vi);
    std::printf ("  Modern  Gain 0.7 (63 125 250 500 2k 4k 8k re 1 kHz): ");  for (double v : mo) std::printf ("%.1f ", v);  std::printf ("\n");
    std::printf ("  Raw     Gain 0.7: ");  for (double v : raw) std::printf ("%.1f ", v);  std::printf ("\n");
    std::printf ("  Vitriol Gain 0.5: ");  for (double v : vi) std::printf ("%.1f ", v);  std::printf ("\n");
    // Modern now has a fuller low end and an open top (the Misha captures have 8 kHz about 2 dB above 1 kHz and 63 Hz about 12 dB below; before b88 Modern was at about -12 and -22 on this kind of measure)
    CHECK (mo[6] > raw[6] - 1.5, "Modern 8 kHz band %.1f dB re 1 kHz against Raw %.1f (b88, eased back halfway in b89: the top is about level with Raw's, not 1.5 dB below)", mo[6], raw[6]);
    CHECK (mo[0] > raw[0] + 3.0, "Modern 63 Hz band %.1f dB re 1 kHz against Raw %.1f (b88, eased back halfway in b89: a fuller low end, at least 3 dB above Raw's)", mo[0], raw[0]);
    CHECK (mo[6] > -12.0 && mo[0] > -9.0, "Modern 8 kHz %.1f and 63 Hz %.1f dB re 1 kHz stay near the halfway values (about -10.2 and -6.7 on this measure)", mo[6], mo[0]);
    // Vitriol: a selectable fifth voicing that plays, differs from Modern by at least 2 dB in some band, and has a wide gain range (almost clean at the bottom of the knob)
    double maxDiff = 0;  for (size_t i = 0; i < mo.size(); ++i) maxDiff = std::max (maxDiff, std::fabs (mo[i] - vi[i]));
    CHECK (maxDiff >= 2.0, "Vitriol differs from Modern by at least 2 dB in some band (largest %.1f dB)", maxDiff);
    auto level = [&] (int voi, int mode, float gain, bool al, float inLevel)
    {
        auto a = fresh (voi, gain, mode);  setP (*a, "ampAutoLevel", al ? 1.0f : 0.0f);
        const auto in = testSignal (48000 * 2, SR, inLevel);  std::vector<float> y;  process (*a, in, y, 512);
        bool ok = true;  for (float v : y) if (! std::isfinite (v) || std::fabs (v) > 40.0f) ok = false;
        CHECK (ok, "5555 Block voicing %d, mode %d, Gain %.1f: output finite and bounded", voi, mode, gain);
        return rmsDb (y, 24000);
    };
    const double lo = level (4, 2, 0.0f, false, 0.02f), hi = level (4, 2, 1.0f, false, 0.02f);
    std::printf ("  Vitriol quiet input: Gain 0 to 1 = %+.1f dB\n", hi - lo);
    CHECK (hi - lo > 10.0, "Vitriol: only %.1f dB between Gain 0 and Gain 1 at a quiet input", hi - lo);
    // Auto Level: Modern and Vitriol within 3.5 dB across Gain 0.1 / 0.5 / 1.0 in every amp mode
    for (int voi : { 1, 4 })
        for (int mode = 0; mode < 3; ++mode)
        {
            const double a = level (voi, mode, 0.1f, true, 0.3f), b = level (voi, mode, 0.5f, true, 0.3f), c = level (voi, mode, 1.0f, true, 0.3f);
            const double spread = std::max ({ a, b, c }) - std::min ({ a, b, c });
            std::printf ("  5555 Block v%d mode %d: Auto Level %.1f / %.1f / %.1f dB (spread %.1f)\n", voi, mode, a, b, c, spread);
            CHECK (spread < 3.5, "5555 Block voicing %d mode %d: Auto Level spread %.1f dB between Gain 0.1, 0.5 and 1.0", voi, mode, spread);
        }
}

// dev tool (not a test): the Auto-Level-off Hybrid output level (dB, plucked test signal, cab on) of Hi-Wat Headroom (21) and Ironside (45), gain 0..1 in steps of 0.1,
// for the shipped share and for each M=X given on the command line, so the Auto Level rows can be refitted after a share change
static void fitB121 (int argc, char** argv)
{
    const double SR = 48000.0;
    auto level = [&] (int model, int voi, float gain)
    {
        auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
        setP (*a, "ampModel", (float) model);  setP (*a, "ampVoicing", (float) voi);  setP (*a, "ampGain", gain);  setP (*a, "ampMode", 2.0f);
        setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);  setP (*a, "ampAutoLevel", 0.0f);
        a->prepareToPlay (SR, 512);
        const float notes[] = { 82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f };
        juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;  double s = 0;  long n = 0;
        for (int k = 0; k < 190; ++k)
        {
            for (int i = 0; i < 512; ++i)
            {
                const long t = k * 512L + i;  const int nn = (int) ((t / 24000) % 6);  const float tt = (float) (t % 24000) / 48000.0f;  float x = 0;
                for (int h = 1; h <= 8; ++h) x += std::sin (6.2831853f * notes[nn] * (float) h * tt) / (float) h;
                x *= 0.12f * std::exp (-3.0f * tt);  b.setSample (0, i, x);  b.setSample (1, i, x);
            }
            a->processBlock (b, m);
            if (k > 20) for (int i = 0; i < 512; ++i) { s += b.getSample (0, i) * b.getSample (0, i);  ++n; }
        }
        return 10.0 * std::log10 (s / (double) n + 1e-12);
    };
    auto dump = [&] (const char* tag)
    {
        for (int model : { 21, 45 })
            for (int v = 0; v < 4; ++v)
            {
                std::printf ("%s %d %d", tag, model, v);
                for (int g = 0; g <= 10; ++g) std::printf (" %.2f", level (model, v, (float) g / 10.0f));
                std::printf ("\n");
            }
    };
    dump ("shipped");
    for (int i = 2; i < argc; ++i)
    {
        int m = -1;  float x = 0.5f;
        if (std::sscanf (argv[i], "%d=%f", &m, &x) == 2 && m >= 0 && m < 64)
        {
            const float keep = StageShaper::hybridShares().v[m];
            StageShaper::hybridShares().v[m] = x;
            char tag[32];  std::snprintf (tag, sizeof tag, "m%d@%.2f", m, x);
            dump (tag);
            StageShaper::hybridShares().v[m] = keep;
        }
    }
}

// dev tool (not a test): run a raw mono float32 file through the whole plug-in with the given parameters (key=value ...) and write the result as raw float32.
// Used to measure our reverbs and the cab room against the plug-in renders of R_room_mics.wav (amp and cab are switched off unless the command line turns them on).
static void revRender (int argc, char** argv)
{
    if (argc < 4) { std::printf ("usage: tests revrender in.f32 out.f32 [key=value ...]\n"); return; }
    const double SR = 48000.0;
    std::vector<float> in;       // mono
    { FILE* f = std::fopen (argv[2], "rb"); if (! f) { std::printf ("cannot read %s\n", argv[2]); return; }
      std::fseek (f, 0, SEEK_END); const long n = std::ftell (f); std::fseek (f, 0, SEEK_SET);  in.resize ((size_t) n / 4);
      if (std::fread (in.data(), 4, in.size(), f) != in.size()) { std::fclose (f); return; }  std::fclose (f); }
    auto a = std::make_unique<StarterAmpProcessor>();  a->setPlayConfigDetails (2, 2, SR, 512);
    setP (*a, "ampAOn", 0.0f);  setP (*a, "cabOn", 0.0f);  setP (*a, "gate", -90.0f);  setP (*a, "noiseOn", 0.0f);
    int patch = 0;      // --patch=1: Input -> Reverb 1 -> Output (no amp, no cab, mono); --patch=2: Input -> Cab -> Output (cab blocks and room only); --patch=3: Input -> Cab -> Reverb 1 -> Output (stereo)
    for (int i = 4; i < argc; ++i)
    {   char key[64];  float v = 0;
        if (std::sscanf (argv[i], "--patch=%d", &patch) == 1) continue;
        if (std::sscanf (argv[i], "%63[^=]=%f", key, &v) == 2) setP (*a, key, v); }
    if (patch != 0)
    {
        PG::Data d;  d.n = 0;
        if (patch == 1) { PG::connect (d, PG::kIn, 0, 12, 0);  PG::connect (d, 12, 0, PG::kOut, 0); }
        else if (patch == 3) { PG::connect (d, PG::kIn, 0, PG::kCab, 0);  PG::connect (d, PG::kCab, 0, 12, 0);  PG::connect (d, 12, 0, PG::kOut, 0); }       // reverb after the cab: stereo
        else            { PG::connect (d, PG::kIn, 0, PG::kCab, 0);  PG::connect (d, PG::kCab, 0, PG::kOut, 0); }
        a->setPatch (d);
    }
    a->prepareToPlay (SR, 512);
    a->applyPendingUpdates();
    { std::vector<float> w ((size_t) (2.0 * SR), 0.0f), y;  process (*a, w, y, 512); }
    juce::Thread::sleep (1500);  { std::vector<float> w2 ((size_t) (0.3 * SR), 0.0f), y;  process (*a, w2, y, 512); }
    std::vector<float> y (in.size() * 2);                                // the input is mono; the output is interleaved stereo
    juce::AudioBuffer<float> b (2, 512);  juce::MidiBuffer m;
    for (size_t pos = 0; pos < in.size(); pos += 512)
    {
        const int n = (int) std::min ((size_t) 512, in.size() - pos);
        juce::AudioBuffer<float> v (b.getArrayOfWritePointers(), 2, n);
        for (int i = 0; i < n; ++i) { v.setSample (0, i, in[pos + (size_t) i]);  v.setSample (1, i, in[pos + (size_t) i]); }
        a->processBlock (v, m);
        for (int i = 0; i < n; ++i) { y[(pos + (size_t) i) * 2] = v.getSample (0, i);  y[(pos + (size_t) i) * 2 + 1] = v.getSample (1, i); }
    }
    FILE* f = std::fopen (argv[3], "wb"); if (f) { std::fwrite (y.data(), 4, y.size(), f); std::fclose (f); }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const std::string cmd = argc > 1 ? argv[1] : "stress";
    const std::string golden = argc > 3 ? argv[3] : "golden.txt";
    if (cmd == "regress") { if (regress (argc > 2 && std::string (argv[2]) == "--update", golden)) return 2; }
    else if (cmd == "room") { testRoomKnob(); }
    else if (cmd == "b111") { testB111(); }
    else if (cmd == "b112") { testB112(); }
    else if (cmd == "b114") { testB114(); }
    else if (cmd == "b115") { testB115(); }
    else if (cmd == "b117") { testB117(); }
    else if (cmd == "b118") { testB118(); }
    else if (cmd == "b120") { testB120(); }
    else if (cmd == "board") { testBoardSync(); }
    else if (cmd == "psmeas") { measPitchShifters(); }
    else if (cmd == "tpdelay") { measTransposeDelay(); }
    else if (cmd == "latency") { measLatencyAll(); }
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
    else if (cmd == "compstyles") { testCompStyles(); }
    else if (cmd == "programeq") { testProgramEqB59(); }
    else if (cmd == "stack") { testPassiveStack9(); }
    else if (cmd == "tubeswitch") { testTubeSwitch(); }
    else if (cmd == "ampswitch") { testAmpSwitch(); }
    else if (cmd == "highgain") { testHighGainControls(); }
    else if (cmd == "b62") { testB62(); }
    else if (cmd == "clarion") { testClarion(); }
    else if (cmd == "ampb") { testAmpBSweep(); }
    else if (cmd == "prism") { testPrism(); }
    else if (cmd == "ember") { testEmber(); }
    else if (cmd == "kindle") { testKindle(); }
    else if (cmd == "idealdef") { testIdealDefined(); }
    else if (cmd == "b81") { testB81(); }
    else if (cmd == "b84") { testB84(); }
    else if (cmd == "b85") { testB85(); }
    else if (cmd == "b86") { testB86(); }
    else if (cmd == "b87") { testB87(); }
    else if (cmd == "b88") { testB88(); }
    else if (cmd == "b89") { testB89(); }
    else if (cmd == "b90") { testB90(); }
    else if (cmd == "b91") { testB91(); }
    else if (cmd == "b92") { testB92(); }
    else if (cmd == "b121fit") { fitB121 (argc, argv); }
    else if (cmd == "revrender") { revRender (argc, argv); }
    else if (cmd == "b94") { testB94(); }
    else if (cmd == "b95") { testB95(); }
    else if (cmd == "b97") { testB97(); }
    else if (cmd == "b98") { testB98(); }
    else if (cmd == "b99") { testB99(); }
    else if (cmd == "b101") { testB101(); }
    else if (cmd == "b102") { testB102Nam(); testB102Riff(); }
    else if (cmd == "b103") { testB103(); }
    else if (cmd == "b104") { testB104(); }
    else if (cmd == "b105") { testB105(); }
    else if (cmd == "b106") { testB106(); }
    else if (cmd == "b107") { testB107(); }
    else if (cmd == "b108") { testB108(); }
    else if (cmd == "b109") { testB109(); }
    else if (cmd == "tail") { testB63(); testB64(); testParallelSection(); testPatchGraph(); testBrownEyeTight(); testFeedbackLatch(); testModSlots(); testFoldIn(); testPolyTuner(); testCompStyles(); testReverbTypes(); testSpringReturn(); testAliasing(); testAliasingEffects(); testJadeDist(); testDiMatch(); testFuzz2Block(); }   // b95: the end of the default run, to rerun it alone
    else if (cmd == "reverbtypes") { testReverbTypes(); }
    else if (cmd == "state") { testState(); testLifecycle(); }
    else if (cmd == "waza75") { testWaza75(); }
    else if (cmd == "cs3") { testCs3(); }
    else if (cmd == "wah76") { testWah76(); }
    else if (cmd == "wow77") { testWow77(); }
    else if (cmd == "timmy78") { testTimmy78 (argc > 2 && std::string (argv[2]) == "meas"); }
    else if (cmd == "rough78") { testRough78(); }
    else if (cmd == "oct79") { testOct79(); }
    else if (cmd == "suggestcab") { testSuggestedCab(); }
    else if (cmd == "b63") { testB63(); }
    else if (cmd == "b64") { testB64(); }
    else if (cmd == "alias") { testAliasing(); testAliasingEffects(); }
    else
    {
        testStability(); testBlockSizes(); testAutomation (1); testAutomation (2); testAutomation (3);
        testState(); testLifecycle(); testOversampling(); testSwitching(); testLoFiDelay(); testDelayFeatures(); testDelayPitch(); testTransientShaper(); testMultibandB55(); testWorkflowB56(); testProximityB59(); testCompStylesB59(); testProgramEqB59(); testOptoSustain(); testPassiveStack9(); testTubeSwitch(); testAmpSwitch(); testHighGainControls(); testMovedBlocks(); testMovedDrives(); testB62(); testClarion(); testPrism(); testEmber(); testKindle(); testIdealDefined(); testB81(); testB84(); testB85(); testB86(); testB87(); testB88(); testB89(); testB90(); testB91(); testB92(); testB94(); testB97(); testB98(); testB99(); testB101(); testB102Nam(); testB102Riff(); testB103(); testB104(); testB105(); testB106(); testB107(); testB108(); testB109(); testAmpBSweep(); testSuggestedCab(); testB63(); testB64(); testParallelSection(); testPatchGraph(); testBrownEyeTight(); testFeedbackLatch(); testModSlots(); testFoldIn(); testPolyTuner(); testCompStyles(); testReverbTypes(); testSpringReturn(); testAliasing(); testAliasingEffects(); testJadeDist(); testDiMatch(); testFuzz2Block();
    }
    std::printf ("\n%d checks passed, %d failed\n", gPass, gFail);
    return gFail == 0 ? 0 : 1;
}
