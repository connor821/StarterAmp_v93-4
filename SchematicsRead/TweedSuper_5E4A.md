# Fender "Super-Amp" Model 5E4-A (tweed) - schematic read

Source: `9cbfd225-fender_super_5e4a.pdf` (2 pages: p1 schematic 1524x870, p2 layout 2250x1287, hand lettered, "G-EE"). Notice: "Voltages read to ground with electronic voltmeter, values shown +/-20 %". Printed note: "* NOTE - (A) was 22K ohms" (the circled A is the 18k resistor in the bias supply). "Model 5E4-A*" (asterisk = see note on reverse side).
Conventions: "1 MEG" = 1 Mohm; "25-25" = 25 uF/25 V; "16-450" = 16 uF/450 V; "100-25" = 100 uF/25 V. "(?)" = not certain.

## 1. Tube complement and heater notes
| Tube | Function |
|---|---|
| 12AY7 (first, V1) | Instrument-channel stage (upper triode) and Mic-channel stage (lower triode), shared cathode |
| 12AY7 (second, V2) | Triode 1: mixed-channel gain stage; triode 2: cathode follower |
| 12AX7 (V3) | Triode 1: gain stage with NFB injection at its cathode; triode 2: cathodyne (split-load) phase inverter |
| 6V6GT x2 | Output, push-pull |
| 5U4GA | Rectifier |
- Heaters: 6.3 V winding with centre terminal grounded, "To all 6.3 volt filaments and pilot lite". Ground switch with .05-600 cap; AC switch; 3 amp fuse; standby switch.

## 2. Stage by stage
### Inputs
- Two channels (INST.: jacks 1, 2; MIC: jacks 1, 2). Each jack pair: **68k + 68k** in series (grid at the junction), **1 MEG** from the jack-1 end to ground (jack-2 / jack-1 switching as in the later amps).
### V1 12AY7 (one triode per channel)
- Plate loads: **100k** per plate; the two 100k meet at a centre node that goes to the **+250 V** rail; plates about **+125 V** (value printed on the layout page next to the socket) (?).
- **Shared cathode: 820 ohm || 25 uF/25 V**, **+1.9 V**.
- Coupling: **.02-600** from each plate to its Volume pot. **Instrument Volume 1 MEG** with a **.0001 uF (100 pF)** bright cap across top to wiper; **Mic Volume 1 MEG** (no bright cap). Layout notes "INST. VOL. 1 MEG" and "MIC. VOL. 1 MEG" (taper not printed).
- Each Volume wiper -> **270k**; the two 270k join at the grid node of V2 triode 1.
### V2 12AY7
- **Triode 1**: grid at the 270k junction; cathode **1500 ohm || 25 uF/25 V (written "25-25")**, **+1.9 V**; plate load **100k** to the +250 V rail (drawn between the plate node and the follower's plate node, which is on +250 V).
- **Triode 2 (cathode follower)**: grid on triode-1 plate; plate on +250 V; cathode **100k** to ground, **+130 V** (printed).
- **5 MEG** resistor from the V2 triode-1 grid node to the tone-stack node after the .1-200 cap (positive-feedback/bright-style network as drawn, (?) function).
- Follower cathode -> tone stack.

## 3. Tone stack (tweed Super: 1 M / 1 M, .005 and .00025 caps)
Topology as drawn from the follower output node T:
- **.1-200** from T to node N; **220k** from N to ground.
- N -> **100k** -> node M -> **220k** -> PI input line.
- **Bass 1 MEG**: wiper at M; one pot end to ground, the other end through **.005-600 (printed ".005/600"; layout ".005 600 V.")** to ground.
- **Treble 1 MEG**: wiper = PI input line; top end through **.01-400** to ground; bottom end through **.00025 uF** to T.
- Layout shows Bass and Treble as "1 MEG" pots and a 220k between them (matches the 220k from M to the PI input line).
- No presence-type cap in the stack; no switching.

## 4. Reverb / tremolo: none on this amp.

## 5. Phase inverter: cathodyne (split-load), 12AX7 triode 2
- Triode 1 (V3a): grid from the tone-stack output; cathode **1500 ohm unbypassed**, **+1.6 V**; plate load **100k** to the **+300 V** rail.
- V3a plate -> coupling cap (value not legible; layout shows .02/.1 type caps, (?)) -> V3b grid; **1 MEG** grid leak from V3b grid to the cathode-bottom node (**+56.5 V**).
- V3b: plate load **56k** to **+300 V**; cathode path **1500 ohm** (cathode node **+58 V**) in series with **56k** to ground (node between them **+56.5 V**). So plate load 56k, cathode load 1500 + 56k = 57.5k (split-load, almost balanced).
- Outputs: plate -> **.1-400** -> upper-phase node; cathode node -> **.1-200** -> lower-phase node.
- Plate voltage of V3b not printed on the schematic (layout page shows "+245 V" near the coupling cap (?)).

## 6. Output stage
- **2 x 6V6GT**, cathodes to ground (fixed bias), **1500 ohm** stoppers; **no screen resistors** (screens wired directly to the OT centre tap / +385 V node).
- Grid returns: **220k + 220k** in series between the two phase nodes, centre node to the bias supply (**-32 V**).
- Bias: **-32 V** printed; supply below. No adjust control.
- OT: centre-tapped primary at **+385 V**; secondary to speaker jacks: two speaker symbols plus "EXTRA 8 ohm SPKR." jack (so each speaker output is a switched jack; impedance values not printed except the extra jack note).
- **Global NFB**: **56k** from the OT secondary hot side to the **cathode of V3a** (node with the 1500 ohm unbypassed cathode resistor). Divider (computed): 1500/(56k + 1500) = 0.026, about -32 dB. **Presence: 5K pot** with its wiper to a **.1-200** cap to ground, one end on the NFB line (the other end not connected as drawn (?)).

## 7. Supply
- **5U4GA** rectifier; HV winding voltage not printed. Standby switch in the rectifier cathode line.
- **+390 V** node after standby: **16-450** cap. Choke (not numbered) -> **+385 V** node (**16-450**): plates (OT centre tap) and 6V6 screens.
- +385 V -> **10k** -> **+300 V** (**16-450**): V3a plate load, V3b plate load, cathodyne.
- +300 V -> **10k** -> **+250 V** (**8-450**): V1 and V2 plate loads.
- **Bias supply**: tap from the HV winding -> **18k** (note A: previously **22k**) -> diode (layout: "SELENIUM RECTIFIER", + to the winding side, - to the output) -> node **-32 V** with **100-25** (100 uF/25 V) cap and **56k** to ground.
- Layout page voltage labels (not on the schematic): +125 V (V1 plates), +190 V (near .02-600 coupling cap, (?)), +245 V, +250 V, +300 V, +385 V, +390 V, -32 V, +58 V, +56.5 V, +130 V, +1.9 V, +1.6 V.
- Sag sources: tube rectifier (5U4GA, high internal resistance), choke ahead of both plates and screens (everything after the choke), 6V6 screens on the same node as the plates.

## 8. Not legible / uncertain
- PI coupling cap values; whether V3a plate +245 V is the V3b plate or V3a plate; +190 V label.
- Function of the 5 MEG and the exact ground/cap arrangement of the Bass pot.
- OT impedances, speaker impedances, HV winding voltage, choke value.
- Pot tapers (only "1 MEG" printed on the schematic; layout does not print AUD/LIN for these).
