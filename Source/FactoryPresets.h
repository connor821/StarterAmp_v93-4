// Generated factory preset data. Do not edit by hand.
#pragma once
#include <juce_core/juce_core.h>

namespace factory
{
struct Entry { const char* id; float value; };
struct Preset { const char* category; const char* name; const Entry* entries; int numEntries; };

// b60: the built-in presets were removed (they were from versions long out of date). The list is empty on purpose: the menu,
// the browser's Factory tab, the < > buttons and Program Change all cope with no entries. To add built-in presets again,
// put entries in kPresets (a one-entry placeholder keeps the array legal C++) and set kNumPresets to the real count.
static const Preset kPresets[] = { { "", "", nullptr, 0 } };
static const int kNumPresets = 0;
}
