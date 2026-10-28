# Matchless DC30 clone sheet: "P/I - Power Amp" (read from a hand-drawn page)

Source: `matchless_dc30-1.png` (2550x1651, 150 dpi, hand-drawn). The text extraction `matchless_dc30.txt` is empty (1 byte). The page is headed "MATCHLESS D/C 30, P/I - Power Amp".

## 0. What this page is, and is not (read this first)

This is **one sheet only: the phase inverter ("P/I") and the power amp.** It contains:
- inputs labelled CH1 and CH2 (the outputs of the two preamp channels, entering the phase inverter through caps);
- one 12AX7 long-tail phase inverter;
- four EL84s in push-pull parallel;
- the Cut control and a Master 1MA pot with a pull switch (both between the two inverter outputs);
- the Hi/Lo power switch with the shared cathode resistors and bypass cap;
- the output transformer ("O/T WTI 9356") with its impedance taps and a Phase/Reverse switch.

It does **not** contain: the preamp channels (the DC30 as drawn has CH1 and CH2, **not** Normal / Top Boost / Vibrato channels; those names belong to Vox-style amps and do not appear anywhere on this page), any preamp tube types or plate loads or cathode values, the tone stack, the vibrato or any tremolo, the rectifier, the power transformer, the choke, or the filter supply values. The B+ points are only shown as circled nodes 1, 2, 3 with no voltages. So the items asked for under "preamp tube types and plate loads and cathode values for each stage", "tone-stack values", "rectifier" and "vibrato channel" are **not on this sheet** and are marked "not on sheet" below. Only the phase inverter's own tube stage (the 12AX7 at the left) is a preamp-type stage on this page.

Reading notes: hollow arrow up with a circled number = B+ node (circled 1, 2, 3); hollow triangle (pointing down, or sideways when drawn sideways) = ground; the little loops on wires are wire hops (no connection). Values are handwritten; ones I am unsure of are marked **(?)**. Capacitor values are printed without units: ".01/400V", ".047 400V", ".0022" are in uF; "250/100V" is uF/V.

## 1. Inputs and the 12AX7 phase inverter (left part of the sheet)

Tube: **12AX7** (printed). Pins as drawn: top triode plate 6, grid 7, cathode 8; bottom triode plate 1, grid 2, cathode 3. (These are the correct 12AX7 pin numbers for the two sections.)

- **CH1** input: a **.01 uF / 400 V** cap to the **grid (pin 7)** of the top triode. **CH2** input: **.01 uF / 400 V** to the **grid (pin 2)** of the bottom triode. (So each channel's output feeds one grid of the pair; the other grid is not driven by that channel.)
- Grid leaks: **1 M** from each grid to a common node (two 1M resistors in series, node between them). Each grid is returned through its 1M to this node.
- The two **cathodes (pins 8 and 3) are tied together** and go through **R = 1K2** to the same node as the grid leaks; that node goes through **47K** to ground (hollow triangle at left). So the tail is 1K2 + 47K to ground: the cathodes sit about (tail current x 1.2k) above the grid-leak node, which sets the bias (self-biased long-tail pair with a 47K tail, about 48.2k in total).
- Plate loads: **100K** from each plate (pin 6 and pin 1) to B+ node **3** (circled 3).
- Output coupling: plate 6 -> **.047 uF / 400 V** -> the "phase A" line; plate 1 -> **.047 uF / 400 V** -> the "phase B" line. (Both caps are marked .047, 400V; the bottom one is rotated, reading ".047 400V".)

## 2. Cut and Master (between the two phase lines)

- **CUT: a 250KA pot** (printed "CUT 250KA") **in series with a ".0022" cap**, connected from the phase A line (after its .047 cap) to the phase B line (after its .047 cap). The pot's wiper is tied to one end, so it acts as a variable series resistor. So the Cut control is an RC shunt **across the two inverter outputs**: cap alone across the pair with the pot at 0 ohm, and nearly open with the pot at 250k. Its value ".0022" (2.2 nF) is as read; it could be ".0022" or ".022" **(?)**, the handwriting is rotated.
- **Master: a 1MA pot** (printed "1MA Master"), also connected between the two phase lines, in series with a switch labelled "Pull ... to close" (handwritten, reads "Pull MID to close" or "Pull MV to close" **(?)**). The wiper is tied to the top end (variable resistor). So the Master is a 0 to 1 M variable bridge resistance between the two inverter outputs, which the pull switch connects or removes. Exactly what the switch does to the sound is not described; the wiring only shows it connecting the pot bottom to the phase B line.
- Both of these loads are placed **after the .047 coupling caps and before the 1K5 grid stoppers and the 220K grid leaks**.

## 3. Power amp: 4 x EL84

Tube: **EL-84** (printed), four of them, in push-pull parallel.

Phase A line feeds the two upper tubes (tubes 1 and 2); phase B line feeds the two lower tubes (tubes 3 and 4):

- **Grid stoppers: 1K5** (printed "1K5") from the phase line to each EL84 grid (drawn as pin 2 on each tube; the standard EL84 control grid is pin 1, so the pin labels on this sketch are probably the author's shorthand **(?)**).
- **Grid leaks: 220K + 220K in series** between phase A line and phase B line, with their **midpoint to ground** (hollow triangle). So each phase line has 220K to ground.
- **Screen resistors: 100 ohm** from each screen (pin 9) to the screen supply bus, B+ node **2** (circled 2).
- **Plates (pin 7):** tubes 1 and 2 plates are joined onto one bus that goes to one end of the output-transformer primary; tubes 3 and 4 plates are joined onto the other bus that goes to the other end. The primary centre tap goes to B+ node **1** (circled 1).
- **Cathodes:** each tube's cathode goes to a labelled terminal: tube 1 -> **Y**, tube 2 -> **X**, tube 3 -> **X**, tube 4 -> **Y**. (So each cathode group X and Y contains one tube from each side of the push-pull pair.) The cathode pin is drawn as pin 3 on tubes 3 and 4 (the standard EL84 cathode pin is 2) **(?)**.

## 4. Cathode bias, Hi/Lo power switch (right-centre)

- The labelled terminals "to Y" and "to X" go to a two-pole **HI / LO power** switch (labelled "Hi/Lo Power", positions HI and LO).
- **Shared cathode resistors: 62 ohm 10 W** (printed "62 ohm 10W") from the common node to ground, and a second **68 ohm 10 W** (printed "68 ohm 10W", the 68 is slightly less clear than the 62 **(?)**) that is selected in series by the switch's LO position.
- **Cathode bypass: 250 uF / 100 V electrolytic** (printed "250/100V", + side on the cathode node) to ground, on the X node.
- Switch positions as drawn:
  - **HI:** the X and Y cathode groups are tied together (all four tubes conduct) and returned to ground through the **62 ohm** only (the 68 ohm is left open at one end). The 250 uF bypasses the common cathode node.
  - **LO:** the Y group is left unconnected (open contact, so tubes 1 and 4 are off) and the X group (tubes 2 and 3) returns through **68 + 62 = 130 ohm**, still bypassed by the 250 uF. So LO is two tubes (one per side of the push-pull), higher cathode resistance. This is my reading of the switch drawing **(?)**; the dots on the switch contacts are small.

So "the shared cathode resistor" is **62 ohm 10 W with a 250 uF/100 V bypass** (HI), or 62 + 68 = **130 ohm with the same bypass** (LO). All four tubes are cathode biased; there is no fixed (negative-supply) bias on this sheet.

## 5. Output transformer

- Primary: centre-tapped, centre tap to B+ node 1; the two ends to the plate buses (see section 3).
- Printed: "O/T WTI 9356".
- Secondary: taps marked **04, 08, 16** (ohm) with a note "Z SEL" (impedance selector), a **Phase / Reverse** DPDT switch and the speaker jack ("V" symbol at the right, ground triangle below). The 16 ohm end goes to the switch contact next to the tap marked 16. No turns ratio, primary impedance or DC resistances are printed.

## 6. Nodes and voltages

| Node | What it feeds | Voltage |
|---|---|---|
| 1 | output transformer primary centre tap (main B+) | not printed on this sheet |
| 2 | EL84 screens, through four 100 ohm resistors | not printed |
| 3 | 12AX7 phase inverter plate loads (two 100K) | not printed |

The supply (rectifier, reservoirs, choke, dropping resistors) is not on this sheet.

## 7. Asked-for items and where they stand

| Item | On this sheet? |
|---|---|
| Channels Normal / Top Boost / Vibrato | No. The sheet has CH1 and CH2 inputs only. No vibrato of any kind. |
| EL84 power stage | Yes (section 3) |
| Shared cathode resistor and bypass | Yes: 62 ohm 10 W (+ 68 ohm 10 W for LO), 250 uF/100 V |
| Cathode follower | **No** (no cathode follower on this sheet; the phase inverter is a long-tail pair) |
| Tone cut (Cut control) | Yes: 250KA + .0022 between the phase lines |
| Tone stack values | **No** |
| B+ nodes | Circled 1, 2, 3 only, no voltages |
| Rectifier | **No** |
| Preamp tubes, plate loads, cathode values per stage | Only the phase inverter 12AX7: 100K plate loads, 1K2 + 47K tail |
| Output transformer | Yes, label only: "WTI 9356", taps 04/08/16, Phase/Reverse switch |

## 8. Unreadable or doubtful

1. ".0022" Cut cap: could be .0022 or .022 (rotated handwriting).
2. 68 ohm resistor value (68 vs 6B?), and the exact switch contact connections of the Hi/Lo switch.
3. The pull-switch label on the Master pot (MID vs MV).
4. EL84 pin numbers drawn for grid (2) and cathode (3) are not the standard EL84 pins.
5. "250/100V": the 250 could also be 220 or 25 **(?)**; the sheet reads 250.
