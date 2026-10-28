# Sunn Model T (D-1029, ECN 441, 9/25/1973) and Model T Super (D1029 rev C, 2-18-78)

Source images: `sunnT-1.png` (3518x2359, clean line drawing, nearly every value legible) and `sunnTsuper-1.png` (2747x2122, lower resolution; the power section reads well, the tone stack is only partly legible). Values marked (?) are not reliable. Computed values use my own nodal solver of the plugin's tone-stack topology (taper per the printed notes) and simple RC formulas; they are estimates.

Printed notes (both sheets): "All resistors 1/2 W 5 % unless noted. Tone pots R15, R16, R18, also R44, R48, are linear taper. R25 is reverse log. R8, R9, R19 are linear taper." Voltage conditions: 1 kHz sine into BOTH IN, brite and normal volumes, bass, mid, treble and master at 10, presence set for "10", load 4 ohm resistive with the output selector at 4 ohm, input level adjusted to 24.5 V RMS across the load (150 W RMS). Bias set for -55 V with the standby switch on. Preamp stages must be shielded for the hum-balance adjustment.

## 1. Model T (1973) stage by stage

### Tube complement
V1 12AX7A (one envelope, two triodes, one per input: brite and normal); V2 12AX7A (voltage-gain triode then cathode follower); V3 12AX7A (long-tail phase inverter); output V4 to V7 four 6550 (printed "6550" at all four positions). 

### Inputs and V1
- Jacks: J1 Brite in, J2 Brite out, J3 Both in, J4 Normal in, J5 Normal out (the jacks are wired so that J3 jumpers both channels).
- Grid stoppers R1 and R4 68K on each half; grid leaks R2 1M and R3 1M to ground.
- Shared cathode: R5 680 ohm parallel with C1 250 uF/25 V (so both halves have the same bias; 1/(2 pi 680 250 uF) is under 1 Hz, i.e. fully bypassed).
- Plate loads R6 100K (brite half) and R7 100K (normal half) to the C rail (330 V printed "C 330 V"). Plate voltages 227 V (brite) and 232 V (normal) no signal (chart points 1 and 3, both 218 / 219 V at 150 W); cathode 1.71 V.
- Coupling: C2 0.022 uF and C3 0.022 uF into the volume pots R8 and R9, 1M linear taper (corner about 7 Hz).
- Bright circuit: C4 470 pF in parallel with R10 470K on the brite channel volume pot's wiper path (a cap across the series resistor, not across the pot). R10 470K (brite) and R11 470K (normal) are the mixing resistors that sum the two channels at V2's grid.
- No channel switching: two inputs and two volumes mixed passively. Gain structure: one 12AX7 stage per input, one more gain stage in V2, a cathode follower into the passive stack. This is a clean/headroom amp: three triode stages before the stack plus the long-tail PI; there are no clipping diodes or cold clippers.

### V2 (gain stage + cathode follower)
- First triode: plate R12 100K to C (330 V), plate 203 V (point 4), cathode R13 820 ohm parallel with a small electrolytic (22 uF printed "2.2" / "22", (?)); cathode 1.3 V; corner 1/(2 pi 820 22 uF) = about 8.8 Hz if 22 uF.
- Second triode: cathode follower (plate to C, grid directly from the first plate, cathode load R14 100K to ground, cathode 205 V (point 5)). Source impedance into the stack about 1K (1/gm), low because of the follower.

### Tone stack (passive TMB, after the cathode follower)
Printed values:
- C5 270 pF (treble cap, from the follower cathode to the treble pot top)
- R15 250K treble pot (linear)
- R16 1M bass pot (linear)
- R18 25K mid pot (linear)
- R17 56K slope resistor
- C6 0.022 uF and C7 0.022 uF (the bass and mid caps)
- R19 1M master volume pot (linear taper), followed by C8 0.022 uF into V3A (R21 1M grid leak).
This is the same topology as the plugin's stack type 10 (`{250e3, 1.0e6, 25e3, 56e3, 270e-12, 22e-9, 22e-9, 1.0e3, 1.0e6, ...}`), so the plugin values reproduce the schematic exactly.
Computed with the solver (source 1K cathode follower, load 1M volume, all pots linear): noon insertion loss mean about -9.4 dB (100 Hz to 5 kHz), -12.5 dB at 1 kHz, notch about -13.6 dB near 656 Hz; all-10 minimum -11.1 dB near 450 Hz, maximum -0.6 dB. This is a very low loss stack compared with a Marshall-style stack because the 56K slope resistor (against 33K in a JCM800) and the 1K source dominate; the mid pot is only 25K.

### V3 phase inverter
- 12AX7A long-tail: V3A grid from C8 through R21 1M; V3B grid from the feedback network through C10 0.1 uF with R24 1M. Cathode network: R22 4.7K and R23 470 ohm (exact connection (?); a measured cathode of 38 V no signal (point 9) suggests a combined cathode resistance of about 4.7K to ground with the 470 ohm in series with one grid return).
- Plate loads: R27 82K (V3A) and R28 120K (V3B) from the B rail (+400 V); measured plates 271 V (V3A) and 251 V (V3B). The unequal loads are a deliberate balance trim.
- C11 270 pF between the two plates (high-frequency roll-off, corner depends on the plate impedances).
- Coupling C12 and C13 0.25 uF/600 V into the output grids; grid leaks R29 100K and R30 100K to E (-55 V bias).

### Presence and negative feedback
- NFB taken from the 16 ohm tap: R20 22K to R26 10K to ground; the junction goes through C10 0.1 uF to V3B grid. Flat feedback ratio R26/(R20 + R26) = 10K/32K = 0.31, about -10 dB (estimate, assuming the V3B grid network is high-impedance).
- Presence: R25 25K reverse-log pot with C9 0.022 uF/100 V shunting the feedback junction to ground. At full presence the cap shunts the divider at high frequency: at 5 kHz the 0.022 uF is about 1.45K, so the divider falls from -10 dB to about -25 dB (estimated), i.e. presence removes feedback at high frequencies.
- R51 33K and R52 680 ohm form the LINE OUT (J8) divider from the 16 ohm tap.

### Output stage
- Four 6550 in parallel push-pull (two pairs per phase).
- Grid stoppers R31, R32, R33, R34 1K.
- Plate resistors R35, R38, R39, R42 are printed "47 ohm 5 W" on the Model T (the Super prints "50 ohm 5 W"; the "470" that appears in the older code doc reading is the 47 ohm + the ohm symbol misread). These 47 ohm resistors are in series with each plate.
- Screen resistors R36, R37, R40, R41 1.5K 5 W; drawn from pin 4 to a tap line that runs to small tap marks on the output transformer primary rather than to the plate or to a B+ node (this is an ultralinear-style connection); voltage chart: plates 507 V (no signal) / 472 V (150 W), screens 504 V / 459 V, bias at the grids -53 V / -55 V.
- Output transformer T1: A center tap +500 V ("A +500 V" at the primary center); secondary taps selected by S4 (24-0111): 8, 4, 2 ohm, plus the 16 ohm tap for feedback and line out. Two speaker jacks J6, J7.

### Supply (Model T)
- Rectifier: two diodes CR2 and CR3 (22-0416 / 24-0112) full-wave from the red center-tapped winding; standby switch S3; reservoir C17 20 uF/600 V; chokes L3 and L4 (28-3100) in series; C18 20 uF/600 V. Node A 519 V no output / 481 V at 24.5 V rms (150 W). B: 419 V no output / 390 V (printed "<19v (no output) 390V", read as 419 V). C: 351 V no output / 330 V at power. Dropper R46 10K 2 W and R47 10K 2 W to C19/C20 (60-80 uF/450 V, 11-0080).
- Bias: CR1 1N4004, R43 330 ohm, R44 10K (linear taper pot), R45 18K, C15 50 uF/150 V and C16 50 uF/150 V; bias test point -55 V.
- Heaters 6.3 V with hum-balance pot R48 10K and R49/R50 1K; mains 120 V 60 Hz, 1.95 A breaker (52-0700), polarity switch S2 with C14 0.047 uF/600 V, pilot light, aux AC outlet.

### Computed corners (estimates)
- C2, C3 (0.022 uF into 1M) 7.2 Hz; C8 0.022 uF into about 1M 7.2 Hz; C12, C13 0.25 uF into 100K about 6 Hz. So the whole amp is flat well below the guitar range; low-frequency behaviour comes from the OT and supply (power amp is a big headroom amp).

## 2. Model T Super (1978 revision C)
The Super is the same chassis family (voltage chart numbers are copied unchanged from the Model T: A 519 V no load / 481 V; B 419 V / 390 V; C 351 V / 330 V, bias -55 V, 150 W), with these printed differences that I could read:
- Inputs and V1: same as the Model T (R1, R4 68K, R2, R3 1M, R5 680 ohm, C1 250 uF/25 V (printed "220 uF 25V" or 250 uF, (?)), R6 and R7 100K, C2 and C3 0.022 uF/600 V), but the volumes are labelled CH1 VOL and CH2 VOL (1M, linear per note 4) and C4 470 pF with R10 470K and R11 470K as before.
- Gain stage V2A: plate R12 printed "68K" (Model T: 100K), grid fed from the volume mix, cathode R13 470 ohm (Model T: 820 ohm) with R17 10K in the cathode area; coupling C21 0.022 uF/600 V (printed ".02"); R14 100K.
- Stack: BASS R16 1M linear, TREB R15 1M linear (Model T: 250K), MID R18 25K reverse log ("R18 25K REV LOG") with a series R54 1K and a three-position mid-shape switch ("SS") with capacitors C23, C24, C25 (values near .047 uF, .0047 uF, .1 uF (?)) and an inductor L1 in the mid-switch leg (printed "L1 1H?" / "680 (?)"). Other stack parts printed: R53 33K, C5 (about .001 uF, 5 %), C6 (about .0?? uF), C7 (about .0?? uF/100 V), C22 (about .0015 uF/100 V), R17 10K. Because the capacitor values are not legible, no insertion loss was computed for the Super's stack.
- Extra gain stage V2B: plate R67 120K (printed "R67 120K"), coupling C26 0.022 uF/600 V, then a VOL pot (1M) with R55 22K, R56 22K and C27 10 uF/100 V in the presence/feedback network; presence R25 22K reverse log (printed "22K"), C9 0.22 uF/100 V printed ".22".
- PI: same long-tail (R22 4.7K, R23 470 ohm, R24 1M, R21 1M, R26 10K, R20 22K, R27 82K, R28 120K, C11 270 pF, C12/C13 0.25 uF/600 V, R29/R30 100K).
- Output: four 6550; plate resistors "50 ohm, 5 W" (R35, R38, R31, R42), screens 1.5K 5 W (R36, R37, R40, R41), grid stoppers 1K (R31 .. R34, labelled "R31-1K" etc.), the same screen-to-OT-tap arrangement.
- Speaker selector: two jacks, output selector with 8, 4, 2 ohm positions printed.
- Supply values as above (printed R46 10K 2 W, R47 10K 2 W, L2 and L3 chokes "28-3100", reservoirs 50 uF/600 V? (printed "50 uf" (?)), C19/C20 80 uF/450 V).

## 3. Uncertain or illegible
- Model T: whether the V2 first-stage cathode bypass is 2.2 uF or 22 uF; the connection of R22/R23 in the PI cathode area; whether the screen taps are true ultralinear taps (the drawing shows tap marks on T1 near R36/R37; I read them as taps).
- Model T Super: the cap and inductor values in the stack and the mid-shape switch network, R12 (68K vs 100K), R13 (470 vs 820 ohm), R67, the C1 value, and the electrolytic ratings in the supply.
