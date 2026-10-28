# Fender Bandmaster 5E7 (tweed, 3x10) - schematic read

- Source file: `bc9b0ee4-Fender_Bandmaster_5E7.pdf` (1 page scan; raster 3065x1850 used; good legibility)
- Printed title: `FENDER "BANDMASTER" SCHEMATIC  MODEL 5E7`; mark "E-EE" (initials) near the notice. No date/revision other than the model code.
- NOTICE printed: "Voltages read to ground with electronic voltmeter. Values shown +/-20%". No parts-number box on this drawing.
- Cap notation: "value-voltage", e.g. ".02-600" = 0.02 uF/600 V; "25-25" = 25 uF/25 V; "16-450" = 16 uF/450 V; "8-450" = 8 uF/450 V.

## 1. Tube complement / heaters
| Pos | Tube | Function |
|---|---|---|
| V1 | 12AY7 | Two input channels (Channel 1 with bright cap, Channel 2 plain) |
| V2 | 12AX7 | V2a gain stage after mixing; V2b cathode follower (drives tone stack) |
| V3/V4 | 12AX7 (one envelope, two halves) | V3 gain stage (cathode-feedback node); V4 cathodyne phase inverter |
| Output | 2 x 6L6G | Push-pull, fixed bias, cathodes grounded |
| Rectifier | 5U4G | Full-wave tube rectifier |

Heaters: "TO ALL 6.3 VOLT FILAMENTS AND PILOT LIGHT" from the 6.3 V winding with grounded center tap. Mains: AC plug, GROUND SWITCH with **.05-600** cap, AC SWITCH, **3 AMP. FUSE**, power transformer; HV winding feeds 5U4G plates; 5U4G filament winding separate.

## 2. Preamp stages
### Inputs (both channels)
- Jacks 1 and 2 per channel: **68k** from each jack to the V1 grid (grid between two 68k); **1 MEG** from the jack-1 node to ground; jack-2 switch contact connected to jack-1 node.

### V1a - Channel 1 (12AY7 half, "bright" channel)
- Plate load **100k** to the B+ rail (+280 V rail); plate voltage **+140 V**.
- Cathode: **820 ohm || 25-25 (25 uF/25 V)** shared with V1b; cathode **+2.2 V**.
- Coupling: plate -> **.02-600** -> VOLUME pot top. VOLUME pot **1 MEG** (taper not printed). **.0001 uF (100 pF)** from the volume-pot top to the wiper (bright cap). Wiper -> **270k** -> mix node.

### V1b - Channel 2 (12AY7 half)
- Plate load **100k**, plate **+140 V**; shared cathode network; coupling **.02-600** -> VOLUME **1 MEG**; no bright cap; wiper -> **270k** -> mix node.
- (The two volume pots' grounded ends are shown joined at a ground symbol.)

### V2a (12AX7 first half)
- Grid = mix node (junction of the two 270k).
- Also **10 MEG** resistor from this grid back to the tone-stack output node (the node after the .1-200 coupling cap, where the 220k-to-ground and the 100k branch meet).
- Cathode **1500 ohm || 25-25 (25 uF/25 V)**; cathode **+2.2 V**.
- Plate load **100k** to the B+ line (the +280 V rail through V2b's plate wire); plate voltage **+140 V**.
- Plate direct-coupled to V2b grid.

### V2b (12AX7 second half) - cathode follower
- Plate tied directly to the +280 V rail. Cathode resistor **100k** to ground; cathode voltage **+140 V**.

## 3. Tone stack (Bandmaster/tweed bass-treble)
Driven from the V2b cathode node:
- **TREBLE**: pot **1 MEG**. Path: V2b cathode -> **.00025 uF (250 pF)** -> treble pot bottom end; the pot's top end -> **.01 uF** to ground; treble wiper -> the V3 grid node (stack output).
- Mid/bass path: V2b cathode -> **.1-200** -> node M; node M -> **220k** to ground; M -> **100k** -> node B (bass pot wiper) -> **220k** -> V3 grid node.
- **BASS**: pot **1 MEG** ("1 MEG BASS"), used as shunt-to-ground: its wiper connects to node B; one end through **.005 uF** to ground, other end to ground.
- Output node (V3 grid) = junction of the 220k and treble wiper.
- Taper letters not printed. No middle control.

## 4. Reverb / tremolo / vibrato
None.

## 5. V3 and Phase inverter (V4, 12AX7, cathodyne/split-load)
- **V3 (12AX7 half)**: grid = stack output; cathode **1500 ohm to ground, unbypassed**, cathode **+1.7 V**; the NFB node (56k from OT secondary, presence control) connects to this cathode. Plate load **100k** to the B+ line; plate **+213 V**. Plate -> **.02-600** -> V4 grid.
- **V4 (12AX7 half) cathodyne**: grid resistor **1 MEG** from the grid to the bottom of the cathode load (node printed **+53.3 V**). Plate load **56k** to the **+332 V** node; plate voltage **+270 V**. Cathode load: **1500 ohm** then **56k** to ground; cathode (top of 1500) printed **+55 V**.
- Outputs: plate -> **.1-400** -> upper 6L6 grid node; cathode (+55 V node) -> **.1-200** -> lower 6L6 grid node. (Note: coupling cap voltage ratings .1-400 and .1-200 as printed.)

## 6. Output stage
- 2 x **6L6G**, cathodes tied together directly to ground (no cathode resistor), **fixed bias -40 V**.
- Grid stoppers: **1500 ohm** each; grid leaks **220k + 220k** in series between the two grids, center node = bias line (**-40 V**).
- Screens connected directly (no screen resistors) to the **+410 V** line (the same line as the OT center tap).
- Plate/screen voltage printed **+410 V**.
- Output transformer: primary center tap to +410 V; secondary feeds **three speakers in parallel** (three speaker symbols, 3x10"); one end of secondary grounded; no ratio/impedance printed.
- Global NFB: **56k** from the hot end of the OT secondary to the NFB node; the node ties to the V3 cathode (1500 ohm) and to a **PRESENCE 5k** pot (variable resistor with wiper to **.1-200** to ground: pot in series with the cap from the NFB node to ground). So feedback returns to V3 cathode.
- Bias supply: tap from the HV winding -> **6800 ohm** -> diode (negative output) -> node (**-40 V**) with filter cap (value not printed; cap drawn "-" at node, "+" at ground side) in parallel with **56k** to ground.

## 7. Power supply
- Rectifier **5U4G**, plates from the HV winding (voltage not printed), cathode (filament) node feeds the **STANDBY SWITCH**; **.05-600** cap from the HV center/rectifier line to ground at the switch.
- After the standby switch: **+415 V** node with **16-450** (16 uF/450 V) -> choke (printed, no part number) -> **+410 V** node (**16-450**; OT center tap and 6L6 screens).
- **10K** -> **+332 V** node (**16-450**; V4 plate load 56k and 6L6 grid-leak reference nodes) -> (same rail) V3 plate load 100k -> **10K** -> **+280 V** node (**8-450**): V1, V2 plate loads (100k each) and V2b plate.
- Printed B+ voltages: +415, +410, +332, +280 (the preamp rail printed as "+280 V" vertical label).
- Other printed: -40 V bias.

## 8. Not legible / uncertain
- Taper letters for all pots (none printed).
- Exact node printed as "+2?3 V" for V3 plate: read as +213 V (?).
- Bias filter cap value; HV winding AC voltage; OT impedance/ratio; speaker impedances; choke inductance.
- The orientation of the BASS pot ends (one end through .005 uF to ground, other end grounded) read as drawn.
- Which side of the stack the 10 MEG feedback attaches to: read as the node after the .1-200 coupling cap (also the 220k-to-ground node) (?).
