#pragma once
// v70: IR library. Scans folders of impulse responses, measures each one's smoothed frequency curve,
// and keeps favourites, hidden files and near-duplicate marks. No audio from the files is kept:
// only a 160-point curve per file (and its length and sample rate) goes into the cache.
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

namespace irlib
{
    constexpr int kGrid = 160;                       // 50 Hz to 9 kHz, log spaced
    using Curve = std::array<float, kGrid>;

    inline const Curve& gridHz()
    {
        static const Curve g = []
        {
            Curve a {};
            for (int i = 0; i < kGrid; ++i)
                a[(size_t) i] = (float) (50.0 * std::pow (9000.0 / 50.0, (double) i / (kGrid - 1.0)));
            return a;
        }();
        return g;
    }

    // Root-mean-square difference in dB between two curves, 60 Hz to 6.5 kHz.
    inline float distanceDb (const float* a, const float* b)
    {
        const auto& g = gridHz();
        double sum = 0.0;
        int n = 0;
        for (int i = 0; i < kGrid; ++i)
            if (g[(size_t) i] >= 60.0f && g[(size_t) i] <= 6500.0f)
            {
                const double d = (double) a[i] - (double) b[i];
                sum += d * d;
                ++n;
            }
        return n > 0 ? (float) std::sqrt (sum / n) : 0.0f;
    }

    // Builds a curve from a power-versus-frequency function: each point is the average power over
    // 1/6 octave around it, in dB, then the whole curve is moved so that 500 Hz to 2 kHz averages 0 dB.
    template <class PowerAt>
    inline void curveFromPower (PowerAt powerAt, float* outDb)
    {
        const auto& g = gridHz();
        double ref = 0.0;
        int refN = 0;
        for (int i = 0; i < kGrid; ++i)
        {
            double sum = 0.0;
            for (int k = 0; k < 9; ++k)
                sum += powerAt ((double) g[(size_t) i] * std::pow (2.0, (double) (k - 4) / 48.0));
            const double db = 10.0 * std::log10 (sum / 9.0 + 1.0e-20);
            outDb[i] = (float) db;
            if (g[(size_t) i] >= 500.0f && g[(size_t) i] <= 2000.0f) { ref += db; ++refN; }
        }
        const float off = refN > 0 ? (float) (ref / refN) : 0.0f;
        for (int i = 0; i < kGrid; ++i)
            outDb[i] -= off;
    }

    // The curve of an impulse response (first 170 ms, with a fade at the end of that window).
    // 'channels' is how many channels of 'data' to use; their powers are averaged.
    inline bool analyseSamples (const float* const* data, int channels, int n, double sampleRate, float* outDb)
    {
        if (n < 32 || channels < 1 || sampleRate < 8000.0)
            return false;
        n = std::min (n, (int) std::lround (0.17 * sampleRate));

        int order = 10;
        while ((double) (1 << order) < sampleRate / 3.0 || (1 << order) < n)
            ++order;
        const int size = 1 << order;

        std::vector<double> power ((size_t) size / 2 + 1, 0.0);
        std::vector<float> work ((size_t) size * 2);
        juce::dsp::FFT fft (order);

        for (int c = 0; c < channels; ++c)
        {
            std::fill (work.begin(), work.end(), 0.0f);
            const int fadeStart = (int) (n * 0.75f);
            for (int i = 0; i < n; ++i)
            {
                float w = 1.0f;
                if (i > fadeStart)
                    w = 0.5f + 0.5f * std::cos (juce::MathConstants<float>::pi * (float) (i - fadeStart) / (float) (n - fadeStart));
                work[(size_t) i] = data[c][i] * w;
            }
            fft.performRealOnlyForwardTransform (work.data(), true);
            for (size_t k = 0; k < power.size(); ++k)
            {
                const double re = work[2 * k], im = work[2 * k + 1];
                power[k] += (re * re + im * im) / (double) channels;
            }
        }

        const double binHz = sampleRate / (double) size;
        double total = 0.0;
        for (double p : power) total += p;
        if (! (total > 1.0e-18))
            return false;

        auto powerAt = [&] (double f)
        {
            const double pos = f / binHz;
            const size_t k = (size_t) std::min<double> (std::floor (pos), (double) power.size() - 2.0);
            const double t = pos - (double) k;
            return power[k] * (1.0 - t) + power[k + 1] * t;
        };
        curveFromPower (powerAt, outDb);
        return true;
    }

    // What the curve says in words, as numbers: balance of five bands against the 500 Hz to 2 kHz level,
    // the low resonance, and where the top rolls off.
    struct Summary
    {
        float low = 0, lowMid = 0, upperMid = 0, presence = 0;   // dB
        float lowPeakHz = 0, lowPeakDb = 0;
        float topCutHz = 0;                                       // 0 = no roll-off found below 9 kHz
    };

    inline Summary summarise (const float* c)
    {
        const auto& g = gridHz();
        auto mean = [&] (float lo, float hi)
        {
            double s = 0.0; int n = 0;
            for (int i = 0; i < kGrid; ++i)
                if (g[(size_t) i] >= lo && g[(size_t) i] < hi) { s += c[i]; ++n; }
            return n > 0 ? (float) (s / n) : 0.0f;
        };
        Summary r;
        r.low = mean (80.0f, 250.0f);
        r.lowMid = mean (250.0f, 500.0f);
        r.upperMid = mean (2000.0f, 4000.0f);
        r.presence = mean (4000.0f, 6500.0f);
        r.lowPeakDb = -1000.0f;
        for (int i = 0; i < kGrid; ++i)
            if (g[(size_t) i] >= 60.0f && g[(size_t) i] <= 300.0f && c[i] > r.lowPeakDb)
            {
                r.lowPeakDb = c[i];
                r.lowPeakHz = g[(size_t) i];
            }
        const float ref = r.upperMid;
        for (int i = 0; i < kGrid; ++i)
            if (g[(size_t) i] > 4000.0f && c[i] < ref - 10.0f) { r.topCutHz = g[(size_t) i]; break; }
        return r;
    }

    struct Entry
    {
        juce::String path;
        juce::int64 size = 0, mtime = 0;
        float sampleRate = 0.0f, lengthMs = 0.0f;
        int channels = 1;
        bool ok = false;                 // false: not usable as a cab IR (too long, unreadable or silent)
        Curve curve {};
        // Worked out after a scan (not saved)
        int closest = -1;                // index of the closest built-in speaker
        float closestDist = 0.0f;
        int dupOf = -1;                  // index in the sorted list of the file this one nearly duplicates
        int dupCount = 0;                // for the first of a group: how many others match it

        juce::String name() const { return juce::File (path).getFileNameWithoutExtension(); }
    };

    constexpr float kDuplicateDb = 1.0f;   // curves closer than this (rms dB) count as the same sound

    //==========================================================================
    class Library : private juce::Thread
    {
    public:
        explicit Library (const juce::File& dataFolder)
            : juce::Thread ("IR library scan"), folder (dataFolder)
        {
            formats.registerBasicFormats();
            folder.createDirectory();
            loadSettings();
            loadCache();
            startThread();
        }

        ~Library() override
        {
            stopThread (4000);
            saveSettings();
        }

        // The curves of the built-in speakers, for "closest built-in". Set before the first scan.
        void setReferenceCurves (const std::vector<Curve>* refs) { reference = refs; }

        // Folders -------------------------------------------------------
        juce::StringArray getRoots() const { const juce::ScopedLock l (lock); return roots; }
        void addRoot (const juce::File& f)
        {
            {
                const juce::ScopedLock l (lock);
                if (! f.isDirectory() || roots.contains (f.getFullPathName()))
                    return;
                roots.add (f.getFullPathName());
            }
            saveSettings();
            rescan();
        }
        void removeRoot (const juce::String& path)
        {
            {
                const juce::ScopedLock l (lock);
                roots.removeString (path);
            }
            saveSettings();
            rescan();
        }

        // Marks ---------------------------------------------------------
        bool isFavourite (const juce::String& p) const { const juce::ScopedLock l (lock); return favourites.count (p) > 0; }
        bool isHidden    (const juce::String& p) const { const juce::ScopedLock l (lock); return hidden.count (p) > 0; }
        void setFavourite (const juce::String& p, bool on) { { const juce::ScopedLock l (lock); if (on) favourites.insert (p); else favourites.erase (p); } saveSettings(); ++version; }
        void setHidden    (const juce::String& p, bool on) { { const juce::ScopedLock l (lock); if (on) hidden.insert (p); else hidden.erase (p); } saveSettings(); ++version; }

        // Scanning ------------------------------------------------------
        void rescan() { wantScan = true; notify(); }
        bool isScanning() const { return scanning.load(); }
        int  scannedCount() const { return scanned.load(); }
        int  totalCount() const { return total.load(); }
        int  skippedCount() const { return skipped.load(); }
        int  getVersion() const { return version.load(); }

        // A copy of the list, sorted by path (usable files only).
        std::vector<Entry> getEntries() const { const juce::ScopedLock l (lock); return published; }

    private:
        void run() override
        {
            while (! threadShouldExit())
            {
                if (! wantScan.exchange (false))
                {
                    wait (-1);
                    continue;
                }
                doScan();
            }
        }

        void doScan()
        {
            scanning = true;
            scanned = 0;
            skipped = 0;

            juce::StringArray rootList;
            { const juce::ScopedLock l (lock); rootList = roots; }

            std::vector<juce::File> files;
            for (auto& r : rootList)
            {
                for (const auto& it : juce::RangedDirectoryIterator (juce::File (r), true, "*.wav;*.aif;*.aiff;*.WAV;*.AIF;*.AIFF", juce::File::findFiles))
                {
                    if (threadShouldExit()) return;
                    files.push_back (it.getFile());
                }
            }
            std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b) { return a.getFullPathName() < b.getFullPathName(); });
            files.erase (std::unique (files.begin(), files.end()), files.end());
            total = (int) files.size();

            std::vector<Entry> result;
            result.reserve (files.size());
            auto lastPublish = juce::Time::getMillisecondCounter();

            for (const auto& f : files)
            {
                if (threadShouldExit()) return;
                if (wantScan.load())   // the folder list changed: start over
                    break;

                Entry e;
                const auto path = f.getFullPathName();
                const auto size = f.getSize();
                const auto mtime = f.getLastModificationTime().toMilliseconds();
                auto cached = cache.find (path);
                if (cached != cache.end() && cached->second.size == size && cached->second.mtime == mtime)
                    e = cached->second;
                else
                {
                    e = measure (f);
                    e.path = path; e.size = size; e.mtime = mtime;
                    cache[path] = e;
                }
                ++scanned;
                if (e.ok) result.push_back (e); else ++skipped;

                if (juce::Time::getMillisecondCounter() - lastPublish > 600)
                {
                    finish (result);
                    lastPublish = juce::Time::getMillisecondCounter();
                }
            }

            finish (result);
            saveCache();
            scanning = false;
        }

        // Fills in the closest speaker and the duplicate marks, then publishes the list.
        void finish (std::vector<Entry> list)
        {
            std::sort (list.begin(), list.end(), [] (const Entry& a, const Entry& b) { return a.path < b.path; });

            if (reference != nullptr)
                for (auto& e : list)
                {
                    float best = 1.0e9f;
                    for (size_t k = 0; k < reference->size(); ++k)
                    {
                        const float d = distanceDb (e.curve.data(), (*reference)[k].data());
                        if (d < best) { best = d; e.closest = (int) k; }
                    }
                    e.closestDist = best;
                }

            // Near duplicates: each file is compared with the files kept so far
            std::vector<int> kept;
            for (int i = 0; i < (int) list.size(); ++i)
            {
                for (int k : kept)
                    if (distanceDb (list[(size_t) i].curve.data(), list[(size_t) k].curve.data()) < kDuplicateDb)
                    {
                        list[(size_t) i].dupOf = k;
                        ++list[(size_t) k].dupCount;
                        break;
                    }
                if (list[(size_t) i].dupOf < 0)
                    kept.push_back (i);
            }

            const juce::ScopedLock l (lock);
            published = std::move (list);
            ++version;
        }

        Entry measure (const juce::File& f)
        {
            Entry e;
            std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));
            if (reader == nullptr || reader->sampleRate < 8000.0 || reader->lengthInSamples < 32)
                return e;
            e.sampleRate = (float) reader->sampleRate;
            e.channels = (int) reader->numChannels;
            e.lengthMs = (float) (1000.0 * (double) reader->lengthInSamples / reader->sampleRate);
            if (reader->lengthInSamples > (juce::int64) (reader->sampleRate * 5.0))
                return e;                                  // a loop or a song, not a cab IR
            const int n = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) std::lround (0.17 * reader->sampleRate));
            const int ch = (int) std::min<unsigned int> (reader->numChannels, 2u);
            juce::AudioBuffer<float> buf (ch, n);
            buf.clear();
            reader->read (&buf, 0, n, 0, true, ch > 1);
            e.ok = analyseSamples (buf.getArrayOfReadPointers(), ch, n, reader->sampleRate, e.curve.data());
            return e;
        }

        //----------------------------------------------------------------------
        juce::File settingsFile() const { return folder.getChildFile ("irlibrary.json"); }
        juce::File cacheFile() const { return folder.getChildFile ("irlibrary.cache"); }

        void loadSettings()
        {
            auto v = juce::JSON::parse (settingsFile());
            if (auto* o = v.getDynamicObject())
            {
                auto read = [&] (const char* key, auto&& add)
                {
                    if (auto* a = o->getProperty (key).getArray())
                        for (auto& x : *a) add (x.toString());
                };
                read ("roots", [this] (const juce::String& s) { roots.add (s); });
                read ("favourites", [this] (const juce::String& s) { favourites.insert (s); });
                read ("hidden", [this] (const juce::String& s) { hidden.insert (s); });
            }
        }

        void saveSettings()
        {
            auto* o = new juce::DynamicObject();
            juce::Array<juce::var> r, f, h;
            {
                const juce::ScopedLock l (lock);
                for (auto& s : roots) r.add (s);
                for (auto& s : favourites) f.add (s);
                for (auto& s : hidden) h.add (s);
            }
            o->setProperty ("roots", r);
            o->setProperty ("favourites", f);
            o->setProperty ("hidden", h);
            settingsFile().replaceWithText (juce::JSON::toString (juce::var (o)));
        }

        static constexpr int kCacheMagic = 0x49524c32;   // "IRL2"

        void loadCache()
        {
            juce::FileInputStream in (cacheFile());
            if (! in.openedOk() || in.readInt() != kCacheMagic)
                return;
            const int n = in.readInt();
            for (int i = 0; i < n && ! in.isExhausted(); ++i)
            {
                Entry e;
                e.path = in.readString();
                e.size = in.readInt64();
                e.mtime = in.readInt64();
                e.sampleRate = in.readFloat();
                e.lengthMs = in.readFloat();
                e.channels = in.readInt();
                e.ok = in.readBool();
                for (auto& v : e.curve) v = in.readFloat();
                cache[e.path] = e;
            }
        }

        void saveCache()
        {
            juce::MemoryOutputStream out;
            out.writeInt (kCacheMagic);
            out.writeInt ((int) cache.size());
            for (auto& kv : cache)
            {
                const auto& e = kv.second;
                out.writeString (e.path);
                out.writeInt64 (e.size);
                out.writeInt64 (e.mtime);
                out.writeFloat (e.sampleRate);
                out.writeFloat (e.lengthMs);
                out.writeInt (e.channels);
                out.writeBool (e.ok);
                for (float v : e.curve) out.writeFloat (v);
            }
            cacheFile().replaceWithData (out.getData(), out.getDataSize());
        }

        juce::File folder;
        juce::AudioFormatManager formats;
        mutable juce::CriticalSection lock;
        juce::StringArray roots;
        std::set<juce::String> favourites, hidden;
        std::map<juce::String, Entry> cache;       // only touched by the scan thread (after the constructor)
        std::vector<Entry> published;
        const std::vector<Curve>* reference = nullptr;
        std::atomic<bool> wantScan { false }, scanning { false };
        std::atomic<int> scanned { 0 }, total { 0 }, skipped { 0 }, version { 0 };
    };
}
