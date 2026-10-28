# Tube datasheets as read (Electro-Harmonix, Tung-Sol, Philips, Genalex/M-O Valve, vintage)

Source: 28 datasheet scans in `scratchpad/stg/tubes/` (mostly image-only PDFs, 100 dpi). Printed values were read by zooming the pages; curve values were read off the plots (calibrated to the printed gridlines), so they carry roughly +/-3 to 5 percent on the steep parts and more where noted.

Conventions:
- `(?)` means not legible, or read with real doubt.
- `~` means read from a curve, so approximate.
- "as printed" means the sheet itself may contain a typo, and the value is copied unchanged.
- Units: V, mA, W, mA/V (gm), pF (capacitance), ohm.
- Eg = grid 1 voltage. Vg2 = screen voltage.
- "ra" and "rp" are both plate resistance.
- Where a sheet prints no curves, that is stated explicitly. No curve points were invented.

Curve points were read for 12AX7, 12AU7, 6V6 (EH and Tung-Sol), 6L6, EL34 (EH), KT88 (EH) and 6550 (EH).
12AT7, EL84 (both sheets), the Philips EL34, the Genalex KT88 and the Tung-Sol 6550 print no curves.

---

## Part 1. Preamp and small-signal triodes

### 12AX7 (EH 12AX7EH) - high-mu double triode

| Item | Value |
|---|---|
| Type | Double triode, mu about 92 (min 78) |
| Heater | 6.0-6.6 V or 12.0-13.2 V (series/parallel) |
| Max plate V | 300 V |
| Plate dissipation | 1.0 W per triode |
| Cathode current | 9 mA max |
| Max Vhk | +100 / -200 V |
| Max negative grid | 55 V |
| Grid resistor | 1 M fixed, 2.2 M self-bias |
| Cold (cut-off) voltage | 600 V |
| Typical point | 250 V, -2 V (1 k grid R, 12.6 V heater) |
| Ia at typical point | 1.2 mA (spec min 0.75; Ia at -4 V <= 10 uA) |
| gm | 1.7 mA/V nominal (min 1.4) |
| mu | 92 nominal (min 78) |
| rp | 54.1k nominal |
| Cgp | 1.6 pF |
| Cg-(k+h) | 1.6 pF |
| Cp-(k+h) | 0.44 (section 1) / 0.36 (section 2) pF |
| Ck-h | 5.0 nF as printed (likely pF; (?)) |
| Cpp | 520 pF as printed (probably 0.52 pF; (?)) |

Plate characteristics, Ia in mA (read from the printed chart; plate axis 0-500 V, 0-6 mA; Pd max 1 W):

| Eg \ Vp | 50 | 100 | 150 | 200 | 250 | 300 | 350 | 400 | 450 |
|---|---|---|---|---|---|---|---|---|---|
| 0 V | 1.14 | 2.1 | 3.2 | ~4.5 | - | - | - | - | - |
| -0.5 V | - | 1.16 | 2.12 | 3.26 | 4.52 | - | - | - | - |
| -1 V | - | ~0.5 | 1.24 | 2.2 | 3.36 | 4.64 | - | - | - |
| -1.5 V | - | - | ~0.5 (?) | 1.17 | 2.17 | 3.31 | - | - | - |
| -2 V | - | - | - | - | 1.19 | 2.18 | 3.34 | - | - |
| -2.5 V | - | - | - | - | 0.55 | 1.26 | 2.23 | ~3.2 | - |
| -3 V | - | - | - | - | 0.12 | - | 1.29 | 2.2 | - |
| -3.5 V | - | - | - | - | - | - | 0.61 | 1.27 | 2.09 |
| -4 V | - | - | - | - | - | - | ~0.23 | ~0.65 | 1.2 |
| -4.5 V | - | - | - | - | - | - | - | ~0.4 | ~0.65 |
| -5 V | - | - | - | - | - | - | - | ~0.2 | ~0.33 |

Curve ends: Eg 0 stops at about 240 V / 5.5 mA (Pd line); -2 V stops at about 384 V / 3.9 mA.

Consistency checks:
- The 250 V / -2 V point reads 1.19 mA, matching the printed 1.2 mA.
- The family obeys Ia = f(Vp + 92*Vg) well, which confirms mu of about 92.
- gm from the curves at 250 V is about 1.6 mA/V, matching the printed 1.7.

### 12AT7 (EH 12AT7EH) - medium-mu double triode

| Item | Value |
|---|---|
| Type | Double triode (ECC81 type) |
| Heater | 6.3 V / 330 mA min, 360 max; or 12.6 V / 165-180 mA |
| Limits | heater 6.0-6.6 or 12.0-13.2 V |
| Max plate V | 330 V |
| Plate dissipation | 3.25 W per triode |
| Cathode current | 18 mA |
| Max Vhk | +200 / -200 V |
| Max negative grid | 55 V |
| Grid resistor | 0.25 M fixed, 1.0 M self |
| Test point | 250 V, -2 V |
| Ia at test point | 7.0-13.0 mA |
| gm | min 4.5 mA/V (3.5 in service) |
| mu | min 50 (no nominal printed) |
| rp | not printed |
| Grid reverse current | 0.5 uA (0.7 in service) |
| Ia at curve start | <= 10 uA (at -12 V) |
| Capacitances | input 2.5, output 0.4, transfer 1.6 pF |

Curves: NONE printed. The sheet gives limits only, so no plate-characteristic points can be listed.

### 12AU7 (EH 12AU7EH) - low-mu double triode

| Item | Value |
|---|---|
| Type | Double triode, mu about 17 |
| Heater | 12.6 V / 0.15 A or 6.3 V / 0.30 A (range 6.0-6.6 or 12.0-13.2 V); warm-up 12 s |
| Max plate V | 330 V |
| Plate dissipation | 2.75 W per triode |
| Cathode current | 22 mA |
| Max Vhk | +100 / -200 V |
| Grid resistor | 1 M fixed, 2.2 M self |
| Typical point | 250 V, -8.5 V |
| Ia at typical point | min 8 mA (curves give about 10.4 mA) |
| Ia at -18 V | 10 uA |
| gm | min 2.2 mA/V |
| mu | min 15, nominal 17 |
| rp | 7.5k nominal |
| Cgp | 1.8 pF |
| Cg-(k+h) | 2.0 pF |
| Cp-(k+h) | 1.5 pF |
| Ck-h | 5.0 nF as printed (likely pF; (?)) |
| Cpp | 500 pF as printed (likely 0.5 pF; (?)) |

Plate characteristics, Ia in mA (axes 0-500 V, 0-35 mA; Pd max 2.75 W):

| Eg \ Vp | 100 | 125 | 150 | 175 | 200 | 225 | 250 | 275 | 300 |
|---|---|---|---|---|---|---|---|---|---|
| 0 V | 11.4 | 15.4 | 19.8 | 24.5 | 30.2 | - | - | - | - |
| -2 V | 5.9 | 9.3 | 13.0 | ~17.4 | ~21.8 | ~26.5 | - | - | - |
| -4 V | 2.4 | 4.5 | 7.6 | 10.9 | ~14.6 | ~19.0 | - | - | - |
| -6 V | - | 1.85 | 3.8 | 6.4 | 9.6 | 13.0 | 16.5 | - | - |
| -8 V | - | - | 1.9 | 3.4 | 5.4 | 8.2 | 11.3 | 14.8 | - |
| -10 V | - | - | - | 1.3 | 2.8 | 5.0 | 7.0 | 9.7 | 13.5 |
| -12 V | - | - | - | - | 1.1 | 2.6 | 4.2 | 6.4 | 9.1 |
| -14 V | - | - | - | - | - | - | 2.3 | 3.9 | 6.0 |

Lower-curve points (Eg -14 to -22 V) at higher plate voltage, all read with doubt:
- -14 V: 350 V, ~11.6 (?)
- -16 V: 300 V ~3.7; 350 V ~8.0 (?)
- -18 V: 300 V ~2.3; 350 V ~5.4 (?)
- -20 V: 350 V ~3.5; 400 V ~6.8 (?)
- -22 V: 350 V ~1.8; 400 V ~4.8 (?)

Curves for Eg -24 to -30 V are labelled but their labels fall outside the plot area on the right, so those values are not readable (?).

### 12BH7 (EH 12BH7EH) - low-mu power-ish double triode

| Item | Value |
|---|---|
| Type | Double triode, mu about 16.5 |
| Heater | 12.6 V / 300 mA or 6.3 V / 600 mA; warm-up 11 s |
| Max Vhk | -200 V; +100 V DC / 200 V peak |
| Max plate V | 300 V (class A1) |
| Plate dissipation | 3.5 W per plate |
| Cathode current | 20 mA |
| Grid resistor | 0.25 M fixed, 1.0 M cathode bias |
| Typical point | 250 V, -10.5 V |
| Ia at typical point | 11.5 mA |
| gm | 3100 umho |
| mu | 16.5 |
| rp | 5300 ohm |
| Eg for Ib 50 uA | -23 V |
| Cgp | 2.6 pF (both sections) |
| Input | 3.2 pF |
| Output | 0.5 / 0.4 pF |
| Cp-p | 0.8 pF |

Curves: NONE printed.

### 12DW7 (EH 12DW7EH) - dissimilar double triode (12AX7 half plus 12AU7 half)

| Item | Value |
|---|---|
| Heater | 12.6 V / 0.15 A (series) or 6.3 V / 0.30 A (parallel) |
| Max plate V | 330 V (both sections) |
| Plate dissipation | 1.2 W (section 1) / 3.3 W (section 2) |
| Cathode current | 22 mA (section 2) |
| Max positive grid | 0 V |
| Max negative grid | 55 V |
| Grid resistor | 0.25 M fixed, 1.0 M self |
| Max Vhk | -200 V; +100 V DC / 200 V peak |

Typical operation:

| Point | Ia | gm | mu | rp |
|---|---|---|---|---|
| S1 at 100 V, -1 V | 0.5 mA | 1250 | 100 | 80,000 |
| S1 at 250 V, -2 V | 1.2 mA | 1600 | 100 | 62,500 |
| S2 at 100 V, 0 V | 11.8 mA | 3100 | 20 | 6500 |
| S2 at 250 V, -8.5 V | 10.5 mA | 2200 | 17 | 7700 |

For S2 the grid-1 voltage for Ib 10 uA is -24 V.

Capacitances (pF):

| | S1 shield / no shield | S2 shield / no shield |
|---|---|---|
| Cgp | 1.7 / 1.7 | 1.5 / 1.5 |
| Input | 1.8 / 1.6 | 1.8 / 1.7 |
| Output | 2.0 / 0.44 | 2.4 / 0.4 |

Curves: NONE printed.

### 6922 / 6DJ8 / ECC88 (EH 6922EH) - low-noise high-gm double triode

| Item | Value |
|---|---|
| Heater | 6.3 V, 285-335 mA |
| Max plate V | 300 V |
| Plate dissipation | 2.0 W each |
| Cathode current | 20 mA |
| Cold plate voltage | 1000 V |
| Max Vhk | +/-200 V |
| Grid resistor | 1 M (auto bias) |
| Test point | 100 V, +9 V via Rk 680 ohm |
| Ia at test point | 10-20 mA |
| gm | 10-16 mA/V (>= 7.5 in service) |
| mu | 24-40 |
| Balance | +/-20 % |
| Ia at curve start | <= 100 uA (90 V, -8 V) |
| Grid reverse current | <= 2.0 uA in service |
| Capacitances | input 32.5, output 2.0, transfer 1.5, plate-to-plate 0.09 pF |

Curves: NONE printed.

### 6AQ8 / ECC85 (EH 6AQ8EH)

| Item | Value |
|---|---|
| Heater | 6.3 V / 435 mA (9AQ8 variant: 9.0 V / 300 mA) |
| Max Vhk | 90 V |
| Max plate V | 300 V |
| Plate dissipation | 2.5 W |
| Cathode current | 15 mA |
| Max negative grid | 100 V |
| Grid resistor | 1.0 M |
| R-F amp, column 1 | 250 V, -2.3 V, 10 mA, gm 5900, mu 57 |
| R-F amp, column 2 | Eb 250 via Rp 1800 (Vp 230), -2.0 V (Rk 200), 10 mA, gm 6000, rp 9700, Rin at 100 MHz 6000, ENR 500 |
| Autodyne mixer | 250 V via 12k, grid leak 1 M, osc 3 V, 5.2 mA, conversion gm 2300, rp 22,000, Rin 15,000 |
| Capacitances (per section) | Cgp 1.5, input 3.0, output 1.2 pF |

Curves: NONE printed.

### 6CG7 / 6FQ7 (EH 6CG7EH)

| Item | Value |
|---|---|
| Heater | 6.3 V / 600 mA (also 8CG7 8.4 V / 450 mA and 12FQ7 12.6 V / 300 mA); warm-up 11 s |
| Max Vhk | -200 V; +100 V DC / 200 V peak |
| Max plate V | 330 V |
| Plate dissipation | 4 W each, 5.7 W both |
| Cathode current | 22 mA average |
| Grid resistor | 1.0 M fixed bias |
| Typical point 1 | 90 V, 0 V, 10 mA, gm 3000, mu 20, rp 6700, Eg(10 uA) -7 V |
| Typical point 2 | 250 V, -8 V, 9 mA, gm 2600, mu 20, rp 7700, Eg(10 uA) -18 V (row alignment slightly uncertain (?)) |
| Ia at -12.5 V | 1.3 mA |
| Capacitances | Cgp 3.6 (S1) / 3.8 (S2); input 2.4; output 0.34 / 0.26; Cp1-p2 1.0 pF |

Curves: NONE printed.

### 6H30Pi (EH 6H30Pi Gold) - low-rp high-current double triode

| Item | Value |
|---|---|
| Heater | 6.0-6.6 V, 850 mA |
| Max plate V | 250 V |
| Plate dissipation | 4.0 W per triode |
| Cathode current | 100 mA peak / 50 mA continuous |
| Max Vhk | +100 / -200 V |
| Grid resistor | 0.1 M fixed, 0.6 M self |
| mu | 15 |
| gm | 18 mA/V |
| rp | 840 ohm |
| Cgp | 6 pF |
| Cg-(k+h) | 6.3 pF |
| Cp-(k+h) | 2.4 (S1) / 0.38 (S2) pF |

Curves: printed (Ep 0-300 V, Ip 0-80 mA, Ec 0 to -16 V in 1 V steps) but NOT digitised in this pass (?).

### 6BM8 / ECL82 (EH 6BM8EH) - triode plus power pentode

| Item | Value |
|---|---|
| Heater | 6.3 V / 780 mA |
| Max Vhk | 100 V |
| Triode supply V | 550 V (plate 300 V) |
| Triode cathode current | 15 mA |
| Triode plate dissipation | 1 W |
| Pentode supply V | 900 V (plate 600 V) |
| Pentode g2 | supply 550 V, 300 V |
| Pentode cathode current | 50 mA |
| Pentode plate dissipation | 7 W |
| Pentode g2 input | 1.8 W |
| Grid resistor | 1 M fixed, 2 M cathode bias (both sections) |
| Triode typical | 100 V, 0 V; mu 70, gm 2500, Ia 3.5 mA |
| Pentode typical | 200 / 200 V, Vg1 -16 V; mu(g1-g2) 9.5, rp 20,000, gm 6400, Ia 35 mA, Ig2 7 mA |
| Triode capacitances | Cg-all 2.7, Cp-all 4.0, Cpg 4.0 (as printed (?)), Cg-h 0.1 pF |
| Pentode capacitances | Cg1-all 9.3, Cp-all 8, Cg1p 0.3, Cg1-h 0.3 pF |
| Coupling capacitances | Tp-Pg1 0.02, Tg-Pp 0.02, Tg-Pg1 0.025, Tp-Pp 0.25 pF |

Curves: NONE printed.

---

## Part 2. Power tubes

### 6V6 - Electro-Harmonix 6V6EH (beam power tetrode)

| Item | Value |
|---|---|
| Heater | 6.0-6.5 V / 0.45 A |
| Max plate V | 450 V |
| Max screen V | 350 V |
| Plate dissipation | 14 W |
| g2 dissipation | 2.5 W |
| Cathode current | 80 mA |
| Max Vhk | +100 / -200 V |
| Grid resistor | 100k fixed, 0.5 M self |
| gm | 4.1 mA/V |
| rp | 50k |
| Capacitances | Cgp 0.3, input 10, output 7.2 pF |

No operating-point table is printed; only the curves below.

Plate curves at Eg2 = 250 V, Eg3 = 0 (Pd max 14 W); Ia in mA:

| Eg \ Vp | 110 | 210 | 310 | 410 |
|---|---|---|---|---|
| 0 V | 110.7 | 114.7 | 118.4 | ~120 |
| -5 V | 78.5 | ~83 | 85.3 | 87.1 |
| -10 V | ~51 | 55.8 | 58.3 | ~59.5 |
| -15 V | 28.8 | 35.0 | 36.2 | 36.8 |
| -20 V | 12.9 | 18.4 | ~19.6 | ~19.9 |
| -25 V | ~5 | ~9-10 | ~9.8 | ~9.2 (?) |

### 6V6 - Tung-Sol 6V6 / 6V6GT / G (vintage, 1941)

| Item | Value |
|---|---|
| Heater | 6.3 V / 0.45 A |
| Max plate V | 315 V |
| Max screen V | 285 V |
| Plate dissipation | 12 W |
| g2 dissipation | 2 W |

Single tube, class A1:

| | 180 V | 250 V | 315 V |
|---|---|---|---|
| Vg2 | 180 | 250 | 225 |
| Vg1 | -8.5 | -12.5 | -13 |
| Signal peak | 8.5 | 12.5 | 13 |
| Ia (zero signal) | 29 mA | 45 mA | 34 mA |
| Ig2 (zero signal) | 3 mA | 4.5 mA | 2.2 mA |
| Ia (max signal) | 30 mA | 47 mA | 35 mA |
| Ig2 (max signal) | 4 mA | 7 mA | 6 mA |
| rp | 58k | 52k | 77k |
| gm | 3700 | 4100 | 3750 |
| RL | 5500 | 5000 | 8500 |
| THD | 8 % | 8 % | 12 % |
| Po | 2.0 W | 4.5 W | 5.5 W |

Push-pull AB1, two tubes:

| | 250 V | 285 V |
|---|---|---|
| Vg2 | 250 | 285 |
| Vg1 | -15 | -19 |
| Signal g-g | 30 | 38 |
| Ia0 (total) | 70 mA | 70 mA |
| Ig2(0) | 5.0 | 4.0 |
| Ia max | 79 | 92 |
| Ig2 max | 13 | 13.5 |
| rp | 66k | 65k |
| gm | 3750 | 3600 |
| RL plate-to-plate | 10k | 8k |
| THD | ~5 % (?) | 3.5 % |
| Po | 10 W | 14 W |

Plate curves at Eg2 = 250 V; Ia in mA:

| Eg \ Vp | 100 | 200 | 300 | 400 |
|---|---|---|---|---|
| 0 V | 94.6 | 103.6 | 107.5 | 110.7 |
| -5 V | ~70 | 77.6 | 80.6 | 83.3 |
| -10 V | 45.3 | 54.2 | 57.0 | 58.8 |
| -12.5 V | - | 44.4 | 45.5 | - |
| -15 V | 27.6 | 34.0 | 36.0 | 38.4 |
| -20 V | 14.4 | 18.4 | 20.3 | 21.1 |
| -25 V | ~4.5 (at 110 V) | 6.8 | 8.7 | ~9.5 |
| -30 V | ~0.6 (at 110 V) | 1.8 | 2.4 | ~3.0 |

Page 3 also has load-line and distortion graphs (Eb 250, Eg -12.5, RL 5000), not digitised. The 250 V / -12.5 V curve reads about 45 mA, matching the table.

### 6L6 - Electro-Harmonix 6L6EH (beam power tetrode)

| Item | Value |
|---|---|
| Heater | 5.7-6.9 V / 0.9 A |
| Max plate V | 500 V |
| Max screen V | 450 V |
| Plate dissipation | 30 W |
| g2 dissipation | 5 W |
| Cathode current | 80 mA |
| Max Vhk | +/-200 V |
| Grid resistor | 100k fixed, 0.5 M self |
| gm | 5.2 mA/V |
| rp | 33k |
| Capacitances | Cgp 0.6, input 10, output 6.4 pF |

No operating-point table is printed. The curve chart does NOT state its screen voltage (?).

Plate curves (axes 0-550 V, 0-220 mA; calibrated to the printed gridlines); Ia in mA:

| Eg \ Vp | 105 | 205 | 255 | 305 | 405 | End of curve |
|---|---|---|---|---|---|---|
| 0 V | 175.4 | 186.2 | - | - | - | ~235 V, ~190 mA |
| -5 V | 128.5 | 135.2 | 139.3 | - | - | ~285 V |
| -10 V | ~90 (?) | 95.3 | - | ~99 | - | ~300 V |
| -15 V | 58.6 | 65.6 | - | 71.1 | - | ~420 V |
| -20 V | 37.0 | 43.4 | - | ~45 | 45.2 | ~434 V |
| -25 V | ~19.8 (?) | ~24.8 (?) | - | 26.5 | 27.1 | ~476 V |
| -30 V | 8.7 | 12.2 | - | 14.3 | ~14.3 | ~513 V |

The gm between Eg -10 and -20 V is about 5-6 mA/V, consistent with the printed 5.2.

### EL34 - Electro-Harmonix EL34EH (power pentode)

| Item | Value |
|---|---|
| Heater | 6.0-6.5 V / 1.5 A |
| Max plate V | 800 V |
| Max screen V | 425 V |
| Plate dissipation | 25 W |
| g2 dissipation | 7.5 W |
| Cathode current | 150 mA |
| Max Vhk | +/-200 V |
| Grid resistor | 100k fixed, 0.5 M self |
| gm | 11 mA/V |
| rp | 15k |
| Capacitances | Cgp 1.2, input 16, output 7.2 pF |

No operating-point table is printed.

Plate curves, upper chart: Eg2 = 250 V, Eg3 = 0, Pd 25 W, axes 0-300 mA, 0-500 V. The scan is degraded (thin dotted curves), so these values are good to about +/-10-20 mA. Ia in mA:

| Eg \ Vp | 100 | 150 | 200 | 250 | 300 | 350 | 450 |
|---|---|---|---|---|---|---|---|
| 0 V | ~252 | ~260 | ~270 | ~276 | ~280 | - | - |
| -5 V | - | ~185 | ~194 | ~200 | ~200 | - | - |
| -10 V | - | - | ~130 | ~127 | ~129 | ~132 | - |
| -15 V | - | - | 64.6 | 66 | 68.6 | 72.7 | 75.4 |
| -20 V | - | - | - | - | - | ~29.6 | ~38-42 |
| -25 V | - | - | - | - | - | ~8 | ~16 |

Plateau levels read from the on-chart labels were higher (-10 V: ~150; -15 V: ~89; -20 V: ~41; -25 V: ~18). The -10 and -15 V values above come from visible dashes. The disagreement is real, so treat those two rows as +/-15 mA (?).

Lower chart (Eg2 = 360 V, 0-500 mA, 0-800 V, Pd 25 W): curves are NOT legible. Labels read Eg = 0, -4, -8, -12, -16, -20, -24, -28, and a last one "-32" (?). No values read.

### EL34 - Philips (Jan 1969; pages 1-3 only; no curves, no limit-ratings page)

| Item | Value |
|---|---|
| Quick reference | Ia 100 mA, S 12.5 mA/V, mu(g2-g1) 11, Po class B 100 W |
| Heater | 6.3 V / 1.5 A |
| Capacitances | Ca(g1) 8.4; Cg1(a) 15.2; Cag1 max 1.1; Cg1f max 1.0; Ckf 10 pF |

Class A, 250 V (Vb 265 V):

| Rg2 | Vg1 | Ia | Ig2 | S | mu | Ri | Ra | Vi | Po | THD |
|---|---|---|---|---|---|---|---|---|---|---|
| 2k | -14.5 | 70 mA | 10 | 11 | 11 | 20k | 3.0k | 9.3 V | 8 W | 10 % |
| 0 | -13.5 | 100 mA | 14.9 | 12.5 | 11 | 17k | 2.0k | 8.7 V | 11 W | 10 % |

Class B push-pull, two tubes (Vb = supply, Va = plate at the stated condition; each Ia and Ig2 is per tube):

| Rg2 | Vg1 | Vb | Va | Ia | Ig2 | Raa | Vi | Po | THD |
|---|---|---|---|---|---|---|---|---|---|
| 1000 | -38 | 425 | 420 | 2x30 | 2x4.4 | - | 0 | 0 | - |
| 1000 | -38 | 425 | 400 | 2x120 | 2x25 | 3.4k | 27 | 55 W | 5 % |
| 1000 | -38 | 400 | 375 | 2x100 | 2x25 | 4.0k | 27 | 45 W | 6 % |
| 470 | -32 | 375 | 370 | 2x35 | 2x4.7 | - | 0 | 0 | - |
| 470 | -32 | 375 | 350 | 2x120 | 2x25 | 2.8k | 22.7 | 44 W | 5 % |
| 470 | -32 | 350 | 325 | 2x93 | 2x25 | 3.8k | 22.7 | 36 W | 6 % |
| 750 | -36 | 500 | 495 | 2x30 | 2x4 | - | 0 | 0 | - |
| 750 | -36 | 500 | 475 | 2x125 | 2x25 | 4k | 25.8 | 70 W | 5 % |
| 750 | -36 | 475 | 450 | 2x102 | 2x25 | 5k | 25.8 | 58 W | 6 % |
| 750 | -39 | 800 | 795 | 2x25 | 2x3 | - | 0 | 0 | - |
| 750 | -39 | 800 | 775 | 2x91 | 2x19 | 11k | 23.4 | 100 W | 5 % |
| 750 | -39 | 750 | 725 | 2x84 | 2x19 | 11k | 23.4 | 90 W | 6 % |

The screen-supply voltages for the 750-ohm groups were read as 400 / 400 / 375 V for both the 500 V and the 800 V groups (?).

### EL34 - 6CA7 (EH 6CA7EH)

| Item | Value |
|---|---|
| Heater | 6.3 V, 1.5 +/- 0.1 A (limits 5.7-7.0 V) |
| Max Vhk | +/-300 V |
| Test point | 250 / 250 V, -13.5 V |
| Ia at test point | 100 +/- 20 mA |
| Ig2 | <= 20 mA |
| gm | >= 10 mA/V |
| Output test | Po >= 9.0 W, THD <= 13.5 % at Ug1 alt. eff. 9.6 V, Ra 2.0k |
| Max plate V | 800 V |
| Max screen V | 425 V |
| Max negative g1 | 100 V |
| Cathode current | 150 mA |
| Plate dissipation | 25 W |
| g2 dissipation | 8 W |
| Grid resistor | 0.5 M fixed, 0.7 M auto |

Capacitances and curves: NONE printed.

### EL84 - Electro-Harmonix EL84EH (Russian-origin sheet; no curves)

| Item | Value |
|---|---|
| Heater | 6.3 V, 700-820 mA (limits 5.7-7.0 V) |
| Test point | 250 / 250 V, -7.3 V, Ra 5.2k, Rg 0.51 M |
| Ia at test point | 44-64 mA |
| Ig2 | <= 10 mA |
| Ig1 reverse | <= 1 uA (1.5 uA in operation) |
| gm | min 9.0, nominal 11.3 mA/V |
| Distortion ("unlinear") | <= 17 % |
| Po | >= 5.5 W (Vi 5.2 V); 5.0 W at 5.7 V heater; >= 4.0 W at normal temperature |
| Rhk | >= 5 Mohm |
| Capacitances | input 11, output 7.0, transfer 0.2 pF |
| Max plate V | 300 V (500 V at zero current) |
| Max screen V | 300 V (500 V at zero current) |
| Max negative g1 | 100 V |
| Max Vhk | +/-100 V |
| Rg1 | 1.0 M |
| Cathode current | 65 mA |
| Plate dissipation | 14 W |
| g2 dissipation | 2.2 W |
| Envelope | 573 K |

### EL84 - Philips (Dec 1969; pages 1-3; no max-ratings page, no curves)

| Item | Value |
|---|---|
| Quick reference | Ia 48 mA, S 11.3 mA/V, mu(g2-g1) 19, Po 6.0 W |
| Heater | 6.3 V / 760 mA |
| Capacitances | Ca(g1) 6.5; Cg1(a) 10.8; Cag1 max 0.5; Cg1f max 0.25 pF |

Class A operating points, fixed bias unless noted; Ig1 = +0.3 uA at maximum drive:

| Block | Vp / Vg2 | Vg1 | Rk | Ra | Ia0 | Ig2(0) | gm | ri | Drive (Vi) to Po and THD |
|---|---|---|---|---|---|---|---|---|---|
| 1 | 250 / 250 | -7.3 | 135 | 5.2k | 48 mA | 5.5 mA | 11.3 | 38k | 3.4 V to 4.5 W (6.8 % THD, d2 3.0 %, d3 5.8 %); 4.3 V to 5.7 W (10 %); 4.7 V to 6.0 W |
| 2 | 250 / 250 | -7.3 | 135 | 4.5k | 48 | 5.5 | 11.3 | 38k | 3.5 V to 4.5 W (7.5 %); 4.4 V to 5.7 W (10 %); 4.8 V to 6.0 W; Ia 50.6 mA and Ig2 10 mA at full drive |
| 3 | 250 / 250 | -8.4 | 210 | 7k | 36 | 4.1 | 10 | 40k | 3.5 V to 4.2 W at 10 % THD (d2 1.7 %, d3 8.7 %); Ig2 14.6 mA at 5.5 V drive to 5.6 W |
| 4 | 250 / 210 | -6.4 | 160 | 7k | 36 | 3.9 | 10.4 | 40k | 3.4 V to 4.3 W at 10 % (d2 1.8 %, d3 9.3 %); 3.8 V to 4.7 W |

### KT88 - Electro-Harmonix KT88EH (beam power tetrode; 3 pages)

| Item | Value |
|---|---|
| Heater | 6.3 V / 1.6 A (5.7-6.9 V) |
| Max Vhk | +300 / -200 V |
| Capacitances | input 18.5, output 12.5, transfer 1.1 pF |
| Typical single tube | Vp 400, Vg2 225, Vg1 -16.5, Ia 90 mA, Ig2 5 mA, gm 9.5 mA/V, load 3k, input amplitude 16.5 V, Po 20 W, THD 13.5 % |

Limiting values:

| | Triode | Tetrode |
|---|---|---|
| Plate V | 440 V | 800 V |
| Screen V | 440 V | 600 V |
| Negative g1 | 300 V | 300 V |
| Plate dissipation | 44 W | 42 W |
| g2 dissipation | 6.6 W | 6.6 W |
| Cathode current | 193 mA | 230 mA |
| Rg1 | 0.051 M fixed, 0.24 M auto | 0.051 M fixed, 0.24 M auto |
| Envelope | 250 C | 250 C |

Plate curves (page 2, printed rotated; Ug2 = 250 V, Pp max 42 W; axes 0-500 V and 0-300+ mA); Ia in mA, read about +/-5-10 mA:

| Ug1 \ Vp | 50 | 100 | 200 | 300 | 400 | 500 |
|---|---|---|---|---|---|---|
| 0 V | ~328 | ~343 | ~368 | - | - | - |
| -5 V | ~233 | ~254 | ~279 | - | - | - |
| -10 V | ~177 | ~182 | ~197 | ~208 | - | - |
| -15 V | ~113 | ~120 | ~134 | ~144 | ~150 | - |
| -20 V | ~54 | ~67 | ~79 | ~86 | ~92 | ~98 |
| -25 V | ~26 | ~26 | ~41 | ~46 | ~51 | ~56 |

Curve ends: Ug1 = 0 stops at about 200 V, ~370 mA; -5 V at about 250 V, ~290 mA; -10 V at about 360 V, ~214 mA. The -20 and -25 V curves show a small hump near 30-50 V (as drawn).

Page 3: transfer curves at Up 400 V, Ug2 225 V (Ia and Ig2 versus Ug1 from -40 to 0 V); Ig2 about 20 mA and Ia about 100-125 mA near -5 V. Not digitised (?).

### KT88 - Genalex / M-O Valve (Dec 1974, issue 5; no curves)

| Item | Value |
|---|---|
| Heater | 6.3 V / 1.6 A |

Maximum ratings:

| | Absolute | Design max |
|---|---|---|
| Va | 800 V | 800 V |
| Vg2 | 600 V | 600 V |
| Va,g2 | 600 V | 600 V |
| -Vg1 | 200 V | 200 V |
| Pa | 42 W | 35 W |
| Pg2 | 8 W | 6 W |
| Pa+g2 | 46 W | 40 W |
| Ik | 230 mA | 230 mA |
| Vh-k | 250 V | 200 V |
| Tbulb | 250 C | 250 C |

Grid resistor limits:
- Cathode bias: 470k at Pa+g2 <= 35 W; 270k above 35 W.
- Fixed bias: 220k at Pa+g2 <= 35 W; 100k above 35 W.

Capacitances (pF):

| | Cg1-a(,g2) | Cg1-all less a(,g2) | Ca(,g2)-all less g1 |
|---|---|---|---|
| Triode connected | 7.9 | 9.3 | 17 |
| Tetrode connected | 1.2 | 16 | 12 |

Characteristics at 250 V:

| Connection | Ia | Ig2 | -Vg1 | gm | ra | mu |
|---|---|---|---|---|---|---|
| Tetrode (Vg2 250) | 140 mA | ~3 mA | ~15 V | 11.5 mA/V | 12k | 8 (mu g1-g2) |
| Triode (Va,g2 250) | 143 mA (Ia+g2) | - | ~15 V | 12 mA/V | 670 ohm | 8 |

Push-pull class AB1, tetrode (per-tube currents, anode-to-anode load):

| | Cathode bias | Fixed bias |
|---|---|---|
| Va(b) | 560 V | 560 V |
| Va(o) | 521 V | 552 V |
| Vg2 | 300 V | 300 V |
| Ia(o) | 2x64 mA | 2x60 mA |
| Ia max | 2x73 mA | 2x145 mA |
| Ig2(o) | 2x1.7 mA | 2x1.7 mA |
| Ig2 max | 2x9 mA | 2x15 mA |
| RL(a-a) | 9k | 4.5k |
| Rk | 2x460 | - |
| -Vg1 | ~30 V | ~34 V |
| Pout | 50 W | 100 W |
| Dtot | 3 % | 2.5 % |
| IM | 11 % | 10 % |
| Pa(o) | 2x33 W | 2x33 W |
| Pa max signal | 2x12 W | 2x28 W |
| Pg2(o) | 2x0.5 W | 2x0.5 W |
| Pg2 max signal | 2x2.7 W | 2x4.5 W |
| Vg-g crest | 60 V | 67 V |

Push-pull class AB1 ultra-linear (40 % taps), cathode bias:

| | Column 1 | Column 2 |
|---|---|---|
| Va,g2(b) | 500 V | 375 V |
| Va,g2(o) | 436 V | 328 V |
| Ia+g2(o) | 2x87 mA | 2x87 mA |
| Max signal | 2x99 mA | 2x96 mA |
| RL(a-a) | 6k | 5k |
| Rk | 2x600 | 2x400 |
| -Vg1 | ~52 V | ~35 V |
| Pout | 50 W | 30 W |
| Dtot | 1.5 % | 1 % |
| IM | 4 % | 3 % |
| Pa+g2(o) | 2x38 W | 2x28.5 W |
| Max signal | 2x17 W | 2x16 W |
| Vg-g crest | 104 V | 71 V |
| Zout | 4.8k | 4.5k |

### KT90 (EH KT90EH)

| Item | Value |
|---|---|
| Type | Beam power tetrode, octal, 125 mm tall, 39 mm diameter |
| Heater | 6.3 V, 1.6 +/- 0.1 A (limits 6.0-6.6 V) |
| Test point | 250 / 250 V, -14 V, Rg1 0.5 M |
| Ia at test point | 140 +/- 25 mA |
| Ig2 | <= 14 mA |
| gm | >= 12 mA/V |
| Ig1 reverse | <= 0.5 uA |
| Grid-1 locking voltage | <= 42 V at Ip = 1 mA |
| Cathode-heater leakage | <= 30 uA at +/-300 V |
| Output test | Po >= 12.0 W and THD <= 14 % at Ra 2k, Ug1 eff. 9.9 V |
| Max plate V | 850 V |
| Max screen V | 650 V |
| Max negative g1 | 200 V |
| Cathode current | 230 mA |
| Plate dissipation | 46 W |
| g2 dissipation | 8 W |
| Max Vhk | +/-300 V |

Capacitances and curves: NONE printed.

### 6550 - Electro-Harmonix 6550EH (beam power tetrode)

| Item | Value |
|---|---|
| Heater | 6.0-6.5 V / 1.6 A |
| Max Vhk | +300 / -200 V |
| Max plate V | 800 V (tetrode), 440 V (triode / ultra-linear) |
| Cathode current | 230 mA (tetrode), 190 mA (triode) |
| Plate dissipation | 42 W tetrode, 44 W triode |
| Max screen V | 440 V |
| g2 dissipation | 6.6 W |
| Grid resistor | 50k fixed, 0.24 M self |
| Capacitances | Cgp 0.88, input 18, output 12.5 pF |
| gm | 11.2 mA/V |
| rp | 14.8k |
| Envelope | 250 C |

Plate curves (screen voltage NOT labelled (?); axes 0-600 V, 0-400 mA; Pd max 42 W); Ia in mA, read about +/-5-8 mA:

| Ec \ Ep | 50 | 100 | 150 | 200 | 300 | 400 | 500 |
|---|---|---|---|---|---|---|---|
| 0 V | ~335 (?) | ~360 | ~368 | - | - | - | - |
| -5 V | ~250 | ~260 | ~265 | ~272 | - | - | - |
| -10 V | ~172 | ~182 | - | ~190 | ~198 (?) | - | - |
| -14 V | ~113 | ~124 | - | ~136 | ~140 | ~143 | - |
| -20 V | ~55 | ~61 | - | ~72 | ~78 | ~84 | ~87 |
| -30 V | ~10 | ~12 | - | ~14 | ~17 | ~19 | ~27 (at 550 V) (?) |

Curve ends: Ec 0 stops at about 150 V; -5 V at about 200 V; -10 V at about 280 V; -14 V at about 400 V. The chart is on a very fine grid, so the low-current rows are the least certain.

### 6550 - Tung-Sol (July and March 1963; 3 pages; no curves)

| Item | Value |
|---|---|
| Heater | 6.3 V / 1.6 A |
| Max Vhk | 300 V negative, 100 V DC / 200 V peak positive |
| Capacitances | Cg1p 0.8, input 15, output 10 pF |
| Max plate V | 660 V |
| Max screen V | 440 V (pentode), 500 V (triode / UL) |
| Vg1 range | -300 to 0 V |
| Plate dissipation | 42 W |
| g2 dissipation | 6.0 W continuous, 10.0 W peaks |
| Cathode current | 190 mA |
| Rg1 | 50k fixed, 250k self |
| Bulb | 250 C |

Average pentode characteristics at 250 / 250 V, -14 V: Ia 140 mA, Ig2 12 mA, gm 11,000 umho, rp 15,000 ohm, triode mu 8, Vg1 for Ia = 1 mA is -40 V.

Single-tube class A1:

| Vp / Vg2 / Vg1 | Signal peak | Ia (zero / max) | Ig2 (zero / max) | RL | THD | Po |
|---|---|---|---|---|---|---|
| 250 / 250 / -14 | 14 V | 140 / 150 mA | 12 / 22 mA | 1500 | 7 % | 12.5 W |
| 400 / 225 / -16.5 | 16.5 V | 87 / 105 mA | 4 / 14 mA | 3000 | 13.5 % | 20 W |

Push-pull AB1, pentode, two tubes:

| Vp | Vg2 | Vg1 / Rk | Vg-g peak | Ia0 | Ia max | Ig2 0 | Ig2 max | Raa | THD | Po |
|---|---|---|---|---|---|---|---|---|---|---|
| 400 | 310 | Rk 140 (bypassed) | 43 | 170 | 185 | 10 | 25 | 5000 | 0.7 % | 40 W |
| 400 | 270 | -23 | 46 | 170 | 275 | 9 | 35 | 3500 | 0.6 % | 60 W |
| 450 | 310 | -29.5 | 58 | 150 | 295 | 9 | 38 | 3500 | 1.5 % | 77 W |
| 600 | 300 | -32.5 | 65 | 100 | 270 | 5 | 33 | 5000 | 3.0 % | 100 W |

Ultra-linear (grid 2 tapped at 40 % of primary turns):

| Mode | Vp | Vg1 or Rk | Vg-g | Ia (zero / max) | Ig2 (zero / max) | Raa | THD | Po |
|---|---|---|---|---|---|---|---|---|
| Self bias, class A1 | 395 | Rk 200 | 70 V | 170 / 174 mA | 12.5 / 23 mA | 5600 | 1.5 % | 34 W |
| Fixed bias, class AB1 | 450 | -48 V | 96 V | 150 / 265 mA | 12 / 38 mA | 4000 | 2.4 % | 70 W |

The sheet recommends a small resistor in each cathode for balance.

### 6973 (EH 6973EH) - miniature beam power pentode

| Item | Value |
|---|---|
| Heater | 6.3 V / 450 mA |
| Max Vhk | -200 V; +100 V DC / 200 V peak |
| Capacitances | Cg1p 0.4, input 9.0, output 6.0 pF |
| Bulb | 250 C |

Ratings:

| | UL | Pentode |
|---|---|---|
| Vp+g2 | 410 V | - |
| Vp | - | 440 V |
| Vg2 | - | 330 V |
| Pd | 12 W | 12 W |
| Pg2 | 1.75 W | 2 W |
| Rg1 fixed | 0.5 M | 0.5 M |
| Rg1 self | 1.0 M | 1.0 M |

Single-tube class A at 250 / 250 V, -15 V: Ia 46 mA, Ig2 3.5 mA, gm 4.8k, rp 73k, Vg1 for Ib 100 uA is -40 V.

Push-pull AB, fixed bias:

| Vp | Vg2 | Vg1 | Vsig | Ia0 | Ia max | Ig2 0 | Ig2 max | Raa | Po | THD |
|---|---|---|---|---|---|---|---|---|---|---|
| 250 | 250 | -15 | 15 | 92 | 105 | 7 | 16 | 8k | 12.5 W | 2 % |
| 350 | 280 | -22 | 22 | 58 | 106 | 3.5 | 14 | 7.5k | 20 W | 1.5 % |
| 400 | 290 | -25 | 25 | 50 | 107 | 2.5 | 13.7 | 8k | 24 W | 2 % |

Push-pull AB, self bias:

| Vp / Vg2 | Rk | Vsig | Ia0 | Ia max | Ig2 0 | Ig2 max | Raa | Po | THD |
|---|---|---|---|---|---|---|---|---|---|
| 300 | 230 | 24 | 80 | 96 | 6 | 14 | 5.5k | 15 W | 2 % |
| 310 | 270 | 22.5 | 77 | 92 | 5 | 14 | 6k | 17 W | 4 % |

Ultra-linear, class AB:
- 375 V (note 1: 50 % taps): -33.5 V, 33.5 V peak, Ik 62 / 95 mA, 12.5k, 18.5 W, 1.5 %.
- 370 V (note 2: 43 % taps (?)): Rk 355, 31 V, Ik 74 / 84 mA, 13k, 15 W, 1.2 %.

Curves: NONE printed.

### 2A3 (EH 2A3EH Gold) - directly heated power triode

| Item | Value |
|---|---|
| Heater | 2.5 V / 2.5 A (limits 2.25-2.75 V) |
| Max plate V | 450 V |
| Cathode current | 100 mA |
| Plate dissipation | 15 W |
| Max negative grid | -120 V |
| Rg fixed | 0.51 M |
| Envelope | 150 C |
| Typical | 250 V, bias -42 V (DC) / -45 V (AC), grid effective voltage 30 V, load 2.5k |
| Ia | 60 mA |
| gm | 5.5 mA/V |
| mu | 4.2 |
| Po | >= 3.0 W |
| Capacitances | printed as "7.5 / 5.5 / 16.5 uf max" (unit typo, probably pF; (?)) |

Curves: not digitised (?).

### 300B (EH 300B-EH) - directly heated power triode

| Item | Value |
|---|---|
| Heater | 5.0 V / 1.25 A |
| Max plate V | 450 V |
| Cathode current | 100 mA |
| Plate dissipation | 40 W |
| Rg | 50k fixed, 0.5 M self |
| Envelope | 250 C |
| gm | 5.5 mA/V |
| rp | 700 ohm |
| mu | 3.85 |
| Capacitances | Cgp 15, Cg-f 9, Cp-f 4.3 pF |

Curves: printed (Ec 0 to -120 V, Pd 40 W), not digitised (?).

---

## Part 3. Rectifiers

### 5U4GB (EH 5U4GBEH)

| Item | Value |
|---|---|
| Type | Full-wave vacuum rectifier |
| Filament | 5.0 V / 3000 mA |
| PIV | 1550 V |
| Max AC plate supply | 550 V RMS |
| Steady peak current | 1.0 A per plate |
| Transient peak | 4.6 A |

Capacitor-input operation, 40 uF; the left-column values are less certain:

| | Left | Right |
|---|---|---|
| AC volts per plate (RMS) | 300 V | 450 V |
| Plate-supply resistance per plate | 21 ohm | 67 ohm |
| DC output current | 300 mA | 275 mA |
| DC output voltage | 290 V (?) | 460 V |

Choke input: 550 V RMS, 10 H, 275 mA, 420 V DC out. Tube drop: 44 / 50 / 54 V at 225 / 275 / 300 mA per plate. A rating chart is printed but was not digitised.

### 6CA4 (EH 6CA4EH) - EZ81 type

| Item | Value |
|---|---|
| Heater | 6.3 V +/-10 %, 1.0 A |
| PIV | 1200 V |
| Steady peak current | 500 mA per plate |
| Transient peak | 1.85 A (0.2 s) |
| Max Vhk | 500 V (heater negative) |
| AC plate supply and DC output current | "see rating chart #1" (not digitised) |

Capacitor-input, 50 uF:

| AC volts per plate (RMS) | Plate-supply resistance per plate | DC current | DC output |
|---|---|---|---|
| 250 V | 150 ohm | 150 mA | 245 V |
| 300 V | 200 ohm | 150 mA | 293 V |
| 350 V | 240 ohm | 150 mA | 347 V |

Tube drop: 20 V at 150 mA.

---

## Coverage notes

- Curves read: 12AX7, 12AU7, 6V6 (EH and Tung-Sol), 6L6 (EH), EL34 (EH, upper chart only), KT88 (EH), 6550 (EH).
- No curves printed: 12AT7, EL84 (EH and Philips), EL34 (Philips), 6CA7, KT90, KT88 (Genalex), 6550 (Tung-Sol), 6973, 12BH7, 12DW7, 6922, 6AQ8, 6CG7, 6BM8, the rectifiers.
- Curves printed but not digitised: 6H30Pi, 300B, 2A3, EL34EH lower chart (illegible), and the KT88EH page-3 transfer curves.
- The `6CG7__1_` copy of the 6CG7 sheet is identical to `6cg7eh`.
- Screen voltage is not stated on the 6L6EH and 6550EH curve charts.
