# Boss MT-2 Metal Zone (read from the 1991 Service Notes, 4 pages)

Source: `MetalZone_p1..p4.png` (Roland/Boss "MT-2 Service Notes, First Edition", Apr. 1991, 2100x1483 scans).
p1 = specs and exploded view, p2 = parts list, p3 = PCB layout + circuit diagram (MT board 1/2, VR board 2/2, LED board), p4 = output-waveform check appendix (scope pictures only).

How this was read: the circuit diagram on p3 was cropped and enlarged 3x to 6x. Printed part labels are legible. Where I had to read a connection from line work, or where a value is printed ambiguously, it is marked **(?)**. Capacitor values are printed as they appear on the sheet: a bare decimal (0.047) is in uF, "10p"/"47p" is pF, "1/50", "10/16" mean uF/V (electrolytic), "100/6.3" is 100 uF/6.3 V. Resistors: "10k", "1M". Hollow arrow up = +9 V, filled arrow up = +4.5 V (Vref, the bias reference), hatched triangle = ground.

## 1. Headline facts (page 1 specs, page 2 parts)

| Item | Value |
|---|---|
| Input impedance | 1 Mohm |
| Output impedance | 1 kohm |
| Recommended load | 10 kohm or greater |
| Input equivalent noise | less than -110 dBm (IHF-A), EQ knob centre, 0 dBm = 0.775 V |
| Supply | 9 V (battery 6F22/S-006P, or PSA-series AC adaptor); current draw 20 mA |
| Controls | LEVEL, EQUALIZER (HIGH/LOW dual knob), MIDDLE (level), MID FREQ, DIST. Panel prints "-15 +15" on the HIGH/LOW EQ knob and "200 / 5k" on the mid-frequency knob |
| Op-amps | 4 x M5218AL (dual, IC1 to IC4), so 8 sections, drawn as 1a, 1b, 2a, 2b, 3a, 3b, 4a, 4b |
| Transistors | 2SC3378GR (Q001, Q004, Q007, Q008, Q010), 2SC2458GR (Q005, Q006), JFETs 2SK184GR (Q011; alt 2SK117GR) and 2SK118Y (Q002, Q003, Q009) |
| Diodes | 1SS133 x 6 (D001 to D005 and a pair at the clipper; see below), S5500G (D006), zener RD5.6EB3 printed on the schematic as MTZ5.6B (ZD001), LED LN-28RP (LED001) |
| Pots (parts list) | DIST 250kA; LEVEL 50kA; HIGH/LOW 100kW + 100kW (dual); MIDDLE 50kC x2 (MID FREQ gang) + 100kW (MIDDLE level). The schematic prints the 100k pots as "100kG" |
| Switch | JM-0404 push switch (on/off is electronic, see section 8) |

## 2. Signal flow in one line

Input jack -> JFET buffer Q011 -> (effect path) C039 / switch JFET Q009 -> op-amp 3b (first gain stage, with transistor shaping cell Q010) -> RC network -> op-amp 3a (DIST stage, 1 to about 250x) -> 2.2k into a pair of 1SS133 diodes -> shelf cut + coupling -> op-amp 4b (second gain stage, with two transistor cells Q007/Q008) -> op-amp 4a (inverter, gain -1) -> **tone section**: 1a (HIGH/LOW), 2a/2b (MIDDLE, MID FREQ) -> LEVEL pot 50kA -> switch JFET Q002 -> output buffer Q001 -> 1k -> OUTPUT. A separate bypass path (Q004 buffer, JFET switch Q003) feeds the same output buffer.

The tone section is **after** all gain and clipping, and there is no pre-EQ ahead of the diodes other than the fixed high-pass and low-pass networks noted in sections 4 to 6.

## 3. Supply and bias (MT board, lower left)

- 9 V enters from the battery or AC adaptor jack (jack pins 1, 2, 3 and board nets 2 and 19).
- D006 S5500G (Schottky) hangs from the 9 V rail to ground at the board entry (reverse-polarity protection) **(?)** (symbol is drawn from the rail to ground, which is the usual shunt-clamp arrangement; I cannot tell the orientation).
- C041 100 uF/16 V across the 9 V rail.
- Vref (+4.5 V): R056 10k and R057 10k divide the 9 V; C040 100 uF/6.3 V and C019 100 uF/6.3 V on the 4.5 V node.
- All op-amps run from single +9 V (pin 8) and ground (pin 4); the bias references (filled arrows) are the +4.5 V node.
- Power for the check LED (R010 1.2k from +9 V, zener ZD001 MTZ5.6B 5.6 V, LED001): the LED lights only while the supply is above about 5.6 V plus the LED drop, which is how it doubles as the battery check.

## 4. Input buffer and bypass path

- INPUT jack (nets 20/21) -> R059 10k -> C042 0.047 uF -> gate of Q011 (2SK184GR). R058 1 M from the gate to Vref. This gives the 1 Mohm input impedance and a 3 Hz high-pass.
- Q011: drain to +9 V, source loaded by R060 10k to ground. Source follower.
- Q011's source node splits two ways:
  1. **Bypass:** R055 10k -> base of Q004 (2SC3378GR) emitter follower, R008 10k emitter to ground -> C003 1 uF/50 V (R007 1 M to Vref) -> JFET switch Q003 (2SK118Y) -> C002 10 uF/16 V (R004 100k to Vref) -> base of output follower Q001.
  2. **Effect:** C039 1 uF/50 V (R052 100k to Vref, high-pass about 1.6 Hz) -> JFET switch Q009 (2SK118Y, gate driven through D005 1SS133 from the switching flip-flop) -> C033 0.015 uF (R047 1 M to Vref at the JFET; R043 100k to Vref after C033) -> non-inverting input (pin 5) of op-amp 3b. C033 into R043 is a high-pass at 106 Hz.

## 5. Gain stages

### 5.1 First gain stage: op-amp 3b (pin 5 input, pin 6 inverting, pin 7 out)

- Non-inverting. Feedback from pin 7 to pin 6: R044 220k in parallel with C032 100 pF (pole at 7.2 kHz for the 220k alone).
- Gain-leg network on pin 6 (a "transistor shaping cell", used three times in this pedal; see 5.4):
  - C034 0.027 uF in series with R046 2.2k, to the transistor emitter node.
  - C035 0.01 uF to the transistor base node.
  - Q010 (2SC3378GR): collector to +9 V, base node biased by R053 47k to Vref, emitter node with R054 10k to ground.
- No DIST control in this stage; its gain is fixed by the network (upper bound if the cell node were an ideal AC ground: 1 + 220k/2.2k = 101, about 40 dB at high frequency; the real gain is lower and frequency-dependent **(?)**, I did not solve the cell).
- Output (pin 7) -> R045 10k -> node with R042 10k to Vref and C031 0.047 uF to Vref (a low-pass: R045 against R042 with C031 across it, pole near 680 Hz, passband attenuated 6 dB) -> C029 0.033 uF (R040 100k to Vref, high-pass 48 Hz) -> pin 3 of op-amp 3a.

### 5.2 DIST stage: op-amp 3a (pin 3 input +, pin 2 -, pin 1 out)

- Non-inverting. Pin 2 leg: R041 1k in series with C030 10 uF/16 V to ground (gain leg, DC gain 1, corner 16 Hz).
- Feedback from pin 1: R051 1k in series with **VR01 DIST 250kA**, and C028 47 pF directly from pin 1 to pin 2.
- Gain = 1 + (1k + VR01)/(1k) from 2 (pot at 0) to 1 + 251k/1k = **252 (48 dB)** at the top, above the 16 Hz leg corner. C028 47 pF with the pot at max rolls off at about 13 kHz (1/(2 pi 251k 47p)); with the pot at 0 it is far above the audio band.
- VR01 shows jumpers on pins 1 to 3 (variable resistor), so this is a plain rheostat in the feedback.
- Output pin 1 -> C027 10 uF/16 V -> next section.

### 5.3 Clipping diodes (which, how many, where)

- After C027: **R033 2.2k in series, then two diodes D003 and D004, both type 1SS133 (silicon switching diode), each from the node to ground.** There are exactly two clipping diodes in the whole pedal, both at this node.
- Their orientation is shown only by small symbols that I cannot resolve at this scan resolution **(?)**. The standard topology and the two separate vertical drawings are consistent with an anti-parallel pair (symmetric hard clipping, about +/-0.6 V), but I could not confirm they are anti-parallel rather than both the same way. There is no capacitor visible at this node (the DS-1 style 10 nF shunt is not present as drawn).
- The diodes follow the DIST stage (3a), not the final EQ stages.

After the diodes: R032 10k in series, then R031 4.7k in series with C023 0.015 uF to ground (a shelving cut: about -10 dB above roughly 700 Hz to 2.3 kHz), then C021 1 uF/50 V (R029 100k to Vref) -> pin 5 of op-amp 4b.

### 5.4 Second gain stage: op-amp 4b (pin 5 input +, pin 6 -, pin 7 out)

- Non-inverting. Feedback: R030 **5.3k** (printed "5.3k"; unusual value, as printed) in parallel with C022 47 pF (pole at 640 kHz).
- Pin 6 is also the top rail of two more transistor cells, each a copy of the Q010 cell:

| Cell | Rail cap to emitter node | Resistor in series with it | Cap to base node | Base bias to Vref | Emitter resistor to ground |
|---|---|---|---|---|---|
| Q008 (2SC3378GR) | C024 0.015 uF | R034 1k | C025 0.0015 uF | R036 47k | R037 10k |
| Q007 (2SC3378GR) | C020 0.22 uF/35 V | R027 470 ohm | C017 0.047 uF | R025 470k | R024 10k |
| Q010 (first stage, for comparison) | C034 0.027 uF | R046 2.2k | C035 0.01 uF | R053 47k | R054 10k |

  In each, collector goes to +9 V. Which transistor lead is the emitter is read from the symbol (base bar on the top lead, the lead going to the 10k to ground = emitter, the lead going to the +9 V arrow = collector); confidence medium **(?)**. These behave as emitter followers that make the gain leg frequency-dependent (the "metal" voicing of the pedal comes from this).
- Output pin 7 -> R028 22k -> op-amp 4a.

### 5.5 Inverter: op-amp 4a

- Inverting gain -1: input R028 22k to pin 2, feedback R026 22k in parallel with C018 10 pF (pole 720 kHz), pin 3 to Vref. Output pin 1 -> **R014 22k** -> HIGH net (pin 8 of the VR board connector).

## 6. Tone section (VR board plus op-amps 1a, 1b, 2a, 2b)

Connector nets in the diagram: 8 = HIGH net, 7 = LOW/summing net, 6 = LOW pot wiper, 5 = ground, 12/11/10/9/13 = MIDDLE and MID FREQ nets, 4/3 = LEVEL pot. I traced these from the lines through the VR-board connector; the assignments below are my best reading **(?)**.

### 6.1 HIGH / LOW (VR03a LOW 100k, VR03b HIGH 100k, a dual pot) with op-amp 1a

- Op-amp 1a: pin 2 (-) tied to the HIGH net (the net fed from 4a through R014 22k); feedback R015 22k in parallel with C010 10 pF; pin 3 (+) on the LOW/summing net (net 7). So the input resistor is R014 22k and the feedback R015 22k, giving unity inversion at the midpoint.
- VR03b (HIGH): ends between net 8 and net 7; its wiper goes through R061 2.2k in series with C044 0.01 uF to ground (a treble shunt that the pot weights).
- VR03a (LOW): one end on net 7, the other end on net 5 (ground), wiper (net 6) to a unity-gain follower circuit built round op-amp 1b: C008 0.22 uF/35 V, C009 0.047 uF in series, R012 2.2k from the cap junction to the 1b output, R013 100k from the + input to Vref (a unity-gain Sallen-Key high-pass, about 105 Hz by the usual formula). The 1b output appears to connect only back into this network **(?)**. I cannot see where 1b's output is used outside that loop, so the LOW control is read as shelving by loading net 6 with this bootstrapped high-pass.
- 1a output (pin 1) -> C011 1 uF/50 V -> the mid section.

### 6.2 MIDDLE and MID FREQ (VR02b MIDDLE 100k, VR02a MID FREQ 50kC x2) with op-amps 2a and 2b

- 2a (pin 1 out, pin 2 -, pin 3 +): pin 2 gets the 1a signal through R038 47k; feedback R035 47k in parallel with C026 100 pF; pin 3 (+) receives the mid-network signal through C039 0.1 uF, and has R039 1 M to Vref.
- MIDDLE pot VR02b (100k) is the mid level: one end takes the 1a signal through R050 330 ohm (net 12), the other end takes the 2a output through C037 1 uF/50 V and R049 330 ohm (net 11); its wiper (net 13) is the + input of buffer 2b.
- 2b is a voltage follower (pin 6 tied to pin 7). Its output goes through **C036 0.022 uF in series with R048 2.2k** (net 9) to the MID FREQ pot.
- MID FREQ: the two 50k gangs of VR02a (50kC x2). One gang is in the signal path from net 9 to net 10; the other gang is in series with R062 2.2k to ground. C043 0.0082 uF from net 10 to ground. Net 10 is the node that feeds C039 0.1 uF into 2a's + input.
- So the mid-band is a variable resonant/band-pass network (R-C ladder with the 2.2k resistors and the dual gang) in a positive-feedback path around 2a, with the MIDDLE pot choosing between input signal and 2a output to set boost or cut. Printed range on the panel: MID FREQ 200 Hz to 5 kHz. Mid level (MIDDLE) range in dB is **not printed** on these pages (the panel "-15 +15" mark belongs to the HIGH/LOW knob) **(?)**.
- 2a output (pin 1) -> C005 10 uF/16 V -> **VR04 LEVEL 50kA** (lug 3 = net 4 in, wiper = net 3 out, lug 1 ground).

### 6.3 Mid section summary of R and C values

R015 22k, R038 47k, R035 47k, R050 330, R049 330, R048 2.2k, R062 2.2k, R039 1M, R012 2.2k, R013 100k, R061 2.2k; C010 10p, C011 1u/50, C026 100p, C037 1u/50, C039 0.1, C036 0.022, C043 0.0082, C008 0.22/35, C009 0.047, C044 0.01, C005 10u/16.

## 7. Output stage

LEVEL wiper (net 3) -> C004 1 uF/50 V (R006 1 M to Vref) -> JFET switch Q002 (2SK118Y, gate via D001 1SS133) -> joins the bypass path at the node feeding C002 10 uF/16 V -> base of Q001 (2SC3378GR; R004 100k bias to Vref) -> emitter R002 10k to ground -> C001 10 uF/16 V -> R001 1k series -> OUTPUT jack, R003 100k to ground at the output side. This gives the 1k output impedance. A pull-down R005 1 M and D002 1SS133 with R009 1 M and C006 0.047 uF sit on the gate network of Q003 and Q002 for click-free switching.

## 8. Footswitch and LED (electronic switching)

- SWITCH jack/pin 18 (momentary push, JM-0404) -> R018 100 ohm in series, R023 1 M to +9 V and C016 0.01 uF to ground at the node, then into a two-transistor flip-flop: Q005 and Q006 (2SC2458GR), collectors R016 56k and R017 56k to +9 V, bases R021 56k and R022 56k to ground, cross-coupled through C014 220 pF and C015 220 pF, with C013 470 pF + R020 100k and C012 470 pF + R019 100k, R011 1 M, C007 0.047 uF.
- The flip-flop outputs bias the JFET switches: Q009 (effect in, via D005), Q003 (bypass), Q002 (effect out, via D001). Exact gate-drive polarity and the D001/D002/D005 orientations are not resolvable at this scale **(?)**.
- CHECK LED board: R010 1.2k, ZD001 MTZ5.6B (5.6 V zener), LED001 LN-28RP, between nets 16 and 17.

## 9. Full parts value list (as printed on p3)

Resistors: R001 1k, R002 10k, R003 100k, R004 100k, R005 1M, R006 1M, R007 1M, R008 10k, R009 1M, R010 1.2k, R011 1M, R012 2.2k, R013 100k, R014 22k, R015 22k, R016 56k, R017 56k, R018 100, R019 100k, R020 100k, R021 56k, R022 56k, R023 1M, R024 10k, R025 470k, R026 22k, R027 470, R028 22k, R029 100k, R030 5.3k, R031 4.7k, R032 10k, R033 2.2k, R034 1k, R035 47k, R036 47k, R037 10k, R038 47k, R039 1M, R040 100k, R041 1k, R042 10k, R043 100k, R044 220k, R045 10k, R046 2.2k, R047 1M, R048 2.2k, R049 330, R050 330, R051 1k, R052 100k, R053 47k, R054 10k, R055 10k, R056 10k, R057 10k, R058 1M, R059 10k, R060 10k, R061 2.2k, R062 2.2k.

Capacitors: C001 10u/16, C002 10u/16, C003 1u/50, C004 1u/50, C005 10u/16, C006 0.047, C007 0.047, C008 0.22/35, C009 0.047, C010 10p, C011 1u/50, C012 470p, C013 470p, C014 220p, C015 220p, C016 0.01, C017 0.047, C018 10p, C019 100u/6.3, C020 0.22/35, C021 1u/50, C022 47p, C023 0.015, C024 0.015, C025 0.0015, C026 100p, C027 10u/16, C028 47p, C029 0.033, C030 10u/16, C031 0.047, C032 100p, C033 0.015, C034 0.027, C035 0.01, C036 0.022, C037 1u/50, C039 0.1, C040 100u/6.3, C041 100u/16, C042 0.047, C043 0.0082, C044 0.01.

No C038 was found on the circuit diagram (the numbering skips it); the PCB layout silkscreen on p3 is too small to read, so I did not use it. A few R/C numbers (for example R005/R006/R007/R009 and C006) may be swapped between neighbouring switch-gate parts **(?)**.

## 10. Items I could not read or resolve

1. Diode orientations (D003/D004 clippers, D001/D002/D005 switch gates, D006): symbols too small. The clippers are assumed anti-parallel from the standard topology only.
2. Exactly which lead of Q007, Q008 and Q010 is the emitter (read from symbol shape).
3. The destination of op-amp 1b's output (it appears to be only in its own bootstrap loop).
4. Whether VR03a (LOW) wiper is net 6 and which pot end is ground (nets 5 and 7).
5. R030 prints as "5.3k"; I could not rule out "3.3k" at this scale.
6. Mid boost/cut range in dB (not printed).
7. Appendix p4 (waveform check) contains scope pictures at 200 Hz 5 mVp-p square input with knob settings drawn as pictures; no numeric gain or voltage figures are printed that I can use beyond the scope scales (1 ms/div, 50 mV/div for the first picture, 1 V/div for the others).
