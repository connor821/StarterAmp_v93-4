# Distressor, SSL bus clone and Tascam 424 documents versus the plugin

Compared: `Distressor_manual.md`, `SSL_Mixbus_clone.md`, `Tascam424_service.md` (this folder) against `MODELED_GEAR.md` (Compressor tables near line 356, "Compressor audit" b59 at line 362), `CHANGELOG.md` (b58/b59 and b62), `CAPTURE_COMPARISON_b62.md` (section 3, Distressor captures), `DATASHEET_AUDIT_b58.md` (style table, line 242) and the code: `Source/PluginProcessor.h` (class `CompressorEffect`, lines 2692 to 2960), `Source/PluginProcessor.cpp` (compressor parameters, lines 2985 to 3013; call at 6270), `Source/BoostStage.h` (Tape mode, lines 237 to 239, 315 to 320, 332), `Source/LoFi.h` (Cassette voicing, lines 59 to 160). No code or existing document was changed.

How the numbers marked "calc" were obtained: I re-implemented the plugin's own formulas (the shaper lines 2898 to 2928, the RBJ filters of BoostStage.h) in a throwaway script and measured them with a sine (THD by FFT). They describe the code, not a listening test. 0 dBFS is taken as the plugin's internal full scale; where the guitar sits in that scale is not known from the code (the comment at line 2901 says "guitar signals are small").

How the plugin maps to these units:
- The "Standard" Compressor style (style 0) is the Distressor idea (MODELED_GEAR line 356). Same eight ratio entries as the Distressor's ratio button: 1:1, 2, 3, 4, 6, 10, 20, Limit (= 100:1, line 2724; Distressor: 1:1, 2:1, 3:1, 4:1, 6:1, 10:1 Opto, 20:1, Nuke).
- Style 5 "Bus" is the SSL-style clone (MODELED_GEAR line 362, b59).
- Boost mode 7 "Tape" (BoostStage.h) and Lo-Fi "Cassette" voicing (LoFi.h) are the Tascam 424 idea (CHANGELOG b62).

## 1. Distressor manual versus Standard compressor

| # | Quantity | Distressor manual | Plugin (location) | Verdict |
|---|---|---|---|---|
| 1 | Ratio list | 1:1, 2, 3, 4, 6, 10 (Opto), 20, Nuke | 1, 2, 3, 4, 6, 10, 20, Limit (PluginProcessor.h:2698, 2724; menu at .cpp:2989) | **Agrees** (Limit 100:1 stands in for Nuke; both hold normal signals within 1 dB: 20:1 gives 1 dB per 20 dB over) |
| 2 | Attack range | 50 us to 30 ms | knob 0.05 to 50 ms (.cpp:2991), floor 0.02 ms (h:2744), default 5 ms | **Agrees at the fast end (0.05 ms = 50 us exactly)**; the slow end is 50 ms against 30 ms (the knob goes 20 ms further than the hardware) |
| 3 | Release range | 50 ms to 3.5 s (spec; text says 3 s) | knob 50 to 3500 ms (.cpp:2992), default 250 ms | **Agrees exactly** |
| 4 | Release shape | per-ratio; Nuke logarithmic: fast first, then slow | single pole in the dB domain (h:2886 onward), which is also fast-first, slow-later; style 0 has no program dependence (`f = 1` for style 0, h:2847 to 2858 apply only to styles 1 to 5) | **Agrees in kind** for Nuke; no per-ratio release shapes |
| 5 | Knee | **2:1: "+15 dB knee", up to 30 dB depending on attack/release; 3:1 and 4:1 "very long"; 6:1 and 10:1 shorter; 20:1 and Nuke hard** | one fixed **6 dB** quadratic knee, centred on the threshold, for every ratio except Limit (hard) (h:2742, gain computer h:2836 to 2845) | **Disagrees.** Constant 6 dB everywhere. Also the plugin's knee is symmetric about the threshold (6 dB = 3 dB above), while the Distressor's "+15 dB" runs above its threshold. The 2:1 soft knee is already listed as "not modelled" (MODELED_GEAR line 362) |
| 6 | Detector high-pass | one detector HP; frequency **not given** | Off / 80 / 160 / 320 Hz, one-pole (h:2784, 2823 to 2826), default 80 Hz (.cpp:3000) | **Agrees in concept**; frequency choices are the plugin's (the manual gives none), and the one-pole is 6 dB/octave (the manual does not give a slope either) |
| 7 | Audio-path high-pass | **80 Hz, 3rd-order Bessel, -3 dB at 65 Hz, -12 dB at 30 Hz, 18 dB/octave, in the audio path, separate from the detector HP** | none (the plugin's 80 Hz option is detector-only) | **Missing.** Easy to add (Bessel3 high-pass; -1.9 dB at 80 Hz, -5.3 dB at 50 Hz, -13.6 dB at 30 Hz computed) |
| 8 | Detector band emphasis | **6 kHz band** emphasis; amount and Q not given | none | **Missing** (already listed as not modelled) |
| 9 | Distressor Opto (10:1 position) | separate detector; release up to **20 s**; attack above 4 on the knob | the separate **Opto** style (style 2), two-stage release; its slow stage reaches about 5 s (complete release at default Release) to over a minute at Release = 3.5 s (calc: tau 0.11 to 1.11 times Release/250 ms seconds) | Different structure; the 10:1 button on Standard is a plain 10:1. Not a defect |
| 10 | Dist 2 | THD 0.05 to 3%; emphasised 2nd, "especially while compressing" | asymmetric tanh, bias 0.3, `k = 1 + 2 x depth`, pre 3 (h:2793, 2898 to 2920); **calc: 0.6% at -40 dBFS peak, 1.9% at -30, 3.8% at -24, 5.8% at -20, 12% at -12 dBFS, with no gain reduction at all (always on)**; 2nd harmonic -34 dB at -30 dBFS | **Disagrees at normal levels**: sits at or above the manual's 3% maximum for any input above about -26 dBFS. Direction (even harmonics) agrees |
| 11 | Dist 3 | THD 0.1 to 20%; 3rd harmonic "along with 2nd"; the capture of the red-line setting has H2 -35 dB, H3 -12 dB (CAPTURE_COMPARISON_b62 row) | symmetric tanh, `k = 1 + 3 x depth` (h:2909 to 2913); calc: 0.02% at -40, 0.19% at -30, 0.75% at -24, 1.8% at -20, 9% at -12, 21% at -6 dBFS; H2 absent (-155 dB) unless "Both" is chosen | **Agrees in range** (0.02 to about 25% against 0.1 to 20%); **disagrees in spectrum**: the real Dist 3 has 2nd harmonic with the 3rd (-35 dB against -12 dB in the capture), the Dist 3 option has none |
| 12 | Distortion versus gain reduction | grows with VCA output level; LEDs light most in Dist 3; "stronger while compressing" | the shaper is after the gain control and its strength rises with gain reduction (h:2900, `depth = 0.2 + grDb/15`), but the signal into it falls by the same dB: **calc: at -12 dBFS input, Dist 3 THD is 9.0% at 0 dB GR, 8.6% at 3, 7.3% at 6, 4.2% at 12, 1.4% at 20 dB** (it falls) | **Disagrees with the code comment's claim "stronger with more gain reduction"** at a fixed input; with a threshold in use THD still rises with input (calc, threshold -18 dB, 6:1: Dist 3 2.8% at -18, 8.8% at -12, 17% at -6, 24.5% at 0 dBFS), so the net behaviour is level-driven, not compression-driven |
| 13 | Distortion order | VCA, then distortion generator, then 80 Hz HP, then Output gain (block diagram) | gain control, shaper, makeup (h:2894 to 2940) | **Agrees** (distortion before output gain) |
| 14 | "Normal" mode | THD 0.025 to 0.3% | Saturation Off = exactly clean (default is Tube-like, .cpp:3002) | Agrees (cleaner than the real one); the default is on for every style |
| 15 | Detector type and domain | not stated | power average 1.5 ms (h:2778), feed-forward | cannot compare |
| 16 | Stereo / Image link | two units; summing detector, image link locks gain reduction | one mono channel copied to the other (.cpp:6270 to 6279) | n/a for a mono guitar chain |

## 2. SSL bus clone versus the Bus style

| # | Quantity | Clone document | Plugin (location) | Verdict |
|---|---|---|---|---|
| 1 | Ratios | 2, 4, 10 | full menu 1 to Limit (h:2698); Bus shares it | Superset; fine |
| 2 | Attack | 0.1, 0.3, 1, 3, 10, 30 ms | continuous knob 0.05 to 50 ms; Bus floor 0.1 ms, scale 1 (h:2762) | **Agrees** (the knob covers the published range) |
| 3 | Release | 0.1, 0.3, 0.6, 1.2 s, Auto (nominal 2.4 s; two constants) | knob 50 to 3500 ms times a program factor 0.5 to 3.0 (h:2854, `0.5 + 2.5 x held`) | **Agrees in kind**; the Auto "two constants, short for peaks, slow for long compression" is what `held` does. The text's own numbers (91k x 6.8 uF = 0.62 s, 750k x 0.47 uF = 0.35 s) do not match "2.4 s", so the Auto values cannot be checked |
| 4 | Detector filter | **none mentioned** (flat sidechain from the 47k summing; no sidechain HP in the text) | detector HP default **80 Hz** for every style (.cpp:3000) | **Disagrees by default** for Bus (the document describes no filter; the SSL G-series guide in `reference/compressors/sources` may add one on its own units, not checked here) |
| 5 | Topology | combination of feed-forward and feed-back, "acting mostly as a feed-back compressor"; detector after the gain element | feed-forward (h:2820 to 2845: level from the input signal `in`) | **Disagrees in structure.** Feed-back detection gives a lower effective ratio and a slower, more "forgiving" onset than a feed-forward detector at the same settings. Not corrected anywhere |
| 6 | Threshold law | ratio switch changes the threshold "a bit" (amount unknown); threshold knob = sidechain gain | independent Threshold | already listed as not modelled; amount cannot be set from the text |
| 7 | Knee | not given | 10 dB (h:2762) | cannot compare |
| 8 | Make-up | 0 to 20 dB | output -12 to +18 dB, Auto Level | n/a |
| 9 | Distortion | -75 dB (0.018%), mostly 2nd | default Comp Saturation = Tube-like for every style (.cpp:3002); calc 0.6% to 6% at -40 to -20 dBFS (Dist 2 numbers above) | **Disagrees by default** for Bus: 30 to 300 times the clone's figure at ordinary levels. Setting Saturation Off gives the clean unit |
| 10 | Threshold range | -20 to +20 dBm (40 dB) | -40 to 0 dB (40 dB) | same width |
| 11 | Frequency response | below 15 Hz to above 35 kHz | not applicable | n/a |

## 3. Tascam 424 documents versus the Tape boost mode and the Cassette voicing

What the 424 documents say, short (details in `Tascam424_service.md`): the **mixer channel is flat from 20 Hz to 30 kHz (+/-3 dB), THD 0.04 to 0.06%, with two 100 Hz / 10 kHz shelving EQs of +/-10 dB**; the **tape section** has the playback equalisation (3.3 ms LF constant = 48 Hz corner, HF 39 to 84 us (or to 104 us) with a 70 us target), 87.5 kHz bias, dbx Type II, and an **overall record/play response of 40 Hz to 16 kHz +/-3 dB at 9.5 cm/s (HIGH) and 40 Hz to 12.5 kHz +/-3 dB at 4.8 cm/s (NORM)**; wow and flutter 0.05% WRMS, +/-1.0% peak (HIGH); S/N 55 to 58 dB without dbx, 90 to 95 dB with.

Response of the plugin's Tape boost mode (calc, small signal, BoostStage.h:239: low shelf 90 Hz, +2.5 + 3 x sat dB; high-pass 45 Hz Q 0.7; low-pass 9000 - 3000 x sat Hz Q 0.6; low-pass 12000 - 2500 x sat Hz Q 0.5; all four before or after the shaper as the code shows):

| Sat (Boost saturation 0..1) | Low shelf | LF peak (relative to 1 kHz) | -3 dB point, top | -10 dB point, top |
|---|---|---|---|---|
| 0.0 | +2.5 dB at 90 Hz | +0.9 dB at about 90 Hz; -3 dB at about 35 Hz | **6.1 kHz** | 10.1 kHz |
| 0.3 | +3.4 dB | +1.2 dB near 100 Hz | 5.5 kHz | 9.3 kHz |
| 1.0 | +5.5 dB | +2.1 to +3.3 dB at 60 to 100 Hz | **4.2 kHz** | 7.2 kHz |

Response of the Cassette voicing in Lo-Fi (calc, LoFi.h:69, 86 to 88, 144 to 150): one 2nd-order low-pass plus one one-pole at fc = 2000 x 4.5^Tone Hz (Tone 0..1 gives 2 to 9 kHz), one-pole high-pass 35 Hz, and a head bump of 0.45 x (LP 130 Hz - LP 45 Hz): **+1.7 dB peak at 76 Hz**, +1.4 dB at 45 and 130 Hz, +0.6 dB at 20 Hz and 300 Hz. The -3 dB point relative to 1 kHz is about 1.7 kHz (Tone 0), 3.2 kHz (0.5), 6.6 kHz (1.0). Flutter: 8 Hz component, excursion 0.0012 s x Drift in cassette mode (LoFi.h:64, 100 to 103), i.e. about 6% peak pitch at Drift = 1 (calc 2 pi x 8 Hz x 1.2 ms), so Drift of about 0.17 corresponds to the 424's +/-1.0% peak (HIGH). Hiss: `0.025 x Noise^2` through a one-pole at 6 kHz, rms about -29 dBFS at Noise = 1 (calc) against the 424's S/N of 55 to 58 dB without dbx.

| # | Item | 424 documents | Plugin | Verdict |
|---|---|---|---|---|
| 1 | What the "channel" is | The 424 channel itself is clean and flat to 30 kHz; the dull top end belongs to the tape section (and its record/play response) | BoostStage.h:237 comment and the Tape tooltip (PluginEditor.cpp:1241) call the filters "a cassette four-track's channel ... the cassette's top roll-off" | **Mismatch in description**: the response is tape, not channel. The channel's own colour is the +/-10 dB two-band EQ (derived corners: LOW half-gain 265 Hz, HIGH half-gain 6.8 kHz) |
| 2 | Top roll-off | -3 dB at **16 kHz (HIGH) / 12.5 kHz (NORM)** (spec); the dbx-out mask is within +/-3 dB to 10 kHz and -4 dB at 14 kHz (HIGH) | Tape boost: -3 dB at **6.1 kHz falling to 4.2 kHz** as Sat rises; Cassette: -3 dB at **1.7 to 6.6 kHz** | **Disagrees**: the plugin is 2 to 4 times lower in frequency than the 424's own response limits. (Part of this is the intent "worn/dark cassette"; the 424 at normal level is brighter; at higher levels real tape also loses top.) |
| 3 | Rumble/low end | -3 dB at **40 Hz** (spec, overall); 3180 us playback boost below 48 Hz | HP 45 Hz Q 0.7 (BoostStage.h:239): -3 dB near 35 Hz (calc, with the shelf); Cassette HP 35 Hz one-pole | **Agrees** (within 5 to 10 Hz) |
| 4 | Head bump near 90 Hz | **Not in any 424 page.** The playback EQ gives +13.8 dB at 30 Hz and +9.0 dB at 100 Hz relative to 315 Hz, which the head and tape do not follow below about 40 Hz; the bump itself depends on the head contour and scales with tape speed (so would be about twice as high in frequency at HIGH 9.5 cm/s as at a standard cassette's 4.8 cm/s) | shelf 90 Hz in Tape; bump 45 to 130 Hz in Cassette | **Cannot be confirmed or refuted from these documents.** The plugin's number is an estimate (b62 says so); keep, or measure a 424 recording |
| 5 | Saturation | tape THD 1.0% at 0 dB, 2.0% allowed at -10 dBV record; mixer 0.04 to 0.06% | Tape mode: two-band tanh with lows saturating earlier (BoostStage.h:315 to 320, `d = 1 + 5 sat^2 + 1.5 sat`) | **Plausible in kind** (lows first, odd harmonics); no level calibration in the plugin ties it to "0 dB = 1% THD" |
| 6 | Flutter | 0.05% WRMS, +/-1.0% peak (HIGH), 0.07% / +/-1.2% (NORM); pitch +/-12% | Cassette voicing flutter 8 Hz, up to 6% peak at Drift = 1 | **Agrees at Drift near 0.17**; the control range goes about 6 times past the machine's flutter (fine for a wobble effect) |
| 7 | Hiss | 55 to 58 dB S/N without dbx (-55 to -58 dB re nominal), 90 to 95 dB with | rms about -29 dBFS at Noise = 1, scales as Noise squared | The 424 is quieter than the plugin by about 25 dB even without dbx; with dbx by about 60 dB. The plugin's maximum is an intentionally "worn" tape |
| 8 | Noise reduction | dbx Type II compander is part of every 424 recording (and of the "breathing" sound) | none (b62 says: not modelled: "tape speed and noise reduction") | **Not modelled** (as stated) |
| 9 | Bias | 87.5 kHz, current-fed | none (inaudible; only its effect on top end and saturation) | n/a |
| 10 | Channel level structure | mic -50 dBV, line -10 dBV, 40 dB trim | not modelled | n/a |

## 4. Ranked suggestions (what is worth changing)

Ranked by (strength of the document evidence) x (audible effect for a guitar plugin) / (work):

1. **Raise the top end of the Cassette and Tape voicings, or add a speed choice.** Documents give hard numbers: -3 dB at 16 kHz (HIGH) and 12.5 kHz (NORM), against 4.2 to 6.1 kHz (Tape boost) and 1.7 to 6.6 kHz (Cassette). Suggest: a "424" setting with the low-pass pair at about 16 kHz (HIGH) or 12.5 kHz (NORM) that closes toward the current values with Sat/Dirt (keep today's darker range as the "worn" end). Small code change (BoostStage.h:239; LoFi.h:69).
2. **Reduce Dist 2 strength and give Dist 3 some 2nd harmonic.** The manual's THD (Dist 2 0.05 to 3%, Dist 3 0.1 to 20%) and the Distressor capture (H2 -35 dB, H3 -12 dB at the red line) say Dist 2 is too strong by 2 to 7 times above -26 dBFS (1.9% at -30, 5.8% at -20) and Dist 3 has no 2nd harmonic. Suggest: lower `k` slope or `pre` for Dist 2 (h:2900 to 2905: about half the current k brings the -20 dBFS figure to about 3%), and add a small asymmetry (bias about 0.05 to 0.1) to Dist 3 so that H2 is about 23 dB below H3. Also correct the comment "stronger with more gain reduction" (it is stronger with more level; with more reduction at the same input it is weaker, calc).
3. **Make the knee depend on the ratio.** Hard evidence only for 2:1 (+15 dB, up to 30 dB) and that 20:1/Nuke are hard; qualitative for the rest. Suggest a table (width in dB centred on the threshold): 2:1 = 30 (gives the "+15 dB above threshold" knee), 3:1 about 20 (guess), 4:1 about 14 (guess), 6:1 about 8, 10:1 about 6, 20:1 about 2, Limit 0. Mark the 3:1/4:1/6:1/10:1 numbers as guesses from "long, long, shorter, shorter". Replace the constant at h:2742 for style 0 only.
4. **Bus style defaults: detector HP Off and a much lower saturation.** The clone has no sidechain filter in its text and a distortion of -75 dB (0.018%); the plugin gives every style the 80 Hz detector HP and Tube-like saturation by default (.cpp:3000, 3002), which for Bus is 30 to 300 times the clone's distortion at normal levels. Per-style defaults, or a note in the style tooltip. Zero DSP change.
5. **Add the Distressor's audio-path 80 Hz Bessel high-pass** (3rd order, -3 dB at 65 Hz; separate from the detector HP). Numbers are in the manual; it also explains the "HP" LED combos with Dist 2/3. Moderate value for guitar (the low E is 82 Hz).
6. **Add a 6 kHz detector band emphasis** (on/off). The manual names only the centre; amount and Q would be chosen by ear (for example +6 to +10 dB at Q 1; these are my guesses, not from the manual). Value for guitar is limited because the detector sees a DI-style signal with little at 6 kHz.
7. **Correct the Tape/Cassette descriptions** so they claim "cassette tape response" rather than "a Tascam 424 channel": the 424's channel is flat to 30 kHz with +/-10 dB shelves. If a real 424-channel colour is wanted, the existing Boost EQ low shelf (100 Hz) and high shelf (6 kHz) can already do it with approximate settings: low shelf about 200 to 265 Hz with +/-12 dB, high shelf about 6.8 kHz with +/-7.5 dB (derived, +/-30%), and a flat top to 30 kHz. Text change only.
8. **Feed-back detection for Bus** (the clone is "mostly feed-back"): a larger change (the gain computer would read the compressed signal); evidence is a sentence, no numbers. Defer until the real schematic is available.
9. **Flutter default for Cassette**: choose Drift about 0.17 for the "424" voicing (matches +/-1.0% peak), keep the current maximum for effect use.
10. **dbx-style noise reduction (compander)** is the one big 424 trait absent. Not documented in these pages beyond "Type II"; skip unless a tape-sound feature is wanted.

## 5. What these documents cannot settle (do not act on without more data)

- Head bump frequency of the 424 and the playback-EQ/head interaction (no figure in the pages read).
- The Distressor's detector HP corner, band-emphasis boost, knees other than 2:1, the Dist 2/Dist 3 frequency shaping, and release per ratio.
- The SSL clone's attack resistors, release capacitors, ratio-switch offsets and the real Auto release (the schematic image is 196 x 249 pixels and the component list is missing).
- The 424's record-current path and record EQ (values read, topology not traced) and the condition that switches its playback HF time constant (U112b).
- Which numbers in the Boost "Tape" mode correspond to a measured 424 (b62 states they are estimates, and nothing in these documents contradicts the 40 Hz low end).
