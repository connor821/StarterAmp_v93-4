#pragma once
// Tone Lab (v75): matches your sound to a recording by comparing two files.
//
//   DI file     : your guitar, recorded clean (no amp)
//   Target file : the same kind of playing, after the amp, pedals and cab you want to sound like
//
// It plays the DI through a private copy of this plugin and measures how close the result is to the target, then
// changes the settings (amp model, gain and tone, cab speaker and mic, compressor, overdrive, boost, and finally the
// Post EQ) one step at a time and keeps each change that gets closer. It never makes a result worse than the start.
//
// What it measures (all long-term, so the two takes do not have to line up in time or be the same notes):
//   - the average tonal balance, in 25 third-octave bands from 50 Hz to 12.7 kHz (the biggest part of the score)
//   - the crest factor (how far the peaks stand above the average level): a sign of compression and clipping
//   - the dynamic spread (how much the level moves between quiet and loud passages)
// It does not match loudness, the ambience of a room or reverb, delay times, or modulation.
#include "ToneMatch.h"
#include "PluginProcessor.h"
#include <atomic>
#include <functional>
#include <map>
#include <string>

namespace ToneLab
{
    struct Features
    {
        std::array<float, ToneMatch::kBands> shape {};      // dB, mean removed
        float crestDb = 0.0f, spreadDb = 0.0f, rmsDb = -100.0f;
        bool ok = false;
    };

    // Mono mix of a file, at the file's own sample rate. maxSeconds = 0 reads everything (up to 120 s).
    inline bool loadMono (const juce::File& file, std::vector<float>& mono, double& sampleRate, juce::String& error)
    {
        juce::AudioFormatManager fm;
        fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
        if (reader == nullptr || reader->lengthInSamples < 16384) { error = "That file could not be read (or it is too short)."; return false; }
        sampleRate = reader->sampleRate;
        const juce::int64 total = std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (120.0 * sampleRate));
        mono.assign ((size_t) total, 0.0f);
        juce::AudioBuffer<float> buf ((int) reader->numChannels, 65536);
        for (juce::int64 pos = 0; pos < total; pos += 65536)
        {
            const int n = (int) std::min<juce::int64> (65536, total - pos);
            reader->read (&buf, 0, n, pos, true, true);
            for (int i = 0; i < n; ++i)
            {
                float s = 0.0f;
                for (int c = 0; c < buf.getNumChannels(); ++c) s += buf.getSample (c, i);
                mono[(size_t) (pos + i)] = s / (float) buf.getNumChannels();
            }
        }
        return true;
    }

    // The loudest stretch of 'seconds' (so a long DI file is not rendered in full on every step).
    inline std::vector<float> loudestExcerpt (const std::vector<float>& x, double sr, double seconds)
    {
        const size_t len = (size_t) (seconds * sr);
        if (x.size() <= len) return x;
        const size_t hop = (size_t) (0.25 * sr);
        std::vector<double> cum (x.size() / hop + 2, 0.0);
        for (size_t i = 0; i < x.size(); ++i) cum[i / hop + 1] += (double) x[i] * x[i];
        for (size_t i = 1; i < cum.size(); ++i) cum[i] += cum[i - 1];
        const size_t blocks = len / hop;
        size_t best = 0;
        double bestE = -1.0;
        for (size_t b = 0; b + blocks < cum.size(); ++b)
        {
            const double e = cum[b + blocks] - cum[b];
            if (e > bestE) { bestE = e; best = b; }
        }
        const size_t start = std::min (best * hop, x.size() - len);
        return std::vector<float> (x.begin() + (std::ptrdiff_t) start, x.begin() + (std::ptrdiff_t) (start + len));
    }

    inline Features analyse (const std::vector<float>& xIn, double sr)
    {
        Features f;
        if (xIn.size() < (size_t) (ToneMatch::Analyzer::size * 2)) return f;

        // normalise the whole signal to about -20 dB RMS (only shapes and ratios are compared)
        double e = 0.0;
        for (float v : xIn) e += (double) v * v;
        const double rms = std::sqrt (e / (double) xIn.size());
        if (rms < 1.0e-6) return f;
        const float g = (float) (0.1 / rms);
        std::vector<float> x (xIn.size());
        for (size_t i = 0; i < x.size(); ++i) x[i] = xIn[i] * g;

        // tonal balance
        ToneMatch::Analyzer an;
        for (size_t pos = 0; pos + (size_t) ToneMatch::Analyzer::size <= x.size(); pos += (size_t) ToneMatch::Analyzer::size / 2)
            an.addFrame (x.data() + pos, sr);
        if (an.frames < 3) return f;
        f.shape = an.shapeDb();

        // dynamics, from 50 ms frames every 25 ms
        const size_t win = (size_t) (0.05 * sr), hop = win / 2;
        std::vector<float> db;
        double peak = 0.0;
        for (size_t pos = 0; pos + win <= x.size(); pos += hop)
        {
            double s = 0.0;
            for (size_t i = 0; i < win; ++i) { const double v = x[pos + i]; s += v * v; peak = std::max (peak, std::abs (v)); }
            db.push_back ((float) (10.0 * std::log10 (s / (double) win + 1.0e-12)));
        }
        if (db.size() < 10) return f;
        const float top = *std::max_element (db.begin(), db.end());
        std::vector<float> active;
        double act = 0.0;
        for (float v : db) if (v > top - 40.0f) { active.push_back (v); act += std::pow (10.0, v / 10.0); }
        if (active.size() < 8) return f;
        std::sort (active.begin(), active.end());
        f.spreadDb = active[(size_t) ((double) active.size() * 0.9)] - active[(size_t) ((double) active.size() * 0.1)];
        const double activeRms = std::sqrt (act / (double) active.size());
        f.crestDb = (float) (20.0 * std::log10 (peak / std::max (1.0e-9, activeRms)));
        f.rmsDb = (float) (20.0 * std::log10 (rms + 1.0e-12));
        f.ok = true;
        return f;
    }

    // 0 = identical. Roughly "dB of difference": the tonal balance counts most.
    inline float shapeDistance (const Features& a, const Features& b)
    {
        double s = 0.0, wsum = 0.0;
        for (int k = 0; k < ToneMatch::kBands; ++k)
        {
            const float hz = ToneMatch::bandHz (k);
            const double w = (hz < 80.0f || hz > 9000.0f) ? 0.5 : 1.0;
            const double d = (double) a.shape[(size_t) k] - b.shape[(size_t) k];
            s += w * d * d;  wsum += w;
        }
        return (float) std::sqrt (s / wsum);
    }
    inline float distance (const Features& target, const Features& mine)
    {
        if (! target.ok || ! mine.ok) return 1000.0f;
        return shapeDistance (target, mine) + 0.5f * std::abs (target.crestDb - mine.crestDb) + 0.35f * std::abs (target.spreadDb - mine.spreadDb);
    }

    struct Options { bool amp = true, cab = true, pedals = true, eq = true; };

    struct Result
    {
        bool ok = false;
        juce::String message;
        float startCost = 0.0f, endCost = 0.0f;
        Features target, start, end;
        std::vector<std::pair<juce::String, float>> changes;     // parameter id and its new real value
        juce::StringArray notes;                                 // one line per change, for people
        int evaluations = 0;
    };

    class Fitter
    {
    public:
        using Progress = std::function<void (float, const juce::String&)>;

        // 'probe' is a private StarterAmpProcessor already holding the settings to start from.
        Fitter (StarterAmpProcessor& probeIn, std::vector<float> diMono, double sampleRateIn, const Features& targetIn, Options o)
            : probe (probeIn), di (std::move (diMono)), sr (sampleRateIn), target (targetIn), opts (o) {}

        Result run (const Progress& progress, const std::atomic<bool>& cancelFlag)
        {
            Result r;
            r.target = target;
            cancel = &cancelFlag;
            report = &progress;
            probe.setPlayConfigDetails (2, 2, sr, 512);
            probe.setNonRealtime (true);

            // remember every parameter we might touch, to report what changed
            static const char* watched[] = { "ampModel", "ampVoicing", "ampGain", "bass", "lowMid", "highMid", "treble", "presence",
                                             "cabSpkA", "cabMicA", "cabCbnA", "cabPosA", "cabDistA",
                                             "odOn", "odGain", "compOn", "compThresh", "bsOn", "bsGain", "eqOn" };
            for (auto* id : watched) initial[id] = get (id);
            for (int b = 1; b <= kEqBands; ++b) initial[("eqB" + juce::String (b)).toStdString()] = get (("eqB" + juce::String (b)).toStdString());

            Features cur = measure();
            if (! cur.ok) { r.message = "The DI file did not produce any sound (is it silent?)."; return r; }
            r.start = cur;
            best = distance (target, cur);
            r.startCost = best;

            const bool cabUsable = opts.cab && get ("cabOnA") > 0.5f && get ("cabSrcA") < 0.5f;

            // fractions of the progress bar for each stage
            float done = 0.0f;
            auto stage = [&] (float share, const juce::String& name) { stageStart = done; stageShare = share; done += share; say (0.0f, name); };

            if (opts.amp && ! cancelled())
            {
                stage (0.34f, "Trying the amp models...");
                searchAmps();
            }
            if (cabUsable && ! cancelled())
            {
                stage (0.24f, "Trying cab speakers and mics...");
                searchCab();
            }
            if (opts.pedals && ! cancelled())
            {
                stage (0.14f, "Trying the compressor, overdrive and boost...");
                searchPedals();
                stage (0.08f, "Fine-tuning the amp...");
                refineTone (2);
            }
            else if (! cancelled())
            {
                stage (0.14f, "Fine-tuning the amp...");
                refineTone (3);
            }
            if (opts.eq && ! cancelled())
            {
                stage (0.06f, "Fitting the Post EQ...");
                fitEq();
            }

            Features fin = measure();
            r.end = fin;
            r.endCost = distance (target, fin);
            r.evaluations = evals;
            r.ok = true;
            const auto changedIds = collectChanges (r);
            juce::ignoreUnused (changedIds);
            if (cancelled()) r.message = "Stopped early. Here is the best result so far.";
            return r;
        }

    private:
        bool cancelled() const { return cancel != nullptr && cancel->load(); }
        void say (float within, const juce::String& text)
        {
            if (report != nullptr) (*report) (stageStart + stageShare * juce::jlimit (0.0f, 1.0f, within), text);
        }

        float get (const std::string& id) const
        {
            if (auto* v = probe.apvts.getRawParameterValue (id)) return v->load();
            return 0.0f;
        }
        void set (const std::string& id, float real)
        {
            if (auto* prm = probe.apvts.getParameter (id))
                prm->setValueNotifyingHost (prm->convertTo0to1 (real));
        }
        int numChoices (const std::string& id) const
        {
            if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (probe.apvts.getParameter (id))) return c->choices.size();
            return 0;
        }
        std::pair<float, float> rangeOf (const std::string& id) const
        {
            if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (probe.apvts.getParameter (id)))
                return { prm->getNormalisableRange().start, prm->getNormalisableRange().end };
            return { 0.0f, 1.0f };
        }

        // Cab and other slow changes are finished on the message thread: wait for it.
        void waitIdle()
        {
            for (int i = 0; i < 600 && probe.hasPendingRefresh(); ++i) juce::Thread::sleep (5);
        }

        std::vector<float> render()
        {
            probe.prepareToPlay (sr, 512);
            std::vector<float> out;
            out.reserve (di.size());
            juce::AudioBuffer<float> buf (2, 512);
            juce::MidiBuffer midi;
            for (size_t pos = 0; pos < di.size(); pos += 512)
            {
                const int n = (int) std::min<size_t> (512, di.size() - pos);
                buf.setSize (2, n, false, false, true);
                for (int i = 0; i < n; ++i) { buf.setSample (0, i, di[pos + (size_t) i]); buf.setSample (1, i, di[pos + (size_t) i]); }
                probe.processBlock (buf, midi);
                for (int i = 0; i < n; ++i) out.push_back (0.5f * (buf.getSample (0, i) + buf.getSample (1, i)));
            }
            const size_t skip = (size_t) (0.25 * sr);
            if (out.size() > skip + 32768) out.erase (out.begin(), out.begin() + (std::ptrdiff_t) skip);
            return out;
        }

        Features measure()
        {
            waitIdle();
            ++evals;
            return analyse (render(), sr);
        }

        float cost()
        {
            const float c = distance (target, measure());
            return std::isfinite (c) ? c : 1000.0f;
        }

        // Tries a set of parameter assignments; keeps the best if it beats 'best' by at least 'margin'.
        struct Setting { std::string id; float value; };
        bool tryCandidates (const std::vector<std::vector<Setting>>& cands, float margin)
        {
            std::map<std::string, float> before;
            for (auto& c : cands) for (auto& s : c) before[s.id] = get (s.id);
            int bestIdx = -1;
            float bestCost = best - margin;
            for (size_t i = 0; i < cands.size() && ! cancelled(); ++i)
            {
                for (auto& s : cands[i]) set (s.id, s.value);
                const float c = cost();
                if (c < bestCost) { bestCost = c; bestIdx = (int) i; }
                for (auto& kv : before) set (kv.first, kv.second);
            }
            if (bestIdx < 0) { waitIdle(); return false; }
            for (auto& s : cands[(size_t) bestIdx]) set (s.id, s.value);
            best = bestCost;
            waitIdle();
            return true;
        }

        void refineTone (int rounds)
        {
            struct P { const char* id; float step; };
            P ps[] = { { "ampGain", 0.12f }, { "bass", 3.0f }, { "lowMid", 3.0f }, { "highMid", 3.0f }, { "treble", 3.0f }, { "presence", 0.2f } };
            for (int round = 0; round < rounds && ! cancelled(); ++round)
            {
                for (auto& p : ps)
                {
                    if (cancelled()) return;
                    const auto rg = rangeOf (p.id);
                    const float cur = get (p.id);
                    const float step = p.step / (float) (1 << round);
                    tryCandidates ({ { { p.id, juce::jlimit (rg.first, rg.second, cur + step) } },
                                     { { p.id, juce::jlimit (rg.first, rg.second, cur - step) } } }, 0.03f);
                    say ((float) (round * 6 + (&p - ps) + 1) / (float) (rounds * 6), "Fine-tuning the amp...");
                }
            }
        }

        void searchAmps()
        {
            const int n = numChoices ("ampModel");
            const float gains[3] = { 0.35f, 0.62f, 0.88f };
            struct Cand { int model; float gain; float cost; };
            std::vector<Cand> all;
            const float startModel = get ("ampModel"), startGain = get ("ampGain");
            for (int m = 0; m < n && ! cancelled(); ++m)
            {
                Cand bc { m, gains[0], 1.0e9f };
                for (float g : gains)
                {
                    set ("ampModel", (float) m);  set ("ampGain", g);
                    const float c = cost();
                    if (c < bc.cost) bc = { m, g, c };
                }
                all.push_back (bc);
                say ((float) (m + 1) / (float) n, "Trying the amp models...");
            }
            set ("ampModel", startModel);  set ("ampGain", startGain);
            if (all.empty() || cancelled()) return;
            std::sort (all.begin(), all.end(), [] (const Cand& a, const Cand& b) { return a.cost < b.cost; });

            // refine the three best, keep the winner (never worse than where we started)
            std::map<std::string, float> bestState;
            float bestOverall = best;
            bool improved = false;
            const int keep = std::min<int> (3, (int) all.size());
            for (int i = 0; i < keep && ! cancelled(); ++i)
            {
                set ("ampModel", (float) all[(size_t) i].model);  set ("ampGain", all[(size_t) i].gain);
                best = cost();
                // voicing
                const int nv = std::max (1, numChoices ("ampVoicing"));
                std::vector<std::vector<Setting>> vs;
                for (int v = 0; v < nv; ++v) vs.push_back ({ { "ampVoicing", (float) v } });
                tryCandidates (vs, 0.03f);
                refineTone (2);
                if (best < bestOverall)
                {
                    bestOverall = best;  improved = true;
                    for (auto* id : { "ampModel", "ampVoicing", "ampGain", "bass", "lowMid", "highMid", "treble", "presence" }) bestState[id] = get (id);
                }
                say ((float) (i + 1) / (float) keep, "Refining the best amps...");
            }
            if (improved) for (auto& kv : bestState) set (kv.first, kv.second);
            else { set ("ampModel", startModel); set ("ampGain", startGain); }
            waitIdle();
            best = cost();
        }

        void searchCab()
        {
            auto scan = [&] (const char* id, const char* label)
            {
                const int n = numChoices (id);
                std::vector<std::vector<Setting>> cands;
                for (int i = 0; i < n; ++i) cands.push_back ({ { id, (float) i } });
                for (size_t i = 0; i < cands.size() && ! cancelled(); ++i)
                {
                    // evaluate one at a time so the progress bar moves
                    const float before = get (id);
                    set (id, (float) i);
                    const float c = cost();
                    if (c < best - 0.03f) { best = c; }
                    else set (id, before);
                    say ((float) (i + 1) / (float) std::max (1, n), label);
                }
                waitIdle();
            };
            scan ("cabSpkA", "Trying cab speakers...");
            scan ("cabMicA", "Trying mics...");
            scan ("cabCbnA", "Trying cabinets...");
            std::vector<std::vector<Setting>> pos, dist;
            for (float p : { 0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 1.0f }) pos.push_back ({ { "cabPosA", p } });
            tryCandidates (pos, 0.03f);
            for (float d : { 0.0f, 2.0f, 5.0f, 9.0f }) dist.push_back ({ { "cabDistA", d } });
            tryCandidates (dist, 0.03f);
            best = cost();
        }

        void searchPedals()
        {
            const float margin = 0.12f;
            tryCandidates ({ { { "odOn", 0.0f } },
                             { { "odOn", 1.0f }, { "odGain", 0.2f } },
                             { { "odOn", 1.0f }, { "odGain", 0.55f } },
                             { { "odOn", 1.0f }, { "odGain", 0.85f } } }, margin);
            say (0.33f, "Trying the compressor...");
            tryCandidates ({ { { "compOn", 0.0f } },
                             { { "compOn", 1.0f }, { "compThresh", -30.0f } },
                             { { "compOn", 1.0f }, { "compThresh", -22.0f } },
                             { { "compOn", 1.0f }, { "compThresh", -14.0f } } }, margin);
            say (0.66f, "Trying the boost...");
            tryCandidates ({ { { "bsOn", 0.0f } },
                             { { "bsOn", 1.0f }, { "bsGain", 6.0f } },
                             { { "bsOn", 1.0f }, { "bsGain", 12.0f } },
                             { { "bsOn", 1.0f }, { "bsGain", 18.0f } } }, margin);
            best = cost();
        }

        void fitEq()
        {
            const Features now = measure();
            if (! now.ok) return;
            const auto resp = ToneMatch::responses (sr, get ("eqMode") > 0.5f, get ("eqQ"));
            std::array<float, ToneMatch::kBands> wanted {};
            for (int k = 0; k < ToneMatch::kBands; ++k) wanted[(size_t) k] = target.shape[(size_t) k] - now.shape[(size_t) k];

            std::array<float, kEqBands> startFaders {};
            for (int b = 0; b < kEqBands; ++b) startFaders[(size_t) b] = get ("eqB" + juce::String (b + 1).toStdString());
            const float startOn = get ("eqOn");

            // the faders are applied on top of what is already set: the measurement above already includes the current EQ
            float bestStrength = 0.0f, bestCost = best;
            for (float strength : { 1.0f, 0.7f, 0.4f })
            {
                if (cancelled()) break;
                const auto g = ToneMatch::fit (wanted, resp, strength);
                for (int b = 0; b < kEqBands; ++b)
                    set ("eqB" + juce::String (b + 1).toStdString(), juce::jlimit (-12.0f, 12.0f, startFaders[(size_t) b] + g[(size_t) b]));
                set ("eqOn", 1.0f);
                const float c = cost();
                if (c < bestCost - 0.03f) { bestCost = c; bestStrength = strength; }
            }
            if (bestStrength > 0.0f)
            {
                const auto g = ToneMatch::fit (wanted, resp, bestStrength);
                for (int b = 0; b < kEqBands; ++b)
                    set ("eqB" + juce::String (b + 1).toStdString(), juce::jlimit (-12.0f, 12.0f, startFaders[(size_t) b] + g[(size_t) b]));
                set ("eqOn", 1.0f);
                best = bestCost;
            }
            else
            {
                for (int b = 0; b < kEqBands; ++b) set ("eqB" + juce::String (b + 1).toStdString(), startFaders[(size_t) b]);
                set ("eqOn", startOn);
            }
            waitIdle();
        }

        juce::String describe (const std::string& id)
        {
            static const std::map<std::string, const char*> names = {
                { "ampModel", "Amp model" }, { "ampVoicing", "Amp voicing" }, { "ampGain", "Amp gain" }, { "bass", "Bass" },
                { "lowMid", "Low mid" }, { "highMid", "High mid" }, { "treble", "Treble" }, { "presence", "Presence" },
                { "cabSpkA", "Cab block A speaker" }, { "cabMicA", "Cab block A mic" }, { "cabCbnA", "Cab block A cabinet" },
                { "cabPosA", "Cab block A mic position" }, { "cabDistA", "Cab block A mic distance" },
                { "odOn", "Overdrive" }, { "odGain", "Overdrive gain" }, { "compOn", "Compressor" }, { "compThresh", "Compressor threshold" },
                { "bsOn", "Boost" }, { "bsGain", "Boost gain" } };
            auto it = names.find (id);
            if (it == names.end()) return {};
            if (auto* prm = probe.apvts.getParameter (id))
                return juce::String (it->second) + ": " + prm->getText (prm->getValue(), 60);
            return {};
        }

        int collectChanges (Result& r)
        {
            int n = 0;
            juce::String eqNote;
            for (auto& kv : initial)
            {
                const float now = get (kv.first);
                if (std::abs (now - kv.second) < 0.0005f) continue;
                r.changes.push_back ({ juce::String (kv.first), now });
                ++n;
                if (kv.first.rfind ("eqB", 0) == 0 || kv.first == "eqOn") { eqNote = "Post EQ: nine faders set to fit the remaining difference (EQ switched on)"; continue; }
                if (describe (kv.first).isNotEmpty()) r.notes.add (describe (kv.first));
            }
            if (eqNote.isNotEmpty()) r.notes.add (eqNote);
            return n;
        }

        StarterAmpProcessor& probe;
        std::vector<float> di;
        double sr;
        Features target;
        Options opts;
        std::map<std::string, float> initial;
        const std::atomic<bool>* cancel = nullptr;
        const Progress* report = nullptr;
        float best = 1.0e9f, stageStart = 0.0f, stageShare = 1.0f;
        int evals = 0;
    };
}
