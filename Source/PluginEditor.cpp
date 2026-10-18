#include "PluginEditor.h"
#include "IrLibraryPanel.h"
#include <array>

namespace
{
    // Hover text for every control, by parameter ID. Cab block controls (cabSpkA..D and so on) share the
    // "blk_" entries; the nine EQ faders (eqB1..9) share "eqBand".
    const std::map<std::string, const char*>& tooltipTable()
    {
        static const std::map<std::string, const char*> table
        {
            { "inputMode", "Which input channel feeds the plugin. Left or Right uses just that channel (a guitar plugged into one side of an interface); Mono sums both. The result goes to both sides." },
            { "inGain", "Input level (dB) before everything else. Set it so your guitar is strong but not clipping; the gate, pedals and amp all react to it." },
            { "gate", "Noise gate threshold (dB). Signal quieter than this is turned down, which cuts noise between notes. Raise it to gate more; too high will chop quiet notes." },
            { "deNoiseOn", "Pickup de-noise: removes mains hum with notch filters and closes a low-pass in the gaps between notes to hide hiss. It sits before the gate." },
            { "deNoiseHz", "The mains frequency where you live (50 or 60 Hz), so the hum notches line up with your hum." },
            { "deNoiseHum", "How much of the hum (and its harmonics) is removed. Too much can thin the tone slightly." },
            { "deNoiseHiss", "How hard the hiss filter closes between notes. 0 = off. Higher closes down to about 1.5 kHz in the gaps." },
            { "noiseOn", "Master switch for the Noise pedal. When off, none of its footswitches do anything." },
            { "noiseOct", "Octave up while held (or latched on). The pedal is a pitch shifter for harsh, dissonant sounds." },
            { "noiseOct2", "Two octaves up while held (or latched on). Wins over OCT UP if both are on." },
            { "noiseGross", "Adds clashing pitch-shifted voices above your note (the scene menu picks which). Hold or latch it on." },
            { "noiseRing", "Ring modulator on while held or latched: replaces your harmonics with metallic sum and difference tones." },
            { "noiseOctLatch", "Off: the OCT UP switch is on only while held. On: click once to turn it on, again to turn it off." },
            { "noiseOct2Latch", "Off: the 2 OCT UP switch is on only while held. On: click once to turn it on, again to turn it off." },
            { "noiseGrossLatch", "Off: the CLASH switch is on only while held. On: click once to turn it on, again to turn it off." },
            { "noiseRingLatch", "Off: the RING switch is on only while held. On: click once to turn it on, again to turn it off." },
            { "noiseRise", "Glide time. For the octaves: how long the pitch slides up and down. For Clash and Ring Mod: how fast they fade in and out. Low = instant, high = a slow swell." },
            { "noiseSpeed", "Rate of the wobble and random jumps: the octave wobble, the Clash detune, and the ring mod's Wobble." },
            { "noisePanic", "Octave pages: depth of a smooth, warbling pitch wobble (up to about 1.5 semitones)." },
            { "noiseChaos", "Octave pages: depth of random, stepped pitch jumps (up to about 7 semitones) for glitchy, dissonant sounds." },
            { "noiseGrossScene", "Which clashing intervals Clash uses: Bright (tritone + flat ninth), Dark (minor second + tritone), Full (all three, maximum dissonance)." },
            { "noiseNuke", "How hard the signal is pushed into soft clipping before the Clash pitch engine. Higher = more aggressive and messier." },
            { "noiseSicken", "Clash wet/dry mix. 0 = only your dry signal, 1 = only the Clash voices." },
            { "ringMode", "Ring = plain multiplication (original notes mostly disappear). AM = amplitude modulation (original notes stay, with a shimmering layer)." },
            { "ringWave", "Shape of the ring modulator's carrier: Sine is cleanest, Square buzzier, Triangle in between." },
            { "ringFreq", "Carrier frequency, 1 Hz to 5 kHz. About 1-20 Hz is a tremolo chop, 50-500 Hz growl and metal, higher gives bells." },
            { "ringMix", "Ring modulator wet/dry. 0 = only your dry signal." },
            { "ringWobble", "Moves the carrier up and down by up to an octave, at the Speed rate. 0 = steady." },
            { "fuzzOn", "Fuzz pedal on/off. It sits before the compressor, overdrive and amp." },
            { "fuzzStyle", "Wall (thick, sustained wall of sound), Tight (tight, aggressive, mid-forward), Gated (old, replaced by Factory), Buzzsaw (a heavy-metal pedal circuit: two gain stages, clipper, and a Low / High tone section with scooped mids), Face (round, two-transistor, cleans up with the guitar's volume), Bender (raspy, two or three transistors) Factory (gated, with Comp, that can squeal), Nuke (a wave-shaper fuzz: Gain drives the signal through a curve you morph with Shape, with optional bit and sample-rate reduction from Crush) or Crusher (a bit-crusher: Bits, Rate, Gain and Tone). Face, Bender and Factory have a Germanium / silicon switch. Hand-built approximations, not circuit models." },
            { "fuzzGain", "Amount of fuzz. Wall: sustain. Tight: distortion. Gated: drive. Buzzsaw: distortion. Face and Bender: the fuzz amount (the Face's Fuzz knob, the Bender's Attack). Factory: drive." },
            { "fuzzTone", "0 = dark, 1 = bright (the same direction for all styles). Buzzsaw has no Tone knob (it has Low and High). On Wall it is the Muff tone control (a scooped middle). On Face, Bender and Factory it is a top-end roll-off." },
            { "fuzzLevel", "Output level of the fuzz pedal (dB). Fuzz can be loud, so turn this down if the amp is overloaded." },
            { "fuzzGate", "Gated and Factory styles. Higher = the stage is starved, so notes cut off and sputter as they die away." },
            { "perfMode", "Normal uses the quality menus as you set them. Low Latency turns off oversampling and shortens the transpose window for the least delay and CPU. High Quality uses 4x oversampling, the smooth transpose window and a longer limiter look-ahead, for the cleanest sound at the highest CPU." },
            { "limOn", "Final limiter on/off. It sits after the Output level and stops the signal going over the ceiling, to avoid clipping. With look-ahead it adds 1 ms of delay (2 ms in High Quality, none in Low Latency), which is reported to your DAW." },
            { "limCeil", "The highest level the limiter lets through (dB). It limits sample peaks, not true peaks, so leave a little headroom (-1 dB is a safe start)." },
            { "limRel", "How fast the limiter lets go after a peak (ms). Short = louder but can sound pumpy or gritty; long = smoother." },
            { "shimmerMode", "What gets pitch-shifted. Feedback: the reverb's own tail, so the shimmer blooms and climbs with every trip around the loop. Input: only your playing, shifted once into the reverb, for a clean octave pad that does not build up. Both: the two together." },
            { "shimmerDetune", "Splits the shimmer into two voices a few cents either side of the pitch, each drifting slowly. Adds width and a choir-like softness, and hides the grain of the pitch shifter. 0 = one steady voice." },
            { "buzzLow", "Buzzsaw style only. The low band, about -20 to +20 dB around 90 Hz." },
            { "buzzHigh", "Buzzsaw style only. The high band, about -20 to +20 dB around 1 kHz and up. (The original has one knob for both mids and highs; this circuit splits it into Mid and High, as a modded version does. Set them equal for something close to the original.)" },
            { "buzzMid", "Buzzsaw style only. The upper-mid band, about -20 to +20 dB around 1.3 kHz. Split out of the original single tone knob (see High)." },
            { "buzzSoft", "Buzzsaw style only. The soft clipper: two diodes in the loop of the first op-amp. On is the original circuit; off leaves that stage clipping only at its supply limit, which is harder and louder." },
            { "buzzHard", "Buzzsaw style only. The hard clipper: two silicon diodes to ground after the germanium pair. On is the original circuit; off leaves only the germanium pair, which is softer and a little quieter." },
            { "buzzBlend", "Buzzsaw style only. Mixes in a clean copy of your input (twice as loud as the input, as in the modded circuit). 0 = all distortion. Keeps the pick attack and low end clear under the heavy part." },
            { "buzzPol", "Buzzsaw style only. Flips the clean copy upside down. With Blend up, this changes which frequencies add and which cancel against the distorted signal." },
            { "fuzzGe", "Face, Bender and Factory styles. On = germanium: soft, warm, round, with a lot of lopsided clipping and a leaky, sputtery feel. Off = silicon: tighter, brighter, harder-edged, with more gain and a less compressed sound." },
            { "fuzzStarve", "Face and Bender styles. A weak, dying battery: less headroom, a gap at the zero crossing, and a gated, sputtering, spitty sound. 0 is a fresh battery." },
            { "fuzzInput", "Face, Bender and Factory styles. The volume knob on your guitar: turn it down and the fuzz cleans up, the way a fuzz pedal reacts to your guitar. 1 is full." },
            { "fuzzComp", "Factory style only. Compression ahead of the fuzz: higher squashes the input, so quiet notes get louder and the sound sustains more." },
            { "fuzzShape", "Nuke: moves through five wave-shaping curves (soft, sine-fold, odd-harmonic, staircase, asymmetric fold), blending neighbours." },
            { "fuzzCrush", "Nuke: bit depth and sample rate reduced together, from clean (0) to crushed (1)." },
            { "fuzzBits", "Crusher: bit depth, from 16 (clean) down to 1 (a harsh square-ish buzz)." },
            { "fuzzRate", "Crusher: sample-rate reduction, from none (0) down to about 1/32 (1). More makes more aliasing, metallic and gritty." },
            { "fuzzBenderModel", "Bender style only. Early: two transistors, thick, woolly mids. Classic: three transistors, raspy and cutting. Late: three transistors with a tone sweep, tighter low end and more fizz." },
            { "fuzzBuffer", "Face, Bender and Factory styles. A buffer in front, so the pickup is not loaded: a brighter, tighter, clearer sound. Off is the authentic woolly, pickup-loaded sound (the top end is softer and the fuzz reacts more to your guitar)." },
            { "fuzzMuffVer", "Wall style only. Standard (the original sound), Smooth (deeper mid scoop, a smoother top), Gritty (thick, a bit rough, more low end), Mid-forward (more bite and mids, tighter bass) or Heavy (more gain, fuzzy huge lows)." },
            { "fuzzMuffDiode", "Wall style only. The clipping diodes: Silicon (the original), LED (more headroom, louder, more open, less compression) or Germanium (clips early, softer, more compressed)." },
            { "fuzzMuffOpAmp", "Wall style only. Op-amp mode: the stages run out of swing and then clip hard, for a tighter, brighter, harder, more modern fuzz." },
            { "fuzzMuffTonePass", "Wall style only. Bypasses the tone control for the full, unfiltered fuzz." },
            { "fuzzMuffMids", "Wall style only. The middle of the sound: 0.5 is the original. Lower scoops the mids further, higher fills them back in for a more forward fuzz." },
            { "fuzzDrift", "Temperature drift: the gain and bias slowly wander, the way a germanium fuzz changes as it warms up. 0 is rock steady." },
            { "fuzzHiss", "Transistor hiss in front of the fuzz. It gets louder as you turn the fuzz up. 0 is silent." },
            { "fuzzOctave", "An octave-up rectifier in front of the fuzz (full-wave rectified): a sputtering, ringing octave-up fuzz. Works best on single notes, high on the neck." },
            { "fuzzStab", "Gated and Factory styles. 1 = stable; lower = more feedback, giving squeals and howls as notes decay." },
            { "compOn", "Compressor on/off. It sits after the noise and fuzz pedals and before the amp." },
            { "compThresh", "Level (dB) above which the compressor works. Lower = more compression." },
            { "compRatio", "How hard it squeezes: 1:1 (saturation only) up to 20:1, 10:1 Soft (slow-release, light-controlled feel) and Limit (a hard limiter)." },
            { "compAttack", "How fast it reacts (0.05-50 ms). Short catches the pick attack; longer lets it through." },
            { "compRelease", "How fast the gain recovers (50 ms-3.5 s)." },
            { "compOutput", "Makeup gain (dB) to bring the level back up after compressing." },
            { "compMix", "Dry/wet blend for parallel compression. 1 = all compressed." },
            { "compDetHp", "High-pass on what the compressor listens to (Off, 80, 160, 320 Hz), so low palm mutes don't pump the whole sound." },
            { "compSat", "Gentle saturation that grows with compression: tube-like (even harmonics), tape-like (odd harmonics), both, Transformer (low end only), or one of the four voiced for the compressor styles: FET stage, Opto tube stage, Mu iron + tube, Diode." },
            { "compStyle", "The kind of compressor. Standard is the original. FET is fast and punchy, Opto is smooth and slow to let go, Variable-mu is gentle glue, Sustainer is a pedal-style squeeze that lets the pick through and holds notes, Bus is light glue for chords." },
            { "compBloom", "Makes the main compressor work harder and let go more slowly, so notes squeeze down and then swell back up (up to 2.5x the gain reduction and 3x the release). 0 = off." },
            { "compSmack", "Level of a second, parallel compressor that runs alongside the main one. It is fast, heavy and transparent, with the lows lifted, so it adds low-end impact and thickens quiet notes without squashing the attack. 0 = off." },
            { "compClarity", "A tilt EQ on the compressor's output. Turn right for more highs and less lows (clarity, bite); turn left for warmth and thickness. 0 = flat." },
            { "wahOn", "Wah pedal on/off. It sits after the compressor and before the phaser, chorus, overdrive and amp." },
            { "wahMode", "Manual: you set the Position (automate it from your DAW). Auto: Envelope: playing harder opens the wah. Auto: LFO: it sweeps by itself." },
            { "wahPos", "Filter position, 0 = heel (low, bassy) to 1 = toe (high, trebly). In Envelope mode it's the resting position; in LFO mode it's the centre of the sweep." },
            { "wahVoice", "Generic uses the Heel and Toe knobs. Classic circuit fixes the sweep at 450 Hz to 1.6 kHz, the range given in the published analysis of a classic wah; Heel and Toe are then ignored." },
            { "wahHeel", "Lowest centre frequency of the wah sweep." },
            { "wahToe", "Highest centre frequency of the wah sweep." },
            { "wahQ", "Resonance: how sharp and vocal the wah peak is. Higher = more nasal and pointed." },
            { "wahMix", "Wet/dry. 1 = fully wah; lower blends in your dry signal for a subtler effect." },
            { "wahSens", "Envelope mode: how strongly your playing level opens the wah." },
            { "wahDecay", "Envelope mode: how fast the wah closes again after you play (ms)." },
            { "wahRate", "LFO mode: speed of the automatic sweep (Hz)." },
            { "wahDepth", "LFO mode: width of the automatic sweep." },
            { "phaserOn", "Phaser on/off. It sits after the wah and before the chorus and amp." },
            { "phaserStages", "Number of all-pass stages (2-12). More stages = more notches and a deeper, more complex sweep." },
            { "phaserShape", "Shape of the sweep: sine, triangle or random." },
            { "phaserRate", "Speed of the sweep (Hz)." },
            { "phaserDepth", "How far the sweep travels." },
            { "phaserCenter", "Centre frequency the sweep moves around." },
            { "phaserFeedback", "Feeds the output back in for a sharper, more resonant sweep. Negative values give a hollower sound." },
            { "phaserMix", "Wet/dry blend. About 0.5 is the classic phaser sound." },
            { "phaserEnv", "How much your playing level also moves the sweep (envelope follower)." },
            { "chorusOn", "Chorus on/off. It sits after the phaser, before the overdrive and amp." },
            { "chorusMode", "Chorus voicing. I, II and I+II are the classic modes (I+II is a fast, vibrato-like shimmer). Dimension, Tri-chorus, Ensemble and Detune are the wider, slower voicings." },
            { "chorusMix", "Wet/dry blend of the chorus." },
            { "chorusRate", "Multiplies the speed of the mode's built-in sweep." },
            { "chorusDepth", "Multiplies how far the mode's sweep moves the pitch." },
            { "chorusTone", "Low-pass on the chorus signal (Hz). Lower = darker, warmer." },
            { "chorusHiss", "Adds the faint hiss of the analog circuit. 0 = silent." },
            { "preDelayOn", "Pre-amp delay on/off. Its repeats go through the overdrive and amp, so they get compressed and distorted." },
            { "postDelayOn", "Post-amp delay on/off. Its repeats are clean copies of the amped sound, before the reverb." },
            { "preDelayNote", "Free (ms) uses the Time knob. A note value sets the delay time from the tempo instead (and greys out Time)." },
            { "postDelayNote", "Free (ms) uses the Time knob. A note value sets the delay time from the tempo instead (and greys out Time)." },
            { "preDelayMix", "How much of the repeats you hear. 0 = off." },
            { "postDelayMix", "How much of the repeats you hear. 0 = off." },
            { "preDelayTime", "Delay time in milliseconds (used when Note is Free)." },
            { "postDelayTime", "Delay time in milliseconds (used when Note is Free)." },
            { "preDelayFeedback", "How many repeats. Keep it below about 0.8 for a clean decay." },
            { "postDelayFeedback", "How many repeats. Keep it below about 0.8 for a clean decay." },
            { "preDelayLofi", "Darker, crunchier, more degraded repeats (lower sample rate and bit depth). Only affects the repeats." },
            { "postDelayLofi", "Darker, crunchier, more degraded repeats (lower sample rate and bit depth). Only affects the repeats." },
            { "preDelayWarble", "Tape-style pitch wobble on the repeats: a slow wow, a faster flutter and a little drift." },
            { "postDelayStereo", "How the repeats are spread across the stereo field. Stereo: both sides echo together. Ping-Pong: the repeats bounce between left and right. Dual: the right side runs at a different time to the left. Needs a stereo signal; the pre-amp delay is always mono." },
            { "postDelayOffset", "Dual mode only. How much longer the right delay is than the left: 0 = the same, 0.5 = 1.5x (a dotted feel against the left), 1 = 2x." },
            { "postDelaySpread", "How far apart the left and right repeats are. 1 = hard apart, 0 = both in the centre. Useful for a softer ping-pong." },
            { "postDelayWarble", "Tape-style pitch wobble on the repeats: a slow wow, a faster flutter and a little drift." },
            { "tempoBpm", "The tempo used by note-value delays and the tremolo. TAP to set it by tapping." },
            { "tempoHost", "On: follow your DAW's tempo when it reports one. Off: use the BPM box (or TAP)." },
            { "metroOn", "Starts the metronome." },
            { "metroBeats", "Beats in a bar (1 to 12)." },
            { "metroSub", "Extra quieter clicks between the beats: none, eighth notes, triplets or sixteenth notes." },
            { "metroSound", "The click sound: a sharp Click, a Wood block, or a soft Beep." },
            { "metroVol", "How loud the metronome is, in dB." },
            { "metroAccent", "Plays the first beat of each bar higher and louder." },
            { "tunerRef", "The tuner's reference pitch for A (427 to 453 Hz). 440 is the standard." },
            { "fuzz2On", "Fuzz 2 on/off. A second, separate copy of the fuzz block: it can stack after Fuzz 1 or run beside it (see Route). It runs in the Fuzz slot of the effect order, straight after Fuzz 1." },
            { "fuzz2Route", "Series: Fuzz 1 feeds Fuzz 2 (stacked fuzzes). Parallel: both fuzzes get the same input and their outputs are added (each at -3 dB); two fuzzes added together can partly cancel, which sounds hollow. Only applies when both are on; with one on it just runs. Real fuzzes depend on what drives them, so a fuzz behind another fuzz is a model of the circuit, not a promise of what the real pedals do together." },
            { "od2On", "Drive 2 on/off. A second, separate copy of the drive block: it can stack after Drive 1 or run beside it (see Route)." },
            { "od2Route", "Series: Drive 1 feeds Drive 2, which feeds the amp (stacked pedals). Parallel: both drives get the same input and their outputs are added (each at -3 dB). Series and Parallel only apply when both drives are on; with one on, it simply feeds the amp. Using both adds the oversampling delay of one extra pass." },
            { "odOn", "Overdrive pedal on/off. It sits before the amp and pushes its first stage." },
            { "odStyle", "Transparent (a Centaur-type blend drive), Tight (a modern metal-style boost with a gate), Open (a dynamic, amp-like overdrive with a range switch), Screamer (a mid-hump drive with clipping, voicing, Bass and Mid controls), Distortion (a hard-clipped distortion with a clipping choice), Tube Driver (a full-range, warm, creamy tube-style drive), Op-Amp Distortion or Stack Drive. All except Open and Tube Driver are circuit models from published component values; where the analyses leave values out the numbers are my estimates (see MODELED_GEAR)." },
            { "odClip", "Screamer and Distortion (and Tight in Circuit mode): what clips the signal. 'None' (circuit mode only) lets the op-amp run into its own supply limits instead of diodes. Silicon is the classic sound; LED has more headroom (louder, more open); Germanium clips early and soft; Asymmetric clips the two halves differently, which adds even harmonics and a more amp-like feel." },
            { "odVoice", "Screamer only. Classic: the familiar mid hump and gain. Mild: leaner, with less gain and no hump. Open: brighter, with more gain and a looser low end." },
            { "odBass", "Screamer only: how much low end goes into the clipper. Low = tight and mid-focused (the classic sound); high = fuller and thicker." },
            { "odMid", "Screamer only: cuts or boosts the mid hump before the clipper. Centre = the voicing's own hump." },
            { "odMidFreq", "Screamer only: where the mid control works, from about 350 Hz (honky, vocal) up to about 2 kHz (nasal, cutting)." },
            { "odMidMod", "Distortion only: Scoop is the stock sound (a mid dip, scooped and fizzy), Flat leaves the mids alone, Boost adds a mid hump for a thicker, more lead-like tone." },
            { "odRange", "Open style only. High: more gain and a fuller, more compressed drive. Low: lighter, with more headroom and a clearer, more dynamic feel." },
            { "odGain", "Amount of drive. Transparent and Screamer blend a clean path with the gain path; Tight and Distortion send almost everything through the gain path." },
            { "odTone", "Treble control after the clipper (darker to brighter). On the Distortion it tilts the highs around 1.2 kHz." },
            { "odEdge", "Transparent, Screamer and Distortion styles: extra treble fed into the clipper for more bite (shown as Bright on the Screamer and Distortion)." },
            { "odAttack", "Tight style: positions 1-6. Higher tightens the low end more and adds more bite around 750 Hz." },
            { "odGate", "Tight style: a noise gate ahead of the gain, for chopped palm mutes. 0 = off. Too high cuts quiet notes." },
            { "odVolume", "Output level of the overdrive (dB). A boost into the amp can be much louder than the amp alone." },
            { "odGrip", "Transparent style: a tighter, more present voicing. In the hand-tuned version the low cut ahead of the clipper moves from 120 Hz up to 240 Hz and the Edge shelf starts lower (1.5 kHz) with 1.5 dB extra. In Circuit Mode it is a -3 dB cut around 330 Hz and +1 dB above 1.2 kHz, as the Grip switch on the capture it was matched to." },
            { "odMix", "Blends the drive with your unprocessed input (before the drive's Volume). 1 = all drive. Lower keeps your clean attack and low end under the drive." },
            { "odCircuit", "Transparent, Tight, Screamer and Distortion: off = the original hand-tuned voicings (default). On = the circuit models (the real resistor, capacitor and diode values, solved sample by sample). Grip works in both modes (a different recipe in each); the Clipping menu on Tight only works in circuit mode. The other pedals are always circuit models." },
            { "ampMode", "Amp mode. Classic is the original voicing (default). Circuit-derived uses clipping curves from tube-stage simulations and works the power section, supply droop and preamp corners out from the circuit values." },
            { "ampBMode", "Amp B mode: Classic (the original voicing), or Circuit-derived (clipping curves and power section worked out from the circuit)." },
            { "ampModel", "Which amp. The menu is sorted into folders by gain level: Clean, Low / mid gain and High gain (Acoustic is at the top). Amps: Clean (sparkly), Vintage Chime (class-A), British Hot-Rod (tight), Saturated Lead (smooth), Modern High-Gain (scooped), Classic Rock (crunch), Boutique Lead (smooth, singing), Tight Metal (raw to modern), Low-Watt Combo (spongy breakup), Glass Clean (solid-state), Creamy Lead (tight, mid-hump), Doom Stack (thick, loose), Precision High-Gain (a modern boutique high-gain amp), Digital Extreme (a modern digital-style amp for extreme styles), Master-Volume Rock (80s hard rock), or Acoustic (piezo to acoustic)." },
            { "ampBright", "Bright switch: a treble lift that fades out as the Gain goes up (like the bright switch on a vintage amp). Most noticeable at low gain." },
            { "ampBias", "Power-tube bias. Negative: hotter, with more bite and a tighter feel. Positive: colder, with a looser, crossover-style sound and more gap in the attack. 0 is the amp's own." },
            { "ampMaster", "Master volume of the circuit amp: how hard the power tubes are driven. Turn it up with the Gain low for a clean-but-crunchy power-amp breakup, or keep it low and use Gain for preamp distortion. Only shown in Full circuit mode." },
            { "ampPower", "Power-amp drive: pushes the output stage into saturation for more squash and sizzle. 0 is off." },
            { "ampReso", "Resonance: low-end thump from the power amp and speaker interaction (around 100 Hz). 0 is off. With Derived switched on, the speaker's own bass resonance is already in the model (about +3 dB at 100 Hz), so use this sparingly." },
            { "ampSagAmt", "How far the power supply droops under loud notes (squash and bloom). 0.5 is the amp's own, 0 is stiff, 1 is double." },
            { "ampTube", "The preamp tube: 12AX7 (high gain, tight and crisp), 12AT7 (medium) or 12AY7 (low gain, softer and rounder)." },
            { "ampPwrTube", "The power tubes: As voiced (the amp's own), EL34 (crunchy mids), 6L6 (tight, clean headroom), 6V6 (soft, early breakup, lots of sag), EL84 (bright, chimey) or KT88 (big and tight). Changes the feel, tone and how soon the power stage breaks up." },
            { "ampBPwrTube", "Amp B power tubes: As voiced, EL34, 6L6, 6V6, EL84 or KT88." },
            { "ampStack", "The tone stack's character: as voiced (the amp's own), American or Neutral (most of the amp's own tone shape taken out), or one of the passive stacks solved from component values. Bass, Low Mid, High Mid and Treble still work on top." },
            { "ampFeel", "How much negative feedback the power amp has. Loose: sags and blooms, fuller and looser lows, less presence. Normal: the amp as voiced. Tight: stiff and fast, tighter lows, more presence." },
            { "ampDeep", "A low-end boost into the first stage and the tone stack: more weight and thump." },
            { "ampBStack", "Amp B tone stack: as voiced, American, British or Neutral." },
            { "ampBFeel", "Amp B feedback feel: Loose, Normal or Tight." },
            { "ampBDeep", "Amp B low-end boost." },
            { "bsOn", "Switches the Boost on. It sits after the Lo-Fi pedal and before the NAM pedal, the overdrive and the amp: tighten filter, gain, saturation and a 4-band EQ, in that order (the EQ can go before the saturation instead)." },
            { "bsMode", "The saturation character: Clean (an op-amp boost from published component values, 0 to 26 dB, no colour), EP (warm, bass-heavy and soft, like a small tube preamp), Console (transformer-style: smooth, the lows saturate first), Tube (an asymmetric stage with even harmonics) Treble (a one-transistor treble booster from published component values: Saturation moves its input capacitor from 5 nF to 47 nF, Gain goes up to 38 dB), Mid (a mid-forward push for leads) or FET (a transparent solid-state stage)." },
            { "bsGain", "How much the boost lifts the signal before the saturation, in dB. More gain pushes harder into the saturation, and into the amp after it." },
            { "bsSat", "How much saturation is added. 0 = clean gain; higher = more compression and harmonics. The level rises with it, so turn Level down if it gets loud." },
            { "bsTight", "The tighten filter: cuts the low end that makes a high-gain amp flabby. 0 = off; turning it up raises the cut from 40 Hz to 300 Hz. The style menu picks its character." },
            { "bsTightStyle", "Mid hump: a low cut plus a mid hump around 720 Hz. Low cut: only the low cut, nothing else. Focus: a low cut plus a push around 1.5 kHz for pick attack." },
            { "bsLevel", "The boost's output level, in dB." },
            { "bsMix", "Blends the boosted signal with the unboosted input. 1 = all boost. Lower keeps your clean attack under the boost." },
            { "bsEqOn", "Switches the 4-band EQ on. Bands set to 0 dB cost nothing." },
            { "bsEqPos", "Before saturation: the EQ shapes what hits the saturation (cut lows to keep it tight, boost mids to make it sing). After saturation: it shapes the finished sound." },
            { "bsLoHz", "Low shelf frequency." },
            { "bsLoDb", "Low shelf gain, in dB." },
            { "bsM1Hz", "Band 1 centre frequency (a bell, 100 Hz to 2 kHz)." },
            { "bsM1Db", "Band 1 gain, in dB." },
            { "bsM1Q", "Band 1 width. Low Q = broad, high Q = narrow." },
            { "bsM2Hz", "Band 2 centre frequency (a bell, 500 Hz to 8 kHz)." },
            { "bsM2Db", "Band 2 gain, in dB." },
            { "bsM2Q", "Band 2 width. Low Q = broad, high Q = narrow." },
            { "bsHiHz", "High shelf frequency." },
            { "bsHiDb", "High shelf gain, in dB." },
            { "mfAOn", "Switches Mod FX A on." },
            { "mfAType", "The effect: Flanger, Through-zero flanger (the dry signal is delayed 4 ms too, which adds that fixed delay), Vibrato (pitch wobble only), Uni-V (a throbbing phase-vibrato), Rotary (a rotating horn and drum), Auto-pan (a tremolo if the signal is mono) or Harmonic tremolo (lows and highs pulse in opposite directions)." },
            { "mfAPos", "Before the amp: the effect hits the amp's input (and its gain). After the cab: it is applied to the finished sound, in stereo." },
            { "mfAShape", "The LFO wave: Sine, Triangle or Soft square." },
            { "mfARate", "How fast the effect moves, in Hz (used when Note is Free). For the Rotary this is the horn speed: about 0.8 Hz is slow, 6.7 Hz is fast; the drum follows more slowly, like the real thing." },
            { "mfANote", "Free uses the Rate knob. A note value locks the speed to the tempo (one note length per cycle)." },
            { "mfADepth", "How far the effect sweeps." },
            { "mfAFeedback", "Flangers: positive rings, negative sounds hollow. Other types ignore it." },
            { "mfAManual", "Flangers: the centre delay time (0.3 to 7.8 ms): low = bright and jet-like, high = deep and slow. Rotary: the balance between horn (high) and drum (low). Other types ignore it." },
            { "mfATone", "Rolls off the top end of the effect (not the dry signal)." },
            { "mfAMix", "Dry to wet. Vibrato and Uni-V sound like a pure vibrato at 100%; lower values blend in the dry signal for a flanger/chorus-like character." },
            { "mfBOn", "Switches Mod FX B on." },
            { "mfBType", "The effect: Flanger, Through-zero flanger (the dry signal is delayed 4 ms too, which adds that fixed delay), Vibrato (pitch wobble only), Uni-V (a throbbing phase-vibrato), Rotary (a rotating horn and drum), Auto-pan (a tremolo if the signal is mono) or Harmonic tremolo (lows and highs pulse in opposite directions)." },
            { "mfBPos", "Before the amp: the effect hits the amp's input (and its gain). After the cab: it is applied to the finished sound, in stereo." },
            { "mfBShape", "The LFO wave: Sine, Triangle or Soft square." },
            { "mfBRate", "How fast the effect moves, in Hz (used when Note is Free). For the Rotary this is the horn speed: about 0.8 Hz is slow, 6.7 Hz is fast; the drum follows more slowly, like the real thing." },
            { "mfBNote", "Free uses the Rate knob. A note value locks the speed to the tempo (one note length per cycle)." },
            { "mfBDepth", "How far the effect sweeps." },
            { "mfBFeedback", "Flangers: positive rings, negative sounds hollow. Other types ignore it." },
            { "mfBManual", "Flangers: the centre delay time (0.3 to 7.8 ms): low = bright and jet-like, high = deep and slow. Rotary: the balance between horn (high) and drum (low). Other types ignore it." },
            { "mfBTone", "Rolls off the top end of the effect (not the dry signal)." },
            { "mfBMix", "Dry to wet. Vibrato and Uni-V sound like a pure vibrato at 100%; lower values blend in the dry signal for a flanger/chorus-like character." },
            { "postDelayStyle", "Forward: normal repeats. Reverse: each repeat is played backwards, swelling up to the delay time. Combine it with Ping-Pong in the menu above for reversed echoes bouncing between the sides. (Warble is ignored in Reverse.)" },
            { "preDelayDuck", "Ducking: the repeats of the pre-amp delay dip while you play and swell back when you stop, so they stay out of the way of your notes. 0 = off." },
            { "preDelayDuckRel", "How quickly the repeats come back after you stop playing (the ducking release)." },
            { "preDelayModDepth", "Slowly wobbles the delay time, so the repeats shimmer and drift like a chorus. 0 = off." },
            { "preDelayModRate", "The speed of the delay-time wobble." },
            { "preDelayDiffuse", "Smears each repeat, so the echoes bloom into a soft wash instead of staying clean and distinct. 0 = off." },
            { "preDelayHold", "Hold the delay. Freeze: the delay stops listening and loops what it holds. Infinite: the repeats never die away, but new playing still joins in. Switch it off to let the repeats fade again." },
            { "preDelayHoldMode", "Freeze: the delay loops what it holds and ignores new playing. Infinite: the repeats never die away, and new playing keeps adding to them." },
            { "preDelayTaps", "Multi-tap: extra repeats at fractions of the delay time, ahead of the main repeat. The main repeat and the feedback stay at the full time. Not available as a separate sound in Reverse." },
            { "preDelayTapLevel", "How loud the extra taps are. Needs a Multi-Tap pattern." },
            { "postDelayDuck", "Ducking: the repeats of the post-amp delay dip while you play and swell back when you stop, so they stay out of the way of your notes. 0 = off." },
            { "postDelayDuckRel", "How quickly the repeats come back after you stop playing (the ducking release)." },
            { "postDelayModDepth", "Slowly wobbles the delay time, so the repeats shimmer and drift like a chorus. 0 = off." },
            { "postDelayModRate", "The speed of the delay-time wobble." },
            { "postDelayDiffuse", "Smears each repeat, so the echoes bloom into a soft wash instead of staying clean and distinct. 0 = off." },
            { "postDelayHold", "Hold the delay. Freeze: the delay stops listening and loops what it holds. Infinite: the repeats never die away, but new playing still joins in. Switch it off to let the repeats fade again." },
            { "postDelayHoldMode", "Freeze: the delay loops what it holds and ignores new playing. Infinite: the repeats never die away, and new playing keeps adding to them." },
            { "postDelayTaps", "Multi-tap: extra repeats at fractions of the delay time, ahead of the main repeat. The main repeat and the feedback stay at the full time. Not available as a separate sound in Reverse." },
            { "postDelayTapLevel", "How loud the extra taps are. Needs a Multi-Tap pattern." },
            { "hmOn", "Switches the harmonizer on: two extra voices that follow the note you play. It adds a little delay (shown to your DAW)." },
            { "hmPos", "Before the amp: the harmony is mono and goes through the amp's gain with your playing. After the cab: the harmony is added to the finished sound, in stereo, with a pan for each voice." },
            { "hmMode", "Interval: each voice is a fixed number of semitones away. Scale: each voice is a number of steps up or down the scale you choose, so the harmony stays in key (it listens to the note you are playing)." },
            { "hmTrack", "Single note: tracks one note at a time (fast, about 14 ms), best for leads and single-note lines. Chords: follows several notes at once and moves each by its own amount (about 90 ms of delay). Chords (fast): the same with less delay (about 45 ms), but it cannot tell low notes apart as well, so it suits upper-string voicings." },
            { "hmKey", "The key of the song, for Scale mode." },
            { "hmScale", "The scale the harmony stays in, for Scale mode." },
            { "hmAInt", "Voice A, in Interval mode: how far from your note, in semitones (7 = a fifth up, -12 = an octave down)." },
            { "hmBInt", "Voice B, in Interval mode: how far from your note, in semitones." },
            { "hmADeg", "Voice A, in Scale mode: how many scale steps from your note (2 = a third up, 4 = a fifth up, 7 = an octave up, -2 = a third down)." },
            { "hmBDeg", "Voice B, in Scale mode: how many scale steps from your note." },
            { "hmALvl", "How loud voice A is. 0 = off." },
            { "hmBLvl", "How loud voice B is. 0 = off." },
            { "hmAPan", "Where voice A sits in the stereo picture (only after the cab; before the amp everything is mono)." },
            { "hmBPan", "Where voice B sits in the stereo picture (only after the cab; before the amp everything is mono)." },
            { "hmDry", "The level of your own playing next to the harmony." },
            { "hmTone", "Rolls off the top of the harmony voices. Lower = softer and darker." },
            { "hmGlide", "How quickly the voices slide to a new note. Short = snappy, long = a smooth slide between notes." },
            { "riffMode", "DI: the recorder records your guitar at the input and plays it back into the input, so the pedals, amp and effects shape it as if you were playing it live. Full tone: it records the finished sound at the output and plays it back at the output. The mode applies to the next recording." },
            { "riffLoop", "Play the riff over and over until you press STOP." },
            { "riffLevel", "How loud the playback is." },
            { "riffMute", "While the riff plays, your live playing is muted (DI mode), so only the riff goes through the amp. Off: you can play along on top of it." },
            { "reverbDiffuse", "How smooth the tail is. Low = echoes that tumble over each other and stay distinct; high = a dense, smooth wash. The middle is the sound as voiced. Applies to every type except Classic." },
            { "reverbSpace", "The size of the room, as a multiplier on the type's delay lengths. Below 1 = smaller and tighter, above 1 = a bigger, more distant space. 1 = as voiced. Applies to every type except Classic." },
            { "reverbMod", "Adds slow movement: a gentle three-voice chorus on the reverb's output, and a slight sway in the tail. 0 = none. Works on every type." },
            { "reverbDrive", "Spring type only: pushes the spring's driver. Low is clean; higher thins the lows and adds the soft, lopsided clip that makes a hit spring splash and drip. Off at 0." },
            { "reverbDrag", "Pulls the tail's pitch. In the middle (as voiced) nothing happens. Move it and the tail glides up (right) or down (left), by up to 2 octaves, then settles; turn it while the tail rings for the swirling pitch bends. The Steps menu rounds the destination to musical steps. Applies to every type except Classic." },
            { "reverbDragMode", "Smooth: any pitch in between. Semitones: whole notes. Fifths and octaves: only fifths and octaves. Octaves: only octaves." },
            { "reverbBloom", "Delays the build-up: the reverb swells in behind each note instead of starting at once, like a slow-attack pad. 0 = off. Works on every type." },
            { "reverbSymph", "Adds a pitch-shifted layer to the reverb: a second voice in the tail an octave up, an octave down, a fifth up or two octaves up (see the menu). 0 = none. Works on every type." },
            { "reverbSymphPitch", "The pitch of the Symphony layer." },
            { "granScale", "Snaps the random grain pitches to a scale (instead of any pitch). It works from Pitch Rand of 1 st or more: Pitch Rand sets how far up and down the grains can go. Free = no snapping." },
            { "granShape", "The fade of each grain. Smooth: even fade in and out. Swell: slow fade in, quick end (grains swell). Pluck: sharp start, long decay. Flat: almost no fade (percussive)." },
            { "granScan", "Slowly sweeps the point the grains are taken from, back and forth through the last 1.2 seconds, so the texture keeps evolving. 0 = off." },
            { "granTone", "Rolls off the highs of the cloud. Fully up = no filtering." },
            { "tpRange", "Which guitar this is. The Transpose, Octaver and Pitch pedal all use it. 6-string is the default and sounds as before. 7-string and 8-string tune the pitch shifters to follow the lower strings (down to a low B, or a low F#) and add a little delay for it; use them only if you play those instruments." },
            { "namBOn", "Uses Amp B's own neural capture in place of Amp B's preamp and power stage. Amp B must also be switched on (AMP B ON on the Amp B tab)." },
            { "namBIn", "How hard the signal drives the Amp B model, in dB." },
            { "namBOut", "The Amp B model's output level, in dB." },
            { "namBNorm", "Evens out the volume between models using the loudness stored in the file." },
            { "nmP1On", "Runs the pedal 1 capture (a drive, compressor or other effect), before the overdrive and the amp." },
            { "nmP1In", "How hard the signal drives the pedal 1 model, in dB." },
            { "nmP1Out", "The pedal 1 model's output level, in dB." },
            { "nmP1Norm", "Evens out the volume between models using the loudness stored in the file." },
            { "nmP1Mix", "Dry to wet. The dry signal is delayed to match the model's own delay, so the blend stays in phase." },
            { "nmP2On", "Runs the pedal 2 capture (a drive, compressor or other effect), after the cab." },
            { "nmP2In", "How hard the signal drives the pedal 2 model, in dB." },
            { "nmP2Out", "The pedal 2 model's output level, in dB." },
            { "nmP2Norm", "Evens out the volume between models using the loudness stored in the file." },
            { "nmP2Mix", "Dry to wet. The dry signal is delayed to match the model's own delay, so the blend stays in phase." },
            { "nmP2Stereo", "Off: the stereo signal is summed to mono for one model, whose output goes to both sides (less CPU). On: a second copy of the model runs, one for each side, so a stereo signal stays stereo (about twice the CPU)." },
            { "namOn", "Uses the loaded neural capture (.nam file) in place of Amp A's preamp and power stage. The overdrive pedal still feeds it, and the tone stack and cab come after it. Amp B can still run beside it." },
            { "namIn", "How hard the signal drives the model, in dB. Captures are made at a particular input level: raise this for more gain and breakup, lower it for a cleaner, quieter sound." },
            { "namOut", "The model's output level, in dB." },
            { "namNorm", "Evens out the volume between models using the loudness stored in the file, so different captures sit at similar levels. Off: the raw level of the model." },
            { "lfOn", "Switches the Lo-Fi Warble pedal on. It sits just before the overdrive." },
            { "lfShape", "The shape of the pitch warble: Sine (smooth), Triangle, or Random (a new random pitch each cycle, like a worn tape)." },
            { "lfRate", "How fast the pitch warbles, in Hz." },
            { "lfDepth", "How far the pitch swings. Low = gentle chorus-like movement, high = a seasick vibrato." },
            { "lfDrift", "Random pitch wander and a little fast flutter, like an unsteady tape transport." },
            { "lfTone", "The top end of the worn sound: turn down for dull and muffled, up for brighter." },
            { "lfDirt", "Crunch from soft clipping plus bit and sample-rate crushing. High settings sound gritty and digital." },
            { "lfNoise", "Hiss and crackle, like an old recording." },
            { "lfDropouts", "Random short dips in level, like a worn tape losing contact. 0 = none; more = more often and deeper." },
            { "lfMix", "Dry to wet. At 100% you hear only the warbling signal (pure vibrato); lower values blend in the clean signal for a chorus." },
            { "lfLevel", "The pedal's output level, in dB." },
            { "ampBBright", "Amp B bright switch: a treble lift that fades out as the Gain goes up." },
            { "ampBBias", "Amp B power-tube bias. Negative: hotter and tighter. Positive: colder and looser." },
            { "ampBPower", "Amp B power-amp drive: more output-stage saturation. 0 is off." },
            { "ampBReso", "Amp B resonance: low-end thump around 100 Hz. 0 is off." },
            { "ampBSagAmt", "How far Amp B's power supply droops under loud notes. 0.5 is the amp's own, 0 is stiff, 1 is double." },
            { "ampBTube", "Amp B preamp tube: 12AX7 (high gain), 12AT7 (medium) or 12AY7 (low gain)." },
            { "wowOn", "Pitch pedal on/off. While on it adds a small processing delay (about 14 to 27 ms, set by the Transpose quality menu), which is reported to your DAW." },
            { "wowMode", "What the pedal does. Octave up / Two octaves up / Octave down / Two octaves down: the pedal sweeps from your normal pitch (Position 0) to that shift (Position 1). Dive bomb: a falling pitch drop (Position 0 = none, 1 = deepest). Bend: -12 to +12 semitones (centre = no shift). Custom: you set the heel and toe shifts." },
            { "wowPos", "The pedal position (0 = heel, 1 = toe). Automate it from your DAW, or from a controller mapped to this knob, to play the pedal." },
            { "wowHeel", "Custom mode: the pitch shift (semitones) when the pedal is at the heel." },
            { "wowToe", "Custom mode: the pitch shift (semitones) when the pedal is at the toe." },
            { "wowGlide", "How quickly the pitch follows the pedal position (ms). Short is snappy; long gives a smooth slide." },
            { "wowMix", "Wet/dry mix. 1 = only the shifted sound (like a real pedal); lower blends in the dry guitar for harmony effects." },
            { "octOn", "Octaver on/off. Two shifted voices blended with the dry guitar. While on it adds a small processing delay, which is reported to your DAW." },
            { "octA", "Pitch of voice A: two octaves down, one octave down, one octave up, or two octaves up." },
            { "octALvl", "Level of voice A." },
            { "octB", "Pitch of voice B: two octaves down, one octave down, one octave up, or two octaves up." },
            { "octBLvl", "Level of voice B. 0 turns it off." },
            { "octDry", "Level of the unshifted guitar." },
            { "octTone", "Darkens or brightens the shifted voices (0 = dark and smooth, 1 = bright)." },
            { "blk_cabEq1", "Low shelf on this block (about 110 Hz), in dB." },
            { "blk_cabEq2", "Low-mid peak on this block (about 330 Hz), in dB." },
            { "blk_cabEq3", "Mid peak on this block (about 1 kHz), in dB." },
            { "blk_cabEq4", "High-mid peak on this block (about 2.8 kHz), in dB." },
            { "blk_cabEq5", "High shelf on this block (about 6.5 kHz), in dB." },
            { "blk_cabRso", "Extra low-frequency resonance on this block, at the shared Reso Hz, in dB." },
            { "blk_cabPhs", "All-pass phase shift on this block: moves the phase around 700 Hz to 5.6 kHz without changing the tone. Use it to tune how blocks combine." },
            { "cabDyn", "Speaker dynamics: how much the speakers compress and break up at high levels (bass limiting, cone breakup, heating and a duller top). 0 is off, which is the previous sound. The two Real modes add a cone that stiffens, a top that fades while the lows work hard, and a mid breakup." },
            { "cabTame", "Tame: turns down the resonances in your cab sound (fizz, honk, boxiness) only while they stick out, so a loaded impulse response stops sounding harsh on some notes without going dull overall. 0 is off. It works on built-in cabs and loaded impulse responses." },
            { "cabTameDelta", "Listen to what Tame is removing instead of the sound, to check it is only taking out what you want." },
            { "cabMotion", "Motion: slow, random movement in the cab's sound, different left and right, and bigger the harder you play. A fixed impulse response never changes; a real speaker and mic do. 0 is off, which is the previous sound." },
            { "cabMotionRate", "How fast the Motion changes: low is a slow drift, high is livelier." },
            { "cabDynMode", "Tight (modern): quick and controlled. Vintage (spongy): slower, softer and more compressed." },
            { "ampQuality", "Oversampling for the amp's distortion. Best (4x) sounds smoothest but uses the most CPU and adds the most delay. Balanced (2x) is about half the CPU. Lowest latency has no added delay and the least CPU, but high-gain settings can sound slightly fizzier." },
            { "ampVoicing", "The amp's voicing, with four positions per amp (the fourth was added in v63: Clean Studio, Vintage Chime Jangle, Hot-Rod Mid push, Saturated Lead Sustain, Modern High-Gain Chunk, Classic Rock Bright, Boutique Lead Blues, Tight Metal Buzz, Low-Watt Combo Gritty, Glass Clean Shimmer, Creamy Lead Singing, Doom Stack Fuzz, Precision High-Gain Polished, Digital Extreme Lead, Master-Volume Rock Boosted). Clean: Sparkly, Edge push (a small old amp turned up) or Boutique (fuller mids, more compression). Vintage Chime: Chime, Treble boost or Normal (a darker, rounder, thicker channel without the top boost). Hot-Rod: Tight, Heavy (deep but controlled lows, more gain, forward upper mids) or Brown (a thicker, saggier, mid-rich crunch). Saturated Lead: Smooth, Lead or Crunch (a low-gain channel). Modern High-Gain: Scooped, Clarity or Vintage (spongy, looser, fatter lows). Classic Rock: Crunch, Pushed or Jumped (both channels linked: fatter, hotter, more sag). Boutique Lead: Jazz, Rock or Lead (the boosted lead mode). Tight Metal: Raw, Modern or Crunch (a low-gain rhythm channel). Low-Watt Combo: Soft, Cranked or Four-ten (a 4x10-style voicing with more headroom and bigger lows). Glass Clean: Glass, Warm or Funk (tight, percussive and scooped). Creamy Lead: Rhythm, Lead or Vintage (an early-style sound with lower gain and thick creamy mids). Doom Stack: Stoner, Wall or Bass stack (massive low mids, a cleaner top, a grind only when pushed). Precision High-Gain: Tight (the main high-gain channel), Lead (more gain, mid hump, sustain) or Crunch (an open, lower-gain channel). Digital Extreme: Thall (ultra-tight, scooped, hard clipping), Industrial (harsh, gritty, fizzy upper mids) or Cavern (huge, slow, dark, for drop tunings). Master-Volume Rock: Stock (the classic 80s master-volume sound), Modded (an extra gain stage: tighter, smoother, more saturated) or Cranked (master wide open: loose, saggy, loud). Does nothing in Acoustic mode." },
            { "ampTune", "Focus: tightens the low end and trims bass and gain so low-tuned guitars stay clear. Turn it up as your tuning gets lower (about 1 per semitone below E standard). Only the Saturated Lead, Modern High-Gain, Tight Metal, Creamy Lead, Precision High-Gain and Digital Extreme amps use it." },
            { "ampGain", "Amount of preamp gain for the chosen amp. Since v39 the top of the knob (above about 55%) adds extra gain on top of the old range, most on the high-gain amps, for heavier modern metal and djent saturation; the lower part is unchanged." },
            { "bass", "Bass control (dB), added to the amp's own voicing." },
            { "lowMid", "Low-mid control (dB), around 400-500 Hz." },
            { "highMid", "High-mid control (dB), around 1.2-2 kHz." },
            { "treble", "Treble control (dB)." },
            { "presence", "Power-amp presence: adds treble after the clipping, like a real amp's presence knob." },
            { "acBody", "The size and shape of the acoustic guitar body being imitated: Dreadnought, Jumbo (bigger, lower), Parlor / OM (smaller, more midrange) or Nylon classical (softer top)." },
            { "acBodyAmt", "How strongly the body resonances are applied. 0 = just the piezo signal." },
            { "acFix", "Removes the nasal 'quack' (about 2.3 kHz) and glassy edge (about 4.8 kHz) of a piezo pickup and fills in the hollow low mids." },
            { "acPreamp", "Soft saturation for warmth, like a tube preamp. 0 = clean." },
            { "acSqueeze", "Gentle compression for an even, polished acoustic level. 0 = off." },
            { "acSparkle", "High shelf at 5 kHz (dB): adds or removes sparkle." },
            { "acWarmth", "Low shelf at 200 Hz (dB): adds or removes warmth." },
            { "acRoom", "Short early reflections for a sense of room and stereo width." },
            { "acLevel", "Output level of the acoustic section (dB)." },
            { "cabOn", "Cab section on/off. Off = only the amp's own low-pass. (Not used in Acoustic mode.)" },
            { "blk_cabCbn", "The cabinet shape used by this block's built-in model, in folders by speaker count: 1x (1x12, 1x10, 1x15, 1x8), 2x (2x12, 2x10), 4x (4x12 and 4x10) and Oversized ported cabs (deeper, tighter lows and cleaner low mids, for downtuned playing). Open backs lose bass; closed cabs add thump near 100 Hz." },
            { "cabRoom", "How much room is added to the built-in cab models. The type is chosen in the menu above." },
            { "cabRoomType", "Original: a short, dark ambience (as in earlier versions). Studio room, Large studio room and Living room are modelled from room-microphone recordings: a longer tail, more bass than treble, a dark top. Each has its own decay time." },
            { "cabMeasured", "On (default for new sessions): the mic position and distance controls follow what recordings show, which is a smaller change than the earlier setting. Off: the earlier, stronger behaviour. Sessions saved before v71 open with this off, so they sound the same." },
    { "cabAutoAlign", "Automatically align the cab blocks (time and polarity) whenever you load an IR file with two or more blocks on." },
            { "cabSpread", "Pushes the cab blocks apart in the stereo field (A left, B right, C a little left, D a little right) on top of each block's Pan. Needs two or more blocks." },
            { "cabLowCut", "Cab-wide low cut after the cab, to remove rumble." },
            { "cabHighCut", "Cab-wide high cut after the cab, to remove fizz." },
            { "cabTilt", "Makes the whole cab darker or brighter around 800 Hz." },
            { "cabReso", "A low resonance peak (dB) for speaker-cabinet thump, at the Reso Hz frequency. 0 = off." },
            { "cabResoHz", "Frequency of the cab resonance peak." },
            { "cabAir", "Gentle high shelf from about 6 kHz (dB)." },
            { "cabType", "Fallback cab filter used when no cab block has an IR." },
            { "blk_cabOn", "Turns this cab block on or off. Up to four blocks play together." },
            { "blk_cabSrc", "Built-in model: the plugin builds the cab from speaker, mic and position. IR file: load your own WAV or AIFF impulse response." },
            { "blk_cabSpk", "Speaker for the built-in model, in folders. Realistic: hand-built speaker models by cab type (1x12, 2x12, 4x12, 4x12 oversized), plus Classic (older styles): five speakers fitted to close-mic recordings of older rock and metal cabs. Mix ready: speakers fitted to the measured frequency response of mixed (already EQ'd) cab recordings: 4x12 and compact cabs. Mix-ready speakers are closest to their recordings with the Flat mic and the 4x12 Closed cabinet. The folders say what each is typically used for; any speaker works with any cabinet." },
            { "blk_cabMic", "Mic for the built-in model: six general mics, six aimed at heavier playing (Dynamic Aggressive, Dynamic Punch, Dynamic Scoop, Ribbon Dark, Condenser Warm, and a dynamic + ribbon blend), or Flat for no mic colouring." },
            { "blk_cabPos", "Mic position: 0 = centre of the speaker's dust cap (bright, direct), 1 = the cone's edge (darker, warmer)." },
            { "blk_cabDist", "Mic distance (inches): close = more bass (proximity effect), far = a little less top and a later arrival." },
            { "blk_cabAng", "Mic angle (degrees): pointing the mic off-axis loses highs." },
            { "blk_cabTight", "Low end of the built-in cab: right = tighter (less below about 150 Hz), left = fuller. Centre = unchanged." },
            { "blk_cabBite", "Upper-mid bite of the built-in cab (2.4 and 3.8 kHz): right = more bite, left = smoother. Centre = unchanged." },
            { "blk_cabBox", "Adds the narrow resonances and cancellations a real cabinet has (peaks and notches between 600 Hz and 4.5 kHz) to the built-in model. 0 = smooth (as before). They move with the Cabinet menu: smaller boxes ring higher." },
            { "blk_cabLevel", "Level of this cab block in the mix (dB)." },
            { "blk_cabPan", "Stereo position of this cab block." },
            { "blk_cabDelay", "Delays this block (0-10 ms), used to line up blocks in time by hand (ALIGN NOW sets it for you, and you can fine-tune it here). 0.01 ms steps; 1 ms is about 34 cm of distance. Watch the IR ALIGNMENT display while you turn it." },
            { "blk_cabTilt", "Brighter or darker, for this block only (-12 to +12 dB). Positive lifts the top and cuts the bottom around 800 Hz by the same amount; negative does the opposite. 0 = no change." },
            { "blk_cabPol", "Flips this block's polarity (upside down). Alignment sets this automatically when blocks are out of phase." },
            { "blk_cabLowCut", "Low cut on this block alone, before the blend." },
            { "blk_cabHighCut", "High cut on this block alone, before the blend." },
            { "eqOn", "Post EQ on/off. It sits after the cab, before the textures, delays and reverb." },
            { "eqMode", "Graphic: fixed-width bands. Mod: the Q knob changes every band's width together." },
            { "eqQ", "Mod mode: width of all bands (higher = narrower)." },
            { "eqLowCut", "Post EQ high-pass (Hz): removes everything below this." },
            { "eqHighCut", "Post EQ low-pass (Hz): removes everything above this." },
            { "eqSlope", "How steep the low cut and high cut are (12 or 24 dB per octave)." },
            { "eqLevel", "Overall level after the EQ (dB)." },
            { "eqBand", "Boost or cut this band by up to 12 dB." },
            { "tremOn", "Tremolo / auto-pan on/off. It sits after the Post EQ." },
            { "tremMode", "Tremolo: both sides pulse together. Stereo: the left and right sides alternate (an auto-pan feel)." },
            { "tremShape", "Shape of the pulse: sine (smooth), triangle, or soft square (choppy)." },
            { "tremRate", "Speed in Hz (used when Note is Free)." },
            { "wahNote", "Free uses the LFO Rate knob. A note value locks the wah sweep to the tempo (one note length per cycle)." },
            { "phaserNote", "Free uses the Rate knob. A note value locks the phaser sweep to the tempo (one note length per cycle)." },
            { "wobNote", "Free uses the Rate knob. A note value locks the wobble to the tempo (one note length per cycle)." },
            { "swellNote", "Free uses the Fade-In knob. A note value makes each swell last exactly that note length at the tempo." },
            { "granSizeNote", "Free uses the Size knob. A note value makes every grain exactly that long at the tempo." },
            { "granDensNote", "Free uses the Density knob. A note value starts one grain per note length, so the cloud pulses with the tempo." },
            { "granPosNote", "Free uses the Position knob. A note value reads grains from exactly that many beats back in the loop." },
            { "preDelayMode", "Delay character. Digital: clean and bright. Analog: darker, bucket-brigade style, with soft saturation and a slight wobble. Tape: dark and saturated with wow, flutter and drift. Lo-Fi: dark with reduced sample rate and bit depth. The Lo-Fi and Warble knobs add to the mode." },
            { "postDelayMode", "Delay character. Digital: clean and bright. Analog: darker, bucket-brigade style, with soft saturation and a slight wobble. Tape: dark and saturated with wow, flutter and drift. Lo-Fi: dark with reduced sample rate and bit depth. The Lo-Fi and Warble knobs add to the mode." },
            { "tpOn", "Transpose on/off. It is the first effect in the chain, so everything after it (fuzz, overdrive, amp) plays the shifted notes. While it is on it adds a small delay, which the plugin reports to your DAW." },
            { "tpSemis", "Shift the pitch by whole semitones (-24 to +24)." },
            { "tpFine", "Fine pitch shift in cents (hundredths of a semitone), for detuning." },
            { "tpFormant", "Formant shift: moves the tone's emphasis up or down. This is an EQ-style approximation (it cuts and re-adds emphasis around two regions), not a true formant resynthesis. Plus the Lock amount if Formant Lock is on." },
            { "tpLock", "Formant Lock: automatically shifts the formant the opposite way to the transpose, so the tone stays close to the original instead of getting chipmunk-bright (up) or dark (down)." },
            { "tpMix", "Blend between your original notes and the shifted ones. Around 0.5 gives a harmony or detune." },
            { "tpQuality", "Fast has less delay (about 14 ms); Smooth adds about 27 ms and is better for low strings and for shifting down. Works best on single notes. (The Mode menu in the header can override this.)" },
            { "ampAutoLevel", "Auto level: this amp's output is held at the same loudness for every setting of the Gain knob and for every amp model and voicing, so changing the gain changes the sound, not the volume. The level comes from a table I measured, so it adds no processing. Overdrive and Modern Metal settings can still change the loudness a little." },
            { "ampBAutoLevel", "Auto level for Amp B: same as Amp A's. Holds Amp B at the same loudness for every setting of its Gain knob and every model. B Level still works on top of it." },
            { "compAuto", "Auto level: adds the makeup gain for you, based on the Threshold, Ratio and Mix (a table I measured with a plucked test signal). The Output knob then trims on top of it, with 0 dB meaning 'about as loud as the compressor off'." },
            { "odAuto", "Auto level: sets the overdrive's loudness to match the guitar without it (measured through the Clean amp, so it is the pedal's own level change). The Volume knob then works relative to its -6 dB starting point. Turn it off when you want the overdrive as a boost into the amp." },
            { "fuzzMix", "Blend between your clean guitar (0) and the fuzz (1). Around 0.3 to 0.6 keeps the note defined under the fuzz. The clean part is delayed to match the fuzz, so the two stay in phase." },
            { "fuzzAuto", "Auto level: a slow level follower keeps the fuzz about as loud as the clean guitar going into it, whatever the Gain and style. The Level knob then works relative to its -9 dB starting point. It holds its setting during silence." },
            { "eqAuto", "Auto level: a slow level follower keeps the Post EQ's output at the loudness of its input, so boosting or cutting bands changes the tone, not the volume. The Level knob still trims on top of it." },
            { "ampBOn", "Turns on the parallel Amp B. It gets the same signal as Amp A (after the overdrive pedal) and runs its own preamp and tone stack. Each cab block then picks Amp A, Amp B or both. Uses about as much CPU as Amp A again. Not available with the Acoustic amp." },
            { "ampBModel", "Which electric amp Amp B is. Use a different one from Amp A for layered tones, such as a tight high-gain with a cleaner amp underneath." },
            { "ampBVoicing", "Amp B's voicing: the same four-position switch as Amp A's, with the names for Amp B's model." },
            { "ampBUseOd", "Lets the overdrive pedal feed Amp B too (it always feeds Amp A). Turn it off to hear Amp B without the pedal's boost." },
            { "ampBGain", "Amp B's preamp gain." },
            { "ampBTight", "Amp B's tightness: raises the high-passes between its gain stages (Hot-Rod, Lead and High-Gain only)." },
            { "ampBBass", "Amp B bass (dB)." },
            { "ampBLowMid", "Amp B low mid (dB)." },
            { "ampBHighMid", "Amp B high mid (dB)." },
            { "ampBTreble", "Amp B treble (dB)." },
            { "ampBPresence", "Amp B presence: treble on its power stage." },
            { "ampALevel", "Amp A's level while Amp B is on (dB). Balance the two amps with this and B Level." },
            { "ampBLevel", "Amp B's level (dB)." },
            { "blk_cabAmp", "Which amp feeds this cab block: Amp A, Amp B, or both added together (about 3 dB quieter each, so the sum is not much louder). Only matters while Amp B is on." },
            { "mtLead", "Lead boost: more gain in the first stages, a little more mid push and sag, with the level trimmed to match. For solos." },
            { "mtTight", "Raises the high-passes between the gain stages (up to about 2.8 times), so low palm mutes stay clear and fast." },
            { "mtDepth", "Low-end depth: a shelf below about 90 Hz after the preamp (up to +6 dB) for a bigger thump." },
            { "mtMidHz", "Slides the pre-gain mid push up or down in frequency. The middle position is the amp's own." },
            { "mtMid", "How much extra mid push goes in before the gain stages (up to +8 dB), for a more focused, aggressive mid." },
            { "mtFizz", "Rolls off the top after the preamp (from 12 kHz down to about 3.8 kHz) to remove fizz without dulling the pick attack as much as turning Presence down." },
            { "mtChug", "A fast noise gate after the preamp that cuts the gain's hiss between palm mutes. 0 = off. Too high cuts quiet notes." },
            { "reverbType", "Reverb character. Classic is the original simple reverb. Room is short and natural, Hall is large and smooth, Plate is bright and dense, Ambient is long, lush and slowly moving, Cathedral is huge and dark, Spring is short and boingy, Soft Plate is dense and gently moving, Tumble has echoes that tumble over each other, Abyss is huge, dark and drifting. All are hand-tuned approximations. Size, Damp and Width work on every type." },
            { "reverbPreNote", "Free uses the Pre-Dly knob. A note value sets the pre-delay to that note length (up to 250 ms)." },
            { "tremNote", "Free uses the Rate knob. A note value locks the speed to the tempo (one note length per cycle)." },
            { "tremDepth", "How deep the volume dips. 1 = down to silence." },
            { "wobOn", "Wobble on/off: pitch vibrato like a loosely held tremolo arm." },
            { "wobRate", "Speed of the vibrato (Hz)." },
            { "wobDepth", "How far the pitch moves. Small values give a few cents of drift; high values sound seasick." },
            { "wobDrift", "Adds a slow random drift to the pitch, like an unsteady hand." },
            { "wobMix", "1 = pure vibrato; lower blends in the dry signal for a chorus-like effect." },
            { "swellOn", "Swell on/off: automatically fades in each new note (volume swell / slow gear)." },
            { "swellAttack", "How long each note takes to fade in (ms)." },
            { "swellSens", "How easily it detects a new note. Higher catches softer notes; lower ignores them." },
            { "granOn", "Granular cloud on/off. It records a 4-second loop and plays back lots of tiny overlapping grains. It sits after the Post EQ." },
            { "granFreeze", "Stops recording, so the cloud keeps playing whatever was in the loop. Play something, then freeze it." },
            { "granSize", "Grain length (ms). Short = grainy and glitchy; long = smooth and pad-like." },
            { "granDensity", "How many grains start each second. More = thicker cloud." },
            { "granPos", "How far back in the loop grains start (ms)." },
            { "granSpray", "Scatters the grain start points further back, up to 1.5 seconds." },
            { "granPitch", "Pitch of the grains in semitones. +12 with long grains gives a shimmer-like octave-up cloud." },
            { "granPitchRand", "Random pitch variation per grain (up to +-12 semitones) for a detuned, shimmery wash." },
            { "granReverse", "Chance that a grain plays backwards." },
            { "granWidth", "How far grains are scattered left and right." },
            { "granFeedback", "Feeds the cloud back into the recording, so it evolves and builds." },
            { "granMix", "Crossfade between your dry signal (0) and the cloud (1)." },
            { "reverbOn", "Reverb on/off. It's the last effect before the output level." },
            { "reverbMix", "Wet/dry amount of reverb." },
            { "reverbSize", "Decay time in seconds (how long the tail takes to fade by 60 dB), from 0.1 to 30 s. Longer decays also use a bigger space. The Classic type tops out around 12 s; the other types go all the way to 30 s." },
            { "reverbPreDelay", "Gap before the reverb starts (ms). Longer keeps the dry note clearer." },
            { "reverbDamp", "How quickly the highs die away in the tail. Higher = darker, softer tail." },
            { "reverbWidth", "Stereo width of the reverb." },
            { "reverbLowCut", "Removes lows from the reverb (Hz) so it doesn't get muddy." },
            { "reverbHighCut", "Removes highs from the reverb (Hz) for a darker wash." },
            { "reverbDuck", "Lowers the reverb while you play and lets it bloom when you stop." },
            { "reverbFreeze", "Holds the reverb tail so it sustains indefinitely." },
            { "shimmerOn", "Shimmer on/off: pitch-shifted sound fed through the reverb for a rising, ethereal wash." },
            { "shimmerAmount", "How much pitch-shifted sound is fed into the reverb." },
            { "shimmerPitch", "The interval the shimmer is shifted by, from an octave down to two octaves up, including two-voice choices such as octave + fifth." },
            { "shimmerTone", "Tone (Hz) of the shimmer signal: lower = darker." },
            { "outGain", "Final output level (dB). Keep it low while you raise gain knobs." },
            { "outMono", "Sums the output to mono (both sides get the same signal). Use it to check how a wide sound folds down on a mono system." },
            { "dblOn", "Doubler on/off: imitates double-tracking by adding two slightly delayed, drifting copies spread left and right. It sits after the textures, before the post-amp delay." },
            { "dblTime", "Base delay of the two copies (ms), 5 to 60. About 15-30 ms sounds like a second take; longer starts to sound like an echo; below about 10 ms it turns into a thick, flanger-like thickening." },
            { "dblVar", "How much the copies drift in time at random, like two takes that never line up exactly." },
            { "dblSpread", "How far the two copies are pushed left and right. 0 = both in the middle." },
            { "dblMix", "How much of the doubled copies you hear against the original." },
            { "widOn", "Widener on/off. It sits after the reverb, just before the output level. It needs a stereo signal (from the doubler, stereo cab blocks or the reverb) to have anything to widen." },
            { "widMode", "Three flavours of the widening (my own design): Classic, Desktop (a little more low-mid body) and Rack (subtler)." },
            { "widWow", "How much the stereo image is widened: the left-right difference signal is lifted and made louder." },
            { "widSpace", "Smears the difference signal in time and phase for a bigger, more diffuse space. Too much can sound unfocused." },
            { "widBass", "Bass Lift: adds harmonics of the low notes so bass is audible on small speakers. 0 = off." },
            { "widBassHz", "The bass frequency Bass Lift works on." },
            { "widMono", "Frequencies below this are kept in the centre (taken out of the widened part), so the bass stays solid and mono-compatible." },
            { "widMix", "Wet/dry blend of the widener." },
            { "widLevel", "Level after the widener (dB), to compensate for loudness changes." },
        };
        return table;
    }
}


//==============================================================================
// Hover help for the items inside the dropdown menus. Hovering a menu shows what the current item
// does; hovering an item while the menu is open shows what that item does. Keyed by the parameter
// id, then the item's exact text. (Cab block menus end in A to D and share one entry each, and all
// the note-value menus share the table at the bottom.)
//==============================================================================
namespace
{
    using ItemTips = std::vector<std::pair<const char*, const char*>>;

    const std::map<std::string, ItemTips>& itemTipTable()
    {
        static const std::map<std::string, ItemTips> table =
        {
            { "perfMode", {
                { "Normal", "Uses the individual quality menus (Amp Quality, Transpose Mode) as you set them. The limiter looks ahead 1 ms." },
                { "Low Latency", "Overrides them for the least delay and CPU: no oversampling in the amp or fuzz, the shortest transpose window, and a limiter with no look-ahead. High-gain tones can sound slightly fizzier." },
                { "High Quality", "Overrides them for the cleanest sound: 4x oversampling in the amp and fuzz, the smoothest transpose window, and a 2 ms look-ahead limiter. Uses the most CPU and adds the most delay." } } },
            { "deNoiseHz", {
                { "60 Hz (US)", "Mains hum at 60 Hz (North America and parts of Asia). The de-noise targets 60 Hz and its harmonics." },
                { "50 Hz (EU)", "Mains hum at 50 Hz (Europe and most of the world). The de-noise targets 50 Hz and its harmonics." } } },
            { "inputMode", {
                { "Input: Left", "Use the left input channel only, and play it in both ears." },
                { "Input: Right", "Use the right input channel only, and play it in both ears." },
                { "Input: Mono (L + R)", "Add both input channels together." } } },
            { "noiseGrossScene", {
                { "Bright (tritone)", "A tritone (+6 semitones) and a flat ninth (+13) on top: bright, tense dissonance." },
                { "Dark (minor 2nd)", "A minor second (+1) and a tritone (+6), with the highs rolled off: dark and murky." },
                { "Full (all three)", "A minor second, tritone and major seventh (+1, +6, +11) all at once: full dissonance." } } },
            { "ringMode", {
                { "Ring", "Plain multiplication. The original notes mostly disappear, leaving metallic clangs." },
                { "AM", "Amplitude modulation. The original notes stay, with a shimmering, tremolo-like layer on top." } } },
            { "ringWave", {
                { "Sine", "The smoothest ring-mod tone." },
                { "Square", "A harsher, buzzier ring-mod tone with more harmonics." },
                { "Triangle", "In between sine and square." } } },
            { "fuzzStyle", {
                { "Wall (sustained)", "Thick, sustained wall of sound: two clipping stages and a tone control that scoops the mids." },
                { "Tight (op-amp)", "Tight, aggressive, mid-forward distortion from a high-gain op-amp stage and hard clipping." },
                { "Gated (velcro)", "The older gated fuzz, kept only so old sessions sound the same. It cannot be picked any more: Factory does the same job with more control (Comp, germanium / silicon, Guitar Vol)." },
                { "Buzzsaw (scooped)", "Heavy-metal pedal circuit: two gain stages, op-amp, clipper, then a two-band tone section (Low and High). Circuit-derived; part values read from a schematic, transistor and op-amp details estimated." },
                { "Face (round, germanium or silicon)", "A round, two-transistor fuzz that cleans up when you turn your guitar down. Germanium / silicon switch, Starve and Guitar Vol knobs, and a buffer option." },
                { "Bender (raspy, 2 or 3 stage)", "A raspy, cutting fuzz with a mid emphasis. Early, Classic or Late model, a germanium / silicon switch and a Starve knob." },
                { "Factory (gated, squealing)", "A gated, squealing, unstable fuzz with Gate, Stab and Comp knobs and a germanium / silicon switch." },
                { "Nuke (wave-shaper)", "A wave-shaper fuzz: Gain drives the signal through a curve (soft, sine-fold, odd-harmonic, staircase or asymmetric fold) that you morph with Shape. Crush adds bit and sample-rate reduction; Tone is a resonant output filter." },
                { "Crusher (bits and rate)", "A bit-crusher with drive in front: Bits lowers the bit depth, Rate lowers the sample rate (aliasing), Gain drives it harder and Tone tames the harsh top." } } },
            { "fuzzBenderModel", {
                { "Early (2 stage)", "Two transistors: thick, woolly, round mids and a lower gain." },
                { "Classic (3 stage)", "Three transistors: raspy, cutting, with the mid emphasis higher up." },
                { "Late (3 stage, tone sweep)", "Three transistors, a tighter low end and more fizz; Tone sweeps a wide range." } } },
            { "fuzzMuffVer", {
                { "Standard", "The original Wall sound." },
                { "Smooth (deep scoop)", "A deeper mid scoop and a smoother, rounder top." },
                { "Gritty (thick)", "Thick and a bit rough, with more low end." },
                { "Mid-forward (bite)", "More mids and bite, a tighter low end." },
                { "Heavy (fuzzy lows)", "More gain and a huge, fuzzy low end." },
                { "Circuit (from the schematic)", "Solved from the real circuit values: booster, two diode-limited stages, the passive tone network and output stage. Sustain, Tone, Diodes, Op-Amp and Tone Bypass all act on the circuit itself. The Mids knob is not used here." },
                { "Circuit (variant B, 500 pF)", "The later build of the same fuzz, solved from its own schematic: smaller coupling caps (tighter lows), smaller capacitors in series with the diodes, lower stage gains, a 20k tone resistor (a shallower mid dip) and 500 pF feedback capacitors. Quieter and less compressed than the first circuit." },
                { "Circuit (variant C, 470 pF)", "Like variant B but with 470 pF feedback capacitors: a slightly brighter, more open top." },
                { "Circuit (op-amp build)", "An op-amp version of the fuzz, solved from its schematic: an inverting booster, a resonant low-pass (about 1 kHz) with the sustain control after it, an op-amp clipper with three diodes each way, and a Muff-style tone network. Tone Bypass switches to the direct output. Very high gain; Sustain mostly sets how hard the clipper is driven." } } },
            { "fuzzMuffDiode", {
                { "Silicon", "The original clipping." },
                { "LED (open)", "More headroom, louder, more open and less compressed." },
                { "Germanium (soft)", "Clips earlier, softer and more compressed." } } },
            { "compRatio", {
                { "1:1", "No compression. Only the saturation (if on) is applied." },
                { "2:1", "Gentle compression." },
                { "3:1", "Light to medium compression." },
                { "4:1", "Medium compression." },
                { "6:1", "Firm compression, a good starting point for guitar." },
                { "10:1", "Heavy compression." },
                { "10:1 Soft", "Heavy, but with a soft knee and a release that slows as it compresses harder, like a light-controlled compressor." },
                { "20:1", "Very heavy compression, almost limiting." },
                { "Limit", "A hard-knee brickwall limiter." } } },
            { "compDetHp", {
                { "Off", "The compressor listens to the whole signal." },
                { "80 Hz", "Ignores the lowest bass when deciding how much to compress." },
                { "160 Hz", "Ignores the low end below 160 Hz, so low palm mutes pump less." },
                { "320 Hz", "Ignores everything below 320 Hz: the compressor reacts mostly to the mids and highs." } } },
            { "compSat", {
                { "Off", "No added harmonics." },
                { "Tube-like", "Adds 2nd-harmonic warmth, stronger with more gain reduction." },
                { "Tape-like", "Adds 3rd-harmonic saturation, stronger with more gain reduction." },
                { "Both", "Tube-like and tape-like saturation together." },
                { "Transformer", "Soft saturation of the low end only, like an iron transformer: thickens and rounds the lows while the highs stay clean." },
                { "FET stage", "Class-A solid-state output with a little extra odd-harmonic edge. Pairs with the FET style." },
                { "Opto tube stage", "A soft, warm, even-harmonic tube stage with gentler growth. Pairs with the Opto style." },
                { "Mu iron + tube", "A tube stage plus low-end iron saturation together. Pairs with Variable-mu." },
                { "Diode", "A rounded, symmetric diode-style softening, like a squeeze pedal's output. Pairs with Sustainer." } } },
            { "compStyle", {
                { "Standard", "The original compressor." },
                { "FET", "Fast attack, hard knee and a release that snaps back after deep squeezes: punchy and forward." },
                { "Opto", "A soft knee and a slow grab. The release slows down the longer and deeper it squeezes, so it is smooth and hard to hear." },
                { "Variable-mu", "Very soft knee and a ratio that deepens as the signal gets louder: gentle, even glue." },
                { "Sustainer", "A pedal-style squeeze that lets the first part of the pick through, then holds the note up with a long release." },
                { "Bus", "Light glue (4:1 at most, slow attack) that holds chords together without flattening the pick." },
                { "Dyna (OTA)", "A squashing pedal compressor from a published analysis: Threshold sets how hard it squeezes (lower = more), Attack, Release, Output and Mix work as labelled, and the ratio, ratio curve and detector are my own estimates because the analysis does not give them. Ratio, Auto makeup and the extras are ignored." } } },
            { "odStyle", {
                { "Transparent", "A Centaur-type overdrive: a gain pot that fades the clean signal out as the clipped path comes in, germanium clipping, and an active treble shelf (Tone). Edge adds extra top end into the clipper. Component values the published analysis does not give are my estimates." },
                { "Tight", "A tight, modern metal-style boost with Drive, Bright, Gate and Attack." },
                { "Open", "A dynamic, amp-like overdrive with Drive and Tone and a High / Low range switch. It stays open and responds to your picking." },
                { "Screamer", "A versatile mid-hump drive: a clean path blended with a clipped, low-cut gain path. Clipping and voicing menus, Drive, Tone and Mid; Bass, Mid Freq and Bright behind ADV." },
                { "Distortion", "A hard-clipped distortion: all the signal goes through a high-gain stage into hard diodes. Clipping menu, Dist and Tone; the Mids switch and Bright behind ADV." },
                { "Tube Driver", "A full-range, warm, creamy tube-style drive that keeps a lot of the clean signal at low Drive and turns thick and compressed when pushed, a lead boost for rhythm and solos. Drive, Bass (a low cut: tight at 0, full at 1) and Treble; Bright behind ADV." },
                { "Op-Amp Distortion", "A single op-amp gain stage (1.5 to 213 times) into a diode clipper, with a 720 Hz low-cut at full gain, so it is mid-forward and bright. The Clipping menu picks the diodes; the stock pedal has germanium." },
                { "Stack Drive", "Two gain stages (up to 70 dB) into red-LED clipping, then a tone section with a mid scoop. Gain and Tone. The tone section is my approximation: the analysis does not list its component values." } } },
            { "odClip", {
                { "Silicon (classic)", "The classic clipping: tight, even, familiar." },
                { "LED (open)", "A higher clipping threshold: more headroom, louder and more open, with a later, harder bend." },
                { "Germanium (soft)", "A low threshold: it clips early and gently, so it is compressed, smooth and a little quieter." },
                { "Asymmetric", "One half of the wave clips harder than the other: more even harmonics, a warmer and more amp-like drive." },
                { "None (op-amp rails)", "Circuit mode only: no diodes, the op-amp itself runs out of swing. Louder, more open and more gradual. (In the Distortion style this behaves like Asymmetric.)" } } },
            { "odVoice", {
                { "Classic (mid hump)", "The familiar mid hump with the full gain range." },
                { "Mild (lean)", "Less gain and no extra hump: leaner and more transparent." },
                { "Open (bright)", "More gain, a looser low end and extra top end." } } },
            { "odMidMod", {
                { "Scoop (stock)", "A mid dip: scooped, fizzy and aggressive." },
                { "Flat", "Leaves the mids alone." },
                { "Boost", "A mid hump: thicker and more lead-like." } } },
            { "tpQuality", {
                { "Fast (~14 ms delay)", "Shorter processing window: less delay, but low notes (below about 100 Hz) and down-shifts sound rougher." },
                { "Smooth (~27 ms delay)", "Longer processing window: smoother on low strings and down-shifts, but adds more delay." } } },
            { "ampMode", {
                { "Classic", "The original voicing: the amp as set up by ear, with simple clipping curves. This is the default." },
                { "Circuit-derived", "Clipping curves from tube-circuit simulation, plus feedback, presence, supply droop, transformer saturation and preamp corners worked out from the circuit values. About the same CPU as Classic." } } },
            { "ampBMode", {
                { "Classic", "The original voicing." },
                { "Circuit-derived", "Clipping curves and power section worked out from the circuit values." } } },
            { "ampModel", {
                { "Acoustic (piezo to acoustic)", "Replaces the guitar amp and cab with a chain that makes a piezo pickup sound like a miked acoustic guitar." },
                { "Clean (sparkly)", "A bright, sparkly clean amp with lots of headroom." },
                { "Vintage Chime (class-A)", "A chimey class-A style amp that breaks up in the mids." },
                { "British Hot-Rod (tight)", "A tight, punchy high-gain British-style amp." },
                { "Saturated Lead (smooth)", "A smooth, singing lead amp. Has the Focus knob." },
                { "Modern High-Gain (scooped)", "A modern, scooped high-gain amp. Has the Focus knob." },
                { "Classic Rock (crunch)", "A classic British rock-crunch amp: open, mid-forward, loose and dynamic. It stays clear at low gain. Voicing: Crunch is the main sound; Pushed is a hard-rock lead tone; Jumped is both channels linked, fatter and hotter." },
                { "Boutique Lead (smooth, singing)", "A boutique-style amp with lots of clean headroom that turns into a smooth, vocal, compressed lead when you push it. Voicing: Jazz (off) is clean, round and warm; Rock (on) is pushed and mid-forward." },
                { "Tight Metal (raw to modern)", "A tight, aggressive three-stage high-gain amp for chugging rhythm and lead. Voicing: Raw (off) is bright and edgy; Modern (on) has more gain, a tighter low end and a smoother top. Has the Modern Metal controls and Focus." },
                { "Low-Watt Combo (spongy breakup)", "A small, low-power combo that breaks up early and soft, with a lot of sag. It cleans up when you play softly. Voicing: Soft (off) is spongy and honky; Cranked (on) is louder, spittier and thicker." },
                { "Glass Clean (solid-state)", "A stiff, solid-state style clean with huge headroom and no sag, great under chorus. Voicing: Glass (off) is bright and wide open; Warm (on) is rounder and darker." },
                { "Creamy Lead (tight, mid-hump)", "A tight, creamy high-gain amp with a mid hump. Voicing: Rhythm (off) is tight and punchy for chunk; Lead (on) has more gain, a big mid hump and long sustain. Has the Modern Metal controls and Focus." },
                { "Doom Stack (thick, loose)", "A thick, loose, heavy amp with a huge low end for doom, stoner and sludge. Voicing: Stoner (off) is fuzzy and saturated; Wall (on) is huge, deep and slow, with more headroom." },
                { "Precision High-Gain (tight, modern)", "A modern boutique high-gain amp: very tight, articulate and clear, with a firm low end, a focused upper mid and a smooth top. Voicing: Tight is the main high-gain channel; Lead adds gain, a mid hump and sustain; Crunch is an open, dynamic, lower-gain channel. Has the Modern Metal controls and Focus." },
                { "Digital Extreme (brutal, digital)", "A modern digital-style amp for extreme styles: brutally tight, very high gain, hard clean-edged clipping and almost no sag. Voicing: Thall is ultra-tight and scooped for chugs; Industrial is harsh, gritty and fizzy; Cavern is huge, slow and dark for drop tunings and doom. Has the Modern Metal controls and Focus." },
                { "Master-Volume Rock (80s hard rock)", "A hard-rock amp from the master-volume era: a cascaded gain stage, tight, bitey and compressed. Voicing: Stock is the classic 80s sound; Modded adds a gain stage for a tighter, smoother, more saturated sound; Cranked has the master volume wide open, so the power stage does the clipping: looser, saggier and louder. No Modern Metal controls." } } },
            { "ampBModel", {
                { "Clean (sparkly)", "A bright, sparkly clean amp with lots of headroom." },
                { "Vintage Chime (class-A)", "A chimey class-A style amp that breaks up in the mids." },
                { "British Hot-Rod (tight)", "A tight, punchy high-gain British-style amp." },
                { "Saturated Lead (smooth)", "A smooth, singing lead amp." },
                { "Modern High-Gain (scooped)", "A modern, scooped high-gain amp." },
                { "Classic Rock (crunch)", "A classic British rock-crunch amp: open, mid-forward, loose and dynamic." },
                { "Boutique Lead (smooth, singing)", "Clean headroom that turns into a smooth, singing lead. Voicing: Jazz (off) or Rock (on)." },
                { "Tight Metal (raw to modern)", "A tight, aggressive high-gain amp. Voicing: Raw (off) or Modern (on)." },
                { "Low-Watt Combo (spongy breakup)", "A small combo that breaks up early and soft, built from a tweed Deluxe schematic: its single Tone control is the Treble knob; Bass and the mid knobs sit on top as extra EQ. Voicing: Soft (off) or Cranked (on)." },
                { "Glass Clean (solid-state)", "A stiff, solid-state style clean with huge headroom. Voicing: Glass (off) or Warm (on)." },
                { "Creamy Lead (tight, mid-hump)", "A tight, creamy high-gain amp. Voicing: Rhythm (off) or Lead (on)." },
                { "Doom Stack (thick, loose)", "A thick, loose, heavy amp for doom and sludge. Voicing: Stoner (off) or Wall (on)." },
                { "Precision High-Gain (tight, modern)", "A modern boutique high-gain amp. Voicing: Tight, Lead or Crunch." },
                { "Digital Extreme (brutal, digital)", "A modern digital-style amp for extreme styles. Voicing: Thall, Industrial or Cavern." },
                { "Master-Volume Rock (80s hard rock)", "An 80s master-volume hard-rock amp. Voicing: Stock, Modded or Cranked." } } },
            { "wowMode", {
                { "Octave up", "Heel: normal pitch. Toe: one octave up." },
                { "Two octaves up", "Heel: normal pitch. Toe: two octaves up." },
                { "Octave down", "Heel: normal pitch. Toe: one octave down." },
                { "Two octaves down", "Heel: normal pitch. Toe: two octaves down." },
                { "Dive bomb", "A falling pitch drop. Position 0 = no shift; turned up, it dives deeper (the response is curved for a natural feel)." },
                { "Bend (-12 to +12)", "Heel: an octave down. Centre: no shift. Toe: an octave up." },
                { "Custom (heel and toe)", "You choose the shift at the heel and at the toe (Heel and Toe knobs)." } } },
            { "ampTube", {
                { "12AX7 (high gain)", "Tighter and crisper, with more headroom." },
                { "12AT7 (medium)", "In between." },
                { "12AY7 (low gain)", "Softer and rounder, with earlier power-stage breakup." } } },
            { "ampBTube", {
                { "12AX7 (high gain)", "Tighter and crisper, with more headroom." },
                { "12AT7 (medium)", "In between." },
                { "12AY7 (low gain)", "Softer and rounder, with earlier power-stage breakup." } } },
            { "ampPwrTube", {
                { "As voiced", "The power section the amp model was voiced with." },
                { "EL34", "Crunchy upper mids and a little more power-stage grit." },
                { "6L6", "Tight and clean with lots of headroom, a touch scooped." },
                { "6V6", "Soft and spongy: breaks up early, sags a lot, rolled-off top." },
                { "EL84", "Bright and chimey, compressed, with a forward upper mid." },
                { "KT88", "Big and tight: the most headroom and the least sag." } } },
            { "ampBPwrTube", {
                { "As voiced", "The power section the amp model was voiced with." },
                { "EL34", "Crunchy upper mids and a little more power-stage grit." },
                { "6L6", "Tight and clean with lots of headroom, a touch scooped." },
                { "6V6", "Soft and spongy: breaks up early, sags a lot, rolled-off top." },
                { "EL84", "Bright and chimey, compressed, with a forward upper mid." },
                { "KT88", "Big and tight: the most headroom and the least sag." } } },
            { "lfShape", {
                { "Sine", "A smooth, steady warble." },
                { "Triangle", "A slightly more even pitch swing." },
                { "Random", "A new random pitch each cycle, with smooth steps: an unsteady, worn-tape wobble." } } },
            { "%sStack", {
                { "Stack as voiced", "The amp's own tone shape." },
                { "Neutral (flat)", "Most of the amp's own tone shape taken out, so the knobs start from nearly flat." },
                { "Passive stack: American combo", "A real passive bass/mid/treble stack solved from component values, in the style of a 100 W American combo: deep mid scoop that the Mid knob fills in, strong treble sweep, bass that needs to be turned up. The three knobs interact like the real ones. Mid is the average of Low Mid and High Mid; the amp's own tone shaping is replaced." },
                { "Passive stack: British 50 W", "The passive stack of a British master-volume 50 W lead amp: a mid dip around 500 to 800 Hz that the Mid knob fills in, a big treble range and a bass control that mostly sets the low end. The knobs interact like the real ones." },
                { "Passive stack: British lead", "The passive stack of a British-style 100 W lead amp: like the 50 W stack with slightly more slope, a deeper mid notch and more upper-mid weight." },
                { "Passive stack: American lead", "The stack of an American lead amp: a combo-style stack with a lighter load, so the response is a little more open." },
                { "Passive stack: chime (class A)", "A chime-style stack with big treble range and a bass control that sets how much low end gets through: open, glassy and mid-cut at the top. The 'Cut' character is approximated with the Treble knob turned down." },
                { "Passive stack: high-gain, 680 pF", "A two-channel high-gain amp's stack from its schematic: 220k treble, 1M bass, 22k mid, 47.5k slope, 680 pF treble cap and 0.02 uF caps. Close to the British lead stack with a slightly smaller treble pot and mid pot: a little darker and tighter. Mid is the average of Low Mid and High Mid." },
                { "Passive stack: high-gain, 500 pF", "The same stack with a 500 pF treble cap, as on that amp's second channel: a little brighter and more open at the top." } } },
            { "%sFeel", {
                { "Loose feel (low feedback)", "Sags and blooms: fuller, looser lows and a softer top end." },
                { "Normal feel", "The amp as voiced." },
                { "Tight feel (high feedback)", "Stiff and fast: tighter lows and more presence." } } },
            { "%sStack", {
                { "Stack as voiced", "The amp's own tone shape." },
                { "Neutral (flat)", "Most of the amp's own tone shape taken out, so the knobs start from nearly flat." },
                { "Passive stack: American combo", "A real passive bass/mid/treble stack solved from component values, in the style of a 100 W American combo: deep mid scoop that the Mid knob fills in, strong treble sweep, bass that needs to be turned up. The three knobs interact like the real ones. Mid is the average of Low Mid and High Mid; the amp's own tone shaping is replaced." },
                { "Passive stack: British 50 W", "The passive stack of a British master-volume 50 W lead amp: a mid dip around 500 to 800 Hz that the Mid knob fills in, a big treble range and a bass control that mostly sets the low end. The knobs interact like the real ones." },
                { "Passive stack: British lead", "The passive stack of a British-style 100 W lead amp: like the 50 W stack with slightly more slope, a deeper mid notch and more upper-mid weight." },
                { "Passive stack: American lead", "The stack of an American lead amp: a combo-style stack with a lighter load, so the response is a little more open." },
                { "Passive stack: chime (class A)", "A chime-style stack with big treble range and a bass control that sets how much low end gets through: open, glassy and mid-cut at the top. The 'Cut' character is approximated with the Treble knob turned down." },
                { "Passive stack: high-gain, 680 pF", "A two-channel high-gain amp's stack from its schematic: 220k treble, 1M bass, 22k mid, 47.5k slope, 680 pF treble cap and 0.02 uF caps. Close to the British lead stack with a slightly smaller treble pot and mid pot: a little darker and tighter. Mid is the average of Low Mid and High Mid." },
                { "Passive stack: high-gain, 500 pF", "The same stack with a 500 pF treble cap, as on that amp's second channel: a little brighter and more open at the top." } } },
            { "%sFeel", {
                { "Loose feel (low feedback)", "Sags and blooms: fuller, looser lows and a softer top end." },
                { "Normal feel", "The amp as voiced." },
                { "Tight feel (high feedback)", "Stiff and fast: tighter lows and more presence." } } },
            { "cabDynMode", {
                { "Tight (modern)", "Quick, controlled compression and a clean limit on the low end." },
                { "Vintage (spongy)", "Slower, softer, more compressed and more forgiving, like a vintage speaker working hard." },
                { "Real: tight", "Tight, plus a cone whose bass resonance rises and fades as it moves, a top that fades while the lows work hard, and a breakup in the upper mids." },
                { "Real: vintage", "Vintage, plus the same cone behaviours: stiffening, top loss and upper-mid breakup." } } },
            { "cabAmp", {
                { "Amp A", "This block hears only the main amp." },
                { "Amp B", "This block hears only the parallel amp." },
                { "Amp A + B", "This block hears both amps added together." } } },
            { "ampQuality", {
                { "Best (4x, more CPU)", "4x oversampling: the smoothest distortion, the most CPU and delay." },
                { "Balanced (2x)", "2x oversampling: about half the CPU of Best." },
                { "Lowest latency (no oversampling)", "No oversampling: no added delay and the least CPU, but high gain can sound fizzier." } } },
            { "acBody", {
                { "Body: Dreadnought", "A big, balanced steel-string body." },
                { "Body: Jumbo", "A bigger, lower, boomier body." },
                { "Body: Parlor / OM", "A small, focused, mid-forward body." },
                { "Body: Nylon classical", "A soft, warm nylon-string body." } } },
            { "cabSrc", {
                { "Built-in model", "The plugin builds this cab itself from a speaker, mic, position, distance and angle." },
                { "IR file", "Load your own impulse response (WAV or AIFF)." } } },
            { "cabCbn", {
                { "1x12 Open", "One 12-inch speaker in an open-back cabinet: airy and loose." },
                { "2x12 Open", "Two 12-inch speakers in an open-back cabinet: wide and chimey." },
                { "2x12 Closed", "Two 12-inch speakers in a closed cabinet: tighter lows." },
                { "4x12 Closed", "Four 12-inch speakers in a closed cabinet: big, tight low end." },
                { "4x12 Straight", "A 4x12 with a square front. The mic hears a more direct, upper-mid focused sound." },
                { "4x12 Angled", "A 4x12 with the top speakers angled up. A little less upper-mid focus and a touch more air and low warmth than the straight front. The difference is small." },
                { "4x12 Open Back", "Four 12-inch speakers in an open-back cabinet: airy and loose, with less bass than a closed cab." },
                { "Oversized 4x12 Straight (ported)", "A large, deep, ported 4x12 with a square front. The box resonance sits lower, so the lows are deeper and tighter, the boxy low mids are cleaned up and the upper mids stay focused. Good for downtuned and djent rhythm." },
                { "Oversized 4x12 Angled (ported)", "The same big ported cab with an angled front: the same deep, tight lows, with slightly smoother upper mids and a little more air." },
                { "1x12 Closed", "One 12-inch speaker in a closed cabinet: a tight, focused low end with more thump than the open back." },
                { "1x10 Open", "One 10-inch speaker in an open-back cabinet: small, quick and bright, with a thin low end." },
                { "2x10 Open", "Two 10-inch speakers in an open-back cabinet: snappy and open, with a scooped, airy low mid." },
                { "4x10 Closed", "Four 10-inch speakers in a closed cabinet: punchy and fast, with a firm low end and a defined top." },
                { "1x15 Closed", "One 15-inch speaker in a closed cabinet: big, round lows and a soft top, like a bass-leaning combo." },
                { "1x8 Small", "One 8-inch speaker in a tiny cabinet: very little low end and a honky, radio-like midrange." },
                { "Oversized 2x12 (ported)", "A deep, ported 2x12: the tight, clean low end of the oversized 4x12 in a smaller package." } } },
            { "cabSpk", {
                { "Modern Tight", "A tight, controlled low end with sharp upper mids, for modern high gain." },
                { "Vintage Crunch", "Warm and punchy with a rounded top end." },
                { "Rock Bite", "Aggressive upper mids with a lot of bite." },
                { "Smooth Vintage", "Soft, rounded and forgiving." },
                { "Chimey Alnico", "Bright and chimey with a vintage, open feel." },
                { "Vintage American", "Bright and glassy, American-style." },
                { "Hi-Fi Full", "Full and extended, the closest to flat." },
                { "Modern Scoop", "Big lows, scooped low mids and a bright top: modern, scooped rhythm tones." },
                { "Tight Ceramic", "Fast and tight with a strong upper-mid and presence attack; controlled low end." },
                { "Dark Thump", "Thick, extended lows and a rolled-off top. Good for very low tunings and taming fizz." },
                { "Hi-Def Light Cone", "Light, fast and detailed with extended highs for clear, articulate chords." },
                { "Smooth Lead", "Singing mids and a steeply rolled-off top, so very high gain stays smooth." },
                { "Mid Forward", "A strong midrange push around 1.8 kHz that cuts through a band mix." },
                { "Studio Standard", "A balanced mixed-4x12 voicing: firm low end, a little dip around 1.4 kHz and a defined upper-mid peak." },
                { "Studio Smooth Tight", "Like Studio Standard with less low end and a smoother top: tight and easy to place in a mix." },
                { "Heavy Bright", "A big low-mid bump, a dip around 600 Hz and a sharp top end: heavy and clear." },
                { "Heavy Tight", "Very little bass below 120 Hz, a controlled low mid and a strong upper-mid and presence attack." },
                { "Studio Hollow", "A deep dip around 670 Hz with a narrow peak just above it: the hollow, scooped character of some mixed cabs." },
                { "Rhythm Dark", "Big lows, a rolled-off top and a warm midrange: dark and thick for rhythm playing." },
                { "Rhythm Smooth", "Strong low end, a forward 600 Hz and a smooth, tamed top." },
                { "Compact Scooped", "A small-cab voicing with a scooped low mid and a hint of upper-mid bite." },
                { "Compact Balanced", "A small-cab voicing with a balanced midrange and a gentle top." },
                { "Compact Warm", "A small-cab voicing with an extended low end, a warm low mid and a restrained top." },
                { "Compact Dark", "A small-cab voicing with a thick low mid and a darker top." },
                { "Compact Round Low", "A small-cab voicing with a rounded, focused low end and little top." },
                { "Compact Open Air", "A small-cab voicing with an open, airy midrange and a gently rolled-off top." },
                { "Studio Dark", "A flat midrange with a smooth, dark top end: calm and easy on harsh amps." },
                { "Studio Thick Dark", "A big bump around 100 Hz and a steeply rolled-off top: thick, dark and tight." },
                { "Heavy Hi-Lo", "Big lows and a bright, forward top with a slightly scooped low mid." },
                { "Studio Warm Mid", "A warm, full low mid with a gentle top-end roll-off." },
                { "Studio Thick Bright", "A big bump around 100 Hz plus a strong upper-mid and presence lift: thick and bright." },
                { "Classic Dark", "A mid-heavy, quickly rolled-off voicing in an older rock style: warm and dark." },
                { "Classic Big Low", "Huge lows with a bright upper-mid lift: loose and boomy, from a close-mic'd old-school cab." },
                { "Classic Mid Focus", "Thin lows and a focused midrange with a smooth top: classic rock and hair metal." },
                { "Classic Punch", "A firm bump around 100 Hz and a dark top, in the style of older metal cabs: punchy and thick." },
                { "Classic Warm Presence", "Thick lows, a deep dip between 1 and 1.6 kHz and a presence lift near 1.8 kHz: warm, with the highs rolled off above 4 kHz." } ,
                { "Raw Big Low", "A raw, unmixed cab with a huge low-mid bump and a dark top. Bold: try Tilt towards bright, or a brighter mic." },
                { "Raw Bright Cap", "A raw, unmixed cab recorded at the centre of the cap: big lows and a very strong 3 to 5 kHz presence. Bold: try Position towards the edge, or Tilt towards dark." } } },
            { "cabMic", {
                { "Dynamic Bright", "A bright dynamic mic with a presence peak: cutting and aggressive." },
                { "Dynamic Full", "A fuller dynamic mic with more low mids." },
                { "Dynamic Presence", "A dynamic mic with a smooth presence lift." },
                { "Ribbon Smooth", "A ribbon mic: dark, smooth and rounded." },
                { "Condenser Detailed", "A condenser mic: detailed, extended highs." },
                { "Flat (no mic)", "No mic coloration: the speaker sound only." },
                { "Dynamic Aggressive", "A dynamic mic with a strong presence and air peak and less low end: cutting and tight." },
                { "Dynamic Punch", "A dynamic mic with a big low-end bump and strong presence: heavy and punchy, with a strong proximity effect." },
                { "Dynamic Scoop", "A dynamic mic with a built-in low-mid dip and extra top: scooped and clear." },
                { "Ribbon Dark", "A dark ribbon mic with thick lows and a rolled-off top. Smooths very bright cabs." },
                { "Condenser Warm", "A condenser mic with warm lows and gentle air: detailed without being harsh." },
                { "Blend (dynamic + ribbon)", "A blend of a dynamic and a ribbon mic: the dynamic's presence on top of the ribbon's body." },
                { "Ribbon Open", "A ribbon mic measured on real cabs: a low-mid bump, a small dip around 1.3 kHz and a smooth roll-off above 5 kHz." },
                { "Condenser Small", "A small condenser measured on real cabs: slightly less low mid and a notch of presence around 5 kHz, with a wide-open top." },
                { "Condenser Large", "A large condenser measured on real cabs: more low end and a lift around 7 kHz, rolled off above 6 kHz." },
                { "Dynamic Large", "A large-diaphragm dynamic measured on real cabs: firmer lows and a clear presence lift near 4 kHz." },
                { "Dynamic Long Throw", "A broadcast-style dynamic measured on real cabs: strong low end and a bright upper-mid and air lift with a steep top roll-off." },
                { "Dynamic Hyper", "A tight-pattern dynamic measured on real cabs: extra low end and a pronounced 2.7 kHz lift." },
                { "Dynamic Wide", "A dynamic mic measured on real cabs: full lows with an upper-mid lift and a top that rolls off from 5 kHz." } } },
            { "bsMode", {
                { "Clean", "An op-amp boost from published component values: 0 to 26 dB of gain with almost no colour; it only clips when the op-amp runs out of swing. The Saturation knob does nothing here." },
                { "EP", "Warm, bass-heavy, gently asymmetric: the sound of a small tube preamp. The top is rolled off a little." },
                { "Console", "Transformer-style: the lows saturate earlier than the highs, symmetrical and smooth, with a little silk on top." },
                { "Tube", "An asymmetric triode-style stage with even and odd harmonics and a touch of top-end loss." },
                { "Treble", "A one-transistor treble booster from published component values: bass is cut below about 3 kHz before the gain (up to 38 dB) and the stage clips lopsidedly. Saturation moves the input capacitor from 5 nF (stock, thin) to 47 nF (fuller)." },
                { "Mid", "A mid-forward boost: trims the bass and pushes around 900 Hz with a gentle clip, so leads cut through." },
                { "FET", "A transparent solid-state stage with very little top loss and mostly even harmonics. Keeps the guitar's own character." } } },
            { "bsTightStyle", {
                { "Mid hump", "A low cut plus a mid hump around 720 Hz: tight and vocal." },
                { "Low cut", "Only a low cut, with no other colour." },
                { "Focus", "A low cut plus a push around 1.5 kHz, for pick attack and note definition." } } },
            { "bsEqPos", {
                { "Before saturation", "The EQ shapes what feeds the saturation." },
                { "After saturation", "The EQ shapes the finished sound." } } },
            { "mfAType", {
                { "Flanger", "A short swept delay with feedback: positive rings, negative sounds hollow." },
                { "Through-zero flanger", "The dry signal is delayed as well, so the sweep passes through zero: the deep, jet-like null. Adds a fixed 4 ms delay." },
                { "Vibrato", "A swept delay with no dry signal: pure pitch wobble." },
                { "Uni-V", "Four swept all-pass stages with a lamp-like lag: a throbbing phase-vibrato. Half Mix = phase-chorus notches, full Mix = vibrato." },
                { "Rotary", "A rotating horn and drum with speed inertia. Rate is the horn speed; Manual balances horn and drum." },
                { "Auto-pan", "Pans between the left and right (a plain tremolo if the signal is mono)." },
                { "Harmonic tremolo", "The lows and the highs pulse in opposite directions." } } },
            { "mfAPos", {
                { "Before the amp", "The effect feeds the amp (so it is affected by the amp's gain)." },
                { "After the cab", "The effect is applied to the finished sound, in stereo." } } },
            { "mfAShape", {
                { "Sine", "A smooth sweep." },
                { "Triangle", "An even, linear sweep." },
                { "Soft square", "Jumps between the two ends with smooth edges." } } },
            { "preDelayHoldMode", {
                { "Freeze", "The delay loops what it holds and ignores new playing." },
                { "Infinite", "The repeats never die away, and new playing keeps adding to them." } } },
            { "preDelayTaps", {
                { "Off", "No extra taps." },
                { "Half + full", "One extra tap halfway to the main repeat." },
                { "Thirds", "Taps at one third and two thirds." },
                { "Quarters", "Three taps at a quarter, a half and three quarters." },
                { "Off-beat", "Taps at a quarter and three quarters." },
                { "Dotted cluster", "Three uneven taps: a dotted rhythm." } } },
            { "hmPos", {
                { "Before the amp", "Mono: the harmony goes into the amp's gain with your playing." },
                { "After the cab", "Stereo: the harmony is added to the finished sound, with a pan for each voice." } } },
            { "hmTrack", {
                { "Single note", "One note at a time. Fast (about 14 ms) and clean for lead lines. A chord will confuse it." },
                { "Chords", "Follows up to 6 notes at once. About 90 ms of delay, shown to your DAW. Best for full chords and low strings." },
                { "Chords (fast)", "Chords with about 45 ms of delay. Good for upper-string voicings; low notes close together blur." } } },
            { "hmMode", {
                { "Interval", "Each voice is a fixed number of semitones away." },
                { "Scale", "Each voice follows your note up or down the scale you choose." } } },
            { "hmScale", {
                { "Major", "Seven notes, bright and happy." },
                { "Natural minor", "Seven notes, dark." },
                { "Dorian", "Minor with a lifted sixth: jazzy and funky." },
                { "Mixolydian", "Major with a flat seventh: bluesy rock." },
                { "Harmonic minor", "Minor with a raised seventh: dramatic and neo-classical." },
                { "Major pentatonic", "Five notes: sweet, country-style harmonies." },
                { "Minor pentatonic", "Five notes: the rock and blues scale." } } },
            { "riffMode", {
                { "DI (before the chain)", "Records the guitar at the input; the playback goes through everything." },
                { "Full tone (after the chain)", "Records the finished sound at the output; the playback is added at the output." } } },
            { "postDelayStyle", {
                { "Forward", "Normal repeats." },
                { "Reverse", "Each repeat is played backwards and swells up to the delay time." } } },
            { "reverbDragMode", {
                { "Smooth", "Any pitch in between." },
                { "Semitones", "Whole semitones only." },
                { "Fifths and octaves", "Only fifths and octaves." },
                { "Octaves", "Only octaves." } } },
            { "reverbSymphPitch", {
                { "Octave up", "A shimmering layer an octave above." },
                { "Octave down", "A deep layer an octave below." },
                { "Fifth up", "A layer a fifth above." },
                { "Two octaves up", "A very high, glassy layer." } } },
            { "granScale", {
                { "Free", "Any pitch." },
                { "Octaves", "Only octaves (and the root)." },
                { "Fifths", "Root and fifth." },
                { "Major pentatonic", "A five-note major scale: hard to play a wrong note." },
                { "Minor pentatonic", "A five-note minor scale." },
                { "Major", "The seven-note major scale." },
                { "Minor", "The seven-note natural minor scale." },
                { "Whole tone", "A dreamy six-note scale with no resolution." } } },
            { "granShape", {
                { "Smooth", "An even fade in and out." },
                { "Swell", "A slow fade in and a quick end." },
                { "Pluck", "A sharp start and a long decay." },
                { "Flat", "Almost no fade: percussive and buzzy." } } },
            { "tpRange", {
                { "6-string", "The default: tuned for a standard 6-string guitar." },
                { "7-string", "Follows a low B string (about 58 Hz)." },
                { "8-string", "Follows a low F# string (about 40 Hz)." } } },
            { "reverbType", {
                { "Classic", "The original simple reverb. Its decay tops out at about 12 seconds." },
                { "Room", "A small, close space." },
                { "Hall", "A large concert hall: smooth and spacious." },
                { "Plate", "A bright, dense plate reverb with no early reflections." },
                { "Ambient", "A diffuse, blooming wash for pads and soundscapes." },
                { "Cathedral", "A huge, slowly decaying stone space." },
                { "Spring", "A twangy, boingy spring-tank style reverb." },
                { "Soft Plate", "A dense, gently moving plate: smooth and lush." },
                { "Tumble", "Echoes that tumble over each other, with a loose, uneven tail." },
                { "Abyss", "Huge, dark and slowly drifting." } } },
            { "postDelayStereo", {
                { "Stereo (same time)", "Left and right echo together at the same time, each on its own side." },
                { "Ping-Pong (left first)", "The first repeat is on the left, then it bounces right, left, right..." },
                { "Ping-Pong (right first)", "The first repeat is on the right, then it bounces left, right, left..." },
                { "Dual (right offset)", "The right side echoes at a different time to the left (set with R Offset) for a wide, rhythmic spread." } } },
            { "shimmerMode", {
                { "Feedback (blooming)", "The reverb's output is shifted and fed back in, so each repeat climbs another step: the classic rising shimmer." },
                { "Input (no build-up)", "Only your playing is shifted, once, into the reverb: a clean octave (or interval) pad that does not climb." },
                { "Both", "Shifts your playing and the reverb's tail together: the fullest shimmer." } } },
            { "shimmerPitch", {
                { "+12 (octave)", "The classic shimmer: each trip around the loop goes up an octave." },
                { "+7 (fifth)", "Each trip goes up a fifth: a more harmonic, open shimmer." },
                { "+24 (two octaves)", "Each trip goes up two octaves: very bright and airy." },
                { "-12 (octave down)", "Each trip goes down an octave: a dark, deep, swelling sub-shimmer." },
                { "+12 & +7 (octave + fifth)", "Two voices at once, an octave and a fifth up: open, harmonic, almost string-like." },
                { "+12 & -12 (up and down)", "An octave up and an octave down together: huge and hollow." },
                { "+5 (fourth)", "Each trip goes up a fourth: a suspended, floating shimmer." },
                { "+19 (octave + fifth up)", "A twelfth up (an octave and a fifth) in one voice: bright and bell-like." } } },
            { "preDelayMode", {
                { "Digital", "Clean and bright, with repeats that stay clear (16 kHz)." },
                { "Analog", "Darker repeats with soft saturation and a little wobble, like a bucket-brigade delay." },
                { "Tape", "Darker still, with strong saturation and plenty of wow and flutter." },
                { "Lo-Fi", "Dark, with reduced sample rate and bit depth: crunchy, degraded repeats." } } },
            { "widMode", {
                { "Wide: Classic", "The default widening: a balanced amount of smearing and lift." },
                { "Wide: Desktop", "Tuned for small speakers: more lift in the low mids and highs." },
                { "Wide: Rack", "A longer, more spacious spread." } } },
            { "tremMode", {
                { "Tremolo (both sides)", "Left and right go up and down together: a classic tremolo." },
                { "Stereo (sides alternate)", "Left and right alternate: an auto-pan feel." } } },
            { "tremShape", {
                { "Sine", "A smooth, round pulse." },
                { "Triangle", "A slightly sharper, more even pulse." },
                { "Square (soft)", "A choppy on/off pulse with softened edges." },
                { "Optical (bulb and photocell)", "The vibrato of a classic combo: a lamp lit by the oscillator and a light-sensitive cell. Rounded, a little lopsided (quick on, slow off) and shallower the faster you go." } } },
            { "wahMode", {
                { "Manual (Position)", "You set the filter with the Position knob (automate it from your DAW like a pedal)." },
                { "Auto: Envelope", "Playing harder opens the wah (an auto-wah)." },
                { "Auto: LFO", "The wah sweeps by itself at the LFO rate." } } },
            { "phaserStages", {
                { "2 stages", "A single, subtle notch." },
                { "4 stages (classic)", "The classic two-notch phaser sound." },
                { "6 stages", "Deeper, with three notches." },
                { "8 stages", "A thicker, more pronounced sweep." },
                { "10 stages", "A very deep, rich sweep." },
                { "12 stages", "The deepest, most intense sweep." } } },
            { "phaserShape", {
                { "Sine", "A smooth, steady sweep." },
                { "Triangle", "A linear sweep with a slightly different feel at the ends." },
                { "Random", "A smoothed random sweep that wanders." } } },
            { "chorusMode", {
                { "I", "The gentler, slower chorus setting." },
                { "II", "The deeper, faster chorus setting." },
                { "I + II", "Both settings together for a wide, lush sound." },
                { "Dimension", "A very slow, shallow, two-line widener. You hear a soft thickening more than a sweep. Try Mix at 0.3 to 0.5." },
                { "Tri-chorus", "Three lines a third of a cycle apart: a smooth, wide, swirling chorus." },
                { "Ensemble", "A string-machine style ensemble: three lines with a slow sweep and a faint fast shimmer on top." },
                { "Detune", "Two longer lines drifting slowly in opposite directions, a few cents sharp and flat, like a doubled take." } } },
            { "eqMode", {
                { "Graphic", "All nine bands have the same medium Q (1.4)." },
                { "Mod (custom Q)", "Each band has its own Q, narrower in the mids and wider at the ends. The Q knob scales them." } } },
            { "eqSlope", {
                { "12 dB/oct", "A gentle low cut / high cut slope." },
                { "24 dB/oct", "A steeper low cut / high cut slope." } } },
        };
        return table;
    }

    // Note-value menus (delays, LFOs, grains, reverb pre-delay): the text after any "Label: " prefix.
    const ItemTips& noteTips()
    {
        static const ItemTips tips =
        {
            { "1/2", "A half note (2 beats at the tempo)." },
            { "Dotted 1/4", "A dotted quarter note (1.5 beats)." },
            { "1/4", "A quarter note (1 beat)." },
            { "1/4 triplet", "A quarter-note triplet (2/3 of a beat)." },
            { "Dotted 1/8", "A dotted eighth note (3/4 of a beat)." },
            { "1/8", "An eighth note (1/2 beat)." },
            { "1/8 triplet", "An eighth-note triplet (1/3 of a beat)." },
            { "Dotted 1/16", "A dotted sixteenth note (3/8 of a beat)." },
            { "1/16", "A sixteenth note (1/4 beat)." },
            { "1/16 triplet", "A sixteenth-note triplet (1/6 of a beat)." },
        };
        return tips;
    }
}

juce::String comboItemTip (const juce::String& paramId, const juce::String& itemText)
{
    juce::String id = paramId;
    if (id.length() > 4 && juce::String ("ABCD").containsChar (id.getLastCharacter())
        && (id.startsWith ("cabSrc") || id.startsWith ("cabSpk") || id.startsWith ("cabCbn") || id.startsWith ("cabMic") || id.startsWith ("cabAmp")))
        id = id.dropLastCharacters (1);
    if (id == "postDelayMode") id = "preDelayMode";
    if (id == "postDelayHoldMode") id = "preDelayHoldMode";
    if (id == "postDelayTaps") id = "preDelayTaps";
    if (id == "mfBType" || id == "mfBPos" || id == "mfBShape")
        id = "mfA" + id.substring (3);

    if (id.endsWith ("Note"))
    {
        const juce::String text = itemText.contains (": ") ? itemText.fromLastOccurrenceOf (": ", false, false) : itemText;
        if (text.startsWith ("Free"))
            return "Free-running: use the knob next to this menu, not the tempo.";
        for (auto& entry : noteTips())
            if (text == entry.first)
                return juce::String (entry.second) + " Locks the setting to the tempo (TAP, BPM or HOST TEMPO) and greys out the knob.";
        return {};
    }

    const auto& table = itemTipTable();
    auto it = table.find (id.startsWith ("od2") ? ("od" + id.substring (3)).toStdString() : id.startsWith ("fuzz2") ? ("fuzz" + id.substring (5)).toStdString() : id.toStdString());
    if (it == table.end())
        return {};
    for (auto& entry : it->second)
        if (itemText == entry.first)
            return entry.second;
    return {};
}

//==============================================================================
// A menu item that shows its own hover help.
//==============================================================================
namespace
{
    class TipItem : public juce::PopupMenu::CustomComponent, public juce::SettableTooltipClient
    {
    public:
        TipItem (const juce::String& itemText, int itemId, juce::ComboBox* owner, const juce::String& tip)
            : juce::PopupMenu::CustomComponent (true), text (itemText), id (itemId), box (owner)
        {
            setTooltip (tip);
        }

        void getIdealSize (int& width, int& height) override
        {
            getLookAndFeel().getIdealPopupMenuItemSize (text, false, -1, width, height);
        }

        void paint (juce::Graphics& g) override
        {
            const bool ticked = box != nullptr && box->getSelectedId() == id;
            getLookAndFeel().drawPopupMenuItem (g, getLocalBounds(), false, true, isItemHighlighted(), ticked, false,
                                                text, {}, nullptr, nullptr);
        }

    private:
        juce::String text;
        int id;
        juce::Component::SafePointer<juce::ComboBox> box;
    };
}

void TipCombo::showPopup()
{
    // Give every item that has a description its own hover help, then let JUCE show the menu as usual.
    if (auto* menu = getRootMenu())
        for (juce::PopupMenu::MenuItemIterator it (*menu, true); it.next();)
        {
            auto& item = it.getItem();
            if (item.itemID == 0)
                continue;
            const auto tip = comboItemTip (paramId, item.text);
            if (tip.isNotEmpty())
                item.setCustomComponent (new TipItem (item.text, item.itemID, this, tip));
        }
    juce::ComboBox::showPopup();
}

juce::String TipCombo::getTooltip()
{
    const auto base = juce::ComboBox::getTooltip();
    const auto tip  = comboItemTip (paramId, getText());
    return tip.isEmpty() ? base : base + "\n" + getText() + ": " + tip;
}

juce::String tipFor (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId)
{
    const auto& table = tooltipTable();
    auto find = [&table] (const std::string& key) -> const char*
    {
        auto it = table.find (key);
        return it != table.end() ? it->second : nullptr;
    };

    if (auto* text = find (paramId.toStdString()))
        return text;
    if (paramId.startsWith ("od2"))                       // Drive 2 is a copy of Drive 1: same descriptions
        if (auto* text = find ("od" + paramId.substring (3).toStdString()))
            return text;
    if (paramId.startsWith ("fuzz2"))                     // so is Fuzz 2
    {
        if (paramId == "fuzz2BuzzLow")  return find ("buzzLow");
        if (paramId == "fuzz2BuzzHigh") return find ("buzzHigh");
        if (paramId.startsWith ("fuzz2Buzz")) if (auto* t = find ("buzz" + paramId.substring (9).toStdString())) return t;
        if (auto* text = find ("fuzz" + paramId.substring (5).toStdString()))
            return text;
    }

    // Cab block controls end in A to D (cabSpkB, cabLevelC...); they share one description each.
    if (paramId.startsWith ("cab") && paramId.length() > 4 && juce::String ("ABCD").containsChar (paramId.getLastCharacter()))
        if (auto* text = find ("blk_" + paramId.dropLastCharacters (1).toStdString()))
            return text;

    // The nine EQ faders
    if (paramId.startsWith ("eqB") && paramId.length() == 4)
        if (auto* text = find ("eqBand"))
            return juce::String (text) + " (band " + paramId.getLastCharacter() + " of 9.)";

    if (auto* param = apvts.getParameter (paramId))
        return param->getName (64);
    return {};
}

//==============================================================================
// Look and feel: "haze". A deep violet-black window with soft coloured glows, frosted
// panels with a gradient edge, glowing halo knobs, LED switches and pill-shaped menus.
// Everything is drawn in code (no image files), so changing the palette in PluginEditor.h
// re-skins it.
//==============================================================================
namespace
{
    // A soft glow around a path: a few wider, fainter strokes underneath.
    void strokeGlow (juce::Graphics& g, const juce::Path& path, juce::Colour c, float width)
    {
        g.setColour (c.withAlpha (0.10f));
        g.strokePath (path, juce::PathStrokeType (width + 7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (c.withAlpha (0.18f));
        g.strokePath (path, juce::PathStrokeType (width + 3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Point<float> at0 (float cx, float cy, float r, float a)
    {
        return { cx + r * std::sin (a), cy - r * std::cos (a) };
    }

    // A little indicator light: lit with a halo, or a dark ring.
    void drawLed (juce::Graphics& g, juce::Point<float> c, float radius, bool on, float alpha)
    {
        if (on)
        {
            const float haloR = radius * 3.4f;
            g.setGradientFill (juce::ColourGradient (ui::accent.withAlpha (0.50f * alpha), c.x, c.y,
                                                     ui::accent.withAlpha (0.0f), c.x + haloR, c.y, true));
            g.fillEllipse (c.x - haloR, c.y - haloR, haloR * 2.0f, haloR * 2.0f);
            g.setColour (ui::accent.brighter (0.35f).withMultipliedAlpha (alpha));
            g.fillEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
        }
        else
        {
            g.setColour (ui::background.withMultipliedAlpha (alpha));
            g.fillEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f);
            g.setColour (ui::track.brighter (0.2f).withMultipliedAlpha (alpha));
            g.drawEllipse (c.x - radius, c.y - radius, radius * 2.0f, radius * 2.0f, 1.2f);
        }
    }


    // The look of each amp's panel and its cab blocks (generic descriptions of the usual styling of these kinds of amp):
    //   0 Clean: black tolex, silver piping and plate, sparkle cloth, chrome caps, icy-blue ticks
    //   1 Vintage Chime: dark tolex, cream piping, tan diamond cloth, cream plate
    //   2 British Hot-Rod: black tolex, gold piping and plate, salt-and-pepper cloth, gold caps
    //   3 Saturated Lead: black tolex, chrome piping and plate, woven basket cloth, red ticks
    //   4 Modern High-Gain: matte black, white piping, dark anodised plate, pinstripe cloth, blue-lit caps
    //   5 Classic Rock: orange tolex, cream piping, black plate, basket cloth, cream caps
    //   6 Boutique Lead: black tolex, pale-gold piping, silver-cream plate, wheat basket cloth, amber ticks
    //   7 Tight Metal: matte black, red piping, dark plate with white lettering, dark diamond cloth, red ticks
    //   8 Low-Watt Combo: dark brown tolex, tan piping, cream plate, brown cloth, warm ticks
    //   9 Glass Clean: black vinyl, silver piping, silver plate, grey sparkle cloth, teal ticks
    //  10 Creamy Lead: black tolex, white piping, dark brushed plate, dark wicker cloth, green ticks
    //  11 Doom Stack: charcoal tolex, bone piping, dark plate, black salt-and-pepper cloth, violet ticks
    //  12 Precision High-Gain: warm graphite tolex, copper piping, black plate with amber lettering, pinstripe cloth, orange ticks
    //  13 Digital Extreme: blue-black, ice-white piping, dark glass plate with cyan lettering, dot-matrix cloth, cyan ticks
    //  14 Master-Volume Rock: black tolex, gold piping, brushed-gold plate with black lettering, dark basket cloth, pale-amber ticks
    // (a function maps an amp model to its look: themeForModel, below)
    struct AmpTheme
    {
        juce::uint32 tolexTop, tolexBottom, pipeA, pipeB, plateA, plateB, plateText;
        juce::uint32 clothTop, clothBottom, fleck;
        int clothStyle;          // 0 sparkle, 1 diamond, 2 salt and pepper, 3 basket, 4 pinstripe, 5 dot matrix
        juce::uint32 capA, capB, tick;
    };

    const AmpTheme& ampThemeFor (int index)
    {
        static const AmpTheme themes[] =
        {
            { 0xff1c1c1f, 0xff0b0b0c, 0xffe3e6ea, 0xff8c929b, 0xffe9ecf0, 0xffa7adb6, 0xff15161a, 0xff23242a, 0xff121317, 0xffbfc5cf, 0, 0xfff4f6f8, 0xff868c96, 0xff8ad4ff },
            { 0xff261d15, 0xff0f0b08, 0xffe8dcb8, 0xff8c7c52, 0xffefe2b8, 0xffb59d66, 0xff2b1a08, 0xff8f7344, 0xff4d3b1f, 0xffd8c28a, 1, 0xfff0e6c8, 0xff8f7e52, 0xffffbf5a },
            { 0xff1d1d20, 0xff0b0b0c, 0xffe9c76c, 0xff8a6a26, 0xffe9c96f, 0xffa07d2e, 0xff2b1e06, 0xff2a2620, 0xff14110d, 0xffd8c690, 2, 0xfff0d484, 0xff8a6a22, 0xffff8a63 },
            { 0xff18181b, 0xff09090a, 0xffd5d8dd, 0xff747a84, 0xffd8dce2, 0xff7d838d, 0xff0f1013, 0xff6c5535, 0xff382a16, 0xffa88a56, 3, 0xffe6e9ee, 0xff737985, 0xffff5a3a },
            { 0xff111113, 0xff050506, 0xfff4f4f4, 0xffa9a9a9, 0xff303238, 0xff0f1012, 0xffeef0f4, 0xff101012, 0xff060607, 0xff3a3d44, 4, 0xff3a6fb8, 0xff112a52, 0xff4aa8ff },
            { 0xffc9661c, 0xff8f4410, 0xfff0e6c8, 0xff9a8a60, 0xff1b1b1d, 0xff0c0c0d, 0xfff0e6c8, 0xff4a3a1c, 0xff241a0a, 0xff8c7440, 3, 0xfff0e6c8, 0xff8a7a50, 0xffffa040 },
            { 0xff1b1a1c, 0xff0a0a0b, 0xffd4c7a0, 0xff857a5a, 0xffd9d5c8, 0xff9a9586, 0xff1c1b17, 0xffb89a64, 0xff7a6236, 0xffdcc58e, 3, 0xffe3d7b2, 0xff8a7e58, 0xffffc766 },
            { 0xff101011, 0xff040405, 0xffc8382e, 0xff6e1c18, 0xff1e1e21, 0xff0a0a0b, 0xffe8e8ea, 0xff17171a, 0xff08080a, 0xff3c3c42, 1, 0xffd6d6da, 0xff5a5a5e, 0xffff4034 },
            { 0xff4a3a1e, 0xff241a0a, 0xffe0c070, 0xff8a6a2a, 0xffe9dcb4, 0xffb59d66, 0xff2b1a08, 0xffb89a64, 0xff7a6236, 0xffe6d3a0, 2, 0xfff0e6c8, 0xff8a7a50, 0xffffd27a },
            { 0xff222427, 0xff0e0f10, 0xffd5d8dd, 0xff8c929b, 0xffd3d6db, 0xff9aa0a8, 0xff111214, 0xff9aa0a8, 0xff5f656d, 0xffe6e9ee, 0, 0xfff4f6f8, 0xff868c96, 0xff3fe0c8 },
            { 0xff17171a, 0xff08080a, 0xffe8e8e8, 0xff9a9a9a, 0xff2a2c30, 0xff101113, 0xffe9ecf0, 0xff3a2c1a, 0xff150f08, 0xff6b5636, 3, 0xffdcdfe4, 0xff7b7f87, 0xff7ee06a },
            { 0xff2a2a30, 0xff0f0f13, 0xffe6dfc6, 0xff8a846c, 0xff1d1d22, 0xff0a0a0c, 0xffe6dfc6, 0xff1a1a1e, 0xff070709, 0xff4a4a54, 2, 0xffd9d3be, 0xff5e5a4a, 0xffb06aff },
            { 0xff26211f, 0xff0d0b0a, 0xffd0763a, 0xff7a3d18, 0xff161412, 0xff0a0908, 0xffe8a86a, 0xff1b1816, 0xff0a0908, 0xff4a4038, 4, 0xffc9743a, 0xff55260e, 0xffff7a2a },
            { 0xff0e1218, 0xff04060a, 0xffe6f4ff, 0xff8aa0b4, 0xff0b0f14, 0xff030507, 0xff7ef0ff, 0xff0a0e13, 0xff03050a, 0xff2c5468, 5, 0xff2a3a4a, 0xff0a121a, 0xff23e8ff },
            { 0xff141415, 0xff060607, 0xffe6c870, 0xff8a6e2a, 0xffd9b86a, 0xffa8863a, 0xff1a1206, 0xff2b2118, 0xff100c07, 0xffb9965a, 3, 0xffe9d6a0, 0xff8a7442, 0xffffe27a },
        };
        return themes[juce::jlimit (0, 14, index)];
    }

    // The look for an amp model (the ampModel parameter's value): Acoustic wears the Vintage Chime look,
    // Classic Rock (6) is look 5, the two v54 amps (7, 8) are looks 6 and 7, the four v55 amps (9 to 12) are looks 8 to 11,
    // the two v61 amps (13, 14) are looks 12 and 13, and the v62 Master-Volume Rock amp (15) is look 14.
    int themeForModel (int model)
    {
        return model == 5 ? 1 : model >= 6 ? juce::jmin (model - 1, 14) : juce::jlimit (0, 4, model);
    }

    // Upper-case text with a little extra space between the letters (used for titles).
    void drawSpaced (juce::Graphics& g, const juce::String& text, float x, float y, float height, float spacing,
                     float maxX, const juce::Font& font)
    {
        g.setFont (font);
        for (auto c : text.toUpperCase())
        {
            const auto s = juce::String::charToString (c);
            const float w = juce::GlyphArrangement::getStringWidth (font, s);
            if (x + w > maxX)
                break;
            g.drawText (s, juce::Rectangle<float> (x - 1.0f, y, w + 2.0f, height), juce::Justification::centred);
            x += w + spacing;
        }
    }

    // ---- v61: pedal art. Simple hand-drawn-style line motifs (eye, sun, moon, waves, mountains, spiral...) drawn with paths
    //      in a given colour, used on the pedal enclosure bands and as faint watermarks. All shapes are my own drawings.
    void drawPedalMotif (juce::Graphics& g, int motif, juce::Rectangle<float> a, juce::Colour c, float sw)
    {
        const float cx = a.getCentreX(), cy = a.getCentreY();
        const float rr = 0.5f * std::min (a.getWidth(), a.getHeight());
        g.setColour (c);
        const juce::PathStrokeType st (sw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        auto star = [&] (float x, float y, float s)
        {
            juce::Path p;
            p.startNewSubPath (x, y - s);  p.quadraticTo (x, y, x + s, y);  p.quadraticTo (x, y, x, y + s);
            p.quadraticTo (x, y, x - s, y);  p.quadraticTo (x, y, x, y - s);  p.closeSubPath();
            g.fillPath (p);
        };
        switch (motif % 9)
        {
            case 0:                                                    // an eye
            {
                juce::Path p;
                p.startNewSubPath (a.getX(), cy);
                p.quadraticTo (cx, cy - rr * 1.5f, a.getRight(), cy);
                p.quadraticTo (cx, cy + rr * 1.5f, a.getX(), cy);
                g.strokePath (p, st);
                g.drawEllipse (cx - rr * 0.48f, cy - rr * 0.48f, rr * 0.96f, rr * 0.96f, sw);
                g.fillEllipse (cx - rr * 0.2f, cy - rr * 0.2f, rr * 0.4f, rr * 0.4f);
                for (int i = -2; i <= 2; ++i)
                    g.drawLine (cx + (float) i * rr * 0.5f, cy - rr * (1.0f - 0.12f * (float) i * (float) i), cx + (float) i * rr * 0.62f, cy - rr * (1.25f - 0.12f * (float) i * (float) i), sw);
                break;
            }
            case 1:                                                    // a sun with rays
                g.drawEllipse (cx - rr * 0.38f, cy - rr * 0.38f, rr * 0.76f, rr * 0.76f, sw);
                for (int i = 0; i < 14; ++i)
                {
                    const float ang = (float) i * juce::MathConstants<float>::twoPi / 14.0f;
                    const float r0 = rr * (i % 2 ? 0.55f : 0.50f), r1 = rr * (i % 2 ? 0.80f : 1.0f);
                    g.drawLine (cx + r0 * std::cos (ang), cy + r0 * std::sin (ang), cx + r1 * std::cos (ang), cy + r1 * std::sin (ang), sw);
                }
                break;
            case 2:                                                    // a crescent moon and stars
            {
                juce::Path moon;
                moon.addEllipse (cx - rr * 0.7f, cy - rr * 0.7f, rr * 1.4f, rr * 1.4f);
                juce::Path bite;
                bite.addEllipse (cx - rr * 0.35f, cy - rr * 0.85f, rr * 1.3f, rr * 1.3f);
                moon.addPath (bite);
                moon.setUsingNonZeroWinding (false);
                g.fillPath (moon);
                star (cx + rr * 0.75f, cy - rr * 0.5f, rr * 0.22f);
                star (cx + rr * 0.45f, cy + rr * 0.55f, rr * 0.15f);
                star (cx - rr * 0.95f, cy + rr * 0.6f, rr * 0.12f);
                break;
            }
            case 3:                                                    // waves
                for (int k = 0; k < 3; ++k)
                {
                    juce::Path p;
                    const float y0 = cy + ((float) k - 1.0f) * rr * 0.55f;
                    p.startNewSubPath (a.getX(), y0);
                    for (int i = 1; i <= 24; ++i)
                    {
                        const float fx = (float) i / 24.0f;
                        p.lineTo (a.getX() + fx * a.getWidth(), y0 + rr * 0.2f * std::sin (fx * 14.0f + (float) k));
                    }
                    g.strokePath (p, st);
                }
                break;
            case 4:                                                    // mountains under a sun
            {
                juce::Path p;
                p.startNewSubPath (a.getX(), a.getBottom());
                p.lineTo (a.getX() + a.getWidth() * 0.28f, cy - rr * 0.1f);
                p.lineTo (a.getX() + a.getWidth() * 0.45f, cy + rr * 0.35f);
                p.lineTo (a.getX() + a.getWidth() * 0.65f, a.getY() + rr * 0.1f);
                p.lineTo (a.getRight(), a.getBottom());
                g.strokePath (p, st);
                g.drawEllipse (a.getX() + a.getWidth() * 0.62f, a.getY() - rr * 0.35f, rr * 0.5f, rr * 0.5f, sw);
                break;
            }
            case 5:                                                    // an eye in a triangle
            {
                juce::Path t;
                t.addTriangle (cx, a.getY(), a.getRight(), a.getBottom(), a.getX(), a.getBottom());
                g.strokePath (t, st);
                g.fillEllipse (cx - rr * 0.16f, cy + rr * 0.15f - rr * 0.16f, rr * 0.32f, rr * 0.32f);
                juce::Path e;
                e.startNewSubPath (cx - rr * 0.42f, cy + rr * 0.15f);
                e.quadraticTo (cx, cy - rr * 0.35f, cx + rr * 0.42f, cy + rr * 0.15f);
                e.quadraticTo (cx, cy + rr * 0.65f, cx - rr * 0.42f, cy + rr * 0.15f);
                g.strokePath (e, st);
                break;
            }
            case 6:                                                    // a spiral
            {
                juce::Path p;
                for (int i = 0; i <= 120; ++i)
                {
                    const float t = (float) i / 120.0f, ang = t * 6.0f * juce::MathConstants<float>::twoPi * 0.5f;
                    const juce::Point<float> pt (cx + rr * t * std::cos (ang), cy + rr * t * std::sin (ang));
                    if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
                }
                g.strokePath (p, st);
                break;
            }
            case 7:                                                    // halftone dots
                for (int iy = 0; iy < 4; ++iy)
                    for (int ix = 0; ix < 7; ++ix)
                    {
                        const float d = 0.18f + 0.62f * (float) ix / 6.0f;
                        const float fx = a.getX() + (float) ix * a.getWidth() / 6.0f, fy = a.getY() + 0.1f * a.getHeight() + (float) iy * a.getHeight() * 0.27f;
                        g.fillEllipse (fx - rr * 0.16f * d * 1.6f, fy - rr * 0.16f * d * 1.6f, rr * 0.32f * d * 1.6f, rr * 0.32f * d * 1.6f);
                    }
                break;
            default:                                                   // a lightning bolt
            {
                juce::Path p;
                p.startNewSubPath (cx + rr * 0.15f, a.getY());
                p.lineTo (cx - rr * 0.5f, cy + rr * 0.1f);  p.lineTo (cx - rr * 0.05f, cy + rr * 0.1f);
                p.lineTo (cx - rr * 0.2f, a.getBottom());   p.lineTo (cx + rr * 0.55f, cy - rr * 0.15f);
                p.lineTo (cx + rr * 0.1f, cy - rr * 0.15f); p.closeSubPath();
                g.strokePath (p, st);
                break;
            }
        }
    }

    int motifForTitle (const juce::String& title)
    {
        const auto t = title.toLowerCase();
        static const std::pair<const char*, int> keys[] = { { "fuzz", 6 }, { "noise", 8 }, { "comp", 5 }, { "overdrive", 1 }, { "lo-fi", 7 },
            { "delay", 2 }, { "reverb", 3 }, { "chorus", 3 }, { "phaser", 6 }, { "wah", 0 }, { "trem", 7 }, { "wow", 4 }, { "pitch", 4 },
            { "oct", 4 }, { "harm", 2 }, { "transpose", 4 }, { "boost", 8 }, { "wobble", 3 }, { "swell", 3 }, { "doubler", 0 } };
        for (auto& k : keys)
            if (t.contains (k.first))
                return k.second;
        return std::abs (title.hashCode()) % 9;
    }
}

AmpLookAndFeel::AmpLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, ui::background);

    setColour (juce::Slider::textBoxTextColourId,       ui::text);
    setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha (0.28f));
    setColour (juce::Label::outlineColourId,            juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,  ui::accent2.withAlpha (0.4f));
    setColour (juce::Label::textColourId,               ui::text);
    setColour (juce::Slider::trackColourId,             ui::accent2.withAlpha (0.45f));      // the tempo box
    setColour (juce::Slider::backgroundColourId,        ui::knobBody);

    setColour (juce::ComboBox::backgroundColourId, ui::knobBody);
    setColour (juce::ComboBox::textColourId,       ui::text);
    setColour (juce::ComboBox::outlineColourId,    ui::panelEdge.brighter (0.2f));
    setColour (juce::ComboBox::arrowColourId,      ui::textDim);

    setColour (juce::TooltipWindow::backgroundColourId, ui::panel.darker (0.4f));
    setColour (juce::TooltipWindow::textColourId,       ui::text);
    setColour (juce::TooltipWindow::outlineColourId,    ui::accent2.withAlpha (0.7f));

    setColour (juce::PopupMenu::backgroundColourId,            ui::panel);
    setColour (juce::PopupMenu::textColourId,                  ui::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent2);
    setColour (juce::PopupMenu::highlightedTextColourId,       juce::Colours::black);
}

void AmpLookAndFeel::applyAccent()
{
    setColour (juce::Slider::textBoxHighlightColourId,         ui::accent.withAlpha (0.4f));
    setColour (juce::Slider::trackColourId,                    ui::accent.withAlpha (0.45f));
    setColour (juce::TooltipWindow::outlineColourId,           ui::accent.withAlpha (0.7f));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, ui::accent);
}

void AmpLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = std::min (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const float cx = bounds.getCentreX(), cy = bounds.getCentreY();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float alpha = slider.isEnabled() ? 1.0f : 0.4f;

    // The knob follows the hardware style of its panel: machined aluminium (rack), black rubber (pedal),
    // or a black amp knob with a brass cap (amp, cab, acoustic).
    PanelKind kind = PanelKind::Rack;
    int themeIndex = 2;
    if (auto* panel = slider.findParentComponentOfClass<Panel>())
    {
        kind = panel->getKind();
        themeIndex = panel->getTheme();
    }
    if (kind == PanelKind::Rack || kind == PanelKind::Pedal)
    {
        // Flat dark knob inside a ring of light: the lit arc runs from the start (or from zero, for knobs
        // that go both ways) to the current value.
        const float r = radius - 1.5f;
        juce::Path trackArc;
        trackArc.addCentredArc (cx, cy, r - 1.5f, r - 1.5f, 0.0f, startAngle, endAngle, true);
        g.setColour (juce::Colours::white.withAlpha (0.10f * alpha));
        g.strokePath (trackArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        float zeroPos = 0.0f;
        if (slider.getMinimum() < 0.0 && slider.getMaximum() > 0.0)
            zeroPos = (float) slider.valueToProportionOfLength (0.0);
        const float a0 = startAngle + std::min (zeroPos, sliderPos) * (endAngle - startAngle);
        const float a1 = startAngle + std::max (zeroPos, sliderPos) * (endAngle - startAngle);
        if (a1 - a0 > 0.01f)
        {
            juce::Path valueArc;
            valueArc.addCentredArc (cx, cy, r - 1.5f, r - 1.5f, 0.0f, a0, a1, true);
            g.setColour (ui::accent.withAlpha (0.18f * alpha));
            g.strokePath (valueArc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (ui::accent.withMultipliedAlpha (alpha));
            g.strokePath (valueArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        const float bodyR = r - 7.0f;
        g.setColour (juce::Colours::black.withAlpha (0.5f * alpha));
        g.fillEllipse (cx - bodyR, cy - bodyR + 2.0f, bodyR * 2.0f, bodyR * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34343e).withMultipliedAlpha (alpha), cx - bodyR * 0.4f, cy - bodyR * 0.6f,
                                                 juce::Colour (0xff111116).withMultipliedAlpha (alpha), cx + bodyR * 0.7f, cy + bodyR * 0.9f, true));
        g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.10f * alpha));
        g.drawEllipse (cx - bodyR + 0.5f, cy - bodyR + 0.5f, bodyR * 2.0f - 1.0f, bodyR * 2.0f - 1.0f, 1.0f);
        g.setColour (ui::text.withMultipliedAlpha (alpha));
        g.drawLine (juce::Line<float> (at0 (cx, cy, bodyR * 0.30f, angle), at0 (cx, cy, bodyR - 3.0f, angle)), 2.4f);
        return;
    }

    const auto& th = ampThemeFor (themeIndex);
    const bool silver = kind == PanelKind::Rack, pedal = kind == PanelKind::Pedal;
    const juce::Colour litColour = (silver || pedal) ? ui::accent : juce::Colour (th.tick);

    auto at = [&] (float r, float a) { return juce::Point<float> (cx + r * std::sin (a), cy - r * std::cos (a)); };

    // Scale: eleven ticks around the knob; the ones up to the current value light up
    const float ringR = radius - 0.5f;
    for (int i = 0; i <= 10; ++i)
    {
        const float t = (float) i / 10.0f;
        const float a = startAngle + t * (endAngle - startAngle);
        const bool lit = t <= sliderPos + 0.001f;
        g.setColour ((lit ? litColour : ui::textDim.withAlpha (0.6f)).withMultipliedAlpha (alpha));
        g.drawLine (juce::Line<float> (at (ringR - 4.5f, a), at (ringR, a)), lit ? 2.0f : 1.2f);
    }

    // Body, with a soft shadow under it
    const float bodyR = ringR - 8.0f;
    g.setColour (juce::Colours::black.withAlpha (0.38f * alpha));
    g.fillEllipse (cx - bodyR + 0.5f, cy - bodyR + 2.5f, bodyR * 2.0f, bodyR * 2.0f);

    const juce::Colour light = silver ? juce::Colour (0xfff0f2f5) : (pedal ? juce::Colour (0xff4d4d54) : juce::Colour (0xff3d3d42));
    const juce::Colour dark  = silver ? juce::Colour (0xff666c78) : juce::Colour (0xff070708);
    g.setGradientFill (juce::ColourGradient (light.withMultipliedAlpha (alpha), cx - bodyR * 0.35f, cy - bodyR * 0.45f,
                                             dark.withMultipliedAlpha (alpha),  cx + bodyR * 0.9f,  cy + bodyR * 0.95f, true));
    g.fillEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    // Knurled edge
    for (int i = 0; i < 30; ++i)
    {
        const float a = (float) i * juce::MathConstants<float>::twoPi / 30.0f;
        g.setColour ((silver ? juce::Colours::black.withAlpha (0.20f) : juce::Colours::white.withAlpha (0.10f)).withMultipliedAlpha (alpha));
        g.drawLine (juce::Line<float> (at (bodyR - 3.0f, a), at (bodyR - 0.5f, a)), 1.0f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.55f * alpha));
    g.drawEllipse (cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 1.0f);

    // Cap
    const float capR = bodyR * (kind == PanelKind::Rack || pedal ? 0.62f : 0.48f);
    if (silver || pedal)
    {
        g.setGradientFill (juce::ColourGradient ((silver ? juce::Colour (0xffd8dce2) : juce::Colour (0xff2a2a2f)).withMultipliedAlpha (alpha),
                                                 cx - capR * 0.4f, cy - capR * 0.5f,
                                                 (silver ? juce::Colour (0xff8b919c) : juce::Colour (0xff0e0e10)).withMultipliedAlpha (alpha),
                                                 cx + capR, cy + capR, true));
    }
    else
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (th.capA).withMultipliedAlpha (alpha), cx - capR * 0.4f, cy - capR * 0.5f,
                                                 juce::Colour (th.capB).withMultipliedAlpha (alpha), cx + capR, cy + capR, true));
    }
    g.fillEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f * alpha));
    g.drawEllipse (cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 0.8f);

    // Pointer line
    g.setColour ((silver ? juce::Colour (0xff14161b) : juce::Colour (0xfff4eed8)).withMultipliedAlpha (alpha));
    const bool longPointer = kind != PanelKind::Rack && ! pedal;
    g.drawLine (juce::Line<float> (at (longPointer ? capR : bodyR * 0.25f, angle), at (bodyR - 1.5f, angle)), 2.6f);
}

void AmpLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                       bool highlighted, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = button.getToggleState();
    const float alpha = button.isEnabled() ? 1.0f : 0.45f;

    g.setColour ((on ? ui::accent.withAlpha (0.13f) : (highlighted ? ui::knobBody.brighter (0.12f) : ui::knobBody).withAlpha (0.8f)).withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour ((on ? ui::accent.withAlpha (0.55f) : juce::Colours::white.withAlpha (0.10f)).withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    drawLed (g, { r.getX() + 13.0f, r.getCentreY() }, 3.6f, on, alpha);

    g.setColour ((on ? ui::text : ui::textDim).withMultipliedAlpha (alpha));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().withTrimmedLeft (26).withTrimmedRight (6),
                      juce::Justification::centredLeft, 1);
}

void AmpLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                           const juce::Colour&, bool highlighted, bool down)
{
    auto r = button.getLocalBounds().toFloat().reduced (1.0f);

    if (button.getComponentID() == "tab")   // page tabs: plain text, with a glowing line under the chosen one
    {
        if (button.getToggleState())
        {
            g.setColour (ui::accent.withAlpha (0.16f));
            g.fillRoundedRectangle (r.getX() + 14.0f, r.getBottom() - 6.0f, r.getWidth() - 28.0f, 6.0f, 3.0f);
            g.setColour (ui::accent);
            g.fillRoundedRectangle (r.getX() + 14.0f, r.getBottom() - 3.0f, r.getWidth() - 28.0f, 2.0f, 1.0f);
        }
        else if (highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (0.05f));
            g.fillRoundedRectangle (r.reduced (6.0f, 3.0f), 6.0f);
        }
        return;
    }

    const bool lit = down || button.getToggleState();   // held, or latched on

    g.setColour (lit ? ui::accent : (highlighted ? ui::knobBody.brighter (0.15f) : ui::knobBody).withAlpha (0.85f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (lit ? ui::accent.brighter (0.3f) : juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void AmpLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool down)
{
    g.setFont (getTextButtonFont (button, button.getHeight()));
    if (button.getComponentID() == "tab")
        g.setColour (button.getToggleState() ? ui::text : ui::textDim);
    else
        g.setColour ((down || button.getToggleState()) ? juce::Colours::black : ui::text);
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (4, 2),
                      juce::Justification::centred, 1);
}

void AmpLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    const float alpha = box.isEnabled() ? 1.0f : 0.45f;

    g.setColour (ui::knobBody.withAlpha (0.85f * alpha));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour ((box.hasKeyboardFocus (true) ? ui::accent : juce::Colours::white.withAlpha (0.11f)).withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    const float cx = (float) width - 14.0f, cy = (float) height * 0.5f;
    juce::Path chevron;
    chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
    chevron.lineTo (cx, cy + 2.0f);
    chevron.lineTo (cx + 4.0f, cy - 2.0f);
    g.setColour (ui::textDim.withMultipliedAlpha (alpha));
    g.strokePath (chevron, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void AmpLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float minSliderPos, float maxSliderPos,
                                       juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    const float alpha = slider.isEnabled() ? 1.0f : 0.4f;
    const float cx    = (float) x + (float) width * 0.5f;
    const float top   = (float) y + 4.0f, bottom = (float) (y + height) - 4.0f;

    // Groove
    g.setColour (ui::track.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (cx - 1.5f, top, 3.0f, bottom - top, 1.5f);

    // Glowing fill from 0 dB to the thumb
    const float zero = juce::jlimit (top, bottom, (float) slider.getPositionOfValue (0.0));
    const float fillTop = std::min (zero, sliderPos), fillH = std::abs (sliderPos - zero);
    if (fillH > 0.5f)
    {
        g.setColour (ui::accent.withAlpha (0.16f * alpha));
        g.fillRoundedRectangle (cx - 4.0f, fillTop, 8.0f, fillH, 4.0f);
        g.setGradientFill (juce::ColourGradient (ui::accent2.withMultipliedAlpha (alpha), 0.0f, top,
                                                 ui::accent.withMultipliedAlpha (alpha),  0.0f, bottom, false));
        g.fillRoundedRectangle (cx - 1.5f, fillTop, 3.0f, fillH, 1.5f);
    }

    // 0 dB tick
    g.setColour (ui::textDim.withMultipliedAlpha (alpha * 0.8f));
    g.drawLine (cx - 9.0f, zero, cx + 9.0f, zero, 1.0f);

    // Thumb: a glassy pill with a bright centre line
    juce::Rectangle<float> thumb (cx - 11.0f, sliderPos - 5.5f, 22.0f, 11.0f);
    g.setGradientFill (juce::ColourGradient (ui::knobBody.brighter (0.6f), thumb.getX(), thumb.getY(),
                                             ui::knobBody.darker (0.5f),   thumb.getX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 5.5f);
    g.setColour (ui::panelEdge.brighter (0.5f));
    g.drawRoundedRectangle (thumb, 5.5f, 1.0f);
    g.setColour (ui::text.withMultipliedAlpha (alpha));
    g.drawLine (thumb.getX() + 5.0f, thumb.getCentreY(), thumb.getRight() - 5.0f, thumb.getCentreY(), 1.5f);
}

juce::Font AmpLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return juce::Font (juce::FontOptions (11.5f, juce::Font::bold));
}

//==============================================================================
// KnobControl
//==============================================================================
KnobControl::KnobControl (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                          const juce::String& name, const juce::String& suffix, int decimals)
    : paramId (paramId)
{
    nameLabel.setText (name, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, ui::textDim);
    nameLabel.setFont (juce::Font (juce::FontOptions (11.5f)));
    addAndMakeVisible (nameLabel);

    const auto tip = tipFor (apvts, paramId);
    nameLabel.setTooltip (tip);
    slider.setTooltip (tip);

    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 16);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::black.withAlpha (0.28f));
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);

    // The attachment sets its own value text, so set ours afterwards.
    slider.textFromValueFunction = [suffix, decimals] (double v) { return juce::String (v, decimals) + suffix; };
    slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    if (paramId == "odAttack")      // stored as 0 to 5, shown as positions 1 to 6
    {
        slider.textFromValueFunction = [] (double v) { return juce::String ((int) std::lround (v) + 1); };
        slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() - 1.0; };
    }
    slider.updateText();
}

void KnobControl::resized()
{
    auto r = getLocalBounds();
    nameLabel.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

void KnobControl::setUsed (bool used, const juce::String& why)
{
    if (baseTip.isEmpty() && usedNow)
        baseTip = slider.getTooltip();
    if (used == usedNow && (used || why == whyNow))
        return;
    usedNow = used;
    whyNow = why;
    const auto tip = used ? baseTip : "Not in use right now: " + why + "\n" + baseTip;
    nameLabel.setTooltip (tip);
    slider.setTooltip (tip);
    setEnabled (used && ! tempoLocked);
}

//==============================================================================
// Panel
//==============================================================================
void Panel::addRow (std::initializer_list<juce::Component*> comps, std::vector<float> weights,
                    int height, int page)
{
    Row row;
    for (auto* c : comps)
    {
        if (c != nullptr)
            addAndMakeVisible (*c);
        row.comps.push_back (c);
    }
    row.weights = std::move (weights);
    row.height = height;
    row.page = page;
    rows.push_back (std::move (row));
}

void Panel::addKnob (KnobControl& knob, int page)
{
    addAndMakeVisible (knob);
    knobs.push_back ({ &knob, page });
}

void Panel::setContentDimmed (juce::Component* keep, bool dim)
{
    if (dim == dimmed)
        return;
    dimmed = dim;
    const float a = dim ? 0.45f : 1.0f;
    for (auto& r : rows)
        for (auto* c : r.comps)
            if (c != nullptr && c != keep)
                c->setAlpha (a);
    for (auto& k : knobs)
        k.knob->setAlpha (a);
}

void Panel::setHidden (juce::Component* c, bool hide)
{
    if (c == nullptr || isExplicitlyHidden (c) == hide)
        return;
    if (hide) hiddenComps.push_back (c);
    else      hiddenComps.erase (std::remove (hiddenComps.begin(), hiddenComps.end(), c), hiddenComps.end());
    resized();
}

void Panel::markAdvanced (juce::Component* c)
{
    if (c == nullptr)
        return;
    if (advancedComps.empty())
    {
        advButton.setClickingTogglesState (true);
        advButton.setColour (juce::TextButton::buttonColourId, ui::knobBody);
        advButton.setColour (juce::TextButton::buttonOnColourId, ui::accent2);
        advButton.setColour (juce::TextButton::textColourOffId, ui::textDim);
        advButton.setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        advButton.setTooltip ("Basic / Advanced. Off: only the main controls of this panel are shown. On: all of them. Hidden controls keep their settings and still work.");
        advButton.onClick = [this] { setAdvancedShown (advButton.getToggleState()); if (onAdvancedChanged) onAdvancedChanged (advancedShown); };
        addAndMakeVisible (advButton);
    }
    advancedComps.push_back (c);
    resized();
}

void Panel::setAdvancedShown (bool shown)
{
    if (shown == advancedShown)
        return;
    advancedShown = shown;
    advButton.setToggleState (shown, juce::dontSendNotification);
    resized();
}

void Panel::setPage (int newPage)
{
    if (newPage == currentPage)
        return;

    currentPage = newPage;
    resized();
}

void Panel::paint (juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;

    const float scale = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int pw = juce::roundToInt ((float) w * scale), ph = juce::roundToInt ((float) h * scale);
    if (! cache.isValid() || cache.getWidth() != pw || cache.getHeight() != ph)
    {
        cache = juce::Image (juce::Image::ARGB, pw, ph, true);
        juce::Graphics cg (cache);
        cg.addTransform (juce::AffineTransform::scale (scale));
        renderFace (cg, w, h);
    }
    g.drawImage (cache, 0, 0, w, h, 0, 0, pw, ph);
}

void Panel::renderFace (juce::Graphics& g, int w, int h) const
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (1.0f);
    juce::Random rng (title.hashCode());
    const juce::Font titleFont (juce::FontOptions (11.0f, juce::Font::bold));
    auto textWidth = [&] (float spacing)
    {
        float total = 0.0f;
        for (auto c : title.toUpperCase())
            total += juce::GlyphArrangement::getStringWidth (titleFont, juce::String::charToString (c)) + spacing;
        return total;
    };
    auto screw = [&] (float x, float y, float rad, float tilt)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8eaee), x - rad * 0.4f, y - rad * 0.5f,
                                                 juce::Colour (0xff6b7079), x + rad, y + rad, true));
        g.fillEllipse (x - rad, y - rad, rad * 2.0f, rad * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (x - rad, y - rad, rad * 2.0f, rad * 2.0f, 0.7f);
        g.drawLine (x - rad * 0.7f * std::cos (tilt), y - rad * 0.7f * std::sin (tilt),
                    x + rad * 0.7f * std::cos (tilt), y + rad * 0.7f * std::sin (tilt), 1.1f);
    };
    [[maybe_unused]] auto jewel = [&] (float x, float y, juce::Colour c)
    {
        g.setGradientFill (juce::ColourGradient (c.withAlpha (0.35f), x, y, c.withAlpha (0.0f), x + 9.0f, y, true));
        g.fillEllipse (x - 9.0f, y - 9.0f, 18.0f, 18.0f);
        g.setGradientFill (juce::ColourGradient (c.brighter (0.7f), x - 1.0f, y - 1.2f, c.darker (0.6f), x + 3.5f, y + 3.5f, true));
        g.fillEllipse (x - 3.6f, y - 3.6f, 7.2f, 7.2f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (x - 3.6f, y - 3.6f, 7.2f, 7.2f, 0.8f);
    };

    switch (kind)
    {
        case PanelKind::Pedal:
        case PanelKind::Rack:
        {
            // A dark card with a hairline edge and a faint glow in the top-left corner. Pedals carry their own
            // colour in the little bar beside the title; the rest use the amp's accent.
            const bool pedal = kind == PanelKind::Pedal;
            const auto mark = pedal ? tint.brighter (0.9f) : ui::accent;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff181820), 0.0f, r.getY(), juce::Colour (0xff0e0e13), 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 12.0f);
            g.setGradientFill (juce::ColourGradient (mark.withAlpha (pedal ? 0.13f : 0.09f), r.getX() + 20.0f, r.getY(),
                                                     mark.withAlpha (0.0f), r.getX() + 360.0f, r.getY() + 40.0f, true));
            g.fillRoundedRectangle (r, 12.0f);
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.drawRoundedRectangle (r.reduced (0.5f), 12.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.09f));
            g.drawLine (r.getX() + 16.0f, r.getY() + 1.0f, r.getRight() - 16.0f, r.getY() + 1.0f, 1.0f);

            if (pedal)
            {
                // v61: the enclosure band: a bold coat of the pedal's colour across the top with line-art on it, a faint
                // watermark of the same art in the lower corner, and a footswitch-style screw in each top corner.
                const int motif = motifForTitle (title);
                const auto coat = tint.withMultipliedSaturation (1.55f).withBrightness (juce::jlimit (0.55f, 0.85f, tint.getBrightness() * 1.9f));
                const auto ink  = juce::Colour (0xff0c0c10);
                juce::Path band;
                band.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), 30.0f, 12.0f, 12.0f, true, true, false, false);
                g.setGradientFill (juce::ColourGradient (coat.brighter (0.15f), 0.0f, r.getY(), coat.darker (0.25f), 0.0f, r.getY() + 30.0f, false));
                g.fillPath (band);
                g.setColour (ink.withAlpha (0.55f));
                g.drawLine (r.getX() + 1.0f, r.getY() + 30.0f, r.getRight() - 1.0f, r.getY() + 30.0f, 1.5f);
                g.setColour (juce::Colours::white.withAlpha (0.22f));
                g.drawLine (r.getX() + 14.0f, r.getY() + 1.5f, r.getRight() - 14.0f, r.getY() + 1.5f, 1.0f);
                // the art, right of the title and left of the ADV button
                {
                    juce::Graphics::ScopedSaveState ss (g);
                    g.reduceClipRegion (band);
                    drawPedalMotif (g, motif, { r.getRight() - 190.0f, r.getY() + 4.0f, 100.0f, 22.0f }, ink.withAlpha (0.62f), 1.5f);
                    for (int i = 0; i < 5; ++i)      // a row of little tick marks, like a screen-print border
                        g.fillRect (r.getRight() - 84.0f + (float) i * 3.0f, r.getY() + 12.0f, 1.2f, 7.0f - (float) (i % 2) * 3.0f);
                }
                // watermark
                {
                    juce::Graphics::ScopedSaveState ss (g);
                    g.reduceClipRegion (r.toNearestInt());
                    drawPedalMotif (g, motif, { r.getRight() - 130.0f, r.getBottom() - 92.0f, 110.0f, 76.0f }, coat.withAlpha (0.07f), 2.2f);
                }
                screw (r.getX() + 9.0f, r.getY() + 9.0f, 2.6f, 0.6f);
                screw (r.getRight() - 9.0f, r.getY() + 9.0f, 2.6f, -0.4f);
                g.setColour (ink.withAlpha (0.90f));
                drawSpaced (g, title, r.getX() + 22.0f, r.getY() + 6.0f, 18.0f, 1.6f, r.getRight() - 196.0f, titleFont);
                break;
            }
            g.setColour (mark);
            g.fillRoundedRectangle (r.getX() + 14.0f, r.getY() + 11.0f, 3.0f, 14.0f, 1.5f);
            g.setColour (ui::text.withAlpha (0.92f));
            const float tx = r.getX() + 25.0f;
            drawSpaced (g, title, tx, r.getY() + 9.0f, 18.0f, 1.4f, r.getRight() - 62.0f, titleFont);
            break;
        }

        case PanelKind::Amp:
        case PanelKind::Cab:
        case PanelKind::Wood:
        {
            const bool cab = kind == PanelKind::Cab, wood = kind == PanelKind::Wood;
            const auto& th = ampThemeFor (theme);
            const auto top    = wood ? juce::Colour (0xff74492a) : juce::Colour (cab ? th.clothTop : th.tolexTop);
            const auto bottom = wood ? juce::Colour (0xff3c2411) : juce::Colour (cab ? th.clothBottom : th.tolexBottom);
            g.setGradientFill (juce::ColourGradient (top, 0.0f, r.getY(), bottom, 0.0f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 7.0f);

            if (wood)                              // long wavy grain
            {
                for (int i = 0; i < 46; ++i)
                {
                    const float y0 = r.getY() + 4.0f + rng.nextFloat() * (r.getHeight() - 8.0f);
                    const float amp = 1.0f + 3.0f * rng.nextFloat(), phase = rng.nextFloat() * 6.0f;
                    juce::Path p;
                    p.startNewSubPath (r.getX() + 4.0f, y0);
                    for (int s = 1; s <= 12; ++s)
                    {
                        const float fx = (float) s / 12.0f;
                        p.lineTo (r.getX() + 4.0f + fx * (r.getWidth() - 8.0f), y0 + amp * std::sin (phase + fx * 7.0f));
                    }
                    g.setColour (rng.nextBool() ? juce::Colours::black.withAlpha (0.10f) : juce::Colour (0xffe0b070).withAlpha (0.06f));
                    g.strokePath (p, juce::PathStrokeType (1.0f));
                }
            }
            else if (cab)                          // speaker cloth, in the style of the selected amp's cabinet
            {
                const juce::Colour fleck (th.fleck);
                const auto inner = r.reduced (4.0f);
                g.saveState();
                g.reduceClipRegion (inner.toNearestInt());
                switch (th.clothStyle)
                {
                    case 0:                        // sparkle
                        for (int i = 0; i < 900; ++i)
                        {
                            g.setColour (fleck.withAlpha (0.08f + 0.30f * rng.nextFloat()));
                            g.fillRect (inner.getX() + rng.nextFloat() * inner.getWidth(), inner.getY() + rng.nextFloat() * inner.getHeight(), 1.2f, 1.2f);
                        }
                        break;
                    case 1:                        // diamond lattice
                        for (float k = -inner.getHeight(); k < inner.getWidth(); k += 8.0f)
                        {
                            g.setColour (fleck.withAlpha (0.20f));
                            g.drawLine (inner.getX() + k, inner.getY(), inner.getX() + k + inner.getHeight(), inner.getBottom(), 1.0f);
                            g.drawLine (inner.getX() + k + inner.getHeight(), inner.getY(), inner.getX() + k, inner.getBottom(), 1.0f);
                        }
                        break;
                    case 2:                        // salt and pepper
                        for (int i = 0; i < 1400; ++i)
                        {
                            g.setColour (rng.nextBool() ? fleck.withAlpha (0.35f) : juce::Colours::black.withAlpha (0.35f));
                            g.fillRect (inner.getX() + rng.nextFloat() * inner.getWidth(), inner.getY() + rng.nextFloat() * inner.getHeight(), 1.4f, 1.4f);
                        }
                        break;
                    case 3:                        // basket weave
                        for (int iy = 0; iy < (int) (inner.getHeight() / 5.0f) + 1; ++iy)
                            for (int ix = 0; ix < (int) (inner.getWidth() / 5.0f) + 1; ++ix)
                            {
                                const float gx = inner.getX() + (float) ix * 5.0f, gy = inner.getY() + (float) iy * 5.0f;
                                g.setColour (fleck.withAlpha (0.20f));
                                if ((ix + iy) % 2 == 0) g.fillRect (gx, gy + 1.0f, 5.0f, 2.0f);
                                else                    g.fillRect (gx + 1.0f, gy, 2.0f, 5.0f);
                                g.setColour (juce::Colours::black.withAlpha (0.25f));
                                g.drawRect (gx, gy, 5.0f, 5.0f, 0.5f);
                            }
                        break;
                    case 5:                        // dot matrix (v61)
                        for (float gy = inner.getY() + 3.0f; gy < inner.getBottom(); gy += 6.0f)
                            for (float gx = inner.getX() + 3.0f; gx < inner.getRight(); gx += 6.0f)
                            {
                                g.setColour (fleck.withAlpha (0.55f));
                                g.fillEllipse (gx - 1.0f, gy - 1.0f, 2.0f, 2.0f);
                            }
                        break;
                    default:                       // pinstripe
                        for (float gx = inner.getX(); gx < inner.getRight(); gx += 9.0f)
                        {
                            g.setColour (fleck.withAlpha (0.45f));
                            g.fillRect (gx, inner.getY(), 1.0f, inner.getHeight());
                        }
                        break;
                }
                g.restoreState();
            }
            else                                   // pebbled tolex
            {
                for (int i = 0; i < 700; ++i)
                {
                    g.setColour (rng.nextBool() ? juce::Colours::white.withAlpha (0.05f) : juce::Colours::black.withAlpha (0.30f));
                    g.fillRect (r.getX() + 4.0f + rng.nextFloat() * (r.getWidth() - 8.0f), r.getY() + 4.0f + rng.nextFloat() * (r.getHeight() - 8.0f), 1.3f, 1.3f);
                }
            }

            // Piping (gold on the amp, cream on the cab, binding on the wood)
            const auto pipeA = wood ? juce::Colour (0xfff0dfb0) : juce::Colour (th.pipeA);
            const auto pipeB = wood ? juce::Colour (0xffb59a62) : juce::Colour (th.pipeB);
            g.setGradientFill (juce::ColourGradient (pipeA, 0.0f, r.getY(), pipeB, 0.0f, r.getBottom(), false));
            g.drawRoundedRectangle (r.reduced (wood ? 2.5f : 4.0f), 5.0f, wood ? 3.2f : 2.2f);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawRoundedRectangle (r, 7.0f, 1.4f);

            // Nameplate with the title
            if (bigNameplate && title.isNotEmpty())
            {
                // The amp's front plate, centred on the grille: the model name in big letters
                const juce::Font big (juce::FontOptions (24.0f, juce::Font::bold));
                float tw = 0.0f;
                for (auto ch : title.toUpperCase())
                    tw += juce::GlyphArrangement::getStringWidth (big, juce::String::charToString (ch)) + 8.0f;
                const float pw = juce::jmin (tw + 80.0f, r.getWidth() - 40.0f);
                const juce::Rectangle<float> bigPlate (r.getCentreX() - pw * 0.5f, r.getCentreY() - 28.0f, pw, 56.0f);
                g.setColour (juce::Colours::black.withAlpha (0.35f));
                g.fillRoundedRectangle (bigPlate.translated (0.0f, 3.0f), 5.0f);
                g.setGradientFill (juce::ColourGradient (juce::Colour (th.plateA), 0.0f, bigPlate.getY(), juce::Colour (th.plateB), 0.0f, bigPlate.getBottom(), false));
                g.fillRoundedRectangle (bigPlate, 5.0f);
                g.setColour (juce::Colours::black.withAlpha (0.65f));
                g.drawRoundedRectangle (bigPlate, 5.0f, 1.2f);
                g.setColour (juce::Colours::white.withAlpha (0.3f));
                g.drawLine (bigPlate.getX() + 5.0f, bigPlate.getY() + 2.0f, bigPlate.getRight() - 5.0f, bigPlate.getY() + 2.0f, 1.0f);
                screw (bigPlate.getX() + 9.0f, bigPlate.getCentreY(), 3.0f, 0.6f);
                screw (bigPlate.getRight() - 9.0f, bigPlate.getCentreY(), 3.0f, 2.0f);
                g.setColour (juce::Colour (th.plateText));
                drawSpaced (g, title, bigPlate.getX() + 40.0f, bigPlate.getY() + 8.0f, 40.0f, 8.0f, bigPlate.getRight() - 30.0f, big);
                break;
            }
            const float plateW = juce::jmin (textWidth (1.3f) + 28.0f, r.getWidth() - 40.0f);
            const juce::Rectangle<float> plate (r.getX() + 16.0f, r.getY() + 8.0f, plateW, 17.0f);
            const auto plateA = wood ? juce::Colour (0xfff3e3b8) : juce::Colour (th.plateA);
            const auto plateB = wood ? juce::Colour (0xffc7ad76) : juce::Colour (th.plateB);
            g.setGradientFill (juce::ColourGradient (plateA, 0.0f, plate.getY(), plateB, 0.0f, plate.getBottom(), false));
            g.fillRoundedRectangle (plate, 3.0f);
            g.setColour (juce::Colours::black.withAlpha (0.65f));
            g.drawRoundedRectangle (plate, 3.0f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.3f));
            g.drawLine (plate.getX() + 3.0f, plate.getY() + 1.5f, plate.getRight() - 3.0f, plate.getY() + 1.5f, 1.0f);
            screw (plate.getX() + 5.0f, plate.getCentreY(), 1.9f, 0.6f);
            screw (plate.getRight() - 5.0f, plate.getCentreY(), 1.9f, 2.0f);
            g.setColour (wood ? juce::Colour (0xff3b2412) : juce::Colour (th.plateText));
            drawSpaced (g, title, plate.getX() + 13.0f, plate.getY() - 0.5f, 18.0f, 1.3f, plate.getRight() - 12.0f, titleFont);
            break;
        }
    }
}

void Panel::resized()
{
    auto area = getLocalBounds().reduced (12, 8);
    area.removeFromTop (22);   // the title

    if (! advancedComps.empty())
    {
        advButton.setBounds (getWidth() - 12 - 44, 7, 44, 18);
        advButton.setVisible (advAvailable);
    }

    for (auto& row : rows)
    {
        bool shown = (row.page < 0 || row.page == currentPage);

        // A row made only of advanced controls vanishes while ADV is off.
        bool anyComp = false, allAdvHidden = true;
        for (auto* c : row.comps)
            if (c != nullptr) { anyComp = true; allAdvHidden = allAdvHidden && isAdvancedHidden (c); }
        if (anyComp && allAdvHidden)
            shown = false;

        // v56: a row whose controls are all hidden (for the current settings) collapses too, and a hidden
        // control in a row gives its share of the width to the others.
        bool allHidden = true;
        for (auto* c : row.comps)
            if (c != nullptr && ! isHidden (c))
                allHidden = false;
        if (anyComp && allHidden)
            shown = false;

        for (auto* c : row.comps)
            if (c != nullptr)
                c->setVisible (shown && ! isHidden (c));

        if (! shown)
            continue;

        auto rowArea = area.removeFromTop (row.height);
        area.removeFromTop (4);

        auto weightOf = [&] (size_t i)
        {
            if (row.comps[i] != nullptr && isHidden (row.comps[i]))
                return 0.0f;
            return (i < row.weights.size()) ? row.weights[i] : 1.0f;
        };
        float total = 0.0f;
        size_t lastShown = 0;
        for (size_t i = 0; i < row.comps.size(); ++i)
        {
            total += weightOf (i);
            if (weightOf (i) > 0.0f) lastShown = i;
        }

        const float fullWidth = (float) rowArea.getWidth();
        for (size_t i = 0; i < row.comps.size(); ++i)
        {
            const float weight = weightOf (i);
            if (weight <= 0.0f)
                continue;
            const int   w = (i == lastShown) ? rowArea.getWidth() : (int) std::round (fullWidth * weight / total);
            auto slot = rowArea.removeFromLeft (w).reduced (2, 0);
            if (row.comps[i] != nullptr)
                row.comps[i]->setBounds (slot);
        }
    }

    // Knobs: the ones for this page, in one row or wrapped into several.
    std::vector<KnobControl*> shownKnobs;
    for (auto& k : knobs)
    {
        const bool shown = (k.page < 0 || k.page == currentPage) && ! isHidden (k.knob);
        k.knob->setVisible (shown);
        if (shown)
            shownKnobs.push_back (k.knob);
    }

    if (! shownKnobs.empty())
    {
        const int count    = (int) shownKnobs.size();
        const int columns  = knobColumns > 0 ? std::min (knobColumns, count) : count;
        const int knobRows = (count + columns - 1) / columns;
        const int cellW    = area.getWidth() / columns;
        const int cellH    = area.getHeight() / knobRows;

        for (int i = 0; i < count; ++i)
        {
            juce::Rectangle<int> cell (area.getX() + (i % columns) * cellW,
                                       area.getY() + (i / columns) * cellH, cellW, cellH);
            shownKnobs[(size_t) i]->setBounds (cell.withSizeKeepingCentre (cell.getWidth(), std::min (cell.getHeight(), 104)));
        }
    }
}

//==============================================================================
// IRView
//==============================================================================
IRView::IRView (StarterAmpProcessor& p) : Panel ("IR ALIGNMENT"), proc (p)
{
    startTimerHz (10);
}

std::vector<float> IRView::currentValues() const
{
    auto get = [this] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };

    std::vector<float> v { (float) proc.getIRVersion() };
    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        const auto& id = kCabIds[blk];
        v.push_back (get (id.on));
        v.push_back (get (id.level));
        v.push_back (get (id.delay));
        v.push_back (get (id.pol));
    }
    return v;
}

void IRView::timerCallback()
{
    if (! isShowing())
        return;

    const auto values = currentValues();
    if (values != lastValues)
    {
        recompute (values);
        lastValues = values;
        repaint();
    }
}

// Magnitude response (in dB, smoothed to about 1/3 octave) at kPoints frequencies from 60 Hz to 12 kHz.
void IRView::computeResponse (const std::vector<float>& ir, std::array<float, kPoints>& out)
{
    const int size = 1 << kFftOrder;
    std::vector<float> data ((size_t) size * 2, 0.0f);
    std::copy (ir.begin(), ir.begin() + std::min<size_t> (ir.size(), (size_t) size), data.begin());

    fft.performFrequencyOnlyForwardTransform (data.data());   // magnitudes end up in the first half

    const double binHz = StarterAmpProcessor::kIRAnalysisRate / (double) size;
    for (int k = 0; k < kPoints; ++k)
    {
        const double f  = 60.0 * std::pow (12000.0 / 60.0, (double) k / (double) (kPoints - 1));
        const int lo = std::max (1, (int) std::floor (f * std::pow (2.0, -1.0 / 6.0) / binHz));
        const int hi = std::min (size / 2 - 1, (int) std::ceil (f * std::pow (2.0, 1.0 / 6.0) / binHz));

        double power = 0.0;
        int count = 0;
        for (int bin = lo; bin <= hi; ++bin)
        {
            power += (double) data[(size_t) bin] * (double) data[(size_t) bin];
            ++count;
        }
        out[(size_t) k] = (count > 0) ? (float) (10.0 * std::log10 (power / (double) count + 1.0e-12)) : -120.0f;
    }
}

void IRView::recompute (const std::vector<float>& v)
{
    report = proc.getAlignReport();
    const size_t size = (size_t) 1 << kFftOrder;

    // v: [0] = IR version, then four numbers per block: on, level, delay, invert
    numActive = 0;
    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        irs[blk] = proc.getIRSnapshot (blk);
        active[blk] = v[(size_t) (1 + 4 * blk)] > 0.5f && ! irs[blk].empty();
        if (active[blk]) ++numActive;
    }
    const float norm = numActive > 0 ? 1.0f / std::sqrt ((float) numActive) : 1.0f;

    std::vector<float> hMix (size, 0.0f);
    float peak = 1.0e-6f;

    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        resp[blk].fill (-120.0f);
        waves[blk].fill (0.0f);
        if (! active[blk])
            continue;

        const float level = juce::Decibels::decibelsToGain (v[(size_t) (2 + 4 * blk)]);
        const int   delay = (int) std::lround (v[(size_t) (3 + 4 * blk)] * 0.001 * StarterAmpProcessor::kIRAnalysisRate);
        const float pol   = v[(size_t) (4 + 4 * blk)] > 0.5f ? -1.0f : 1.0f;
        const float weight = norm * level * pol;

        // The IR as it is heard: delayed, flipped and scaled
        std::vector<float> h (size, 0.0f);
        for (size_t n = 0; n < irs[blk].size() && n + (size_t) delay < size; ++n)
            h[n + (size_t) delay] = weight * irs[blk][n];
        for (size_t n = 0; n < size; ++n)
            hMix[n] += h[n];
        computeResponse (h, resp[blk]);

        // First 6 ms, all drawn with the same scale so timing and polarity can be compared
        for (int n = 0; n < kWaveSamples; ++n)
        {
            waves[blk][(size_t) n] = (n >= delay && (size_t) (n - delay) < irs[blk].size()) ? irs[blk][(size_t) (n - delay)] * pol : 0.0f;
            peak = std::max (peak, std::abs (waves[blk][(size_t) n]));
        }
    }
    for (auto& w : waves)
        for (auto& x : w)
            x /= peak;

    computeResponse (hMix, respMix);

    // Put the combined curve's average (200 Hz to 4 kHz) at 0 dB, and shift the others the same amount
    double sum = 0.0;
    int count = 0;
    for (int k = 0; k < kPoints; ++k)
    {
        const double f = 60.0 * std::pow (200.0, (double) k / (double) (kPoints - 1));
        if (f >= 200.0 && f <= 4000.0) { sum += respMix[(size_t) k]; ++count; }
    }
    const float offset = (count > 0) ? (float) (sum / count) : 0.0f;
    for (int k = 0; k < kPoints; ++k)
    {
        for (auto& r : resp)
            r[(size_t) k] -= offset;
        respMix[(size_t) k] -= offset;
    }
}

static juce::Colour blockColour (int blk)
{
    switch (blk) { case 0: return ui::accent; case 1: return ui::colourB; case 2: return ui::colourC; default: return ui::colourD; }
}

void IRView::paint (juce::Graphics& g)
{
    Panel::paint (g);

    auto area = getLocalBounds().reduced (12, 8);
    area.removeFromTop (22);
    auto status = area.removeFromBottom (34);
    area.removeFromBottom (4);

    // Message at the bottom
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f)));

    if (numActive == 0)
        g.drawFittedText ("Turn on a cab block (and load an IR, or use a built-in model) to see it here. With two or more blocks, the display shows how they line up.",
                          status, juce::Justification::topLeft, 3);
    else if (report.isNotEmpty())
        g.drawFittedText (report, status, juce::Justification::topLeft, 3);
    else
        g.drawFittedText (numActive > 1 ? "Press ALIGN NOW to line the blocks up." : "Turn on a second cab block to compare and align.",
                          status, juce::Justification::topLeft, 3);

    // Two plots side by side
    auto waveArea = area.removeFromLeft ((int) (area.getWidth() * 0.38f));
    area.removeFromLeft (8);
    auto respArea = area;

    auto drawFrame = [&g] (juce::Rectangle<int> r, const juce::String& caption)
    {
        g.setColour (ui::background);
        g.fillRoundedRectangle (r.toFloat(), 4.0f);
        g.setColour (ui::panelEdge);
        g.drawRoundedRectangle (r.toFloat(), 4.0f, 1.0f);
        g.setColour (ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        g.drawText (caption, r.reduced (4, 2), juce::Justification::topLeft);
    };

    drawFrame (waveArea, "0 - 6 ms");
    drawFrame (respArea, "60 Hz - 12 kHz");

    // Legend
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    {
        int offsetX = 0;
        g.setColour (ui::text);
        g.drawText ("ALL", respArea.reduced (6, 2).withTrimmedRight (offsetX), juce::Justification::topRight);
        offsetX += 28;
        for (int blk = kCabBlocks - 1; blk >= 0; --blk)
        {
            g.setColour (blockColour (blk));
            g.drawText (cabLetter (blk), respArea.reduced (6, 2).withTrimmedRight (offsetX), juce::Justification::topRight);
            offsetX += 14;
        }
    }

    if (numActive == 0)
        return;

    // Waveforms
    {
        auto r = waveArea.reduced (4, 14).toFloat();
        const float mid = r.getCentreY();

        g.setColour (ui::panelEdge);
        g.drawHorizontalLine ((int) mid, r.getX(), r.getRight());

        for (int blk = 0; blk < kCabBlocks; ++blk)
        {
            if (! active[blk])
                continue;
            juce::Path path;
            for (int n = 0; n < kWaveSamples; ++n)
            {
                const float x = r.getX() + r.getWidth() * (float) n / (float) (kWaveSamples - 1);
                const float y = mid - waves[blk][(size_t) n] * r.getHeight() * 0.48f;
                if (n == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (blockColour (blk).withAlpha (0.9f));
            g.strokePath (path, juce::PathStrokeType (1.2f));
        }
    }

    // Frequency response, from -30 dB to +10 dB around the average
    {
        auto r = respArea.reduced (4, 14).toFloat();
        const float lowDb = -30.0f, highDb = 10.0f;

        auto yFor = [&] (float db) { return r.getBottom() - r.getHeight() * (juce::jlimit (lowDb, highDb, db) - lowDb) / (highDb - lowDb); };

        g.setColour (ui::panelEdge);
        g.drawHorizontalLine ((int) yFor (0.0f), r.getX(), r.getRight());
        for (double f : { 100.0, 1000.0, 10000.0 })
        {
            const float x = r.getX() + r.getWidth() * (float) (std::log (f / 60.0) / std::log (200.0));
            g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        }

        auto drawCurve = [&] (const std::array<float, kPoints>& c, juce::Colour colour, float thickness, float alpha)
        {
            juce::Path path;
            for (int k = 0; k < kPoints; ++k)
            {
                const float x = r.getX() + r.getWidth() * (float) k / (float) (kPoints - 1);
                const float y = yFor (c[(size_t) k]);
                if (k == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
            }
            g.setColour (colour.withAlpha (alpha));
            g.strokePath (path, juce::PathStrokeType (thickness));
        };

        if (numActive > 1)
            for (int blk = 0; blk < kCabBlocks; ++blk)
                if (active[blk])
                    drawCurve (resp[blk], blockColour (blk), 1.0f, 0.5f);
        drawCurve (respMix, ui::text, 1.8f, 1.0f);
    }
}

//==============================================================================
// GainReductionMeter
//==============================================================================
void GainReductionMeter::timerCallback()
{
    if (! isShowing())
        return;

    const float target = proc.getCompGainReductionDb();
    // Quick to rise, slower to fall, so short dips can be seen
    const float before = shown;
    shown = (target > shown) ? target : shown + 0.25f * (target - shown);
    if (std::abs (shown - before) > 0.02f || (shown > 0.0f) != (before > 0.0f))
        repaint();
}

void GainReductionMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (ui::background);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (ui::panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    const float fraction = juce::jlimit (0.0f, 1.0f, shown / 20.0f);
    if (fraction > 0.003f)
    {
        g.setColour (ui::accent);
        g.fillRoundedRectangle (r.reduced (2.0f).withWidth ((r.getWidth() - 4.0f) * fraction), 2.0f);
    }

    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("GR " + juce::String (shown, 1) + " dB", r.toNearestInt().reduced (6, 0), juce::Justification::centredLeft);
}

//==============================================================================
// FaderControl
//==============================================================================
FaderControl::FaderControl (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId,
                            const juce::String& name)
{
    nameLabel.setText (name, juce::dontSendNotification);
    nameLabel.setJustificationType (juce::Justification::centred);
    nameLabel.setColour (juce::Label::textColourId, ui::textDim);
    nameLabel.setFont (juce::Font (juce::FontOptions (11.5f)));
    addAndMakeVisible (nameLabel);

    const auto tip = tipFor (apvts, paramId);
    nameLabel.setTooltip (tip);
    slider.setTooltip (tip);

    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 52, 16);
    addAndMakeVisible (slider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, paramId, slider);
    slider.textFromValueFunction = [] (double v) { return juce::String (v, 1); };
    slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    slider.updateText();
}

void FaderControl::resized()
{
    auto r = getLocalBounds();
    nameLabel.setBounds (r.removeFromTop (14));
    slider.setBounds (r);
}

//==============================================================================
// EqCurveView
//==============================================================================
void EqCurveView::timerCallback()
{
    auto get = [this] (const juce::String& id) { return proc.apvts.getRawParameterValue (id)->load(); };

    std::array<float, kEqBands + 6> now {};
    for (int b = 0; b < kEqBands; ++b)
        now[(size_t) b] = get ("eqB" + juce::String (b + 1));
    now[kEqBands]     = get ("eqMode");
    now[kEqBands + 1] = get ("eqQ");
    now[kEqBands + 2] = get ("eqOn");
    now[kEqBands + 3] = get ("eqLowCut");
    now[kEqBands + 4] = get ("eqHighCut");
    now[kEqBands + 5] = get ("eqSlope");

    if (now == lastValues)
        return;
    lastValues = now;
    enabled = now[kEqBands + 2] > 0.5f;

    const bool  modMode = now[kEqBands] > 0.5f;
    const float qKnob   = now[kEqBands + 1];
    const double sr     = 48000.0;

    curve.fill (0.0f);
    for (int b = 0; b < kEqBands; ++b)
    {
        if (std::abs (now[(size_t) b]) < 0.05f)
            continue;

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sr, kEqBandHz[b], eqBandQ (b, modMode, qKnob), juce::Decibels::decibelsToGain (now[(size_t) b]));

        for (int k = 0; k < kPoints; ++k)
        {
            const double f = 20.0 * std::pow (1000.0, (double) k / (double) (kPoints - 1));   // 20 Hz to 20 kHz
            curve[(size_t) k] += juce::Decibels::gainToDecibels ((float) coeffs->getMagnitudeForFrequency (f, sr));
        }
    }

    // The high-pass and low-pass
    const bool steep = now[kEqBands + 5] > 0.5f;
    auto addPass = [&] (bool high, float hz)
    {
        for (int stage = 0; stage < (steep ? 2 : 1); ++stage)
        {
            auto coeffs = high ? juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, hz, eqPassQ (steep, stage))
                               : juce::dsp::IIR::Coefficients<float>::makeLowPass  (sr, hz, eqPassQ (steep, stage));
            for (int k = 0; k < kPoints; ++k)
            {
                const double f = 20.0 * std::pow (1000.0, (double) k / (double) (kPoints - 1));
                curve[(size_t) k] += juce::Decibels::gainToDecibels ((float) coeffs->getMagnitudeForFrequency (f, sr));
            }
        }
    };
    if (now[kEqBands + 3] > 20.5f)    addPass (true,  now[kEqBands + 3]);
    if (now[kEqBands + 4] < 19900.0f) addPass (false, std::min (now[kEqBands + 4], 22000.0f));

    repaint();
}

//==============================================================================
// SpectrumView
//==============================================================================
SpectrumView::SpectrumView (StarterAmpProcessor& p)
    : Panel ("SPECTRUM  (what you are hearing)"), proc (p),
      fftData ((size_t) kFftSize * 2, 0.0f), samples ((size_t) kFftSize, 0.0f)
{
    static const char* names[3] = { "OUTPUT", "PRE-EQ", "BOTH" };
    for (int i = 0; i < 3; ++i)
    {
        auto& b = modeButtons[i];
        b.setButtonText (names[i]);
        b.setComponentID ("tab");
        b.setClickingTogglesState (true);
        b.setRadioGroupId (4713);
        b.onClick = [this, i] { mode = i; };
        addAndMakeVisible (b);
    }
    modeButtons[0].setToggleState (true, juce::dontSendNotification);
    modeButtons[0].setTooltip ("Show the final output spectrum.");
    modeButtons[1].setTooltip ("Show the spectrum just before the Post EQ (after the amp and cab).");
    modeButtons[2].setTooltip ("Show both, so you can see what the Post EQ changes.");

    for (auto* b : { &tiltButton, &holdButton })
    {
        b->setComponentID ("tab");
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    tiltButton.setTooltip ("Tilts the display up by 4.5 dB per octave so a typical guitar sound looks flat. Off = true spectrum.");
    holdButton.setTooltip ("Draws a thin line that holds recent peaks and slowly falls.");
    tiltButton.setToggleState (true, juce::dontSendNotification);   // tilt on: a typical guitar signal looks flatter
    holdButton.setToggleState (true, juce::dontSendNotification);

    outShown.fill (-120.0f); outPeaks.fill (-120.0f); preShown.fill (-120.0f); prePeaks.fill (-120.0f);
    startTimerHz (30);
}

void SpectrumView::resized()
{
    int x = getWidth() - 12;
    for (auto* b : { &holdButton, &tiltButton })
    {
        x -= 58;
        b->setBounds (x, 6, 54, 20);
        x -= 4;
    }
    x -= 10;
    for (int i = 2; i >= 0; --i)
    {
        x -= 70;
        modeButtons[i].setBounds (x, 6, 66, 20);
        x -= 2;
    }
}

void SpectrumView::analyse (const StarterAmpProcessor::AnalyserTap& tap, std::array<float, kPoints>& shown,
                            std::array<float, kPoints>& peaks, float& peakDb, float& rmsDb)
{
    tap.copyLatest (samples.data(), kFftSize);

    // Peak and RMS of this window, in dBFS
    float peak = 0.0f;
    double sum = 0.0;
    for (float s : samples) { peak = std::max (peak, std::abs (s)); sum += (double) s * s; }
    peakDb = juce::Decibels::gainToDecibels (peak, -120.0f);
    rmsDb  = juce::Decibels::gainToDecibels ((float) std::sqrt (sum / kFftSize), -120.0f);

    std::copy (samples.begin(), samples.end(), fftData.begin());
    std::fill (fftData.begin() + kFftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) kFftSize);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const double sr = proc.getSampleRate() > 0.0 ? proc.getSampleRate() : 48000.0;
    const double binHz = sr / kFftSize;
    const float  scale = 4.0f / (float) kFftSize;     // makes a full-scale sine read about 0 dB (Hann window)
    const bool tilt = tiltButton.getToggleState();
    const bool hold = holdButton.getToggleState();

    for (int i = 0; i < kPoints; ++i)
    {
        const double f  = 20.0 * std::pow (1000.0, (double) i / (kPoints - 1));
        const double lo = f / std::pow (2.0, 1.0 / 16.0), hi = f * std::pow (2.0, 1.0 / 16.0);
        int b0 = (int) std::floor (lo / binHz), b1 = (int) std::ceil (hi / binHz);
        b0 = juce::jlimit (1, kFftSize / 2 - 1, b0);
        b1 = juce::jlimit (b0, kFftSize / 2 - 1, b1);

        float m = 0.0f;
        for (int b = b0; b <= b1; ++b)
            m = std::max (m, fftData[(size_t) b]);

        float db = juce::Decibels::gainToDecibels (m * scale, -120.0f);
        if (tilt)
            db += 4.5f * (float) std::log2 (f / 1000.0);

        shown[(size_t) i] = db > shown[(size_t) i] ? db : shown[(size_t) i] + (db - shown[(size_t) i]) * 0.22f;
        peaks[(size_t) i] = hold ? std::max (shown[(size_t) i], peaks[(size_t) i] - 0.35f) : shown[(size_t) i];
    }
}

void SpectrumView::timerCallback()
{
    const bool showing = isShowing();
    proc.analyserWanted.store (showing, std::memory_order_relaxed);   // the audio thread only fills the taps while we are visible
    if (! showing)
        return;
    analyse (proc.outputTap, outShown, outPeaks, outPeakDb, outRmsDb);
    if (mode != 0)
        analyse (proc.preEqTap, preShown, prePeaks, prePeakDb, preRmsDb);
    repaint();
}

void SpectrumView::paint (juce::Graphics& g)
{
    Panel::paint (g);

    auto area = getLocalBounds().reduced (12, 8);
    area.removeFromTop (26);
    auto readout = area.removeFromBottom (16);
    area.removeFromBottom (2);
    area.removeFromLeft (30);                               // room for the dB labels

    g.setColour (ui::background);
    g.fillRoundedRectangle (area.toFloat(), 4.0f);

    const float topDb = -6.0f, bottomDb = -96.0f;
    auto yFor = [&] (float db) { return area.getBottom() - area.getHeight() * (juce::jlimit (bottomDb, topDb, db) - bottomDb) / (topDb - bottomDb); };
    auto xFor = [&] (double f) { return area.getX() + area.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };

    // Grid
    g.setFont (juce::Font (juce::FontOptions (9.5f)));
    for (float db = -90.0f; db <= -10.0f; db += 20.0f)
    {
        g.setColour (ui::panelEdge);
        g.drawHorizontalLine ((int) yFor (db), (float) area.getX(), (float) area.getRight());
        g.setColour (ui::textDim);
        g.drawText (juce::String ((int) db), area.getX() - 30, (int) yFor (db) - 6, 26, 12, juce::Justification::centredRight);
    }
    for (double f : { 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        g.setColour (ui::panelEdge);
        g.drawVerticalLine ((int) xFor (f), (float) area.getY(), (float) area.getBottom());
        g.setColour (ui::textDim);
        g.drawText (f >= 1000.0 ? juce::String (f / 1000.0, f == 1000.0 || f == 2000.0 || f == 5000.0 || f == 10000.0 ? 0 : 1) + "k" : juce::String ((int) f),
                    (int) xFor (f) + 2, area.getBottom() - 13, 30, 12, juce::Justification::centredLeft);
    }

    auto pathFor = [&] (const std::array<float, kPoints>& c, bool closed)
    {
        juce::Path path;
        for (int i = 0; i < kPoints; ++i)
        {
            const float x = area.getX() + area.getWidth() * (float) i / (float) (kPoints - 1);
            const float y = yFor (c[(size_t) i]);
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        if (closed)
        {
            path.lineTo ((float) area.getRight(), (float) area.getBottom());
            path.lineTo ((float) area.getX(), (float) area.getBottom());
            path.closeSubPath();
        }
        return path;
    };

    g.saveState();
    g.reduceClipRegion (area);

    if (mode != 0)      // pre-EQ spectrum, teal
    {
        g.setColour (ui::colourB.withAlpha (mode == 1 ? 0.25f : 0.0f));
        if (mode == 1) g.fillPath (pathFor (preShown, true));
        g.setColour (ui::colourB.withAlpha (0.9f));
        g.strokePath (pathFor (preShown, false), juce::PathStrokeType (1.3f));
    }
    if (mode != 1)      // output spectrum, accent
    {
        g.setColour (ui::accent.withAlpha (0.22f));
        g.fillPath (pathFor (outShown, true));
        g.setColour (ui::accent);
        g.strokePath (pathFor (outShown, false), juce::PathStrokeType (1.5f));
        if (holdButton.getToggleState())
        {
            g.setColour (ui::text.withAlpha (0.45f));
            g.strokePath (pathFor (outPeaks, false), juce::PathStrokeType (0.8f));
        }
    }
    g.restoreState();

    // Readout
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f)));
    juce::String text;
    if (mode != 1)
        text << (mode == 2 ? "Output (amber): " : "Output: ") << "peak " << juce::String (outPeakDb, 1) << " dBFS   RMS " << juce::String (outRmsDb, 1) << " dBFS";
    if (mode != 0)
        text << (mode == 2 ? "      Pre-EQ (teal): " : "Pre-EQ: ") << "peak " << juce::String (prePeakDb, 1) << " dBFS   RMS " << juce::String (preRmsDb, 1) << " dBFS";
    text << (tiltButton.getToggleState() ? "      Tilt on (+4.5 dB/octave)" : "");
    g.drawText (text, readout, juce::Justification::centredLeft);
}

void EqCurveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (ui::background);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (ui::panelEdge);
    g.drawRoundedRectangle (r, 4.0f, 1.0f);

    const float range = 15.0f;   // the display shows +/- 15 dB
    auto yFor = [&] (float db) { return r.getCentreY() - juce::jlimit (-range, range, db) / range * (r.getHeight() * 0.5f - 3.0f); };
    auto xFor = [&] (float hz) { return r.getX() + r.getWidth() * (float) (std::log (hz / 20.0f) / std::log (1000.0f)); };

    g.setColour (ui::panelEdge);
    for (float hz : kEqBandHz)
        g.drawVerticalLine ((int) xFor (hz), r.getY() + 1.0f, r.getBottom() - 1.0f);
    g.setColour (ui::textDim.withAlpha (0.6f));
    g.drawHorizontalLine ((int) r.getCentreY(), r.getX() + 1.0f, r.getRight() - 1.0f);

    juce::Path line;
    for (int k = 0; k < kPoints; ++k)
    {
        const float x = r.getX() + r.getWidth() * (float) k / (float) (kPoints - 1);
        if (k == 0) line.startNewSubPath (x, yFor (curve[(size_t) k]));
        else        line.lineTo (x, yFor (curve[(size_t) k]));
    }

    const float alpha = enabled ? 1.0f : 0.35f;
    juce::Path fill (line);
    fill.lineTo (r.getRight(), r.getCentreY());
    fill.lineTo (r.getX(), r.getCentreY());
    fill.closeSubPath();
    g.setColour (ui::accent.withAlpha (0.15f * alpha));
    g.fillPath (fill);
    g.setColour (ui::accent.withAlpha (alpha));
    g.strokePath (line, juce::PathStrokeType (1.8f));
}

//==============================================================================
// PostEqPanel
//==============================================================================
void PostEqPanel::setup (std::vector<std::unique_ptr<FaderControl>>& fadersIn, std::vector<KnobControl*> knobsIn)
{
    addAndMakeVisible (curve);
    for (auto& f : fadersIn)
    {
        addAndMakeVisible (*f);
        faders.push_back (f.get());
    }
    knobs = std::move (knobsIn);
    for (auto* k : knobs)
        addAndMakeVisible (*k);
}

void PostEqPanel::resized()
{
    Panel::resized();   // title and the on / mode row

    auto area = getLocalBounds().reduced (12, 8);
    area.removeFromTop (22 + 26 + 4);

    // Faders on the left, curve and knobs on the right
    const int faderW = 62;
    auto faderArea = area.removeFromLeft (faderW * (int) faders.size());
    for (auto* f : faders)
        f->setBounds (faderArea.removeFromLeft (faderW));

    area.removeFromLeft (10);
    curve.setBounds (area.removeFromTop (area.getHeight() / 2 - 4));
    area.removeFromTop (8);
    if (! knobs.empty())
    {
        const int w = area.getWidth() / (int) knobs.size();
        for (auto* k : knobs)
            k->setBounds (area.removeFromLeft (w));
    }
}

//==============================================================================
// MeterView
//==============================================================================
void MeterView::timerCallback()
{
    const float v[2] { proc.meterIn.exchange (0.0f, std::memory_order_relaxed), proc.meterOut.exchange (0.0f, std::memory_order_relaxed) };
    bool changed = false;
    for (int i = 0; i < 2; ++i)
    {
        const float old = level[i], oldHold = hold[i];
        level[i] = std::max (v[i], level[i] * 0.88f);
        if (v[i] >= 0.999f && ! clip[i]) { clip[i] = true; changed = true; }
        if (v[i] >= hold[i]) { hold[i] = v[i]; holdAge[i] = 0; }
        else if (++holdAge[i] > 45) hold[i] *= 0.9f;
        if (level[i] < 1.0e-5f) level[i] = 0.0f;
        if (hold[i] < 1.0e-5f) hold[i] = 0.0f;
        changed = changed || std::abs (level[i] - old) > 1.0e-4f || std::abs (hold[i] - oldHold) > 1.0e-4f;
    }
    if (changed)
        repaint();
}

void MeterView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const char* names[2] { "IN", "OUT" };
    const float rowH = r.getHeight() / 2.0f;
    constexpr int segments = 36;
    auto dbOf = [] (float v) { return 20.0f * std::log10 (std::max (v, 1.0e-5f)); };

    for (int i = 0; i < 2; ++i)
    {
        auto row = r.removeFromTop (rowH).reduced (0.0f, 3.0f);
        g.setColour (ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (names[i], row.removeFromLeft (26.0f), juce::Justification::centredLeft);
        auto clipLed = row.removeFromRight (12.0f);
        g.setColour (clip[i] ? juce::Colour (0xffff3b30) : juce::Colour (0xff3a1210));
        g.fillEllipse (clipLed.withSizeKeepingCentre (8.0f, 8.0f));

        const float segW = row.getWidth() / (float) segments;
        const float lvDb = dbOf (level[i]);
        const float holdDb = dbOf (hold[i]);
        for (int s = 0; s < segments; ++s)
        {
            const float segDb = -60.0f + 60.0f * (float) (s + 1) / (float) segments;
            const bool lit = lvDb >= segDb - 0.01f && level[i] > 0.0f;
            juce::Colour c = segDb > -0.5f ? juce::Colour (0xffff3b30) : (segDb > -6.0f ? juce::Colour (0xffffa030) : (segDb > -18.0f ? juce::Colour (0xffe8d84a) : juce::Colour (0xff3fd47a)));
            g.setColour (lit ? c : c.withAlpha (0.14f));
            g.fillRect (row.getX() + (float) s * segW, row.getY(), std::max (1.0f, segW - 1.0f), row.getHeight());
        }
        if (hold[i] > 0.0f)
        {
            const float hx = row.getX() + row.getWidth() * juce::jlimit (0.0f, 1.0f, (holdDb + 60.0f) / 60.0f);
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.fillRect (hx - 1.0f, row.getY(), 1.5f, row.getHeight());
        }
    }
}

//==============================================================================
// TunerView
//==============================================================================
TunerView::TunerView (StarterAmpProcessor& p) : Panel ("TUNER  (OUTPUT MUTED WHILE OPEN)"), proc (p)
{
    setStyle (PanelKind::Rack);
    setVisible (false);
    refSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    refSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 70, 18);
    refSlider.textFromValueFunction = [] (double v) { return "A = " + juce::String ((int) std::lround (v)) + " Hz"; };
    refSlider.setTooltip ("The reference pitch for A (427 to 453 Hz). 440 is the standard. Double-click to reset.");
    refAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "tunerRef", refSlider);
    refSlider.updateText();
    addAndMakeVisible (refSlider);
}

void TunerView::resized()
{
    Panel::resized();
    refSlider.setBounds (getWidth() - 250, 6, 232, 18);
}

void TunerView::visibilityChanged()
{
    if (isVisible())
    {
        midi = -1;
        missed = 100;
        startTimerHz (20);
    }
    else
        stopTimer();
}

float TunerView::detectPitch (const std::vector<float>& x, double sr)
{
    constexpr int W = 2048;
    const int tauMax = std::min (W, (int) (sr / 30.0));          // down to 30 Hz
    const int tauMin = std::max (2, (int) (sr / 1100.0));        // up to about 1.1 kHz
    if ((int) x.size() < W + tauMax + 1 || tauMax <= tauMin + 2)
        return 0.0f;

    const float* d0 = x.data() + (x.size() - (size_t) (W + tauMax + 1));
    std::vector<float> d ((size_t) tauMax + 1, 0.0f);
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        float sum = 0.0f;
        for (int j = 0; j < W; ++j)
        {
            const float diff = d0[j] - d0[j + tau];
            sum += diff * diff;
        }
        d[(size_t) tau] = sum;
    }

    // Cumulative mean normalised difference
    float running = 0.0f;
    d[0] = 1.0f;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        running += d[(size_t) tau];
        d[(size_t) tau] = running > 0.0f ? d[(size_t) tau] * (float) tau / running : 1.0f;
    }

    int best = -1;
    for (int tau = tauMin; tau < tauMax; ++tau)
        if (d[(size_t) tau] < 0.15f)
        {
            while (tau + 1 < tauMax && d[(size_t) tau + 1] < d[(size_t) tau])
                ++tau;
            best = tau;
            break;
        }
    if (best < 0)
    {
        float lowest = 0.3f;
        for (int tau = tauMin; tau < tauMax; ++tau)
            if (d[(size_t) tau] < lowest) { lowest = d[(size_t) tau]; best = tau; }
    }
    if (best < 0)
        return 0.0f;

    float tauF = (float) best;
    if (best > 1 && best < tauMax)
    {
        const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
        const float denom = a - 2.0f * b + c;
        if (std::abs (denom) > 1.0e-9f)
            tauF += 0.5f * (a - c) / denom;
    }
    return tauF > 0.0f ? (float) (sr / (double) tauF) : 0.0f;
}

void TunerView::timerCallback()
{
    const double sr = proc.getSampleRate();
    if (sr <= 0.0)
        return;
    const bool decimate = sr > 60000.0;
    const int rawCount = decimate ? 8192 : 4096;
    std::vector<float> raw ((size_t) rawCount);
    proc.tunerTap.copyLatest (raw.data(), rawCount);

    std::vector<float> x;
    if (decimate)
    {
        x.resize ((size_t) rawCount / 2);
        for (size_t i = 0; i < x.size(); ++i)
            x[i] = 0.5f * (raw[2 * i] + raw[2 * i + 1]);
    }
    else
        x = std::move (raw);

    double energy = 0.0;
    for (float v : x)
        energy += (double) v * (double) v;
    rms = (float) std::sqrt (energy / (double) x.size());

    const float f = rms > 0.004f ? detectPitch (x, decimate ? sr * 0.5 : sr) : 0.0f;
    if (f > 20.0f)
    {
        const float m = 69.0f + 12.0f * std::log2 (f / proc.apvts.getRawParameterValue ("tunerRef")->load());
        const int mi = (int) std::lround (m);
        const float c = 100.0f * (m - (float) mi);
        cents = (mi == midi && missed < 3) ? cents + 0.5f * (c - cents) : c;
        midi = mi;
        freq = f;
        missed = 0;
    }
    else if (++missed > 10)
        midi = -1;
    repaint();
}

void TunerView::paint (juce::Graphics& g)
{
    Panel::paint (g);
    auto area = getLocalBounds().reduced (26, 8).toFloat();
    area.removeFromTop (24.0f);

    static const char* names[12] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    auto bar = area.removeFromBottom (44.0f).reduced (10.0f, 6.0f);

    if (midi < 0)
    {
        g.setColour (ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (15.0f)));
        g.drawText ("Play a single string", area, juce::Justification::centred);
    }
    else
    {
        const bool inTune = std::abs (cents) <= 5.0f;
        const juce::Colour c = inTune ? juce::Colour (0xff4bff8a) : (std::abs (cents) <= 15.0f ? juce::Colour (0xffe8d84a) : juce::Colour (0xffff7a45));
        g.setColour (c);
        g.setFont (juce::Font (juce::FontOptions (58.0f, juce::Font::bold)));
        g.drawText (names[((midi % 12) + 12) % 12], area.withSizeKeepingCentre (110.0f, area.getHeight()).translated (-14.0f, 0.0f), juce::Justification::centredRight);
        g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
        g.drawText (juce::String (midi / 12 - 1), juce::Rectangle<float> (area.getCentreX() + 40.0f, area.getCentreY() + 4.0f, 30.0f, 26.0f), juce::Justification::centredLeft);
        g.setColour (ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (juce::String (freq, 1) + " Hz", area.removeFromLeft (110.0f), juce::Justification::centred);
        g.drawText ((cents >= 0 ? "+" : "") + juce::String (juce::roundToInt (cents)) + " cents  " + (inTune ? "IN TUNE" : (cents < 0 ? "FLAT" : "SHARP")),
                    area.removeFromRight (150.0f), juce::Justification::centred);
    }

    // Cents scale, -50 to +50, with the in-tune window lit
    g.setColour (juce::Colour (0xff0a0b0d));
    g.fillRoundedRectangle (bar, 4.0f);
    const float cx = bar.getCentreX(), half = bar.getWidth() * 0.5f;
    g.setColour (juce::Colour (0xff4bff8a).withAlpha (0.18f));
    g.fillRect (cx - half * 0.1f, bar.getY(), half * 0.2f, bar.getHeight());
    for (int t = -5; t <= 5; ++t)
    {
        g.setColour (ui::textDim.withAlpha (t == 0 ? 0.9f : 0.5f));
        const float tx = cx + half * (float) t / 5.0f;
        g.drawLine (tx, bar.getY() + (t == 0 ? 1.0f : 5.0f), tx, bar.getBottom() - (t == 0 ? 1.0f : 5.0f), t == 0 ? 2.0f : 1.0f);
    }
    if (midi >= 0)
    {
        const float nx = cx + half * juce::jlimit (-1.0f, 1.0f, cents / 50.0f);
        g.setColour (std::abs (cents) <= 5.0f ? juce::Colour (0xff4bff8a) : juce::Colour (0xffffc850));
        g.fillRoundedRectangle (nx - 3.0f, bar.getY() + 1.0f, 6.0f, bar.getHeight() - 2.0f, 2.5f);
    }
}

//==============================================================================
// MicView
//==============================================================================
namespace
{
    const juce::Colour micColour[kCabBlocks] { ui::accent, ui::colourB, ui::colourC, ui::colourD };
    const float micAxis[kCabBlocks] { -0.55f, -2.59f, 0.55f, 2.59f };    // direction of each block's marker in the front view (radians)
    constexpr float kMicLength = 34.0f;
}

MicView::MicView (juce::AudioProcessorValueTreeState& a) : Panel ("MIC POSITION"), apvts (a)
{
    setStyle (PanelKind::Rack);
    setTooltip ("The built-in cab blocks as seen by their microphones. Left: the speaker from the front; each coloured marker is one block's mic "
                "(centre of the cone to the edge). Right: the speaker from the side, with the selected block's mic at its distance and angle. "
                "Drag a marker to move the mic; drag the small dot on the back of the mic to tilt it (or use the mouse wheel). "
                "Click A to D to choose a block. Blocks that are off or use an IR file have no mic.");
    startTimerHz (15);
}

MicView::Geo MicView::geo() const
{
    auto area = getLocalBounds().reduced (24, 8).toFloat();      // clear of the rack ears
    area.removeFromTop (22.0f);
    Geo gm;
    gm.chips = area.removeFromBottom (20.0f);
    area.removeFromBottom (4.0f);
    const float sq = juce::jmin (area.getHeight(), area.getWidth() * 0.45f);
    auto front = area.removeFromLeft (sq);
    gm.fc = front.getCentre();
    gm.fr = sq * 0.5f - 2.0f;
    area.removeFromLeft (8.0f);
    gm.side = area;
    gm.yc = area.getCentreY();
    gm.x0 = area.getX() + 38.0f;
    gm.rr = juce::jmin (area.getHeight() * 0.5f - 8.0f, 46.0f);
    gm.k  = juce::jmax (4.0f, (area.getRight() - 20.0f - (gm.x0 + 8.0f)) / 12.0f);
    return gm;
}

float MicView::val (const char* base, int blk) const
{
    if (auto* p = apvts.getRawParameterValue (juce::String (base) + cabLetter (blk)))
        return p->load();
    return 0.0f;
}

void MicView::setParam (const char* base, int blk, float v)
{
    if (auto* p = apvts.getParameter (juce::String (base) + cabLetter (blk)))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

bool MicView::usable (int blk) const
{
    return val ("cabOn", blk) > 0.5f && val ("cabSrc", blk) < 0.5f;
}

juce::Point<float> MicView::markerPos (const Geo& gm, int blk) const
{
    const float r = juce::jlimit (0.0f, 1.0f, val ("cabPos", blk)) * gm.fr * 0.86f;
    return { gm.fc.x + r * std::cos (micAxis[blk]), gm.fc.y + r * std::sin (micAxis[blk]) };
}

juce::Point<float> MicView::micHead (const Geo& gm, int blk) const
{
    return { gm.x0 + 8.0f + juce::jlimit (0.0f, 12.0f, val ("cabDist", blk)) * gm.k,
             gm.yc - juce::jlimit (0.0f, 1.0f, val ("cabPos", blk)) * gm.rr };
}

juce::Rectangle<float> MicView::chipRect (const Geo& gm, int blk) const
{
    return { gm.chips.getX() + (float) blk * 32.0f, gm.chips.getY(), 28.0f, gm.chips.getHeight() };
}

void MicView::timerCallback()
{
    std::vector<float> state { (float) selected };
    for (int b = 0; b < kCabBlocks; ++b)
        for (auto* id : { "cabOn", "cabSrc", "cabPos", "cabDist", "cabAng", "cabMic" })
            state.push_back (val (id, b));
    if (state != lastState)
    {
        lastState = std::move (state);
        repaint();
    }
}

void MicView::paint (juce::Graphics& g)
{
    Panel::paint (g);
    const auto gm = geo();
    const float Ro = gm.fr, Rc = Ro * 0.86f;
    const juce::Font small (juce::FontOptions (9.5f, juce::Font::bold));

    // ---- Front view: the speaker on its baffle
    g.setColour (juce::Colour (0xff09090b));
    g.fillRoundedRectangle (gm.fc.x - Ro - 1.0f, gm.fc.y - Ro - 1.0f, 2.0f * Ro + 2.0f, 2.0f * Ro + 2.0f, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (gm.fc.x - Ro - 1.0f, gm.fc.y - Ro - 1.0f, 2.0f * Ro + 2.0f, 2.0f * Ro + 2.0f, 6.0f, 1.0f);
    g.setColour (juce::Colour (0xff1d1d21));
    g.fillEllipse (gm.fc.x - Ro + 2.0f, gm.fc.y - Ro + 2.0f, 2.0f * Ro - 4.0f, 2.0f * Ro - 4.0f);          // surround
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff55555d), gm.fc.x - Rc * 0.35f, gm.fc.y - Rc * 0.4f,
                                             juce::Colour (0xff141417), gm.fc.x + Rc, gm.fc.y + Rc, true));
    g.fillEllipse (gm.fc.x - Rc, gm.fc.y - Rc, 2.0f * Rc, 2.0f * Rc);                                      // cone
    for (float t : { 0.40f, 0.62f, 0.82f })
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawEllipse (gm.fc.x - Rc * t, gm.fc.y - Rc * t, 2.0f * Rc * t, 2.0f * Rc * t, 1.0f);
    }
    const float cap = Rc * 0.22f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a7a84), gm.fc.x - cap * 0.4f, gm.fc.y - cap * 0.5f,
                                             juce::Colour (0xff202024), gm.fc.x + cap, gm.fc.y + cap, true));
    g.fillEllipse (gm.fc.x - cap, gm.fc.y - cap, 2.0f * cap, 2.0f * cap);                                  // dust cap

    for (int pass = 0; pass < 2; ++pass)
        for (int b = 0; b < kCabBlocks; ++b)
        {
            if (! usable (b) || (pass == 0) == (b == selected))
                continue;
            const auto m = markerPos (gm, b);
            const bool sel = b == selected;
            g.setColour (micColour[b].withAlpha (sel ? 0.55f : 0.25f));
            g.drawLine (gm.fc.x, gm.fc.y, m.x, m.y, 1.0f);
            g.setColour (micColour[b].withAlpha (sel ? 0.30f : 0.0f));
            g.fillEllipse (m.x - 13.0f, m.y - 13.0f, 26.0f, 26.0f);
            g.setColour (micColour[b].withAlpha (sel ? 1.0f : 0.8f));
            g.fillEllipse (m.x - 8.0f, m.y - 8.0f, 16.0f, 16.0f);
            g.setColour (juce::Colours::black.withAlpha (0.7f));
            g.drawEllipse (m.x - 8.0f, m.y - 8.0f, 16.0f, 16.0f, 1.2f);
            g.setFont (small);
            g.drawText (cabLetter (b), juce::Rectangle<float> (m.x - 8.0f, m.y - 8.0f, 16.0f, 16.0f), juce::Justification::centred);
        }

    // ---- Side view: the selected block's mic at its distance and angle
    g.setColour (ui::textDim);
    g.setFont (small);
    if (! usable (selected))
    {
        g.drawText (val ("cabOn", selected) < 0.5f ? "Block " + cabLetter (selected) + " is off" : "Block " + cabLetter (selected) + " uses an IR file (no mic)",
                    gm.side.toNearestInt(), juce::Justification::centred);
    }
    else
    {
        const float x0 = gm.x0, yc = gm.yc, rr = gm.rr;
        g.setColour (juce::Colours::white.withAlpha (0.12f));                               // axis
        const float dashes[] { 3.0f, 4.0f };
        g.drawDashedLine (juce::Line<float> (x0, yc, gm.side.getRight() - 6.0f, yc), dashes, 2, 1.0f);

        juce::Path coneShape;                                                               // the cone, seen from the side (front faces right)
        coneShape.startNewSubPath (x0, yc - rr);
        coneShape.lineTo (x0 - 15.0f, yc - rr * 0.2f);
        coneShape.lineTo (x0 - 15.0f, yc + rr * 0.2f);
        coneShape.lineTo (x0, yc + rr);
        coneShape.closeSubPath();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a5a63), x0 - 15.0f, yc - rr, juce::Colour (0xff1b1b1f), x0, yc + rr, false));
        g.fillPath (coneShape);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.strokePath (coneShape, juce::PathStrokeType (1.0f));
        g.setColour (juce::Colour (0xff2a2a30));                                            // magnet
        g.fillRoundedRectangle (x0 - 36.0f, yc - rr * 0.38f, 22.0f, rr * 0.76f, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.drawRoundedRectangle (x0 - 36.0f, yc - rr * 0.38f, 22.0f, rr * 0.76f, 3.0f, 1.0f);
        g.setColour (juce::Colour (0xff0d0d0f));                                            // baffle
        g.fillRect (x0 - 2.0f, yc - rr - 9.0f, 5.0f, 2.0f * rr + 18.0f);

        const int b = selected;
        const auto head = micHead (gm, b);
        const float ang = juce::degreesToRadians (juce::jlimit (0.0f, 60.0f, val ("cabAng", b)));
        const juce::Point<float> tail (head.x + kMicLength * std::cos (ang), head.y - kMicLength * std::sin (ang));

        g.setColour (micColour[b].withAlpha (0.6f));                                        // distance
        g.drawDashedLine (juce::Line<float> (x0 + 8.0f, head.y, head.x, head.y), dashes, 2, 1.0f);
        g.setColour (ui::text.withAlpha (0.85f));
        g.drawText (juce::String (val ("cabDist", b), 1) + " in",
                    juce::Rectangle<float> ((x0 + 8.0f + head.x) * 0.5f - 24.0f, head.y + 2.0f, 48.0f, 12.0f), juce::Justification::centred);

        juce::Path mic;                                                                     // the mic: grille at the head, body behind it
        mic.addRoundedRectangle (0.0f, -5.5f, kMicLength, 11.0f, 3.5f);
        const auto xf = juce::AffineTransform::rotation (-ang).translated (head.x, head.y);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a52), head.x, head.y - 6.0f, juce::Colour (0xff131316), head.x, head.y + 6.0f, false));
        g.fillPath (mic, xf);
        juce::Path grille;
        grille.addRoundedRectangle (0.0f, -5.5f, 9.0f, 11.0f, 3.5f);
        g.setColour (micColour[b].withAlpha (0.85f));
        g.fillPath (grille, xf);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.strokePath (mic, juce::PathStrokeType (1.0f), xf);
        g.setColour (ui::text);
        g.fillEllipse (tail.x - 3.5f, tail.y - 3.5f, 7.0f, 7.0f);                           // tilt handle
        g.setColour (ui::textDim);
        g.drawText (juce::String (juce::roundToInt (val ("cabAng", b))) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")),
                    juce::Rectangle<float> (tail.x + 4.0f, tail.y - 6.0f, 30.0f, 12.0f), juce::Justification::centredLeft);
    }

    // ---- Block chips and a readout
    for (int b = 0; b < kCabBlocks; ++b)
    {
        const auto cr = chipRect (gm, b);
        const bool sel = b == selected;
        g.setColour (micColour[b].withAlpha (sel ? 0.95f : (usable (b) ? 0.30f : 0.10f)));
        g.fillRoundedRectangle (cr, 5.0f);
        g.setColour (sel ? juce::Colours::black : ui::text.withAlpha (usable (b) ? 0.9f : 0.4f));
        g.setFont (small);
        g.drawText (cabLetter (b), cr, juce::Justification::centred);
    }
    if (usable (selected))
    {
        juce::String micName;
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter ("cabMic" + cabLetter (selected))))
            micName = choice->getCurrentChoiceName();
        g.setColour (ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (10.0f)));
        g.drawText (micName + "   pos " + juce::String (val ("cabPos", selected), 2) + "   " + juce::String (val ("cabDist", selected), 1) + " in   "
                        + juce::String (juce::roundToInt (val ("cabAng", selected))) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")),
                    gm.chips.withTrimmedLeft (4.0f * 32.0f + 6.0f), juce::Justification::centredLeft);
    }
}

void MicView::beginGestures()
{
    if (gesturesOpen)
        return;
    for (auto* base : { "cabPos", "cabDist", "cabAng" })
        if (auto* p = apvts.getParameter (juce::String (base) + cabLetter (selected)))
            p->beginChangeGesture();
    gesturesOpen = true;
}

void MicView::endGestures()
{
    if (! gesturesOpen)
        return;
    for (auto* base : { "cabPos", "cabDist", "cabAng" })
        if (auto* p = apvts.getParameter (juce::String (base) + cabLetter (selected)))
            p->endChangeGesture();
    gesturesOpen = false;
}

void MicView::applyDrag (juce::Point<float> p)
{
    const auto gm = geo();
    if (dragMode == 1)
    {
        setParam ("cabPos", selected, juce::jlimit (0.0f, 1.0f, p.getDistanceFrom (gm.fc) / (gm.fr * 0.86f)));
    }
    else if (dragMode == 2)
    {
        setParam ("cabDist", selected, juce::jlimit (0.0f, 12.0f, (p.x - (gm.x0 + 8.0f)) / gm.k));
        setParam ("cabPos", selected, juce::jlimit (0.0f, 1.0f, (gm.yc - p.y) / gm.rr));
    }
    else if (dragMode == 3)
    {
        const auto head = micHead (gm, selected);
        const float deg = juce::radiansToDegrees (std::atan2 (head.y - p.y, p.x - head.x));
        setParam ("cabAng", selected, juce::jlimit (0.0f, 60.0f, deg));
    }
}

void MicView::mouseDown (const juce::MouseEvent& ev)
{
    const auto gm = geo();
    const auto p = ev.position;
    dragMode = 0;

    for (int b = 0; b < kCabBlocks; ++b)
        if (chipRect (gm, b).contains (p))
        {
            endGestures();
            selected = b;
            repaint();
            return;
        }

    if (p.getDistanceFrom (gm.fc) <= gm.fr + 4.0f)
    {
        int best = -1;
        float bestD = 14.0f;
        for (int b = 0; b < kCabBlocks; ++b)
            if (usable (b) && markerPos (gm, b).getDistanceFrom (p) < bestD)
            {
                bestD = markerPos (gm, b).getDistanceFrom (p);
                best = b;
            }
        if (best >= 0 && best != selected)
        {
            endGestures();
            selected = best;
        }
        if (! usable (selected))
            return;
        dragMode = 1;
    }
    else if (gm.side.contains (p) && usable (selected))
    {
        const auto head = micHead (gm, selected);
        const float ang = juce::degreesToRadians (val ("cabAng", selected));
        const juce::Point<float> tail (head.x + kMicLength * std::cos (ang), head.y - kMicLength * std::sin (ang));
        dragMode = p.getDistanceFrom (tail) <= 9.0f ? 3 : 2;
    }
    else
        return;

    beginGestures();
    applyDrag (p);
    repaint();
}

void MicView::mouseDrag (const juce::MouseEvent& ev)
{
    if (dragMode != 0)
    {
        applyDrag (ev.position);
        repaint();
    }
}

void MicView::mouseUp (const juce::MouseEvent&)
{
    dragMode = 0;
    endGestures();
}

void MicView::mouseWheelMove (const juce::MouseEvent& ev, const juce::MouseWheelDetails& wheel)
{
    if (usable (selected) && geo().side.contains (ev.position))
        setParam ("cabAng", selected, juce::jlimit (0.0f, 60.0f, val ("cabAng", selected) + wheel.deltaY * 20.0f));
}


//==============================================================================
// AmpHero (v61)
//==============================================================================
void AmpHero::paint (juce::Graphics& g)
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0)
        return;
    const float scale = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
    const int pw = juce::roundToInt ((float) w * scale), ph = juce::roundToInt ((float) h * scale);
    if (! cache.isValid() || cache.getWidth() != pw || cache.getHeight() != ph)
    {
        cache = juce::Image (juce::Image::ARGB, pw, ph, true);
        juce::Graphics cg (cache);
        cg.addTransform (juce::AffineTransform::scale (scale));
        render (cg, w, h);
    }
    g.drawImage (cache, 0, 0, w, h, 0, 0, pw, ph);
}

void AmpHero::render (juce::Graphics& g, int w, int h) const
{
    const auto& th = ampThemeFor (theme);
    const juce::Colour glow (th.tick);
    juce::Random rng (name.hashCode() + theme);
    const float W = (float) w, H = (float) h;

    // a soft light behind the stack, in the amp's accent colour
    g.setGradientFill (juce::ColourGradient (glow.withAlpha (0.16f), W * 0.5f, H * 0.42f, glow.withAlpha (0.0f), W * 0.5f, H * 0.0f, true));
    g.fillRect (0.0f, 0.0f, W, H);

    auto cloth = [&] (juce::Rectangle<float> r, bool dim)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (th.clothTop), 0.0f, r.getY(), juce::Colour (th.clothBottom), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.0f);
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (r.toNearestInt());
        const juce::Colour f (th.fleck);
        const float a = dim ? 0.5f : 1.0f;
        switch (th.clothStyle)
        {
            case 1:
                for (float k = -r.getHeight(); k < r.getWidth(); k += 9.0f)
                {
                    g.setColour (f.withAlpha (0.20f * a));
                    g.drawLine (r.getX() + k, r.getY(), r.getX() + k + r.getHeight(), r.getBottom(), 1.0f);
                    g.drawLine (r.getX() + k + r.getHeight(), r.getY(), r.getX() + k, r.getBottom(), 1.0f);
                }
                break;
            case 3:
                for (float gy = r.getY(); gy < r.getBottom(); gy += 6.0f)
                    for (float gx = r.getX(); gx < r.getRight(); gx += 6.0f)
                    {
                        g.setColour (f.withAlpha (0.18f * a));
                        if (((int) (gx / 6.0f) + (int) (gy / 6.0f)) % 2 == 0) g.fillRect (gx, gy + 1.0f, 6.0f, 2.0f);
                        else                                                   g.fillRect (gx + 1.0f, gy, 2.0f, 6.0f);
                    }
                break;
            case 4:
                for (float gx = r.getX(); gx < r.getRight(); gx += 9.0f)
                {
                    g.setColour (f.withAlpha (0.40f * a));
                    g.fillRect (gx, r.getY(), 1.0f, r.getHeight());
                }
                break;
            case 5:
                for (float gy = r.getY() + 3.0f; gy < r.getBottom(); gy += 7.0f)
                    for (float gx = r.getX() + 3.0f; gx < r.getRight(); gx += 7.0f)
                    {
                        g.setColour (f.withAlpha (0.55f * a));
                        g.fillEllipse (gx - 1.0f, gy - 1.0f, 2.0f, 2.0f);
                    }
                break;
            default:
                for (int i = 0; i < 900; ++i)
                {
                    g.setColour (rng.nextBool() ? f.withAlpha (0.30f * a) : juce::Colours::black.withAlpha (0.30f));
                    g.fillRect (r.getX() + rng.nextFloat() * r.getWidth(), r.getY() + rng.nextFloat() * r.getHeight(), 1.3f, 1.3f);
                }
                break;
        }
    };
    auto tolex = [&] (juce::Rectangle<float> r, float radius)
    {
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 5.0f).expanded (2.0f, 0.0f), radius + 2.0f);       // contact shadow
        g.setGradientFill (juce::ColourGradient (juce::Colour (th.tolexTop), 0.0f, r.getY(), juce::Colour (th.tolexBottom), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, radius);
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;  clip.addRoundedRectangle (r, radius);
        g.reduceClipRegion (clip);
        for (int i = 0; i < 520; ++i)       // pebbled grain
        {
            g.setColour (rng.nextBool() ? juce::Colours::white.withAlpha (0.045f) : juce::Colours::black.withAlpha (0.12f));
            g.fillRect (r.getX() + rng.nextFloat() * r.getWidth(), r.getY() + rng.nextFloat() * r.getHeight(), 1.2f, 1.2f);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.10f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX() + r.getWidth() * 0.5f, r.getY() + r.getHeight() * 0.5f, false));
        g.fillRect (r);
        g.setGradientFill (juce::ColourGradient (juce::Colour (th.pipeA), r.getX(), 0.0f, juce::Colour (th.pipeB), r.getRight(), 0.0f, false));
        g.drawRoundedRectangle (r.reduced (3.0f), radius - 2.0f, 1.6f);                                 // piping
    };

    // ---- the cab, behind and below the head
    const juce::Rectangle<float> cab (W * 0.07f, H * 0.40f, W * 0.86f, H * 0.57f);
    tolex (cab, 10.0f);
    {
        const auto grille = cab.reduced (16.0f, 16.0f).withTrimmedTop (14.0f);
        cloth (grille, false);
        // four speakers showing through the cloth
        for (int iy = 0; iy < 2; ++iy)
            for (int ix = 0; ix < 2; ++ix)
            {
                const float cx = grille.getX() + grille.getWidth() * (0.25f + 0.5f * (float) ix);
                const float cy = grille.getY() + grille.getHeight() * (0.25f + 0.5f * (float) iy);
                const float rr = std::min (grille.getWidth(), grille.getHeight()) * 0.22f;
                g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.55f), cx, cy, juce::Colours::black.withAlpha (0.10f), cx + rr, cy, true));
                g.fillEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f);
                g.setColour (juce::Colours::white.withAlpha (0.07f));
                g.drawEllipse (cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 1.0f);
                g.drawEllipse (cx - rr * 0.45f, cy - rr * 0.45f, rr * 0.9f, rr * 0.9f, 1.0f);
            }
        for (const auto& c : { grille.getTopLeft(), grille.getTopRight(), grille.getBottomLeft(), grille.getBottomRight() })
        {
            g.setColour (juce::Colour (th.capA).withAlpha (0.85f));
            g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        }
    }

    // ---- the head
    const juce::Rectangle<float> head (W * 0.04f, H * 0.075f, W * 0.92f, H * 0.30f);
    {   // handle
        juce::Path handle;
        const float hx0 = head.getCentreX() - 46.0f, hx1 = head.getCentreX() + 46.0f, hy = head.getY();
        handle.startNewSubPath (hx0, hy + 2.0f);  handle.quadraticTo (hx0 + 6.0f, hy - 16.0f, hx0 + 22.0f, hy - 16.0f);
        handle.lineTo (hx1 - 22.0f, hy - 16.0f);  handle.quadraticTo (hx1 - 6.0f, hy - 16.0f, hx1, hy + 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.strokePath (handle, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (th.pipeB).withAlpha (0.9f));
        g.strokePath (handle, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    tolex (head, 9.0f);
    {   // vent slots along the top
        for (int i = 0; i < 18; ++i)
        {
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (head.getX() + 22.0f + (float) i * ((head.getWidth() - 44.0f) / 17.0f) - 2.0f, head.getY() + 9.0f, 4.0f, 7.0f, 1.5f);
        }
    }
    const auto plate = head.reduced (14.0f, 0.0f).withTrimmedTop (24.0f).withTrimmedBottom (13.0f);
    {
        g.setGradientFill (juce::ColourGradient (juce::Colour (th.plateA), 0.0f, plate.getY(), juce::Colour (th.plateB), 0.0f, plate.getBottom(), false));
        g.fillRoundedRectangle (plate, 4.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawRoundedRectangle (plate, 4.0f, 1.0f);
        // the model name, engraved on the left
        g.setColour (juce::Colour (th.plateText));
        drawSpaced (g, name, plate.getX() + 12.0f, plate.getY() + 6.0f, 14.0f, 1.8f, plate.getX() + plate.getWidth() * 0.48f,
                    juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
        g.setColour (juce::Colour (th.plateText).withAlpha (0.45f));
        g.fillRect (plate.getX() + 12.0f, plate.getY() + 24.0f, plate.getWidth() * 0.36f, 1.0f);
        // eight knobs along the right, each with its pointer at a different angle
        const int nk = 8;
        const float kx0 = plate.getX() + plate.getWidth() * 0.50f, kx1 = plate.getRight() - 22.0f;
        for (int i = 0; i < nk; ++i)
        {
            const float cx = kx0 + (kx1 - kx0) * (float) i / (float) (nk - 1), cy = plate.getCentreY() + 2.0f, rad = 8.5f;
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillEllipse (cx - rad, cy - rad + 2.0f, rad * 2.0f, rad * 2.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (th.capA), cx - rad * 0.4f, cy - rad * 0.6f, juce::Colour (th.capB), cx + rad * 0.6f, cy + rad, true));
            g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
            const float ang = -2.3f + 4.6f * rng.nextFloat() * 0.8f + 0.4f;
            g.setColour (juce::Colour (th.plateText).withAlpha (0.9f));
            g.drawLine (cx, cy, cx + rad * 0.9f * std::sin (ang), cy - rad * 0.9f * std::cos (ang), 1.6f);
            for (int t = 0; t < 7; ++t)         // ticks round the knob
            {
                const float ta = -2.3f + 4.6f * (float) t / 6.0f;
                g.setColour (juce::Colour (th.plateText).withAlpha (0.35f));
                g.drawLine (cx + (rad + 3.0f) * std::sin (ta), cy - (rad + 3.0f) * std::cos (ta), cx + (rad + 5.0f) * std::sin (ta), cy - (rad + 5.0f) * std::cos (ta), 0.9f);
            }
        }
        // the power lamp
        const float lx = plate.getRight() - 9.0f, ly = plate.getY() + 7.0f;
        g.setGradientFill (juce::ColourGradient (glow.withAlpha (0.55f), lx, ly, glow.withAlpha (0.0f), lx + 9.0f, ly, true));
        g.fillEllipse (lx - 9.0f, ly - 9.0f, 18.0f, 18.0f);
        g.setColour (glow.brighter (0.4f));
        g.fillEllipse (lx - 2.6f, ly - 2.6f, 5.2f, 5.2f);
    }
    // little feet
    for (float fx : { head.getX() + 20.0f, head.getRight() - 32.0f })
    {
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.fillRoundedRectangle (fx, head.getBottom() - 1.0f, 12.0f, 5.0f, 2.0f);
    }
    // faint footer line: the cabinet's cloth shows a glimpse of light from the head
    g.setGradientFill (juce::ColourGradient (glow.withAlpha (0.12f), W * 0.5f, head.getBottom() + 4.0f, glow.withAlpha (0.0f), W * 0.5f, head.getBottom() + 46.0f, false));
    g.fillRoundedRectangle (cab.getX() + 18.0f, head.getBottom() + 4.0f, cab.getWidth() - 36.0f, 42.0f, 8.0f);
}
//==============================================================================
// Backdrop (window background and header)
//==============================================================================
void Backdrop::paint (juce::Graphics& g)
{
    // The background is drawn once into an image (it never changes), then reused.
    if (! cache.isValid() || cache.getWidth() != getWidth() || cache.getHeight() != getHeight())
    {
        cache = juce::Image (juce::Image::RGB, std::max (1, getWidth()), std::max (1, getHeight()), true);
        juce::Graphics cg (cache);
        paintBackground (cg);
    }
    g.drawImageAt (cache, 0, 0);
}

void Backdrop::paintBackground (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth(), h = bounds.getHeight();
    const auto accent = ui::accent;

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101015), 0.0f, 0.0f, juce::Colour (0xff050507), 0.0f, h, false));
    g.fillAll();

    // Light from above, tinted by the amp: two soft beams and a glow in each corner
    {
        auto beam = [&] (float x0, float x1, float spread, float alpha)
        {
            juce::Path p;
            p.startNewSubPath (x0, 0.0f);
            p.lineTo (x1, 0.0f);
            p.lineTo (x1 + spread, h);
            p.lineTo (x0 - spread, h);
            p.closeSubPath();
            g.setGradientFill (juce::ColourGradient (accent.withAlpha (alpha), 0.0f, 0.0f, accent.withAlpha (0.0f), 0.0f, h * 0.9f, false));
            g.fillPath (p);
        };
        beam (w * 0.16f, w * 0.20f, 260.0f, 0.075f);
        beam (w * 0.62f, w * 0.64f, 180.0f, 0.045f);

        auto glow = [&] (juce::Colour col, float cx, float cy, float radius, float alpha)
        {
            g.setGradientFill (juce::ColourGradient (col.withAlpha (alpha), cx, cy, col.withAlpha (0.0f), cx + radius, cy, true));
            g.fillRect (bounds);
        };
        glow (accent, w * 0.08f, -80.0f, 820.0f, 0.12f);
        glow (accent, w * 0.97f, h + 60.0f, 900.0f, 0.10f);
    }

    // A speaker cone rising out of the bottom-right corner: concentric rings, very faint
    {
        const float cx = w * 0.90f, cy = h * 0.86f;
        for (int i = 0; i < 16; ++i)
        {
            const float rad = 70.0f + (float) i * 44.0f;
            g.setColour (accent.withAlpha (juce::jmax (0.006f, 0.05f - 0.003f * (float) i)));
            g.drawEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f, i % 4 == 0 ? 1.6f : 0.8f);
        }
        g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.07f), cx, cy, accent.withAlpha (0.0f), cx + 70.0f, cy, true));
        g.fillEllipse (cx - 70.0f, cy - 70.0f, 140.0f, 140.0f);
    }

    // Vignette
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, w * 0.5f, h * 0.5f,
                                             juce::Colours::black.withAlpha (0.55f), 0.0f, 0.0f, true));
    g.fillRect (bounds);

    // Fine grain
    juce::Random rng (7);
    for (int i = 0; i < 1400; ++i)
    {
        g.setColour (juce::Colours::white.withAlpha (0.010f + 0.025f * rng.nextFloat()));
        g.fillRect (rng.nextInt (juce::jmax (1, getWidth())), rng.nextInt (juce::jmax (1, getHeight())), 1, 1);
    }

    // Header: a small level-meter mark, the wordmark, a version tag
    {
        const float bx = 16.0f;
        const float heights[3] = { 11.0f, 22.0f, 15.0f };
        for (int i = 0; i < 3; ++i)
        {
            g.setColour (i == 1 ? accent : accent.withAlpha (0.55f));
            g.fillRoundedRectangle (bx + (float) i * 7.0f, 33.0f - heights[i], 4.0f, heights[i], 2.0f);
        }
        g.setColour (ui::text);
        drawSpaced (g, "STARTER AMP", 42.0f, 8.0f, 28.0f, 2.2f, 190.0f, juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    }

    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (186.0f, 14.0f, 34.0f, 17.0f, 8.5f, 1.0f);
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("v93", juce::Rectangle<int> (186, 14, 34, 17), juce::Justification::centred);

    g.setColour (ui::textDim);

    // The line the pages sit under
    g.setGradientFill (juce::ColourGradient (accent.withAlpha (0.55f), 12.0f, 0.0f, accent.withAlpha (0.0f), w - 12.0f, 0.0f, false));
    g.fillRect (12.0f, 114.0f, w - 24.0f, 1.0f);
}

//==============================================================================
// ChainStrip
//==============================================================================
ChainStrip::ChainStrip (juce::AudioProcessorValueTreeState& state, std::function<void (int)> open)
    : apvts (state), openPage (std::move (open))
{
    // In signal order. The page numbers are the tabs (see the list below).
    // (pages: 0 INPUT, 1 GAIN, 2 MOD, 3 PITCH, 4 AMP, 5 CAB, 6 EQ, 7 DELAY, 8 SCAPES, 9 POST FX)
    items = { { "Transp.", "tpOn", 0 },  { "Octave", "octOn", 3 },     { "Harm.", "hmOn", 3 },       { "Pitch", "wowOn", 3 },      { "Noise", "noiseOn", 8 },
              { "Fuzz", "fuzzOn", 1, 0, 0 },     { "Comp", "compOn", 1, 0, 1 },      { "Wah", "wahOn", 2, 0, 2 },        { "Phaser", "phaserOn", 2, 0, 3 },
              { "Chorus", "chorusOn", 2, 0, 4 }, { "Pre-Dly", "preDelayOn", 7, 0, 5 }, { "Mod FX", "mfAOn", 2, -1, -1, "mfBOn" }, { "Lo-Fi", "lfOn", 8 },
              { "Boost", "bsOn", 1 }, { "Drive", "odOn", 1 }, { "Drive 2", "od2On", 1 },       { "Amp", {}, 4 },
              { "Cab", "cabOn", 5 },       { "EQ", "eqOn", 6, 1, 0 },          { "Delay", "postDelayOn", 7, 1, 3 }, { "Reverb", "reverbOn", 9, 1, 4 } };
    lastState.assign (items.size(), 0);
    rebuildShown();
    setTooltip ("Signal flow, from your guitar (IN) to the output (OUT). Click a stage's light to switch it on or off; click its name to open its page. The stage whose page is open is outlined. The Amp stage shows the current amp model. To change the order of the effects, drag a stage left or right (the fuzz, compressor, wah, phaser, chorus, pre-amp delay, and after the cab the EQ, delay and reverb can be moved; the ORDER button has the rest).");
    startTimerHz (10);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

// The effects before the amp and after the cab can be reordered (see the ORDER window). The strip draws
// each stage where it now sits: the visible stages of a group fill that group's positions in order.
void ChainStrip::rebuildShown()
{
    shown.assign (items.size(), 0);
    for (size_t i = 0; i < items.size(); ++i) shown[i] = (int) i;
    if (processor == nullptr) return;
    for (int group = 0; group < 2; ++group)
    {
        std::vector<int> positions, order;
        for (size_t i = 0; i < items.size(); ++i)
            if (items[i].group == group) positions.push_back ((int) i);
        for (int block : processor->getFxOrder (group))
            for (size_t i = 0; i < items.size(); ++i)
                if (items[i].group == group && items[i].block == block) order.push_back ((int) i);
        for (size_t k = 0; k < positions.size() && k < order.size(); ++k)
            shown[(size_t) positions[k]] = order[k];
    }
}

bool ChainStrip::isOn (int index) const
{
    const auto& id = at (index).onId;
    if (id.isEmpty())
        return true;
    bool on = false;
    if (auto* v = apvts.getRawParameterValue (id))
        on = v->load() > 0.5f;
    if (! on && at (index).onId2.isNotEmpty())
        if (auto* v = apvts.getRawParameterValue (at (index).onId2))
            on = v->load() > 0.5f;
    return on;
}

void ChainStrip::timerCallback()
{
    bool changed = false;
    const auto before = shown;
    rebuildShown();
    if (shown != before) changed = true;
    for (size_t i = 0; i < items.size(); ++i)
    {
        const char now = isOn ((int) i) ? 1 : 0;
        if (now != lastState[i]) { lastState[i] = now; changed = true; }
    }
    if (changed)
        repaint();
}

juce::Rectangle<float> ChainStrip::itemBounds (int index) const
{
    const float left = 36.0f, right = 46.0f, gap = 9.0f;
    const int n = (int) items.size();
    const float itemW = ((float) getWidth() - left - right - gap * (float) (n - 1)) / (float) n;
    return { left + (float) index * (itemW + gap), 2.0f, itemW, (float) getHeight() - 4.0f };
}

void ChainStrip::mouseMove (const juce::MouseEvent& e)
{
    int found = -1;
    for (int i = 0; i < (int) items.size(); ++i)
        if (itemBounds (i).contains (e.position))
            found = i;
    if (found != hover) { hover = found; repaint(); }
}

void ChainStrip::mouseDown (const juce::MouseEvent& e)
{
    pressed = -1;
    dragging = false;
    downPos = e.position;
    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto b = itemBounds (i);
        if (! b.contains (e.position))
            continue;
        const auto& item = at (i);
        if (item.onId.isNotEmpty() && e.position.x < b.getX() + 24.0f)
        {
            const bool wasOn = isOn (i);
            for (const auto& pid : { item.onId, item.onId2 })
                if (pid.isNotEmpty())
                    if (auto* p = apvts.getParameter (pid))
                    {
                        if (pid == item.onId2 && ! wasOn)
                            continue;                       // switching on: only the first
                        p->beginChangeGesture();
                        p->setValueNotifyingHost (wasOn ? 0.0f : 1.0f);
                        p->endChangeGesture();
                    }
            timerCallback();
        }
        else
            pressed = i;                                    // opens the page when the mouse is released, unless it was dragged
        return;
    }
}

// Dragging a stage moves it past its neighbours (only among the effects of its own group, before the amp or after the cab).
void ChainStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (pressed < 0 || processor == nullptr)
        return;
    if (! dragging)
    {
        if (e.position.getDistanceFrom (downPos) < 6.0f || at (pressed).group < 0)
            return;
        dragging = true;
        setMouseCursor (juce::MouseCursor::DraggingHandCursor);
        repaint();
    }
    int over = -1;
    for (int i = 0; i < (int) items.size(); ++i)
    {
        const auto b = itemBounds (i);
        if (e.position.x >= b.getX() - 4.0f && e.position.x <= b.getRight() + 5.0f)
            over = i;
    }
    if (over < 0 || over == pressed || at (over).group != at (pressed).group)
        return;
    const int group = at (pressed).group, movedBlock = at (pressed).block, targetBlock = at (over).block;
    const auto order = processor->getFxOrder (group);
    int from = -1, to = -1;
    for (int k = 0; k < (int) order.size(); ++k)
    {
        if (order[(size_t) k] == movedBlock)  from = k;
        if (order[(size_t) k] == targetBlock) to = k;
    }
    if (from < 0 || to < 0)
        return;
    processor->moveFxBlock (group, from, to);
    rebuildShown();
    for (int i = 0; i < (int) items.size(); ++i)            // follow the stage to its new place
        if (at (i).group == group && at (i).block == movedBlock)
            pressed = i;
    repaint();
}

void ChainStrip::mouseUp (const juce::MouseEvent&)
{
    if (! dragging && pressed >= 0)
        openPage (at (pressed).page);
    dragging = false;
    pressed = -1;
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    repaint();
}

void ChainStrip::paint (juce::Graphics& g)
{
    const int n = (int) items.size();
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.setColour (ui::textDim.withAlpha (0.8f));
    g.drawText ("IN", juce::Rectangle<float> (0.0f, 0.0f, 30.0f, (float) getHeight()), juce::Justification::centredRight, false);
    g.drawText ("OUT", juce::Rectangle<float> ((float) getWidth() - 42.0f, 0.0f, 38.0f, (float) getHeight()), juce::Justification::centredLeft, false);

    for (int i = 0; i < n; ++i)
    {
        const auto b = itemBounds (i);
        const auto& item = at (i);
        const bool amp = item.onId.isEmpty();
        const bool on = isOn (i);
        const bool current = item.page == currentPage;

        const bool beingDragged = dragging && pressed == i;
        g.setColour (beingDragged ? ui::accent.withAlpha (0.28f) : juce::Colours::white.withAlpha (current ? 0.075f : (hover == i ? 0.065f : 0.035f)));
        g.fillRoundedRectangle (b, 7.0f);
        if (current)
        {
            g.setColour (ui::accent.withAlpha (0.45f));
            g.drawRoundedRectangle (b.reduced (0.5f), 7.0f, 1.0f);
        }

        // Light: lit and haloed when the stage is on
        const float lx = b.getX() + 12.0f, ly = b.getCentreY();
        if (on)
        {
            g.setGradientFill (juce::ColourGradient (ui::accent.withAlpha (0.45f), lx, ly, ui::accent.withAlpha (0.0f), lx + 9.0f, ly, true));
            g.fillEllipse (lx - 9.0f, ly - 9.0f, 18.0f, 18.0f);
            g.setColour (ui::accent.brighter (0.3f));
            g.fillEllipse (lx - 3.0f, ly - 3.0f, 6.0f, 6.0f);
        }
        else
        {
            g.setColour (ui::textDim.withAlpha (0.55f));
            g.drawEllipse (lx - 3.0f, ly - 3.0f, 6.0f, 6.0f, 1.2f);
        }

        juce::String label = item.label;
        if (amp && modelName.isNotEmpty())
            label = modelName;
        g.setColour (on ? ui::text : ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (11.0f, on ? juce::Font::bold : juce::Font::plain)));
        g.drawFittedText (label, b.toNearestInt().withTrimmedLeft (22).withTrimmedRight (1), juce::Justification::centredLeft, 1);

        if (i + 1 < n)      // an arrow to the next stage
        {
            const float ax = b.getRight() + 6.5f, ay = b.getCentreY();
            juce::Path chevron;
            chevron.startNewSubPath (ax - 2.0f, ay - 3.5f);
            chevron.lineTo (ax + 2.0f, ay);
            chevron.lineTo (ax - 2.0f, ay + 3.5f);
            g.setColour (ui::textDim.withAlpha (0.45f));
            g.strokePath (chevron, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }
}

//==============================================================================
// Editor
//==============================================================================
StarterAmpEditor::StarterAmpEditor (StarterAmpProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setLookAndFeel (&lookAndFeel);

    addAndMakeVisible (content);
    content.setBounds (0, 0, ui::kWidth, ui::kHeight);
    // Five pages, one per section of the signal path. The tabs at the top switch between them.
    for (auto* page : { &pageInput, &pageGain, &pageMod, &pageDelay, &pagePitch, &pageAmp, &pageCab, &pageSpeaker, &pageEq, &pageScapes, &pagePost })
        content.addAndMakeVisible (*page);
    // v73 pages: INPUT, GAIN, MOD, PITCH, AMP, CAB (blocks + speaker), EQ, DELAY, SCAPES, POST FX
    for (auto* panel : { &inputPanel, &transposePanel })
        pageInput.addAndMakeVisible (*panel);
    for (auto* panel : { &fuzzPanel, &compPanel, &overdrivePanel, &drive2Panel, &boostPanel, &boostEqPanel, &fuzz2Panel })
        pageGain.addAndMakeVisible (*panel);
    for (auto* panel : { &wahPanel, &phaserPanel, &chorusPanel, &mfPanel0, &mfPanel1 })
        pageMod.addAndMakeVisible (*panel);
    for (auto* panel : { &preDelayPanel, &preDelayFxPanel, &postDelayPanel, &postDelayFxPanel })
        pageDelay.addAndMakeVisible (*panel);
    for (auto* panel : { &wowPanel, &octPanel, &harmPanel })
        pagePitch.addAndMakeVisible (*panel);
    for (auto* panel : { &shapePanel, &dynPanel, &exportPanel })
        pageSpeaker.addAndMakeVisible (*panel);
    pageAmp.addAndMakeVisible (ampPanel);
    pageAmp.addAndMakeVisible (acousticPanel);
    pageAmp.addAndMakeVisible (metalPanel);
    pageAmp.addAndMakeVisible (ampHero);
    pageAmp.addAndMakeVisible (ampArtPanel);
    ampArtPanel.setStyle (PanelKind::Cab);
    ampArtPanel.setBigNameplate (true);
    ampArtPanel.setInterceptsMouseClicks (false, false);
    pageAmp.addAndMakeVisible (ampBPanel);
    for (auto& ns : namStrip)
        pageAmp.addAndMakeVisible (ns.panel);
    irPanels[0] = &irAPanel;  irPanels[1] = &irBPanel;  irPanels[2] = &irCPanel;  irPanels[3] = &irDPanel;
    for (auto* panel : { &irAPanel, &irBPanel, &irCPanel, &irDPanel, &cabPanel })
        pageCab.addAndMakeVisible (*panel);
    pageCab.addAndMakeVisible (irView);
    pageCab.addAndMakeVisible (micView);
    pageEq.addAndMakeVisible (eqPanel);
    pageEq.addAndMakeVisible (spectrumView);
    for (auto* panel : { &grainPanel, &tremPanel, &wobblePanel, &swellPanel, &lofiPanel, &noisePanel })
        pageScapes.addAndMakeVisible (*panel);
    for (auto* panel : { &reverbPanel, &reverbShapePanel, &outputPanel, &doublerPanel, &widenerPanel })
        pagePost.addAndMakeVisible (*panel);

    // ---- INPUT & GATE
    inputPanel.addRow ({ &makeCombo ("inputMode") });
    inputPanel.addRow ({ &makeToggle ("deNoiseOn", "PICKUP DE-NOISE"), &makeCombo ("deNoiseHz") });
    inputPanel.setKnobColumns (2);
    addKnob (inputPanel, "inGain",      "Input",  " dB", 1);
    addKnob (inputPanel, "gate",        "Gate",   " dB", 1);
    addKnob (inputPanel, "deNoiseHum",  "Hum");
    addKnob (inputPanel, "deNoiseHiss", "Hiss");

    // ---- NOISE PEDAL. Four footswitches; each is momentary (on only while held) or,
    //      with its LATCH button lit, latching (click on, click off). Below them are
    //      tabs that choose which engine's settings are shown. Rise and Speed are
    //      shared by all of them, so they are on every page.
    setupSwitch (noiseOctButton,   "noiseOct",   "noiseOctLatch",   noiseOctAttachment);
    setupSwitch (noiseOct2Button,  "noiseOct2",  "noiseOct2Latch",  noiseOct2Attachment);
    setupSwitch (noiseGrossButton, "noiseGross", "noiseGrossLatch", noiseGrossAttachment);
    setupSwitch (noiseRingButton,  "noiseRing",  "noiseRingLatch",  noiseRingAttachment);

    auto& octLatch   = makeToggle ("noiseOctLatch",   "LATCH");
    auto& oct2Latch  = makeToggle ("noiseOct2Latch",  "LATCH");
    auto& grossLatch = makeToggle ("noiseGrossLatch", "LATCH");
    auto& ringLatch  = makeToggle ("noiseRingLatch",  "LATCH");

    // If a switch is changed from latching to momentary while it is on, let it go.
    auto releaseOnUnlatch = [this] (juce::ToggleButton& latch, juce::TextButton& sw, const char* paramId)
    {
        latch.onClick = [this, &latch, &sw, paramId]
        {
            if (! latch.getToggleState() && ! sw.isDown())
                setSwitch (paramId, false);
        };
    };
    releaseOnUnlatch (octLatch,   noiseOctButton,   "noiseOct");
    releaseOnUnlatch (oct2Latch,  noiseOct2Button,  "noiseOct2");
    releaseOnUnlatch (grossLatch, noiseGrossButton, "noiseGross");
    releaseOnUnlatch (ringLatch,  noiseRingButton,  "noiseRing");

    // Tabs: radio buttons that only change which page of the panel is shown.
    int tabIndex = 0;
    for (auto* tab : { &tabOctave, &tabGross, &tabRing })
    {
        const int index = tabIndex++;
        tab->setComponentID ("tab");
        tab->setClickingTogglesState (true);
        tab->setRadioGroupId (4711);
        tab->onClick = [this, index] { noisePanel.setPage (index); };
    }
    tabOctave.setToggleState (true, juce::dontSendNotification);

    noisePanel.addRow ({ &makeToggle ("noiseOn", "NOISE PEDAL ON") }, {}, 22);
    noisePanel.addRow ({ &noiseOctButton, &noiseOct2Button, &noiseGrossButton, &noiseRingButton });
    noisePanel.addRow ({ &octLatch, &oct2Latch, &grossLatch, &ringLatch }, {}, 18);
    noisePanel.addRow ({ &tabOctave, &tabGross, &tabRing }, {}, 24);
    noisePanel.addRow ({ &makeCombo ("noiseGrossScene") }, {}, 26, 1);                          // Clash page
    noisePanel.addRow ({ &makeCombo ("ringMode"), &makeCombo ("ringWave") }, {}, 26, 2);        // Ring mod page

    addKnob (noisePanel, "noiseRise",   "Rise");                       // every page
    addKnob (noisePanel, "noiseSpeed",  "Speed");
    addKnob (noisePanel, "noisePanic",  "Panic",  {}, 2, 0);           // Octave page
    addKnob (noisePanel, "noiseChaos",  "Chaos",  {}, 2, 0);
    addKnob (noisePanel, "noiseNuke",   "Drive",   {}, 2, 1);           // Clash page
    addKnob (noisePanel, "noiseSicken", "Blend", {}, 2, 1);
    addKnob (noisePanel, "ringFreq",    "Freq",   " Hz", 1, 2);        // Ring mod page
    addKnob (noisePanel, "ringMix",     "Mix",    {}, 2, 2);
    addKnob (noisePanel, "ringWobble",  "Wobble", {}, 2, 2);
    noisePanel.setPage (0);

    // ---- FUZZ (and, v93, FUZZ 2: a full copy)
    for (int w = 0; w < 2; ++w)
    {
    Panel& fp = w == 0 ? fuzzPanel : fuzz2Panel;
    const std::string pre = w == 0 ? "fuzz" : "fuzz2";
    auto ID = [&pre] (const char* n) { return pre + n; };
    // One page (v57). Always visible: on/style/auto-level, the style's own switches (germanium, op-amp, version,
    // model) and its main knobs. Behind ADV: the extras (diodes, tone bypass, buffer, octave up, guitar volume,
    // comp, drift, hiss).
    {
        auto& fzOn     = makeToggle (ID ("On").c_str(), w == 0 ? "FUZZ ON" : "FUZZ 2 ON");
        auto& fzStyle  = makeCombo (ID ("Style").c_str());
        fzStyle.setItemEnabled (3, false);   // v57: "Gated (velcro)" overlapped Factory; it stays (greyed out) so old sessions sound the same
        auto& fzAuto   = makeToggle (ID ("Auto").c_str(), "AUTO LEVEL");
        fp.addRow ({ &fzOn, &fzStyle, &fzAuto }, { 0.8f, 1.5f, 1.0f });
        auto& geToggle = makeToggle (ID ("Ge").c_str(), "GERMANIUM");
        auto& opAmp    = makeToggle (ID ("MuffOpAmp").c_str(), "OP-AMP");
        auto& version  = makeCombo (ID ("MuffVer").c_str());
        auto& model    = makeCombo (ID ("BenderModel").c_str());
        fzc[w].ge = &geToggle;  fzc[w].opAmp = &opAmp;  fzc[w].version = &version;  fzc[w].model = &model;
        fp.addRow ({ &geToggle, &opAmp, &version, &model }, { 1.0f, 0.8f, 1.5f, 1.5f }, 26);
        auto& diodes   = makeCombo (ID ("MuffDiode").c_str());
        auto& tonePass = makeToggle (ID ("MuffTonePass").c_str(), "TONE BYPASS");
        auto& buffer   = makeToggle (ID ("Buffer").c_str(), "BUFFER");
        auto& octave   = makeToggle (ID ("Octave").c_str(), "OCTAVE UP");
        fzc[w].diode = &diodes;  fzc[w].tonePass = &tonePass;  fzc[w].buffer = &buffer;
        fp.addRow ({ &diodes, &tonePass, &buffer, &octave }, { 1.5f, 1.2f, 1.0f, 1.0f }, 26);
        auto& bzSoft = makeToggle ((w == 0 ? "buzzSoft" : "fuzz2BuzzSoft"), "SOFT CLIP");
        auto& bzHard = makeToggle ((w == 0 ? "buzzHard" : "fuzz2BuzzHard"), "HARD CLIP");
        auto& bzPol  = makeToggle ((w == 0 ? "buzzPol" : "fuzz2BuzzPol"), "INVERT BLEND");
        fzc[w].buzzSoft = &bzSoft;  fzc[w].buzzHard = &bzHard;  fzc[w].buzzPol = &bzPol;
        fp.addRow ({ &bzSoft, &bzHard, &bzPol }, { 1.0f, 1.0f, 1.2f }, 26);
        for (auto* c : { (juce::Component*) &diodes, (juce::Component*) &tonePass, (juce::Component*) &buffer, (juce::Component*) &octave })
            fp.markAdvanced (c);
    }
    fp.setKnobColumns (5);
    addKnob (fp, ID ("Gain").c_str(),  "Gain",  {}, 2);
    addKnob (fp, ID ("Tone").c_str(),  "Tone",  {}, 2);
    fzc[w].tone = knobs.back().get();
    addKnob (fp, ID ("Level").c_str(), "Level", " dB", 1);
    addKnob (fp, ID ("Mix").c_str(),   "Mix",   {}, 2);
    addKnob (fp, ID ("Gate").c_str(),  "Gate",  {}, 2);
    fzc[w].gate = knobs.back().get();
    addKnob (fp, ID ("Stab").c_str(),  "Stab",  {}, 2);
    fzc[w].stab = knobs.back().get();
    addKnob (fp, ID ("Starve").c_str(), "Starve", {}, 2);
    fzc[w].starve = knobs.back().get();
    addKnob (fp, ID ("MuffMids").c_str(), "Mids", {}, 2);
    fzc[w].mids = knobs.back().get();
    addKnob (fp, (w == 0 ? "buzzLow" : "fuzz2BuzzLow"),   "Low",   {}, 2);
    fzc[w].buzzLow = knobs.back().get();
    addKnob (fp, (w == 0 ? "buzzMid" : "fuzz2BuzzMid"),    "Mid",   {}, 2);
    fzc[w].buzzMid = knobs.back().get();
    addKnob (fp, (w == 0 ? "buzzHigh" : "fuzz2BuzzHigh"),  "High",  {}, 2);
    fzc[w].buzzHigh = knobs.back().get();
    addKnob (fp, (w == 0 ? "buzzBlend" : "fuzz2BuzzBlend"), "Blend", {}, 2);
    fzc[w].buzzBlend = knobs.back().get();
    addKnob (fp, ID ("Shape").c_str(), "Shape", {}, 2);                 // v66: Nuke
    fzc[w].shape = knobs.back().get();
    addKnob (fp, ID ("Crush").c_str(), "Crush", {}, 2);
    fzc[w].crush = knobs.back().get();
    addKnob (fp, ID ("Bits").c_str(),  "Bits",  {}, 0);                 // v66: Crusher
    fzc[w].bits = knobs.back().get();
    addKnob (fp, ID ("Rate").c_str(),  "Rate",  {}, 2);
    fzc[w].rate = knobs.back().get();
    addAdvKnob (fp, ID ("Comp").c_str(),  "Comp",  {}, 2);
    fzc[w].comp = knobs.back().get();
    addAdvKnob (fp, ID ("Input").c_str(), "Guitar Vol", {}, 2);
    fzc[w].input = knobs.back().get();
    addAdvKnob (fp, ID ("Drift").c_str(), "Drift", {}, 2);
    fzc[w].drift = knobs.back().get();
    addAdvKnob (fp, ID ("Hiss").c_str(),  "Hiss",  {}, 2);
    fzc[w].hiss = knobs.back().get();
    setupAdvanced (fp, pre.c_str());
    if (w == 1)
        fp.addRow ({ &makeCombo ("fuzz2Route") }, {}, 26);
    }

    // ---- COMPRESSOR
    compPanel.addRow ({ &makeToggle ("compOn", "COMP ON"), &makeCombo ("compStyle"), &makeCombo ("compRatio"), &grMeter, &makeToggle ("compAuto", "AUTO LEVEL") }, { 0.9f, 1.25f, 0.75f, 1.0f, 1.0f });
    {
        auto& sat = makeCombo ("compSat");
        auto& det = makeCombo ("compDetHp");
        compPanel.addRow ({ &sat, &det }, { 1.3f, 0.9f });
        compPanel.markAdvanced (&sat);
        compPanel.markAdvanced (&det);
    }
    compPanel.setKnobColumns (5);
    addKnob (compPanel, "compThresh",  "Thresh",  " dB", 1);
    addKnob (compPanel, "compAttack",  "Attack",  " ms", 2);
    addKnob (compPanel, "compRelease", "Release", " ms", 0);
    addKnob (compPanel, "compOutput",  "Output",  " dB", 1);
    addKnob (compPanel, "compMix",     "Mix");
    addAdvKnob (compPanel, "compBloom",   "Bloom");
    addAdvKnob (compPanel, "compSmack",   "Smack");
    addAdvKnob (compPanel, "compClarity", "Clarity");
    setupAdvanced (compPanel, "comp");

    // ---- PHASER and CHORUS
    // ---- Doubler and widener (POST FX page, second row)
    doublerPanel.addRow ({ &makeToggle ("dblOn", "DOUBLER ON") });
    doublerPanel.setKnobColumns (4);
    addKnob (doublerPanel, "dblTime",   "Time",      " ms", 1);
    addKnob (doublerPanel, "dblVar",    "Variation");
    addKnob (doublerPanel, "dblSpread", "Spread");
    addKnob (doublerPanel, "dblMix",    "Mix");

    widenerPanel.addRow ({ &makeToggle ("widOn", "WIDENER ON"), &makeCombo ("widMode") }, { 1.0f, 1.3f });
    widenerPanel.setKnobColumns (4);
    addKnob (widenerPanel, "widWow",    "Wide");
    addKnob (widenerPanel, "widSpace",  "Space");
    addAdvKnob (widenerPanel, "widBass",   "Bass Lift");
    addAdvKnob (widenerPanel, "widBassHz", "Bass Freq", " Hz", 0);
    addAdvKnob (widenerPanel, "widMono",   "Mono Bass", " Hz", 0);
    addKnob (widenerPanel, "widMix",    "Mix");
    addAdvKnob (widenerPanel, "widLevel",  "Level",     " dB", 1);
    setupAdvanced (widenerPanel, "widener");

    // A note-value menu: choosing a note locks that knob to the tempo and greys the knob out.
    auto makeSyncCombo = [this] (const char* id) -> juce::ComboBox&
    {
        auto& c = makeCombo (id);
        c.onChange = [this] { syncDelayKnobs(); };
        return c;
    };

    // ---- SCAPES: granular cloud, tremolo, wobble, swell
    grainPanel.addRow ({ &makeToggle ("granOn", "GRAINS ON"), &makeToggle ("granFreeze", "FREEZE"), &makeCombo ("granScale"), &makeCombo ("granShape") },
                       { 0.9f, 0.8f, 1.4f, 1.0f });
    {
        auto& n1 = makeSyncCombo ("granSizeNote");
        auto& n2 = makeSyncCombo ("granDensNote");
        auto& n3 = makeSyncCombo ("granPosNote");
        grainPanel.addRow ({ &n1, &n2, &n3 });
        grainPanel.markAdvanced (&n1);
        grainPanel.markAdvanced (&n2);
        grainPanel.markAdvanced (&n3);
    }
    grainPanel.setKnobColumns (6);
    addKnob (grainPanel, "granSize",      "Size",       " ms", 0);
    granSizeKnob = knobs.back().get();
    addKnob (grainPanel, "granDensity",   "Density",    " /s", 1);
    granDensKnob = knobs.back().get();
    addKnob (grainPanel, "granPos",       "Position",   " ms", 0);
    granPosKnob = knobs.back().get();
    addAdvKnob (grainPanel, "granSpray",     "Spray");
    addKnob (grainPanel, "granPitch",     "Pitch",      " st", 0);
    addKnob (grainPanel, "granScan",      "Scan");
    addAdvKnob (grainPanel, "granPitchRand", "Pitch Rand", " st", 1);
    addAdvKnob (grainPanel, "granReverse",   "Reverse");
    addAdvKnob (grainPanel, "granWidth",     "Width");
    addAdvKnob (grainPanel, "granFeedback",  "Feedback");
    addAdvKnob (grainPanel, "granTone",   "Tone",       " Hz", 0);
    addKnob (grainPanel, "granMix",       "Mix");
    setupAdvanced (grainPanel, "grain");

    auto& tremNote = makeCombo ("tremNote");
    tremNote.onChange = [this] { syncDelayKnobs(); };
    tremPanel.addRow ({ &makeToggle ("tremOn", "TREMOLO ON"), &makeCombo ("tremMode") }, { 0.9f, 1.4f });
    tremPanel.addRow ({ &makeCombo ("tremShape"), &tremNote }, { 1.0f, 1.0f });
    tremPanel.setKnobColumns (2);
    addKnob (tremPanel, "tremRate",  "Rate",  " Hz", 2);
    tremRateKnob = knobs.back().get();
    addKnob (tremPanel, "tremDepth", "Depth");

    wobblePanel.addRow ({ &makeToggle ("wobOn", "WOBBLE ON"), &makeSyncCombo ("wobNote") }, { 1.0f, 1.3f });
    wobblePanel.setKnobColumns (2);
    addKnob (wobblePanel, "wobRate",  "Rate",  " Hz", 2);
    wobRateKnob = knobs.back().get();
    addKnob (wobblePanel, "wobDepth", "Depth");
    addKnob (wobblePanel, "wobDrift", "Drift");
    addKnob (wobblePanel, "wobMix",   "Mix");

    swellPanel.addRow ({ &makeToggle ("swellOn", "SWELL ON"), &makeSyncCombo ("swellNote") }, { 1.0f, 1.3f });
    swellPanel.setKnobColumns (2);
    addKnob (swellPanel, "swellAttack", "Fade-In", " ms", 0);
    swellKnob = knobs.back().get();
    addKnob (swellPanel, "swellSens",   "Sensitivity");

    wahPanel.addRow ({ &makeToggle ("wahOn", "WAH ON"), &makeCombo ("wahMode"), &makeSyncCombo ("wahNote") }, { 0.8f, 1.4f, 1.2f });
    wahPanel.addRow ({ &makeCombo ("wahVoice") }, {});
    wahPanel.setKnobColumns (5);
    addKnob (wahPanel, "wahPos",   "Position");
    addAdvKnob (wahPanel, "wahHeel",  "Heel",       " Hz", 0);
    addAdvKnob (wahPanel, "wahToe",   "Toe",        " Hz", 0);
    addAdvKnob (wahPanel, "wahQ",     "Resonance",  "",    1);
    addKnob (wahPanel, "wahMix",   "Mix");
    addKnob (wahPanel, "wahSens",  "Sensitivity");
    addAdvKnob (wahPanel, "wahDecay", "Decay",      " ms", 0);
    addKnob (wahPanel, "wahRate",  "LFO Rate",   " Hz", 2);
    wahRateKnob = knobs.back().get();
    addAdvKnob (wahPanel, "wahDepth", "LFO Depth");
    setupAdvanced (wahPanel, "wah");

    phaserPanel.addRow ({ &makeToggle ("phaserOn", "PHASER ON") }, {});
    {
        auto& stages = makeCombo ("phaserStages");
        auto& shape  = makeCombo ("phaserShape");
        auto& note   = makeSyncCombo ("phaserNote");
        phaserPanel.addRow ({ &stages, &shape, &note }, { 1.5f, 0.9f, 1.2f });
        phaserPanel.markAdvanced (&stages);
        phaserPanel.markAdvanced (&shape);
        phaserPanel.markAdvanced (&note);
    }
    addKnob (phaserPanel, "phaserRate",     "Rate",     " Hz", 2);
    phaserRateKnob = knobs.back().get();
    addKnob (phaserPanel, "phaserDepth",    "Depth");
    addAdvKnob (phaserPanel, "phaserCenter",   "Centre",   " Hz", 0);
    addAdvKnob (phaserPanel, "phaserFeedback", "Feedback");
    addKnob (phaserPanel, "phaserMix",      "Mix");
    addAdvKnob (phaserPanel, "phaserEnv",      "Envelope");
    setupAdvanced (phaserPanel, "phaser");

    chorusPanel.addRow ({ &makeToggle ("chorusOn", "CHORUS ON"), &makeCombo ("chorusMode") });
    addKnob (chorusPanel, "chorusMix",   "Mix");
    addKnob (chorusPanel, "chorusRate",  "Rate",  " x", 2);
    addKnob (chorusPanel, "chorusDepth", "Depth");
    addKnob (chorusPanel, "chorusTone",  "Tone",  " Hz", 0);
    addKnob (chorusPanel, "chorusHiss",  "Hiss");

    // ---- PRE-AMP DELAY
    auto& preNote = makeCombo ("preDelayNote");
    preNote.onChange = [this] { syncDelayKnobs(); };
    preDelayPanel.addRow ({ &makeToggle ("preDelayOn", "DELAY ON"), &makeCombo ("preDelayMode"), &preNote }, { 1.0f, 1.0f, 1.3f });
    addKnob (preDelayPanel, "preDelayMix",      "Mix");
    addKnob (preDelayPanel, "preDelayTime",     "Time", " ms", 0);
    preTimeKnob = knobs.back().get();
    addKnob (preDelayPanel, "preDelayFeedback", "Feedback");
    addAdvKnob (preDelayPanel, "preDelayLofi",     "Lo-Fi");
    addAdvKnob (preDelayPanel, "preDelayWarble",   "Warble");
    setupAdvanced (preDelayPanel, "preDelay");

    // ---- DELAY FX (v52): ducking, modulation, diffusion, hold and multi-tap, one panel for each delay
    for (int d = 0; d < 2; ++d)
    {
        auto& panel = d == 0 ? preDelayFxPanel : postDelayFxPanel;
        const std::string p = d == 0 ? "preDelay" : "postDelay";
        panel.addRow ({ &makeToggle ((p + "Hold").c_str(), "HOLD"), &makeCombo ((p + "HoldMode").c_str()), &makeCombo ((p + "Taps").c_str()) }, { 0.8f, 1.1f, 1.4f });
        panel.setKnobColumns (6);
        addKnob (panel, (p + "Duck").c_str(),     "Duck");
        addKnob (panel, (p + "DuckRel").c_str(),  "Duck Release", " ms", 0);
        addKnob (panel, (p + "ModDepth").c_str(), "Mod Depth");
        addKnob (panel, (p + "ModRate").c_str(),  "Mod Rate", " Hz", 2);
        addKnob (panel, (p + "Diffuse").c_str(),  "Diffusion");
        addKnob (panel, (p + "TapLevel").c_str(), "Tap Level");
    }

    // ---- OVERDRIVE: three styles. Page 0 = Transparent, page 1 = Tight, page 2 = Open (with a range switch).
    //      The Volume knob is shared; the other knobs change with the style.
    for (int w = 0; w < 2; ++w)
    {
    Panel& overdrivePanel = w == 0 ? this->overdrivePanel : this->drive2Panel;
    const std::string pre = w == 0 ? "od" : "od2";
    auto ID = [&pre] (const char* n) { return pre + n; };
    auto& odStyleCombo = makeCombo (ID ("Style").c_str());
    odStyleCombo.onChange = [this] { syncOverdrivePage(); };
    overdrivePanel.addRow ({ &makeToggle (ID ("On").c_str(), w == 0 ? "OVERDRIVE ON" : "DRIVE 2 ON"), &odStyleCombo, &makeToggle (ID ("Auto").c_str(), "AUTO LEVEL") }, { 1.0f, 1.1f, 1.0f });
    overdrivePanel.setKnobColumns (5);
    overdrivePanel.addRow ({ &makeToggle (ID ("Range").c_str(), "HIGH RANGE") }, {}, 26, 2);                    // Open page
    overdrivePanel.addRow ({ &makeToggle ("odCircuit", "CIRCUIT MODE"), &makeToggle (ID ("Grip").c_str(), "GRIP") }, { 1.0f, 1.0f }, 26, 0);   // Transparent page (v94)
    // v94: Tight, Screamer and Distortion are hand-tuned again unless CIRCUIT MODE is on
    overdrivePanel.addRow ({ &makeToggle ("odCircuit", "CIRCUIT MODE") }, {}, 26, 1);
    overdrivePanel.addRow ({ &makeToggle ("odCircuit", "CIRCUIT MODE") }, {}, 26, 3);
    overdrivePanel.addRow ({ &makeToggle ("odCircuit", "CIRCUIT MODE") }, {}, 26, 4);
    // v58: Screamer (page 3) and Distortion (page 4). The clip and voicing menus are always visible; the extras are behind ADV.
    overdrivePanel.addRow ({ &makeCombo (ID ("Clip").c_str()), &makeCombo (ID ("Voice").c_str()) }, { 1.0f, 1.0f }, 26, 3);
    // v77: circuit mode for Tight (page 1) and Screamer (page 3); off by default, so older sessions sound the same
    overdrivePanel.addRow ({ &makeCombo (ID ("Clip").c_str()) }, {}, 26, 1);
    overdrivePanel.addRow ({ &makeCombo (ID ("Clip").c_str()) }, {}, 26, 6);                      // v92: Op-Amp Distortion (stock = Germanium)
    {
        auto& midMod = makeCombo (ID ("MidMod").c_str());
        overdrivePanel.addRow ({ &makeCombo (ID ("Clip").c_str()), &midMod }, { 1.0f, 1.0f }, 26, 4);
        overdrivePanel.markAdvanced (&midMod);
    }
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Gain",   {}, 2, 0);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Tone",   {}, 2, 0);
    addKnob (overdrivePanel, ID ("Edge").c_str(),   "Edge",   {}, 2, 0);
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Drive",  {}, 2, 1);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Bright", {}, 2, 1);
    addKnob (overdrivePanel, ID ("Gate").c_str(),   "Gate",   {}, 2, 1);
    addKnob (overdrivePanel, ID ("Attack").c_str(), "Attack", {}, 0, 1);
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Drive",  {}, 2, 2);
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Drive",  {}, 2, 3);
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Dist",   {}, 2, 4);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Tone",   {}, 2, 2);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Tone",   {}, 2, 3);
    addKnob (overdrivePanel, ID ("Mid").c_str(),    "Mid",    {}, 2, 3);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Tone",   {}, 2, 4);
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Drive",  {}, 2, 5);         // v64 Tube Driver
    addKnob (overdrivePanel, ID ("Bass").c_str(),   "Bass",   {}, 2, 5);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Treble", {}, 2, 5);
    // v92: five pedals from published analyses
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Dist",    {}, 2, 6);
    addKnob (overdrivePanel, ID ("Gain").c_str(),   "Gain",    {}, 2, 7);
    addKnob (overdrivePanel, ID ("Tone").c_str(),   "Tone",    {}, 2, 7);
    addKnob (overdrivePanel, ID ("Volume").c_str(), "Volume", " dB", 1);
    addKnob (overdrivePanel, ID ("Mix").c_str(),    "Mix",    {}, 2);
    addAdvKnob (overdrivePanel, ID ("Bass").c_str(),    "Bass",     {}, 2, 3);
    addAdvKnob (overdrivePanel, ID ("MidFreq").c_str(), "Mid Freq", {}, 2, 3);
    addAdvKnob (overdrivePanel, ID ("Edge").c_str(),    "Bright",   {}, 2, 3);
    addAdvKnob (overdrivePanel, ID ("Edge").c_str(),    "Bright",   {}, 2, 4);
    addAdvKnob (overdrivePanel, ID ("Edge").c_str(),    "Bright",   {}, 2, 5);
    setupAdvanced (overdrivePanel, pre.c_str());
    if (w == 1)
        overdrivePanel.addRow ({ &makeCombo ("od2Route") }, {}, 26);
    }
    syncOverdrivePage();

    // ---- AMP
    auto& ampModelBox = makeAmpModelCombo ("ampModel");
    voicingToggle = &makeCombo ("ampVoicing");
    ampQualityBox = &makeCombo ("ampQuality");
    // v59: streamlined. Always visible: model, voicing, auto level, then the switches that change the amp's feel most
    // (bright, tone stack, feel, power). Behind ADV: the tubes and rectifier, class, deep and the oversampling quality.
    ampPanel.addRow ({ &ampModelBox, voicingToggle, &makeToggle ("ampAutoLevel", "AUTO LEVEL") }, { 1.6f, 1.3f, 1.1f });
    {
        auto& bright = makeToggle ("ampBright", "BRIGHT");
        auto& stack  = makeCombo ("ampStack");
        auto& feel   = makeCombo ("ampFeel");
        auto& mode   = makeCombo ("ampMode");
        ampPanel.addRow ({ &bright, &stack, &feel, &mode }, { 0.8f, 1.4f, 1.5f, 1.8f });
        auto& tube   = makeCombo ("ampTube");
        auto& ptube  = makeCombo ("ampPwrTube");
        auto& deep   = makeToggle ("ampDeep", "DEEP");
        ampPanel.addRow ({ &tube, &ptube, &deep, ampQualityBox }, { 1.3f, 1.1f, 0.8f, 1.4f });
        for (juce::Component* comp : { (juce::Component*) &tube, (juce::Component*) &ptube, (juce::Component*) &deep, (juce::Component*) ampQualityBox })
            ampPanel.markAdvanced (comp);
        for (juce::Component* comp : { (juce::Component*) &bright, (juce::Component*) &stack, (juce::Component*) &feel, (juce::Component*) &mode,
                                       (juce::Component*) &tube, (juce::Component*) &ptube, (juce::Component*) &deep })
            ampRealismComps.push_back (comp);
    }
    addKnob (ampPanel, "ampGain",  "Gain");
    electricAmpKnobs.push_back (knobs.back().get());
    addKnob (ampPanel, "bass",     "Bass",     " dB", 1);       // the tone stack also works on the acoustic model
    addKnob (ampPanel, "lowMid",   "Low Mid",  " dB", 1);
    addKnob (ampPanel, "highMid",  "High Mid", " dB", 1);
    addKnob (ampPanel, "treble",   "Treble",   " dB", 1);
    addKnob (ampPanel, "presence", "Presence");
    electricAmpKnobs.push_back (knobs.back().get());
    addKnob (ampPanel, "ampMaster", "Master");
    masterKnob = knobs.back().get();
    electricAmpKnobs.push_back (knobs.back().get());
    addKnob (ampPanel, "ampTune",  "Focus",    "",    1);
    focusKnob = knobs.back().get();
    for (auto spec : { std::make_pair ("ampBias", "Bias"), std::make_pair ("ampPower", "Power Drive"),
                       std::make_pair ("ampReso", "Resonance"), std::make_pair ("ampSagAmt", "Sag") })
    {
        addAdvKnob (ampPanel, spec.first, spec.second);
        electricAmpKnobs.push_back (knobs.back().get());
    }
    setupAdvanced (ampPanel, "amp");

    // ---- AMP B (parallel amp, v40)
    voicingBoxB = &makeCombo ("ampBVoicing");
    ampBPanel.addRow ({ &makeAmpModelCombo ("ampBModel"), voicingBoxB, &makeToggle ("ampBOn", "AMP B ON"),
                        &makeToggle ("ampBAutoLevel", "AUTO LEVEL"), &makeToggle ("ampBUseOd", "FED BY OVERDRIVE") },
                      { 1.5f, 1.2f, 1.1f, 1.1f, 1.4f });
    {
        auto& bright = makeToggle ("ampBBright", "BRIGHT");
        auto& stack  = makeCombo ("ampBStack");
        auto& feel   = makeCombo ("ampBFeel");
        auto& mode   = makeCombo ("ampBMode");
        ampBPanel.addRow ({ &bright, &stack, &feel, &mode }, { 0.8f, 1.4f, 1.5f, 1.8f });
        auto& tube   = makeCombo ("ampBTube");
        auto& ptube  = makeCombo ("ampBPwrTube");
        auto& deep   = makeToggle ("ampBDeep", "DEEP");
        ampBPanel.addRow ({ &tube, &ptube, &deep }, { 1.3f, 1.1f, 0.8f });
        for (juce::Component* comp : { (juce::Component*) &tube, (juce::Component*) &ptube, (juce::Component*) &deep })
            ampBPanel.markAdvanced (comp);
    }
    addKnob (ampBPanel, "ampBGain",     "Gain");
    addKnob (ampBPanel, "ampBBass",     "Bass",     " dB", 1);
    addKnob (ampBPanel, "ampBLowMid",   "Low Mid",  " dB", 1);
    addKnob (ampBPanel, "ampBHighMid",  "High Mid", " dB", 1);
    addKnob (ampBPanel, "ampBTreble",   "Treble",   " dB", 1);
    addKnob (ampBPanel, "ampBPresence", "Presence");
    addKnob (ampBPanel, "ampBTight",    "Tight");
    addKnob (ampBPanel, "ampALevel",    "A Level",  " dB", 1);
    addKnob (ampBPanel, "ampBLevel",    "B Level",  " dB", 1);
    addAdvKnob (ampBPanel, "ampBBias",    "Bias");
    addAdvKnob (ampBPanel, "ampBPower",   "Power Drive");
    addAdvKnob (ampBPanel, "ampBReso",    "Resonance");
    addAdvKnob (ampBPanel, "ampBSagAmt",  "Sag");
    setupAdvanced (ampBPanel, "ampB");

    // ---- NAM model player (v50)
    {
        struct Def { const char* title; const char* on; const char* norm; const char* in; const char* out; const char* mix; };
        static const Def defs[4] = {
            { "NAM: AMP A  (in place of Amp A's preamp and power stage)", "namOn",  "namNorm",  "namIn",  "namOut",  nullptr },
            { "NAM: AMP B  (in place of Amp B's preamp and power stage)", "namBOn", "namBNorm", "namBIn", "namBOut", nullptr },
            { "NAM: PEDAL 1  (before the overdrive and the amp)",         "nmP1On", "nmP1Norm", "nmP1In", "nmP1Out", "nmP1Mix" },
            { "NAM: PEDAL 2  (after the cab)",                            "nmP2On", "nmP2Norm", "nmP2In", "nmP2Out", "nmP2Mix" } };
        static const char* const names[4] = { "Amp A", "Amp B", "pedal 1", "pedal 2" };
        for (int s = 0; s < 4; ++s)
        {
            auto& ns = namStrip[s];
            ns.panel.setTitle (defs[s].title);
            ns.info.setJustificationType (juce::Justification::centredLeft);
            ns.info.setMinimumHorizontalScale (1.0f);
            ns.info.setFont (juce::Font (juce::FontOptions (13.0f)));
            ns.info.setColour (juce::Label::textColourId, ui::textDim);
            ns.info.setText ("No model loaded. Press LOAD MODEL and pick a .nam file (WaveNet or LSTM captures).", juce::dontSendNotification);
            ns.info.setTooltip (juce::String ("The model loaded in the ") + names[s] + " slot. If a file will not load, the reason is shown here.");
            ns.load.setTooltip (juce::String ("Loads a neural capture (.nam file) into the ") + names[s] + " slot. WaveNet and LSTM captures are supported. The model file stays where it is: the session remembers its location.");
            ns.clear.setTooltip (juce::String ("Unloads the ") + names[s] + " model.");
            ns.load.onClick  = [this, s] { chooseNamFile (s); };
            ns.clear.onClick = [this, s] { processorRef.clearNam (s); namStrip[(size_t) s].lastText = {}; updateNamInfo (s); };
            ns.panel.addRow ({ &makeToggle (defs[s].on, "ON"), &ns.load, &ns.clear }, { 0.8f, 1.6f, 0.8f });
            if (s == 3)
                ns.panel.addRow ({ &makeToggle (defs[s].norm, "MATCH LOUDNESS"), &makeToggle ("nmP2Stereo", "STEREO: ONE MODEL PER SIDE") }, { 1.0f, 1.3f });
            else
                ns.panel.addRow ({ &makeToggle (defs[s].norm, "MATCH LOUDNESS") }, { 1.0f });
            ns.panel.addRow ({ &ns.info }, { 1.0f }, 40);
            addKnob (ns.panel, defs[s].in,  "Input",  " dB", 1);
            addKnob (ns.panel, defs[s].out, "Output", " dB", 1);
            if (s == 0)
            {
                ns.panel.setKnobColumns (6);
                addKnob (ns.panel, "bass",    "Bass",     " dB", 1);
                addKnob (ns.panel, "lowMid",  "Low Mid",  " dB", 1);
                addKnob (ns.panel, "highMid", "High Mid", " dB", 1);
                addKnob (ns.panel, "treble",  "Treble",   " dB", 1);
            }
            else if (s == 1)
            {
                ns.panel.setKnobColumns (6);
                addKnob (ns.panel, "ampBBass",    "Bass",     " dB", 1);
                addKnob (ns.panel, "ampBLowMid",  "Low Mid",  " dB", 1);
                addKnob (ns.panel, "ampBHighMid", "High Mid", " dB", 1);
                addKnob (ns.panel, "ampBTreble",  "Treble",   " dB", 1);
            }
            else
            {
                ns.panel.setKnobColumns (3);
                addKnob (ns.panel, defs[s].mix, "Mix");
            }
        }
    }

    // ---- LO-FI WARBLE (v50), in front of the overdrive
    lofiPanel.addRow ({ &makeToggle ("lfOn", "LO-FI ON"), &makeCombo ("lfShape") }, { 1.0f, 1.2f });
    addKnob (lofiPanel, "lfRate",     "Rate",     " Hz", 2);
    addKnob (lofiPanel, "lfDepth",    "Depth");
    addKnob (lofiPanel, "lfDrift",    "Drift");
    addKnob (lofiPanel, "lfTone",     "Tone");
    addKnob (lofiPanel, "lfDirt",     "Dirt");
    addKnob (lofiPanel, "lfNoise",    "Noise");
    addKnob (lofiPanel, "lfDropouts", "Dropouts");
    addKnob (lofiPanel, "lfMix",      "Mix");
    addKnob (lofiPanel, "lfLevel",    "Level", " dB", 1);

    // ---- BOOST page (v51): boost with a tighten filter and saturation modes, plus a 4-band parametric EQ
    boostPanel.addRow ({ &makeToggle ("bsOn", "BOOST ON"), &makeCombo ("bsMode") }, { 1.0f, 1.4f });
    boostPanel.addRow ({ &makeCombo ("bsTightStyle") }, {});
    boostPanel.setKnobColumns (4);
    addKnob (boostPanel, "bsGain",  "Gain",       " dB", 1);
    addKnob (boostPanel, "bsSat",   "Saturation");
    addKnob (boostPanel, "bsTight", "Tighten");
    addKnob (boostPanel, "bsLevel", "Level",      " dB", 1);
    addKnob (boostPanel, "bsMix",   "Mix",        {}, 2);

    boostEqPanel.addRow ({ &makeToggle ("bsEqOn", "EQ ON"), &makeCombo ("bsEqPos") }, { 1.0f, 1.6f });
    boostEqPanel.setKnobColumns (10);
    addKnob (boostEqPanel, "bsLoHz", "Low Freq",    " Hz", 0);
    addKnob (boostEqPanel, "bsLoDb", "Low Gain",    " dB", 1);
    addKnob (boostEqPanel, "bsM1Hz", "Mid 1 Freq",  " Hz", 0);
    addKnob (boostEqPanel, "bsM1Db", "Mid 1 Gain",  " dB", 1);
    addKnob (boostEqPanel, "bsM1Q",  "Mid 1 Q",     "",    2);
    addKnob (boostEqPanel, "bsM2Hz", "Mid 2 Freq",  " Hz", 0);
    addKnob (boostEqPanel, "bsM2Db", "Mid 2 Gain",  " dB", 1);
    addKnob (boostEqPanel, "bsM2Q",  "Mid 2 Q",     "",    2);
    addKnob (boostEqPanel, "bsHiHz", "High Freq",   " Hz", 0);
    addKnob (boostEqPanel, "bsHiDb", "High Gain",   " dB", 1);

    // ---- MOD FX page (v51): two slots of the same multi-type modulation effect
    {
        static const char* const ids[2][11] = {
            { "mfAOn", "mfAType", "mfAPos", "mfAShape", "mfANote", "mfARate", "mfADepth", "mfAFeedback", "mfAManual", "mfATone", "mfAMix" },
            { "mfBOn", "mfBType", "mfBPos", "mfBShape", "mfBNote", "mfBRate", "mfBDepth", "mfBFeedback", "mfBManual", "mfBTone", "mfBMix" } };
        for (int s = 0; s < 2; ++s)
        {
            Panel& pn = s == 0 ? mfPanel0 : mfPanel1;
            pn.addRow ({ &makeToggle (ids[s][0], s == 0 ? "MOD FX A ON" : "MOD FX B ON"), &makeCombo (ids[s][1]), &makeCombo (ids[s][2]),
                         &makeCombo (ids[s][3]), &makeSyncCombo (ids[s][4]) }, { 1.0f, 1.6f, 1.2f, 0.9f, 1.2f });
            pn.setKnobColumns (6);
            addKnob (pn, ids[s][5], "Rate", " Hz", 2);
            mfRateKnob[s] = knobs.back().get();
            addKnob (pn, ids[s][6],  "Depth");
            addKnob (pn, ids[s][7],  "Feedback");
            addKnob (pn, ids[s][8],  "Manual");
            addKnob (pn, ids[s][9],  "Tone", " Hz", 0);
            addKnob (pn, ids[s][10], "Mix");
        }
    }

    // ---- REVERB SHAPING (v51), on the POST page under the reverb
    reverbShapePanel.addRow ({ &makeCombo ("reverbDragMode"), &makeCombo ("reverbSymphPitch") }, { 1.0f, 1.0f });
    reverbShapePanel.setKnobColumns (7);
    addKnob (reverbShapePanel, "reverbDiffuse", "Diffuse");
    addKnob (reverbShapePanel, "reverbSpace",   "Space", " x", 2);
    addKnob (reverbShapePanel, "reverbMod",     "Mod");
    addKnob (reverbShapePanel, "reverbDrag",    "Drag");
    addKnob (reverbShapePanel, "reverbBloom",   "Bloom");
    addKnob (reverbShapePanel, "reverbSymph",   "Symphony");
    addKnob (reverbShapePanel, "reverbDrive",   "Spring Drive");

    // ---- MODERN METAL (only for the British Hot-Rod, Saturated Lead and Modern High-Gain amps)
    metalPanel.addRow ({ &makeToggle ("mtLead", "LEAD BOOST") }, {}, 26);
    metalPanel.setKnobColumns (7);
    addKnob (metalPanel, "mtTight", "Tight");
    addAdvKnob (metalPanel, "mtDepth", "Depth");
    addAdvKnob (metalPanel, "mtMidHz", "Mid Freq");
    addAdvKnob (metalPanel, "mtMid",   "Mid Boost");
    addAdvKnob (metalPanel, "mtFizz",  "Fizz Tamer");
    addKnob (metalPanel, "mtChug",  "Chug Gate");
    setupAdvanced (metalPanel, "metal");

    // ---- TRANSPOSE / FORMANT (first in the chain)
    transposePanel.addRow ({ &makeToggle ("tpOn", "TRANSPOSE ON"), &makeCombo ("tpQuality"), &makeToggle ("tpLock", "FORMANT LOCK") },
                           { 1.2f, 1.5f, 1.3f });
    transposePanel.addRow ({ &makeCombo ("tpRange") }, {});
    transposePanel.setKnobColumns (4);
    addKnob (transposePanel, "tpSemis",   "Transpose", " st", 0);
    addKnob (transposePanel, "tpFine",    "Fine",      " ct", 0);
    addKnob (transposePanel, "tpFormant", "Formant",   " st", 1);
    addKnob (transposePanel, "tpMix",     "Mix");

    // ---- PITCH PEDAL (expression-pedal style) and OCTAVER, on the MOD page
    wowPanel.addRow ({ &makeToggle ("wowOn", "PITCH PEDAL ON"), &makeCombo ("wowMode") }, { 1.0f, 1.7f });
    wowPanel.setKnobColumns (5);
    addKnob (wowPanel, "wowPos",   "Position");
    addKnob (wowPanel, "wowHeel",  "Heel", " st", 1);
    wowHeelKnob = knobs.back().get();
    addKnob (wowPanel, "wowToe",   "Toe",  " st", 1);
    wowToeKnob = knobs.back().get();
    addAdvKnob (wowPanel, "wowGlide", "Glide", " ms", 0);
    addKnob (wowPanel, "wowMix",   "Mix");
    setupAdvanced (wowPanel, "wow");

    octPanel.addRow ({ &makeToggle ("octOn", "OCTAVER ON") }, {}, 26);
    octPanel.addRow ({ &makeCombo ("octA"), &makeCombo ("octB") }, {}, 26);
    octPanel.setKnobColumns (4);
    addKnob (octPanel, "octALvl", "Voice A");
    addKnob (octPanel, "octBLvl", "Voice B");
    addKnob (octPanel, "octDry",  "Dry");
    addKnob (octPanel, "octTone", "Tone");

    // ---- HARMONIZER (v52)
    harmPanel.addRow ({ &makeToggle ("hmOn", "HARMONIZER ON"), &makeCombo ("hmPos"), &makeCombo ("hmMode") }, { 1.0f, 1.2f, 0.9f });
    harmPanel.addRow ({ &makeCombo ("hmTrack"), &makeCombo ("hmKey"), &makeCombo ("hmScale") }, { 1.1f, 0.5f, 1.1f });
    harmHeard.setFont (juce::Font (juce::FontOptions (12.0f)));
    harmHeard.setColour (juce::Label::textColourId, ui::textDim);
    harmHeard.setJustificationType (juce::Justification::centredLeft);
    harmHeard.setText ("Hearing: nothing yet", juce::dontSendNotification);
    harmHeard.setTooltip ("The notes the harmonizer is hearing right now. If these are wrong, the harmony will be wrong: check that the guitar is in tune and the input is clean.");
    harmPanel.addRow ({ &harmHeard }, {}, 20);
    harmPanel.setKnobColumns (6);
    addKnob (harmPanel, "hmAInt",  "A Interval", " st", 0);
    addKnob (harmPanel, "hmADeg",  "A Step",     "", 0);
    addKnob (harmPanel, "hmALvl",  "A Level");
    addKnob (harmPanel, "hmAPan",  "A Pan", "", 2);
    addKnob (harmPanel, "hmDry",   "Dry");
    addKnob (harmPanel, "hmTone",  "Tone");
    addKnob (harmPanel, "hmBInt",  "B Interval", " st", 0);
    addKnob (harmPanel, "hmBDeg",  "B Step",     "", 0);
    addKnob (harmPanel, "hmBLvl",  "B Level");
    addKnob (harmPanel, "hmBPan",  "B Pan", "", 2);
    addKnob (harmPanel, "hmGlide", "Glide", " ms", 0);

    // ---- SPEAKER page: per-block EQ / resonance / phase, speaker dynamics, and exporting the blend as an IR
    {
        for (int blk = 0; blk < kCabBlocks; ++blk)
        {
            auto& tab = shapeTabs[blk];
            tab.setButtonText ("BLOCK " + cabLetter (blk));
            tab.setComponentID ("tab");
            tab.setClickingTogglesState (true);
            tab.setRadioGroupId (4713);
            tab.onClick = [this, blk] { shapePanel.setPage (blk); };
            tab.setTooltip ("Show the shaping controls of cab block " + cabLetter (blk) + ". This doesn't turn the block on.");
        }
        shapePanel.addRow ({ &shapeTabs[0], &shapeTabs[1], &shapeTabs[2], &shapeTabs[3] }, {}, 24);
        shapePanel.setKnobColumns (7);
        static const char* eqNames[5] = { "Low", "Low Mid", "Mid", "High Mid", "High" };
        for (int blk = 0; blk < kCabBlocks; ++blk)
        {
            const auto L = cabLetter (blk);
            for (int b2 = 0; b2 < 5; ++b2)
                addKnob (shapePanel, ("cabEq" + juce::String (b2 + 1) + L).toRawUTF8(), eqNames[b2], " dB", 1, blk);
            addKnob (shapePanel, ("cabRso" + L).toRawUTF8(), "Reso", " dB", 1, blk);
            addKnob (shapePanel, ("cabPhs" + L).toRawUTF8(), "Phase", {}, 2, blk);
        }
        shapeTabs[0].setToggleState (true, juce::dontSendNotification);
        shapePanel.setPage (0);

        dynPanel.addRow ({ &makeCombo ("cabDynMode") }, {}, 26);
        {
            auto& listen = makeToggle ("cabTameDelta", "LISTEN TO CUTS");
            dynPanel.addRow ({ &listen }, {}, 26);
            dynPanel.markAdvanced (&listen);
        }
        dynPanel.setKnobColumns (3);
        addKnob (dynPanel, "cabDyn", "Amount");
        addKnob (dynPanel, "cabTame", "Tame");
        addKnob (dynPanel, "cabMotion", "Motion");
        addAdvKnob (dynPanel, "cabMotionRate", "Rate");
        setupAdvanced (dynPanel, "dyn");

        exportStatus.setText ("Saves the cab blend you hear (every active block with its cuts, EQ, resonance, phase, delay, polarity, level and pan) as a stereo 24-bit 48 kHz WAV, to use in any IR loader. The speaker dynamics, room, amp A / B routing and Post EQ are not included.",
                              juce::dontSendNotification);
        exportStatus.setJustificationType (juce::Justification::topLeft);
        exportStatus.setMinimumHorizontalScale (1.0f);
        exportStatus.setColour (juce::Label::textColourId, ui::textDim);
        exportStatus.setFont (juce::FontOptions (13.0f));
        exportButton.setTooltip ("Choose where to save the WAV. The cab blocks must be on and have an IR; file IRs are exported at their analysed length.");
        exportButton.onClick = [this] { exportCab(); };
        exportPanel.addRow ({ &exportButton }, {}, 30);
        exportPanel.addRow ({ &exportStatus }, {}, 52);
    }

    acousticPanel.addRow ({ &makeCombo ("acBody"), nullptr }, { 1.0f, 2.0f });
    acousticPanel.setKnobColumns (8);
    addKnob (acousticPanel, "acBodyAmt", "Body");
    addKnob (acousticPanel, "acFix",     "Piezo Fix");
    addKnob (acousticPanel, "acPreamp",  "Preamp");
    addKnob (acousticPanel, "acSqueeze", "Squeeze");
    addKnob (acousticPanel, "acSparkle", "Sparkle", " dB", 1);
    addKnob (acousticPanel, "acWarmth",  "Warmth",  " dB", 1);
    addKnob (acousticPanel, "acRoom",    "Room");
    addKnob (acousticPanel, "acLevel",   "Level",   " dB", 1);

    // ---- CAB: two IR slots, the shared cab controls, and the IR display
    for (int blk = 0; blk < kCabBlocks; ++blk)
        irButtons[blk].onClick = [this, blk] { chooseIR (blk); };
    alignButton.onClick = [this] { processorRef.alignIRs(); };
    libraryButton.onClick = [this] { openLibraryWindow(); };

    // Each IR slot is either a built-in model (speaker, mic, position, distance, angle) or an IR file.
    // Its Source menu picks which; page 0 shows the model controls, page 1 the file button.
    auto buildSlot = [this] (Panel& panel, const char* s, juce::TextButton& fileButton)
    {
        const juce::String slot (s);
        auto& src = makeCombo (("cabSrc" + slot).toRawUTF8());
        src.onChange = [this] { syncIRPages(); };
        panel.addRow ({ &makeToggle (("cabOn" + slot).toRawUTF8(), "ON"), &src, &makeToggle (("cabPol" + slot).toRawUTF8(), "INVERT") }, { 0.7f, 1.5f, 1.0f });
        panel.addRow ({ &makeCombo (("cabAmp" + slot).toRawUTF8()) }, {}, 26);
        panel.addRow ({ &makeSpeakerCombo (("cabSpk" + slot).toRawUTF8()), &makeMicCombo (("cabMic" + slot).toRawUTF8()) }, {}, 26, 0);
        panel.addRow ({ &makeCabinetCombo (("cabCbn" + slot).toRawUTF8()) }, {}, 26, 0);
        panel.addRow ({ &fileButton }, {}, 26, 1);
        panel.setKnobColumns (5);
        addKnob (panel, ("cabPos" + slot).toRawUTF8(),  "Position", {}, 2, 0);
        addKnob (panel, ("cabDist" + slot).toRawUTF8(), "Distance", " in", 1, 0);
        addKnob (panel, ("cabAng" + slot).toRawUTF8(),  "Angle", " deg", 0, 0);
        addKnob (panel, ("cabTight" + slot).toRawUTF8(), "Tight", {}, 2, 0);   // v67
        addKnob (panel, ("cabBite" + slot).toRawUTF8(),  "Bite",  {}, 2, 0);
        addKnob (panel, ("cabBox" + slot).toRawUTF8(),   "Box",   {}, 2, 0);
        addKnob (panel, ("cabLevel" + slot).toRawUTF8(),   "Level",    " dB", 1);
        addKnob (panel, ("cabPan" + slot).toRawUTF8(),     "Pan");
        addKnob (panel, ("cabDelay" + slot).toRawUTF8(),   "Delay",    " ms", 2);   // v70: manual alignment, on the main page
        addKnob (panel, ("cabTilt" + slot).toRawUTF8(),    "Tilt",     " dB", 1);   // v70: bright to dark
        addAdvKnob (panel, ("cabLowCut" + slot).toRawUTF8(),  "Low Cut",  " Hz", 0);
        addAdvKnob (panel, ("cabHighCut" + slot).toRawUTF8(), "High Cut", " Hz", 0);
        setupAdvanced (panel, ("cab" + slot).toRawUTF8());
    };
    for (int blk = 0; blk < kCabBlocks; ++blk)
        buildSlot (*irPanels[blk], cabLetter (blk).toRawUTF8(), irButtons[blk]);
    syncIRPages();

    cabPanel.addRow ({ &makeToggle ("cabOn", "CAB ON"), &makeToggle ("cabAutoAlign", "AUTO ALIGN"), &alignButton, &libraryButton, &makeToggle ("cabMeasured", "REALISTIC") });
    cabPanel.addRow ({ &makeCombo ("cabRoomType") }, {}, 26);
    cabPanel.setKnobColumns (8);
    addKnob (cabPanel, "cabRoom",    "Room");
    addKnob (cabPanel, "cabSpread",  "Spread");
    addKnob (cabPanel, "cabLowCut",  "Low Cut",  " Hz", 0);
    addKnob (cabPanel, "cabHighCut", "High Cut", " Hz", 0);
    addKnob (cabPanel, "cabTilt",    "Tilt",     " dB", 1);
    addKnob (cabPanel, "cabReso",    "Reso",     " dB", 1);
    addKnob (cabPanel, "cabResoHz",  "Reso Hz",  " Hz", 0);
    addKnob (cabPanel, "cabAir",     "Air",      " dB", 1);

    // ---- POST EQ: nine faders, curve, Q and Level
    toneMatchButton.setTooltip ("Tone matching: opens a window where you load a reference recording, play your guitar, and have the Post EQ fitted so your tone's overall balance moves toward the reference.");
    toneMatchButton.onClick = [this] { openToneWindow(); };
    toneLabButton.setTooltip ("Tone Lab: load a clean recording of your guitar (DI) and a recording of the sound you want, and the plugin searches its amp, cab, pedal and EQ settings for the closest match. It matches tone and dynamics, not reverb or delay.");
    toneLabButton.onClick = [this] { openToneLabWindow(); };
    eqPanel.addRow ({ &makeToggle ("eqOn", "EQ ON"), &makeCombo ("eqMode"), &makeCombo ("eqSlope"), &makeToggle ("eqAuto", "AUTO LEVEL"), &toneMatchButton, &toneLabButton }, { 0.8f, 1.3f, 0.9f, 1.0f, 0.9f, 1.0f });
    {
        static const char* names[kEqBands] = { "63", "125", "250", "500", "1k", "2k", "4k", "8k", "16k" };
        for (int b2 = 0; b2 < kEqBands; ++b2)
            eqFaders.push_back (std::make_unique<FaderControl> (processorRef.apvts, "eqB" + juce::String (b2 + 1), names[b2]));

        std::vector<KnobControl*> eqKnobs;
        auto makeEqKnob = [this, &eqKnobs] (const char* id, const char* name, const char* suffix, int decimals)
        {
            knobs.push_back (std::make_unique<KnobControl> (processorRef.apvts, id, name, suffix, decimals));
            eqKnobs.push_back (knobs.back().get());
        };
        makeEqKnob ("eqQ",        "Q (Mod)",  "",     2);
        makeEqKnob ("eqLevel",    "Level",    " dB",  1);
        makeEqKnob ("eqLowCut",   "Low Cut",  " Hz",  0);
        makeEqKnob ("eqHighCut",  "High Cut", " Hz",  0);
        eqPanel.setup (eqFaders, eqKnobs);
    }

    // ---- POST-AMP DELAY
    auto& postNote = makeCombo ("postDelayNote");
    postNote.onChange = [this] { syncDelayKnobs(); };
    postDelayPanel.addRow ({ &makeToggle ("postDelayOn", "DELAY ON"), &makeCombo ("postDelayMode"), &postNote }, { 1.0f, 1.0f, 1.3f });
    postDelayPanel.addRow ({ &makeCombo ("postDelayStereo"), &makeCombo ("postDelayStyle") }, { 1.6f, 1.0f });
    addKnob (postDelayPanel, "postDelayMix",      "Mix");
    addKnob (postDelayPanel, "postDelayTime",     "Time", " ms", 0);
    postTimeKnob = knobs.back().get();
    addKnob (postDelayPanel, "postDelayFeedback", "Feedback");
    addAdvKnob (postDelayPanel, "postDelayLofi",     "Lo-Fi");
    addAdvKnob (postDelayPanel, "postDelayWarble",   "Warble");
    setupAdvanced (postDelayPanel, "postDelay");
    addKnob (postDelayPanel, "postDelayOffset",   "R Offset");
    delayOffsetKnob = knobs.back().get();
    addKnob (postDelayPanel, "postDelaySpread",   "Spread");
    delaySpreadKnob = knobs.back().get();

    // ---- REVERB
    reverbPanel.addRow ({ &makeToggle ("reverbOn", "REVERB ON"), &makeToggle ("shimmerOn", "SHIMMER"),
                          &(shimmerPitchBox = &makeCombo ("shimmerPitch"), *shimmerPitchBox), &makeToggle ("reverbFreeze", "FREEZE") },
                        { 1.2f, 1.0f, 1.4f, 1.0f });
    reverbPanel.addRow ({ &makeCombo ("reverbType"), &makeSyncCombo ("reverbPreNote"), &(shimmerModeBox = &makeCombo ("shimmerMode"), *shimmerModeBox) }, {});
    reverbPanel.setKnobColumns (6);
    addKnob (reverbPanel, "reverbMix",      "Mix");
    addKnob (reverbPanel, "reverbSize",     "Decay", " s", 1);
    addKnob (reverbPanel, "reverbPreDelay", "Pre-Dly",  " ms", 0);
    reverbPreKnob = knobs.back().get();
    addKnob (reverbPanel, "reverbDamp",     "Damp");
    addAdvKnob (reverbPanel, "reverbWidth",    "Width");
    addAdvKnob (reverbPanel, "reverbLowCut",   "Low Cut",  " Hz", 0);
    addAdvKnob (reverbPanel, "reverbHighCut",  "High Cut", " Hz", 0);
    addAdvKnob (reverbPanel, "reverbDuck",     "Duck");
    setupAdvanced (reverbPanel, "reverb");
    addKnob (reverbPanel, "shimmerAmount",  "Shimmer");
    shimmerAmountKnob = knobs.back().get();
    addKnob (reverbPanel, "shimmerTone",    "Tone",     " Hz", 0);
    shimmerToneKnob = knobs.back().get();
    addKnob (reverbPanel, "shimmerDetune",  "Detune",   " ct", 1);
    shimmerDetuneKnob = knobs.back().get();

    // ---- OUTPUT
    outputPanel.addRow ({ &makeToggle ("limOn", "LIMITER"), &makeToggle ("outMono", "MONO") });
    meterView.setTooltip ("Peak level of the input (after the Input setting, before the gain) and of the final output. The light at the right of each bar stays on after a sample reached 0 dBFS; click to clear it.");
    outputPanel.addRow ({ &meterView }, {}, 44);
    outputPanel.setKnobColumns (3);
    addKnob (outputPanel, "outGain", "Level", " dB", 1);
    addKnob (outputPanel, "limCeil", "Ceiling", " dB", 1);
    limCeilKnob = knobs.back().get();
    addKnob (outputPanel, "limRel",  "Release", " ms", 0);
    limRelKnob = knobs.back().get();

    // ---- Pages. Each page is laid out in its own coordinates, starting at (0, 0).
    auto place = [] (juce::Component& panel, int& x, int y, int width, int height)
    {
        panel.setBounds (x, y, width, height);
        x += width + 10;
    };
    for (auto* page : { &pageInput, &pageGain, &pageMod, &pageDelay, &pagePitch, &pageAmp, &pageCab, &pageSpeaker, &pageEq, &pageScapes, &pagePost })
        page->setBounds (10, ui::kPageY, ui::kWidth - 20, ui::kPageHeight);

    // The AMP page has three tabs: Amp A, the parallel Amp B (same layout and size), and the NAM model player.
    for (int i = 0; i < 3; ++i)
    {
        auto& t = i == 0 ? ampTabA : i == 1 ? ampTabB : ampTabN;
        t.setComponentID ("tab");
        t.setClickingTogglesState (true);
        t.setRadioGroupId (4713);
        t.onClick = [this, i] { showAmpTab (i); };
        t.setBounds (i * 128, 0, 124, 28);
        pageAmp.addAndMakeVisible (t);
    }
    ampTabA.setTooltip ("Amp A: the main amp.");
    ampTabB.setTooltip ("Amp B: the parallel second amp. Same controls as Amp A. It only runs when AMP B ON is switched on (not available with the Acoustic amp).");
    ampTabN.setTooltip ("NAM: load a neural amp capture (.nam file) to use in place of Amp A's preamp and power stage.");
    ampTab = juce::jlimit (0, 2, (int) processorRef.apvts.state.getProperty ("uiAmpTab", 0));

    // ---- Hardware looks: stompboxes for the pedals and effects, black tolex / cab cloth / wood for the amp
    //      and cabs, and brushed rack units for everything else.
    {
        auto pedal = [] (Panel& p, juce::uint32 colour) { p.setStyle (PanelKind::Pedal, juce::Colour (colour)); };
        pedal (noisePanel, 0xff3b4b5e);    pedal (fuzzPanel, 0xffa04a14); pedal (fuzz2Panel, 0xffb85a1e);     pedal (compPanel, 0xff22508c);
        pedal (overdrivePanel, 0xff1f6b40); pedal (drive2Panel, 0xff2a7a4c); pedal (wahPanel, 0xff8a2a3a);      pedal (phaserPanel, 0xff8a6410);
        pedal (chorusPanel, 0xff176670);   pedal (preDelayPanel, 0xff4e3a8e);  pedal (postDelayPanel, 0xff4e3a8e);
        pedal (transposePanel, 0xff7c2a66); pedal (grainPanel, 0xff2f5d52);    pedal (tremPanel, 0xff6b4326);
        pedal (wobblePanel, 0xff5b3a70);   pedal (swellPanel, 0xff2c4f7a);     pedal (doublerPanel, 0xff6a3030);
        pedal (widenerPanel, 0xff2b5c68);  pedal (wowPanel, 0xff8e2323);       pedal (octPanel, 0xff2a5c3a);
        ampPanel.setStyle (PanelKind::Amp);
        metalPanel.setStyle (PanelKind::Amp);
        ampBPanel.setStyle (PanelKind::Amp);
        namStrip[0].panel.setStyle (PanelKind::Amp);
        namStrip[1].panel.setStyle (PanelKind::Amp);
        pedal (namStrip[2].panel, 0xff3d5a2e);  pedal (namStrip[3].panel, 0xff5a3d6a);
        pedal (lofiPanel, 0xff6b5a2c);
        pedal (harmPanel, 0xff5a3a8a);  pedal (preDelayFxPanel, 0xff4e3a8e);  pedal (postDelayFxPanel, 0xff4e3a8e);
        pedal (boostPanel, 0xff9a6b1a);
        pedal (mfPanel0, 0xff1f5a7a);  pedal (mfPanel1, 0xff3a5a2a);
        acousticPanel.setStyle (PanelKind::Wood);
        for (auto* p : irPanels)
            p->setStyle (PanelKind::Cab);
        // inputPanel, cabPanel, eqPanel, reverbPanel, outputPanel and the displays keep the default rack look
    }

    int x = 0;
    // AMP
    x = 0;
    // (the amp panels are positioned by syncVisibility: Amp A and Amp B share one spot, chosen by the tabs)
    ampPanel.setBounds (0, 32, 1000, 260);
    ampBPanel.setBounds (0, 32, 1000, 260);
    namStrip[0].panel.setBounds (0,   32,  735, 238);   namStrip[1].panel.setBounds (745, 32,  735, 238);
    namStrip[2].panel.setBounds (0,   278, 735, 238);   namStrip[3].panel.setBounds (745, 278, 735, 238);
    x = 0;
    place (acousticPanel, x, 190, 1000, 220);
    // The Modern Metal panel must be given its size here, after its controls exist. (Before v34 it was only
    // sized by an early call made while the window was being built, when the panel was still empty, so its
    // controls were never laid out and never appeared.)
    x = 0;
    place (metalPanel, x, 300, 1000, 190);

    // CAB: the two IR slots and the shared cab controls, then the IR display
    // v73: the CAB tab has two sub-tabs (cab blocks / speaker shaping and export), so the blocks start below the sub-tab row.
    x = 0;
    for (int blk = 0; blk < kCabBlocks; ++blk)
        place (*irPanels[blk], x, 28, 360, 308);
    x = 0;
    place (cabPanel, x, 344, 560, 176);  place (micView, x, 344, 400, 176);  place (irView, x, 344, 500, 176);

    // SPEAKER (the second sub-tab of CAB)
    x = 0;
    place (shapePanel, x, 32, 900, 262);  place (dynPanel, x, 32, 340, 262);
    x = 0;
    place (exportPanel, x, 302, 900, 130);
    for (int pg = 0; pg < 2; ++pg)
        for (int t = 0; t < 2; ++t)
        {
            auto& b = cabSubTabs[pg][t];
            b.setButtonText (t == 0 ? "CAB BLOCKS" : "SPEAKER SHAPING & EXPORT");
            b.setComponentID ("tab");
            b.setClickingTogglesState (true);
            b.setRadioGroupId (4720 + pg);
            b.setBounds (t == 0 ? 0 : 168, 0, t == 0 ? 160 : 250, 24);
            b.onClick = [this, t] { showCabTab (t); };
            (pg == 0 ? pageCab : pageSpeaker).addAndMakeVisible (b);
        }
    cabSubTabs[0][0].setTooltip ("The four cab blocks, the mic position and the alignment display.");
    cabSubTabs[0][1].setTooltip ("Per-block EQ, resonance and phase, speaker dynamics, and exporting the cab blend as a WAV impulse response.");
    cabSubTabs[1][0].setTooltip (cabSubTabs[0][0].getTooltip());
    cabSubTabs[1][1].setTooltip (cabSubTabs[0][1].getTooltip());
    cabTab = juce::jlimit (0, 1, (int) processorRef.apvts.state.getProperty ("uiCabTab", 0));

    // EQ
    x = 0;
    place (eqPanel, x, 0, 900, 260);
    x = 0;
    place (spectrumView, x, 268, 900, 240);

    // The pages made of pedals (INPUT, GAIN, MOD, PITCH, DELAY, SCAPES, POST FX) are laid out by layoutFlow (v73).
    {
        auto item = [] (Panel& main, int w, const char* onId, const char* name, Panel* extra = nullptr, int wExtra = 0, int lanes = 1, int lane = -1)
        {
            FlowItem it;
            it.main = &main;  it.w = w;  it.onId = onId;  it.name = name;
            it.extra = extra;  it.wExtra = wExtra;  it.lanes = lanes;  it.lane = lane;
            return it;
        };
        auto page = [this] (juce::Component& pg, std::vector<FlowItem> items) { addFlowPage (pg, std::move (items)); };
        std::vector<FlowItem> v;

        v.push_back (item (inputPanel, 300, "", "", nullptr, 0, 2));
        v.push_back (item (transposePanel, 560, "tpOn", "TRANSPOSE / FORMANT"));
        page (pageInput, std::move (v));  v.clear();

        v.push_back (item (fuzzPanel, 380, "fuzzOn", "FUZZ"));
        v.push_back (item (fuzz2Panel, 380, "fuzz2On", "FUZZ 2"));
        v.push_back (item (compPanel, 540, "compOn", "COMPRESSOR"));
        v.push_back (item (overdrivePanel, 360, "odOn", "OVERDRIVE"));
        v.push_back (item (drive2Panel, 360, "od2On", "DRIVE 2"));
        v.push_back (item (boostPanel, 560, "bsOn", "BOOST"));
        v.push_back (item (boostEqPanel, 880, "bsEqOn", "BOOST EQ"));
        page (pageGain, std::move (v));  v.clear();

        v.push_back (item (wahPanel, 520, "wahOn", "WAH"));
        v.push_back (item (phaserPanel, 440, "phaserOn", "PHASER"));
        v.push_back (item (chorusPanel, 380, "chorusOn", "CHORUS"));
        v.push_back (item (mfPanel0, 720, "mfAOn", "MOD FX A"));
        v.push_back (item (mfPanel1, 720, "mfBOn", "MOD FX B"));
        page (pageMod, std::move (v));  v.clear();

        v.push_back (item (wowPanel, 570, "wowOn", "PITCH PEDAL", nullptr, 0, 1, 0));
        v.push_back (item (octPanel, 570, "octOn", "OCTAVER", nullptr, 0, 1, 1));
        v.push_back (item (harmPanel, 890, "hmOn", "HARMONIZER", nullptr, 0, 2));
        page (pagePitch, std::move (v));  v.clear();

        v.push_back (item (preDelayPanel, 420, "preDelayOn", "PRE-AMP DELAY", &preDelayFxPanel, 620));
        v.push_back (item (postDelayPanel, 540, "postDelayOn", "POST-AMP DELAY", &postDelayFxPanel, 620));
        page (pageDelay, std::move (v));  v.clear();

        v.push_back (item (grainPanel, 760, "granOn", "GRANULAR CLOUD"));
        v.push_back (item (tremPanel, 330, "tremOn", "TREMOLO / AUTO-PAN"));
        v.push_back (item (wobblePanel, 330, "wobOn", "WOBBLE"));
        v.push_back (item (swellPanel, 330, "swellOn", "SWELL"));
        v.push_back (item (lofiPanel, 620, "lfOn", "LO-FI WARBLE"));
        v.push_back (item (noisePanel, 418, "noiseOn", "NOISE PEDAL"));
        page (pageScapes, std::move (v));  v.clear();

        v.push_back (item (reverbPanel, 640, "reverbOn", "REVERB", &reverbShapePanel, 500));
        v.push_back (item (outputPanel, 270, "", ""));
        v.push_back (item (doublerPanel, 400, "dblOn", "DOUBLER"));
        v.push_back (item (widenerPanel, 560, "widOn", "WIDENER"));
        page (pagePost, std::move (v));
    }

    tidyOn = (bool) processorRef.apvts.state.getProperty ("uiTidy", true);
    layoutFlow (true);

    // Every panel has its controls by now: lay each one out (a panel whose size does not change when it is
    // placed would otherwise keep the empty layout from the moment it was first sized, and show nothing).
    for (auto* panel : { &inputPanel, &noisePanel, &fuzzPanel, &fuzz2Panel, &preDelayPanel, &compPanel, &wahPanel, &phaserPanel, &chorusPanel,
                         &overdrivePanel, &drive2Panel, &transposePanel, &grainPanel, &tremPanel, &wobblePanel, &swellPanel, &doublerPanel,
                         &widenerPanel, &ampPanel, &metalPanel, &ampBPanel, &namStrip[0].panel, &namStrip[1].panel, &namStrip[2].panel, &namStrip[3].panel, &lofiPanel, &boostPanel, &boostEqPanel, &mfPanel0, &mfPanel1, &reverbShapePanel, &acousticPanel, &irAPanel, &irBPanel, &irCPanel, &irDPanel,
                         &cabPanel, &postDelayPanel, &reverbPanel, &outputPanel, &wowPanel, &octPanel, &shapePanel, &dynPanel, &exportPanel,
                         &harmPanel, &preDelayFxPanel, &postDelayFxPanel })
        panel->resized();

    // ---- Tempo strip in the header (shared by both delays): TAP, a BPM box, and host-tempo switch
    tapButton.onClick = [this] { tapTempo(); };
    tapButton.setBounds (ui::kWidth - 360, 8, 64, 28);
    content.addAndMakeVisible (tapButton);

    tempoSlider.setSliderStyle (juce::Slider::LinearBar);
    tempoSlider.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 100, 28);
    tempoAttachment = std::make_unique<APVTS::SliderAttachment> (processorRef.apvts, "tempoBpm", tempoSlider);
    tempoSlider.textFromValueFunction = [] (double bpm) { return juce::String (bpm, 1) + " BPM"; };
    tempoSlider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue(); };
    tempoSlider.updateText();
    tempoSlider.setBounds (ui::kWidth - 288, 8, 130, 28);
    content.addAndMakeVisible (tempoSlider);

    auto& hostTempo = makeToggle ("tempoHost", "HOST TEMPO");
    hostTempo.setBounds (ui::kWidth - 150, 8, 138, 28);
    content.addAndMakeVisible (hostTempo);

    // ---- Global mode (Normal / Low Latency / High Quality), in the header
    {
        auto& modeBox = makeCombo ("perfMode");
        modeBox.setBounds (800, 8, 120, 28);
        content.addAndMakeVisible (modeBox);

        // Tuner: a button in the header; the tuner opens over the page and mutes the output
        processorRef.tunerActive.store (false);
        tunerButton.setClickingTogglesState (true);
        tunerButton.setTooltip ("Opens the tuner (the reference pitch is adjustable in it) and mutes the output while it is open. Play one string at a time.");
        tunerButton.onClick = [this]
        {
            const bool on = tunerButton.getToggleState();
            processorRef.tunerActive.store (on);
            tunerView.setVisible (on);
            if (on)
                tunerView.toFront (false);
        };
        tunerButton.setBounds (926, 8, 48, 28);
        content.addAndMakeVisible (tunerButton);

        midiButton.setBounds (1032, 8, 42, 28);
        midiButton.setTooltip ("MIDI: Program Change loads presets, and controllers (CC, pitch bend) can move any parameter. Opens the MIDI window. You can also right-click any knob to learn a controller for it.");
        midiButton.onClick = [this] { openMidiWindow(); };
        content.addAndMakeVisible (midiButton);
        midiWatcher.onTick = [this] { midiTick(); };
        midiWatcher.begin();
        addMouseListener (this, true);
        tunerView.setBounds ((ui::kWidth - 520) / 2, ui::kPageY + 20, 520, 180);
        content.addChildComponent (tunerView);
    }

    // ---- Presets, in the header between the title and the tempo controls
    presetPrev.setBounds (232, 8, 26, 28);
    presetBox.setBounds (262, 8, 190, 28);
    presetNext.setBounds (456, 8, 26, 28);
    browseButton.setBounds (488, 8, 56, 28);
    presetSave.setBounds (548, 8, 46, 28);
    presetBox.setTextWhenNothingSelected ("Presets...");
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1000)
            loadFactoryPreset (id - 1000);
        else if (id >= 1 && id <= presetFiles.size())
            loadPresetFile (presetFiles[id - 1]);
    };
    presetPrev.onClick   = [this] { stepPreset (-1); };
    presetNext.onClick   = [this] { stepPreset (+1); };
    presetSave.onClick   = [this] { savePresetDialog(); };
    presetDelete.onClick = [this] { deletePreset(); };
    for (juce::Component* c : { (juce::Component*) &presetPrev, (juce::Component*) &presetBox, (juce::Component*) &presetNext,
                                (juce::Component*) &presetSave, (juce::Component*) &browseButton })
        content.addAndMakeVisible (c);

    // ---- A/B compare, undo and redo, next to the presets
    abA.setBounds (602, 8, 24, 28);   abB.setBounds (630, 8, 24, 28);   abMenu.setBounds (658, 8, 44, 28);
    undoButton.setBounds (710, 8, 40, 28);   redoButton.setBounds (754, 8, 40, 28);
    for (auto* b : { &abA, &abB, &abMenu, &undoButton, &redoButton })
        content.addAndMakeVisible (b);
    abA.onClick = [this] { processorRef.abSelect (0); afterPresetLoad ((int) processorRef.apvts.state.getProperty ("uiPage", 0)); };
    abB.onClick = [this] { processorRef.abSelect (1); afterPresetLoad ((int) processorRef.apvts.state.getProperty ("uiPage", 0)); };
    abMenu.onClick = [this] { abMenuClicked(); };
    undoButton.onClick = [this] { if (processorRef.undo()) afterPresetLoad ((int) processorRef.apvts.state.getProperty ("uiPage", 0)); };
    redoButton.onClick = [this] { if (processorRef.redo()) afterPresetLoad ((int) processorRef.apvts.state.getProperty ("uiPage", 0)); };
    abA.setTooltip ("Sound A. Press A or B to switch between two versions of your sound. The first time you press B, it starts as a copy of A.");
    abB.setTooltip ("Sound B. Change anything while B is active, then press A to hear the difference. With level match on, the volume is evened out when you switch.");
    abMenu.setTooltip ("A/B options: copy the active sound to the other slot, level matching, and forgetting the measured levels. A/B slots are not saved with the session.");
    undoButton.setTooltip ("Undo the last change you made by hand (a knob, a switch, a preset, the effect order, an A/B switch). Changes that come from MIDI or your DAW's automation are not counted as steps. Shortcut: Cmd+Z.");
    redoButton.setTooltip ("Redo what you just undid. Shortcut: Cmd+Shift+Z.");
    browseButton.setTooltip ("Opens the preset browser: Factory and User tabs, search, and tags.");
    browseButton.onClick = [this] { openBrowserWindow(); };

    // ---- Tools next to the tuner
    metroButton.setBounds (978, 8, 50, 28);
    orderButton.setBounds (1078, 8, 52, 28);
    metroButton.setTooltip ("Metronome: opens its settings. The button is lit while the metronome is running.");
    metroButton.onClick = [this] { openMetroWindow(); };
    orderButton.setTooltip ("Change the order of the effects before the amp (fuzz, compressor, wah, phaser, chorus, delay) and after the cab (Post EQ, textures, doubler, delay, reverb, widener).");
    orderButton.onClick = [this] { openOrderWindow(); };
    content.addAndMakeVisible (metroButton);
    content.addAndMakeVisible (orderButton);
    chainStrip.setProcessor (&processorRef);
    setWantsKeyboardFocus (true);
    refreshPresets();

    // ---- Tabs along the top
    for (int i = 0; i < 10; ++i)
    {
        static const char* tabNames[10] = { "INPUT", "GAIN", "MOD", "PITCH", "AMP", "CAB", "EQ", "DELAY", "SCAPES", "POST FX" };
        auto& tab = pageTabs[i];
        tab.setButtonText (tabNames[i]);
        tab.setComponentID ("tab");
        tab.setClickingTogglesState (true);
        tab.setRadioGroupId (4712);
        tab.onClick = [this, i] { showPage (i); };
        tab.setBounds (12 + i * 122, 46, 118, 28);
        content.addAndMakeVisible (tab);
    }
    chainStrip.setBounds (12, 80, ui::kWidth - 24, 30);
    content.addAndMakeVisible (chainStrip);
    tidyButton.setClickingTogglesState (true);
    tidyButton.setToggleState (tidyOn, juce::dontSendNotification);
    tidyButton.setBounds (1244, 46, 96, 28);
    tidyButton.setTooltip ("Tidy view. On: pedals that are switched off shrink to small tiles, so you only see the ones you are using. Click SWITCH ON on a tile to bring a pedal back. Off: every pedal is shown in full, faded when it is off.");
    tidyButton.onClick = [this]
    {
        tidyOn = tidyButton.getToggleState();
        processorRef.apvts.state.setProperty ("uiTidy", tidyOn, nullptr);
        layoutFlow (true);
    };
    content.addAndMakeVisible (tidyButton);
    helpButton.setBounds (1346, 46, 66, 28);
    helpButton.setTooltip ("Opens a short guide: how to pick a sound, switch effects on, change their order and save your work.");
    helpButton.onClick = [this] { openHelpWindow(); };
    content.addAndMakeVisible (helpButton);
    riffButton.setBounds (12 + 11 * 128, 46, 66, 28);
    riffButton.setTooltip ("Riff recorder: record a phrase and loop it back. In DI mode the playback goes through all your pedals and the amp, so you can change the tone while it plays. The button turns red while recording and green while playing.");
    riffButton.onClick = [this] { openRiffWindow(); };
    content.addAndMakeVisible (riffButton);
    {
        static const char* tabTips[10] = {
            "INPUT: the input level and noise gate, pickup de-noise, and transpose with formant control.",
            "GAIN: fuzz, compressor, overdrive, and the boost with its 4-band EQ. These sit in front of the amp.",
            "MOD: wah, phaser, chorus, and the two mod effects slots (flanger, vibrato, rotary, auto-pan and more).",
            "PITCH: the pitch pedal, the octaver and the harmonizer.",
            "AMP: the amp model, tone controls, voicing, tuning, the acoustic / piezo model, and the NAM tab with its four neural-model slots.",
            "CAB: up to four cab blocks (built-in models or IR files) with the mic position and alignment. The second sub-tab has per-block EQ, speaker dynamics and exporting the cab as an impulse response.",
            "EQ: the Post EQ and a spectrum display of what you are hearing.",
            "DELAY: the delay before the amp and the delay after the cab, each with ducking, modulation, diffusion, hold and multi-tap.",
            "SCAPES: granular cloud, tremolo, wobble, swell, lo-fi warble and the noise pedal, for ambient and experimental textures.",
            "POST FX: reverb (with shimmer and shaping), the doubler, the widener and the output level with the limiter." };
        for (int i = 0; i < 10; ++i)
            pageTabs[i].setTooltip (tabTips[i]);

        tapButton.setTooltip ("Tap tempo: tap at least twice; the tempo is the average of your last 5 taps (a pause over 2 seconds starts again). Also turns HOST TEMPO off.");
        tempoSlider.setTooltip ("Tempo in BPM (drag, or double-click to type). Used by note-value delays and the tremolo.");
        presetBox.setTooltip ("Factory presets (grouped by style) and your saved presets (files in Documents/StarterAmp/Presets). Choose one to load it. A factory preset resets the sound to defaults first; your input, quality, tempo and limiter settings are kept.");
        presetPrev.setTooltip ("Load the previous preset.");
        presetNext.setTooltip ("Load the next preset.");
        presetSave.setTooltip ("Save the current settings as a preset. An existing name is replaced.");
        presetDelete.setTooltip ("Delete the selected preset file.");
        libraryButton.setTooltip ("Browse folders of IR files, see each one's frequency curve, hear it in place of your cab blocks (AUDITION), find near-duplicates, and send it to a block.");
        alignButton.setTooltip ("Line the active cab blocks up with the first one: sets each block's Delay and Invert from the measured offset. Blocks that match poorly are left alone.");
        for (int blk = 0; blk < kCabBlocks; ++blk)
            irButtons[blk].setTooltip ("Load a WAV or AIFF impulse response for this cab block (used when Source is IR file).");
        noiseOctButton.setTooltip (tipFor (processorRef.apvts, "noiseOct"));
        noiseOct2Button.setTooltip (tipFor (processorRef.apvts, "noiseOct2"));
        noiseGrossButton.setTooltip (tipFor (processorRef.apvts, "noiseGross"));
        noiseRingButton.setTooltip (tipFor (processorRef.apvts, "noiseRing"));
        tabOctave.setTooltip ("Show the octave settings (Panic, Chaos). This doesn't turn the effect on.");
        tabGross.setTooltip ("Show the Clash settings (scene, Drive, Blend). This doesn't turn the effect on.");
        tabRing.setTooltip ("Show the ring modulator settings. This doesn't turn the effect on.");
    }
    {
        // Sessions saved before v73 remember a page number from the old 11-tab layout.
        int savedPage = (int) processorRef.apvts.state.getProperty ("uiPage", 0);
        if (! processorRef.apvts.state.hasProperty ("uiLayout"))
        {
            static const int oldToNew[11] = { 1, 1, 2, 2, 3, 4, 5, 5, 6, 8, 9 };   // PRE FX, BOOST, MOD, MOD FX, PITCH, AMP, CAB, SPEAKER, EQ, SCAPES, POST FX
            if (savedPage >= 0 && savedPage < 11)
            {
                if (savedPage == 7) processorRef.apvts.state.setProperty ("uiCabTab", 1, nullptr);
                savedPage = oldToNew[savedPage];
            }
        }
        cabTab = juce::jlimit (0, 1, (int) processorRef.apvts.state.getProperty ("uiCabTab", 0));
        showPage (savedPage);
    }

    masterSwitches = { { &noisePanel, "noiseOn" }, { &fuzzPanel, "fuzzOn" }, { &fuzz2Panel, "fuzz2On" }, { &compPanel, "compOn" }, { &wahPanel, "wahOn" },
                       { &phaserPanel, "phaserOn" }, { &chorusPanel, "chorusOn" }, { &preDelayPanel, "preDelayOn" },
                       { &preDelayFxPanel, "preDelayOn" }, { &postDelayPanel, "postDelayOn" }, { &postDelayFxPanel, "postDelayOn" },
                       { &overdrivePanel, "odOn" }, { &drive2Panel, "od2On" }, { &transposePanel, "tpOn" }, { &grainPanel, "granOn" }, { &tremPanel, "tremOn" },
                       { &wobblePanel, "wobOn" }, { &swellPanel, "swellOn" }, { &doublerPanel, "dblOn" }, { &widenerPanel, "widOn" },
                       { &wowPanel, "wowOn" }, { &octPanel, "octOn" }, { &harmPanel, "hmOn" }, { &lofiPanel, "lfOn" }, { &boostPanel, "bsOn" },
                       { &mfPanel0, "mfAOn" }, { &mfPanel1, "mfBOn" }, { &reverbPanel, "reverbOn" }, { &reverbShapePanel, "reverbOn" },
                       { &ampBPanel, "ampBOn" } };
    updateIRButtons();
    syncDelayKnobs();
    syncVisibility();
    syncUsed();
    startTimerHz (10);   // keeps the IR buttons in step if a preset loads while the window is open

    // Resizable, keeping the shape. Everything scales with the window.
    constrainer.setFixedAspectRatio ((double) ui::kWidth / (double) ui::kHeight);
    constrainer.setSizeLimits (ui::kWidth / 2, ui::kHeight / 2, ui::kWidth * 2, ui::kHeight * 2);
    setConstrainer (&constrainer);
    setResizable (true, true);
    setSize (ui::kWidth, ui::kHeight);
}

//==============================================================================
// MidiPanel / MidiWindow
//==============================================================================
static juce::String midiSlotLabel (int slot)
{
    static const std::map<int, const char*> names { { 1, "mod wheel" }, { 2, "breath" }, { 4, "foot" }, { 7, "volume" },
                                                    { 11, "expression" }, { 64, "sustain pedal" }, { 67, "soft pedal" } };
    auto s = StarterAmpProcessor::midiSlotName (slot);
    auto it = names.find (slot);
    return it != names.end() ? s + " (" + it->second + ")" : s;
}

MidiPanel::MidiPanel (StarterAmpProcessor& p) : processor (p)
{
    setSize (560, 470);

    channelLabel.setText ("Listen on channel", juce::dontSendNotification);
    channelLabel.setColour (juce::Label::textColourId, ui::textDim);
    addAndMakeVisible (channelLabel);
    channelBox.addItem ("All channels", 1);
    for (int ch = 1; ch <= 16; ++ch) channelBox.addItem ("Channel " + juce::String (ch), ch + 1);
    channelBox.setSelectedId (processor.midiChannel.load() + 1, juce::dontSendNotification);
    channelBox.onChange = [this] { processor.midiChannel = channelBox.getSelectedId() - 1; };
    channelBox.setTooltip ("Which MIDI channel the plugin listens to. All channels is the usual choice.");
    addAndMakeVisible (channelBox);

    programNote.setText ("Program Change n loads factory preset n, counting from 0 (the first preset is 0).", juce::dontSendNotification);
    programNote.setColour (juce::Label::textColourId, ui::textDim);
    programNote.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (programNote);

    list.setRowHeight (24);
    list.setColour (juce::ListBox::backgroundColourId, ui::panel);
    list.setTooltip ("The controllers that are mapped. Select one to remove or invert it.");
    addAndMakeVisible (list);

    // Parameters, listed by name
    std::vector<std::pair<juce::String, int>> named;
    for (int i = 0; i < processor.midiParamCount(); ++i)
        named.push_back ({ processor.midiParamName (i), i });
    std::sort (named.begin(), named.end(), [] (const auto& a, const auto& b) { return a.first.compareIgnoreCase (b.first) < 0; });
    int id = 1;
    for (auto& n : named) { paramBox.addItem (n.first, id++); sortedParams.push_back (n.second); }
    paramBox.setTextWhenNothingSelected ("Choose a parameter...");
    paramBox.setTooltip ("The parameter to control.");
    addAndMakeVisible (paramBox);

    for (int s = 0; s < StarterAmpProcessor::kMidiSlots; ++s)
        slotBox.addItem (midiSlotLabel (s), s + 1);
    slotBox.setSelectedId (12, juce::dontSendNotification);
    slotBox.setTooltip ("The controller to use: a CC number, or the pitch bend wheel.");
    addAndMakeVisible (slotBox);

    auto paramIndex = [this] { const int sel = paramBox.getSelectedId(); return sel > 0 ? sortedParams[(size_t) sel - 1] : -1; };

    learnButton.setTooltip ("Learn: pick a parameter above, press LEARN, then move a knob, pedal or wheel on your controller. The next controller that moves is assigned to it.");
    learnButton.onClick = [this, paramIndex]
    {
        if (processor.midiLearning() >= 0) { processor.midiCancelLearn(); return; }
        if (paramIndex() >= 0) processor.midiStartLearn (paramIndex());
    };
    bindButton.setTooltip ("Assign the chosen controller to the chosen parameter without moving anything. A parameter has one controller, and a controller drives one parameter.");
    bindButton.onClick = [this, paramIndex]
    {
        if (paramIndex() >= 0) { processor.midiBind (slotBox.getSelectedId() - 1, paramIndex(), false); refreshRows(); }
    };
    removeButton.setTooltip ("Remove the selected mapping.");
    removeButton.onClick = [this] { if (selectedSlot() >= 0) { processor.midiUnbind (selectedSlot()); refreshRows(); } };
    invertButton.setTooltip ("Flip the selected mapping, so turning the controller up turns the parameter down.");
    invertButton.onClick = [this]
    {
        const int s = selectedSlot();
        if (s >= 0) { processor.midiSetInverted (s, ! processor.midiInverted (s)); list.repaint(); }
    };
    defaultsButton.setTooltip ("Go back to the starting mappings: CC 11 (expression pedal) moves the wah position and the pitch wheel moves the pitch pedal position.");
    defaultsButton.onClick = [this] { processor.midiResetDefaults(); refreshRows(); };
    for (auto* b : { &learnButton, &bindButton, &removeButton, &invertButton, &defaultsButton })
        addAndMakeVisible (b);

    status.setColour (juce::Label::textColourId, ui::text);
    status.setFont (juce::Font (juce::FontOptions (13.0f)));
    addAndMakeVisible (status);
    refreshRows();
}

void MidiPanel::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText ("MAPPINGS", 16, 78, 200, 16, juce::Justification::left);
    g.drawText ("ADD A MAPPING", 16, 322, 200, 16, juce::Justification::left);
}

void MidiPanel::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto top = r.removeFromTop (28);
    channelLabel.setBounds (top.removeFromLeft (130));
    channelBox.setBounds (top.removeFromLeft (170));
    r.removeFromTop (6);
    programNote.setBounds (r.removeFromTop (22));
    r.removeFromTop (22);                                   // the MAPPINGS caption
    list.setBounds (r.removeFromTop (190));
    r.removeFromTop (6);
    auto row = r.removeFromTop (28);
    removeButton.setBounds (row.removeFromLeft (100));  row.removeFromLeft (8);
    invertButton.setBounds (row.removeFromLeft (100));  row.removeFromLeft (8);
    defaultsButton.setBounds (row.removeFromLeft (110));
    r.removeFromTop (30);                                   // the ADD A MAPPING caption
    paramBox.setBounds (r.removeFromTop (28));
    r.removeFromTop (8);
    auto row2 = r.removeFromTop (28);
    slotBox.setBounds (row2.removeFromLeft (230));  row2.removeFromLeft (8);
    learnButton.setBounds (row2.removeFromLeft (100));  row2.removeFromLeft (8);
    bindButton.setBounds (row2.removeFromLeft (100));
    r.removeFromTop (10);
    status.setBounds (r.removeFromTop (22));
}

void MidiPanel::refreshRows()
{
    const int keep = selectedSlot();
    rows.clear();
    for (int s = 0; s < StarterAmpProcessor::kMidiSlots; ++s)
        if (processor.midiTargetOf (s) >= 0)
            rows.push_back (s);
    list.updateContent();
    for (size_t i = 0; i < rows.size(); ++i)
        if (rows[i] == keep) list.selectRow ((int) i);
    list.repaint();
}

void MidiPanel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) rows.size())) return;
    const int slot = rows[(size_t) row];
    if (selected) { g.setColour (ui::accent.withAlpha (0.22f)); g.fillRect (0, 0, w, h); }
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (13.0f)));
    g.drawText (midiSlotLabel (slot), 8, 0, 200, h, juce::Justification::centredLeft);
    g.setColour (ui::textDim);
    g.drawText (processor.midiInverted (slot) ? "inverted" : "", w - 90, 0, 82, h, juce::Justification::centredRight);
    g.setColour (ui::text);
    g.drawText (processor.midiParamName (processor.midiTargetOf (slot)), 214, 0, w - 214 - 96, h, juce::Justification::centredLeft);
}

void MidiPanel::timerCallback()
{
    const int learning = processor.midiLearning();
    learnButton.setButtonText (learning >= 0 ? "CANCEL" : "LEARN");
    const int activity = processor.midiActivity.load();
    if (activity != lastActivity) { lastActivity = activity; refreshRows(); }

    juce::String text;
    if (learning >= 0)
        text = "Move a controller now to assign it to " + processor.midiParamName (learning) + "...";
    else if (processor.midiLastSlot.load() >= 0)
        text = "Last received: " + midiSlotLabel (processor.midiLastSlot.load()) + " = " + juce::String (processor.midiLastValue.load())
             + (processor.midiLastProgram.load() >= 0 ? "     Last program change: " + juce::String (processor.midiLastProgram.load()) : juce::String());
    else
        text = processor.midiLastProgram.load() >= 0 ? "Last program change: " + juce::String (processor.midiLastProgram.load()) : "Waiting for MIDI...";
    status.setText (text, juce::dontSendNotification);
}

MidiWindow::MidiWindow (StarterAmpProcessor& p, juce::LookAndFeel& laf)
    : juce::DocumentWindow ("MIDI", ui::background, juce::DocumentWindow::closeButton), panel (p)
{
    setLookAndFeel (&laf);
    panel.setLookAndFeel (&laf);
    setUsingNativeTitleBar (true);
    setContentNonOwned (&panel, true);
    setResizable (false, false);
    setAlwaysOnTop (true);
    centreWithSize (getWidth(), getHeight());
}

MidiWindow::~MidiWindow()
{
    panel.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void StarterAmpEditor::openMidiWindow()
{
    if (midiWindow == nullptr)
        midiWindow = std::make_unique<MidiWindow> (processorRef, lookAndFeel);
    midiWindow->setVisible (true);
    midiWindow->toFront (true);
}

void StarterAmpEditor::mouseDown (const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() || e.originalComponent == nullptr)
        return;
    auto* knob = dynamic_cast<KnobControl*> (e.originalComponent);
    if (knob == nullptr) knob = e.originalComponent->findParentComponentOfClass<KnobControl>();
    if (knob == nullptr)
        return;

    const int index = processorRef.midiIndexOf (knob->getParamId());
    if (index < 0)
        return;
    const int slot = processorRef.midiSlotFor (index);

    juce::PopupMenu menu;
    menu.addItem (1, "MIDI learn: then move a controller");
    if (slot >= 0)
        menu.addItem (2, "Clear MIDI mapping (" + StarterAmpProcessor::midiSlotName (slot) + ")");
    menu.addSeparator();
    menu.addItem (3, "Open MIDI window...");
    menu.showMenuAsync (juce::PopupMenu::Options(), [this, index, slot] (int result)
    {
        if (result == 1) processorRef.midiStartLearn (index);
        else if (result == 2) processorRef.midiUnbind (slot);
        else if (result == 3) openMidiWindow();
    });
}

void StarterAmpEditor::midiTick()
{
    const bool learning = processorRef.midiLearning() >= 0;
    const int activity = processorRef.midiActivity.load();
    const bool flash = activity != lastMidiActivity;
    lastMidiActivity = activity;
    const juce::String text = learning ? "LEARN..." : "MIDI";
    if (midiButton.getButtonText() != text) midiButton.setButtonText (text);
    midiButton.setColour (juce::TextButton::textColourOffId, (learning || flash) ? ui::accent : ui::text);

    // Each NAM strip's description follows its loaded model (a session load or a preset can change it)
    for (int s = 0; s < 4; ++s)
        if (processorRef.getNamSummary (s) != namStrip[(size_t) s].lastText)
            updateNamInfo (s);

    // Undo history, A/B and the tool buttons
    processorRef.undoTick (userBusy());
    undoButton.setEnabled (processorRef.canUndo());
    redoButton.setEnabled (processorRef.canRedo());
    abA.setToggleState (processorRef.abActive() == 0, juce::dontSendNotification);
    abB.setToggleState (processorRef.abActive() == 1, juce::dontSendNotification);
    metroButton.setToggleState (processorRef.apvts.getRawParameterValue ("metroOn")->load() > 0.5f, juce::dontSendNotification);

    // A Program Change loaded a preset: bring the window in step with it.
    const int loads = processorRef.presetLoadCounter.load();
    if (loads != lastPresetLoadCounter)
    {
        lastPresetLoadCounter = loads;
        afterPresetLoad ((int) processorRef.apvts.state.getProperty ("uiPage", 0));
        const int program = processorRef.midiLastProgram.load();
        if (program >= 0)
            presetBox.setSelectedId (1000 + program, juce::dontSendNotification);
    }
}

//==============================================================================
// ToolWindow
//==============================================================================
ToolWindow::ToolWindow (const juce::String& title, std::unique_ptr<juce::Component> content, juce::LookAndFeel& laf)
    : juce::DocumentWindow (title, ui::background, juce::DocumentWindow::closeButton)
{
    setLookAndFeel (&laf);
    content->setLookAndFeel (&laf);
    setUsingNativeTitleBar (true);
    setContentOwned (content.release(), true);
    setResizable (false, false);
    setAlwaysOnTop (true);
    centreWithSize (getWidth(), getHeight());
}

ToolWindow::~ToolWindow()
{
    if (auto* c = getContentComponent()) c->setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

//==============================================================================
// OrderPanel
//==============================================================================
namespace
{
    // The switch that says whether each reorderable block is on (a block can have several).
    bool fxBlockIsOn (StarterAmpProcessor& p, int group, int block)
    {
        static const std::vector<const char*> ids[2][6] =
        {
            { { "fuzzOn" }, { "compOn" }, { "wahOn" }, { "phaserOn" }, { "chorusOn" }, { "preDelayOn" } },
            { { "eqOn" }, { "tremOn", "wobOn", "swellOn", "granOn" }, { "dblOn" }, { "postDelayOn" }, { "reverbOn" }, { "widOn" } }
        };
        for (auto* id : ids[group][block])
            if (auto* v = p.apvts.getRawParameterValue (id))
                if (v->load() > 0.5f)
                    return true;
        return false;
    }
}

void OrderPanel::Model::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    const int block = processor.getFxOrder (group)[(size_t) juce::jlimit (0, StarterAmpProcessor::kFxBlocks - 1, row)];
    if (selected) { g.setColour (ui::accent.withAlpha (0.22f)); g.fillRect (0, 0, w, h); }
    const bool on = fxBlockIsOn (processor, group, block);
    const float cy = (float) h * 0.5f;
    if (on) { g.setColour (ui::accent.brighter (0.3f)); g.fillEllipse (12.0f, cy - 3.0f, 6.0f, 6.0f); }
    else    { g.setColour (ui::textDim.withAlpha (0.55f)); g.drawEllipse (12.0f, cy - 3.0f, 6.0f, 6.0f, 1.2f); }
    g.setColour (on ? ui::text : ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (14.0f, on ? juce::Font::bold : juce::Font::plain)));
    g.drawText (juce::String (row + 1) + ".  " + StarterAmpProcessor::fxName (group, block), 30, 0, w - 36, h, juce::Justification::centredLeft);
}

OrderPanel::OrderPanel (StarterAmpProcessor& p)
    : processor (p), models { { p, 0 }, { p, 1 } }
{
    setSize (600, 400);
    for (int g = 0; g < 2; ++g)
    {
        up[g].setButtonText ("MOVE UP");
        down[g].setButtonText ("MOVE DOWN");
        lists[g].setModel (&models[g]);
        lists[g].setRowHeight (30);
        lists[g].setColour (juce::ListBox::backgroundColourId, ui::panel);
        lists[g].selectRow (0);
        lists[g].setTooltip (g == 0 ? "The effects that come before the amp, in the order the signal passes through them (top first)."
                                    : "The effects that come after the cab, in the order the signal passes through them (top first).");
        addAndMakeVisible (lists[g]);
        lists[g].addMouseListener (this, true);
        up[g].setTooltip ("Move the selected effect earlier in the chain.");
        down[g].setTooltip ("Move the selected effect later in the chain.");
        up[g].onClick   = [this, g] { move (g, -1); };
        down[g].onClick = [this, g] { move (g, +1); };
        addAndMakeVisible (up[g]);
        addAndMakeVisible (down[g]);
    }
    resetButton.setTooltip ("Put both groups back in the standard order.");
    resetButton.onClick = [this] { processor.resetFxOrder(); lists[0].repaint(); lists[1].repaint(); };
    addAndMakeVisible (resetButton);
    notes.setText ("Drag an effect up or down to move it (or select it and use the buttons). Lit dots are effects that are switched on. Each effect keeps its settings when you move it. "
                   "The noise pedal, transpose, pitch pedal, octaver, overdrive, amp and cab stay where they are. "
                   "Moving effects while you play can cause a brief click.", juce::dontSendNotification);
    notes.setFont (juce::Font (juce::FontOptions (12.0f)));
    notes.setColour (juce::Label::textColourId, ui::textDim);
    notes.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (notes);
}

void OrderPanel::mouseDrag (const juce::MouseEvent& e)
{
    for (int g = 0; g < 2; ++g)
        if (e.originalComponent == &lists[g] || lists[g].isParentOf (e.originalComponent))
        {
            const auto p = e.getEventRelativeTo (&lists[g]).getPosition();
            const int row = lists[g].getSelectedRow();
            const int target = lists[g].getRowContainingPosition (p.x, p.y);
            if (row >= 0 && target >= 0 && target != row)
            {
                processor.moveFxBlock (g, row, target);
                lists[g].selectRow (target);
                lists[g].repaint();
            }
        }
}

void OrderPanel::move (int group, int delta)
{
    const int row = lists[group].getSelectedRow();
    if (row < 0) return;
    const int to = juce::jlimit (0, StarterAmpProcessor::kFxBlocks - 1, row + delta);
    processor.moveFxBlock (group, row, to);
    lists[group].selectRow (to);
    lists[group].repaint();
}

void OrderPanel::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText ("BEFORE THE AMP", 16, 14, 270, 16, juce::Justification::left);
    g.drawText ("AFTER THE CAB", 314, 14, 270, 16, juce::Justification::left);
}

void OrderPanel::resized()
{
    for (int g = 0; g < 2; ++g)
    {
        const int x = 16 + g * 298;
        lists[g].setBounds (x, 36, 270, 6 * 30 + 2);
        up[g].setBounds (x, 36 + 6 * 30 + 12, 130, 28);
        down[g].setBounds (x + 140, 36 + 6 * 30 + 12, 130, 28);
    }
    notes.setBounds (16, 290, 568, 56);
    resetButton.setBounds (16, 356, 140, 28);
}

//==============================================================================
// MetroPanel
//==============================================================================
MetroPanel::MetroPanel (juce::AudioProcessorValueTreeState& state)
{
    setSize (420, 290);
    auto label = [this] (juce::Label& l, const char* text) { l.setText (text, juce::dontSendNotification); l.setColour (juce::Label::textColourId, ui::textDim); addAndMakeVisible (l); };
    label (beatsLabel, "Beats per bar");  label (subLabel, "Subdivision");  label (soundLabel, "Sound");  label (volumeLabel, "Volume");

    onButton.setTooltip ("Starts the metronome. It is added to the output after all the effects, and runs at the plugin's tempo (the tempo box, tap tempo, or the host's tempo when HOST TEMPO is on and your DAW is playing).");
    accentButton.setTooltip ("Plays the first beat of each bar higher and louder.");
    beatsSlider.setTooltip ("Beats in a bar (1 to 12).");
    subBox.setTooltip ("Extra quieter clicks between the beats: none, eighth notes, triplets or sixteenth notes.");
    soundBox.setTooltip ("The click sound: a sharp Click, a Wood block, or a soft Beep.");
    volumeSlider.setTooltip ("How loud the metronome is, in dB.");
    for (auto* c : { (juce::Component*) &onButton, (juce::Component*) &accentButton, (juce::Component*) &subBox, (juce::Component*) &soundBox,
                     (juce::Component*) &beatsSlider, (juce::Component*) &volumeSlider })
        addAndMakeVisible (c);

    for (auto* s : { &beatsSlider, &volumeSlider })
    {
        s->setSliderStyle (juce::Slider::LinearHorizontal);
        s->setTextBoxStyle (juce::Slider::TextBoxRight, false, 64, 20);
    }
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter ("metroSub")))   subBox.addItemList (p->choices, 1);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter ("metroSound"))) soundBox.addItemList (p->choices, 1);
    onAttach     = std::make_unique<APVTS::ButtonAttachment> (state, "metroOn", onButton);
    accentAttach = std::make_unique<APVTS::ButtonAttachment> (state, "metroAccent", accentButton);
    subAttach    = std::make_unique<APVTS::ComboBoxAttachment> (state, "metroSub", subBox);
    soundAttach  = std::make_unique<APVTS::ComboBoxAttachment> (state, "metroSound", soundBox);
    beatsAttach  = std::make_unique<APVTS::SliderAttachment> (state, "metroBeats", beatsSlider);
    volumeAttach = std::make_unique<APVTS::SliderAttachment> (state, "metroVol", volumeSlider);
    volumeSlider.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " dB"; };
    volumeSlider.updateText();

    tempoNote.setText ("The tempo is the BPM box in the header (or tap tempo, or your DAW's tempo with HOST TEMPO on).", juce::dontSendNotification);
    tempoNote.setFont (juce::Font (juce::FontOptions (12.0f)));
    tempoNote.setColour (juce::Label::textColourId, ui::textDim);
    tempoNote.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (tempoNote);
}

void MetroPanel::paint (juce::Graphics& g) { g.fillAll (ui::background); }

void MetroPanel::resized()
{
    auto r = getLocalBounds().reduced (20);
    onButton.setBounds (r.removeFromTop (30));
    r.removeFromTop (8);
    auto row = [&r] (juce::Label& l, juce::Component& c)
    {
        auto line = r.removeFromTop (30);
        l.setBounds (line.removeFromLeft (120));
        c.setBounds (line);
        r.removeFromTop (6);
    };
    row (beatsLabel, beatsSlider);
    row (subLabel, subBox);
    row (soundLabel, soundBox);
    row (volumeLabel, volumeSlider);
    accentButton.setBounds (r.removeFromTop (30));
    r.removeFromTop (6);
    tempoNote.setBounds (r.removeFromTop (44));
}

//==============================================================================
// PresetBrowser
//==============================================================================
juce::String PresetBrowser::cleanTags (const juce::String& text)
{
    juce::StringArray out;
    juce::StringArray parts;
    parts.addTokens (text.replaceCharacters (";\n", ",,"), ",", "");
    for (auto p : parts)
    {
        p = p.trim().substring (0, 24);
        if (p.isNotEmpty() && ! out.contains (p, true))
            out.add (p);
        if (out.size() >= 12) break;
    }
    return out.joinIntoString (", ");
}

juce::String PresetBrowser::readTags (const juce::File& file, bool& damaged)
{
    damaged = true;
    juce::MemoryBlock data;
    if (! file.loadFileAsData (data) || data.getSize() < 16)
        return {};
    if (auto xml = juce::AudioProcessor::getXmlFromBinary (data.getData(), (int) data.getSize()))
        if (xml->hasTagName ("PARAMS"))
        {
            damaged = false;
            return cleanTags (xml->getStringAttribute ("presetTags"));
        }
    return {};
}

bool PresetBrowser::writeTags (const juce::File& file, const juce::String& tags)
{
    juce::MemoryBlock data;
    if (! file.loadFileAsData (data))
        return false;
    auto xml = juce::AudioProcessor::getXmlFromBinary (data.getData(), (int) data.getSize());
    if (xml == nullptr || ! xml->hasTagName ("PARAMS"))
        return false;
    xml->setAttribute ("presetTags", cleanTags (tags));
    juce::MemoryBlock out;
    juce::AudioProcessor::copyXmlToBinary (*xml, out);
    return file.replaceWithData (out.getData(), out.getSize());
}

PresetBrowser::PresetBrowser (StarterAmpProcessor& p, Callbacks callbacks) : processor (p), cb (std::move (callbacks))
{
    setSize (640, 520);
    for (auto* t : { &factoryTab, &userTab })
    {
        t->setClickingTogglesState (true);
        t->setRadioGroupId (4821);
        t->setComponentID ("tab");
        addAndMakeVisible (t);
    }
    factoryTab.setToggleState (true, juce::dontSendNotification);
    factoryTab.setTooltip ("The presets that come with the plugin, grouped by style.");
    userTab.setTooltip ("Presets you saved, stored as files in Documents/StarterAmp/Presets.");
    factoryTab.onClick = [this] { rebuildTagMenu(); rebuildView(); };
    userTab.onClick    = [this] { rebuildTagMenu(); rebuildView(); };

    search.setTextToShowWhenEmpty ("Search names and tags...", ui::textDim);
    search.setTooltip ("Type part of a preset name or tag to narrow the list.");
    search.onTextChange = [this] { rebuildView(); };
    addAndMakeVisible (search);
    tagBox.setTooltip ("Show only presets that have this tag.");
    tagBox.onChange = [this] { rebuildView(); };
    addAndMakeVisible (tagBox);

    list.setRowHeight (26);
    list.setColour (juce::ListBox::backgroundColourId, ui::panel);
    list.setTooltip ("Double-click a preset to load it. Damaged files are shown in red and cannot be loaded.");
    addAndMakeVisible (list);

    loadButton.setTooltip ("Load the selected preset.");
    saveButton.setTooltip ("Save the current sound as a user preset (you choose the name and tags).");
    renameButton.setTooltip ("Rename the selected user preset.");
    tagsButton.setTooltip ("Change the tags of the selected user preset. Separate tags with commas.");
    deleteButton.setTooltip ("Delete the selected user preset file. This cannot be undone.");
    folderButton.setTooltip ("Open the folder that holds your user presets.");
    for (auto* b : { &loadButton, &saveButton, &renameButton, &tagsButton, &deleteButton, &folderButton })
        addAndMakeVisible (b);
    status.setColour (juce::Label::textColourId, ui::textDim);
    status.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (status);

    loadButton.onClick = [this] { loadRow (list.getSelectedRow()); };
    saveButton.onClick = [this] { if (cb.saveCurrent) cb.saveCurrent(); };
    folderButton.onClick = [this] { auto f = cb.folder(); f.createDirectory(); f.revealToUser(); };

    renameButton.onClick = [this]
    {
        const auto* e = selectedEntry();
        if (e == nullptr || e->factoryIndex >= 0) return;
        auto* w = new juce::AlertWindow ("Rename preset", "New name:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", e->name);
        w->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        const auto file = e->file;
        juce::Component::SafePointer<PresetBrowser> safe (this);
        w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, file] (int result)
        {
            if (result != 1 || safe == nullptr) return;
            const auto name = juce::File::createLegalFileName (w->getTextEditorContents ("name").trim());
            if (name.isEmpty()) return;
            const auto target = file.getParentDirectory().getChildFile (name + ".starteramp");
            if (target == file) return;
            if (target.exists()) { safe->say ("A preset called \"" + name + "\" already exists. Choose another name."); return; }
            if (! file.moveFileTo (target)) { safe->say ("Could not rename the file."); return; }
            safe->refresh();
            if (safe->cb.changed) safe->cb.changed();
        }), true);
    };
    tagsButton.onClick = [this]
    {
        const auto* e = selectedEntry();
        if (e == nullptr || e->factoryIndex >= 0 || e->damaged) return;
        auto* w = new juce::AlertWindow ("Edit tags", "Tags, separated by commas:", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("tags", e->tags);
        w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        const auto file = e->file;
        juce::Component::SafePointer<PresetBrowser> safe (this);
        w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w, file] (int result)
        {
            if (result != 1 || safe == nullptr) return;
            if (! writeTags (file, w->getTextEditorContents ("tags"))) safe->say ("Could not change the tags of that file.");
            safe->refresh();
        }), true);
    };
    deleteButton.onClick = [this]
    {
        const auto* e = selectedEntry();
        if (e == nullptr || e->factoryIndex >= 0) return;
        const auto file = e->file;
        juce::Component::SafePointer<PresetBrowser> safe (this);
        juce::AlertWindow::showAsync (juce::MessageBoxOptions().withIconType (juce::MessageBoxIconType::QuestionIcon)
                                          .withTitle ("Delete preset")
                                          .withMessage ("Delete \"" + file.getFileNameWithoutExtension() + "\"? This cannot be undone.")
                                          .withButton ("Delete").withButton ("Cancel"),
                                      [safe, file] (int result)
                                      {
                                          if (result != 1 || safe == nullptr) return;
                                          if (! file.deleteFile()) safe->say ("Could not delete that file.");
                                          safe->refresh();
                                          if (safe->cb.changed) safe->cb.changed();
                                      });
    };

    for (int i = 0; i < StarterAmpProcessor::factoryPresetCount(); ++i)
    {
        Entry e;
        e.name = StarterAmpProcessor::factoryPresetName (i);
        e.tags = StarterAmpProcessor::factoryPresetTags (i).joinIntoString (", ");
        e.factoryIndex = i;
        factoryEntries.push_back (e);
    }
    refresh();
}

void PresetBrowser::refresh()
{
    userEntries.clear();
    int damaged = 0;
    const auto folder = cb.folder();
    if (folder.isDirectory())
    {
        auto files = folder.findChildFiles (juce::File::findFiles, false, "*.starteramp");
        files.sort();
        for (const auto& f : files)
        {
            Entry e;
            e.name = f.getFileNameWithoutExtension();
            e.file = f;
            e.tags = readTags (f, e.damaged);
            if (e.damaged) ++damaged;
            userEntries.push_back (e);
        }
    }
    rebuildTagMenu();
    rebuildView();
    if (damaged > 0)
        say (juce::String (damaged) + (damaged == 1 ? " damaged file" : " damaged files") + " in the user folder (shown in red).");
}

void PresetBrowser::rebuildTagMenu()
{
    const auto previous = tagBox.getSelectedId() > 1 ? tagBox.getText() : juce::String();
    tagChoices.clear();
    for (const auto& e : onUserTab() ? userEntries : factoryEntries)
    {
        juce::StringArray t;
        t.addTokens (e.tags, ",", "");
        for (auto s : t) { s = s.trim(); if (s.isNotEmpty()) tagChoices.addIfNotAlreadyThere (s, true); }
    }
    tagChoices.sortNatural();
    tagBox.clear (juce::dontSendNotification);
    tagBox.addItem ("All tags", 1);
    int id = 2, select = 1;
    for (auto& t : tagChoices)
    {
        tagBox.addItem (t, id);
        if (t.equalsIgnoreCase (previous)) select = id;
        ++id;
    }
    tagBox.setSelectedId (select, juce::dontSendNotification);
}

void PresetBrowser::rebuildView()
{
    const auto& source = onUserTab() ? userEntries : factoryEntries;
    const auto wantedTag = tagBox.getSelectedId() > 1 ? tagBox.getText() : juce::String();
    const auto text = search.getText().trim();
    const auto keep = selectedEntry();
    const juce::String keepName = keep != nullptr ? keep->name : juce::String();
    view.clear();
    for (int i = 0; i < (int) source.size(); ++i)
    {
        const auto& e = source[(size_t) i];
        if (wantedTag.isNotEmpty())
        {
            juce::StringArray t;
            t.addTokens (e.tags, ",", "");
            bool has = false;
            for (auto s : t) has = has || s.trim().equalsIgnoreCase (wantedTag);
            if (! has) continue;
        }
        if (text.isNotEmpty() && ! e.name.containsIgnoreCase (text) && ! e.tags.containsIgnoreCase (text))
            continue;
        view.push_back (i);
    }
    list.updateContent();
    list.deselectAllRows();
    for (size_t i = 0; i < view.size(); ++i)
        if (source[(size_t) view[i]].name == keepName) { list.selectRow ((int) i); break; }
    list.repaint();
    updateButtons();
    say (juce::String ((int) view.size()) + " of " + juce::String ((int) source.size()) + " presets");
}

const PresetBrowser::Entry* PresetBrowser::selectedEntry() const
{
    const int row = list.getSelectedRow();
    if (! juce::isPositiveAndBelow (row, (int) view.size())) return nullptr;
    const auto& source = onUserTab() ? userEntries : factoryEntries;
    return &source[(size_t) view[(size_t) row]];
}

void PresetBrowser::updateButtons()
{
    const auto* e = selectedEntry();
    const bool user = onUserTab();
    loadButton.setEnabled (e != nullptr && ! e->damaged);
    renameButton.setEnabled (user && e != nullptr);
    tagsButton.setEnabled (user && e != nullptr && ! e->damaged);
    deleteButton.setEnabled (user && e != nullptr);
    renameButton.setVisible (user);  tagsButton.setVisible (user);  deleteButton.setVisible (user);
    folderButton.setVisible (user);  saveButton.setVisible (user);
}

void PresetBrowser::loadRow (int row)
{
    if (! juce::isPositiveAndBelow (row, (int) view.size())) return;
    const auto& source = onUserTab() ? userEntries : factoryEntries;
    const auto& e = source[(size_t) view[(size_t) row]];
    if (e.damaged) { say ("That file is damaged and cannot be loaded."); return; }
    if (e.factoryIndex >= 0) { if (cb.loadFactory) cb.loadFactory (e.factoryIndex); }
    else if (cb.loadFile) cb.loadFile (e.file);
}

void PresetBrowser::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) view.size())) return;
    const auto& source = onUserTab() ? userEntries : factoryEntries;
    const auto& e = source[(size_t) view[(size_t) row]];
    if (selected) { g.setColour (ui::accent.withAlpha (0.22f)); g.fillRect (0, 0, w, h); }
    g.setColour (e.damaged ? juce::Colour (0xffff6b5e) : ui::text);
    g.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::bold)));
    g.drawText (e.name + (e.damaged ? "  (damaged)" : ""), 10, 0, w / 2 - 10, h, juce::Justification::centredLeft);
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawFittedText (e.tags, w / 2, 0, w / 2 - 10, h, juce::Justification::centredRight, 1);
}

void PresetBrowser::paint (juce::Graphics& g) { g.fillAll (ui::background); }

void PresetBrowser::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto tabs = r.removeFromTop (28);
    factoryTab.setBounds (tabs.removeFromLeft (110));  tabs.removeFromLeft (6);
    userTab.setBounds (tabs.removeFromLeft (110));
    r.removeFromTop (10);
    auto filters = r.removeFromTop (28);
    tagBox.setBounds (filters.removeFromRight (180));  filters.removeFromRight (8);
    search.setBounds (filters);
    r.removeFromTop (10);
    auto bottom = r.removeFromBottom (28);
    status.setBounds (r.removeFromBottom (22));
    r.removeFromBottom (4);
    list.setBounds (r);
    loadButton.setBounds (bottom.removeFromLeft (80));  bottom.removeFromLeft (6);
    saveButton.setBounds (bottom.removeFromLeft (130)); bottom.removeFromLeft (6);
    renameButton.setBounds (bottom.removeFromLeft (80)); bottom.removeFromLeft (6);
    tagsButton.setBounds (bottom.removeFromLeft (90));  bottom.removeFromLeft (6);
    deleteButton.setBounds (bottom.removeFromLeft (80));
    folderButton.setBounds (bottom.removeFromRight (120));
}

StarterAmpEditor::~StarterAmpEditor()
{
    stopTimer();
    midiWatcher.onTick = nullptr;
    midiWindow.reset();
    orderWindow.reset();
    metroWindow.reset();
    browserWindow.reset();
    libraryWindow.reset();
    toneWindow.reset();
    toneLabWindow.reset();

    // If the window closes while a momentary button is held, let go of the switch.
    // (A latched switch stays on, because that is its saved state.)
    for (auto [id, latchId] : { std::pair<const char*, const char*> { "noiseOct",   "noiseOctLatch" },
                                std::pair<const char*, const char*> { "noiseOct2",  "noiseOct2Latch" },
                                std::pair<const char*, const char*> { "noiseGross", "noiseGrossLatch" },
                                std::pair<const char*, const char*> { "noiseRing",  "noiseRingLatch" } })
        if (! isLatched (latchId))
            setSwitch (id, false);

    processorRef.tunerActive.store (false);
    setLookAndFeel (nullptr);
}

void StarterAmpEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
}

void StarterAmpEditor::resized()
{
    const float scale = std::min ((float) getWidth()  / (float) ui::kWidth,
                                  (float) getHeight() / (float) ui::kHeight);
    content.setTransform (juce::AffineTransform::scale (scale));
}

//==============================================================================
// Helpers
//==============================================================================
void StarterAmpEditor::addKnob (Panel& panel, const char* paramId, const juce::String& name,
                                const juce::String& suffix, int decimals, int page)
{
    knobs.push_back (std::make_unique<KnobControl> (processorRef.apvts, paramId, name, suffix, decimals));
    idMap[paramId] = knobs.back().get();
    panel.addKnob (*knobs.back(), page);
}

void StarterAmpEditor::addAdvKnob (Panel& panel, const char* paramId, const juce::String& name,
                                   const juce::String& suffix, int decimals, int page)
{
    addKnob (panel, paramId, name, suffix, decimals, page);
    panel.markAdvanced (knobs.back().get());
}

// A panel's BASIC / ADVANCED choice is remembered with the session (and re-applied after a preset replaces the state).
void StarterAmpEditor::setupAdvanced (Panel& panel, const char* key)
{
    const juce::Identifier prop ("adv_" + juce::String (key));
    panel.setAdvancedShown ((bool) processorRef.apvts.state.getProperty (prop, false));
    panel.onAdvancedChanged = [this, prop] (bool on) { processorRef.apvts.state.setProperty (prop, on, nullptr); };
    advancedPanels.emplace_back (&panel, juce::String (key));
}

juce::ToggleButton& StarterAmpEditor::makeToggle (const char* paramId, const juce::String& text)
{
    auto t = std::make_unique<ToggleControl>();
    t->button.setButtonText (text);
    t->button.setTooltip (tipFor (processorRef.apvts, paramId));
    t->attachment = std::make_unique<APVTS::ButtonAttachment> (processorRef.apvts, paramId, t->button);
    idMap[paramId] = &t->button;
    toggles.push_back (std::move (t));
    return toggles.back()->button;
}

juce::ComboBox& StarterAmpEditor::makeCombo (const char* paramId)
{
    auto c = std::make_unique<ComboControl>();

    // The menu items come from the parameter's own list of choices.
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (processorRef.apvts.getParameter (paramId)))
        c->box.addItemList (choice->choices, 1);

    c->box.setJustificationType (juce::Justification::centred);
    c->box.paramId = paramId;
    c->box.setTooltip (tipFor (processorRef.apvts, paramId));
    c->attachment = std::make_unique<APVTS::ComboBoxAttachment> (processorRef.apvts, paramId, c->box);
    idMap[paramId] = &c->box;
    combos.push_back (std::move (c));
    return combos.back()->box;
}

bool StarterAmpEditor::isLatched (const char* latchId) const
{
    return processorRef.apvts.getRawParameterValue (latchId)->load() > 0.5f;
}

void StarterAmpEditor::setSwitch (const char* paramId, bool on)
{
    if (auto* p = processorRef.apvts.getParameter (paramId))
        if (on != (p->getValue() > 0.5f))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
}

// A footswitch with two modes, chosen by its LATCH button:
//   momentary : on while the mouse button is held down
//   latching  : each click flips it on or off
// The button is drawn lit whenever the parameter is on (held, latched, or
// switched by DAW automation).
void StarterAmpEditor::setupSwitch (juce::TextButton& button, const char* paramId, const char* latchId,
                                    std::unique_ptr<juce::ParameterAttachment>& attachment)
{
    button.onStateChange = [this, &button, paramId, latchId]
    {
        if (! isLatched (latchId))
            setSwitch (paramId, button.isDown());
    };

    button.onClick = [this, paramId, latchId]
    {
        if (isLatched (latchId))
            if (auto* p = processorRef.apvts.getParameter (paramId))
                setSwitch (paramId, p->getValue() < 0.5f);
    };

    if (auto* param = processorRef.apvts.getParameter (paramId))
    {
        attachment = std::make_unique<juce::ParameterAttachment> (
            *param, [&button] (float value) { button.setToggleState (value > 0.5f, juce::dontSendNotification); });
        attachment->sendInitialUpdate();
    }
}

// The amp menus are grouped by gain level into folders (v62): Clean, Low / mid gain, High gain. The menu order does not
// match the parameter's values, so these menus are attached by hand: the item id is the parameter's index + 1.
namespace
{
    // Amp model values (the ampModel parameter) in each folder, quietest first.
    const int kCleanAmps[]   = { 10, 0, 1 };                    // Glass Clean, Clean, Vintage Chime
    const int kMidAmps[]     = { 9, 7, 6, 15, 3 };              // Low-Watt Combo, Boutique Lead, Classic Rock, Master-Volume Rock, Saturated Lead
    const int kHighAmps[]    = { 2, 11, 4, 8, 13, 14, 12 };     // Hot-Rod, Creamy Lead, Modern High-Gain, Tight Metal, Precision, Digital Extreme, Doom Stack

    void buildAmpFolders (juce::ComboBox& box, const juce::StringArray& choices, bool withAcoustic)
    {
        // Amp B's list has no Acoustic entry, so its parameter index is one lower than the amp value above Acoustic.
        auto indexOf = [withAcoustic] (int value) { return (withAcoustic || value < 5) ? value : value - 1; };
        auto add = [&] (juce::PopupMenu& menu, const int* values, int n)
        {
            for (int k = 0; k < n; ++k)
            {
                const int idx = indexOf (values[k]);
                if (idx >= 0 && idx < choices.size())
                    menu.addItem (idx + 1, choices[idx]);
            }
        };
        auto* root = box.getRootMenu();
        if (withAcoustic)
        {
            root->addItem (5 + 1, choices[5]);
            root->addSeparator();
        }
        juce::PopupMenu clean, mid, high;
        add (clean, kCleanAmps, (int) (sizeof (kCleanAmps) / sizeof (int)));
        add (mid,   kMidAmps,   (int) (sizeof (kMidAmps)   / sizeof (int)));
        add (high,  kHighAmps,  (int) (sizeof (kHighAmps)  / sizeof (int)));
        root->addSubMenu ("Clean", clean);
        root->addSubMenu ("Low / mid gain", mid);
        root->addSubMenu ("High gain", high);
    }
}

juce::ComboBox& StarterAmpEditor::makeAmpModelCombo (const char* paramId)
{
    auto c = std::make_unique<ComboControl>();
    auto* param = processorRef.apvts.getParameter (paramId);
    const bool isA = juce::String (paramId) == "ampModel";

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
        buildAmpFolders (c->box, choice->choices, isA);

    c->box.setJustificationType (juce::Justification::centred);
    c->box.paramId = paramId;
    c->box.setTooltip (tipFor (processorRef.apvts, paramId));

    auto* box = &c->box;
    auto* holder = c.get();
    c->paramAttach = std::make_unique<juce::ParameterAttachment> (*param, [this, box] (float v)
    {
        box->setSelectedId ((int) std::lround (v) + 1, juce::dontSendNotification);
        syncVisibility();
    });
    c->box.onChange = [holder, box] { holder->paramAttach->setValueAsCompleteGesture ((float) (box->getSelectedId() - 1)); };
    c->paramAttach->sendInitialUpdate();

    if (! isA) idMap[paramId] = &c->box;
    combos.push_back (std::move (c));
    return combos.back()->box;
}

// The built-in cab speaker menu is grouped into folders (v68; the Classic folder is new in v69). The menu order does not match the parameter's values, so
// like the amp menu it is attached by hand: the item id is the parameter's index + 1.
juce::ComboBox& StarterAmpEditor::makeSpeakerCombo (const char* paramId)
{
    auto c = std::make_unique<ComboControl>();
    auto* param = processorRef.apvts.getParameter (paramId);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        // Realistic = the hand-built speaker models; Mix ready = the ones fitted to mixed (EQ'd) recordings.
        // Then by cab type. The types say what each is typically used for; any speaker works with any Cabinet.
        static const int kR1x12[]   = { 4, 5, 10 };                 // Chimey Alnico, Vintage American, Hi-Def Light Cone
        static const int kR2x12[]   = { 3, 6, 11 };                 // Smooth Vintage, Hi-Fi Full, Smooth Lead
        static const int kR4x12[]   = { 0, 1, 2, 8, 12 };           // Modern Tight, Vintage Crunch, Rock Bite, Tight Ceramic, Mid Forward
        static const int kROver[]   = { 7, 9 };                     // Modern Scoop, Dark Thump
        // v69 numbering: 13 Studio Standard, 14 Studio Smooth Tight, 15 Heavy Bright, 16 Heavy Tight, 17 Studio Hollow, 18 Rhythm Dark,
        // 19 Rhythm Smooth, 20 to 25 Compact ..., 26 to 30 more Studio / Heavy, 31 to 34 Classic ...
        static const int kM4x12[]   = { 13, 14, 17, 26, 29, 27, 30, 15, 16, 28, 18, 19 };   // Studio ..., Heavy ..., Rhythm ...
        static const int kMSmall[]  = { 20, 21, 22, 23, 24, 25 };   // Compact ...
        static const int kClassic[] = { 31, 32, 33, 34, 35 };       // Classic ... (35 = Classic Warm Presence, v71)
        static const int kRaw[]     = { 36, 37 };                   // Raw Big Low, Raw Bright Cap (v71)
        auto* root = c->box.getRootMenu();
        auto fill = [&] (juce::PopupMenu& menu, const char* folder, const int* values, int n)
        {
            juce::PopupMenu sub;
            for (int k = 0; k < n; ++k)
                if (values[k] < choice->choices.size())
                    sub.addItem (values[k] + 1, choice->choices[values[k]]);
            menu.addSubMenu (folder, sub);
        };
        juce::PopupMenu realistic, mixReady;
        fill (realistic, "1x12",           kR1x12,  (int) (sizeof (kR1x12)  / sizeof (int)));
        fill (realistic, "2x12",           kR2x12,  (int) (sizeof (kR2x12)  / sizeof (int)));
        fill (realistic, "4x12",           kR4x12,  (int) (sizeof (kR4x12)  / sizeof (int)));
        fill (realistic, "4x12 oversized", kROver,  (int) (sizeof (kROver)  / sizeof (int)));
        fill (realistic, "Classic (older styles)", kClassic, (int) (sizeof (kClassic) / sizeof (int)));
        fill (realistic, "Raw (bold)",     kRaw,    (int) (sizeof (kRaw)    / sizeof (int)));
        fill (mixReady,  "4x12",           kM4x12,  (int) (sizeof (kM4x12)  / sizeof (int)));
        fill (mixReady,  "Compact cabs",   kMSmall, (int) (sizeof (kMSmall) / sizeof (int)));
        root->addSubMenu ("Realistic", realistic);
        root->addSubMenu ("Mix ready", mixReady);
    }

    c->box.setJustificationType (juce::Justification::centred);
    c->box.paramId = paramId;
    c->box.setTooltip (tipFor (processorRef.apvts, paramId));

    auto* box = &c->box;
    auto* holder = c.get();
    c->paramAttach = std::make_unique<juce::ParameterAttachment> (*param, [box] (float v)
    {
        box->setSelectedId ((int) std::lround (v) + 1, juce::dontSendNotification);
    });
    c->box.onChange = [holder, box] { holder->paramAttach->setValueAsCompleteGesture ((float) (box->getSelectedId() - 1)); };
    c->paramAttach->sendInitialUpdate();

    idMap[paramId] = &c->box;
    combos.push_back (std::move (c));
    return combos.back()->box;
}

juce::ComboBox& StarterAmpEditor::makeMicCombo (const char* paramId)
{
    auto c = std::make_unique<ComboControl>();
    auto* param = processorRef.apvts.getParameter (paramId);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        // v71: the first twelve mics are the hand-built ones; the seven added in v71 were fitted to recordings.
        auto* root = c->box.getRootMenu();
        juce::PopupMenu handBuilt, measured;
        for (int k = 0; k < choice->choices.size(); ++k)
            (k < 12 ? handBuilt : measured).addItem (k + 1, choice->choices[k]);
        root->addSubMenu ("Hand-built", handBuilt);
        root->addSubMenu ("Measured", measured);
    }

    c->box.setJustificationType (juce::Justification::centred);
    c->box.paramId = paramId;
    c->box.setTooltip (tipFor (processorRef.apvts, paramId));

    auto* box = &c->box;
    auto* holder = c.get();
    c->paramAttach = std::make_unique<juce::ParameterAttachment> (*param, [box] (float v)
    {
        box->setSelectedId ((int) std::lround (v) + 1, juce::dontSendNotification);
    });
    c->box.onChange = [holder, box] { holder->paramAttach->setValueAsCompleteGesture ((float) (box->getSelectedId() - 1)); };
    c->paramAttach->sendInitialUpdate();

    idMap[paramId] = &c->box;
    combos.push_back (std::move (c));
    return combos.back()->box;
}

// The cabinet menu is grouped by speaker count (v69). Like the speaker menu it is attached by hand: the item id is the
// parameter's index + 1, so older sessions keep their cabinet.
juce::ComboBox& StarterAmpEditor::makeCabinetCombo (const char* paramId)
{
    auto c = std::make_unique<ComboControl>();
    auto* param = processorRef.apvts.getParameter (paramId);

    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (param))
    {
        static const int k1x[]   = { 0, 9, 10, 13, 14 };   // 1x12 Open, 1x12 Closed, 1x10 Open, 1x15 Closed, 1x8 Small
        static const int k2x[]   = { 1, 2, 11 };           // 2x12 Open, 2x12 Closed, 2x10 Open
        static const int k4x[]   = { 3, 4, 5, 6, 12 };     // 4x12 Closed, Straight, Angled, Open Back, 4x10 Closed
        static const int kOver[] = { 7, 8, 15 };           // Oversized (ported): 4x12 Straight, 4x12 Angled, 2x12
        auto* root = c->box.getRootMenu();
        auto fill = [&] (const char* folder, const int* values, int n)
        {
            juce::PopupMenu sub;
            for (int k = 0; k < n; ++k)
                if (values[k] < choice->choices.size())
                    sub.addItem (values[k] + 1, choice->choices[values[k]]);
            root->addSubMenu (folder, sub);
        };
        fill ("1x",        k1x,   (int) (sizeof (k1x)   / sizeof (int)));
        fill ("2x",        k2x,   (int) (sizeof (k2x)   / sizeof (int)));
        fill ("4x",        k4x,   (int) (sizeof (k4x)   / sizeof (int)));
        fill ("Oversized", kOver, (int) (sizeof (kOver) / sizeof (int)));
    }

    c->box.setJustificationType (juce::Justification::centred);
    c->box.paramId = paramId;
    c->box.setTooltip (tipFor (processorRef.apvts, paramId));

    auto* box = &c->box;
    auto* holder = c.get();
    c->paramAttach = std::make_unique<juce::ParameterAttachment> (*param, [box] (float v)
    {
        box->setSelectedId ((int) std::lround (v) + 1, juce::dontSendNotification);
    });
    c->box.onChange = [holder, box] { holder->paramAttach->setValueAsCompleteGesture ((float) (box->getSelectedId() - 1)); };
    c->paramAttach->sendInitialUpdate();

    idMap[paramId] = &c->box;
    combos.push_back (std::move (c));
    return combos.back()->box;
}

// Shows only the controls that do something for the current settings: the acoustic panel or the
// electric amp controls, Focus on the two amps that use it, the fuzz Gate / Stab on the gated style,
// and the shimmer controls while Shimmer is on.
void StarterAmpEditor::syncVisibility()
{
    auto& vts = processorRef.apvts;
    auto value = [&vts] (const char* id) { return vts.getRawParameterValue (id)->load(); };

    const int  model    = (int) value ("ampModel");
    const bool acoustic = model == 5;

    ampPanel.setHidden (voicingToggle, acoustic);
    ampPanel.setHidden (ampQualityBox, acoustic);          // the acoustic model has no amp stage to oversample
    for (auto* k : electricAmpKnobs)
        ampPanel.setHidden (k, acoustic);
    for (auto* k : ampRealismComps)
        ampPanel.setHidden (k, acoustic);
    ampPanel.setAdvancedAvailable (! acoustic);
    ampPanel.setHidden (masterKnob, true);
    ampPanel.setHidden (focusKnob, acoustic || ! (model == 3 || model == 4 || model == 8 || model == 11 || model == 13 || model == 14));

    // Tabs: Amp A or Amp B (the parallel amp is for the electric amps only, so the acoustic model stays on A)
    const bool onB = ! acoustic && ampTab == 1;
    const bool onNam = ! acoustic && ampTab == 2;
    ampTabB.setEnabled (! acoustic);
    ampTabN.setEnabled (! acoustic);
    (onNam ? ampTabN : onB ? ampTabB : ampTabA).setToggleState (true, juce::dontSendNotification);
    {
        bool namRunning = false;
        for (const char* id : { "namOn", "namBOn", "nmP1On", "nmP2On" })
            if (value (id) > 0.5f)
                namRunning = true;
        const juce::String nText = (! acoustic && namRunning) ? "NAM  (ON)" : "NAM";
        if (ampTabN.getButtonText() != nText)
            ampTabN.setButtonText (nText);
        for (auto& ns : namStrip)
        {
            if (onNam && ! ns.panel.isVisible())
                ns.panel.resized();
            ns.panel.setVisible (onNam);
        }
    }
    {
        const juce::String bText = (! acoustic && value ("ampBOn") > 0.5f) ? "AMP B  (ON)" : "AMP B";
        if (ampTabB.getButtonText() != bText)
            ampTabB.setButtonText (bText);
    }
    if (! onB && ! onNam && ! ampPanel.isVisible())
        ampPanel.resized();
    ampPanel.setVisible (! onB && ! onNam);
    if (onB && ! ampBPanel.isVisible())
        ampBPanel.resized();
    ampBPanel.setVisible (onB);

    if (acoustic && ! acousticPanel.isVisible())
        acousticPanel.resized();                 // make sure its controls are laid out when it appears
    acousticPanel.setVisible (acoustic);

    // The amp panels and the cab blocks wear the look of the selected amp, and the whole window takes its accent
    {
        const int themeIndex = themeForModel (model);
        applyAccent (juce::Colour (ampThemeFor (themeIndex).tick));
        auto* modelParam = dynamic_cast<juce::AudioParameterChoice*> (vts.getParameter ("ampModel"));
        const juce::String modelName = modelParam != nullptr ? modelParam->getCurrentChoiceName() : juce::String();
        static const char* const shortNames[16] = { "Clean", "Vintage", "Hot-Rod", "Lead", "Modern", "Acoustic", "Classic Rock", "Boutique", "Tight Metal", "Low-Watt", "Glass", "Creamy", "Doom", "Precision", "Digital", "MV Rock" };
        chainStrip.setModelName (value ("namOn") > 0.5f && model != 5 && processorRef.namHasModel() ? juce::String ("NAM") : juce::String (shortNames[juce::jlimit (0, 15, model)]));
        auto* modelBParam = dynamic_cast<juce::AudioParameterChoice*> (vts.getParameter ("ampBModel"));
        const juce::String modelBName = modelBParam != nullptr ? modelBParam->getCurrentChoiceName() : juce::String();
        ampArtPanel.setTheme (onB ? juce::jlimit (0, 14, (int) value ("ampBModel")) : themeIndex);
        ampArtPanel.setTitle ((onB ? modelBName : modelName).upToFirstOccurrenceOf (" (", false, false).toUpperCase());
        ampHero.setLook (onB ? juce::jlimit (0, 14, (int) value ("ampBModel")) : themeIndex,
                         (onB ? modelBName : modelName).upToFirstOccurrenceOf (" (", false, false).toUpperCase());
        ampPanel.setTheme (themeIndex);
        metalPanel.setTheme (themeIndex);
        ampBPanel.setTheme (juce::jlimit (0, 14, (int) value ("ampBModel")));
        for (auto* p : irPanels)
            if (p != nullptr)
                p->setTheme (themeIndex);
    }
    ampHero.setBounds (1010, 32, 470, 484);
    ampHero.setVisible (! acoustic && ! onNam);
    const bool metal = ((model >= 2 && model <= 4) || model == 8 || model == 11 || model == 13 || model == 14) && ! onB && ! onNam;
    const juce::Rectangle<int> metalBounds (0, 300, 1000, 190);
    if (metalPanel.getBounds() != metalBounds)
        metalPanel.setBounds (metalBounds);
    if (metal && ! metalPanel.isVisible())
        metalPanel.resized();                    // make sure its controls are laid out when it appears
    metalPanel.setVisible (metal);
    {   // the grille cloth fills the space under the amp's controls when no other panel does
        const juce::Rectangle<int> artBounds (0, 300, 1000, 220);
        if (ampArtPanel.getBounds() != artBounds)
            ampArtPanel.setBounds (artBounds);
        ampArtPanel.setVisible (! metal && ! acoustic);   // (on the Amp B tab it wears Amp B's faceplate)
    }
    // Acoustic: the amp panel shrinks to the model menu and the four tone knobs, and the acoustic panel moves up under it.
    const auto ampBounds = acoustic ? juce::Rectangle<int> (0, 32, 1000, 150) : juce::Rectangle<int> (0, 32, 1000, 260);
    if (ampPanel.getBounds() != ampBounds)
        ampPanel.setBounds (ampBounds);
    const juce::Rectangle<int> acBounds (0, 190, 1000, 220);
    if (acousticPanel.getBounds() != acBounds)
        acousticPanel.setBounds (acBounds);

    const bool customWow = (int) value ("wowMode") == 6;
    wowPanel.setHidden (wowHeelKnob, ! customWow);       // Heel and Toe only matter in Custom mode
    wowPanel.setHidden (wowToeKnob, ! customWow);

    for (int w = 0; w < 2; ++w)
    {
        Panel& fp = w == 0 ? fuzzPanel : fuzz2Panel;
        auto& c = fzc[w];
        const int fuzzStyle = (int) value (w == 0 ? "fuzzStyle" : "fuzz2Style");
        const bool gated = fuzzStyle == 2 || fuzzStyle == 6;
        const bool trio = fuzzStyle >= 4 && fuzzStyle <= 6;      // Face, Bender, Factory
        const bool wall = fuzzStyle == 0;
        fp.setHidden (c.gate, ! gated);
        fp.setHidden (c.stab, ! gated);
        fp.setHidden (c.ge, ! trio);
        fp.setHidden (c.opAmp, ! wall);
        fp.setHidden (c.version, ! wall);
        fp.setHidden (c.model, fuzzStyle != 5);
        fp.setHidden (c.diode, ! wall);
        fp.setHidden (c.tonePass, ! wall);
        fp.setHidden (c.buffer, ! trio);
        fp.setHidden (c.comp, fuzzStyle != 6);
        fp.setHidden (c.starve, fuzzStyle != 4 && fuzzStyle != 5);
        fp.setHidden (c.input, ! trio);
        fp.setHidden (c.mids, ! wall);
        fp.setHidden (c.shape, fuzzStyle != 7);
        fp.setHidden (c.crush, fuzzStyle != 7);
        fp.setHidden (c.bits, fuzzStyle != 8);
        fp.setHidden (c.rate, fuzzStyle != 8);
        fp.setHidden (c.tone, fuzzStyle == 3);
        fp.setHidden (c.buzzLow, fuzzStyle != 3);
        fp.setHidden (c.buzzHigh, fuzzStyle != 3);
        fp.setHidden (c.buzzMid, fuzzStyle != 3);
        fp.setHidden (c.buzzBlend, fuzzStyle != 3);
        fp.setHidden (c.buzzSoft, fuzzStyle != 3);
        fp.setHidden (c.buzzHard, fuzzStyle != 3);
        fp.setHidden (c.buzzPol, fuzzStyle != 3);
    }
    const int delayStereo = (int) value ("postDelayStereo");
    postDelayPanel.setHidden (delayOffsetKnob, delayStereo != 3);     // only the Dual mode has a separate right time
    postDelayPanel.setHidden (delaySpreadKnob, delayStereo == 0);     // Stereo mode has nothing to spread

    const bool limiterOn = value ("limOn") > 0.5f;
    outputPanel.setHidden (limCeilKnob, ! limiterOn);
    outputPanel.setHidden (limRelKnob, ! limiterOn);


    const bool shimmer = value ("shimmerOn") > 0.5f;
    reverbPanel.setHidden (shimmerPitchBox, ! shimmer);
    reverbPanel.setHidden (shimmerAmountKnob, ! shimmer);
    reverbPanel.setHidden (shimmerToneKnob, ! shimmer);
    reverbPanel.setHidden (shimmerDetuneKnob, ! shimmer);
    reverbPanel.setHidden (shimmerModeBox, ! shimmer);

    syncVoicingLabel();
}

// Shows the Transparent or Tight controls to match the Style menu.
void StarterAmpEditor::syncOverdrivePage()
{
    overdrivePanel.setPage (juce::jlimit (0, 7, (int) std::lround (processorRef.apvts.getRawParameterValue ("odStyle")->load())));
    drive2Panel.setPage (juce::jlimit (0, 7, (int) std::lround (processorRef.apvts.getRawParameterValue ("od2Style")->load())));
}

// Shows the model controls or the file button of each IR slot to match its Source menu.
// With a note value chosen, the delay takes its time from the tempo, so its Time knob is greyed out.
void StarterAmpEditor::syncDelayKnobs()
{
    auto sync = [this] (KnobControl* knob, const char* noteId)
    {
        if (knob != nullptr)
            knob->setTempoLocked (processorRef.apvts.getRawParameterValue (noteId)->load() >= 0.5f);
    };
    sync (preTimeKnob, "preDelayNote");
    sync (postTimeKnob, "postDelayNote");
    sync (tremRateKnob, "tremNote");
    sync (wahRateKnob, "wahNote");
    sync (phaserRateKnob, "phaserNote");
    sync (wobRateKnob, "wobNote");
    sync (swellKnob, "swellNote");
    sync (granSizeKnob, "granSizeNote");
    sync (granDensKnob, "granDensNote");
    sync (granPosKnob, "granPosNote");
    sync (reverbPreKnob, "reverbPreNote");
    sync (mfRateKnob[0], "mfANote");
    sync (mfRateKnob[1], "mfBNote");
}

// Tap tempo: the average of the last few taps (a gap over 2 seconds starts a new count).
// It sets this plugin's tempo and switches off "host tempo".
void StarterAmpEditor::tapTempo()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    if (! tapTimes.empty() && now - tapTimes.back() > 2000.0)
        tapTimes.clear();
    tapTimes.push_back (now);
    if (tapTimes.size() > 5)
        tapTimes.erase (tapTimes.begin());

    if (tapTimes.size() >= 2)
    {
        const double interval = (tapTimes.back() - tapTimes.front()) / (double) (tapTimes.size() - 1);
        const float bpm = (float) juce::jlimit (40.0, 300.0, 60000.0 / interval);

        setSwitch ("tempoHost", false);
        if (auto* param = processorRef.apvts.getParameter ("tempoBpm"))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (param->convertTo0to1 (bpm));
            param->endChangeGesture();
        }
    }
}

// The Voicing switch does something different on each amp, so its label says what.
// The names of the Voicing switch's three positions for each amp model value (v60). The first two are the switch's old
// off and on positions; the third is new.
static std::array<const char*, 4> voicingNamesFor (int model)
{
    static const std::array<const char*, 4> names[16] = {
        { "Sparkly", "Edge push", "Boutique", "Studio" },        // 0 Clean
        { "Chime", "Treble boost", "Normal", "Jangle" },         // 1 Vintage Chime
        { "Tight", "Heavy", "Brown", "Mid push" },                 // 2 Hot-Rod
        { "Smooth", "Lead", "Crunch", "Sustain" },                // 3 Saturated Lead
        { "Scooped", "Clarity", "Vintage", "Chunk" },           // 4 Modern High-Gain
        { "-", "-", "-", "-" },                             // 5 Acoustic (not used)
        { "Crunch", "Pushed", "Jumped", "Bright" },              // 6 Classic Rock
        { "Jazz", "Rock", "Lead", "Blues" },                    // 7 Boutique Lead
        { "Raw", "Modern", "Crunch", "Buzz" },                 // 8 Tight Metal
        { "Soft", "Cranked", "Four-ten", "Gritty" },              // 9 Low-Watt Combo
        { "Glass", "Warm", "Funk", "Shimmer" },                   // 10 Glass Clean
        { "Rhythm", "Lead", "Vintage", "Singing" },               // 11 Creamy Lead
        { "Stoner", "Wall", "Bass stack", "Fuzz" },            // 12 Doom Stack
        { "Tight", "Lead", "Crunch", "Polished" },                 // 13 Precision High-Gain
        { "Thall", "Industrial", "Cavern", "Lead" },           // 14 Digital Extreme
        { "Stock", "Modded", "Cranked", "Boosted" } };            // 15 Master-Volume Rock (v62)
    return names[juce::jlimit (0, 15, model)];
}

void StarterAmpEditor::syncVoicingLabel()
{
    if (voicingToggle == nullptr)
        return;
    auto fill = [this] (juce::ComboBox& box, const char* paramId, int model)
    {
        const auto names = voicingNamesFor (model);
        bool same = box.getNumItems() == 4;
        for (int i = 0; same && i < 4; ++i)
            same = box.getItemText (i) == "Voicing: " + juce::String (names[(size_t) i]);
        if (! same)
        {
            box.clear (juce::dontSendNotification);
            for (int i = 0; i < 4; ++i)
                box.addItem ("Voicing: " + juce::String (names[(size_t) i]), i + 1);
        }
        const int idx = juce::jlimit (0, 3, (int) std::lround (processorRef.apvts.getRawParameterValue (paramId)->load()));
        if (box.getSelectedId() != idx + 1)
            box.setSelectedId (idx + 1, juce::dontSendNotification);
    };
    fill (*voicingToggle, "ampVoicing", (int) processorRef.apvts.getRawParameterValue ("ampModel")->load());
    if (voicingBoxB != nullptr)
    {
        const int bm = (int) processorRef.apvts.getRawParameterValue ("ampBModel")->load();
        fill (*voicingBoxB, "ampBVoicing", bm <= 4 ? juce::jmax (0, bm) : juce::jmin (bm, 14) + 1);   // Amp B's menu has no Acoustic entry
    }
}

void StarterAmpEditor::syncIRPages()
{
    for (int blk = 0; blk < kCabBlocks; ++blk)
        irPanels[blk]->setPage (processorRef.apvts.getRawParameterValue ("cabSrc" + cabLetter (blk))->load() > 0.5f ? 1 : 0);
}

//==============================================================================
// Presets: the whole plugin state (every knob, switch and menu, plus the IR file paths) saved
// as a file in Documents/StarterAmp/Presets. Loading goes through the same code as reopening
// a session, so older presets keep working.
//==============================================================================
juce::File StarterAmpEditor::presetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("StarterAmp").getChildFile ("Presets");
}

void StarterAmpEditor::refreshPresets (const juce::String& select)
{
    presetFiles = presetFolder().findChildFiles (juce::File::findFiles, false, "*.starteramp");
    presetFiles.sort();

    presetBox.clear (juce::dontSendNotification);

    // Built-in presets first, grouped by style, then the user's own saved files.
    juce::String lastCategory;
    for (int i = 0; i < StarterAmpProcessor::factoryPresetCount(); ++i)
    {
        const auto category = StarterAmpProcessor::factoryPresetCategory (i);
        if (category != lastCategory)
        {
            if (lastCategory.isNotEmpty())
                presetBox.addSeparator();
            presetBox.addSectionHeading ("Factory: " + category);
            lastCategory = category;
        }
        presetBox.addItem (StarterAmpProcessor::factoryPresetName (i), 1000 + i);
    }

    int selectId = 0;
    if (presetFiles.size() > 0)
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User presets");
    }
    for (int i = 0; i < presetFiles.size(); ++i)
    {
        presetBox.addItem (presetFiles[i].getFileNameWithoutExtension(), i + 1);
        if (select.isNotEmpty() && presetFiles[i].getFileNameWithoutExtension() == select)
            selectId = i + 1;
    }
    if (selectId != 0)
        presetBox.setSelectedId (selectId, juce::dontSendNotification);
    else
        presetBox.setSelectedId (0, juce::dontSendNotification);

    if (browserWindow != nullptr)
        if (auto* browser = dynamic_cast<PresetBrowser*> (browserWindow->getContentComponent()))
            browser->refresh();
}

void StarterAmpEditor::afterPresetLoad (int page)
{
    for (auto& ap : advancedPanels)
        processorRef.apvts.state.setProperty (juce::Identifier ("adv_" + ap.second), ap.first->isAdvancedShown(), nullptr);
    showPage (page);               // stay on the same tab
    updateIRButtons();
    syncIRPages();
    syncDelayKnobs();
}

void StarterAmpEditor::loadFactoryPreset (int index)
{
    const int page = (int) processorRef.apvts.state.getProperty ("uiPage", 0);
    processorRef.loadFactoryPreset (index);
    afterPresetLoad (page);
}

void StarterAmpEditor::loadPresetFile (const juce::File& file)
{
    juce::MemoryBlock data;
    if (! file.loadFileAsData (data) || data.getSize() == 0)
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Preset", "Could not read " + file.getFileName());
        return;
    }

    const int page = (int) processorRef.apvts.state.getProperty ("uiPage", 0);
    if (! processorRef.loadPresetData (data.getData(), (int) data.getSize()))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Preset", file.getFileName() + " is not a valid preset (the file may be damaged).");
        return;
    }
    afterPresetLoad (page);
}

void StarterAmpEditor::stepPreset (int direction)
{
    // Walk the menu order: factory presets, then user files.
    juce::Array<int> ids;
    for (int i = 0; i < StarterAmpProcessor::factoryPresetCount(); ++i)
        ids.add (1000 + i);
    for (int i = 0; i < presetFiles.size(); ++i)
        ids.add (i + 1);
    if (ids.isEmpty())
        return;

    const int at = ids.indexOf (presetBox.getSelectedId());
    const int next = at < 0 ? (direction > 0 ? 0 : ids.size() - 1)
                            : (at + direction + ids.size()) % ids.size();
    presetBox.setSelectedId (ids[next], juce::sendNotificationSync);
}

void StarterAmpEditor::savePresetDialog()
{
    auto* window = new juce::AlertWindow ("Save preset", "Name (an existing name is replaced):", juce::MessageBoxIconType::NoIcon);
    window->addTextEditor ("name", presetBox.getText(), juce::String());
    window->addTextEditor ("tags", processorRef.apvts.state.getProperty ("presetTags").toString(), "Tags (separate with commas):");
    window->addButton ("Save",   1, juce::KeyPress (juce::KeyPress::returnKey));
    window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<StarterAmpEditor> safeThis (this);
    window->enterModalState (true, juce::ModalCallbackFunction::create ([safeThis, window] (int result)
    {
        if (result != 1 || safeThis == nullptr)
            return;

        const auto name = juce::File::createLegalFileName (window->getTextEditorContents ("name").trim());
        if (name.isEmpty())
            return;

        auto folder = presetFolder();
        folder.createDirectory();
        const auto file = folder.getChildFile (name + ".starteramp");

        safeThis->processorRef.apvts.state.setProperty ("presetTags", PresetBrowser::cleanTags (window->getTextEditorContents ("tags")), nullptr);
        juce::MemoryBlock data;
        safeThis->processorRef.getPresetData (data);
        if (! file.replaceWithData (data.getData(), data.getSize()))
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Preset", "Could not write " + file.getFullPathName());
        safeThis->refreshPresets (name);
    }), true);
}

void StarterAmpEditor::deletePreset()
{
    const int id = presetBox.getSelectedId();
    if (id < 1 || id > presetFiles.size())
        return;                    // nothing selected, or a built-in preset (those cannot be deleted)

    const auto file = presetFiles[id - 1];
    juce::Component::SafePointer<StarterAmpEditor> safeThis (this);
    juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                      .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                      .withTitle ("Delete preset")
                                      .withMessage ("Delete \"" + file.getFileNameWithoutExtension() + "\"? This cannot be undone.")
                                      .withButton ("Delete").withButton ("Cancel"),
                                  [safeThis, file] (int result)
                                  {
                                      if (result == 1 && safeThis != nullptr)
                                      {
                                          file.deleteFile();
                                          safeThis->refreshPresets();
                                      }
                                  });
}

void StarterAmpEditor::applyAccent (juce::Colour accent)
{
    if (accent == currentAccent)
        return;
    currentAccent = accent;
    ui::accent  = accent;
    ui::accent2 = accent.interpolatedWith (juce::Colours::white, 0.3f);
    lookAndFeel.applyAccent();

    content.refresh();
    std::function<void (juce::Component&)> walk = [&walk] (juce::Component& comp)
    {
        if (auto* panel = dynamic_cast<Panel*> (&comp))
            panel->refreshFace();
        for (auto* child : comp.getChildren())
            walk (*child);
    };
    walk (*this);
    repaint();
}

void StarterAmpEditor::exportCab()
{
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("StarterAmp");
    chooser = std::make_unique<juce::FileChooser> ("Export the cab blend as a WAV impulse response", start.getChildFile ("CabBlend.wav"), "*.wav");

    juce::Component::SafePointer<StarterAmpEditor> safeThis (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safeThis] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (safeThis == nullptr || file == juce::File())
                                  return;
                              safeThis->exportStatus.setText (safeThis->processorRef.exportCabBlend (file.withFileExtension ("wav")),
                                                              juce::dontSendNotification);
                          });
}


//==============================================================================
// Riff recorder window (v52)
//==============================================================================
RiffPanel::RiffPanel (StarterAmpProcessor& p) : processor (p)
{
    setSize (640, 360);
    auto& state = processor.apvts;

    recButton.setTooltip ("Starts recording. Play your phrase, then press STOP. Recording stops by itself after 60 seconds.");
    playButton.setTooltip ("Plays the riff from the start. With Loop on it repeats until you press STOP.");
    stopButton.setTooltip ("Stops recording or playback.");
    clearButton.setTooltip ("Throws the riff away.");
    exportButton.setTooltip ("Saves the riff as a WAV file. The riff is not saved with the session, so export it if you want to keep it.");
    importButton.setTooltip ("Loads a WAV file as the riff (up to 60 seconds). A guitar DI track works best in DI mode.");
    loopButton.setTooltip (tipFor (state, "riffLoop"));
    muteButton.setTooltip (tipFor (state, "riffMute"));
    modeBox.setTooltip (tipFor (state, "riffMode"));
    levelSlider.setTooltip (tipFor (state, "riffLevel"));

    if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (state.getParameter ("riffMode")))
        modeBox.addItemList (ch->choices, 1);
    modeAttach  = std::make_unique<APVTS::ComboBoxAttachment> (state, "riffMode", modeBox);
    loopAttach  = std::make_unique<APVTS::ButtonAttachment> (state, "riffLoop", loopButton);
    muteAttach  = std::make_unique<APVTS::ButtonAttachment> (state, "riffMute", muteButton);
    levelSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    levelSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
    levelAttach = std::make_unique<APVTS::SliderAttachment> (state, "riffLevel", levelSlider);
    levelSlider.textFromValueFunction = [] (double v) { return juce::String (v, 2) + "x"; };
    levelSlider.updateText();

    for (auto* l : { &modeLabel, &levelLabel })
        l->setColour (juce::Label::textColourId, ui::textDim);
    modeLabel.setText ("Records at", juce::dontSendNotification);
    levelLabel.setText ("Playback level", juce::dontSendNotification);
    status.setFont (juce::Font (juce::FontOptions (15.0f, juce::Font::bold)));
    status.setColour (juce::Label::textColourId, ui::text);
    note.setFont (juce::Font (juce::FontOptions (12.0f)));
    note.setColour (juce::Label::textColourId, ui::textDim);
    note.setJustificationType (juce::Justification::topLeft);
    note.setText ("DI mode records your guitar before any effect, and plays it back into the start of the chain: change pedals, amp and cab while it loops. "
                  "Full tone mode records what you hear and plays it back after everything. The mode applies to the next recording. "
                  "The riff stays in memory while the plugin is open and is not saved with the session: use EXPORT to keep it.", juce::dontSendNotification);

    startTimerHz (20);                      // v94: always running (the visibility hook alone left the panel dead in some windows)
    recButton.onClick   = [this] { processor.riffRecord(); };
    playButton.onClick  = [this] { processor.riffPlay(); };
    stopButton.onClick  = [this] { processor.riffStop(); };
    clearButton.onClick = [this] { processor.riffClear(); lastLen = -2; };
    exportButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save the riff as a WAV file",
                      juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("StarterAmp").getChildFile ("Riff.wav"), "*.wav");
        juce::Component::SafePointer<RiffPanel> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (safe == nullptr || f == juce::File()) return;
                                  f.getParentDirectory().createDirectory();
                                  juce::String msg;
                                  safe->processor.riff.exportWav (f.withFileExtension ("wav"), msg);
                                  safe->status.setText (msg, juce::dontSendNotification);
                              });
    };
    importButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load a WAV file as the riff",
                      juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.wav;*.aif;*.aiff");
        juce::Component::SafePointer<RiffPanel> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (safe == nullptr || ! f.existsAsFile()) return;
                                  juce::String msg;
                                  safe->processor.riffRecMode.store ((int) safe->processor.apvts.getRawParameterValue ("riffMode")->load());
                                  safe->processor.riff.importWav (f, msg);
                                  safe->status.setText (msg, juce::dontSendNotification);
                                  safe->lastLen = -2;
                              });
    };

    for (auto* c : { (juce::Component*) &recButton, (juce::Component*) &playButton, (juce::Component*) &stopButton, (juce::Component*) &clearButton,
                     (juce::Component*) &exportButton, (juce::Component*) &importButton, (juce::Component*) &modeBox, (juce::Component*) &loopButton,
                     (juce::Component*) &muteButton, (juce::Component*) &levelSlider, (juce::Component*) &modeLabel, (juce::Component*) &levelLabel,
                     (juce::Component*) &status, (juce::Component*) &note })
        addAndMakeVisible (c);
    peaks.assign (300, 0.0f);
    updateButtons();
}

void RiffPanel::resized()
{
    auto r = getLocalBounds().reduced (16);
    auto top = r.removeFromTop (36);
    recButton.setBounds (top.removeFromLeft (92));   top.removeFromLeft (8);
    playButton.setBounds (top.removeFromLeft (92));  top.removeFromLeft (8);
    stopButton.setBounds (top.removeFromLeft (92));  top.removeFromLeft (8);
    clearButton.setBounds (top.removeFromLeft (80));
    r.removeFromTop (8);
    status.setBounds (r.removeFromTop (24));
    r.removeFromTop (4);
    waveArea = r.removeFromTop (110);
    r.removeFromTop (10);
    auto row = r.removeFromTop (28);
    modeLabel.setBounds (row.removeFromLeft (80));
    modeBox.setBounds (row.removeFromLeft (230));  row.removeFromLeft (14);
    loopButton.setBounds (row.removeFromLeft (70));
    r.removeFromTop (6);
    row = r.removeFromTop (28);
    levelLabel.setBounds (row.removeFromLeft (110));
    levelSlider.setBounds (row.removeFromLeft (260));
    r.removeFromTop (4);
    muteButton.setBounds (r.removeFromTop (26));
    r.removeFromTop (6);
    auto files = r.removeFromTop (30);
    exportButton.setBounds (files.removeFromLeft (130));  files.removeFromLeft (8);
    importButton.setBounds (files.removeFromLeft (130));
    r.removeFromTop (8);
    note.setBounds (r);
}

void RiffPanel::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
    auto wa = waveArea.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (wa, 5.0f);
    g.setColour (ui::textDim.withAlpha (0.3f));
    g.drawLine (wa.getX(), wa.getCentreY(), wa.getRight(), wa.getCentreY());

    const int st = processor.riff.getState();
    const float head = processor.riff.getPlayhead();
    if (st == RiffRecorder::Recording)
    {
        g.setColour (juce::Colour (0xffc0392b).withAlpha (0.35f));
        g.fillRoundedRectangle (wa.withWidth (wa.getWidth() * head), 5.0f);
    }
    else if (processor.riff.getLength() > 0)
    {
        g.setColour (ui::accent);
        const float w = wa.getWidth() / (float) peaks.size();
        for (size_t i = 0; i < peaks.size(); ++i)
        {
            const float h = std::min (1.0f, peaks[i] * 2.2f) * wa.getHeight() * 0.48f;
            g.fillRect (wa.getX() + (float) i * w, wa.getCentreY() - h, std::max (1.0f, w - 0.5f), 2.0f * h + 1.0f);
        }
        if (st == RiffRecorder::Playing)
        {
            g.setColour (juce::Colours::white);
            const float x = wa.getX() + wa.getWidth() * head;
            g.drawLine (x, wa.getY(), x, wa.getBottom(), 2.0f);
        }
    }
    else
    {
        g.setColour (ui::textDim);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText ("Nothing recorded yet", waveArea, juce::Justification::centred);
    }
}

void RiffPanel::updateButtons()
{
    const int st = processor.riff.getState();
    const bool has = processor.riff.getLength() > 0;
    recButton.setEnabled (st != RiffRecorder::Recording);
    playButton.setEnabled (has && st == RiffRecorder::Idle);
    stopButton.setEnabled (st != RiffRecorder::Idle);
    clearButton.setEnabled (has && st == RiffRecorder::Idle);
    exportButton.setEnabled (has && st == RiffRecorder::Idle);
    importButton.setEnabled (st == RiffRecorder::Idle);
    recButton.setColour (juce::TextButton::buttonColourId, st == RiffRecorder::Recording ? juce::Colour (0xffc0392b) : ui::knobBody);
    playButton.setColour (juce::TextButton::buttonColourId, st == RiffRecorder::Playing ? juce::Colour (0xff2e8b57) : ui::knobBody);
    // The mute switch only means something in DI mode
    muteButton.setEnabled (processor.riffRecMode.load() == 0);
}

void RiffPanel::timerCallback()
{
    const int st = processor.riff.getState();
    const int len = processor.riff.getLength();
    if (len != lastLen && st != RiffRecorder::Recording)
    {
        lastLen = len;
        processor.riff.getPeaks (peaks.data(), (int) peaks.size());
    }
    if (st != lastState) { lastState = st; updateButtons(); }
    const char* modeName = processor.riffRecMode.load() == 0 ? "DI" : "full tone";
    juce::String text;
    if (st == RiffRecorder::Recording)
        text = "Recording (" + juce::String (modeName) + ")  " + juce::String (processor.riff.getPositionSeconds(), 1) + " s  of 60";
    else if (st == RiffRecorder::Playing)
        text = "Playing  " + juce::String (processor.riff.getPositionSeconds(), 1) + " / " + juce::String (processor.riff.getSeconds(), 1) + " s  (" + modeName + ")";
    else if (len > 0)
        text = "Riff ready: " + juce::String (processor.riff.getSeconds(), 1) + " s  (" + modeName + ")";
    else
        text = "Press REC, play your phrase, then press STOP.";
    if (status.getText() != text && (st != RiffRecorder::Idle || len > 0 || status.getText().startsWith ("Press") || status.getText().startsWith ("Riff") || status.getText().startsWith ("Playing") || status.getText().startsWith ("Recording")))
        status.setText (text, juce::dontSendNotification);
    repaint (waveArea);
}

void StarterAmpEditor::openRiffWindow()
{
    if (riffWindow == nullptr)
        riffWindow = std::make_unique<ToolWindow> ("Riff recorder", std::make_unique<RiffPanel> (processorRef), lookAndFeel);
    riffWindow->setVisible (true);
    riffWindow->toFront (true);
}

// ---------------------------------------------------------------------------------------------------------------
// Grey-out system (v52). Controls that do nothing in the current state are greyed, and their tooltip says why.
// A panel whose ON switch is off is faded (it still works, so a setting can be prepared before the effect is on).
// ---------------------------------------------------------------------------------------------------------------
float StarterAmpEditor::pv (const char* id) const
{
    if (auto* v = processorRef.apvts.getRawParameterValue (id))
        return v->load();
    return 0.0f;
}

void StarterAmpEditor::setUsed (const char* id, bool used, const juce::String& why)
{
    auto it = idMap.find (juce::String (id));
    if (it == idMap.end())
        return;
    auto* c = it->second;
    if (auto* k = dynamic_cast<KnobControl*> (c))
    {
        k->setUsed (used, why);
        return;
    }
    auto* tc = dynamic_cast<juce::SettableTooltipClient*> (c);
    if (tc == nullptr)
        return;
    auto base = baseTips.find (c);
    if (base == baseTips.end())
        base = baseTips.emplace (c, tc->getTooltip()).first;
    if (used)
    {
        if (! c->isEnabled())
        {
            c->setEnabled (true);
            tc->setTooltip (base->second);
        }
    }
    else
    {
        const auto tip = "Not in use right now: " + why + "\n" + base->second;
        if (c->isEnabled())
            c->setEnabled (false);
        if (tc->getTooltip() != tip)
            tc->setTooltip (tip);
    }
}

void StarterAmpEditor::syncUsed()
{
    // ---- Panels whose ON switch is off fade out
    for (auto& ms : masterSwitches)
    {
        auto it = idMap.find (ms.second);
        ms.first->setContentDimmed (it != idMap.end() ? it->second : nullptr, pv (ms.second.toRawUTF8()) < 0.5f);
    }
    boostEqPanel.setContentDimmed (idMap.count ("bsEqOn") ? idMap["bsEqOn"] : nullptr, pv ("bsEqOn") < 0.5f || pv ("bsOn") < 0.5f);

    // ---- Delays
    for (int d = 0; d < 2; ++d)
    {
        const std::string p = d == 0 ? "preDelay" : "postDelay";
        setUsed ((p + "TapLevel").c_str(), pv ((p + "Taps").c_str()) > 0.5f, "pick a Multi-Tap pattern first.");
        setUsed ((p + "DuckRel").c_str(),  pv ((p + "Duck").c_str()) > 0.001f, "raise Duck first.");
        setUsed ((p + "ModRate").c_str(),  pv ((p + "ModDepth").c_str()) > 0.001f, "raise Mod Depth first.");
    }

    syncVoicingLabel();

    // ---- Boost
    setUsed ("bsSat", pv ("bsMode") > 0.5f, "the Clean mode has no saturation. Pick another mode.");
    setUsed ("bsTightStyle", pv ("bsTight") > 0.001f, "raise Tighten first.");

    // ---- Mod FX: each type uses only some of the controls
    for (int s = 0; s < 2; ++s)
    {
        const std::string p = s == 0 ? "mfA" : "mfB";
        const int type = (int) pv ((p + "Type").c_str());
        setUsed ((p + "Feedback").c_str(), type == 0 || type == 1 || type == 3, "only the flangers and the Uni-V use Feedback.");
        setUsed ((p + "Manual").c_str(),   type == 0 || type == 1 || type == 4, "only the flangers use Manual as the centre delay, and the rotary uses it as the horn/drum balance.");
        setUsed ((p + "Shape").c_str(),    type != 4, "the rotary speaker has its own motion, so the LFO shape is not used.");
        setUsed ((p + "Tone").c_str(),     type <= 4, "this type has no tone filter.");
    }

    // ---- Harmonizer
    const bool scaleMode = (int) pv ("hmMode") == 1;
    setUsed ("hmAInt", ! scaleMode, "Scale mode uses the Step knob instead.");
    setUsed ("hmBInt", ! scaleMode, "Scale mode uses the Step knob instead.");
    setUsed ("hmADeg", scaleMode, "Interval mode uses the Interval knob instead.");
    setUsed ("hmBDeg", scaleMode, "Interval mode uses the Interval knob instead.");
    setUsed ("hmKey", scaleMode, "only Scale mode needs a key.");
    {
        const int track = (int) pv ("hmTrack");
        setUsed ("hmGlide", track == 0 && scaleMode, track != 0 ? "Chords mode changes pitch at once; Glide only works in Single note mode." : "Interval mode never changes the interval, so there is nothing to glide.");
        const uint64_t heard = processorRef.harmHeardNotes();
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        juce::String t;
        if (! scaleMode) t = "Interval mode: the harmony moves with whatever you play.";
        else if (heard == 0) t = "Hearing: nothing yet";
        else
        {
            t = "Hearing:";
            for (int i = 0; i < 64; ++i)
                if (heard & (uint64_t (1) << i)) { const int m = 24 + i; t << " " << names[m % 12] << juce::String (m / 12 - 1); }
        }
        if (harmHeard.getText() != t) harmHeard.setText (t, juce::dontSendNotification);
    }
    setUsed ("hmScale", scaleMode, "only Scale mode needs a scale.");
    const bool afterCab = (int) pv ("hmPos") == 1;
    setUsed ("hmAPan", afterCab, "before the amp the harmony is mono. Choose After the cab for stereo.");
    setUsed ("hmBPan", afterCab, "before the amp the harmony is mono. Choose After the cab for stereo.");

    // ---- Wah: each mode uses different controls
    const int wahMode = (int) pv ("wahMode");
    setUsed ("wahSens",  wahMode == 1, "only the Envelope mode follows how hard you play.");
    setUsed ("wahDecay", wahMode == 1, "only the Envelope mode has a decay.");
    setUsed ("wahRate",  wahMode == 2, "only the LFO mode sweeps by itself.");
    setUsed ("wahDepth", wahMode == 2, "only the LFO mode sweeps by itself.");
    setUsed ("reverbDrive", (int) pv ("reverbType") == 6, "only the Spring reverb type has a driver. Pick Spring.");
    setUsed ("wahNote",  wahMode == 2, "only the LFO mode sweeps by itself.");

    // ---- Reverb: some controls only exist in the newer reverb types
    const bool fdn = (int) pv ("reverbType") > 0;
    for (const char* id : { "reverbDiffuse", "reverbSpace", "reverbDrag", "reverbDragMode" })
        setUsed (id, fdn, "the Classic reverb does not have this. Pick another reverb type.");

    // ---- Granular: the scale only shapes the random pitches
    setUsed ("granScale", pv ("granPitchRand") >= 0.5f, "the scale shapes the random pitches. Raise Pitch Rand (Advanced) to 0.5 or more.");

    // ---- Pickup de-noise
    for (const char* id : { "deNoiseHz", "deNoiseHum", "deNoiseHiss" })
        setUsed (id, pv ("deNoiseOn") > 0.5f, "switch PICKUP DE-NOISE on first.");

    // ---- Amp B: its own level balance only matters when it runs
    const bool bOn = pv ("ampBOn") > 0.5f;
    setUsed ("ampALevel", bOn, "Amp A has no separate level until AMP B is switched on.");
    setUsed ("ampBLevel", bOn, "switch AMP B ON first.");

    // ---- NAM in place of an amp's preamp and power stage
    const bool namA = pv ("namOn") > 0.5f && processorRef.namHasModel();
    const bool namB = pv ("namBOn") > 0.5f && processorRef.namHasModel (1);
    const juce::String namWhy = "a neural model replaces the preamp and power stage, so this does nothing. The tone controls still work.";
    for (const char* id : { "ampGain", "presence", "ampBias", "ampPower", "ampReso", "ampSagAmt", "ampBright", "ampTube",
                            "ampPwrTube", "ampStack", "ampFeel", "ampMode", "ampDeep", "ampAutoLevel", "ampVoicing" })
        setUsed (id, ! namA, namWhy);
    for (const char* id : { "ampBGain", "ampBPresence", "ampBBias", "ampBPower", "ampBReso", "ampBSagAmt", "ampBBright", "ampBTube",
                            "ampBPwrTube", "ampBStack", "ampBFeel", "ampBMode", "ampBDeep", "ampBAutoLevel", "ampBVoicing", "ampBTight" })
        setUsed (id, ! namB, namWhy);
    metalPanel.setContentDimmed (nullptr, namA);

    // ---- The header's RIFF button shows what the recorder is doing
    {
        const int st = processorRef.riff.getState();
        const juce::String text = st == RiffRecorder::Recording ? "REC" : st == RiffRecorder::Playing ? "PLAY" : "RIFF";
        if (riffButton.getButtonText() != text)
        {
            riffButton.setButtonText (text);
            riffButton.setColour (juce::TextButton::buttonColourId,
                                  st == RiffRecorder::Recording ? juce::Colour (0xffc0392b) : st == RiffRecorder::Playing ? juce::Colour (0xff2e8b57) : ui::knobBody);
        }
    }
}

void StarterAmpEditor::showAmpTab (int index)
{
    ampTab = juce::jlimit (0, 2, index);
    processorRef.apvts.state.setProperty ("uiAmpTab", ampTab, nullptr);   // remembered with the session
    syncVisibility();
}

void StarterAmpEditor::showCabTab (int index)
{
    cabTab = juce::jlimit (0, 1, index);
    processorRef.apvts.state.setProperty ("uiCabTab", cabTab, nullptr);   // remembered with the session
    showPage (5);
}

void StarterAmpEditor::showPage (int index)
{
    index = juce::jlimit (0, 9, index);
    juce::Component* pages[10] = { &pageInput, &pageGain, &pageMod, &pagePitch, &pageAmp, cabTab == 0 ? &pageCab : &pageSpeaker,
                                   &pageEq, &pageDelay, &pageScapes, &pagePost };
    for (auto* pg : { &pageInput, &pageGain, &pageMod, &pageDelay, &pagePitch, &pageAmp, &pageCab, &pageSpeaker, &pageEq, &pageScapes, &pagePost })
        pg->setVisible (false);
    pages[index]->setVisible (true);
    for (int pg = 0; pg < 2; ++pg)
        cabSubTabs[pg][cabTab].setToggleState (true, juce::dontSendNotification);

    pageTabs[index].setToggleState (true, juce::dontSendNotification);
    chainStrip.setCurrentPage (index);
    processorRef.apvts.state.setProperty ("uiPage", index, nullptr);   // remembered with the session
    processorRef.apvts.state.setProperty ("uiLayout", 73, nullptr);
    layoutFlow (false);
}

void StarterAmpEditor::updateIRButtons()
{
    auto text = [] (const juce::String& slotName, const juce::String& file) -> juce::String
    {
        if (file.isEmpty())
            return "Load IR " + slotName + "...";

        const auto shortName = file.length() > 14 ? file.substring (0, 13) + "..." : file;
        return slotName + ": " + shortName;
    };

    for (int blk = 0; blk < kCabBlocks; ++blk)
    {
        const auto label = text (cabLetter (blk), processorRef.getIRName (blk));
        if (irButtons[blk].getButtonText() != label)
            irButtons[blk].setButtonText (label);
    }
}

void StarterAmpEditor::chooseIR (int slot)
{
    chooser = std::make_unique<juce::FileChooser> (
        juce::String ("Choose cab impulse response ") + cabLetter (slot) + " (WAV or AIFF)",
        juce::File(), "*.wav;*.aif;*.aiff");

    // SafePointer: if the window closes while the dialog is open, don't crash.
    juce::Component::SafePointer<StarterAmpEditor> safeThis (this);

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                              | juce::FileBrowserComponent::canSelectFiles,
                          [safeThis, slot] (const juce::FileChooser& fc)
                          {
                              const auto file = fc.getResult();
                              if (safeThis != nullptr && file.existsAsFile())
                              {
                                  safeThis->processorRef.loadIR (file, slot, true);
                                  safeThis->updateIRButtons();
                              }
                          });
}


//==============================================================================
// Tools: effect order, metronome, preset browser, A/B menu, undo
//==============================================================================
bool StarterAmpEditor::userBusy() const
{
    if (juce::ModifierKeys::getCurrentModifiersRealtime().isAnyMouseButtonDown())
        return true;
    if (juce::ModalComponentManager::getInstance()->getNumModalComponents() > 0)
        return true;                                            // a menu or dialog is open
    const auto now = juce::Time::getMillisecondCounter();
    if (dynamic_cast<juce::TextEditor*> (juce::Component::getCurrentlyFocusedComponent()) != nullptr)
        lastTextBusy = now;                                     // typing a value
    if (now - lastTextBusy < 800)
        return true;
    const auto lastDown = juce::Desktop::getInstance().getMainMouseSource().getLastMouseDownTime();
    return (juce::Time::getCurrentTime() - lastDown).inMilliseconds() < 1000;
}

bool StarterAmpEditor::keyPressed (const juce::KeyPress& key)
{
    const auto mods = key.getModifiers();
    if (mods.isCommandDown() || mods.isCtrlDown())
    {
        const auto ch = juce::CharacterFunctions::toUpperCase ((juce::juce_wchar) key.getKeyCode());
        const int page = (int) processorRef.apvts.state.getProperty ("uiPage", 0);
        if (ch == 'Z' && ! mods.isShiftDown()) { if (processorRef.undo()) afterPresetLoad (page); return true; }
        if ((ch == 'Z' && mods.isShiftDown()) || ch == 'Y') { if (processorRef.redo()) afterPresetLoad (page); return true; }
    }
    return false;
}

//==============================================================================
// PedalTile, tidy view and the flow layout (v73)
//==============================================================================
PedalTile::PedalTile (juce::AudioProcessorValueTreeState& apvts, const juce::String& n, const juce::String& d, juce::Colour t,
                      const juce::String& onId, const juce::String& tip)
    : name (n), description (d), tint (t)
{
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, onId, sw);
    sw.setTooltip (tip.isNotEmpty() ? tip : "Switches this effect on. It then appears in full on this page.");
    addAndMakeVisible (sw);
}

void PedalTile::resized()
{
    sw.setBounds (18, getHeight() - 33, getWidth() - 30, 26);
}

void PedalTile::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (ui::panel.darker (0.25f));
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (r, 8.0f, 1.0f);
    g.setColour (tint.brighter (0.25f));
    g.fillRoundedRectangle (r.getX() + 1.0f, r.getY() + 9.0f, 5.0f, r.getHeight() - 18.0f, 2.5f);
    g.setColour (ui::text);
    g.setFont (juce::Font (juce::FontOptions (12.5f, juce::Font::bold)));
    g.drawText (name, 18, 8, getWidth() - 26, 18, juce::Justification::centredLeft);
    g.setColour (ui::textDim);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText (description, 18, 25, getWidth() - 26, 14, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.setColour (ui::textDim.withAlpha (0.8f));
    g.drawText ("OFF", getWidth() - 44, 8, 34, 14, juce::Justification::centredRight);
}

void StarterAmpEditor::addFlowPage (juce::Component& page, std::vector<FlowItem> items)
{
    auto fp = std::make_unique<FlowPage>();
    fp->page = &page;
    static const std::map<juce::String, juce::String> descriptions = {
        { "TRANSPOSE / FORMANT", "Shift pitch and voice" }, { "FUZZ", "Fuzz pedal" }, { "COMPRESSOR", "Evens out loud and soft" },
        { "OVERDRIVE", "Overdrive pedal" }, { "BOOST", "Clean or colour boost" }, { "BOOST EQ", "4-band tone shaper" },
        { "WAH", "Wah pedal" }, { "PHASER", "Swirling phase effect" }, { "CHORUS", "Thickening chorus" }, { "MOD FX A", "Flanger, vibrato, rotary..." },
        { "MOD FX B", "Flanger, vibrato, rotary..." }, { "PITCH PEDAL", "Expression pitch shifter" }, { "OCTAVER", "Adds octaves" },
        { "HARMONIZER", "Adds harmony notes" }, { "PRE-AMP DELAY", "Echo before the amp" }, { "POST-AMP DELAY", "Echo after the cab" },
        { "GRANULAR CLOUD", "Cloud of tiny grains" }, { "TREMOLO / AUTO-PAN", "Pulsing volume or pan" }, { "WOBBLE", "Loose tremolo-arm vibrato" },
        { "SWELL", "Slow volume fade-in" }, { "LO-FI WARBLE", "Worn tape wobble" }, { "NOISE PEDAL", "Harsh pitch shifter" },
        { "REVERB", "Room, hall, shimmer" }, { "DOUBLER", "Double-tracking" }, { "WIDENER", "Stereo widening" } };
    for (auto& it : items)
        if (it.onId.isNotEmpty())
        {
            const auto found = descriptions.find (it.name);
            const juce::String desc = found != descriptions.end() ? found->second : juce::String();
            const juce::Colour tint = it.main->getKind() == PanelKind::Pedal ? it.main->getTint() : juce::Colour (0xff3a4a5c);
            it.tile = std::make_unique<PedalTile> (processorRef.apvts, it.name, desc, tint, it.onId, tipFor (processorRef.apvts, it.onId));
            page.addChildComponent (*it.tile);
        }
    fp->items = std::move (items);
    fp->hint.setText ("Everything on this page is switched off. Press SWITCH ON on a tile to use it. TIDY VIEW (top right) shows every effect in full.", juce::dontSendNotification);
    fp->hint.setFont (juce::Font (juce::FontOptions (14.0f)));
    fp->hint.setColour (juce::Label::textColourId, ui::textDim);
    fp->hint.setJustificationType (juce::Justification::topLeft);
    fp->hint.setInterceptsMouseClicks (false, false);
    page.addChildComponent (fp->hint);
    flowPages.push_back (std::move (fp));
}

// Places the pedals of every flow page. The page is two lanes (rows) high. The pedals that are on are packed from the
// left in their usual order: the first lane they fit in. In TIDY VIEW the pedals that are off become small tiles,
// stacked three high after the pedals that are on.
void StarterAmpEditor::layoutFlow (bool force)
{
    juce::String sig (tidyOn ? "T" : "A");
    for (auto& fp : flowPages)
        for (auto& it : fp->items)
            if (it.onId.isNotEmpty())
                sig << (pv (it.onId.toRawUTF8()) > 0.5f ? '1' : '0');
    if (! force && sig == flowSignature)
        return;
    flowSignature = sig;

    constexpr int W = 1480, gap = 10, tileW = 200, tileGap = 8, h0 = 256, y1 = 266, h1 = 254;
    for (auto& fp : flowPages)
    {
        int x[2] = { 0, 0 };
        std::vector<FlowItem*> off;
        for (auto& it : fp->items)
        {
            const bool collapsed = tidyOn && it.onId.isNotEmpty() && pv (it.onId.toRawUTF8()) < 0.5f;
            if (collapsed)
            {
                off.push_back (&it);
                it.main->setVisible (false);
                if (it.extra != nullptr) it.extra->setVisible (false);
                continue;
            }
            if (it.tile != nullptr) it.tile->setVisible (false);
            it.main->setVisible (true);
            if (it.extra != nullptr) it.extra->setVisible (true);

            const int total = it.w + (it.extra != nullptr ? gap + it.wExtra : 0);
            if (it.lanes == 2)
            {
                const int px = std::max (x[0], x[1]);
                it.main->setBounds (px, 0, std::min (it.w, W - px), y1 + h1);
                x[0] = x[1] = px + it.w + gap;
                continue;
            }
            int lane;
            if (it.lane == 1 && x[0] == 0)  lane = 0;
            else if (it.lane >= 0)          lane = it.lane;
            else                            lane = (x[0] + total <= W) ? 0 : 1;
            const double scale = x[lane] + total <= W ? 1.0 : std::max (0.6, (double) (W - x[lane]) / (double) total);
            const int wMain = (int) std::floor (it.w * scale), wExtra = (int) std::floor (it.wExtra * scale);
            const int y = lane == 0 ? 0 : y1, h = lane == 0 ? h0 : h1;
            it.main->setBounds (x[lane], y, wMain, h);
            if (it.extra != nullptr)
                it.extra->setBounds (x[lane] + wMain + gap, y, wExtra, h);
            x[lane] += (it.extra != nullptr ? wMain + gap + wExtra : wMain) + gap;
        }

        // The tiles
        const bool nothingOn = x[0] == 0 && x[1] == 0;
        int colX[2] = { 0, 0 }, colCount[2] = { 3, 3 };         // 3 = no column open yet
        for (auto* it : off)
        {
            int lane = -1;
            for (int l = 0; l < 2 && lane < 0; ++l)
                if (colCount[l] < 3) lane = l;                   // keep filling an open column
            if (lane < 0)
            {
                for (int l = 0; l < 2 && lane < 0; ++l)          // new columns go next to the pedals, in the top lane first
                    if (x[l] + tileW <= W) lane = l;
                if (lane < 0) lane = x[0] <= x[1] ? 0 : 1;       // no room: stack on the last column (should not happen)
                colX[lane] = std::min (x[lane], W - tileW);
                x[lane] += tileW + gap;
                colCount[lane] = 0;
            }
            const int laneH = lane == 0 ? h0 : h1;
            const int th = (laneH - 2 * tileGap) / 3;
            it->tile->setBounds (colX[lane], (lane == 0 ? 0 : y1) + colCount[lane] * (th + tileGap), tileW, th);
            it->tile->setVisible (true);
            ++colCount[lane];
        }
        fp->hint.setVisible (nothingOn && ! off.empty());
        if (nothingOn && ! off.empty())
            fp->hint.setBounds (std::max (x[0], x[1]) + 10, 6, 420, 60);
    }
}

HelpPanel::HelpPanel()
{
    setSize (600, 560);
    text.setMultiLine (true);
    text.setReadOnly (true);
    text.setScrollbarsShown (true);
    text.setCaretVisible (false);
    text.setFont (juce::Font (juce::FontOptions (15.0f)));
    text.setColour (juce::TextEditor::backgroundColourId, ui::panel);
    text.setColour (juce::TextEditor::textColourId, ui::text);
    text.setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    text.setText (
        "GETTING STARTED\n\n"
        "1. Pick a sound\n"
        "Use the preset box at the top. The < and > buttons next to it step through the list, and BROWSE opens a searchable list. "
        "A preset sets every control, so you can start from one and adjust.\n\n"
        "2. Set your levels\n"
        "Play your hardest chord and watch the IN meter in the OUTPUT box (POST FX tab): it should not touch the red light. On the INPUT tab, turn the "
        "Gate up only if you hear hiss between notes.\n\n"
        "3. Find your way around\n"
        "The tabs along the top group the controls in the order your sound travels:\n"
        "   INPUT: input level, noise gate, transpose.\n"
        "   GAIN: fuzz, compressor, overdrive, boost.\n"
        "   MOD: wah, phaser, chorus, flanger, vibrato, rotary.\n"
        "   PITCH: pitch pedal, octaver, harmonizer.\n"
        "   AMP: the amp itself, with its tone controls.\n"
        "   CAB: the speaker cabinet and microphone.\n"
        "   EQ: a final tone shaper and a display of what you hear.\n"
        "   DELAY: echoes, before or after the amp.\n"
        "   SCAPES: big ambient and experimental effects.\n"
        "   POST FX: reverb, doubler, widener and the output volume.\n\n"
        "4. Switch an effect on or off\n"
        "Every effect has an ON switch at the top of its box. The row of stages under the tabs shows the whole path; "
        "click the small light on a stage to switch it on or off, and click the stage's name to open its page.\n\n"
        "5. TIDY VIEW\n"
        "With TIDY VIEW lit, effects that are switched off shrink to small tiles so you only see what you are using. "
        "Press SWITCH ON on a tile to bring an effect back. Press TIDY VIEW again to see every effect in full.\n\n"
        "6. Change the order of the effects\n"
        "Drag a stage in the row under the tabs to the left or right. Fuzz, compressor, wah, phaser, chorus and the delay before the amp "
        "can be moved around each other, and after the cab the EQ, delay and reverb can. The ORDER button has the full list.\n\n"
        "7. ADV buttons\n"
        "Some boxes have an ADV button. The basic controls are always there; ADV adds the finer ones.\n\n"
        "8. Save your work\n"
        "SAVE stores your sound as a preset with a name you choose. A and B hold two versions of your sound so you can compare them. "
        "UNDO and REDO step back and forward through your changes.\n\n"
        "9. Hover for help\n"
        "Rest the mouse on any control for a few seconds and a short note appears.\n\n"
        "10. If something sounds wrong\n"
        "Turn the sound off by choosing a preset, check the OUTPUT level on the POST FX tab, and make sure your audio device is "
        "set in the app's audio settings. When you report a problem, please say which version label is shown at the top left.",
        false);
    addAndMakeVisible (text);
}

void StarterAmpEditor::openHelpWindow()
{
    if (helpWindow == nullptr)
        helpWindow = std::make_unique<ToolWindow> ("Help", std::make_unique<HelpPanel>(), lookAndFeel);
    helpWindow->setVisible (true);
    helpWindow->toFront (true);
}

void StarterAmpEditor::openOrderWindow()
{
    if (orderWindow == nullptr)
        orderWindow = std::make_unique<ToolWindow> ("Effect order", std::make_unique<OrderPanel> (processorRef), lookAndFeel);
    orderWindow->setVisible (true);
    orderWindow->toFront (true);
}

void StarterAmpEditor::openMetroWindow()
{
    if (metroWindow == nullptr)
        metroWindow = std::make_unique<ToolWindow> ("Metronome", std::make_unique<MetroPanel> (processorRef.apvts), lookAndFeel);
    metroWindow->setVisible (true);
    metroWindow->toFront (true);
}

void StarterAmpEditor::openLibraryWindow()
{
    if (libraryWindow == nullptr)
        libraryWindow = std::make_unique<ToolWindow> ("IR library", std::make_unique<IrLibraryPanel> (processorRef, [this] { updateIRButtons(); syncIRPages(); }), lookAndFeel);
    libraryWindow->setVisible (true);
    libraryWindow->toFront (true);
}

void StarterAmpEditor::openToneWindow()
{
    if (toneWindow == nullptr)
        toneWindow = std::make_unique<ToolWindow> ("Tone matching", std::make_unique<ToneMatchPanel> (processorRef), lookAndFeel);
    toneWindow->setVisible (true);
    toneWindow->toFront (true);
}

void StarterAmpEditor::openToneLabWindow()
{
    if (toneLabWindow == nullptr)
        toneLabWindow = std::make_unique<ToolWindow> ("Tone Lab", std::make_unique<ToneLabPanel> (processorRef), lookAndFeel);
    toneLabWindow->setVisible (true);
    toneLabWindow->toFront (true);
}

void StarterAmpEditor::openBrowserWindow()
{
    if (browserWindow == nullptr)
    {
        PresetBrowser::Callbacks cb;
        cb.loadFactory = [this] (int index) { loadFactoryPreset (index); presetBox.setSelectedId (1000 + index, juce::dontSendNotification); };
        cb.loadFile    = [this] (const juce::File& f) { loadPresetFile (f); refreshPresets (f.getFileNameWithoutExtension()); };
        cb.saveCurrent = [this] { savePresetDialog(); };
        cb.changed     = [this] { refreshPresets(); };
        cb.folder      = [] { return presetFolder(); };
        browserWindow = std::make_unique<ToolWindow> ("Preset browser", std::make_unique<PresetBrowser> (processorRef, cb), lookAndFeel);
    }
    browserWindow->setVisible (true);
    browserWindow->toFront (true);
}

void StarterAmpEditor::abMenuClicked()
{
    const int active = processorRef.abActive();
    const juce::String from = active == 0 ? "A" : "B", to = active == 0 ? "B" : "A";
    juce::PopupMenu menu;
    menu.addItem (1, "Copy " + from + " to " + to);
    menu.addItem (2, "Level match when switching", true, processorRef.abLevelMatch());
    menu.addItem (3, "Forget the measured levels");
    menu.addSeparator();
    const float trim = processorRef.abTrimDb();
    menu.addItem (4, "Level trim now: " + juce::String (trim, 1) + " dB  (play the same part on A and B)", false);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&abMenu), [this] (int result)
    {
        if (result == 1) processorRef.abCopyToOther();
        else if (result == 2) processorRef.abSetLevelMatch (! processorRef.abLevelMatch());
        else if (result == 3) processorRef.abForgetLevels();
    });
}


//==============================================================================
// ToneMatchPanel
//==============================================================================
ToneMatchPanel::ToneMatchPanel (StarterAmpProcessor& p) : processor (p)
{
    setSize (680, 500);
    loadButton.setTooltip ("Pick a recording of the tone you want (a WAV, AIFF or similar file; the first 90 seconds are measured). Guitar parts work best: a mix with drums and vocals pulls the result toward the whole mix.");
    listenButton.setTooltip ("While lit, the plugin measures your playing as it arrives at the Post EQ. Play a mix of chords and single notes for about ten seconds. Pause or press again to stop.");
    clearButton.setTooltip ("Throws away what was measured of your playing, so you can measure again.");
    applyButton.setTooltip ("Sets the nine Post EQ faders to the best fit and turns the EQ on. You can undo it (Cmd+Z) and fine-tune the faders by hand.");
    resetButton.setTooltip ("Sets all nine Post EQ faders back to 0 dB.");
    strength.setTooltip ("How much of the correction is applied. 100% = full match; lower is gentler.");
    listenButton.setClickingTogglesState (true);
    for (auto* b : { &loadButton, &listenButton, &clearButton, &applyButton, &resetButton }) addAndMakeVisible (b);
    loadButton.onClick   = [this] { chooseReference(); };
    listenButton.onClick = [this] { setListening (listenButton.getToggleState()); };
    clearButton.onClick  = [this] { clearYours(); };
    applyButton.onClick  = [this] { applyMatch(); };
    resetButton.onClick  = [this] { resetEq(); };
    strength.setSliderStyle (juce::Slider::LinearHorizontal);
    strength.setTextBoxStyle (juce::Slider::TextBoxRight, false, 56, 20);
    strength.setRange (0.0, 100.0, 1.0);
    strength.setValue (100.0, juce::dontSendNotification);
    strength.textFromValueFunction = [] (double v) { return juce::String ((int) v) + " %"; };
    strength.updateText();
    strength.onValueChange = [this] { repaint(); };
    strengthLabel.setText ("Strength", juce::dontSendNotification);
    strengthLabel.setColour (juce::Label::textColourId, ui::textDim);
    status.setColour (juce::Label::textColourId, ui::text);
    help.setColour (juce::Label::textColourId, ui::textDim);
    help.setFont (juce::Font (juce::FontOptions (12.0f)));
    help.setJustificationType (juce::Justification::topLeft);
    help.setText ("1. Load a reference recording.   2. Press LISTEN and play for about ten seconds (chords and single notes).   "
                  "3. Press APPLY TO EQ. Only the broad tonal balance is matched (the nine Post EQ bands): not the distortion, "
                  "dynamics, room or the recording's own EQ. Turn the amp or cab settings first, then match.",
                  juce::dontSendNotification);
    for (auto* l : { &strengthLabel, &status, &help }) addAndMakeVisible (l);
    addAndMakeVisible (strength);
    showReferenceFromState();
    updateStatus();
}

ToneMatchPanel::~ToneMatchPanel()
{
    stopTimer();
    processor.toneListening.store (false);
}

void ToneMatchPanel::visibilityChanged()
{
    if (isShowing()) startTimerHz (10);
    else { stopTimer(); processor.toneListening.store (false); listenButton.setToggleState (false, juce::dontSendNotification); }
}

void ToneMatchPanel::setListening (bool on)
{
    processor.toneListening.store (on);
    lastPos = processor.preEqTap.pos.load();
    samplesSince = 0;
    updateStatus();
}

void ToneMatchPanel::clearYours() { yours.reset(); samplesSince = 0; updateStatus(); repaint(); }

void ToneMatchPanel::feed (const float* frame)
{
    yours.addFrame (frame, processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000.0);
    updateStatus();
}

void ToneMatchPanel::timerCallback()
{
    if (listenButton.getToggleState())
    {
        const int pos = processor.preEqTap.pos.load (std::memory_order_acquire);
        samplesSince += (pos - lastPos) & (StarterAmpProcessor::AnalyserTap::size - 1);
        lastPos = pos;
        if (samplesSince >= ToneMatch::Analyzer::size / 2)
        {
            samplesSince = 0;
            std::vector<float> frame ((size_t) ToneMatch::Analyzer::size);
            processor.preEqTap.copyLatest (frame.data(), ToneMatch::Analyzer::size);
            feed (frame.data());
        }
        updateStatus();
        repaint();
    }
}

void ToneMatchPanel::updateStatus()
{
    juce::String s = haveRef ? "Reference: " + refName : "No reference loaded.";
    s << "     Your playing: ";
    if (yours.frames == 0) s << (listenButton.getToggleState() ? "waiting for sound..." : "not measured");
    else s << juce::String (yours.frames * ToneMatch::Analyzer::size / 2 / std::max (1.0, processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000.0), 0) << " s measured";
    status.setText (s, juce::dontSendNotification);
    applyButton.setEnabled (haveRef && yours.frames >= 20);
}

void ToneMatchPanel::showReferenceFromState()
{
    haveRef = ToneMatch::fromString (processor.apvts.state.getProperty ("toneRef").toString(), target);
    refName = processor.apvts.state.getProperty ("toneRefName").toString();
    if (refName.isEmpty()) refName = "(saved reference)";
}

bool ToneMatchPanel::loadReferenceFile (const juce::File& f)
{
    ToneMatch::Analyzer an;
    juce::String err;
    if (! ToneMatch::analyseFile (f, an, err))
    {
        status.setText (err, juce::dontSendNotification);
        return false;
    }
    target = an.shapeDb();
    haveRef = true;
    refName = f.getFileName();
    processor.apvts.state.setProperty ("toneRef", ToneMatch::toString (target), nullptr);
    processor.apvts.state.setProperty ("toneRefName", refName, nullptr);
    updateStatus();
    repaint();
    return true;
}

void ToneMatchPanel::chooseReference()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a reference recording", juce::File(), "*.wav;*.aif;*.aiff;*.mp3;*.m4a;*.aac;*.flac;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<ToneMatchPanel> (this)] (const juce::FileChooser& fc)
                          {
                              if (safe != nullptr && fc.getResult().existsAsFile())
                                  safe->loadReferenceFile (fc.getResult());
                          });
}

void ToneMatchPanel::resetEq()
{
    for (int b = 0; b < kEqBands; ++b)
        if (auto* prm = processor.apvts.getParameter ("eqB" + juce::String (b + 1)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (0.0f));
            prm->endChangeGesture();
        }
    repaint();
}

void ToneMatchPanel::applyMatch()
{
    if (! haveRef || yours.frames < 20) return;
    const double sr = processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000.0;
    const auto resp = ToneMatch::responses (sr, processor.apvts.getRawParameterValue ("eqMode")->load() > 0.5f,
                                            processor.apvts.getRawParameterValue ("eqQ")->load());
    const auto mine = yours.shapeDb();
    std::array<float, ToneMatch::kBands> wanted {};
    for (int k = 0; k < ToneMatch::kBands; ++k) wanted[(size_t) k] = target[(size_t) k] - mine[(size_t) k];
    lastFit = ToneMatch::fit (wanted, resp, (float) strength.getValue() / 100.0f);
    for (int b = 0; b < kEqBands; ++b)
        if (auto* prm = processor.apvts.getParameter ("eqB" + juce::String (b + 1)))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (lastFit[(size_t) b]));
            prm->endChangeGesture();
        }
    if (auto* on = processor.apvts.getParameter ("eqOn"))
    {
        on->beginChangeGesture();
        on->setValueNotifyingHost (1.0f);
        on->endChangeGesture();
    }
    repaint();
}

juce::Rectangle<float> ToneMatchPanel::graphArea() const
{
    return juce::Rectangle<float> (20.0f, 80.0f, (float) getWidth() - 40.0f, 280.0f);
}

void ToneMatchPanel::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
    auto r = graphArea();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (r, 6.0f);
    const float range = 18.0f;
    auto xOf = [&] (float hz) { return r.getX() + r.getWidth() * std::log (hz / 50.0f) / std::log (12700.0f / 50.0f); };
    auto yOf = [&] (float db) { return r.getCentreY() - juce::jlimit (-range, range, db) / range * (r.getHeight() * 0.5f - 6.0f); };
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    for (int db = -12; db <= 12; db += 6)
    {
        g.setColour (db == 0 ? ui::panelEdge.brighter (0.3f) : ui::panelEdge);
        g.drawHorizontalLine ((int) yOf ((float) db), r.getX(), r.getRight());
        g.setColour (ui::textDim);
        g.drawText (juce::String (db), (int) r.getX() + 2, (int) yOf ((float) db) - 11, 26, 11, juce::Justification::left);
    }
    for (float hz : { 100.0f, 1000.0f, 10000.0f })
    {
        g.setColour (ui::panelEdge);
        g.drawVerticalLine ((int) xOf (hz), r.getY(), r.getBottom());
        g.setColour (ui::textDim);
        g.drawText (hz >= 1000.0f ? juce::String ((int) (hz / 1000)) + "k" : juce::String ((int) hz), (int) xOf (hz) + 2, (int) r.getBottom() - 13, 30, 12, juce::Justification::left);
    }
    auto trace = [&] (const std::array<float, ToneMatch::kBands>& v, juce::Colour c, float width)
    {
        juce::Path path;
        for (int k = 0; k < ToneMatch::kBands; ++k)
        {
            const float x = xOf (ToneMatch::bandHz (k)), y = yOf (v[(size_t) k]);
            if (k == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved));
    };
    std::array<float, ToneMatch::kBands> mine {}, predicted {};
    if (yours.frames > 0) mine = yours.shapeDb();
    if (haveRef) trace (target, ui::accent, 2.0f);
    if (yours.frames > 0) trace (mine, ui::colourB, 2.0f);
    if (haveRef && yours.frames >= 20)
    {
        const double sr = processor.getSampleRate() > 0 ? processor.getSampleRate() : 48000.0;
        const auto resp = ToneMatch::responses (sr, processor.apvts.getRawParameterValue ("eqMode")->load() > 0.5f,
                                                processor.apvts.getRawParameterValue ("eqQ")->load());
        std::array<float, ToneMatch::kBands> wanted {};
        for (int k = 0; k < ToneMatch::kBands; ++k) wanted[(size_t) k] = target[(size_t) k] - mine[(size_t) k];
        const auto curve = ToneMatch::eqCurve (ToneMatch::fit (wanted, resp, (float) strength.getValue() / 100.0f), resp);
        float shift = 0.0f;
        for (int k = 0; k < ToneMatch::kBands; ++k) { predicted[(size_t) k] = mine[(size_t) k] + curve[(size_t) k]; shift += target[(size_t) k] - predicted[(size_t) k]; }
        shift /= ToneMatch::kBands;
        for (auto& v : predicted) v += shift;
        trace (predicted, ui::colourD, 1.5f);
    }
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    int lx = (int) r.getX() + 36;
    for (auto [name, c] : { std::pair<const char*, juce::Colour> { "Reference", ui::accent }, { "Your playing", ui::colourB }, { "After the EQ (preview)", ui::colourD } })
    {
        g.setColour (c);
        g.fillRect (lx, (int) r.getY() + 8, 14, 3);
        g.setColour (ui::textDim);
        g.drawText (name, lx + 18, (int) r.getY() + 2, 150, 14, juce::Justification::left);
        lx += 140;
    }
}

void ToneMatchPanel::resized()
{
    auto r = getLocalBounds().reduced (20, 14);
    auto top = r.removeFromTop (30);
    loadButton.setBounds (top.removeFromLeft (150));
    top.removeFromLeft (8);
    listenButton.setBounds (top.removeFromLeft (80));
    top.removeFromLeft (8);
    clearButton.setBounds (top.removeFromLeft (90));
    status.setBounds (r.removeFromTop (30).withTrimmedTop (6));
    r.removeFromTop (6);
    r.removeFromTop (280);                    // the graph
    auto row = r.removeFromTop (30);
    strengthLabel.setBounds (row.removeFromLeft (70));
    strength.setBounds (row.removeFromLeft (230));
    row.removeFromLeft (14);
    applyButton.setBounds (row.removeFromLeft (130));
    row.removeFromLeft (8);
    resetButton.setBounds (row.removeFromLeft (100));
    r.removeFromTop (8);
    help.setBounds (r.removeFromTop (60));
}


//==============================================================================
// ToneLabPanel
//==============================================================================
struct ToneLabPanel::Worker : public juce::Thread
{
    Worker (ToneLabPanel& o) : juce::Thread ("Tone Lab"), owner (o) {}
    void run() override
    {
        owner.result = owner.fitter->run ([this] (float p, const juce::String& t)
                                          {
                                              const juce::ScopedLock l (owner.lock);
                                              owner.progressValue = p;  owner.progressText = t;
                                          }, owner.cancel);
        owner.finished.store (true);
    }
    ToneLabPanel& owner;
};

ToneLabPanel::ToneLabPanel (StarterAmpProcessor& p) : processor (p)
{
    setSize (720, 720);
    diButton.setTooltip ("A clean recording of your guitar before any amp or effect (a DI track). Up to 120 seconds are read and the loudest 7 seconds are used. Chords, palm mutes and single notes together work best.");
    targetButton.setTooltip ("A recording of the sound you want: the same kind of playing, after the amp, pedals and cab you want to sound like. It does not need to be the same take, but similar playing helps. Avoid files with drums, vocals or reverb-heavy mixes.");
    startButton.setTooltip ("Starts the search. It takes about a minute. Nothing in your sound changes until you press APPLY.");
    cancelButton.setTooltip ("Stops the search and keeps the best result found so far.");
    applyButton.setTooltip ("Puts the found settings on your plugin. UNDO puts everything back.");
    undoButton.setTooltip ("Puts the settings back the way they were before APPLY.");
    optAmp.setTooltip ("Let it try every amp model and voicing, then fine-tune gain and tone.");
    optCab.setTooltip ("Let it try the speakers, mics and cabinets of cab block A (only when block A uses a built-in model).");
    optPedals.setTooltip ("Let it try switching the compressor, overdrive and boost on or off and setting how hard they work.");
    optEq.setTooltip ("At the end, fit the nine Post EQ faders to whatever difference is left.");
    for (auto* t : { &optAmp, &optCab, &optPedals, &optEq }) { t->setToggleState (true, juce::dontSendNotification); addAndMakeVisible (t); }
    for (auto* b : { &diButton, &targetButton, &startButton, &cancelButton, &applyButton, &undoButton }) addAndMakeVisible (b);
    for (auto* l : { &diLabel, &targetLabel, &status, &summary, &help }) addAndMakeVisible (l);
    addAndMakeVisible (bar);
    diButton.onClick = [this] { chooseFile (true); };
    targetButton.onClick = [this] { chooseFile (false); };
    startButton.onClick = [this] { start(); };
    cancelButton.onClick = [this] { cancel.store (true); };
    applyButton.onClick = [this] { applyResult(); };
    undoButton.onClick = [this] { undoApply(); };
    diLabel.setColour (juce::Label::textColourId, ui::text);
    targetLabel.setColour (juce::Label::textColourId, ui::text);
    status.setColour (juce::Label::textColourId, ui::text);
    summary.setColour (juce::Label::textColourId, ui::text);
    summary.setJustificationType (juce::Justification::topLeft);
    summary.setFont (juce::Font (juce::FontOptions (12.5f)));
    help.setColour (juce::Label::textColourId, ui::textDim);
    help.setJustificationType (juce::Justification::topLeft);
    help.setFont (juce::Font (juce::FontOptions (12.0f)));
    help.setText ("1. Load a clean DI recording of your guitar.   2. Load a recording of the sound you want (the same kind of playing, "
                  "through the amp and pedals you like).   3. Press START MATCH and wait about a minute.   4. Look at the result, then APPLY. "
                  "It matches the tonal balance and the dynamics (how squashed or open it is). It does not match loudness, reverb, delay or "
                  "modulation, and it cannot copy a particular amp exactly: it finds settings here that sound close. Only use recordings you may use.",
                  juce::dontSendNotification);
    diLabel.setText ("No DI loaded", juce::dontSendNotification);
    targetLabel.setText ("No target loaded", juce::dontSendNotification);
    const auto savedDi = processor.apvts.state.getProperty ("labDiPath").toString();
    const auto savedTarget = processor.apvts.state.getProperty ("labTargetPath").toString();
    if (savedDi.isNotEmpty() && juce::File (savedDi).existsAsFile()) loadFile (true, juce::File (savedDi));
    if (savedTarget.isNotEmpty() && juce::File (savedTarget).existsAsFile()) loadFile (false, juce::File (savedTarget));
    updateButtons();
    startTimerHz (10);
}

ToneLabPanel::~ToneLabPanel()
{
    stopTimer();
    cancel.store (true);
    if (worker != nullptr) worker->stopThread (20000);
}

void ToneLabPanel::chooseFile (bool di)
{
    chooser = std::make_unique<juce::FileChooser> (di ? "Choose your clean DI recording" : "Choose the recording of the sound you want",
                                                   juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.m4a;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<ToneLabPanel> (this), di] (const juce::FileChooser& fc)
                          {
                              if (safe != nullptr && fc.getResult().existsAsFile())
                                  safe->loadFile (di, fc.getResult());
                          });
}

bool ToneLabPanel::loadFile (bool di, const juce::File& f)
{
    std::vector<float> mono;
    double sr = 48000.0;
    juce::String err;
    if (! ToneLab::loadMono (f, mono, sr, err)) { status.setText (err, juce::dontSendNotification); return false; }
    if (di)
    {
        double e = 0.0;
        for (float v : mono) e += (double) v * v;
        if (std::sqrt (e / (double) mono.size()) < 1.0e-5) { status.setText ("The DI file is silent.", juce::dontSendNotification); return false; }
        diMono = std::move (mono);  diSr = sr;  haveDi = true;  diName = f.getFileName();
        diLabel.setText ("DI: " + diName + "  (" + juce::String ((double) diMono.size() / diSr, 1) + " s)", juce::dontSendNotification);
        processor.apvts.state.setProperty ("labDiPath", f.getFullPathName(), nullptr);
    }
    else
    {
        auto feat = ToneLab::analyse (mono, sr);
        if (! feat.ok) { status.setText ("The target file is too short or too quiet to measure.", juce::dontSendNotification); return false; }
        targetFeat = feat;  haveTarget = true;  targetName = f.getFileName();
        targetLabel.setText ("Target: " + targetName + "  (" + juce::String ((double) mono.size() / sr, 1) + " s)", juce::dontSendNotification);
        processor.apvts.state.setProperty ("labTargetPath", f.getFullPathName(), nullptr);
    }
    haveResult = false;  applied = false;
    status.setText ({}, juce::dontSendNotification);
    updateButtons();
    repaint();
    return true;
}

void ToneLabPanel::updateButtons()
{
    const bool run = running.load();
    startButton.setEnabled (haveDi && haveTarget && ! run);
    cancelButton.setEnabled (run);
    diButton.setEnabled (! run);
    targetButton.setEnabled (! run);
    for (auto* t : { &optAmp, &optCab, &optPedals, &optEq }) t->setEnabled (! run);
    applyButton.setEnabled (haveResult && ! run && ! result.changes.empty() && ! applied);
    undoButton.setEnabled (applied && ! run);
}

void ToneLabPanel::start()
{
    if (running.load() || ! haveDi || ! haveTarget) return;
    probe = std::make_unique<StarterAmpProcessor>();
    juce::MemoryBlock state;
    processor.getStateInformation (state);
    probe->setStateInformation (state.getData(), (int) state.getSize());
    probe->setPlayConfigDetails (2, 2, diSr, 512);
    probe->prepareToPlay (diSr, 512);
    ToneLab::Options o;
    o.amp = optAmp.getToggleState();  o.cab = optCab.getToggleState();  o.pedals = optPedals.getToggleState();  o.eq = optEq.getToggleState();
    fitter = std::make_unique<ToneLab::Fitter> (*probe, ToneLab::loudestExcerpt (diMono, diSr, 7.0), diSr, targetFeat, o);
    cancel.store (false);  finished.store (false);  running.store (true);
    haveResult = false;  applied = false;
    summary.setText ({}, juce::dontSendNotification);
    { const juce::ScopedLock l (lock); progressValue = 0.0f; progressText = "Starting..."; }
    worker = std::make_unique<Worker> (*this);
    worker->startThread();
    updateButtons();
}

void ToneLabPanel::timerCallback()
{
    if (running.load())
    {
        const juce::ScopedLock l (lock);
        progressBarValue = (double) progressValue;
        status.setText (progressText, juce::dontSendNotification);
        if (finished.load()) { /* handled below, outside the lock */ }
    }
    if (running.load() && finished.load())
        finish();
}

void ToneLabPanel::finish()
{
    if (worker != nullptr) { worker->stopThread (20000); worker.reset(); }
    running.store (false);
    fitter.reset();
    probe.reset();
    haveResult = result.ok;
    progressBarValue = 1.0;
    juce::String s;
    if (! result.ok) s = result.message;
    else
    {
        s << "Tonal difference from the target:  " << juce::String (ToneLab::shapeDistance (result.target, result.start), 1) << " dB before,  "
          << juce::String (ToneLab::shapeDistance (result.target, result.end), 1) << " dB after.\n"
          << "Peak-to-average (crest):  target " << juce::String (result.target.crestDb, 1) << " dB,  you before " << juce::String (result.start.crestDb, 1)
          << " dB,  after " << juce::String (result.end.crestDb, 1) << " dB.\n";
        if (result.message.isNotEmpty()) s << result.message << "\n";
        if (result.changes.empty()) s << "No change got closer to the target, so nothing is suggested.";
        else
        {
            s << "Changes it found:\n";
            for (auto& n : result.notes) s << "   " << n << "\n";
        }
    }
    summary.setText (s.replace ("\\n", "\n"), juce::dontSendNotification);
    status.setText (result.ok ? "Done. Press APPLY TO MY SOUND to use these settings." : "", juce::dontSendNotification);
    updateButtons();
    repaint();
}

void ToneLabPanel::applyResult()
{
    if (! haveResult || applied) return;
    previous.clear();
    for (auto& c : result.changes)
        if (auto* prm = processor.apvts.getParameter (c.first))
        {
            previous.push_back ({ c.first, prm->convertFrom0to1 (prm->getValue()) });
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (c.second));
            prm->endChangeGesture();
        }
    applied = true;
    status.setText ("Applied. UNDO puts your old settings back.", juce::dontSendNotification);
    updateButtons();
}

void ToneLabPanel::undoApply()
{
    if (! applied) return;
    for (auto& c : previous)
        if (auto* prm = processor.apvts.getParameter (c.first))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (c.second));
            prm->endChangeGesture();
        }
    applied = false;
    status.setText ("Your old settings are back.", juce::dontSendNotification);
    updateButtons();
}

juce::Rectangle<float> ToneLabPanel::graphArea() const
{
    return juce::Rectangle<float> (20.0f, 310.0f, (float) getWidth() - 40.0f, 170.0f);
}

void ToneLabPanel::paint (juce::Graphics& g)
{
    g.fillAll (ui::background);
    auto r = graphArea();
    g.setColour (ui::panel);
    g.fillRoundedRectangle (r, 6.0f);
    const float range = 15.0f;
    auto xOf = [&] (float hz) { return r.getX() + r.getWidth() * std::log (hz / 50.0f) / std::log (12700.0f / 50.0f); };
    auto yOf = [&] (float db) { return r.getCentreY() - juce::jlimit (-range, range, db) / range * (r.getHeight() * 0.5f - 6.0f); };
    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    for (int db = -10; db <= 10; db += 5)
    {
        g.setColour (db == 0 ? ui::panelEdge.brighter (0.3f) : ui::panelEdge);
        g.drawHorizontalLine ((int) yOf ((float) db), r.getX(), r.getRight());
        g.setColour (ui::textDim);
        g.drawText (juce::String (db), (int) r.getX() + 2, (int) yOf ((float) db) - 11, 26, 11, juce::Justification::left);
    }
    for (float hz : { 100.0f, 1000.0f, 10000.0f })
    {
        g.setColour (ui::panelEdge);
        g.drawVerticalLine ((int) xOf (hz), r.getY(), r.getBottom());
        g.setColour (ui::textDim);
        g.drawText (hz >= 1000.0f ? juce::String ((int) (hz / 1000)) + "k" : juce::String ((int) hz), (int) xOf (hz) + 2, (int) r.getBottom() - 13, 30, 12, juce::Justification::left);
    }
    auto trace = [&] (const std::array<float, ToneMatch::kBands>& v, juce::Colour c, float width)
    {
        juce::Path path;
        for (int k = 0; k < ToneMatch::kBands; ++k)
        {
            const float x = xOf (ToneMatch::bandHz (k)), y = yOf (v[(size_t) k]);
            if (k == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (c);
        g.strokePath (path, juce::PathStrokeType (width, juce::PathStrokeType::curved));
    };
    if (haveTarget) trace (targetFeat.shape, ui::accent, 2.0f);
    if (haveResult)
    {
        trace (result.start.shape, ui::colourB.withAlpha (0.7f), 1.5f);
        trace (result.end.shape, ui::colourD, 2.0f);
    }
    int lx = (int) r.getX() + 36;
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    for (auto [name, c] : { std::pair<const char*, juce::Colour> { "Target", ui::accent }, { "Yours before", ui::colourB }, { "Yours after", ui::colourD } })
    {
        g.setColour (c);
        g.fillRect (lx, (int) r.getY() + 8, 14, 3);
        g.setColour (ui::textDim);
        g.drawText (name, lx + 18, (int) r.getY() + 2, 120, 14, juce::Justification::left);
        lx += 130;
    }
}

void ToneLabPanel::resized()
{
    auto r = getLocalBounds().reduced (20, 14);
    help.setBounds (r.removeFromTop (92));
    auto row = r.removeFromTop (30);
    diButton.setBounds (row.removeFromLeft (130));
    diLabel.setBounds (row.withTrimmedLeft (10));
    r.removeFromTop (6);
    row = r.removeFromTop (30);
    targetButton.setBounds (row.removeFromLeft (130));
    targetLabel.setBounds (row.withTrimmedLeft (10));
    r.removeFromTop (8);
    auto o1 = r.removeFromTop (24);
    optAmp.setBounds (o1.removeFromLeft (330));
    optCab.setBounds (o1);
    auto o2 = r.removeFromTop (24);
    optPedals.setBounds (o2.removeFromLeft (330));
    optEq.setBounds (o2);
    r.removeFromTop (8);
    row = r.removeFromTop (30);
    startButton.setBounds (row.removeFromLeft (140));
    row.removeFromLeft (8);
    cancelButton.setBounds (row.removeFromLeft (90));
    row.removeFromLeft (16);
    applyButton.setBounds (row.removeFromLeft (170));
    row.removeFromLeft (8);
    undoButton.setBounds (row.removeFromLeft (80));
    r.removeFromTop (6);
    bar.setBounds (r.removeFromTop (16));
    status.setBounds (r.removeFromTop (22));
    r.removeFromTop (180);                       // the graph
    summary.setBounds (r.reduced (0, 6));
}


//==============================================================================
// NAM model loading from the interface
//==============================================================================
void StarterAmpEditor::updateNamInfo (int slot)
{
    auto& ns = namStrip[(size_t) slot];
    ns.lastText = processorRef.getNamSummary (slot);
    ns.info.setColour (juce::Label::textColourId, ui::textDim);
    ns.info.setText (ns.lastText.isEmpty() ? juce::String ("No model loaded. Press LOAD MODEL and pick a .nam file (WaveNet or LSTM captures).") : ns.lastText,
                     juce::dontSendNotification);
}

void StarterAmpEditor::chooseNamFile (int slot)
{
    const juce::File last (processorRef.apvts.state.getProperty (StarterAmpProcessor::namPathKey (slot)).toString());
    namChooser = std::make_unique<juce::FileChooser> ("Choose a .nam model", last.existsAsFile() ? last.getParentDirectory() : juce::File(), "*.nam");
    namChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                             [safe = juce::Component::SafePointer<StarterAmpEditor> (this), slot] (const juce::FileChooser& fc)
                             {
                                 if (safe != nullptr && fc.getResult().existsAsFile())
                                     safe->loadNamModel (slot, fc.getResult());
                             });
}

void StarterAmpEditor::loadNamModel (int slot, const juce::File& file)
{
    static const char* const onIds[4] = { "namOn", "namBOn", "nmP1On", "nmP2On" };
    auto& ns = namStrip[(size_t) slot];
    juce::String message;
    const bool ok = processorRef.loadNamFile (slot, file, message);
    ns.info.setColour (juce::Label::textColourId, ok ? ui::textDim : juce::Colour (0xffff7a6b));
    ns.info.setText (ok ? message : "Could not load " + file.getFileName() + ": " + message, juce::dontSendNotification);
    if (ok)
    {
        ns.lastText = processorRef.getNamSummary (slot);
        if (auto* on = processorRef.apvts.getParameter (onIds[slot]))     // loading a model means you want to hear it
        {
            on->beginChangeGesture();
            on->setValueNotifyingHost (1.0f);
            on->endChangeGesture();
        }
    }
}
