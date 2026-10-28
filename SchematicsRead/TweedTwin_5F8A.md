# Fender "Twin-Amp" Model 5F8-A (tweed) - schematic read

Source: `6bbeaa08-fender_twin_5f8a.pdf` (2 pages, scans: p1 schematic 1506x841, p2 layout 1483x776; hand-lettered, initials "I-EG"). Printed notice: "Voltages read to ground with electronic voltmeter, values shown +/-20 %". Layout note: "All resistors are one-half watt 10 % tolerance unless otherwise noted". Printed parts box: "Power Trans. 7993, Choke 14684, Output Trans. 45268". "117 V. AC" mains, 3 amp fuse.
Conventions: "1500" = 1.5k; ".0001" = 100 pF; ".00025" = 250 pF; "250-6" = 250 uF/6 V; "20-600P" = 20 uF/600 V; "8-450" = 8 uF/450 V; ".02-400" = .02 uF/400 V. "(?)" = not certain; "(computed)" = my arithmetic.

## 1. Tube complement and heater notes
| Tube | Function |
|---|---|
| 12AY7 (one dual triode) | Bright channel stage, Normal channel stage (two halves, shared cathode) |
| 12AX7 (first, "12AX7" drawn left of tone stack) | Triode 1: mixed-channel gain stage; triode 2: cathode follower driving the tone stack |
| 12AX7 (second, PI) | Long-tail phase inverter |
| 5881 x4 | Output, two per phase |
| GZ34 | Rectifier (tube) |
- Heaters: "To all 6.3 volt heaters and pilot light" (winding with centre terminal grounded, drawn with a ground arrow). 5-pin heater/B+ details not drawn beyond that.
- Mains: ground switch with .05-600 cap, AC switch, 3 amp fuse. Standby switch present.

## 2. Stage by stage
### Inputs
- Each channel (Bright, Normal): two jacks (1, 2) with **68k + 68k** in series (grid at the junction) and **1M** from the jack-1 end to ground (jack 2 / jack 1 switch arrangement drawn as in the later Fender amps).
### 12AY7 stages (one triode per channel)
- Plate load **100k** each to the +295 V node (the two 100k resistors meet at a common wire running to +295 V); plates **+145 V** (both labels).
- **Shared cathode: 820 ohm || 250 uF/6 V**, **+2.2 V**.
- **Bright channel**: plate -> **.02-400** -> **Volume 1M** (top); **.0001 (100 pF)** across the Volume top to wiper (bright cap, no switch). Volume taper: layout says "1M AUD."; schematic "VOL. 1M".
- **Normal channel**: plate -> **.02-400** -> **Volume 1M** (layout "1M AUD."); no bright cap.
- Each Volume wiper -> **270k** mixing resistor; both 270k resistors meet at the grid node of the first 12AX7 triode.
### First 12AX7 (mixing/gain + cathode follower)
- **Triode 1**: grid at the 270k junction (no extra grid leak drawn); plate load **100k** from the +295 V line (resistor drawn between the plate node and the follower's plate node, which sits on +295 V); plate **+165 V**; cathode **820 ohm unbypassed**, **+1 V**.
- **Triode 2 (cathode follower)**: grid directly on triode 1's plate; plate on +295 V; cathode resistor **100k** to ground; cathode printed **+165 V** (the same +165 V figure appears on both the triode-1 plate and follower cathode; I read both as printed, (?)). The follower cathode drives the tone stack (see section 3); the follower's cathode output node is also the top of the 56k stack resistor.

## 3. Tone stack (tweed Twin, 56k slope)
- Input from the cathode-follower output. **56k** slope resistor from the input node to the common node. **.00025 (250 pF)** from the input node to the Treble top.
- **Treble 250k** (layout "250K LIN."; schematic "TREB. 250K"); wiper = stack output.
- **.02-400** from the common node to the Treble-bottom / Bass-top node.
- **Bass 1M** (layout "1M AUD."; drawn as a variable resistor).
- **.02-400** from the common node to the **Middle pot wiper**; **Middle pot 2.5K on the schematic, "25K LIN." (?) on the layout**: not resolved (see section 8); its bottom end is grounded and its top connects to the Bass bottom.
- Stack output -> **.02-400** -> PI top grid. No bright or shift switching in the stack.

## 4. Reverb / tremolo: none on this amp.

## 5. Phase inverter: 12AX7 long-tail
- Top grid: **.02-400** from the tone-stack output; top grid **+19 V**. Bottom grid: **.1-200** from the NFB node; bottom grid **+20 V**.
- Grid leaks: two resistors (drawn "1M" (?)) from each grid to the centre node X; X labelled **+27 V** (arrow), but with grids at +19/+20 V this is inconsistent (a grid leak carries no DC current); treat as uncertain.
- Cathodes joined (**+22.5 V** printed) -> **470 ohm** -> X -> **10k** -> NFB node (line to the Presence pot and the 56k NFB resistor).
- **47 MMF (47 pF)** capacitor between the two plates.
- Plate loads: **82k 5 %** (top) and **100k 5 %** (bottom), centre node on the **+355 V** supply node; plates **+230 V** (top), **+225 V** (bottom).
- Plate outputs: **.1-400** each to the phase node. Phase node to output grids through **1500 ohm** stoppers; **220k + 220k** in series between the two phase nodes with the centre node to the bias line (**-41 V**).
- Consistency note (computed): with centre node +355 V the plate currents are about 1.5 mA and 1.3 mA; this would need about 28 V across the 10k, in line with +27 V at X, but then the cathode would be near +28 V, not +22.5 V. The printed cathode and X voltages cannot both be correct.

## 6. Output stage
- **5881 x4**, two per phase in parallel push-pull; cathodes to ground (no cathode resistor, no bypass); **1500 ohm** grid stoppers; **470 ohm 1 W** screen resistor per tube from the **+395 V** node; plate voltages: OT centre tap fed from the **+397 V** node.
- Bias: **-41 V fixed** (printed at the bias node); bias supply from a 300 V AC tap via a selenium rectifier (layout "SEL. RECT."), see section 7. No adjust control.
- OT (part 45268): primary centre-tapped; secondary with two speaker circuits each via a jack with a switch contact; speaker impedance, tap impedance and OT primary impedance not printed. Two speaker symbols drawn.
- **Global NFB**: **56k** from the OT secondary hot side to the PI NFB node (junction of the 10k, the .1-200 bottom-grid cap and the Presence pot). **Presence control: 5K pot ("5K LIN." on the layout)** from the NFB node to ground, wiper to **.1-200** cap to ground (cap shunts the lower part of the pot at HF).

## 7. Supply
- Power transformer 7993: HV secondary printed **300 V AC** each side of the centre tap; **GZ34** rectifier. Filament winding shown for the GZ34 separately.
- GZ34 cathode node -> **STANDBY switch** -> node **+397 V** (two 20-600P filters on this node, plus a .05-600 cap on the rectifier side of the switch); OT centre tap taken from this node.
- Choke (14684) from the +397 V node to the **+395 V** node (one 20-600P cap): screens (470 ohm 1 W each).
- +395 V -> **4700 ohm** -> **+355 V** node (**20-600P**): PI plate loads centre.
- +355 V -> **10k** -> **+295 V** node (**8-450**): preamp plate loads (12AY7, first 12AX7).
- Bias supply: tap on the HV winding (300 V AC) -> selenium rectifier (+ to the winding, - to node A) -> node A with **8-150** filter cap -> **15k** -> node B (**-41 V**, **8-150** filter cap) -> **56k** to ground. Raw node A not printed (computed about -52 V from the 15k/56k divider).
- Sag sources: tube rectifier (GZ34, internal resistance), choke, 4.7k/10k RC dropper chain feeding the screens/PI/preamp, 5881 screens fed after the choke (+395 V), plates ahead of the choke.

## 8. Not legible / uncertain
- Middle pot: schematic reads "2.5K", layout "2.6K/25K LIN." (?).
- PI grid-leak value ("1M" with a slash mark), the +27 V X label, the +22.5 V cathode label, and the follower's "+165 V".
- Whether the bass pot is wired as a variable resistor with the wiper tied (drawn so), jack-switch routing, speaker impedances, OT primary impedance, plate voltage of the 5881 (only the OT centre tap node +397 V printed).
- Rectifier winding heater voltage details, choke DCR.
