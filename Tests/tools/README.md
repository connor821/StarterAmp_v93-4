# fitcapture

Command-line tool: takes an amp capture (amp only, no cab) and ranks every electric amp x voicing in the plug-in by how close it can get, then says whether an existing amp is close or a new amp/voicing may be justified. It links against the plug-in sources and changes nothing in them.

## Inputs
    fitcapture --nam capture.nam                     built-in test signals (steady 196 Hz sine + plucked notes, six levels)
    fitcapture --nam capture.nam --di dry.wav        same, then the best matches are cross-checked on your own DI
    fitcapture --di dry.wav --out recorded.wav       capture = recorded output of that DI (same length, aligned; any rate; mono/stereo)
    fitcapture --list | --build-lib | --capture-amp M:V:G (self test) | --render amp:M:V:G|nam:FILE --di F --write F
Options: `--label`, `--knobs "text"` (only echoed), `--only 13,24:2,modern` (models by number or name part, optional `:voicing` 1-4),
`--lib FILE`, `--share 21=0.5` (b121: try a Hybrid stage-shape share for a model; use a separate `--lib` per value), `--jobs N`, `--refine N`, `--top N`, `--csv FILE`, `--modes 0,1,2`, `--quality N`, `--di-start/--di-sec/--screen-sec`, `--no-offset`. `--help` lists all.

## What is measured
Per candidate (every amp except the acoustic one, Triple Blend included; 4 voicings; Gain 0.2/0.35/0.5/0.65/0.8; auto level off; every `...On` switch off, i.e. pedals, cab, effects, second amp; gate off; output 0 dB), at six input levels (-50 to -10 dB rms, 8 dB steps):
output level of the plucked notes (gain-vs-level curve), H2 and H3 re the fundamental of a steady 196 Hz sine (floor -60 dB), and the 63 Hz to 8 kHz octave-band shape at -26 dB.
Score = (3 x gain + 2 x H2 + 2 x H3 + 1 x band) / 8, in dB. Gain error is the RMS curve error after removing a constant output offset (the capture's loudness is arbitrary; the offset is printed as "out trim"). Band error removes overall level.
The best Gain setting per amp/voicing is kept; the best few are then re-measured at Gain values in between (`--refine`) because five settings are coarse.
Level offset: the capture is also measured on a 2 dB level grid so a free input-level offset (+-12 dB) can be searched; positive = the amp needs that much more drive than its Gain allows.
Verdict: "an existing amp is close: <name voicing>" when a candidate has gain error < 2 dB and H2, H3 errors < 3 dB; otherwise "no existing amp within tolerance: a new amp or voicing may be justified", with the failing components (gain structure / gain range, even-odd balance, tone) and whether a free level offset would fix it.
With `--di` + `--out` the same quantities are taken from short windows of the DI (levels binned by window level; H2/H3 from windows with a clear pitch, re that detected pitch), a first pass on an 8 s excerpt, a second pass on the best 24 over the full excerpt; level offsets are searched for the best five only.

## Library (fast re-use)
The candidate measurements do not depend on the capture, so with built-in signals they are stored in `fitcapture_lib.tsv` (or `--lib`). First build: all 660 runs. Later captures only need the capture's own measurement. A library made with other signals is rebuilt; three stored rows are re-measured on each use and the library is rebuilt if they no longer match the plug-in. **Rebuild (delete the file) after any change to the amp code**; the stale check was not itself tested.

## Build
Same recipe as the tests: a console app made of `PluginProcessor.cpp`, `PluginEditor.cpp` and `fitcapture.cpp`. Do not define STARTERAMP_TUNE.

CMake (an example to add next to the StarterAmpTests target; NOT in CMakeLists.txt and NOT built here):
```
juce_add_console_app(fitcapture PRODUCT_NAME "fitcapture")
target_sources(fitcapture PRIVATE Tests/tools/fitcapture.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(fitcapture PRIVATE Source)
target_compile_definitions(fitcapture PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_USE_FLAC=0 JUCE_USE_OGGVORBIS=0
    JUCE_USE_MP3AUDIOFORMAT=0 JUCE_USE_LAME_AUDIO_FORMAT=0 JUCE_DISABLE_JUCE_VERSION_PRINTING=1)
target_link_libraries(fitcapture PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
```
g++ as actually used (Linux, JUCE 8 modules in `$J`; the module objects are `#include <juce_X/juce_X.cpp>` files compiled with the same defines, plus SheenBidi, as in the other harnesses):
```
g++ -std=c++17 -O2 -w -c Source/PluginProcessor.cpp -o PluginProcessor.o -I$J -ISource $DEFS
g++ -std=c++17 -O2 -w -c Source/PluginEditor.cpp    -o PluginEditor.o    -I$J -ISource $DEFS
g++ -std=c++17 -O2 -w -c Tests/tools/fitcapture.cpp -o fitcapture.o      -I$J -ISource $DEFS
g++ PluginProcessor.o PluginEditor.o fitcapture.o <juce module objects> -o fitcapture -lharfbuzz -lfreetype -lX11 -lXext -lfontconfig -ldl -lpthread
```
`$DEFS` = `-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1 -DJUCE_USE_CURL=0 -DJUCE_WEB_BROWSER=0 -DNDEBUG=1 -DJUCE_MODULE_AVAILABLE_juce_{core,events,graphics,data_structures,gui_basics,audio_basics,audio_formats,audio_processors,dsp}=1 -DJUCE_USE_FLAC=0 -DJUCE_USE_OGGVORBIS=0 -DJUCE_USE_MP3AUDIOFORMAT=0 -DJUCE_USE_LAME_AUDIO_FORMAT=0 -DJUCE_PLUGINHOST_VST3=0 -DJUCE_PLUGINHOST_AU=0 -DJUCE_PLUGINHOST_LADSPA=0 -DJUCE_PLUGINHOST_LV2=0 -DJUCE_PLUGINHOST_VST=0 -DJUCE_DISABLE_JUCE_VERSION_PRINTING=1`.

## Run time (measured, shared 2-core machine)
Library: 622 s wall with `--jobs 2` (930 s CPU, so about 15 min single process). Capture + ranking from the library: self test 15 s without refinement; a NAM capture 2 to 4 minutes (about 60 s measuring the capture, the rest Gain refinement; `--refine 0` skips it). DI + recording mode: about 6.5 min with `--jobs 2`.
Each run uses a fresh processor: `prepareToPlay` does not clear all state (a reused instance differed from a fresh one by up to 0.5 in peak sample value in 8 of 14 probes), and construction costs about 25 ms.

## Limitations
Not eight threads: runs use separate processes (`--jobs N`), the machine here has 2 cores. Hybrid amp mode (plug-in default) only unless `--modes`. Candidate Gain refinement is local. Synthetic sine/pluck signals are not a playing performance; H2/H3 of a NAM model at -50 dB can be model noise. Scores of neighbouring candidates often differ by under 0.5 dB: read the top few as a group. See VALIDATION.md.
