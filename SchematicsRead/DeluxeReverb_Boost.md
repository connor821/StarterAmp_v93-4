# Fender Deluxe Reverb-Amp "w/ Volume Boost" (silverface-era, Dwg 014641) - schematic read

Source: `3cb5c289-Fender_deluxe_reverb_boost.pdf`, 1 page, one embedded 72 dpi 1765x1481 palette scan (low resolution, speckled; I upsampled 4x to read). Title: "FENDER MODEL DELUXE REVERB-AMP SCHEMATIC NOTICE" (no model code in the title); title block: "Schematic & Wiring Diagram, Deluxe Rev-Amp **W/ Volume Boost**", Fender Musical Instruments, Santa Ana (CBS-era), **Dwg No. 014641**, size C, rev "-", sheet 1 of 1, hand-written drawing/release marks (date reads something like 10-6-77 (?)). Sheet notes: voltages +/-10 %; resistors 1/2 W 10 % unless specified; caps at least 400 V unless specified; note 4: "**010873 used in some versions**" (this is the CSA thermal protector in the mains line).
**Result: apart from the volume-boost hardware and a few small items listed below, this is the AA1172 circuit**, and I re-read every value on this scan (not copied).

## 1. Tube complement and heaters
V1 7025, V2 7025, V3 12AT7 (reverb driver, halves paralleled), V4 7025 (V4A mixer, V4B recovery), **V5 7025** (oscillator/modulator - the drawing prints "7025" for V5, while AB868/AA1172 print 12AX7A), V6 12AT7, V7/V8 6V6GT, V9 **5U4GB**.
Heaters: 6.3 V winding with **100 ohm + 100 ohm** to ground (artificial CT). Mains: 120 V 60 Hz, **2 AMP SLO-BLO fuse**, "C.S.A. THERMAL PROTECTOR 010873" in series with the AC switch (the AA1172 shows no thermal protector), ground switch with **.047 uF 125 V AC** cap (AB763/AB868/AA1172: 600 V), 330 VAC per side to the 5U4GB.

## 2. Stage by stage (values identical to AA1172 unless stated)
**Normal:** inputs 68k/68k, **1M** to ground; V1A plate **+180 V**, load **100K 1W**; cathode **1500 || 25 uF, +1.3 V**; volume pot printed "**VOLUME ... ND 014038**" with a value that reads "399K"/"1M" on this scan - hard to read (?); the "ND" and the part number mark it as a different (boost) pot from the AA1172's "1M-J"; V1B plate +180 V, load 100K 1W, cathode 820 || 25 uF (node A) +1.3 V; out **.047 uF -> 220K**.
**Vibrato:** V2A plate **+170 V**, load 100K 1W, cathode 1500 || 25 +1.3 V; **47 pF** bright cap; volume pot "**ND 014031**" (value reads "999K"/"1M" (?)); V2B plate +170 V, load 100K, cathode **820 || 25**, +1.3 V; output **.022 uF** to the V4A grid node.
**Volume boost:** the two volume pots carry "ND" and distinct part numbers (014038 Normal, 014031 Vibrato); additionally a **"PUSH/PULL ON SWITCH 014031"** symbol sits in the reverb-driver input path. I could **not** identify a separate boost gain stage or extra components in the preamp: no extra tubes, no additional resistors or caps in the signal path compared with AA1172. How the boost is wired (push/pull switch contacts at the volume pots) is not resolvable from this scan. (?)

## 3. Tone stack (both channels)
250 pF; 100K; .1 uF; .047 uF; **6800** to ground; TREBLE **250K-J** (part 021895), BASS **250K-J** (021857 / 021870), same topology as AA1172.

## 4. Reverb
- V3: **500 pF** coupling, **PUSH/PULL ON SWITCH** (reverb on/off) in series, **1M** grid leak; cathodes joined to **a single resistor printed "68C" with no bypass capacitor drawn**, cathode printed **+6.0 V** (AA1172: 2200 ohm || 25 uF, +8.7 V). "68C" is probably a code or "680"; I cannot read the exact value (?); plate **+410 V** to TR4, other end to B.
- Recovery V4B: **220K with 2000 pF across**, cathode **820 || 25 uF +1.3 V** (node E), plate **+170 V**, load **100K 1W**, **.003 uF** -> **REVERB 100K-L** (marking "DC18/3") -> wiper -> **470K**; **220K in series with 1K to ground** at the mix node (the 1K is new here; AA1172 has 220K straight to ground); 10 pF || 3.3M into the mixer grid. Mixer V4A plate **+180 V**, **220K + .1 uF** as in AA1172.

## 5. Tremolo (V5 labelled 7025)
Oscillator: plate printed "+21?V" (smudged; AA1172 +270 V) (?), load **220K**; cathode **2700 || 25 uF**, **+2.1 V**; modulator plate **+345 V**, cathode **100K || 5 uF/50 V ("5 / 50")**, **+17.0 V**; **10M**; neon + **100K**; **50K-RA INTENSITY**; **3M-RA SPEED** + **100K**; **1M, 1M, 2.2M, .01, .01, .022**; vibrato pedal jack. Hand-written code "07 865"/"010437"/"0/1915" next to the speed/bias/intensity pots are Fender part numbers.

## 6. Phase inverter V6 12AT7 (identical to AA1172)
**.01 uF** input, **330K / 330K** grid-bias resistors, bias node **+75.5 V**, **22K**, **470 ohm**, cathodes **+90 V**, **.1-200** to V6B grid, **47 ohm**, **82K / 100K** plate loads, V6A **+190 V**, V6B **+180 V**, node C **+375 V**. **NFB 820 ohm** from OT secondary.

## 7. Output stage
6V6GT x2: plates **+415 V**, screens **+415 V** via **470 ohm 1 W**, OT CT **+420 V**; **1500** stoppers, **220K** leaks, **1200 pF** grid caps, bias **-35 V**. Bias supply: **470 ohm 1 W** -> diode -> filter cap printed "**80 / 75**" (AA1172: 50/70) -> **10K-L** pot (+10K to ground). TR3, EXT SPKR jack, **8 ohm** speaker.

## 8. Supply
5U4GB; standby; **2 x 16 uF 450 V** parallel; TR2; B **16/450**; **2.2K 1W**; C **16/450**; **10K 1W**; D **16/450**. Transformer table (export): TR1-125P33A 022749, TR1-125P23B 025130, TR2-125C3A 022707, TR3-125A1A 022640, TR4-125A20B 022921 (same as AA1172). Printed: +420, +415, +410 (V3), +375 (C), +345, preamp +170/+180.

## 9. Not legible / uncertain
- Volume pot values ("399K/999K ND") and the actual boost wiring.
- V3 cathode resistor ("68C") and absence of a bypass cap.
- V5A plate voltage (smudged), V5 tube type label (7025 vs 12AX7A).
- Bias filter cap "80/75".
- Date/revision hand-writing in the title block.
