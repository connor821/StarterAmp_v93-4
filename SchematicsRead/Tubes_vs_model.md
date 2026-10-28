# Tube datasheets versus the StarterAmp tube model

Companion to `Tubes_datasheets.md`, which holds the datasheet values and their `(?)` marks.

## 0. Where the model numbers come from

`TUBES.md` and `DATASHEET_AUDIT_b58.md` contain almost no tube-model parameters.
- `TUBES.md` is a table of dissipation, max-voltage and gm figures. It says "nothing here is wired into the model".
- `DATASHEET_AUDIT_b58.md` is mostly about diodes, transistors, op-amps, speakers and mics. Its only tube content is the rectifier source-resistance (Rs) values.
- Neither doc mentions mu, rp, Kp, Vct, centre-tap voltage or a screen model for any tube.

So the model side of every comparison below comes from three places:
- the actual Koren tables in `Source/TubeNet.h`;
- the stage-curve headers in `Source/StageCurves.h`;
- the supply tuples in `Source/PowerSection.h`.

That is the only place model numbers exist. No source file was changed.

Model structures (`TubeNet.h`):

| Tube | Koren parameters |
|---|---|
| 12AX7 | mu 100, ex 1.4, kg1 1060, kp 600, kvb 300 |
| 12AT7 | mu 60, ex 1.35, kg1 460, kp 300, kvb 300 (marked "approximate") |
| 12AY7 | mu 44, ex 1.35, kg1 780, kp 200, kvb 300 (marked "approximate") |
| EL34 | mu 11, ex 1.35, kg1 650, kg2 4200, kp 60, kvb 24 |
| 6L6 | mu 8.7, ex 1.35, kg1 1460, kg2 4500, kp 48, kvb 12 |
| KT88 | mu 8.0, ex 1.35, kg1 720, kg2 4500, kp 40, kvb 20 (approximate) |
| EL84 | mu 19, ex 1.35, kg1 1250, kg2 4500, kp 25, kvb 25 (approximate) |
| 6V6 | mu 10, ex 1.35, kg1 1250, kg2 4500, kp 35, kvb 18 (approximate) |

- Pentode: Ip = 2*E1^ex/kg1 * atan(Vp/kvb), with E1 = (Vg2/kp)*ln(1 + exp(kp*(1/mu + Vg1/Vg2))).
- The kg2 field is never used. No screen current is modelled; the screen is a fixed supply per table.
- TubeNet.h is an offline generator; the plugin runs the baked curves in `StageCurves.h`.

I evaluated the model equations numerically (Python, same formulas) at the datasheet operating points. The "model" numbers below are those results, not read from the docs.

---

## 1. Parameters that disagree (both numbers shown)

### 1.1 Preamp triodes

| Tube | Parameter | Model | Datasheet | Comment |
|---|---|---|---|---|
| 12AX7 | mu | 100 | 92 nominal (min 78) (EH) | Model mu is 8.7 % above nominal. |
| 12AX7 | Ia at 250 V, -2 V | 0.95 mA | 1.19 mA read from curves; printed typical 1.2 mA (min 0.75) | Model is 20 % low. |
| 12AX7 | gm at 250 V, -2 V | 1.67 mA/V | 1.7 nominal (min 1.4) | Agrees. |
| 12AX7 | rp at 250 V, -2 V | 53.7k | 54.1k | Agrees. |
| 12AX7 | Ia, Vg = -1 V, Vp 100 / 150 / 200 / 250 / 300 V | 0.10 / 0.79 / 1.94 / 3.37 / 5.02 mA | ~0.5 / 1.24 / 2.2 / 3.36 / 4.64 mA | Model knee is sharper. The sheet is 5x higher at 100 V, 57 % higher at 150 V, and 8 % lower at 300 V. |
| 12AX7 | Ia, Vg = 0 V, Vp 50 / 100 / 150 / 200 V | 0.72 / 1.89 / 3.33 / 4.98 mA | 1.14 / 2.1 / 3.2 / ~4.5 mA | Low-Vp end 37 % low; 200 V end 11 % high. |
| 12AX7 | Ia, Vg = -2.5 V, 250 V | 0.34 mA | 0.55 mA | Model cut-off is slightly harder. |
| 12AX7 | stage operating point, B 250 V, Rp 100k, Rk 1500 | 0.80 mA, Vp 169 V, Vk 1.20 V | 0.88 mA, Vp 161 V, Vk 1.32 V | Sheet value from my mu-shift interpolation of the digitised points. About 9 % lower current in the model. |
| 12AX7 | same, Rk 2700 / B 250 | 0.58 mA | 0.63 mA | 8 % lower. |
| 12AX7 | same, Rk 2700 / B 300 | 0.71 mA | 0.76 mA | 7 % lower. |
| 12AX7 | same, Rk 1500 / B 350 | 1.14 mA | 1.19 mA | 4 % lower. |
| 12AX7 | same, Rk 820 / B 320 | 1.34 mA | 1.36 mA | Agrees. |
| 12AT7 | mu | 60 | min 50, no nominal printed (EH) | Not a disagreement against a minimum; the model is above the guaranteed minimum. |
| 12AT7 | Ia at 250 V, -2 V | 12.8 mA | 7.0-13.0 mA | Inside the window but at the top edge. |
| 12AT7 | gm at 250 V, -2 V | 7.2 mA/V | min 4.5 (3.5 in service); no nominal | Model is 60 % above the printed minimum, so it is probably too high. Needs a nominal value to settle. |
| 12AT7 | plate load | 47k | none printed | No curves on the sheet, so the 12AT7 model cannot be curve-checked. |
| 12AY7 | everything | mu 44, rp about 25k (PassiveStack.h estimate) | no sheet | Cannot be checked. |

Overall the preamp model is the best match in the set. The 12AX7 curves are within about 10 % at the plugin's operating conditions. Mismatches concentrate near cut-off and below about 150 V plate.

### 1.2 Power tubes: single-tube class A points

Model values come from evaluating the Koren pentode at the sheet's screen voltage.

| Tube | Condition | Parameter | Model | Datasheet |
|---|---|---|---|---|
| EL34 | 250 / 250 V, -14.5 V | Ia | 85.1 mA | 70 mA (Philips) |
| EL34 | 250 / 250 V, -13.5 V | Ia | 97.0 mA | 100 mA (Philips; 6CA7 EH 100 +/-20 mA) |
| EL34 | 250 / 250 V, -13.5 to -14.5 V | gm | 11.5-12.2 mA/V | 11-12.5 mA/V (Philips), EH 11 |
| EL34 | same | rp | 40-45k | 17-20k (Philips ri); EH printed rp 15k |
| EL34 | mu(g2-g1) | mu | 11 | 11 (Philips) |
| 6L6 | 250 / 250 V, -14 V | Ia | 81.0 mA | not printed |
| 6L6 | 250 / 250 V, -14 V | gm | 6.87 mA/V | 5.2 (EH) |
| 6L6 | same | rp | 98k (350 V / -18 V: 280k) | 33k (EH) |
| 6L6 | mu | mu | 8.7 | not printed |
| KT88 | 250 / 250 V, -15 V | Ia | 185.2 mA | 140 mA (Genalex) |
| KT88 | same | gm | 13.9 mA/V | 11.5 (Genalex) |
| KT88 | same | rp | 25.3k | 12k (Genalex) |
| KT88 | 400 / 225 V, -16.5 V | Ia | 125 mA | 90 mA (EH typical) |
| KT88 | same | gm | 12.2 mA/V | 9.5 (EH typical) |
| KT88 | mu(g1-g2) | mu | 8.0 | 8 (Genalex) |
| EL84 | 250 / 250 V, -7.3 V | Ia | 54.7 mA | 48 mA (Philips); EH window 44-64 mA |
| EL84 | same | gm | 4.61 mA/V | 11.3 (Philips; EH min 9.0, nominal 11.3) |
| EL84 | same | rp | 67.8k | 38k (Philips; 40k at 7k load) |
| EL84 | mu(g2-g1) | mu | 19 | 19 (Philips) |
| 6V6 | 250 / 250 V, -12.5 V | Ia | 81.7 mA | 45 mA (Tung-Sol) |
| 6V6 | same | gm | 6.88 mA/V | 4.1 (Tung-Sol and EH) |
| 6V6 | same | rp | 64.0k | 52k (Tung-Sol); EH printed 50k |
| 6V6 | 180 / 180 V, -8.5 V | Ia | 54.5 mA | 29 mA |
| 6V6 | 315 / 225 V, -13 V | Ia | 60.3 mA | 34 mA |
| 6V6 | mu | mu | 10 | not printed |

Summary of the pattern:
- EL34 current and gm agree; the model rp is 2-2.5x the sheets.
- 6L6, KT88, EL84 and 6V6 are all too soft on gm and too high on rp, so the model's pentode slope is shallower than the real tubes.
- 6V6 and KT88 also draw far more current than the sheets, as the curve comparison in 1.3 shows.
- EL84 is the opposite case: the model Ia is only 14 % high but its gm is 59 % low. At -7.3 V the model sits in its soft knee (cut-off near -Vg2/mu = -13 V), whereas the real tube's transfer curve is much steeper there.

### 1.3 Power tubes: plate-curve comparison (model versus printed curves)

Ia in mA at Vg2 = 250 V (screen voltage assumed for 6L6 and 6550). Sheet values are read from the curves; "model" is Koren at the same Vp, Vg1 and Vg2.

| Tube | Eg | Vp | Model | Sheet | Ratio model / sheet |
|---|---|---|---|---|---|
| 6V6 (Tung-Sol) | 0 V | 100 / 200 / 300 / 400 | 174 / 185 / 189 / 191 | 94.6 / 103.6 / 107.5 / 110.7 | 1.8 |
| 6V6 (Tung-Sol) | -10 V | same | 92.7 / 98.6 / 100.6 / 101.6 | 45.3 / 54.2 / 57.0 / 58.8 | 1.8-2.0 |
| 6V6 (Tung-Sol) | -20 V | same | 36.2 / 38.5 / 39.2 / 39.6 | 14.4 / 18.4 / 20.3 / 21.1 | 1.9-2.5 |
| 6V6 (Tung-Sol) | -25 V | same | 19.3 / 20.5 / 21.0 / 21.2 | ~4.5 / 6.8 / 8.7 / ~9.5 | 2.2-4.3 |
| 6V6 (EH) | -10 V | 110 / 210 / 310 / 410 | about 93 / 99 / 101 / 102 | ~51 / 55.8 / 58.3 / ~59.5 | about 1.8 |
| KT88 (EH) | 0 V | 100 / 200 | 398 / 427 | ~343 / ~368 | 1.16 |
| KT88 (EH) | -10 V | 100 / 200 / 300 | 239 / 256 / 262 | ~182 / ~197 / ~208 | 1.26-1.31 |
| KT88 (EH) | -20 V | 100 / 200 / 300 / 400 | 112 / 120 / 122 / 124 | ~67 / ~79 / ~86 / ~92 | 1.4-1.7 |
| KT88 (EH) | -25 V | 100 / 200 / 300 / 400 | 65 / 70 / 72 / 72 | ~26 / ~41 / ~46 / ~51 | 1.4-2.5 |
| 6550 (EH, screen unknown) | -20 V | 100 / 200 / 400 | model KT88 curve 112 / 120 / 124 | ~61 / ~72 / ~84 | 1.5-1.8 |
| EL34 (EH) | -10 V | 200 / 250 / 300 / 350 | 141 / 143 / 145 / 146 | ~130 / ~127 / ~129 / ~132 | 1.1 |
| EL34 (EH) | -15 V | 200 / 300 / 350 | 78 / 80 / 81 | 64.6 / 68.6 / 72.7 | 1.1-1.2 |
| EL34 (EH) | 0 V | 100 / 200 / 300 | 279 / 303 / 311 | ~252 / ~270 / ~280 | 1.1 |
| 6L6 (EH, screen unknown, assumed 250 V) | 0 V | 105 / 205 | 186 / 193 | 175.4 / 186.2 | 1.06 |
| 6L6 (EH) | -10 V | 105 / 205 / 305 | 105 / 109 / 111 | ~90 (?) / 95.3 / ~99 | 1.12-1.17 |
| 6L6 (EH) | -20 V | 105 / 205 / 305 | 42.5 / 44.1 / 44.6 | 37.0 / 43.4 / ~45 | 0.99-1.15 |
| 6L6 (EH) | -30 V | 105 / 205 / 305 | 8.9 / 9.2 / 9.3 | 8.7 / 12.2 / 14.3 | 0.65-1.0 |

Reading the table:
- Closest matches: EL34 (about 10 % high) and 6L6 (about 5-15 % high, if the sheet is at 250 V screen).
- Worst: 6V6, about 1.8x too much current at every grid voltage, and several times too much near cut-off. Its curve shape matches the sheet well (the 6V6 Tung-Sol 0 to -10 V ratio is about 2:1 in both), so it looks like a scale error in kg1 (model 1250), not a shape error.
- KT88 is 16 % too high at 0 V and 1.4-2.5x too high toward cut-off. The model has a much longer, softer tail.
- The 6550 option reuses this KT88 curve, so it inherits the same errors.

### 1.4 Idle currents and bias in the baked power curves

Header comments in `StageCurves.h` (per tube, in the order given there). These come from the generator's own bias search, so the supply voltage is not documented here; treat them as numbers to confirm.

| Tube | Model bias / idle (medium; hot) | Datasheet |
|---|---|---|
| EL34 | -32 V / 38.9 mA; -26 V / 87.4 mA | Philips class B: -32 V / 2 x 35 mA (Vb 375 V, Rg2 470 ohm), -38 V / 2 x 30 mA (425 V). The medium point agrees. |
| 6L6 | -53 V / 46.3 mA; -46 V / 92.0 mA | no AB1 table on the 6L6EH sheet |
| 6V6 | -15 V / 150.7 mA; -12 V / 176.9 mA | Tung-Sol push-pull AB1: -15 V / 70 mA total (35 mA per tube) at 250 V; -19 V / 70 mA total at 285 V. Model per-tube idle is 4.3x the sheet's. |
| EL84 | -6 V / 98.2 mA; -4 V / 110.4 mA | Philips class A: -7.3 V / 48 mA (-8.4 V / 36 mA). Model idle is 2.0x. |
| KT88 | -62 V / 62.9 mA; -54 V / 117.4 mA | Genalex AB1, tetrode: -34 V / 2 x 60 mA fixed bias (Vg2 300 V, 552 V plate); -30 V / 2 x 64 mA cathode bias. Idle matches; the bias magnitude is roughly 2x. |

The 6V6 and EL84 idle values exceed the datasheet cathode-current limits: the 6V6 at 150 mA is almost double the 80 mA limit on the EH 6V6 and over three times the Tung-Sol class A maximum-signal current (47 mA). The EL84 at 98 mA exceeds the 65 mA EH cathode limit by 50 %. Both figures should be confirmed against the supply used to generate those curves.

### 1.5 Screen behaviour

| Item | Model | Datasheet |
|---|---|---|
| Screen current | not modelled (kg2 unused) | 6V6 4.5 mA at zero signal rising to 7 mA (TS); EL34 10-14.9 mA class A, 2 x 25 mA class B at full signal (Philips); EL84 5.5 mA rising to 10 mA (Philips); 6550 12 mA rising to 22 mA class A, 9 rising to 38 mA AB1 (Tung-Sol); KT88 about 3 mA rising to 15 mA (Genalex, fixed bias, 100 W) |
| Screen voltage | fixed supply per table (value per amp not in the docs read) | Screen limits: EL34 425 V (EH, 6CA7) / 500 V (TUBES.md); 6L6 450 V; 6V6 285 V (TS) / 350 V (EH); EL84 300 V; KT88 600 V; 6550 440 V |
| Screen dissipation | not modelled | EL34 7.5 W (EH) / 8 W (6CA7); 6L6 5 W; 6V6 2 / 2.5 W; EL84 2.2 W; KT88 6.6 W (EH) / 6-8 W (Genalex); 6550 6.0-6.6 W |

Screen current matters for sag. At full drive the sheets show screen current at 2-4x its zero-signal value, none of which the model sees.

### 1.6 Supply voltages in `PowerSection.h` against tube ratings

| Case | Amp / tube | Model B+ | Datasheet limit | Issue |
|---|---|---|---|---|
| 28 | Dual Thirty (EL84) | 350 V | 300 V plate and screen (EH; 500 V only at zero current) | 17 % over the continuous rating. |
| 33 | Brit Eighteen (EL84) | 330 V | 300 V | 10 % over. |
| 9 | Tweed 5E3 (6V6, 5Y3) | 335 V | 315 V plate / 285 V screen (Tung-Sol); 450 / 350 V (EH) | 6 % over Tung-Sol plate, 17 % over Tung-Sol screen; within the EH 6V6. |
| 30 | Cedar Spring (6V6) | 420 V | 315 V (Tung-Sol); 450 V (EH) | 33 % over Tung-Sol; within EH (93 % of limit). |
| 3 | SLO-100 (6L6GC) | 495 V | 500 V plate (EH), screen 450 V | 99 % of the plate limit; screen over if tied to B+. |
| 0 | American combo (6L6) | 480 V | 500 V, screen 450 V | Screen over if tied to B+. |
| 19 | Power Brawl (6L6GC) | 450 V | 500 V, screen 450 V | At the screen limit. |
| 6, 16, 20, 26 | EL34 amps | 470-480 V | plate 800 V; screen 425 V (EH, 6CA7) | Screen over by 11-13 % if tied to B+ (500 V on the Svetlana figure in TUBES.md would pass). |
| 18 | Wreck Stack (6550) | 545 V | 660 V (Tung-Sol); 800 V (EH); screen 440 V | Plate OK (83 % of 660). Screen over if tied to B+. |
| 17 | Diesel Fuel (4x KT77) | 470 V | no KT77 sheet | Cannot check. |

These comparisons assume the screen is near B+ in each amp; the docs do not say so. The plate comparisons are the firmer ones.

### 1.7 Rectifier source resistance (`DATASHEET_AUDIT_b58.md` and `PowerSection.h`)

| Case | Model Rs | Datasheet |
|---|---|---|
| 4 (100 W dual-rectifier) | 80 ohm | 5U4GB EH: 21 ohm per plate at 300 V RMS, 67 ohm at 450 V RMS (both 40 uF). The audit quotes 21-97 ohm for the RCA 5U4-GB. The model Rs is a total supply impedance, so it is not the same quantity; at 450 V RMS the plate resistance alone is 84 % of it. |
| 28 (Dual Thirty) | 140 ohm | 6CA4 EH (EZ81 type): 150 / 200 / 240 ohm per plate at 250 / 300 / 350 V RMS, 50 uF, 150 mA. Model Rs is below the tube's own plate resistance, before the transformer. |
| 33 (Brit Eighteen) | 120 ohm | Same 6CA4 numbers: model is 20 % below the lowest value (150 ohm). |
| 9 (tweed, 5Y3) | 250 ohm | no 5Y3 sheet |
| silicon cases | 60-85 ohm | not tube data |

Note that the tube-rectifier voltage drop is also not modelled: the 5U4GB drops 44-54 V at 225-300 mA and the 6CA4 drops 20 V at 150 mA, and the model uses Rs only. Whether that matches depends on the rectifier fitted in each amp, which the docs do not specify.

### 1.8 Dissipation, voltage and gm figures printed in `TUBES.md`

| Tube | TUBES.md says | Datasheets say | Verdict |
|---|---|---|---|
| EL34 | 25 W; plate 800 V; screen 500 V / 8 W; gm 11,000 uS | EH: 25 W; 800 V; screen 425 V / 7.5 W; gm 11 mA/V. 6CA7 EH: screen 425 V / 8 W. Philips: gm 11-12.5 mA/V. | Screen 500 V is 18 % above EH; screen W agrees with the 6CA7 sheet but not the EL34EH sheet. |
| 6L6GC | 30 W; plate and screen 500 V; screen 5 W; gm 5,200 uS; Rp 33k | EH: 30 W; plate 500 V; screen 450 V; 5 W; gm 5.2; rp 33k | Screen 500 V vs 450 V. Rest agree. |
| 6550C | 35 W; 680 V; screen 400 V / 6 W; 175 mA; gm 9,500 uS | No 6550C sheet supplied. EH 6550: 42 W; 800 V (tetrode); screen 440 V / 6.6 W; 230 mA; gm 11.2 mA/V | Not comparable one to one, but every figure differs. |
| 6550A | 42 W; 660 V; 190 mA; g2 6 W | Tung-Sol 6550: 42 W; 660 V; 190 mA; 6.0 W continuous | Agrees exactly. |
| KT88 | 33 W | EH tetrode 42 W (triode 44 W); Genalex 42 W absolute, 35 W design maximum, Pa+g2 46 W | 33 W matches no sheet; the nearest is the 35 W design max. |
| KT77 | 32 W | no sheet | Unverifiable. |
| KT66 | 30 W; cathode 200 mA | no sheet | Unverifiable. |
| EL84 | 12 W; plate and screen 300 V; 65 mA; gm 11,300 uS; Rp 38-40k | EH: 14 W; 300 V; 65 mA; gm 11.3 nominal; Philips ri 38-40k; screen 2.2 W | Only the dissipation differs (12 W vs 14 W). |
| 6V6 | "not extracted" | Tung-Sol 12 W, plate 315 V, screen 285 V, g2 2 W; EH 14 W, plate 450 V, screen 350 V, g2 2.5 W | Now available; the two sheets differ a lot from each other (about 40 % in plate voltage). |
| EL37, 7189A, 7408 | listed, "not extracted" | no sheets | Unverifiable. |

Doc inconsistency: `TUBES.md` says the Power Tube menu has five entries and that 6550 is "Not added". The actual menu in `PluginProcessor.cpp` has six: As voiced, EL34, 6L6, 6V6, EL84, KT88, and 6550. The 6550 entry reuses the KT88 curve (`pt = pwrSel - 1`, clamped to 4).

---

## 2. Tubes the datasheets cover that the model lacks

| Tube | Datasheet | Model status |
|---|---|---|
| 12AU7 | EH, with curves | No Koren set, no stage curve |
| 12BH7 | EH, no curves | none |
| 12DW7 | EH, no curves | none |
| 6922 / 6DJ8 | EH, no curves | none |
| 6AQ8 / ECC85 | EH, no curves | none |
| 6CG7 / 6FQ7 | EH, no curves | none |
| 6H30Pi | EH, curves printed (not digitised) | none |
| 6BM8 / ECL82 | EH, no curves | none |
| 2A3 | EH, curves printed (not digitised) | none |
| 300B | EH, curves printed (not digitised) | none |
| 6973 | EH, no curves | none |
| 6CA7 (EL34 class) | EH, no curves | falls under the EL34 model, not separate |
| KT90 | EH, no curves | none |
| 6550 (EH and Tung-Sol) | EH with curves; Tung-Sol without | Menu entry exists but uses the KT88 curve; no 6550 Koren set |
| 5U4GB | EH rectifier | No rectifier-tube model, only a series Rs per amp |
| 6CA4 | EH rectifier | No rectifier-tube model, only a series Rs per amp |

Tubes the model has that no sheet here covers:
- 12AY7, with a Koren set and a preamp curve.
- KT66, KT77 and EL37 are used or named in the docs and PowerSection (Diesel Fuel uses KT77) but have neither a sheet nor a Koren set.

---

## 3. What I could not settle

- The screen voltage behind the model's power-curve headers is not documented. All "model" power-tube values in 1.2-1.3 use the datasheet's own screen voltage (250 V for the curve charts). The 6L6EH and 6550EH charts print no screen voltage, so I assumed 250 V.
- The 12AT7 sheet has no curves and gives only limits, so only the Ia window and the gm minimum can be tested.
- The EL34EH curves are a degraded scan (+/-10-20 mA, and the 150-mA-level labels disagree with the dashes), so an EL34 curve comparison of 10 % is inside the read error.
- The idle-current figures in 1.4 depend on the unknown generator supply.
- Whether each amp's screen or rectifier matches the assumptions in 1.6 and 1.7 is not stated in the docs read.
