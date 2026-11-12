# Amp library audit: redundancies and gaps (b89)

Analysis only. Nothing was changed, removed or merged. All numbers are measured, not listened to.

## How it was measured (and what it cannot see)
- The fitting tool re-measured every amp and voicing on the b89 build: 189 combinations (45 amps, Acoustic skipped, including the new Vitriol and Cascade), amp only (no cab, effects off, Auto Level off), Hybrid mode, at five Gain settings (0.2, 0.35, 0.5, 0.65, 0.8) and six input levels (-50 to -10 dB). Per setting it records the compression curve, the 2nd and 3rd harmonic of a 196 Hz sine against level, and an eight-band tone shape (63 Hz to 8 kHz, at the -26 and -10 dB inputs).
- Two voicings are called **essentially identical** when, at the same knob positions and averaged over the five settings, they are within 1.0 dB on gain behaviour, 1.5 dB on harmonics and 1.5 dB on tone. **Close** means within 1.5 / 2.2 / 2.2 dB. The fitting tolerance (2 / 3 / 3 dB) is looser than both; it finds 153 mutually "redundant" pairs, which is too many to be useful.
- **Blind spots:** Gain below 0.2 and above 0.8 (the library never measures them), power-stage sag and feel, the cab, Amp Modes other than Hybrid, and anything about time behaviour. Two voicings that look identical here can still differ in feel.

## Redundancies

### Within one amp, essentially identical (the ones to retune, per your preference for distinct voicings)
| Amp | Voicings | Gain / harmonics / tone difference (dB) |
|---|---|---|
| Diesel Fuel | Lead and Open | 0.1 / 0.1 / 0.4 (the tone differs by less than half a dB) |
| Console DI | Console and Air | 0.0 / 0.1 / 0.5 |
| Triple Blend | Three-in-One, Warm and Open | 0.1 to 0.3 / 0.5 to 1.0 / 0.3 to 0.7 |
| Prism | Glass and Air | 0.1 / 0.2 / 1.1 |
| Wreck Stack | Lead and Smooth | 0.1 / 0.6 / 1.0 |
| Jade Chorus | Glass, Funk and Shimmer | 0.0 to 0.4 / 0.2 to 0.5 / 1.3 to 1.6 |
| Jubilant | Lead and Lead Hi | 0.8 / 1.5 / 0.6 |

### Within one amp, close
Brit Eighteen Clean / Brilliant; Marque Dream Lead / Singing; Stolen Fire Clean / Brilliant; Club Crunch Crunch / Jumped; Revved Up Tight / Lead; Power Brawl Scooped / Bottom; Modern Maul Clarity / Chunk; Cedar Grand Big Spring / Bright; Kindle Chime / Push; Brown Eye Tight / Mid push; Tweedle Dee Jumped / Gritty; Dual Thirty Channel 1 / Cut; Tweedle Dum Tweed Normal / Tweed Bright; Prism Body / Pushed; Master Cranker Stock / Boosted.

### Across amps
- **One large blob, eight voicings:** Slow Burn Smooth, 5555 Block Raw, 5555 Block Modern, Harmonic Haze Lead, Wreck Stack Lead, Wreck Stack Smooth, Power Brawl Scooped and Power Brawl Bottom all sit in the same place (tight mid-gain rhythm). The closest pairs are Wreck Stack Smooth / Lead with Power Brawl Scooped (0.3 / 0.8 to 1.0 / 1.1 to 1.2).
- **Cleans:** Twin Sparkle Sparkly, Wreck Stack Clean, Stolen Fire Clean and Stolen Fire Brilliant are one cluster.
- **Pairs:** Club Crunch Crunch with Club Crunch Jumped and Brit Forty-Five Cranked; Cedar Spring Spanky with Brit Eighteen Clean and Brilliant; Harmonic Haze Crunch with Wreck Stack Crunch and Power Brawl Crunch; Marmalade Heavy with Jubilant Lead and Lead Hi; Marmalade Crunch with Jubilant Rhythm; Slow Burn Lead with Power Brawl Mid-boosted; Club Crunch Pushed with Plex Appeal Jumped; Tweedle Dee Cranked with Tweedle Dum Cranked; Twin Sparkle Edge push with Tweedle Mini Edge; Cedar Spring Blackface with Cedar Grand Brownface.
- **Covered by others (one-sided):** Stolen Fire Clean and Brilliant, Wreck Stack Clean, Master Cranker Stock and Console DI Console / Air are each matched by several other voicings but cover little of their own.
- **New in b88 / b89:** Vitriol and Cascade overlap nothing. Modern, since it was eased back halfway in b89, now sits inside the eight-voicing blob (close to Power Brawl Bottom, 0.2 / 1.3 / 1.9), and Raw sits next to Wreck Stack Lead (0.7 / 1.5 / 0.6). The b88 fit of Modern was further from that blob; the halfway point is the price of the tighter low end you asked for.

Seventy voicings (a third of the library) are in a cluster of two or more at the "close" level.

## Gaps

### From the coverage map (saturation against brightness)
Of 945 measured settings, almost none are bright once they saturate. Empty or nearly empty cells: very clean and bright (0), clean / edge and bright (0), crunch and very bright (0), rhythm hi-gain and very bright (1, Matt's Amp Tight Voice), lead and very bright (2, Marque Dream IV Lead and Cascade). The dark side is crowded (hundreds of settings). So there is no bright, glassy crunch or bright rhythm tone except the cleans.
Character: of 809 saturated settings, 36 are strongly odd-dominant (3rd harmonic more than 24 dB above the 2nd; the symmetric, hard-clipped sound) and only 5 strongly even-dominant (2nd more than 12 dB above the 3rd).

### From your captures (33 captures, one to three per amp, against the new library)
Only **5 of 33** are within the fitting tolerance (gain < 2 dB, harmonics < 3 dB): both CavLabs Orange captures (Jubilant V2, Wreck Stack V1), the Roadster Ch4 modern rock capture (Plex Appeal V3), the Mark VII EET capture (Power Brawl V2) and one Single Rectifier capture (Modern Maul V1). The other 28 miss mostly on harmonics, with the tone second:
- **Fender cleans and breakup (1973 Twin x2, Twin Reverb breakup x2, Bassman x3):** none close. The nearest are unrelated amps (Marque Five Clean, Eclipse, Modern Maul, Slow Burn, Dual Thirty). The errors are in the harmonics (3rd harmonic off by up to 12 dB, 2nd by up to 12 dB) and the tone; in several of them the capture has clearly more 3rd harmonic than the nearest amp (the 1973 Twin by about 9 dB), so our cleans are cleaner in the odd harmonics than these Fenders.
- **Mark IIC+ style lead (Deth Lead, Fat Gain, Solo, Rock):** the nearest are Stolen Fire V1 and Solid Metal, but on average our amps make 2.5 to 13 dB more 2nd harmonic than these captures (Rock 2.5, Solo 5.3, Fat Gain 11.8, Deth Lead 13.4). A tight, low-even-harmonic lead is missing.
- **Extreme scooped metal (both Meshuggah captures, Omegeath RawBrutal):** tone errors of 6 to 13.5 dB (a very scooped, very dark-and-bright shape no voicing reaches), gain off by up to 2.9 dB.
- **Rectifier family (Dual Rect crunch and TS808, Single Rect x3):** nearest Ironside, Marque Dream, Digital Dread; 3rd harmonic off by 4 to 8 dB and tone by 4.5 to 5.7 dB.
- **Bright clean with a boost (Roadster clean, Mark IIC+ 80's clean):** gain error 1.3 to 3.3 dB and tone 3.3 to 7.6 dB off. Nearest are Prism and Stolen Fire.
- **Averages:** the harmonic error is not one-sided (the mean of the captures' 2nd harmonic against ours is -1.4 dB, median -0.8, over 27 captures); it is in how the harmonics grow with level. That is the open second-harmonic item, now with a number: it matters for about half of these captures.

## What I would do next (not done; your call)
1. Retune the seven within-amp essentially-identical sets so each voicing is distinct (Diesel Fuel Lead / Open first: they are within half a dB in tone). This follows your rule of retuning instead of deleting.
2. Decide which of the eight in the blob should be pulled apart (for example Modern back toward its b88 fit, or Power Brawl Bottom lower).
3. Gap voicings worth a decision: a bright rhythm / crunch voicing; a Fender-style clean-to-breakup with odd-harmonic growth; a tight, low-even-harmonic lead (Mark IIC+ style); an extreme scooped metal tone.
4. The harmonic-growth error: look for the cause in the amp engine rather than adding voicings.

## Correction (b90)
Diesel Fuel Open and Console DI Air were flagged above as essentially identical to a sibling voicing. That was a blind spot of the comparison, not a fact: Diesel Open's difference is sag (a service-manual change) and Console Air's is a shelf above about 11 kHz, and the comparison sees neither. Both stay as they are. The retunes done in b90 are listed in `CHANGELOG.md`.
