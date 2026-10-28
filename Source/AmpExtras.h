#pragma once
// b65: extra voicings beyond the four every amp has. Voicing positions 5, 6, 7 (indices 4, 5, 6) exist only for the amps listed here;
// every other amp still holds at its fourth. The order of the names within one amp is the order of its positions.
// The numbers behind each one live in PluginProcessor.cpp (kExtraRows, the Auto Level curves and the Circuit-derived component values).
namespace AmpExtras
{
    struct Item { int model; const char* name; };

    constexpr int kMaxVoicings = 7;        // the Voicing Switch parameter's number of positions

    inline const Item* table (int& n)
    {
        static const Item t[] =
        {
            { 30, "Boost" },                    // Cedar Spring: b63 added Quad Ten here; b67 moved it to Cedar Ten (as the original, voicing 104) and put the silverface Volume Boost here
            { 11, "IV Lead" },                  // Marque Dream: b65 added Mark I here; b67 moved it to Marque One (as the original, voicing 104) and put the Mark IV lead channel here
            { 12, "Model T" },                  // b65 Citrus Doom: the 1970s 200 W bass head, symmetric and feedback-tamed
            { 24, "Clean 100" },                // b65 Matt's Amp: the low-gain, dark clean-to-crunch end of the Matamp captures
            { 24, "Tight Voice" },              // b65 Matt's Amp: the 100 W head's high bass-cut selector positions
            { 3, "Clean" },                     // b66 Slow Burn: the SLO-100's Clean channel (two low-gain stages with an unbypassed cathode, Crunch/Clean divider, clean master)
        };
        n = (int) (sizeof (t) / sizeof (t[0]));
        return t;
    }

    inline int count (int model)
    {
        int n = 0, c = 0;
        const Item* t = table (n);
        for (int i = 0; i < n; ++i) if (t[i].model == model) ++c;
        return c;
    }

    inline const char* name (int model, int k)
    {
        int n = 0, c = 0;
        const Item* t = table (n);
        for (int i = 0; i < n; ++i)
            if (t[i].model == model) { if (c == k) return t[i].name; ++c; }
        return "";
    }

    // the highest valid voicing index of an amp (3 for most amps)
    inline int lastVoicing (int model) { return 3 + count (model); }
}
