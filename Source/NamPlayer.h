#pragma once
// A small player for ".nam" neural amp capture files (an open, published file format).
//
// It reads the JSON file, builds the network and runs it. It supports the two architectures that nearly all shared
// captures use: WaveNet (the standard, lite, feather and nano sizes; plain layers with tanh/ReLU/sigmoid activations,
// optionally gated) and LSTM. Anything else (the newer "A2" WaveNet variants with extra options, ConvNet, containers)
// is refused with a clear message instead of guessing.
//
// This is my own implementation of the published file format, written without copying the engine's code. Its output
// was compared against the official engine on example files (see the README for how it was checked).
//
// Everything that allocates (loading, building, resampler tables) happens on the loading thread. process() only
// reads and writes preallocated buffers.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <vector>
#include <juce_core/juce_core.h>

namespace NamLite
{
    // The same rational tanh approximation the NAM plugin uses by default (fast and within about 1e-4 of tanh).
    inline float fastTanh (float x)
    {
        const float ax = std::fabs (x), x2 = x * x;
        return x * (2.45550750702956f + 2.45550750702956f * ax + (0.893229853513558f + 0.821226666969744f * ax) * x2)
                 / (2.44506634652299f + (2.44506634652299f + x2) * std::fabs (x + 0.814642734961073f * x * ax));
    }
    inline float sigmoid (float x) { return 0.5f * (fastTanh (0.5f * x) + 1.0f); }        // used by the LSTM, like the official engine
    inline float exactSigmoid (float x) { return 1.0f / (1.0f + std::exp (-x)); }         // used by WaveNet gates

    struct Info
    {
        juce::String name, gear, architecture;
        double sampleRate = 48000.0;
        bool   hasLoudness = false;
        float  loudnessDb = 0.0f;
        int    parameters = 0;
        int    receptiveField = 0;
    };

    //==========================================================================
    class Network
    {
    public:
        static constexpr int kBlock = 512;                          // the most samples process() takes at once
        virtual ~Network() = default;
        virtual void reset() = 0;                                   // forget the signal history (and warm up)
        virtual void process (const float* in, float* out, int n) = 0;   // n <= kBlock; in and out may be the same buffer
        virtual int  warmUpSamples() const = 0;
    };

    //==========================================================================
    class WaveNet : public Network
    {
    public:
        enum class Act { Tanh, ReLU, Sigmoid };

        struct LayerSpec { int inputSize, condSize, headSize, channels, kernel; std::vector<int> dilations; Act act; bool gated, headBias; };

        // Weights are consumed in the file's order. Returns false (and an error) if the count does not match exactly.
        bool build (const std::vector<LayerSpec>& specs, const std::vector<float>& w, juce::String& error)
        {
            size_t pos = 0;
            auto take = [&] (std::vector<float>& dst, size_t n) -> bool
            {
                if (pos + n > w.size()) return false;
                dst.assign (w.begin() + (std::ptrdiff_t) pos, w.begin() + (std::ptrdiff_t) (pos + n));
                pos += n;
                return true;
            };
            arrays.clear();
            for (size_t a = 0; a < specs.size(); ++a)
            {
                const auto& s = specs[a];
                Array ar;
                ar.inSize = s.inputSize; ar.cond = s.condSize; ar.C = s.channels; ar.headSize = s.headSize;
                if (a > 0 && s.inputSize != specs[a - 1].channels) { error = "the layer arrays do not line up"; return false; }
                if (a > 0 && s.channels != specs[a - 1].headSize) { error = "the layer arrays do not line up"; return false; }
                if (a == 0 && (s.inputSize != 1 || s.condSize != 1)) { error = "only single-input models are supported"; return false; }
                if (! take (ar.rechW, (size_t) s.channels * (size_t) s.inputSize)) { error = "the weights are shorter than the model needs"; return false; }
                for (int d : s.dilations)
                {
                    Layer L;
                    L.C = s.channels; L.K = s.kernel; L.D = d; L.gated = s.gated; L.act = s.act;
                    L.Z = s.gated ? 2 * s.channels : s.channels;
                    L.cond = s.condSize;
                    std::vector<float> raw;
                    // conv weights are stored as [out][in][kernel]; they are used as [kernel][out][in]
                    if (! take (raw, (size_t) L.Z * (size_t) L.C * (size_t) L.K)) { error = "the weights are shorter than the model needs"; return false; }
                    L.convW.resize (raw.size());
                    for (int o = 0; o < L.Z; ++o)
                        for (int i = 0; i < L.C; ++i)
                            for (int k = 0; k < L.K; ++k)
                                L.convW[((size_t) k * (size_t) L.Z + (size_t) o) * (size_t) L.C + (size_t) i] = raw[((size_t) o * (size_t) L.C + (size_t) i) * (size_t) L.K + (size_t) k];
                    if (! take (L.convB, (size_t) L.Z)) { error = "the weights are shorter than the model needs"; return false; }
                    if (! take (L.mixW, (size_t) L.Z * (size_t) L.cond)) { error = "the weights are shorter than the model needs"; return false; }
                    if (! take (L.w1, (size_t) L.C * (size_t) L.C)) { error = "the weights are shorter than the model needs"; return false; }
                    if (! take (L.b1, (size_t) L.C)) { error = "the weights are shorter than the model needs"; return false; }
                    L.hist = (L.K - 1) * L.D;
                    L.stride = L.hist + kBlock;
                    L.xin.assign ((size_t) L.C * (size_t) L.stride, 0.0f);
                    L.z.assign ((size_t) L.Z * kBlock, 0.0f);
                    L.act_.assign ((size_t) L.C * kBlock, 0.0f);
                    ar.layers.push_back (std::move (L));
                }
                if (! take (ar.headW, (size_t) s.headSize * (size_t) s.channels)) { error = "the weights are shorter than the model needs"; return false; }
                ar.headB.assign ((size_t) s.headSize, 0.0f);
                if (s.headBias && ! take (ar.headB, (size_t) s.headSize)) { error = "the weights are shorter than the model needs"; return false; }
                ar.headAcc.assign ((size_t) s.channels * kBlock, 0.0f);
                ar.headOut.assign ((size_t) s.headSize * kBlock, 0.0f);
                ar.out.assign ((size_t) s.channels * kBlock, 0.0f);
                ar.x0.assign ((size_t) s.channels * kBlock, 0.0f);
                ar.tmpA.assign ((size_t) s.channels * kBlock, 0.0f);
                ar.tmpB.assign ((size_t) s.channels * kBlock, 0.0f);
                arrays.push_back (std::move (ar));
            }
            std::vector<float> scale;
            if (! take (scale, 1)) { error = "the weights are shorter than the model needs"; return false; }
            headScale = scale[0];
            if (pos != w.size()) { error = "the file has more weights than the model uses"; return false; }
            receptive = 1;
            for (auto& ar : arrays) for (auto& L : ar.layers) receptive += L.hist;
            cond.assign (kBlock, 0.0f);
            return true;
        }

        void reset() override
        {
            for (auto& ar : arrays) for (auto& L : ar.layers) std::fill (L.xin.begin(), L.xin.end(), 0.0f);
            std::vector<float> zeros ((size_t) kBlock, 0.0f), sink ((size_t) kBlock);
            for (int done = 0; done < warmUpSamples(); done += kBlock)
                process (zeros.data(), sink.data(), kBlock);
        }
        int warmUpSamples() const override { return receptive + 64; }

        void process (const float* in, float* out, int n) override
        {
            std::memcpy (cond.data(), in, sizeof (float) * (size_t) n);      // the input is also the condition signal
            const float* arrIn = cond.data();
            int arrInRows = 1;
            const float* prevHead = nullptr;
            for (size_t a = 0; a < arrays.size(); ++a)
            {
                auto& ar = arrays[a];
                const int C = ar.C;
                // rechannel: x0 = W * input
                for (int c = 0; c < C; ++c)
                {
                    float* dst = ar.x0.data() + (size_t) c * kBlock;
                    std::fill (dst, dst + n, 0.0f);
                    for (int j = 0; j < arrInRows; ++j)
                    {
                        const float w = ar.rechW[(size_t) c * (size_t) arrInRows + (size_t) j];
                        const float* src = arrIn + (size_t) j * kBlock;
                        for (int t = 0; t < n; ++t) dst[t] += w * src[t];
                    }
                }
                // the head accumulator starts from the previous array's head output (or zero)
                if (prevHead == nullptr) std::fill (ar.headAcc.begin(), ar.headAcc.end(), 0.0f);
                else std::memcpy (ar.headAcc.data(), prevHead, sizeof (float) * (size_t) C * kBlock);

                const float* x = ar.x0.data();                // [C][kBlock]
                for (size_t li = 0; li < ar.layers.size(); ++li)
                {
                    auto& L = ar.layers[li];
                    // put this layer's input after its history
                    for (int c = 0; c < C; ++c)
                        std::memcpy (L.xin.data() + (size_t) c * (size_t) L.stride + (size_t) L.hist, x + (size_t) c * kBlock, sizeof (float) * (size_t) n);
                    // z = bias + conv + condition mixin
                    for (int o = 0; o < L.Z; ++o)
                    {
                        float* z = L.z.data() + (size_t) o * kBlock;
                        const float b = L.convB[(size_t) o];
                        for (int t = 0; t < n; ++t) z[t] = b;
                        for (int k = 0; k < L.K; ++k)
                        {
                            const int shift = (L.K - 1 - k) * L.D;
                            const float* wrow = L.convW.data() + ((size_t) k * (size_t) L.Z + (size_t) o) * (size_t) C;
                            for (int i = 0; i < C; ++i)
                            {
                                const float w = wrow[i];
                                const float* src = L.xin.data() + (size_t) i * (size_t) L.stride + (size_t) (L.hist - shift);
                                for (int t = 0; t < n; ++t) z[t] += w * src[t];
                            }
                        }
                        const float mw = L.mixW[(size_t) o];            // condition size 1
                        for (int t = 0; t < n; ++t) z[t] += mw * cond[(size_t) t];
                    }
                    // activation (gated: primary activation times sigmoid of the second half)
                    for (int c = 0; c < C; ++c)
                    {
                        float* z = L.z.data() + (size_t) c * kBlock;
                        float* r = L.act_.data() + (size_t) c * kBlock;
                        if (L.gated)
                        {
                            const float* g = L.z.data() + (size_t) (C + c) * kBlock;
                            for (int t = 0; t < n; ++t) r[t] = activate (L.act, z[t]) * exactSigmoid (g[t]);
                        }
                        else
                            for (int t = 0; t < n; ++t) r[t] = activate (L.act, z[t]);
                        float* acc = ar.headAcc.data() + (size_t) c * kBlock;
                        for (int t = 0; t < n; ++t) acc[t] += r[t];
                    }
                    // layer 1x1 and the residual: next input = input + W1 * activation + b1. The result goes to a scratch
                    // buffer (two of them take turns), or to the array's output for the last layer.
                    const bool last = li + 1 == ar.layers.size();
                    float* res = last ? ar.out.data() : ar.scratch ((int) (li & 1));
                    for (int c = 0; c < C; ++c)
                    {
                        float* dst = res + (size_t) c * kBlock;
                        const float* xr = x + (size_t) c * kBlock;
                        const float b = L.b1[(size_t) c];
                        for (int t = 0; t < n; ++t) dst[t] = xr[t] + b;
                        for (int j = 0; j < C; ++j)
                        {
                            const float w = L.w1[(size_t) c * (size_t) C + (size_t) j];
                            const float* src = L.act_.data() + (size_t) j * kBlock;
                            for (int t = 0; t < n; ++t) dst[t] += w * src[t];
                        }
                    }
                    // the layer's history keeps its own input for the next block
                    for (int c = 0; c < C; ++c)
                    {
                        float* h = L.xin.data() + (size_t) c * (size_t) L.stride;
                        std::memmove (h, h + n, sizeof (float) * (size_t) L.hist);
                    }
                    x = res;
                }
                // head: headOut = Wh * headAcc + bh
                for (int o = 0; o < ar.headSize; ++o)
                {
                    float* dst = ar.headOut.data() + (size_t) o * kBlock;
                    const float b = ar.headB[(size_t) o];
                    for (int t = 0; t < n; ++t) dst[t] = b;
                    for (int c = 0; c < C; ++c)
                    {
                        const float w = ar.headW[(size_t) o * (size_t) C + (size_t) c];
                        const float* src = ar.headAcc.data() + (size_t) c * kBlock;
                        for (int t = 0; t < n; ++t) dst[t] += w * src[t];
                    }
                }
                prevHead = ar.headOut.data();
                arrIn = ar.out.data();
                arrInRows = C;
            }
            const float* h = arrays.back().headOut.data();
            for (int t = 0; t < n; ++t) out[t] = h[t] * headScale;
        }

    private:
        static float activate (Act a, float v)
        {
            switch (a) { case Act::Tanh: return fastTanh (v); case Act::ReLU: return v > 0.0f ? v : 0.0f; default: return exactSigmoid (v); }
        }
        struct Layer
        {
            int C = 0, K = 0, D = 1, Z = 0, cond = 1, hist = 0, stride = 0; bool gated = false; Act act = Act::Tanh;
            std::vector<float> convW, convB, mixW, w1, b1, xin, z, act_;
        };
        struct Array
        {
            int inSize = 1, cond = 1, C = 0, headSize = 0;
            std::vector<Layer> layers;
            std::vector<float> rechW, headW, headB, headAcc, headOut, out, x0, tmpA, tmpB;
            // two scratch buffers take turns, so a layer can read its input while writing its output
            float* scratch (int which) { return (which ? tmpB : tmpA).data(); }
        };
        std::vector<Array> arrays;
        std::vector<float> cond;
        float headScale = 1.0f;
        int receptive = 1;
    };

    //==========================================================================
    class Lstm : public Network
    {
    public:
        bool build (int numLayers, int inputSize, int hidden, const std::vector<float>& w, juce::String& error)
        {
            if (inputSize != 1) { error = "only single-input models are supported"; return false; }
            H = hidden;
            size_t pos = 0;
            auto take = [&] (std::vector<float>& dst, size_t n) -> bool
            {
                if (pos + n > w.size()) return false;
                dst.assign (w.begin() + (std::ptrdiff_t) pos, w.begin() + (std::ptrdiff_t) (pos + n));
                pos += n;
                return true;
            };
            cells.clear();
            for (int l = 0; l < numLayers; ++l)
            {
                Cell c;
                c.I = l == 0 ? inputSize : hidden;
                if (! take (c.W, (size_t) 4 * (size_t) H * (size_t) (c.I + H)) || ! take (c.b, (size_t) 4 * (size_t) H)
                    || ! take (c.h0, (size_t) H) || ! take (c.c0, (size_t) H))
                { error = "the weights are shorter than the model needs"; return false; }
                c.h = c.h0; c.c = c.c0; c.gates.assign ((size_t) 4 * (size_t) H, 0.0f);
                cells.push_back (std::move (c));
            }
            if (! take (headW, (size_t) H) || ! take (headB, 1)) { error = "the weights are shorter than the model needs"; return false; }
            if (pos != w.size()) { error = "the file has more weights than the model uses"; return false; }
            return true;
        }
        void reset() override
        {
            for (auto& c : cells) { c.h = c.h0; c.c = c.c0; }
            std::vector<float> zeros ((size_t) kBlock, 0.0f), sink ((size_t) kBlock);
            for (int done = 0; done < warmUpSamples(); done += kBlock) process (zeros.data(), sink.data(), kBlock);
        }
        int warmUpSamples() const override { return 24000; }

        void process (const float* in, float* out, int n) override
        {
            for (int t = 0; t < n; ++t)
            {
                const float x = in[t];
                const float* input = &x;
                for (auto& c : cells)
                {
                    const int cols = c.I + H;
                    for (int g = 0; g < 4 * H; ++g)
                    {
                        const float* row = c.W.data() + (size_t) g * (size_t) cols;
                        float s = c.b[(size_t) g];
                        for (int i = 0; i < c.I; ++i) s += row[i] * input[i];
                        for (int i = 0; i < H; ++i) s += row[c.I + i] * c.h[(size_t) i];
                        c.gates[(size_t) g] = s;
                    }
                    for (int i = 0; i < H; ++i)
                    {
                        c.c[(size_t) i] = sigmoid (c.gates[(size_t) (H + i)]) * c.c[(size_t) i]
                                        + sigmoid (c.gates[(size_t) i]) * fastTanh (c.gates[(size_t) (2 * H + i)]);
                        c.h[(size_t) i] = sigmoid (c.gates[(size_t) (3 * H + i)]) * fastTanh (c.c[(size_t) i]);
                    }
                    input = c.h.data();
                }
                float y = headB[0];
                for (int i = 0; i < H; ++i) y += headW[(size_t) i] * cells.back().h[(size_t) i];
                out[t] = y;
            }
        }
    private:
        struct Cell { int I = 1; std::vector<float> W, b, h0, c0, h, c, gates; };
        std::vector<Cell> cells;
        std::vector<float> headW, headB;
        int H = 0;
    };

    //==========================================================================
    struct Model
    {
        Info info;
        std::unique_ptr<Network> net;
    };

    // Reads a .nam file. Returns null and fills 'error' if it cannot be used.
    inline std::unique_ptr<Model> parse (const juce::String& text, juce::String& error)
    {
        juce::var root;
        if (juce::JSON::parse (text, root).failed() || ! root.isObject())
        {
            error = "That file is not a valid .nam file (it is not readable JSON).";
            return nullptr;
        }
        const auto arch = root.getProperty ("architecture", {}).toString();
        const auto cfg  = root.getProperty ("config", {});
        const auto wv   = root.getProperty ("weights", {});
        if (arch.isEmpty() || ! cfg.isObject() || ! wv.isArray())
        {
            error = "That file does not look like a .nam model (no architecture, config or weights).";
            return nullptr;
        }
        std::vector<float> weights;
        weights.reserve ((size_t) wv.size());
        for (const auto& v : *wv.getArray())
        {
            const float f = (float) (double) v;
            if (! std::isfinite (f)) { error = "The model contains invalid numbers."; return nullptr; }
            weights.push_back (f);
        }

        auto model = std::make_unique<Model>();
        auto& info = model->info;
        info.architecture = arch;
        info.parameters = (int) weights.size();
        const auto sr = root.getProperty ("sample_rate", {});
        info.sampleRate = (sr.isDouble() || sr.isInt() || sr.isInt64()) && (double) sr > 1000.0 ? (double) sr : 48000.0;
        const auto meta = root.getProperty ("metadata", {});
        if (meta.isObject())
        {
            info.name = meta.getProperty ("name", {}).toString();
            const auto make = meta.getProperty ("gear_make", {}).toString(), mod = meta.getProperty ("gear_model", {}).toString();
            info.gear = (make + " " + mod).trim();
            const auto ld = meta.getProperty ("loudness", {});
            if (ld.isDouble() || ld.isInt()) { info.hasLoudness = true; info.loudnessDb = (float) (double) ld; }
        }

        if (arch == "WaveNet")
        {
            const auto layers = cfg.getProperty ("layers", {});
            if (! layers.isArray() || layers.size() == 0) { error = "The WaveNet model has no layers."; return nullptr; }
            if (cfg.getProperty ("head", {}).isObject()) { error = "This model uses a newer WaveNet variant that this plugin does not support yet."; return nullptr; }
            const auto inCh = cfg.getProperty ("in_channels", {});
            if ((inCh.isInt() || inCh.isDouble()) && (int) inCh != 1) { error = "Only single-input models are supported."; return nullptr; }
            static const char* known[] = { "input_size", "condition_size", "head_size", "channels", "kernel_size", "dilations", "activation", "gated", "head_bias" };
            std::vector<WaveNet::LayerSpec> specs;
            for (int i = 0; i < layers.size(); ++i)
            {
                const auto L = layers[i];
                auto* obj = L.getDynamicObject();
                if (obj == nullptr) { error = "The WaveNet layer list is malformed."; return nullptr; }
                for (auto& nv : obj->getProperties())
                {
                    bool ok = false;
                    for (auto* k : known) if (nv.name.toString() == k) ok = true;
                    if (! ok) { error = "This model uses a newer WaveNet variant (\"" + nv.name.toString() + "\") that this plugin does not support yet."; return nullptr; }
                }
                WaveNet::LayerSpec s;
                s.inputSize = (int) L.getProperty ("input_size", 0);
                s.condSize  = (int) L.getProperty ("condition_size", 0);
                s.headSize  = (int) L.getProperty ("head_size", 0);
                s.channels  = (int) L.getProperty ("channels", 0);
                s.kernel    = (int) L.getProperty ("kernel_size", 0);
                s.gated     = (bool) L.getProperty ("gated", false);
                s.headBias  = (bool) L.getProperty ("head_bias", false);
                const auto dil = L.getProperty ("dilations", {});
                if (! dil.isArray() || dil.size() == 0 || s.inputSize < 1 || s.condSize != 1 || s.headSize < 1 || s.channels < 1 || s.kernel < 1 || s.channels > 256)
                { error = "The WaveNet layer settings are not valid."; return nullptr; }
                for (int d = 0; d < dil.size(); ++d)
                {
                    const int dv = (int) dil[d];
                    if (dv < 1 || dv > 8192) { error = "The WaveNet dilations are not valid."; return nullptr; }
                    s.dilations.push_back (dv);
                }
                const auto act = L.getProperty ("activation", "Tanh").toString();
                if (act == "Tanh" || act == "Fasttanh") s.act = WaveNet::Act::Tanh;
                else if (act == "ReLU") s.act = WaveNet::Act::ReLU;
                else if (act == "Sigmoid") s.act = WaveNet::Act::Sigmoid;
                else { error = "This model uses an activation (\"" + act + "\") that this plugin does not support."; return nullptr; }
                specs.push_back (std::move (s));
            }
            auto net = std::make_unique<WaveNet>();
            if (! net->build (specs, weights, error)) { error = "The model file is damaged or unsupported: " + error + "."; return nullptr; }
            model->net = std::move (net);
        }
        else if (arch == "LSTM")
        {
            const int nl = (int) cfg.getProperty ("num_layers", 0), is = (int) cfg.getProperty ("input_size", 0), hs = (int) cfg.getProperty ("hidden_size", 0);
            if (nl < 1 || nl > 8 || is < 1 || hs < 1 || hs > 512) { error = "The LSTM settings are not valid."; return nullptr; }
            auto net = std::make_unique<Lstm>();
            if (! net->build (nl, is, hs, weights, error)) { error = "The model file is damaged or unsupported: " + error + "."; return nullptr; }
            model->net = std::move (net);
        }
        else
        {
            error = "This plugin plays WaveNet and LSTM .nam models. This one is \"" + arch + "\".";
            return nullptr;
        }
        info.receptiveField = model->net->warmUpSamples();
        return model;
    }

    inline std::unique_ptr<Model> loadFile (const juce::File& file, juce::String& error)
    {
        if (! file.existsAsFile()) { error = "The model file was not found."; return nullptr; }
        if (file.getSize() > 64 * 1024 * 1024) { error = "That file is too large to be a .nam model."; return nullptr; }
        return parse (file.loadFileAsString(), error);
    }

    //==========================================================================
    // Windowed-sinc sample rate conversion (for hosts that do not run at the model's rate, usually 48 kHz).
    class Resampler
    {
    public:
        static constexpr int kHalf = 16, kPhases = 256;
        void init (double inRate, double outRate, int maxIn)
        {
            step = inRate / outRate;
            const double fc = std::min (1.0, outRate / inRate) * 0.96;       // relative to the input's Nyquist
            table.assign ((size_t) (kPhases + 1) * 2 * kHalf, 0.0f);
            for (int p = 0; p <= kPhases; ++p)
            {
                const double f = (double) p / kPhases;
                double sum = 0.0;
                std::vector<double> row ((size_t) 2 * kHalf);
                for (int t = 0; t < 2 * kHalf; ++t)
                {
                    const double d = (double) (t - (kHalf - 1)) - f;      // distance from the sample to the output position
                    const double a = juce::MathConstants<double>::pi * fc * d;
                    const double sinc = std::abs (d) < 1.0e-9 ? 1.0 : std::sin (a) / a;
                    const double u = d / kHalf;
                    const double win = std::abs (u) >= 1.0 ? 0.0 : 0.42 + 0.5 * std::cos (juce::MathConstants<double>::pi * u) + 0.08 * std::cos (2.0 * juce::MathConstants<double>::pi * u);
                    row[(size_t) t] = fc * sinc * win;
                    sum += row[(size_t) t];
                }
                for (int t = 0; t < 2 * kHalf; ++t) table[(size_t) p * 2 * kHalf + (size_t) t] = (float) (row[(size_t) t] / sum);
            }
            buf.assign ((size_t) maxIn + 4 * kHalf, 0.0f);
            reset();
        }
        void reset() { std::fill (buf.begin(), buf.end(), 0.0f); keep = 2 * kHalf; pos = (double) kHalf; }
        // Converts n input samples; returns how many output samples were written (about n / step).
        int process (const float* in, int n, float* out, int maxOut)
        {
            std::memcpy (buf.data() + keep, in, sizeof (float) * (size_t) n);
            const int len = keep + n;
            int produced = 0;
            while (produced < maxOut)
            {
                const int i = (int) pos;
                if (i + kHalf >= len) break;
                const double f = pos - i;
                const double ph = f * kPhases;
                const int p = (int) ph;
                const float frac = (float) (ph - p);
                const float* r0 = table.data() + (size_t) p * 2 * kHalf;
                const float* r1 = r0 + 2 * kHalf;
                const float* x = buf.data() + (i - (kHalf - 1));
                float acc = 0.0f;
                for (int t = 0; t < 2 * kHalf; ++t) acc += x[t] * (r0[t] + frac * (r1[t] - r0[t]));
                out[produced++] = acc;
                pos += step;
            }
            const int consumed = std::max (0, std::min (len - 2 * kHalf, (int) pos - kHalf));
            std::memmove (buf.data(), buf.data() + consumed, sizeof (float) * (size_t) (len - consumed));
            keep = len - consumed;
            pos -= consumed;
            return produced;
        }
    private:
        std::vector<float> table, buf;
        double step = 1.0, pos = 0.0;
        int keep = 0;
    };

    //==========================================================================
    // What the plugin holds: a model plus the sample rate conversion it needs for this host.
    class Player
    {
    public:
        Player (std::unique_ptr<Model> m, double hostRate, int maxBlock) : model (std::move (m)), maxHost (std::max (64, maxBlock))
        {
            modelRate = model->info.sampleRate;
            convert = std::abs (hostRate - modelRate) > 0.5;
            if (convert)
            {
                down.init (hostRate, modelRate, maxHost);
                const int maxModel = (int) std::ceil ((double) maxHost * modelRate / hostRate) + 8;
                up.init (modelRate, hostRate, maxModel + Network::kBlock);
                low.assign ((size_t) maxModel + 16, 0.0f);
                outModel.assign ((size_t) maxModel + 16, 0.0f);
                fifo.assign ((size_t) maxHost * 2 + 64, 0.0f);
                tmpUp.assign ((size_t) maxHost * 2 + 64, 0.0f);
                fifoCount = leadIn = (int) std::ceil (hostRate / modelRate * 2.0) + 4;       // a few samples of lead-in so a block is never short
            }
            model->net->reset();
        }
        // Back to the start: the model's own state and the rate converters' history
        void reset()
        {
            model->net->reset();
            if (convert)
            {
                down.reset();  up.reset();
                std::fill (fifo.begin(), fifo.end(), 0.0f);
                fifoCount = leadIn;
            }
        }
        const Info& info() const { return model->info; }
        bool resampling() const { return convert; }
        // Extra delay in host samples added by the sample rate conversion (zero when the rates match).
        int latencyHostSamples() const { return convert ? 2 * Resampler::kHalf + 4 : 0; }

        void process (float* x, int n)
        {
            if (! convert)
            {
                for (int p = 0; p < n; p += Network::kBlock)
                {
                    const int m = std::min (Network::kBlock, n - p);
                    model->net->process (x + p, x + p, m);
                }
                return;
            }
            for (int p = 0; p < n; p += maxHost)
            {
                const int m = std::min (maxHost, n - p);
                const int k = down.process (x + p, m, low.data(), (int) low.size());
                for (int q = 0; q < k; q += Network::kBlock)
                {
                    const int c = std::min (Network::kBlock, k - q);
                    model->net->process (low.data() + q, outModel.data() + q, c);
                }
                const int u = up.process (outModel.data(), k, tmpUp.data(), (int) tmpUp.size());
                // the output FIFO makes sure exactly m samples leave, however the two conversions round
                for (int i = 0; i < u && fifoCount < (int) fifo.size(); ++i) fifo[(size_t) fifoCount++] = tmpUp[(size_t) i];
                for (int i = 0; i < m; ++i) x[p + i] = i < fifoCount ? fifo[(size_t) i] : 0.0f;
                const int used = std::min (m, fifoCount);
                std::memmove (fifo.data(), fifo.data() + used, sizeof (float) * (size_t) (fifoCount - used));
                fifoCount -= used;
            }
        }
    private:
        std::unique_ptr<Model> model;
        Resampler down, up;
        std::vector<float> low, outModel, fifo, tmpUp;
        double modelRate = 48000.0;
        int maxHost = 0, fifoCount = 0, leadIn = 0;
        bool convert = false;
    };
}
