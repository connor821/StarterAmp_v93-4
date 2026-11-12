# ENGL Savage 120 (E610, hand-lettered schematic)

Source image: `savage120-1.png` (3504x2515, hand-lettered, readable with zoom; numerals are legible but the small print on caps in the tone stacks is not). Values marked (?) are not reliable. Computed values here are only first-order estimates; no stack solver was run because the tone-stack capacitors are not legible.

Tube list printed on the sheet: V1 ECC83/12AX7 "F.Q." (low-noise selected), V2 and V4 ECC83 "selected", V3, V5, V6 ECC83 "standard", V7 and V8 6550 "matched sets". Fuses: mains 230 V external 2.5 AM, internal 3.15 AT; 100 and 120 V external 5 AM (medium), internal 6.3 AT (slow). A MIDI interface port with a 0.8 AM fuse and an LED "tube array" front panel are also drawn.

## 1. Tube complement

| Ref | Type | Stage (Channel 1 = top row, Channel 2 = lower row) |
|---|---|---|
| V1A | ECC83 | Channel 1 input |
| V4A | ECC83 | Channel 1 second stage (gain pot before it) |
| V4B | ECC83 | Channel 1 third stage (Crunch 1 level before it) |
| V5A | ECC83 | Channel 1 / loop stage |
| V5B | ECC83 | Master A / Master B stage |
| V1B | ECC83 | Channel 2 input |
| V2A | ECC83 | Channel 2 stage 2 (gain pot) |
| V2B | ECC83 | Channel 2 stage 3 (Lead pot before it) |
| V3A | ECC83 | Channel 2 stage 4 (feeds tone stack, Hi balance) |
| V3B | ECC83 | Channel 2 stage 5 (after tone stack; Crunch 2 and Lead volume) |
| V6A, V6B | ECC83 | Phase inverter (long-tail, shared 470 ohm cathode) |
| V7, V8 | 6550 | Power amp, one push-pull pair, 120 W |

(The sheet has the "B" labels in the lower row because the two channels' V1B, V2A, V2B, V3A, V3B stages are labelled with their own names; the mapping above is read from the drawing, with (?) on whether V4 is a channel-1 stage or part of channel 2's shared path.)

## 2. Preamp, channel by channel

Global: filter caps 10 uF/450 V at each branch; 2K2 1.1 W decoupling resistors in the plate supply between the sections (to +U CH I and +U CH II), plate loads 100K (1.4 W as printed "1,4W"), coupling caps 47 nF.

### Channel 1 (top row)
- V1A: grid input with SENSE switch, 68K and 100K to ground (input attenuator), jack INPUT; plate R 100K; cathode R 1K5 parallel 22 uF (about 5 Hz); output C 47 nF.
- V4A: plate R 100K, cathode R 1K parallel 22 uF (about 7 Hz); gain pot 1M log (GAIN) at the grid; RE2A relay selects the channel path.
- V4B: plate R 100K, cathode 1K parallel an electrolytic (22 uF (?)); CRUNCH 1 level 250K log pot at the grid; RE2B relay.
- Tone stack (channel 1): 250K treble, 1M bass, 22K middle (printed "22K MIDDLE"); caps and slope resistor are not legible (?). Volume pots: "Clean volume" 250K log and "Crunch1 volume" 250K log; RE1A / RE1B select clean or crunch.
- V5A: plate R 100K, cathode 1K (no bypass drawn); coupling 0.1 uF via RE9A to the CH I loop (send and return jacks, "CH I / MASTER LOOP"); BALANCE 50K stereo pot (loop balance).
- V5B: plate R 100K, cathode 1K parallel 22 uF; MASTER A and MASTER B pots 1M (RE7 selects) with 47K resistors and a 100K / 1K around the RE9B relay; output 0.1 uF into the PI.

### Channel 2 (lower row)
- V1B: grid 1M, plate R 100K, cathode R 1K5 parallel 22 uF; coupling 47 nF. GAIN pot 1M log at V2A.
- V2A: plate R 100K, cathode R 1K parallel 22 uF; RE4A.
- V2B: plate R 330K, cathode R 3K3 (no bypass); LEAD pot 250K log at the grid (1M grid leak).
- V3A: plate R 100K, cathode R 1K parallel an electrolytic (22 uF (?)); tone stack: 250K treble, 1M bass, 20K middle; HI BALANCE pot (switched by RE10A); coupling 47 nF.
- V3B: plate R 100K, cathode R 1K5 parallel 22 uF; RE6A; CRUNCH 2 volume 250K log, LEAD volume 250K log; RE5A selects.
- The channel-2 outputs go to the line preamp / balance (50K) stages and the master chain.
- Power amp preamp / line-out section: LEVEL 1M pot, 22K, a TL074 op-amp with 47 uF caps and 1K / 10K feedback resistors and a XLR LINE OUT; "Line Preamp" and "Power Amp Preamp" labels; RE8 / relays select.

## 3. Switching
Eight relays (RE1 to RE10, 12 V from a 7824 and BC237B transistors) selected by footswitch contacts: "FS. CH1/CH2 clean/CR.1" (Clean, Crunch 1, Crunch 2, Lead), "FS. CR2/Lead R/S" (rough/smooth), "FS. Ma.A/Ma.B Pre.A/Pre.B" (Master A/B and Presence A/B). No clipping diodes are drawn; all clipping is tube saturation. The gain chain for Lead on channel 2 is V1B, V2A, V2B, V3A, V3B: five cascaded gain stages with a stack after V3A (so the stack is between the fourth and fifth stage).

## 4. Phase inverter and power amp
- PI: V6A grid R 100K to ground via a 0.1 uF input coupling, plate loads 100K each (V6A, V6B), shared cathode R 470 ohm, a 22K and a 0.1 uF at V6B's grid, and NFB R 150K into the V6B grid. Outputs 47 nF to the 6550 grid networks.
- Power tubes: V7 and V8, 6550; grid stoppers 1K; grid leaks 150K to the bias line "-U GI"; screen resistors 1K 1 W (printed "1K 1,4W") from +U S.G (335 V printed "335V") to pin 4. Cathodes: each has a 315 mA "M" fuse ("315mA M", "315mA H") in the cathode and an LED (with an 8R6 (?) series resistor) for tube-failure indication, and a STANDBY switch to ground. A ZY5xx (?) zener is drawn from V8's plate line to ground (printed "ZY509"/"2Y509", (?)).
- Bias: "DC voltage between point A and B approx 0.8 to 1.2 volts" (measured across the cathode resistors, (?) value of the measuring resistors).
- Output transformer: taps 16 ohm, 8 ohm (x2), 4 ohm (x2); B+ "+U A" 545 V on the center tap; BY509 diode on the secondary side for the clamp.
- NFB and presence: from the OT tap through 22K / 4K7 divider (R "4K7", "22K", "0.1 uF") through RE-P relay selecting "Presence A" 50K pot with 4n7 or "Presence B" 50K pot with 4n7 (each in series with the NFB), then 150K into the V6B grid (with 22K to ground). First-order estimate of the loop attenuation at the injection node: 22K / (22K + 150K + about 4.7K) = about -18 dB with presence at its position of least series resistance; the 4n7 caps in the presence arms shunt the series resistor at high frequencies (so highs get more feedback as the pot is raised). Which arm is shunted and the exact tap is (?).

## 5. Supply
- +U A = 545 V (bridge BY255, 220 uF/250 V and 220 uF/450 V in series chain with 150K and 330K 1.1 W balance/bleed resistors).
- +U S.G = 335 V (printed "335V") via 470 ohm 5 W from the +U A node for the screens.
- +U Pre = 340 V (printed "340V") via 2K2 1.1 W and 2K2 1.1 W with 100 uF/450 V and 22 uF/450 V caps; further decoupling R 1K 1.1 W + 10 uF/450 V for +U CH I and CH II.
- Bias supply -U GI: 1N4007, 22K, 100 uF/400 V, 22K + 25K trimmer BIAS, 10 uF/63 V. The exact negative voltage is not printed (?).
- 24 V (printed "+24V") from a 7824 with 470 uF/40 V? and 100 nF / 1 uF caps; 12 V for the line preamp.
- Mains: dual-fused, power switch, two lamps in a stand-by drawing.

## 6. Uncertain or illegible
- Tone-stack caps and slope resistors; whether V4A and V4B are channel 1 or shared; the exact values of the electrolytic bypasses on V3A, V4A, V4B; the cathode measuring resistors; the zener type at V8; the bias voltage; the screen voltage in the two supply notes (335 V and 395 V (?) appear near different lines).

## b90 addendum: what the rough/smooth relay switches (re-read from the sheet you re-sent; hand-drawn scan, low resolution)
- The footswitch contact "CR2/Lead R/S" (labelled ROUGH / SMOOTH) drives relay RE10. RE10A is drawn beside the lead channel's tone stack, next to a variable resistor labelled **HI BALANCE**.
- As far as I can read it, closing RE10A puts the Hi Balance trimmer between the stack's input (the V3A coupling-capacitor output) and its output (the V3B grid line), so some signal goes around the Treble / Bass / Middle stack. Open, the signal goes through the stack alone. Which state is Rough and which is Smooth is not marked on the sheet; the label and the circuit suggest Rough = the bypass in (more highs and mids, a shallower scoop) and Smooth = the stack alone (a deeper scoop, less top).
- **The trimmer's value is not printed**, so the size of the difference cannot be calculated from the sheet. Wreck Stack Smooth is therefore left as the hand-tuned delta it was (bass +0.5, low mid +0.5, high mid -1.5, treble -1.5, presence -1.5 dB, voice 5.2 kHz): it points the way the circuit suggests, and its size is an ESTIMATE. A manual or a capture of the real switch would let it be fitted.
