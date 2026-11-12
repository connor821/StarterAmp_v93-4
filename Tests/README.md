# Automated tests

One program, `StarterAmpTests`, built from `tests.cpp` and the plugin sources.

    StarterAmpTests stress                          stability and robustness checks
    StarterAmpTests latch                           one check alone: a quiet note sounds the same before and after a loud passage and a restart (the power-stage feedback loop)
    StarterAmpTests poly                            the polyphonic tuner: tunings, parameters, accuracy on synthetic chords
    StarterAmpTests alias                           the anti-aliasing measurements alone, with the numbers printed (amp, pedals, boost, fuzz)
    StarterAmpTests ampswitch                      one check alone (b51): switching the amp model mid-note leaves no spike
    StarterAmpTests highgain                       one check alone (b51): Tight, Focus and Fizz Tamer act on every high-gain amp in both circuit modes
    StarterAmpTests lofidelay                      one check alone (b51): the Lo-Fi delay keeps its sample-rate reducer images out of the echoes
    StarterAmpTests delay52                        one check alone (b52): b51 delay settings unchanged, echo styles, saturation, echo EQ, Groove / Feel / Accent
    StarterAmpTests pitchdelay                     one check alone (b53): pitch of each delay repeat, timing, Pitch Mix, stability
    StarterAmpTests multiband                      one check alone (b55): four-band compressor, multiband saturator, the saturator board block, old board / compressor sessions
    StarterAmpTests workflow                       one check alone (b56): page locks, the dry path around a pedal
    StarterAmpTests optosustain                    one check alone (b58): the Opto Sustain compressor style
    StarterAmpTests stack                          one check alone (b58): the German high-gain passive stack
    StarterAmpTests transshaper                    one check alone (b53): transient shaper neutral / attack / sustain / level independence / low notes, and inside the plugin
    StarterAmpTests tubeswitch                     one check alone (b53): no spike when the power or preamp tube is changed while playing
    StarterAmpTests regress x golden.txt            compare a fixed render of every amp and voicing with golden.txt
    StarterAmpTests regress --update golden.txt     rewrite golden.txt (only after a change you meant to make)

Build: `cmake -B build -DSTARTERAMP_BUILD_TESTS=ON` then `cmake --build build --target StarterAmpTests --config Release`.
On GitHub: Actions > Tests > Run workflow (Windows). The CMake target and that workflow have NOT been built yet (b21 was
tested with the same sources built another way); the first run may need a small fix.

What `stress` checks (about 2,500 checks):
- Every amp x voicing x three gain settings, with loud, quiet and silent input: finite output, no runaway, silence settles below -50 dB.
- Block sizes 1 to 4096 (including odd ones): finite, and the level stays within 1 dB of the 512-sample render.
- Random parameter automation (all parameters, six changes per block, three seeds): finite output.
- Save and load: every parameter comes back (an old Wobble setting is converted to a swell pedal by design, so it is skipped).
- Lifecycle: prepare at 22.05 to 192 kHz and 32 to 2048 samples, release, and change sample rate on a live instance.
- Amp Quality modes: finite, reported latency, level close between modes.
- Latch: a quiet note must sound the same (within 3 dB) fresh, after a riff and a transport restart, and after a loud tone and a restart, for Hi-Wat Linked at three gains, Cedar Twelve Big Spring and a Brown Eye voicing.
- Switching: each on/off switch toggled during a sustained note; reports the largest sample jump (a click detector, not a listening test).

What `regress` checks: a fixed synthetic plucked-note signal through every amp and voicing (gain 0.5, Hybrid mode, cab off); level within 0.3 dB
and the eight octave bands (63 Hz to 8 kHz, relative to 1 kHz) within 0.6 dB of golden.txt. When you retune an amp on purpose, the matching rows
fail; check that only those rows moved, then update the file.

Allocations on the audio thread are covered for the main paths by `b91` (it counts calls to operator new while processing; b102 adds the NAM
two-model pedal-slot toggle). Not covered: race conditions between the audio and message threads (needs ThreadSanitizer, which has not been run),
the full parameter-by-parameter allocation matrix, IR and NAM loading under audio, real host behaviour, listening.

Tools: `tools/fitcapture.cpp` ranks the amps against a capture (see tools/README.md); it is not part of the CMake build yet.
