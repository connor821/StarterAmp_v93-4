# Fender Hot Rod DeVille (410 / 212) - schematic read

Source: `3c81df67-hotrod_deville.pdf` (2 pages). Page 1: "Schematic Diagram, Hot Rod DeVille", dwg 050384 Rev B (EC 1701, 24-May-96; Rev A "Released PR323" 13-May-96), Fender Musical Instruments, Corona CA, drawn M. Wilkens, CAD print (text clear). Page 2: "Service Diagram H/R DeVille, H/R DeVille 212" dwg 050386 Rev A (PCB layout with test points and footswitch schematic 050419). Higher assembly 050388 PCB assy, Hot Rod DVL/212.
Conventions: "(?)" = not certain; "(computed)" = my arithmetic. The tube preamp DC plate voltages are NOT printed; test points TP2, TP5, TP7, TP8, TP10, TP11, etc. are AC signal levels (see section 9). Printed DC: cathode voltages, supply nodes, bias, op-amp rails.

Printed notes (page 1): all resistors 1/4 W 5 % carbon film unless stated; voltages with a true-RMS DVM (1 Mohm AC / 10 Mohm DC), +/-20 %; test conditions: 120 VAC 60 Hz, no input signal for DC, reverb control full CCW, other controls 50 %, NORMAL BRIGHT switch OUT, **DRIVE ("YELLOW") mode selected**, bias set to 30 mA per output tube. Note 5: no signal, rated line voltage, adjust BIAS ADJ pot R82 for **+60 mV at TP30** (R66 1 ohm, so 60 mA total, 30 mA per tube, computed); the voltage at the pot wiper is for reference only. Note 6: TP21/TP22 vary with reverb pan; the voltage gain of U2B should be 82. Note 7: colours in parentheses are for 230/240 V units. Note 8: **"MORE DRIVE" adds 12.7 dB of gain to the DRIVE mode.** Output test: 21.1 V RMS into 8 ohm (or 14.9 V RMS into 4 ohm) at 5 % THD with 286 mV 1 kHz at J4, presence and reverb full CCW (about 55.6 W / 55.5 W, computed).

## 1. Tube / semiconductor complement and heater notes
| Ref | Part | Function |
|---|---|---|
| V1A / V1B | 12AX7A | Preamp stage 1 (input) / stage 2 (tone-stack driver) |
| V2A / V2B | 12AX7A | Post-stack stage (always in circuit) / extra drive stage (Drive channel only) |
| V3A / V3B | 12AX7A | Long-tail phase inverter |
| V4, V5 | 6L6GC | Output, push-pull, 2 tubes |
| U1A/U1B, U2A/U2B | TL072 | U1A preamp-out buffer; U1B effects-return gain stage; U2A reverb driver; U2B reverb recovery |
| U3A/U3B | 4560 | Footswitch/relay control comparators |
| Q1, Q2 | J111 JFET | Switched cathode bypass for V2A (C8) and V2B (C9) = MORE DRIVE |
| Q3 2N4403, Q4 2N4401 | BJT | Relay / LED drive |
| K1, K2 | 2-pole relays | K1A/K1B/K2A/K2B "ON-DRIVE" (energised in Drive mode) |
| LD1 | bicolour LED (off = clean, yellow = drive, red = more drive) | |
| Rectifiers | 1N5062 x4 bridge (CR6-CR9) | HV supply, no tube rectifier |
- Heaters: two T2 windings of **3.26 VAC** each (CP13, CP14 printed), series (6.52 V total printed, i.e. 3.26 + 3.26); **R80, R81 100 ohm 1/2 W** from the heater leads to ground; V1-V3 heater pins 4/5 and 9 as drawn (pins 9 centre), V4/V5 pins 2/7; pilot lamp LA1 T47.
- Mains: power switch S4A/S4B (double pole), fuse F1 3 A at 100 V (printed "100V: 3A; 230V: F1.6A; 240V: F1.6A"), thermistor TH1 (label reads "C60-11" (?)), transformer T2 041752 (export T3 041753).

## 2. Stage by stage
All stages 12AX7A; plate loads return to rail X (+388 V).
### Input
- Jacks **J1 (input 1) and J2 (input 2)**: **R1 68k + R2 68k** in series, grid at the junction; **R3 1 Mohm** from the junction to ground; jack contacts as drawn (J2 pads input 1 as usual); **R63 10 ohm** "to pot brackets" ground tie. TP1 = 4.0 mV, 1 kHz sine test signal.
### V1A (first stage; common to both channels)
- Plate load **R4 100k 1/2 W** to X; cathode **R5 1.5k || C1 47 uF/16 V**, cathode **+1.93 V (TP3)**. Plate AC 196 mV (TP2) for 4.0 mV in (about 49x, computed).
- Two outputs from the plate:
  - **Clean path**: C18 **.022 uF/400 V** -> R9 **220k** -> **Volume R6 250k "15A"** (wiper to K1A pin 6, normally closed).
  - **Drive path**: C23 **1.5 nF/400 V** -> R52 **180k** -> **Drive R7 250k "15A"** (wiper to K1A pin 8, normally open). The 1.5 nF/180k high-pass thins the signal before the Drive pot.
- **K1A (ON-DRIVE)**: common pin 4 -> R41 **10k** -> V1B grid.
### V1B
- Grid leak **R44 180k** (to ground), grid stopper R41 10k; plate load **R11 100k 1/2 W**; cathode **R10 1.5k** (+1.93 V, TP6), **C4 .068 uF/100 V** from the cathode to node N4 with **R43 100k** to ground.
- **S1A NORMAL/BRIGHT**: BRIGHT connects N4 (C4/R43) to line L, which is grounded through K1B pins 11 to 13 when the relay is not energised (clean mode) and otherwise only reaches ground through **R102 1 Mohm**. Effect (computed, qualitative): in clean mode with BRIGHT, C4 bypasses R10 (cathode bypass, more gain, treble emphasis); in Drive mode the bright switch is effectively ineffective. Switch-contact numbering read from a mid-resolution crop (?).
- Plate AC 324 mV (TP5). Plate -> tone stack.
### V2A
- Grid from the Treble wiper via **R8 100k**; **C3 250 pF/1 kV** from the grid to node N3; N3 -> **R27 1 Mohm** to ground and to **K1B pin 9 (N.O.)** (common pin 13 grounded). In Drive mode K1B grounds N3 and puts 250 pF across the grid (R8 100k with 250 pF, computed corner about 6.4 kHz, treble roll-off); in clean mode C3 is in series with 1 Mohm and has no practical effect.
- Plate load **R16 100k 1/2 W**; cathode **R17 1.5k** (+1.93 V, TP9) with **C8 1 uF/100 V** to Q1 (J111) drain; **R23 100k** from the C8/Q1 node to ground. Plate AC 1.09 V (TP8).
- Plate -> **C2 .047 uF/400 V** -> node P2.
### Relay routing after V2A (K2A, K2B)
- **Clean mode** (relays de-energised): P2 -> **R18 130k** -> K2A pin 6 (N.C.) -> K2A common 4. K2B common 13 -> R20 100k is grounded via pin 11 (V2B grid grounded through R20, V2B muted).
- **Drive mode** (energised): K2B connects P2 (with **R42 1 Mohm** to ground as the grid reference) via pin 9 -> pin 13 -> **R20 100k** -> V2B grid; **R19 220k** from the V2B grid to ground. K2A connects the Master wiper path (R45 47k) via pin 8 to common 4.
### V2B (Drive channel extra gain stage)
- Plate load **R22 100k 1/2 W**; cathode **R21 1.5k || C9 22 uF/25 V** (C9 connected via Q2 J111), +1.93 V (TP12); plate -> **R25 100k** -> **C10 .047 uF/400 V** -> **Master R26 100k B** (top). **C11 390 pF/1 kV** from the Master top to ground (treble roll-off, shunts the pot top); Master wiper -> **R45 47k** -> K2A pin 8. Plate AC 17.1 V (TP11), V2B grid 736 mV (TP10).
### MORE DRIVE (Q1, Q2)
- Q1 shunts C8 (V2A cathode bypass, 1 uF) and Q2 shunts C9 (V2B cathode bypass, 22 uF) when the J111 gates are driven to about 0 V (clamped by CR16 1N4448, filtered by C44 .068 uF/100 V). Gate drive line "MORE_DRV" from U3A output via R87 100k: **-15.5 V** normal, **-15.4 V** drive, **+16 V** more drive (footswitch table). Effect (computed, qualitative): bypassing the 1.5k cathode resistors raises the stage gain, printed as +12.7 dB total.
### Output of preamp
- K2A common 4 (**TP13 1.51 V** AC): feeds the power-amp-in switching jack J4 (pin 4) and a divider **R28 47k / R29 2.7k** (division 0.054, computed) to U1A. **CR1, CR2 1N5245B 15 V zeners** back-to-back from this node to ground (clamp).
- **U1A TL072** unity buffer (pin 3 from divider, output to pin 2): output (TP14/TP15 84.6 mV) -> **C12 .47 uF/100 V** -> **R31 1.5k** -> **J3 PREAMP OUT**, **R30 100k** to ground. Supply +/-16 V with C54, C55 .1 uF/50 V.

## 3. Tone stack (shared by both channels; located between V1B plate and V2A grid)
- Topology: Fender/Marshall-style TMB. **R12 130k** slope resistor (plate to common node); **C7 250 pF/1 kV** from the plate to the Treble top; **Treble R13 250k "30A"**; **C5 .1 uF/400 V** from the slope node to the Treble-bottom/Bass-top node; **Bass R14 250k "15A"** (wiper tied to the top, so used as a variable resistor); **C6 .022 uF/400 V** from the slope node to the Middle pot (as drawn); **Middle R15 25k B** (linear, bottom grounded); **R105 220k** from the Bass top/wiper node to ground (added late, "last instance" list) (?, connection as read).
- Treble wiper -> R8 100k -> V2A grid. Taper codes "30A"/"15A" printed (meaning of the 30/15 prefix not resolved).
- **Slope resistor 130k** (Twin/Super AB763: 100k). Cap values .1 / .022 (Twin AB763 .1 / .047; Super Reverb AB763 .1 / .022).
- Switching: the relays do not change the stack, only the stage before and after it.

## 4. Reverb and tremolo
- No tremolo/vibrato. Reverb is op-amp driven (spring pan 050394, input/output as drawn):
  - **Driver U2A TL072**: non-inverting; input pin 3 from the power-amp-in line via **C13 470 pF/50 V**, **R32 1 Mohm** to ground; inverting pin 2 with **R34 47 ohm + C14 22 uF/25 V** to ground, feedback **R33 4.7k** from the output (gain about 101 above roughly 150 Hz, computed). Output (TP20 11.6 V AC) drives the reverb pan input.
  - **Recovery U2B TL072**: input from the pan output via **R46 10k** (pin 5), **R35 100k** to ground; pin 6 with **R37 2.7k + C16 .47 uF/100 V** to ground, feedback **R36 220k || C15 47 pF/100 V**: gain 1 + 220k/2.7k = 82.5 (printed note: 82). Output (TP22 792 mV) -> **C17 .47 uF/63 V** -> **R103 1.5k** with **C52 .0068 uF/100 V** -> **Reverb R39 25k B**.
  - Reverb return wiper -> **R38 470k** -> power-amp input node (TP23), which is the PI grid node; the dry signal arrives via **R40 220k**. Dry/wet mixing ratio at TP23 therefore set by 220k and 470k.
  - **C28 47 pF/1 kV** from node TP23 to ground.

## 5. Phase inverter (V3A/V3B 12AX7A long-tail)
- Input node TP23 (971 mV) -> **C24 .022 uF/400 V** -> V3A grid; grid leaks **R53 1 Mohm** (V3A) and **R54 1 Mohm** (V3B) to node M (TP26 **+42.8 V DC**).
- Tail: cathodes joined -> **R55 470 ohm** -> M -> **R56 6.8k 1 W** -> NFB node. Total tail 7.27k to the NFB node, which has **R68 4.7k** to ground, so tail current about 3.7 mA total (computed from +42.8 V / 11.5k).
- V3B bottom grid: **C25 .1 uF/250 V** from the NFB node to the V3B grid (feedback injection point).
- Plate loads: **R57 82k 1/2 W** (V3A), **R58 100k 1/2 W** (V3B) to node Y (+441 V). **C30 47 pF/1 kV** between the two plates. Plate AC 14.7 V each (TP24, TP25).
- Outputs: **C26 .1 uF/630 V** and **C27 .1 uF/630 V** to the phase nodes.

## 6. Output stage
- **2 x 6L6GC (V5 upper, V4 lower)**; grid stoppers **R64, R65 1.5k**; grid leaks **R59, R60 220k** from each phase node to the bias bus "C-" (**-54.6 V** printed; computed range -46.6 to -58.2 V); screens **R61, R62 470 ohm 1 W** from node Z (+483 V); plates to the OT primary (BLU, BRN) with centre tap RED (CP18 B+). Plate voltage: OT centre tap +483 V (Z); plate-to-ground **CR4, CR5 diodes marked "R3000"** from each 6L6 pin 3 (plate) to ground, function not stated, likely a transient clamp (?).
- Cathodes: tied together through **R66 1 ohm 1 W** (bias test, **TP30 +60 mV** = 60 mA total), **CR3 1N5062** across R66 (protection).
- **Bias**: fixed, adjustable by **R82 25k B (BIAS ADJ)**, supply: T2 BRN winding (CP15/CP16) -> **C37 100 uF/100 V** -> **CR15 1N5062** (with CR10 1N5062 clamp) -> raw node **-74.3 V** with **C43 100 uF/100 V** -> **R76 10k** -> node N5 with **R83 51k** to ground; R82 pot top at N5, bottom via **R77 100k** to ground; wiper = bias bus **-54.6 V** with **C38 22 uF/63 V**.
- **OT T1 041754**: primary CT B+; secondary leads labelled GRN 8 ohm, GRN/YEL 4 ohm, YEL, WHT 2 ohm ("CP21 NC"), BLK common (as drawn; which lead carries which tap is partly (?)). Two wiring versions drawn:
  - Hot Rod DeVille (410): **(4) 049832 8 ohm speakers**, EXT SPKR 8 ohm and MAIN SPKR 8 ohm jacks (J6, J7); TP29 7.82 V.
  - **Hot Rod DeVille 212**: **(2) 037617 8 ohm speakers**, output wiring box "EXT SPKR 4 ohm / MAIN SPKR 4 ohm", 8 ohm tap NC; TP29 5.53 V.
  - Output power per test: 21.1 V RMS into 8 ohm (about 55 W, computed).
- **Global NFB**: gray lead from the speaker hot (the lead labelled GRN/YEL, 4 ohm tap, as read (?)) -> **R69 47k** -> NFB node (R56 bottom, R68 4.7k to ground, C25 to V3B grid). Fraction R68/(R69+R68) = 4.7/51.7 = 0.091 (-20.8 dB, computed, LF/mid). **Presence R67 25k "30C"** with **C29 .1 uF/63 V** in series, shunting R68: at HF it reduces the NFB fraction (more presence), so the NFB is HF-shaped by presence.
- Tube cathode current check: 30 mA per tube at 483 V plate is about 14.5 W per tube (computed).

## 7. Supply
- T2 HV winding (two RED leads CP11/CP12, voltage not printed) -> **bridge CR6-CR9 1N5062 (4x)** with **C53 8200 pF/1 kV** across one leg; **STANDBY S5** in the AC lead to the bridge (CP12 side) (as drawn).
- Reservoir **C31, C32 100 uF/350 V** in series (about 50 uF, computed), each with **R70, R71 220k 1/2 W**; node **+485 V "B+"**.
- **SP1** (fuse-like symbol), **choke L1 022707**, **SP2** (fuse-like symbol); node **Z +483 V**: **C33, C34 47 uF/350 V** series (about 23.5 uF) with **R72, R73 220k 1/2 W**; Z feeds the OT centre tap and screens.
- **R74 4.7k 1 W** -> **Y +441 V** (C35 22 uF/500 V): PI plate loads.
- **R75 10k 1 W** -> **X +388 V** (C36 22 uF/500 V): all preamp plate loads.
- Consistency check (computed): preamp about 5.2 mA through R75 gives about 53 V drop (441 to 388, matching); PI about 3.7 mA plus preamp 5.2 mA through R74 gives about 42 V drop (matching 483 to 441).
- Op-amp / control rails: T2 BRN centre-tapped winding **27.3 VAC** each side -> **CR11, CR12 1N5062** half-wave, **C39, C42 100 uF/100 V** (+33.3 V / -33.3 V) -> **R78, R79 330 ohm 5 W** -> **CR13, CR14 1N5353B 5 W (16 V) zeners** with **C40, C41 1000 uF/35 V** -> **+16 V / -16 V**.
- Sag sources (qualitative): solid-state bridge (no rectifier sag), choke, 4.7k/10k RC droppers; plates and screens on the same filtered node Z (+483 V), so power-stage sag is mainly transformer regulation plus about 50 uF + 23.5 uF of filtering.

## 8. Footswitch and channel control (summary)
- Footswitch jack **J5**; front-panel **S2A CHANNEL SELECT** and **S3A MORE DRIVE** (R99 47 ohm / C50 22 uF/25 V and R100 47 ohm / C49 22 uF/25 V, diodes CR21, CR25 1N4448); source from the 27.3 VAC winding through **R97 820 ohm 2 W** -> CR23 1N4448 -> CR28 1N5240B 10 V zener, C46 .22 uF/63 V, R95 470k -> **U3B 4560** (channel select) -> TP33; **U3A 4560** for drive / more drive (TP34 reference, TP35 input, TP36 output, CR17/CR22 1N5231B 5.1 V zeners, CR26 1N5240B). **Q4 2N4401 / Q3 2N4403** drive the relay coils K1/K2 (with CR18 1N4448, CR19 1N5228B 3.9 V zener, R85 270 ohm, R88 1.8k, R104 270 ohm) and LD1 (R84 130 ohm).
- Printed footswitch voltages: TP31 +1.39 V normal / +1.08 V drive / +1.08 V more drive; TP33 +16 V normal / -13.0 V drive / -13.5 V more drive; TP34 -12.5 V / -4.90 V / -4.14 V; TP36 -15.5 / -15.4 / +16 V; TP32 +0.523 V (footswitch, CH SEL off) / +2.54 V (on) / +0.536 V (front panel out) / +9.69 V (front panel in).
- The footswitch overrides the front-panel switches.

## 9. Printed AC test-point levels (4.0 mV, 1 kHz at TP1, drive mode, controls at 50 %)
TP2 196 mV (V1A plate), TP4 15.5 mV (V1B grid), TP5 324 mV (V1B plate), TP7 42.0 mV (V2A grid), TP8 1.09 V (V2A plate), TP10 736 mV (V2B grid), TP11 17.1 V (V2B plate), TP13 1.51 V (K2A common), TP14/TP15 84.6 mV (U1A), TP19 1.13 V (U2A input), TP20 11.6 V, TP21 9.9 mV, TP22 792 mV, TP23 971 mV (PI input), TP24/TP25 14.7 V (PI plates), TP27/TP28 94.3 V (6L6 plates, conditions not stated), TP29 as above. TP16-TP18 with a cable from J3 to J4: 84.0 mV / 1.69 V / 1.50 V.

## 10. Not legible / uncertain
- Preamp DC plate voltages are not printed (computed 388 V - 100k x 1.29 mA = about 259 V per stage).
- Meaning of "15A", "30A", "30C" pot codes; bass pot R105 connection; exact S1A contact numbering.
- Which OT lead the gray NFB line taps; individual OT tap impedances (2/4/8 ohm labelled).
- Function of CR4/CR5 "R3000"; SP1/SP2 symbols; TH1 part number; HV winding voltage.
- Power-amp-in jack J4 switching: contact-by-contact routing not fully traced (U1B output via R51 47k enters the J4 contacts).
- The service-diagram page 2 (layout) gives only test points and the +485/+483/+441/+388 labels (page 2 shows "+388 V", "+441 V", "+483 V", "+485 V", "-74.3 V", "+33.3 V", "-33.3 V", "-16 V/+16 V", "27.3 VAC"), consistent with page 1.
