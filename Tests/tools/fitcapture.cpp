// fitcapture: rank every electric amp of the plug-in by how close it can get to an amp CAPTURE.
//
//   fitcapture --nam capture.nam                          built-in test signals (steady sine + plucked notes at six levels)
//   fitcapture --nam capture.nam --di dry.wav             same, plus a cross-check of the best matches on your own DI
//   fitcapture --di dry.wav --out recorded.wav            the capture is the recorded output of the same DI (aligned)
//   fitcapture --list                                     the amps and voicings that would be tried
//   fitcapture --render amp:13:2:0.65 --di d.wav --write o.wav     (helpers for self tests)
//   fitcapture --render nam:capture.nam --di d.wav --write o.wav
//
// Build and usage notes: Tests/tools/README.md.  Nothing in the plug-in sources is touched; the program links against them.
#include "PluginProcessor.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <climits>
#include <cstdarg>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

namespace {

//==============================================================================
// Fixed measurement settings (changing any of these changes the library signature)
//==============================================================================
const double kPi = 3.14159265358979323846;
constexpr int    NL = 6;                                             // standard input levels
const double     LEV[NL] = { -50, -42, -34, -26, -18, -10 };         // dB rms, full scale = 0 dB
constexpr int    MID = 3;                                            // level used for the band shape (-26 dB)
constexpr double FINE_MIN = -62, FINE_MAX = 2, FINE_STEP = 2;        // capture is measured on this grid so a level offset can be searched
constexpr int    NB = 8;
const double     BANDHZ[NB] = { 63, 125, 250, 500, 1000, 2000, 4000, 8000 };
const double     GAINS[5] = { 0.2, 0.35, 0.5, 0.65, 0.8 };
constexpr double FLOOR_DB = -60.0;                                   // harmonics below this (relative to the fundamental) count as -60: inaudible, and measurement noise
constexpr double SINE_HZ = 196.0;
constexpr double W_GAIN = 3, W_H2 = 2, W_H3 = 2, W_BAND = 1;
constexpr double TOL_GAIN = 2.0, TOL_H = 3.0, TOL_BAND = 3.0;
constexpr int    BLOCK = 512;
const char*      kSignalVersion = "synthetic-v2";

//==============================================================================
// Small helpers
//==============================================================================
double cpuSeconds() { return (double) std::clock() / (double) CLOCKS_PER_SEC; }
double wallSeconds() { return std::chrono::duration<double> (std::chrono::steady_clock::now().time_since_epoch()).count(); }
double db20 (double x) { return 20.0 * std::log10 (std::max (x, 1e-12)); }
double db10 (double x) { return 10.0 * std::log10 (std::max (x, 1e-20)); }
std::string fmt (const char* f, ...) __attribute__ ((format (printf, 1, 2)));
std::string fmt (const char* f, ...)
{
    char buf[1024]; va_list a; va_start (a, f); std::vsnprintf (buf, sizeof buf, f, a); va_end (a); return buf;
}
std::string lower (std::string s) { for (auto& c : s) c = (char) std::tolower ((unsigned char) c); return s; }
std::vector<std::string> split (const std::string& s, char sep)
{
    std::vector<std::string> o; std::string cur;
    for (char c : s) { if (c == sep) { o.push_back (cur); cur.clear(); } else cur += c; }
    o.push_back (cur); return o;
}

//==============================================================================
// WAV reading / writing (no JUCE needed, so it works before the message manager exists)
//==============================================================================
struct Wav { std::vector<float> x; int sr = 0; int channels = 0; };

bool readWav (const std::string& path, Wav& w, std::string& err)
{
    std::ifstream f (path, std::ios::binary);
    if (! f) { err = "cannot open " + path; return false; }
    std::vector<unsigned char> b ((std::istreambuf_iterator<char> (f)), std::istreambuf_iterator<char>());
    if (b.size() < 44 || std::memcmp (b.data(), "RIFF", 4) != 0 || std::memcmp (b.data() + 8, "WAVE", 4) != 0) { err = path + " is not a RIFF/WAVE file"; return false; }
    auto u16 = [&] (size_t o) { return (int) (b[o] | (b[o + 1] << 8)); };
    auto u32 = [&] (size_t o) { return (uint32_t) (b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | ((uint32_t) b[o + 3] << 24)); };
    int fmtTag = 0, ch = 0, sr = 0, bits = 0; size_t dOff = 0, dLen = 0; bool haveData = false;
    size_t p = 12;
    while (p + 8 <= b.size())
    {
        const uint32_t sz = u32 (p + 4);
        if (std::memcmp (&b[p], "fmt ", 4) == 0 && p + 8 + 16 <= b.size())
        {
            fmtTag = u16 (p + 8); ch = u16 (p + 10); sr = (int) u32 (p + 12); bits = u16 (p + 22);
            if (fmtTag == 0xFFFE && sz >= 40 && p + 8 + 26 <= b.size()) fmtTag = u16 (p + 8 + 24);
        }
        else if (std::memcmp (&b[p], "data", 4) == 0) { dOff = p + 8; dLen = std::min<size_t> (sz, b.size() - dOff); haveData = true; break; }
        p += 8 + (size_t) sz + (sz & 1u);
    }
    if (! haveData || ch < 1 || sr < 8000) { err = path + ": no usable fmt/data chunk"; return false; }
    const int bytes = bits / 8;
    if (! ((fmtTag == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32)) || (fmtTag == 3 && (bits == 32 || bits == 64)))) { err = path + fmt (": unsupported format (tag %d, %d bits)", fmtTag, bits); return false; }
    const size_t frames = dLen / (size_t) (bytes * ch);
    w.x.assign (frames, 0.0f); w.sr = sr; w.channels = ch;
    for (size_t i = 0; i < frames; ++i)
    {
        double acc = 0;
        for (int c = 0; c < ch; ++c)
        {
            const unsigned char* q = &b[dOff + (i * (size_t) ch + (size_t) c) * (size_t) bytes];
            double v = 0;
            if (fmtTag == 3 && bits == 32) { float t; std::memcpy (&t, q, 4); v = t; }
            else if (fmtTag == 3) { double t; std::memcpy (&t, q, 8); v = t; }
            else if (bits == 8)  v = ((int) q[0] - 128) / 128.0;
            else if (bits == 16) v = (int16_t) (q[0] | (q[1] << 8)) / 32768.0;
            else if (bits == 24) { int32_t t = (q[0] << 8) | (q[1] << 16) | (q[2] << 24); v = (t >> 8) / 8388608.0; }
            else { int32_t t = (int32_t) ((uint32_t) q[0] | ((uint32_t) q[1] << 8) | ((uint32_t) q[2] << 16) | ((uint32_t) q[3] << 24)); v = t / 2147483648.0; }
            acc += v;
        }
        w.x[i] = (float) (acc / ch);
    }
    return true;
}

bool writeWavFloat (const std::string& path, const std::vector<float>& x, int sr)
{
    std::ofstream f (path, std::ios::binary);
    if (! f) return false;
    auto w32 = [&] (uint32_t v) { f.write ((const char*) &v, 4); };
    auto w16 = [&] (uint16_t v) { f.write ((const char*) &v, 2); };
    const uint32_t dataBytes = (uint32_t) (x.size() * 4);
    f.write ("RIFF", 4); w32 (36 + dataBytes); f.write ("WAVE", 4);
    f.write ("fmt ", 4); w32 (16); w16 (3); w16 (1); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 4); w16 (4); w16 (32);
    f.write ("data", 4); w32 (dataBytes);
    f.write ((const char*) x.data(), (std::streamsize) dataBytes);
    return (bool) f;
}

uint64_t fnv1a (const std::vector<float>& x, size_t a, size_t n)
{
    uint64_t h = 1469598103934665603ull;
    for (size_t i = a; i < a + n && i < x.size(); ++i) { const int16_t q = (int16_t) std::lrint (std::max (-1.0f, std::min (1.0f, x[i])) * 30000.0f); h = (h ^ (uint16_t) q) * 1099511628211ull; }
    return h;
}

//==============================================================================
// Signal analysis
//==============================================================================
struct Pt   // everything measured at one input level
{
    double lev = -120, fund = -120, h2 = FLOOR_DB, h3 = FLOOR_DB;   // output level (dB rms), fundamental (dB), H2 and H3 relative to the fundamental (dB)
    double band[NB] = { 0 };                                       // octave band energies (dB, absolute)
    bool hasLev = false, hasH = false, hasBand = false;
};

// amplitude of one frequency in x[0..n) using a Hann window
double hannDft (const float* x, size_t n, double f, double sr)
{
    double re = 0, im = 0, ws = 0;
    const double w0 = 2.0 * kPi * f / sr;
    for (size_t i = 0; i < n; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) n);
        const double ph = w0 * (double) i;
        re += x[i] * w * std::cos (ph); im += x[i] * w * std::sin (ph); ws += w;
    }
    return 2.0 * std::sqrt (re * re + im * im) / std::max (ws, 1e-9);
}

// power spectrum of a Hann-windowed block of 2^order samples (bins 0..N/2)
void powerSpectrum (const float* x, int order, std::vector<double>& pw, double dcRemoveMean = 0.0)
{
    const int N = 1 << order;
    static thread_local std::map<int, std::unique_ptr<juce::dsp::FFT>> ffts;
    auto& fft = ffts[order]; if (! fft) fft.reset (new juce::dsp::FFT (order));
    std::vector<float> d ((size_t) 2 * N, 0.0f);
    for (int i = 0; i < N; ++i) d[(size_t) i] = (float) ((x[i] - dcRemoveMean) * (0.5 - 0.5 * std::cos (2.0 * kPi * i / N)));
    fft->performRealOnlyForwardTransform (d.data(), true);
    pw.assign ((size_t) N / 2 + 1, 0.0);
    for (int k = 0; k <= N / 2; ++k) pw[(size_t) k] = (double) d[(size_t) 2 * k] * d[(size_t) 2 * k] + (double) d[(size_t) 2 * k + 1] * d[(size_t) 2 * k + 1];
}
void bandEnergies (const std::vector<double>& pw, int order, double sr, double* e)
{
    const double N = (double) (1 << order);
    for (int b = 0; b < NB; ++b)
    {
        const double lo = BANDHZ[b] / std::sqrt (2.0), hi = BANDHZ[b] * std::sqrt (2.0);
        double s = 0; for (size_t k = 1; k < pw.size(); ++k) { const double f = (double) k * sr / N; if (f >= lo && f < hi) s += pw[k]; }
        e[b] = s;
    }
}

//==============================================================================
// Built-in test signals: a steady sine at 196 Hz and six plucked notes, at any rms level
//==============================================================================
std::vector<float> pluckUnit (double sr)     // six notes, 0.5 s each, 12 harmonics at 1/h, exp(-3t) decay; rms = 1
{
    const double nt[6] = { 82.4, 110.0, 146.8, 196.0, 246.9, 329.6 };
    const size_t nl = (size_t) (0.5 * sr);
    std::vector<float> x (nl * 6);
    double e = 0;
    for (int k = 0; k < 6; ++k)
        for (size_t i = 0; i < nl; ++i)
        {
            const double t = (double) i / sr; double s = 0;
            for (int h = 1; h <= 12; ++h) s += std::sin (2.0 * kPi * nt[k] * h * t) / h;
            s *= std::exp (-3.0 * t) * std::min (1.0, t / 0.003);
            x[(size_t) k * nl + i] = (float) s; e += s * s;
        }
    const double r = std::sqrt (e / (double) x.size());
    for (auto& v : x) v = (float) (v / r);
    return x;
}
struct Marks { size_t sineA, sineB, pluckA, pluckB; };
std::vector<float> makeStream (double levelDb, double sr, const std::vector<float>& pluck, Marks& m)
{
    const size_t lead = (size_t) (0.10 * sr), sine = (size_t) (1.5 * sr), gap = (size_t) (0.15 * sr), tail = (size_t) (0.15 * sr);
    std::vector<float> x (lead + sine + gap + pluck.size() + tail, 0.0f);
    const double g = std::pow (10.0, levelDb / 20.0), fade = 0.02 * sr;
    for (size_t i = 0; i < sine; ++i)
    {
        double w = 1.0; if ((double) i < fade) w = 0.5 - 0.5 * std::cos (kPi * (double) i / fade); else if ((double) (sine - i) < fade) w = 0.5 - 0.5 * std::cos (kPi * (double) (sine - i) / fade);
        x[lead + i] = (float) (g * std::sqrt (2.0) * std::sin (2.0 * kPi * SINE_HZ * (double) i / sr) * w);
    }
    m.pluckA = lead + sine + gap; m.pluckB = m.pluckA + pluck.size();
    for (size_t i = 0; i < pluck.size(); ++i) x[m.pluckA + i] = (float) (g * pluck[i]);
    m.sineA = lead + (size_t) (0.7 * sr); m.sineB = lead + sine - (size_t) (0.02 * sr);
    return x;
}
Pt analyzeStream (const std::vector<float>& y, const Marks& m, double sr)
{
    Pt p;
    {   // steady sine: fundamental, H2, H3
        const size_t n = m.sineB - m.sineA; double mean = 0; for (size_t i = 0; i < n; ++i) mean += y[m.sineA + i]; mean /= (double) n;
        std::vector<float> s (n); for (size_t i = 0; i < n; ++i) s[i] = (float) (y[m.sineA + i] - mean);
        const double a1 = hannDft (s.data(), n, SINE_HZ, sr), a2 = hannDft (s.data(), n, 2 * SINE_HZ, sr), a3 = hannDft (s.data(), n, 3 * SINE_HZ, sr);
        p.fund = db20 (a1); p.h2 = std::max (FLOOR_DB, db20 (a2 / std::max (a1, 1e-9))); p.h3 = std::max (FLOOR_DB, db20 (a3 / std::max (a1, 1e-9)));
        p.hasH = a1 > 1e-7;
    }
    {   // plucked notes: rms level and octave bands
        const size_t n = m.pluckB - m.pluckA; double mean = 0; for (size_t i = 0; i < n; ++i) mean += y[m.pluckA + i]; mean /= (double) n;
        double e = 0; for (size_t i = 0; i < n; ++i) { const double v = y[m.pluckA + i] - mean; e += v * v; }
        p.lev = db10 (e / (double) n); p.hasLev = p.lev > -110;
        int order = 17; while ((1u << order) > n && order > 10) --order;
        std::vector<double> pw; powerSpectrum (&y[m.pluckA], order, pw, mean);
        double be[NB]; bandEnergies (pw, order, sr, be); for (int b = 0; b < NB; ++b) p.band[b] = db10 (be[b]);
        p.hasBand = p.hasLev;
    }
    return p;
}

//==============================================================================
// Things that turn an input into an output: one of our amps, a NAM model, or a recording
//==============================================================================
struct AmpCfg { int model = 0, voicing = 0, mode = -1; double gain = 0.5; int quality = -1; };

void setParam (StarterAmpProcessor& p, const char* id, float v)
{
    auto* r = p.apvts.getParameter (id);
    if (r == nullptr) { std::fprintf (stderr, "warning: plug-in has no parameter '%s'\n", id); return; }
    r->setValueNotifyingHost (r->convertTo0to1 (v));
}
// A fresh processor every time: prepareToPlay() does not clear every internal state (measured: a reused instance differs from a fresh one), and a
// fresh instance costs about 25 ms.
std::vector<float> renderAmp (const AmpCfg& c, const std::vector<float>& in, double sr, int* latency = nullptr)
{
    StarterAmpProcessor p;
    p.setPlayConfigDetails (2, 2, sr, BLOCK);
    for (auto* prm : p.getParameters())                       // every switch off: pedals, delays, reverb, cab, second amp, NAM slot ...
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (r->paramID.endsWith ("On") && r->paramID != "ampAOn") r->setValueNotifyingHost (0.0f);      // (b65: Amp A's own switch stays on; before it was switched off with the rest and every capture and candidate measured silent)
    setParam (p, "gate", -90.0f);  setParam (p, "outGain", 0.0f);  setParam (p, "inGain", 0.0f);  setParam (p, "ampAutoLevel", 0.0f);
    setParam (p, "ampModel", (float) c.model);  setParam (p, "ampVoicing", (float) c.voicing);  setParam (p, "ampGain", (float) c.gain);
    if (c.mode >= 0)    setParam (p, "ampMode", (float) c.mode);
    if (c.quality >= 0) setParam (p, "ampQuality", (float) c.quality);
    p.prepareToPlay (sr, BLOCK);
    std::vector<float> out (in.size(), 0.0f);
    juce::AudioBuffer<float> b (2, BLOCK); juce::MidiBuffer midi;
    for (size_t pos = 0; pos < in.size(); pos += BLOCK)
    {
        const int n = (int) std::min<size_t> (BLOCK, in.size() - pos);
        b.clear();
        for (int i = 0; i < n; ++i) { b.setSample (0, i, in[pos + (size_t) i]); b.setSample (1, i, in[pos + (size_t) i]); }
        p.processBlock (b, midi);
        for (int i = 0; i < n; ++i) out[pos + (size_t) i] = b.getSample (0, i);
    }
    if (latency) *latency = p.getLatencySamples();
    return out;
}

struct Renderer { virtual ~Renderer() = default; virtual std::vector<float> run (const std::vector<float>& in) = 0; virtual std::string describe() const = 0; };
struct AmpRenderer : Renderer
{
    AmpCfg cfg; double sr;
    AmpRenderer (AmpCfg c, double s) : cfg (c), sr (s) {}
    std::vector<float> run (const std::vector<float>& in) override { return renderAmp (cfg, in, sr); }
    std::string describe() const override { return "our amp"; }
};
struct NamRenderer : Renderer
{
    std::unique_ptr<NamLite::Player> player; std::string desc; double sr; bool ok = false; std::string err;
    NamRenderer (const std::string& path, double s) : sr (s)
    {
        juce::String e; auto m = NamLite::loadFile (juce::File (juce::String (path)), e);
        if (m == nullptr) { err = e.toStdString(); return; }
        desc = (m->info.name.isNotEmpty() ? m->info.name : juce::File (juce::String (path)).getFileNameWithoutExtension()).toStdString();
        if (m->info.gear.isNotEmpty()) desc += " (" + m->info.gear.toStdString() + ")";
        desc += fmt (", %s, %d parameters, %.1f kHz", m->info.architecture.toRawUTF8(), m->info.parameters, m->info.sampleRate / 1000.0);
        player.reset (new NamLite::Player (std::move (m), sr, BLOCK)); ok = true;
    }
    std::vector<float> run (const std::vector<float>& in) override
    {
        player->reset();
        std::vector<float> y (in);
        for (size_t pos = 0; pos < y.size(); pos += BLOCK) player->process (y.data() + pos, (int) std::min<size_t> (BLOCK, y.size() - pos));
        return y;
    }
    std::string describe() const override { return desc; }
};

// the six standard levels (library candidates) or any list of levels (the capture's fine grid)
std::vector<Pt> measureSynthetic (Renderer& r, const std::vector<double>& levels, double sr)
{
    const auto pluck = pluckUnit (sr);
    std::vector<Pt> o;
    for (double L : levels) { Marks m; auto x = makeStream (L, sr, pluck, m); o.push_back (analyzeStream (r.run (x), m, sr)); }
    return o;
}

//==============================================================================
// Analysis of an arbitrary DI performance and the matching output (used when the DI is the test signal)
//==============================================================================
struct WinInfo { size_t a; double inDb; double f0; };
constexpr double WIN_SEC = 0.1, HOP_SEC = 0.05, BIN_HALF = 4.0;

// normalised autocorrelation pitch estimate on the DI window (decimated by 4); 0 when the window is not clearly one pitch
double estimateF0 (const float* x, size_t n, double sr)
{
    const size_t dn = n / 4; if (dn < 64) return 0;
    std::vector<double> d (dn);
    double mean = 0;
    for (size_t i = 0; i < dn; ++i) { d[i] = 0.25 * (x[4 * i] + x[4 * i + 1] + x[4 * i + 2] + x[4 * i + 3]); mean += d[i]; }
    mean /= (double) dn; for (auto& v : d) v -= mean;
    const double dsr = sr / 4.0;
    const int lmin = std::max (2, (int) (dsr / 700.0)), lmax = std::min ((int) (dn / 2), (int) (dsr / 70.0));
    if (lmax <= lmin + 2) return 0;
    std::vector<double> r ((size_t) lmax + 2, 0.0);
    double e0 = 0; for (size_t i = 0; i < dn; ++i) e0 += d[i] * d[i];
    if (e0 < 1e-14) return 0;
    for (int l = lmin - 1; l <= lmax + 1; ++l)
    {
        double s = 0, e1 = 0; const size_t m = dn - (size_t) l;
        for (size_t i = 0; i < m; ++i) { s += d[i] * d[i + (size_t) l]; e1 += d[i + (size_t) l] * d[i + (size_t) l]; }
        double e2 = 0; for (size_t i = 0; i < m; ++i) e2 += d[i] * d[i];
        r[(size_t) l] = s / std::sqrt (std::max (e1 * e2, 1e-30));
    }
    double best = -1; for (int l = lmin; l <= lmax; ++l) best = std::max (best, r[(size_t) l]);
    if (best < 0.88) return 0;
    for (int l = lmin; l <= lmax; ++l)                        // the first strong local maximum (avoids picking a lower octave)
        if (r[(size_t) l] >= 0.93 * best && r[(size_t) l] >= r[(size_t) l - 1] && r[(size_t) l] >= r[(size_t) l + 1])
        {
            const double a = r[(size_t) l - 1], b = r[(size_t) l], c = r[(size_t) l + 1], den = a - 2 * b + c;
            const double off = std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0;
            return dsr / ((double) l + off);
        }
    return 0;
}
std::vector<WinInfo> planWindows (const std::vector<float>& in, size_t a0, size_t len, double sr)
{
    std::vector<WinInfo> w;
    const size_t N = (size_t) (WIN_SEC * sr), H = (size_t) (HOP_SEC * sr);
    for (size_t a = a0; a + N <= std::min (in.size(), a0 + len); a += H)
    {
        double mean = 0; for (size_t i = 0; i < N; ++i) mean += in[a + i]; mean /= (double) N;
        double e = 0; for (size_t i = 0; i < N; ++i) { const double v = in[a + i] - mean; e += v * v; }
        const double d = db10 (e / (double) N);
        if (d < LEV[0] - BIN_HALF || d >= LEV[NL - 1] + BIN_HALF) continue;
        w.push_back ({ a, d, estimateF0 (&in[a], N, sr) });
    }
    return w;
}
std::vector<Pt> analyzeGeneric (const std::vector<float>& out, const std::vector<WinInfo>& wins, double sr)
{
    const size_t N = (size_t) (WIN_SEC * sr);
    int ordB = 12; while ((1u << ordB) > N && ordB > 8) --ordB;
    const int ordH = 14;                                                         // zero padded transform for the harmonic search
    static thread_local std::unique_ptr<juce::dsp::FFT> fftH; if (! fftH) fftH.reset (new juce::dsp::FFT (ordH));
    double sLev[NL] = { 0 }, sH2[NL] = { 0 }, sH3[NL] = { 0 }, sF[NL] = { 0 }, bandE[NL][NB] = { { 0 } };
    int nW[NL] = { 0 }, nH[NL] = { 0 };
    std::vector<double> pw; std::vector<float> d ((size_t) 2 << ordH);
    for (auto& w : wins)
    {
        int k = -1; for (int j = 0; j < NL; ++j) if (std::abs (w.inDb - LEV[j]) < BIN_HALF) k = j;
        if (k < 0 || w.a + N > out.size()) continue;
        double mean = 0; for (size_t i = 0; i < N; ++i) mean += out[w.a + i]; mean /= (double) N;
        double e = 0; for (size_t i = 0; i < N; ++i) { const double v = out[w.a + i] - mean; e += v * v; }
        sLev[k] += std::max (-120.0, db10 (e / (double) N)); ++nW[k];
        powerSpectrum (&out[w.a], ordB, pw, mean);
        double be[NB]; bandEnergies (pw, ordB, sr, be); for (int b = 0; b < NB; ++b) bandE[k][b] += be[b];
        if (w.f0 > 0 && w.f0 * 3.2 < sr / 2.0)
        {
            std::fill (d.begin(), d.end(), 0.0f); double ws = 0;
            for (size_t i = 0; i < N; ++i) { const double h = 0.5 - 0.5 * std::cos (2.0 * kPi * (double) i / (double) N); d[i] = (float) ((out[w.a + i] - mean) * h); ws += h; }
            fftH->performRealOnlyForwardTransform (d.data(), true);
            auto peak = [&] (double f)
            {
                const double binHz = sr / (double) (1 << ordH); double best = 0;
                for (int q = (int) (f * 0.97 / binHz); q <= (int) (f * 1.03 / binHz) + 1; ++q) best = std::max (best, std::hypot ((double) d[(size_t) 2 * q], (double) d[(size_t) 2 * q + 1]));
                return 2.0 * best / ws;
            };
            const double a1 = peak (w.f0), a2 = peak (2 * w.f0), a3 = peak (3 * w.f0);
            if (a1 > 1e-6) { sH2[k] += std::max (FLOOR_DB, db20 (a2 / a1)); sH3[k] += std::max (FLOOR_DB, db20 (a3 / a1)); sF[k] += db20 (a1); ++nH[k]; }
        }
    }
    std::vector<Pt> o (NL);
    for (int k = 0; k < NL; ++k)
    {
        Pt& p = o[(size_t) k];
        if (nW[k] >= 4) { p.hasLev = true; p.hasBand = true; p.lev = sLev[k] / nW[k]; for (int b = 0; b < NB; ++b) p.band[b] = db10 (bandE[k][b]); }
        if (nH[k] >= 3) { p.hasH = true; p.h2 = sH2[k] / nH[k]; p.h3 = sH3[k] / nH[k]; p.fund = sF[k] / nH[k]; }
    }
    return o;
}
// delay of the recording against the DI in samples (positive = recording is later), from smoothed envelopes
long estimateLag (const std::vector<float>& in, const std::vector<float>& out, double sr)
{
    const size_t step = std::max<size_t> (1, (size_t) (0.001 * sr)), n = std::min (std::min (in.size(), out.size()), (size_t) (30 * sr)) / step;
    if (n < 200) return 0;
    std::vector<double> ei (n), eo (n);
    for (size_t i = 0; i < n; ++i) { double a = 0, b = 0; for (size_t j = 0; j < step; ++j) { a += std::abs (in[i * step + j]); b += std::abs (out[i * step + j]); } ei[i] = a; eo[i] = b; }
    auto norm = [] (std::vector<double>& v) { double m = 0; for (double x : v) m += x; m /= (double) v.size(); double s = 0; for (auto& x : v) { x -= m; s += x * x; } s = std::sqrt (s + 1e-30); for (auto& x : v) x /= s; };
    norm (ei); norm (eo);
    const long maxLag = 300; long bestL = 0; double best = -1e9;
    for (long l = -maxLag; l <= maxLag; ++l)
    {
        double s = 0; for (long i = std::max (0L, -l); i < (long) n - std::max (0L, l); ++i) s += ei[(size_t) i] * eo[(size_t) (i + l)];
        if (s > best) { best = s; bestL = l; }
    }
    return bestL * (long) step;
}

//==============================================================================
// The library: measurements of every candidate, kept in a text file so a new capture only needs its own measurement
//==============================================================================
using Key = std::tuple<int, int, int, int>;      // model, voicing, mode (-1 = plug-in default), gain in thousandths
using Row = std::vector<Pt>;
struct Library
{
    std::string sig;
    std::map<Key, Row> rows;
    double cpu = 0;
};
void writeRow (std::ostream& o, const Key& k, const Row& r)
{
    o << "R " << std::get<0> (k) << ' ' << std::get<1> (k) << ' ' << std::get<2> (k) << ' ' << std::get<3> (k) << ' ' << r.size();
    char buf[64];
    for (auto& p : r)
    {
        o << ' ' << (p.hasLev ? 1 : 0) << (p.hasH ? 1 : 0) << (p.hasBand ? 1 : 0);
        std::snprintf (buf, sizeof buf, " %.4f %.4f %.4f %.4f", p.lev, p.fund, p.h2, p.h3); o << buf;
        for (int b = 0; b < NB; ++b) { std::snprintf (buf, sizeof buf, " %.3f", p.band[b]); o << buf; }
    }
    o << '\n';
}
bool readLibrary (const std::string& path, Library& lib)
{
    std::ifstream f (path); if (! f) return false;
    std::string line;
    while (std::getline (f, line))
    {
        if (line.rfind ("#sig ", 0) == 0) lib.sig = line.substr (5);
        else if (line.rfind ("#cpu ", 0) == 0) lib.cpu += std::atof (line.c_str() + 5);
        else if (line.rfind ("R ", 0) == 0)
        {
            std::istringstream s (line.substr (2)); int m, v, md, g, n; s >> m >> v >> md >> g >> n; Row r;
            for (int i = 0; i < n && s; ++i)
            {
                Pt p; std::string fl; s >> fl >> p.lev >> p.fund >> p.h2 >> p.h3; for (int b = 0; b < NB; ++b) s >> p.band[b];
                p.hasLev = fl.size() > 0 && fl[0] == '1'; p.hasH = fl.size() > 1 && fl[1] == '1'; p.hasBand = fl.size() > 2 && fl[2] == '1'; r.push_back (p);
            }
            if ((int) r.size() == n) lib.rows[Key (m, v, md, g)] = r;
        }
    }
    return true;
}
bool writeLibrary (const std::string& path, const Library& lib, double extraCpu = 0)
{
    std::ofstream f (path); if (! f) return false;
    f << "#fitcapture-library 1\n#sig " << lib.sig << "\n#cpu " << (lib.cpu + extraCpu) << '\n';
    for (auto& kv : lib.rows) writeRow (f, kv.first, kv.second);
    return (bool) f;
}

//==============================================================================
// Scoring
//==============================================================================
struct Err { double G = 0, H2 = 0, H3 = 0, B = 0, E = 1e9, offset = 0; bool ok = false, hasH = false; double dH2 = 0, dH3 = 0; double bandDev[NB] = { 0 }; int bandIdx = MID; };

Err score (const Pt* const* cap, const Pt* const* cand)     // cap[k], cand[k] for the six standard levels (either may be null)
{
    Err e; std::vector<double> d, d2, d3;
    for (int k = 0; k < NL; ++k)
    {
        if (! cap[k] || ! cand[k]) continue;
        if (cap[k]->hasLev && cand[k]->hasLev) d.push_back (cap[k]->lev - cand[k]->lev);
        if (cap[k]->hasH && cand[k]->hasH) { d2.push_back (cap[k]->h2 - cand[k]->h2); d3.push_back (cap[k]->h3 - cand[k]->h3); }
    }
    if (d.size() < 3) return e;
    double mean = 0; for (double v : d) mean += v; mean /= (double) d.size();
    double s = 0; for (double v : d) s += (v - mean) * (v - mean);
    e.G = std::sqrt (s / (double) d.size()); e.offset = mean;
    double wsum = W_GAIN, tot = W_GAIN * e.G;
    if (d2.size() >= 2)
    {
        double a = 0, b = 0, m2 = 0, m3 = 0; for (size_t i = 0; i < d2.size(); ++i) { a += d2[i] * d2[i]; b += d3[i] * d3[i]; m2 += d2[i]; m3 += d3[i]; }
        e.H2 = std::sqrt (a / (double) d2.size()); e.H3 = std::sqrt (b / (double) d2.size()); e.dH2 = m2 / (double) d2.size(); e.dH3 = m3 / (double) d2.size(); e.hasH = true;
        tot += W_H2 * e.H2 + W_H3 * e.H3; wsum += W_H2 + W_H3;
    }
    static const int order[NL] = { MID, 2, 4, 1, 5, 0 };
    for (int q = 0; q < NL; ++q)
    {
        const int k = order[q];
        if (cap[k] && cand[k] && cap[k]->hasBand && cand[k]->hasBand)
        {
            double dd[NB], m = 0; for (int b = 0; b < NB; ++b) { dd[b] = cap[k]->band[b] - cand[k]->band[b]; m += dd[b]; } m /= NB;
            double ss = 0; for (int b = 0; b < NB; ++b) { e.bandDev[b] = dd[b] - m; ss += e.bandDev[b] * e.bandDev[b]; }
            e.B = std::sqrt (ss / NB); e.bandIdx = k; tot += W_BAND * e.B; wsum += W_BAND; break;
        }
    }
    e.E = tot / wsum; e.ok = true; return e;
}

//==============================================================================
// Candidates
//==============================================================================
struct Cand { int model; int voicing; int mode; std::string name; };
struct Names { std::vector<std::string> models, voicings; int acousticIdx = -1; int nModes = 0; };

Names readNames()
{
    Names n; StarterAmpProcessor p;
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("ampModel")))
        for (int i = 0; i < c->choices.size(); ++i) { n.models.push_back (c->choices[i].toStdString()); if (c->choices[i].containsIgnoreCase ("acoustic")) n.acousticIdx = i; }
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("ampVoicing")))
        for (int i = 0; i < c->choices.size(); ++i) n.voicings.push_back (c->choices[i].toStdString());
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("ampMode"))) n.nModes = c->choices.size();
    return n;
}
std::string shortName (const std::string& full) { const auto i = full.find (" ("); return i == std::string::npos ? full : full.substr (0, i); }

//==============================================================================
// Command line
//==============================================================================
struct Opts
{
    std::string nam, di, outWav, label, knobs, only, libPath, csv, renderSpec, writePath, modes, captureAmp, saveCap, loadCap;
    bool synthetic = false, noDiCheck = false, noLib = false, list = false, noOffset = false, quiet = false, shardScreen = false, buildLib = false;
    int refine = 8, jobs = 1, top = 10, shardK = -1, shardN = 0, quality = -1; double diStart = 0, diSec = 30, screenSec = 8;
    std::vector<std::string> raw;
};

void usage()
{
    std::printf (
"fitcapture - rank the plug-in's electric amps by how close they get to an amp capture\n\n"
"Capture (one of):\n"
"  --nam FILE            a .nam model; measured with the built-in test signals (sine + plucked notes, six levels)\n"
"  --di FILE --out FILE  a dry DI and the capture's recorded output (same length, aligned; WAV, any rate, mono or stereo)\n"
"  --nam FILE --di FILE  as --nam, then the best matches are also checked on your own DI (add --synthetic to skip the check)\n"
"Options:\n"
"  --label TEXT          name for the report;   --knobs TEXT   the capture's knob positions (only echoed)\n"
"  --only LIST           restrict candidates: comma list of model numbers or name parts, each optionally ':voicing' (1 to 4),\n"
"                        e.g. --only 13,24:2,modern\n"
"  --lib FILE            measurement library (default fitcapture_lib.tsv for built-in signals; none for a DI). Built on first use,\n"
"                        reused afterwards; rebuilt if it was made with other signals or the amps have changed.   --no-lib to disable\n"
"  --jobs N              measure candidates in N processes (default 1)\n"
"  --modes LIST          amp modes to try, e.g. 0,1,2 (0 Classic, 1 Circuit-derived, 2 Hybrid); default: the plug-in's default\n"
"  --quality N           amp oversampling setting for the candidates (plug-in default if omitted)\n"
"  --refine N            re-measure the best N candidates (and N more by the offset search) at Gain settings between the library's five (default 8; 0 = off)\n"
"  --top N               rows in the ranked table (default 10);   --csv FILE   write every candidate and Gain setting\n"
"  --di-start S --di-sec S   part of the DI to use (default from 0 s, 30 s);  --screen-sec S  length of the first-pass excerpt (8 s)\n"
"  --no-offset           skip the level-offset search\n"
"  --list                list the amps and voicings that would be tried\n"
"  --build-lib           measure the candidates into the library and stop (no capture needed)\n"
"  --capture-amp M:V:G   self test: use one of our own amps (model, voicing 1 to 4, Gain) as the capture, measured with the built-in signals\n"
"Helpers for self tests:\n"
"  --render amp:MODEL:VOICING:GAIN | nam:FILE  --di FILE  --write FILE     render a DI through one of our amps or a model\n");
}

bool parseArgs (int argc, char** argv, Opts& o)
{
    for (int i = 0; i < argc; ++i) o.raw.push_back (argv[i]);
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        auto next = [&] () -> std::string { if (i + 1 >= argc) { std::fprintf (stderr, "missing value after %s\n", a.c_str()); std::exit (1); } return argv[++i]; };
        if (a == "--nam") o.nam = next(); else if (a == "--di") o.di = next(); else if (a == "--out") o.outWav = next();
        else if (a == "--label") o.label = next(); else if (a == "--knobs") o.knobs = next(); else if (a == "--only") o.only = next();
        else if (a == "--lib") o.libPath = next(); else if (a == "--csv") o.csv = next(); else if (a == "--jobs") o.jobs = std::max (1, std::atoi (next().c_str()));
        else if (a == "--refine") o.refine = std::atoi (next().c_str()); else if (a == "--top") o.top = std::max (1, std::atoi (next().c_str())); else if (a == "--modes") o.modes = next();
        else if (a == "--quality") o.quality = std::atoi (next().c_str());
        else if (a == "--share") { int m = -1; float x = 0.5f; const std::string v = next(); if (std::sscanf (v.c_str(), "%d=%f", &m, &x) == 2 && m >= 0 && m < 64) StageShaper::hybridShares().v[m] = x; }   // b121: try a Hybrid share, e.g. --share 21=0.5 (repeat the option for more models)
        else if (a == "--save-cap") o.saveCap = next(); else if (a == "--load-cap") o.loadCap = next();      // b65: keep / reuse the capture's own measurement (it takes about 90 s) while a candidate is being tuned
        else if (a == "--di-start") o.diStart = std::atof (next().c_str()); else if (a == "--di-sec") o.diSec = std::atof (next().c_str());
        else if (a == "--screen-sec") o.screenSec = std::atof (next().c_str());
        else if (a == "--synthetic") o.synthetic = true; else if (a == "--no-di-check") o.noDiCheck = true; else if (a == "--no-lib") o.noLib = true;
        else if (a == "--shard-screen") o.shardScreen = true; else if (a == "--build-lib") o.buildLib = true; else if (a == "--capture-amp") o.captureAmp = next(); else if (a == "--no-offset") o.noOffset = true; else if (a == "--list") o.list = true; else if (a == "--quiet") o.quiet = true;
        else if (a == "--render") o.renderSpec = next(); else if (a == "--write") o.writePath = next();
        else if (a == "--shard") { auto v = split (next(), '/'); if (v.size() != 2) return false; o.shardK = std::atoi (v[0].c_str()); o.shardN = std::atoi (v[1].c_str()); }
        else if (a == "--help" || a == "-h") { usage(); std::exit (0); }
        else { std::fprintf (stderr, "unknown option %s (try --help)\n", a.c_str()); return false; }
    }
    return true;
}

std::vector<Cand> buildCandidates (const Opts& o, const Names& names)
{
    std::vector<int> modes;
    if (o.modes.empty()) modes.push_back (-1); else for (auto& s : split (o.modes, ',')) if (! s.empty()) modes.push_back (std::atoi (s.c_str()));
    struct Sel { int model; int voicing; }; std::vector<Sel> sel;
    for (int m = 0; m < (int) names.models.size(); ++m) { if (m == names.acousticIdx) continue; for (int v = 0; v < (int) names.voicings.size() && v <= AmpExtras::lastVoicing (m); ++v) sel.push_back ({ m, v }); }
    if (! o.only.empty())
    {
        std::vector<Sel> keep;
        for (auto& tok : split (o.only, ','))
        {
            if (tok.empty()) continue;
            auto parts = split (tok, ':'); const std::string key = lower (parts[0]); int vo = parts.size() > 1 ? std::atoi (parts[1].c_str()) - 1 : -1;
            bool isNum = ! key.empty() && std::all_of (key.begin(), key.end(), [] (char c) { return std::isdigit ((unsigned char) c) != 0; });
            for (auto& s : sel)
            {
                const bool hit = isNum ? s.model == std::atoi (key.c_str()) : lower (names.models[(size_t) s.model]).find (key) != std::string::npos;
                if (hit && (vo < 0 || vo == s.voicing)) keep.push_back (s);
            }
        }
        std::set<std::pair<int, int>> seen; sel.clear();
        for (auto& s : keep) if (seen.insert ({ s.model, s.voicing }).second) sel.push_back (s);
    }
    std::vector<Cand> c;
    for (int md : modes) for (auto& s : sel) c.push_back ({ s.model, s.voicing, md, shortName (names.models[(size_t) s.model]) });
    return c;
}

//==============================================================================
// Measurement of one library row
//==============================================================================
struct Ctx
{
    Opts o; Names names; std::vector<Cand> cands;
    double sr = 48000.0; bool generic = false;
    std::vector<float> di; size_t diA = 0, diN = 0; std::vector<WinInfo> wins;     // generic mode: the DI excerpt in use
        std::string sig;
};

Row measureRow (const Ctx& c, const Cand& cd, double gain)
{
    AmpCfg cfg; cfg.model = cd.model; cfg.voicing = cd.voicing; cfg.mode = cd.mode; cfg.gain = gain; cfg.quality = c.o.quality;
    if (! c.generic)
    {
        AmpRenderer r (cfg, c.sr); std::vector<double> lv (LEV, LEV + NL);
        return measureSynthetic (r, lv, c.sr);
    }
    std::vector<float> x (c.di.begin() + (long) c.diA, c.di.begin() + (long) (c.diA + c.diN));
    int lat = 0; auto y = renderAmp (cfg, x, c.sr, &lat);
    if (lat > 0 && (size_t) lat < y.size()) { y.erase (y.begin(), y.begin() + lat); y.resize (x.size(), 0.0f); }
    // window positions are relative to the DI: shift them into the excerpt
    std::vector<WinInfo> w = c.wins; for (auto& q : w) q.a -= c.diA;
    return analyzeGeneric (y, w, c.sr);
}
Key keyOf (const Cand& cd, double gain) { return Key (cd.model, cd.voicing, cd.mode, (int) std::lround (gain * 1000)); }

// measure the listed keys (a slice of them when sharded); returns CPU seconds used
double measureSet (const Ctx& c, const std::vector<std::pair<Cand, double>>& todo, Library& lib, const char* tag, bool progress)
{
    const double c0 = cpuSeconds(), w0 = wallSeconds(); size_t n = 0;
    for (auto& t : todo)
    {
        lib.rows[keyOf (t.first, t.second)] = measureRow (c, t.first, t.second);
        ++n;
        if (progress && (n % 10 == 0 || n == todo.size()))
        {
            const double el = wallSeconds() - w0;
            std::fprintf (stderr, "\r  %s %zu/%zu  %.0f s elapsed, about %.0f s to go   ", tag, n, todo.size(), el, el / (double) n * (double) (todo.size() - n));
            std::fflush (stderr);
        }
    }
    if (progress && ! todo.empty()) std::fprintf (stderr, "\n");
    return cpuSeconds() - c0;
}

std::string quoteArg (const std::string& s) { std::string o = "\""; for (char ch : s) { if (ch == '"') o += "\\\""; else o += ch; } return o + "\""; }

//==============================================================================
// Reporting
//==============================================================================
struct Ranked
{
    Cand cand; double gain = 0; Err e;                    // best Gain setting at no level offset
    Err eFree; double gainFree = 0; int shift = 0;        // best over Gain settings and level offsets
    std::vector<Err> perGain;                             // for the csv
    Row row;                                              // the measurement at the best Gain
    std::vector<double> gains;                            // the Gain settings behind perGain
};

const char* bandName (int b) { static const char* n[NB] = { "63 Hz", "125 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "4 kHz", "8 kHz" }; return n[b]; }

bool slopeOk (const std::vector<Pt>& p) { return p.size() == (size_t) NL && p[0].hasLev && p[2].hasLev && p[3].hasLev && p[5].hasLev; }
std::string slopeText (const std::vector<Pt>& p)       // dB of output per dB of input over the low and the high half of the level range
{
    if (! slopeOk (p)) return "n/a (levels missing)";
    double lo = (p[2].lev - p[0].lev) / (LEV[2] - LEV[0]), hi = (p[5].lev - p[3].lev) / (LEV[5] - LEV[3]);
    return fmt ("%.2f low, %.2f high", lo, hi);
}

} // namespace

//==============================================================================
int main (int argc, char** argv)
{
    Opts o; if (! parseArgs (argc, argv, o)) return 1;
    if (argc < 2) { usage(); return 0; }
    juce::ScopedJuceInitialiser_GUI juceInit;
    const double tStart = wallSeconds();

    //--- helper: render a DI through one of our amps or a NAM model and save it ----------------------------------------
    if (! o.renderSpec.empty())
    {
        Wav di; std::string err; if (o.di.empty() || o.writePath.empty() || ! readWav (o.di, di, err)) { std::fprintf (stderr, "--render needs --di FILE and --write FILE %s\n", err.c_str()); return 1; }
        std::vector<float> y;
        if (o.renderSpec.rfind ("amp:", 0) == 0)
        {
            auto v = split (o.renderSpec.substr (4), ':'); if (v.size() != 3) { std::fprintf (stderr, "--render amp:MODEL:VOICING:GAIN\n"); return 1; }
            AmpCfg c; c.model = std::atoi (v[0].c_str()); c.voicing = std::atoi (v[1].c_str()) - 1; c.gain = std::atof (v[2].c_str()); c.quality = o.quality;
            if (! o.modes.empty()) c.mode = std::atoi (o.modes.c_str());
            y = renderAmp (c, di.x, di.sr);
        }
        else if (o.renderSpec.rfind ("nam:", 0) == 0)
        {
            NamRenderer r (o.renderSpec.substr (4), di.sr); if (! r.ok) { std::fprintf (stderr, "cannot load model: %s\n", r.err.c_str()); return 1; }
            y = r.run (di.x);
        }
        else { std::fprintf (stderr, "--render needs amp:... or nam:...\n"); return 1; }
        if (! writeWavFloat (o.writePath, y, di.sr)) { std::fprintf (stderr, "cannot write %s\n", o.writePath.c_str()); return 1; }
        std::printf ("wrote %s (%zu samples, %d Hz)\n", o.writePath.c_str(), y.size(), di.sr);
        return 0;
    }

    Ctx c; c.o = o; c.names = readNames(); c.cands = buildCandidates (o, c.names);
    if (o.list)
    {
        std::printf ("%zu candidate amp/voicing/mode combinations x %d Gain settings (acoustic skipped)\n", c.cands.size(), 5);
        for (int m = 0; m < (int) c.names.models.size(); ++m) std::printf ("  %2d  %s%s\n", m, c.names.models[(size_t) m].c_str(), m == c.names.acousticIdx ? "   [skipped]" : "");
        std::printf ("voicings 1 to %zu; amp modes: %d (0 Classic, 1 Circuit-derived, 2 Hybrid)\n", c.names.voicings.size(), c.names.nModes);
        return 0;
    }
    if (c.cands.empty()) { std::fprintf (stderr, "no candidates match --only\n"); return 1; }

    //--- decide the mode and load the capture's side ----------------------------------------------------------------------
    const bool haveNam = ! o.nam.empty(), haveDi = ! o.di.empty(), haveOut = ! o.outWav.empty(), haveAmp = ! o.captureAmp.empty();
    if (! haveNam && ! haveAmp && ! o.buildLib && ! (haveDi && haveOut)) { std::fprintf (stderr, "give --nam FILE, or --di FILE together with --out FILE (see --help)\n"); return 1; }
    c.generic = ! haveNam && ! haveAmp && ! o.buildLib;
    std::unique_ptr<Renderer> ampCapture; Renderer* capR = nullptr;
    Wav diW, outW; std::string err;
    std::unique_ptr<NamRenderer> nam;
    if (haveNam)
    {
        nam.reset (new NamRenderer (o.nam, 48000.0));
        if (! nam->ok) { std::fprintf (stderr, "cannot load %s: %s\n", o.nam.c_str(), err.empty() ? nam->err.c_str() : err.c_str()); return 1; }
    }
    if (haveAmp)
    {
        auto v = split (o.captureAmp, ':'); if (v.size() != 3) { std::fprintf (stderr, "--capture-amp MODEL:VOICING:GAIN\n"); return 1; }
        AmpCfg ac; ac.model = std::atoi (v[0].c_str()); ac.voicing = std::atoi (v[1].c_str()) - 1; ac.gain = std::atof (v[2].c_str()); ac.quality = o.quality;
        if (! o.modes.empty()) ac.mode = std::atoi (split (o.modes, ',')[0].c_str());
        ampCapture.reset (new AmpRenderer (ac, 48000.0)); capR = ampCapture.get();
    }
    else if (haveNam) capR = nam.get();
    if (haveDi)
    {
        if (! readWav (o.di, diW, err)) { std::fprintf (stderr, "%s\n", err.c_str()); return 1; }
    }
    if (haveOut)
    {
        if (! readWav (o.outWav, outW, err)) { std::fprintf (stderr, "%s\n", err.c_str()); return 1; }
        if (diW.sr != outW.sr) { std::fprintf (stderr, "the DI (%d Hz) and the recording (%d Hz) have different sample rates\n", diW.sr, outW.sr); return 1; }
    }
    c.sr = c.generic ? (double) diW.sr : 48000.0;
    if (haveNam && haveDi && ! o.synthetic && diW.sr != 48000) { std::printf ("note: the DI is %d Hz; the DI cross-check runs at that rate\n", diW.sr); }

    // generic mode: pick the excerpt, plan the windows
    auto prepareGeneric = [&] (Ctx& g, const std::vector<float>& diX, double sr, bool screening)
    {
        g.sr = sr; g.di = diX;
        size_t a0 = (size_t) (o.diStart * sr), len = std::min (diX.size() - std::min (diX.size(), a0), (size_t) (o.diSec * sr));
        if (screening)
        {
            const size_t sl = (size_t) (o.screenSec * sr);
            if (len > sl * 3 / 2)
            {
                auto all = planWindows (diX, a0, len, sr); size_t bestA = a0; double bestScore = -1;
                for (size_t s = a0; s + sl <= a0 + len; s += (size_t) (2 * sr))
                {
                    int cnt[NL] = { 0 }, pitched = 0;
                    for (auto& w : all) if (w.a >= s && w.a + (size_t) (WIN_SEC * sr) <= s + sl) { for (int k = 0; k < NL; ++k) if (std::abs (w.inDb - LEV[k]) < BIN_HALF) { ++cnt[k]; if (w.f0 > 0) ++pitched; } }
                    int bins = 0; for (int k = 0; k < NL; ++k) bins += cnt[k] >= 6;
                    const double sc = bins * 1000.0 + std::min (pitched, 500);
                    if (sc > bestScore) { bestScore = sc; bestA = s; }
                }
                a0 = bestA; len = sl;
            }
        }
        g.diA = a0; g.diN = len; g.wins = planWindows (diX, a0, len, sr);
    };

    std::vector<float> capOutFull;       // generic mode: the capture's output over the whole DI
    if (c.generic)
    {
        capOutFull = outW.x;
        const size_t n = std::min (diW.x.size(), capOutFull.size());
        if (std::abs ((long) diW.x.size() - (long) capOutFull.size()) > (long) (0.01 * diW.sr)) std::printf ("note: the DI and the recording differ in length (%zu and %zu samples); the shorter one is used\n", diW.x.size(), capOutFull.size());
        capOutFull.resize (n); diW.x.resize (n);
        const long lag = estimateLag (diW.x, capOutFull, (double) diW.sr);
        if (std::abs (lag) > (long) (0.002 * diW.sr))
        {
            std::printf ("note: the recording is %.1f ms %s the DI; shifted to line up\n", std::abs (lag) * 1000.0 / diW.sr, lag > 0 ? "after" : "before");
            if (lag > 0) { capOutFull.erase (capOutFull.begin(), capOutFull.begin() + lag); capOutFull.resize (n, 0.0f); }
            else { capOutFull.insert (capOutFull.begin(), (size_t) -lag, 0.0f); capOutFull.resize (n); }
        }
    }

    //--- signature of what the library holds ------------------------------------------------------------------------------
    std::string libPath = o.libPath;
    const bool useLib = ! o.noLib && (! o.libPath.empty() || ! c.generic);
    if (libPath.empty() && ! c.generic) libPath = "fitcapture_lib.tsv";
    const std::string qual = o.quality >= 0 ? std::to_string (o.quality) : std::string ("default");

    Ctx screen = c, full = c;
    if (c.generic)
    {
        prepareGeneric (screen, diW.x, (double) diW.sr, true);
        prepareGeneric (full, diW.x, (double) diW.sr, false);
    }
    auto sigFor = [&] (const Ctx& g)
    {
        if (! g.generic) return fmt ("%s sr=%.0f quality=%s", kSignalVersion, g.sr, qual.c_str());
        return fmt ("di hash=%016llx start=%zu n=%zu sr=%.0f quality=%s", (unsigned long long) fnv1a (g.di, g.diA, g.diN), g.diA, g.diN, g.sr, qual.c_str());
    };
    screen.sig = sigFor (screen); full.sig = sigFor (full); c.sig = full.sig;

    auto describeSetup = [&]
    {
        std::printf ("fitcapture: %s\n", o.label.empty() ? (haveNam ? o.nam.c_str() : haveAmp ? o.captureAmp.c_str() : o.outWav.c_str()) : o.label.c_str());
        if (! o.knobs.empty()) std::printf ("  knobs (as given): %s\n", o.knobs.c_str());
        if (haveNam) std::printf ("  capture: %s\n", nam->describe().c_str());
        if (haveAmp) std::printf ("  capture: our own amp %s (self test)\n", o.captureAmp.c_str());
        if (c.generic) std::printf ("  capture: recording %s against DI %s (%d Hz, %.1f s used)\n", o.outWav.c_str(), o.di.c_str(), diW.sr, (double) full.diN / diW.sr);
        std::printf ("  candidates: %zu amp/voicing combinations x 5 Gain settings (0.2 to 0.8), pedals/cab/effects off, auto level off\n", c.cands.size());
    };
    if (o.shardK < 0) describeSetup();

    //--- get the candidate measurements (library) ---------------------------------------------------------------------------
    auto wanted = [&] (const Ctx& g) { std::vector<std::pair<Cand, double>> w; for (auto& cd : g.cands) for (double gn : GAINS) w.push_back ({ cd, gn }); return w; };
    auto ensureLibrary = [&] (const Ctx& g, Library& lib, const std::string& path, bool persist, const char* tag, bool allowShards, bool isScreen)
    {
        lib = Library();
        if (persist && readLibrary (path, lib) && lib.sig != g.sig) { std::printf ("  library %s was made for another signal or setting; rebuilding it\n", path.c_str()); lib = Library(); }
        if (persist && ! lib.rows.empty())   // staleness check: re-measure three stored rows
        {
            std::vector<Key> keys; for (auto& kv : lib.rows) keys.push_back (kv.first);
            std::vector<Key> probes = { keys.front(), keys[keys.size() / 2], keys.back() };
            bool stale = false;
            for (auto& k : probes)
            {
                Cand cd; cd.model = std::get<0> (k); cd.voicing = std::get<1> (k); cd.mode = std::get<2> (k);
                Row fresh = measureRow (g, cd, std::get<3> (k) / 1000.0), &old = lib.rows[k];
                for (size_t i = 0; i < fresh.size() && i < old.size(); ++i)
                {
                    if (fresh[i].hasLev && old[i].hasLev && std::abs (fresh[i].lev - old[i].lev) > 0.25) stale = true;
                    if (fresh[i].hasH && old[i].hasH && (std::abs (fresh[i].h2 - old[i].h2) > 1.5 || std::abs (fresh[i].h3 - old[i].h3) > 1.5)) stale = true;
                }
            }
            if (stale) { std::printf ("  library %s no longer matches the plug-in (amp behaviour changed); rebuilding it\n", path.c_str()); lib = Library(); }
        }
        lib.sig = g.sig;
        std::vector<std::pair<Cand, double>> todo;
        for (auto& w : wanted (g)) if (! lib.rows.count (keyOf (w.first, w.second))) todo.push_back (w);
        if (todo.empty()) { if (! o.quiet) std::printf ("  %s: all %zu measurements found in %s\n", tag, wanted (g).size(), persist ? path.c_str() : "memory"); return; }
        const std::string base = persist ? path : ("fitcapture_tmp_" + std::to_string ((long long) std::time (nullptr)) + ".tsv");
        std::printf ("  %s: measuring %zu of %zu candidate runs (%d process%s)\n", tag, todo.size(), wanted (g).size(), o.jobs, o.jobs > 1 ? "es" : "");
        const double w0 = wallSeconds(); double cpuTotal = 0;
        if (allowShards && o.jobs > 1 && todo.size() >= (size_t) o.jobs * 2)
        {
            writeLibrary (base, lib);                                   // shards read the same file
            std::vector<std::thread> th;
            for (int k = 0; k < o.jobs; ++k)
                th.emplace_back ([&, k]
                {
                    std::string cmd = quoteArg (o.raw[0]);
                    for (size_t i = 1; i < o.raw.size(); ++i)
                    {
                        if (o.raw[i] == "--jobs" || o.raw[i] == "--shard") { ++i; continue; }
                        cmd += " " + quoteArg (o.raw[i]);
                    }
                    cmd += " --lib " + quoteArg (base) + " --shard " + std::to_string (k) + "/" + std::to_string (o.jobs) + " --quiet";
                    if (isScreen) cmd += " --shard-screen";
                    std::system (cmd.c_str());
                });
            for (auto& t : th) t.join();
            for (int k = 0; k < o.jobs; ++k)
            {
                const std::string part = base + ".shard" + std::to_string (k);
                Library pl; if (readLibrary (part, pl)) { for (auto& kv : pl.rows) lib.rows[kv.first] = kv.second; cpuTotal += pl.cpu; }
                std::remove (part.c_str());
            }
            todo.clear(); for (auto& w : wanted (g)) if (! lib.rows.count (keyOf (w.first, w.second))) todo.push_back (w);
            if (! todo.empty()) std::printf ("  %zu runs were not delivered by the worker processes; measuring them here\n", todo.size());
        }
        if (! todo.empty()) cpuTotal += measureSet (g, todo, lib, tag, ! o.quiet);
        if (persist) { if (! writeLibrary (path, lib, cpuTotal)) std::fprintf (stderr, "warning: cannot write %s\n", path.c_str()); }
        else std::remove (base.c_str());
        std::printf ("  %s: done in %.0f s wall, %.0f s CPU\n", tag, wallSeconds() - w0, cpuTotal);
    };

    // worker process: measure my slice and stop
    if (o.shardK >= 0)
    {
        const Ctx& g = o.shardScreen ? screen : full;
        Library lib; readLibrary (libPath, lib);
        std::vector<std::pair<Cand, double>> todo, mine;
        for (auto& w : wanted (g)) if (! lib.rows.count (keyOf (w.first, w.second))) todo.push_back (w);
        for (size_t i = 0; i < todo.size(); ++i) if ((int) (i % (size_t) o.shardN) == o.shardK) mine.push_back (todo[i]);
        Library out; out.sig = g.sig; const double cpu = measureSet (g, mine, out, "shard", false);
        out.cpu = cpu; writeLibrary (libPath + ".shard" + std::to_string (o.shardK), out);
        return 0;
    }

    Library lib;
    const std::string tmpLibPath = useLib ? libPath : std::string();
    if (! c.generic) ensureLibrary (full, lib, libPath, useLib, "library", true, false);
    else ensureLibrary (screen, lib, libPath, useLib, "first pass", true, true);
    if (o.buildLib) { std::printf ("library ready: %zu measurements in %s; total time %.0f s\n", lib.rows.size(), libPath.c_str(), wallSeconds() - tStart); return 0; }

    //--- measure the capture ---------------------------------------------------------------------------------------------------
    std::vector<Pt> capFine, cap6;        // capFine: -62 to +2 dB in 2 dB steps (synthetic); cap6: the six standard levels
    bool haveFine = false;
    if (! c.generic)
    {
        std::vector<double> lv;
        if (o.noOffset) lv.assign (LEV, LEV + NL); else for (double L = FINE_MIN; L <= FINE_MAX + 1e-9; L += FINE_STEP) lv.push_back (L);
        bool loaded = false;
        if (! o.loadCap.empty())
        {
            Library cl; if (readLibrary (o.loadCap, cl) && cl.rows.count (Key (-1, -1, -1, 0))) { capFine = cl.rows[Key (-1, -1, -1, 0)]; loaded = capFine.size() == lv.size(); }
            if (loaded) std::printf ("  capture measurement read from %s\n", o.loadCap.c_str());
        }
        if (! loaded)
        {
            std::printf ("  measuring the capture at %zu input levels...\n", lv.size());
            const double w0 = wallSeconds();
            capFine = measureSynthetic (*capR, lv, 48000.0);
            std::printf ("  capture measured in %.0f s\n", wallSeconds() - w0);
            if (! o.saveCap.empty()) { Library cl; cl.sig = "capture"; cl.rows[Key (-1, -1, -1, 0)] = capFine; writeLibrary (o.saveCap, cl); }
        }
        haveFine = ! o.noOffset;
        for (int k = 0; k < NL; ++k)
        {
            const size_t idx = haveFine ? (size_t) std::lround ((LEV[k] - FINE_MIN) / FINE_STEP) : (size_t) k;
            cap6.push_back (capFine[idx]);
        }
    }
    else cap6 = analyzeGeneric (capOutFull, screen.wins, c.sr);

    // the capture's own six points, on the screening excerpt for generic mode
    if (c.generic)
    {
        int usable = 0; for (auto& p : cap6) usable += p.hasLev;
        std::printf ("  DI levels covered: %d of 6 level bins have enough material", usable);
        int hp = 0; for (auto& p : cap6) hp += p.hasH; std::printf ("; %d bins have steady pitched notes for H2/H3\n", hp);
        if (usable < 3) { std::fprintf (stderr, "the DI does not cover enough input levels (need at least 3 of the 6 bins between -54 and -6 dB rms); use a DI with louder and softer playing\n"); return 1; }
    }

    //--- ranking ---------------------------------------------------------------------------------------------------------------------
    using CKey = std::tuple<int, int, int>;
    using ExtraGains = std::map<CKey, std::vector<double>>;
    auto evalCand = [&] (const Cand& cd, const std::vector<double>& gains, const Library& L, const Library& X, const std::vector<Pt>& capSix, const std::vector<Pt>* fine)
    {
        Ranked r; r.cand = cd; double bestE = 1e18, bestF = 1e18;
        for (double gn : gains)
        {
            const Row* rowp = nullptr;
            auto it = L.rows.find (keyOf (cd, gn)); if (it != L.rows.end()) rowp = &it->second;
            else { auto it2 = X.rows.find (keyOf (cd, gn)); if (it2 != X.rows.end()) rowp = &it2->second; }
            if (! rowp) continue;
            const Row& row = *rowp;
            const Pt* cand[NL]; const Pt* cp[NL]; for (int k = 0; k < NL; ++k) { cand[k] = &row[(size_t) k]; cp[k] = &capSix[(size_t) k]; }
            Err e = score (cp, cand); r.perGain.push_back (e); r.gains.push_back (gn);
            if (e.ok && e.E < bestE) { bestE = e.E; r.e = e; r.gain = gn; r.row = row; }
            if (e.ok && e.E < bestF) { bestF = e.E; r.eFree = e; r.gainFree = gn; r.shift = 0; }
            if (fine)
                for (int d = -12; d <= 12; d += 2)
                {
                    if (d == 0) continue;
                    const Pt* cs[NL]; bool okAll = true;
                    for (int k = 0; k < NL; ++k)
                    {
                        const double lv = LEV[k] - d; const long idx = std::lround ((lv - FINE_MIN) / FINE_STEP);
                        if (idx < 0 || idx >= (long) fine->size()) { okAll = false; break; }
                        cs[k] = &(*fine)[(size_t) idx];
                    }
                    if (! okAll) continue;
                    Err es = score (cs, cand);
                    if (es.ok && es.E < bestF) { bestF = es.E; r.eFree = es; r.gainFree = gn; r.shift = d; }
                }
        }
        return r;
    };
    auto gainsOf = [&] (const Cand& cd, const ExtraGains& xg)
    {
        std::vector<double> g (GAINS, GAINS + 5);
        auto it = xg.find (CKey (cd.model, cd.voicing, cd.mode)); if (it != xg.end()) g.insert (g.end(), it->second.begin(), it->second.end());
        return g;
    };
    auto rank = [&] (const Library& L, const Library& X, const ExtraGains& xg, const std::vector<Pt>& capSix, const std::vector<Pt>* fine, const Ctx& g)
    {
        std::vector<Ranked> out; std::set<CKey> seen;
        for (auto& cd : g.cands)
        {
            if (! seen.insert (CKey (cd.model, cd.voicing, cd.mode)).second) continue;
            Ranked r = evalCand (cd, gainsOf (cd, xg), L, X, capSix, fine);
            if (r.e.ok) out.push_back (r);
        }
        std::sort (out.begin(), out.end(), [] (const Ranked& a, const Ranked& b) { return a.e.E < b.e.E; });
        return out;
    };

    // Gain refinement: the five library Gain settings are coarse, so the best few candidates are measured again at Gain settings in between
    // (these extra runs are kept in memory only, so every candidate in the library stays on the same footing).
    Library extra; ExtraGains extraG; int refineRuns = 0; double refineSecs = 0;
    auto refine = [&] (const Ctx& g, const Library& L, std::vector<Ranked>& ranked, const std::vector<Pt>& capSix, const std::vector<Pt>* fine, bool bigFreeSearch)
    {
        if (o.refine <= 0) return;
        const double t0 = wallSeconds();
        std::vector<size_t> pick;
        {
            std::vector<size_t> idx (ranked.size()); for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
            for (size_t i = 0; i < std::min<size_t> (ranked.size(), (size_t) o.refine); ++i) pick.push_back (i);
            if (bigFreeSearch)
            {
                std::sort (idx.begin(), idx.end(), [&] (size_t a, size_t b) { return ranked[a].eFree.E < ranked[b].eFree.E; });
                for (size_t i = 0; i < std::min<size_t> (idx.size(), (size_t) o.refine); ++i) if (std::find (pick.begin(), pick.end(), idx[i]) == pick.end()) pick.push_back (idx[i]);
            }
        }
        std::printf ("  refining the Gain of %zu candidates between the library settings...", pick.size()); std::fflush (stdout);
        for (size_t pi : pick)
        {
            const Cand cd = ranked[pi].cand; const CKey ck (cd.model, cd.voicing, cd.mode);
            auto measureAt = [&] (double gn)
            {
                gn = std::max (0.05, std::min (0.95, gn));
                const Key k = keyOf (cd, gn);
                if (L.rows.count (k) || extra.rows.count (k)) return;
                extra.rows[k] = measureRow (g, cd, std::lround (gn * 1000) / 1000.0); extraG[ck].push_back (std::lround (gn * 1000) / 1000.0); ++refineRuns;
            };
            // two stages around the best Gain at no offset, and around the best Gain with an offset
            for (int stage = 0; stage < 2; ++stage)
            {
                Ranked cur = evalCand (cd, gainsOf (cd, extraG), L, extra, capSix, fine);
                std::vector<double> centres = { cur.gain }; if (std::abs (cur.gainFree - cur.gain) > 1e-6 && bigFreeSearch) centres.push_back (cur.gainFree);
                for (double c0 : centres)
                {
                    if (stage == 0) for (double dg : { -0.075, -0.0375, 0.0375, 0.075 }) measureAt (c0 + dg);
                    else for (double dg : { -0.01875, 0.01875 }) measureAt (c0 + dg);
                }
            }
            ranked[pi] = evalCand (cd, gainsOf (cd, extraG), L, extra, capSix, fine);
        }
        std::sort (ranked.begin(), ranked.end(), [] (const Ranked& a, const Ranked& b) { return a.e.E < b.e.E; });
        refineSecs += wallSeconds() - t0;
        std::printf (" %d extra runs, %.0f s\n", refineRuns, wallSeconds() - t0);
    };

    std::vector<Ranked> ranked;
    Ctx& rankCtx = c.generic ? screen : full;
    ranked = rank (lib, extra, extraG, cap6, haveFine ? &capFine : nullptr, rankCtx);
    if (ranked.empty()) { std::fprintf (stderr, "no candidate could be scored\n"); return 1; }

    //--- generic mode: second pass on the full excerpt for the best 40 ---------------------------------------------------------
    Library lib2; Ctx narrowed = full;
    if (c.generic)
    {
        const size_t keepN = std::min<size_t> (ranked.size(), 24);
        narrowed.cands.clear();
        for (size_t i = 0; i < keepN; ++i) narrowed.cands.push_back (ranked[i].cand);
        cap6 = analyzeGeneric (capOutFull, full.wins, full.sr);
        if (screen.diA != full.diA || screen.diN != full.diN)
        {
            std::printf ("  second pass: the best %zu on the full %.0f s of the DI\n", keepN, (double) full.diN / full.sr);
            ensureLibrary (narrowed, lib2, libPath.empty() ? std::string() : libPath + ".full", useLib && ! libPath.empty(), "second pass", false, false);
        }
        else lib2 = lib;
        extra = Library(); extraG.clear();
        ranked = rank (lib2, extra, extraG, cap6, nullptr, narrowed);
    }
    const Library& rankLib = c.generic ? lib2 : lib;

    refine (c.generic ? narrowed : full, rankLib, ranked, cap6, haveFine ? &capFine : nullptr, ! c.generic && haveFine);

    //--- the level-offset search for generic mode: re-render the best five with the DI scaled ----------------------------------
    if (c.generic && ! o.noOffset)
    {
        const size_t n5 = std::min<size_t> (ranked.size(), 5);
        for (size_t i = 0; i < n5; ++i)
        {
            Ranked& r = ranked[i]; r.eFree = r.e; r.gainFree = r.gain; r.shift = 0;
            for (int d : { -12, -9, -6, -3, 3, 6, 9, 12 })
            {
                // the windows keep the unscaled DI levels, so the bins stay those of the capture
                AmpCfg cfg; cfg.model = r.cand.model; cfg.voicing = r.cand.voicing; cfg.mode = r.cand.mode; cfg.gain = r.gain; cfg.quality = o.quality;
                std::vector<float> x (full.di.begin() + (long) full.diA, full.di.begin() + (long) (full.diA + full.diN));
                const float sc = (float) std::pow (10.0, d / 20.0); for (auto& v : x) v *= sc;
                auto y = renderAmp (cfg, x, full.sr); std::vector<WinInfo> w = full.wins; for (auto& q : w) q.a -= full.diA;
                auto pts = analyzeGeneric (y, w, full.sr);
                const Pt* cs[NL]; const Pt* cd[NL]; for (int k = 0; k < NL; ++k) { cs[k] = &cap6[(size_t) k]; cd[k] = &pts[(size_t) k]; }
                Err es = score (cs, cd); if (es.ok && es.E < r.eFree.E) { r.eFree = es; r.shift = d; }
            }
        }
    }
    const bool freeAll = ! c.generic && haveFine;

    //--- the report --------------------------------------------------------------------------------------------------------------------
    std::printf ("\nCapture curve (what the amps are compared with):\n");
    std::printf ("  input level (dB rms)      ");
    for (int k = 0; k < NL; ++k) std::printf (" %7.0f", LEV[k]);
    std::printf ("\n  output level (dB rms)     "); for (auto& p : cap6) std::printf (p.hasLev ? " %7.1f" : "       -", p.lev);
    std::printf ("\n  H2 re fundamental (dB)    "); for (auto& p : cap6) std::printf (p.hasH ? " %7.1f" : "       -", p.h2);
    std::printf ("\n  H3 re fundamental (dB)    "); for (auto& p : cap6) std::printf (p.hasH ? " %7.1f" : "       -", p.h3);
    std::printf ("\n  octave bands re 1 kHz (dB, at %.0f dB in):", LEV[ranked[0].e.bandIdx]);
    { const Pt& bp = cap6[(size_t) ranked[0].e.bandIdx]; for (int b = 0; b < NB; ++b) std::printf (" %s %+.1f%s", bandName (b), bp.band[b] - bp.band[4], b + 1 < NB ? "," : ""); }
    std::printf ("\n  gain slope (dB out per dB in): %s\n", slopeText (cap6).c_str());
    if (c.generic) std::printf ("  (levels are the DI's own short-window levels; H2/H3 are re the pitch detected in the DI, which can be an octave off - the same pitch is used for every amp)\n");

    std::printf ("\nRanking: weighted error = (3 x gain + 2 x H2 + 2 x H3 + 1 x band) / 8, all in dB; best of the five Gain settings for each amp and voicing\n");
    std::printf ("  gain = RMS error of the output-level-vs-input-level curve after removing a constant output offset (the capture's loudness is arbitrary)\n");
    std::printf ("  H2/H3 = RMS error of the 2nd/3rd harmonic re fundamental; band = RMS error of the octave-band shape after removing overall level\n\n");
    std::printf ("  %2s  %-26s %-4s %5s %6s | %5s %5s %5s %5s | %7s | %6s %6s\n", "#", "amp", "voic", "Gain", "score", "gain", "H2", "H3", "band", "out trim", "shift", "score*");
    const size_t nShow = std::min<size_t> (ranked.size(), (size_t) o.top);
    for (size_t i = 0; i < nShow; ++i)
    {
        const Ranked& r = ranked[i];
        std::string modeTxt = r.cand.mode >= 0 ? fmt (" m%d", r.cand.mode) : "";
        std::printf ("  %2zu  %-26s V%d%-2s %5.2f %6.2f | %5.2f %5.2f %5.2f %5.2f | %+6.1f  | %+5d  %6.2f\n", i + 1, r.cand.name.c_str(), r.cand.voicing + 1, modeTxt.c_str(), r.gain, r.e.E,
                     r.e.G, r.e.H2, r.e.H3, r.e.B, r.e.offset, r.shift, r.eFree.E);
    }
    std::printf ("  (out trim = dB to add to the amp's output to match the capture's level; shift* = input level offset in dB that gives the best score*, positive = the\n"
                 "   amp needs that much more drive than its Gain setting provides; score* = score at that shift and Gain)%s\n", freeAll ? "" : "  Offsets were searched for the best five only.");

    // best with a free offset, over everything (synthetic mode only; generic mode searched the top five)
    std::vector<Ranked> byFree = ranked;
    std::sort (byFree.begin(), byFree.end(), [] (const Ranked& a, const Ranked& b) { return a.eFree.E < b.eFree.E; });
    if (freeAll)
    {
        std::printf ("\nBest fits if the capture's drive were a free choice (level offset allowed on top of the Gain settings):\n");
        for (size_t i = 0; i < std::min<size_t> (5, byFree.size()); ++i)
        {
            const Ranked& r = byFree[i];
            std::printf ("  %zu  %-26s V%d  Gain %.2f, input offset %+3d dB: score %.2f (gain %.2f, H2 %.2f, H3 %.2f, band %.2f)\n", i + 1, r.cand.name.c_str(), r.cand.voicing + 1, r.gainFree, r.shift, r.eFree.E, r.eFree.G, r.eFree.H2, r.eFree.H3, r.eFree.B);
        }
    }

    // side by side curves for the best candidate
    {
        const Ranked& b = ranked[0];
        const Row& row = b.row;
        std::printf ("\nBest match %s V%d at Gain %.2f, level-matched (output %+.1f dB):\n", b.cand.name.c_str(), b.cand.voicing + 1, b.gain, b.e.offset);
        std::printf ("  input level (dB rms)      "); for (int k = 0; k < NL; ++k) std::printf (" %7.0f", LEV[k]);
        std::printf ("\n  capture output            "); for (auto& p : cap6) std::printf (p.hasLev ? " %7.1f" : "       -", p.lev);
        std::printf ("\n  amp output (+trim)        "); for (auto& p : row) std::printf (p.hasLev ? " %7.1f" : "       -", p.lev + b.e.offset);
        std::printf ("\n  capture H2 / H3           "); for (auto& p : cap6) { if (p.hasH) std::printf (" %3.0f/%3.0f", p.h2, p.h3); else std::printf ("       -"); }
        std::printf ("\n  amp H2 / H3               "); for (auto& p : row) { if (p.hasH) std::printf (" %3.0f/%3.0f", p.h2, p.h3); else std::printf ("       -"); }
        std::printf ("\n  gain slope: capture %s; amp %s\n", slopeText (cap6).c_str(), slopeText (row).c_str());
    }

    //--- DI cross-check (NAM + DI) -----------------------------------------------------------------------------------------------
    if (! c.generic && haveDi && ! o.synthetic && ! o.noDiCheck)
    {
        Ctx g = full; g.generic = true; g.sr = (double) diW.sr; g.o = o; g.di = diW.x;
        const size_t a0 = (size_t) (o.diStart * g.sr), len = std::min (diW.x.size() - std::min (diW.x.size(), a0), (size_t) (o.diSec * g.sr));
        g.diA = a0; g.diN = len; g.wins = planWindows (g.di, a0, len, g.sr);
        NamRenderer dn (o.nam, g.sr);
        std::vector<float> x (g.di.begin() + (long) a0, g.di.begin() + (long) (a0 + len));
        auto y = dn.run (x);
        const long lag = estimateLag (x, y, g.sr);
        if (lag > 0) { y.erase (y.begin(), y.begin() + lag); y.resize (x.size(), 0.0f); }
        for (auto& w : g.wins) w.a -= a0;
        auto capDi = analyzeGeneric (y, g.wins, g.sr);
        std::printf ("\nCross-check on your DI (%.0f s from %.0f s): the top matches rendered on the DI, compared with the model's output on the same notes\n", (double) len / g.sr, o.diStart);
        std::printf ("  %2s  %-26s %-4s %5s | %5s %5s %5s %5s %6s\n", "#", "amp", "voic", "Gain", "gain", "H2", "H3", "band", "score");
        for (size_t i = 0; i < std::min<size_t> (nShow, 10); ++i)
        {
            const Ranked& r = ranked[i];
            AmpCfg cfg; cfg.model = r.cand.model; cfg.voicing = r.cand.voicing; cfg.mode = r.cand.mode; cfg.gain = r.gain; cfg.quality = o.quality;
            int latency = 0; auto z = renderAmp (cfg, x, g.sr, &latency); if (latency > 0) { z.erase (z.begin(), z.begin() + latency); z.resize (x.size(), 0.0f); }
            auto pts = analyzeGeneric (z, g.wins, g.sr);
            const Pt* cs[NL]; const Pt* cd[NL]; for (int k = 0; k < NL; ++k) { cs[k] = &capDi[(size_t) k]; cd[k] = &pts[(size_t) k]; }
            Err e = score (cs, cd);
            if (e.ok) std::printf ("  %2zu  %-26s V%d    %5.2f | %5.2f %5.2f %5.2f %5.2f %6.2f\n", i + 1, r.cand.name.c_str(), r.cand.voicing + 1, r.gain, e.G, e.H2, e.H3, e.B, e.E);
            else std::printf ("  %2zu  %-26s V%d    %5.2f | not enough level bins in the DI\n", i + 1, r.cand.name.c_str(), r.cand.voicing + 1, r.gain);
        }
    }

    //--- verdict ---------------------------------------------------------------------------------------------------------------------
    auto within = [] (const Err& e) { return e.ok && e.G < TOL_GAIN && (! e.hasH || (e.H2 < TOL_H && e.H3 < TOL_H)); };
    const Ranked* close = nullptr; size_t closeRank = 0; int nWithin = 0;
    for (size_t i = 0; i < ranked.size(); ++i) if (within (ranked[i].e)) { ++nWithin; if (! close) { close = &ranked[i]; closeRank = i; } }
    std::printf ("\n");
    const Ranked& best = ranked[0];
    if (close)
    {
        std::printf ("VERDICT: an existing amp is close: %s V%d (Gain about %.2f; gain error %.1f dB, H2 %.1f dB, H3 %.1f dB, band shape %.1f dB).\n", close->cand.name.c_str(), close->cand.voicing + 1, close->gain, close->e.G, close->e.H2, close->e.H3, close->e.B);
        if (closeRank > 0) std::printf ("  (it is rank %zu by the weighted score; ranks 1 to %zu are nearer on the other components)\n", closeRank + 1, closeRank);
        if (nWithin > 1) std::printf ("  %d candidates are within tolerance (gain < %.0f dB and H2/H3 < %.0f dB); the table lists the closest.\n", nWithin, TOL_GAIN, TOL_H);
        if (close->e.B >= TOL_BAND)
        {
            std::printf ("  Tone is the difference: ");
            for (int b = 0; b < NB; ++b) if (std::abs (close->e.bandDev[b]) >= TOL_BAND) std::printf ("%s %+.1f dB  ", bandName (b), close->e.bandDev[b]);
            std::printf ("(capture minus amp); the amp's tone controls can probably cover that.\n");
        }
    }
    else
    {
        std::printf ("VERDICT: no existing amp within tolerance: a new amp or voicing may be justified.\n");
        std::printf ("  Closest: %s V%d at Gain %.2f (gain error %.1f dB, H2 %.1f dB, H3 %.1f dB, band shape %.1f dB). Tolerance: gain < %.0f dB, H2 and H3 < %.0f dB.\n", best.cand.name.c_str(), best.cand.voicing + 1, best.gain,
                    best.e.G, best.e.H2, best.e.H3, best.e.B, TOL_GAIN, TOL_H);
        std::printf ("  What is off in the closest match:\n");
        if (best.e.G >= TOL_GAIN)
        {
            std::string t = fmt ("    gain structure (%.1f dB): the capture's output-vs-input slope is %s; the amp's is %s.", best.e.G, slopeText (cap6).c_str(), slopeText (best.row).c_str());
            if (slopeOk (cap6) && slopeOk (best.row))
            {
                const double sc = (cap6[5].lev - cap6[0].lev) / 40.0, sa = (best.row[5].lev - best.row[0].lev) / 40.0;
                if (sc < sa - 0.08) t += " The capture compresses/saturates more than this amp at this Gain.";
                else if (sc > sa + 0.08) t += " The capture stays cleaner (more headroom) than this amp at this Gain.";
            }
            std::printf ("%s\n", t.c_str());
            if (best.shift != 0 || best.eFree.E < best.e.E - 0.3)
            {
                if (best.shift > 0 && best.gain >= 0.79) std::printf ("    needs much more gain range: even at Gain 0.80 the amp needs about %d dB more drive to behave like the capture.\n", best.shift);
                else if (best.shift < 0 && best.gain <= 0.21) std::printf ("    needs a cleaner/lower-gain range: even at Gain 0.20 the amp needs about %d dB less drive.\n", -best.shift);
                else if (best.shift != 0) std::printf ("    with the input level shifted by %+d dB the fit improves (score %.2f -> %.2f): a gain-range or input-sensitivity difference.\n", best.shift, best.e.E, best.eFree.E);
            }
        }
        if (best.e.hasH && (best.e.H2 >= TOL_H || best.e.H3 >= TOL_H))
        {
            std::printf ("    harmonics (H2 error %.1f dB, H3 error %.1f dB): on average the capture's H2 is %+.1f dB and its H3 %+.1f dB against the amp.", best.e.H2, best.e.H3, best.e.dH2, best.e.dH3);
            if (std::abs (best.e.dH2 - best.e.dH3) >= 3.0) std::printf (" Different even/odd balance: the capture is %s than the amp.", best.e.dH2 > best.e.dH3 ? "more even-order (asymmetric clipping, warmer)" : "more odd-order (symmetric clipping, harder)");
            else if (std::abs (best.e.dH2) >= 2.0 && std::abs (best.e.dH3) >= 2.0 && best.e.dH2 * best.e.dH3 > 0) std::printf (" Both are %s in the capture: it is %s at these levels.", best.e.dH2 > 0 ? "higher" : "lower", best.e.dH2 > 0 ? "more distorted" : "cleaner");
            else std::printf (" The overall amount is similar; the difference is in how the harmonics grow with level (compare the H2 / H3 rows above).");
            std::printf ("\n");
        }
        if (best.e.B >= TOL_BAND)
        {
            std::printf ("    tone (%.1f dB band-shape error): ", best.e.B);
            for (int b = 0; b < NB; ++b) if (std::abs (best.e.bandDev[b]) >= TOL_BAND) std::printf ("%s %+.1f dB  ", bandName (b), best.e.bandDev[b]);
            std::printf ("(capture minus amp). Tone differences matter less than the gain and harmonic components above.\n");
        }
        if (freeAll)
        {
            const Ranked& f = byFree[0];
            std::string extra;
            if (std::abs (f.shift) >= 12) extra = " (the search stops at 12 dB, so the real gap may be larger)";
            if (within (f.eFree))
            {
                std::printf ("  With a free input-level offset %s V%d WOULD be within tolerance (Gain %.2f, offset %+d dB%s): this looks like a gain-range or input-sensitivity gap rather than a character gap.\n", f.cand.name.c_str(), f.cand.voicing + 1, f.gainFree, f.shift, extra.c_str());
                if (f.shift > 0 && f.gainFree >= 0.7) std::printf ("    It needs more gain range: the capture is driven harder than this amp can be at Gain 0.80.\n");
                else if (f.shift < 0 && f.gainFree <= 0.3) std::printf ("    It needs a cleaner, lower-gain range: the capture breaks up later than this amp does even at Gain 0.20.\n");
            }
            else std::printf ("  Even with a free input-level offset no amp gets within tolerance (best: %s V%d, offset %+d dB%s, gain %.1f, H2 %.1f, H3 %.1f): the character differs, not just the amount of gain.\n", f.cand.name.c_str(), f.cand.voicing + 1, f.shift, extra.c_str(), f.eFree.G, f.eFree.H2, f.eFree.H3);
        }
    }

    //--- csv ---------------------------------------------------------------------------------------------------------------------------
    if (! o.csv.empty())
    {
        std::ofstream f (o.csv);
        f << "rank,model,model_name,ampVoicing,mode,gain,score,gain_err,h2_err,h3_err,band_err,out_trim_db\n";
        for (size_t i = 0; i < ranked.size(); ++i)
        {
            const Ranked& r = ranked[i];
            for (size_t gi = 0; gi < r.perGain.size(); ++gi)
                if (r.perGain[gi].ok) f << i + 1 << ',' << r.cand.model << ",\"" << r.cand.name << "\"," << r.cand.voicing << ',' << r.cand.mode << ',' << r.gains[gi] << ',' << r.perGain[gi].E << ',' << r.perGain[gi].G << ',' << r.perGain[gi].H2 << ',' << r.perGain[gi].H3 << ',' << r.perGain[gi].B << ',' << r.perGain[gi].offset << '\n';
        }
        std::printf ("wrote %s\n", o.csv.c_str());
    }
    std::printf ("total time %.0f s\n", wallSeconds() - tStart);
    return 0;
}
