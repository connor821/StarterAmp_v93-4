# Fender "Super-Amp" Model 5F4 (tweed, 6L6G) - schematic read

Source: `f6b820eb-fender_super_5f4.pdf` (2 pages: p1 schematic 1506x900, p2 layout 2276x1181, hand lettered "C-EG"). Notice: "Voltages read to ground with electronic voltmeter, values shown +/-20 %". Printed parts box: "Pow. Trans. 8087, Out. Trans. 45216, Choke 14684".
Conventions as in the 5E4-A file ("1 MEG" = 1 Mohm; "25-25" = 25 uF/25 V; "16-450" = 16 uF/450 V). "(?)" = not certain.

## 1. Tube complement and heater notes
| Tube | Function |
|---|---|
| 12AY7 (V1) | Two channels, one triode each, shared cathode |
| 12AX7 (V2) | Triode 1: mixed-channel gain; triode 2: cathode follower to the tone stack |
| 12AX7 (V3) | Triode 1: gain stage with NFB at its cathode; triode 2: cathodyne PI |
| 6L6G x2 | Output |
| 5U4G | Rectifier |
- Heaters: 6.3 V winding, centre terminal grounded, "To all 6.3 volt filaments and pilot light". Ground switch with .05-600 cap, AC switch, 3 amp fuse, standby switch.
- Compared with the 5E4-A: 6L6G output pair instead of 6V6GT, higher B+, same overall preamp topology (feedback resistor 4.7 MEG instead of 5 MEG).

## 2. Stage by stage
- **Inputs**: two channels (the schematic does not name them); each has jacks 1/2 with **68k + 68k** and **1 MEG** to ground, as in the 5E4-A.
- **V1 12AY7**: plate loads **100k** each (centre node to the **+280 V** rail), plates **+140 V** (printed at the upper plate; lower plate "+140 V" read, (?)); shared cathode **820 ohm || 25-25**, **+2.2 V**. Coupling **.02-600** each to a **1 MEG Volume pot**; **.0001 uF** bright cap across the upper channel Volume (top to wiper). Each wiper -> **270k**; the 270k resistors join and go to V2 triode-1 grid.
- **V2 triode 1**: cathode **1500 || 25-25**, **+2.2 V**; plate load **100k** to the +280 V rail (through the follower's plate node); plate **+140 V**.
- **V2 triode 2 (cathode follower)**: grid on triode-1 plate, plate to +280 V, cathode **100k** to ground, **+140 V** printed.
- **4.7 MEG** from the V2 triode-1 grid node to the node after the .1-200 tone-stack cap (5E4-A: 5 MEG).

## 3. Tone stack (same topology as the 5E4-A)
- From the follower cathode node T: **.1-200** to node N; **220k** N to ground; N -> **100k** -> node M -> **220k** -> PI input line (which also receives the Treble wiper).
- **Bass 1 MEG ("1 MEG. BASS")**: wiper at M; one end grounded, the other end through **.005 uF** (label reads ".005"/".00S" (?); layout "0.005 MFD 600 V") to ground.
- **Treble 1 MEG**: top end through **.01** to ground (layout ".01 MFD 600 V"), bottom end through **.00025 uF (?)** to T; wiper -> PI input line.
- No switching.

## 4. Reverb / tremolo: none.

## 5. Phase inverter: cathodyne, 12AX7 V3
- Triode 1: grid from the tone-stack line; cathode **1500 ohm unbypassed**, **+1.7 V**; plate load **100k** to the **+332 V** node; plate printed **+273 V (?)**.
- Triode 1 plate -> **.02-600** -> cathodyne grid; **1 MEG** grid leak to the node between the 1500 and 56k cathode resistors (**+53.3 V**).
- Cathodyne: plate load **56k** to **+332 V**; plate **+270 V**; cathode node **+55 V**; cathode load **1500 ohm** in series with **56k** to ground (total 57.5k; plate load 56k).
- Outputs: plate -> **.1-600** -> upper-phase node; cathode node -> **.1-200** -> lower-phase node.

## 6. Output stage
- **2 x 6L6G**, cathodes to ground; **1500 ohm** grid stoppers; **no screen resistors** (screens wired to the OT centre tap node).
- Grid returns **220k + 220k** between the phase nodes, centre to **-40 V** bias.
- Bias: **-40 V** printed; fixed; no adjust.
- OT 45216: primary centre tap node **+410 V** (plates and screens); secondary with two speaker symbols (each via a jack) and the secondary hot also goes to the NFB resistor; speaker/OT impedances not printed.
- **Global NFB**: **56k** from the OT secondary hot to the V3 triode-1 cathode node (divider 1500/(56k+1500) = 0.026, about -32 dB, computed). **Presence 5K** pot with wiper to **.1-200** to ground, one end on the NFB line, other end unconnected as drawn.

## 7. Supply
- **5U4G**; HV winding voltage not printed. **.05-600** from the rectifier cathode node to ground; **standby switch** between the rectifier node and the **+415 V** node (**16-450**).
- Choke (14684) from +415 V to the **+410 V** node (**16-450**): OT centre tap, plates and screens.
- +410 V -> **10k** -> **+332 V** (**16-450**): PI plate loads.
- +332 V -> **10k** -> **+280 V** (**8-450**): V1 and V2 plate loads.
- Bias supply: HV winding tap -> **6800 ohm** -> diode (+ toward winding, - toward the output) -> **-40 V** node with a filter cap (value not printed on the schematic; layout shows "100 MFD 25 V") and **56k** to ground.
- Compared with the 5E4-A: B+ higher (+410/+415 vs +385/+390), dropper values 10k/10k same, bias -40 V from 6.8k instead of -32 V from 18k.
- Sag sources: tube rectifier, choke feeding both plates and screens, no screen resistors.

## 8. Not legible / uncertain
- Lower V1 plate voltage and "+273 V" at the PI left-triode plate; Treble lower cap (".00025" read), Bass cap (".005").
- HV winding voltage, speaker and OT impedances, pot tapers.
- Bias filter cap value is from the layout page only.
