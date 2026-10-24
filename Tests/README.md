# Automated tests

One program, `StarterAmpTests`, built from `tests.cpp` and the plugin sources.

    StarterAmpTests stress                          stability and robustness checks
    StarterAmpTests regress x golden.txt            compare a fixed render of every amp and voicing with golden.txt
    StarterAmpTests regress --update golden.txt     rewrite golden.txt (only after a change you meant to make)

Build: `cmake -B build -DSTARTERAMP_BUILD_TESTS=ON` then `cmake --build build --target StarterAmpTests --config Release`.
On GitHub: Actions > Tests > Run workflow (Windows). The CMake target and that workflow have NOT been built yet (b21 was
tested with the same sources built another way); the first run may need a small fix.

What `stress` checks (about 1,400 checks):
- Every amp x voicing x three gain settings, with loud, quiet and silent input: finite output, no runaway, silence settles below -50 dB.
- Block sizes 1 to 4096 (including odd ones): finite, and the level stays within 1 dB of the 512-sample render.
- Random parameter automation (all parameters, six changes per block, three seeds): finite output.
- Save and load: every parameter comes back (an old Wobble setting is converted to a swell pedal by design, so it is skipped).
- Lifecycle: prepare at 22.05 to 192 kHz and 32 to 2048 samples, release, and change sample rate on a live instance.
- Amp Quality modes: finite, reported latency, level close between modes.
- Switching: each on/off switch toggled during a sustained note; reports the largest sample jump (a click detector, not a listening test).

What `regress` checks: a fixed synthetic plucked-note signal through every amp and voicing (gain 0.5, Hybrid mode, cab off); level within 0.3 dB
and the eight octave bands (63 Hz to 8 kHz, relative to 1 kHz) within 0.6 dB of golden.txt. When you retune an amp on purpose, the matching rows
fail; check that only those rows moved, then update the file.

Not covered: allocations on the audio thread (the separate `rt` harness did this in earlier builds; not in this folder yet), race conditions between the
audio and message threads (needs ThreadSanitizer), IR and NAM loading under audio, real host behaviour, listening.
