# DIY "SSL Mixbus Compressor Clone" (SSL 4000E 82E26/82E27 cards, THAT 2180/2181 version): notes

Sources read: `SSLclone.txt` (341 lines, read fully), `ssl-01.pgm.png` (page 1: front-panel drawing and intro), `ssl-02.pgm.png` (page 2: "The Circuit" with the schematic thumbnail), `ssl-08.pgm.png` (page 8: prototype photo and "Problems when building"). The same document is `reference/compressors/sources/SSL_Mixbus_Compressor_Clone.pdf` (11 pages).

What is NOT readable
- **The schematic is a 196 x 249 pixel thumbnail** (embedded in the PDF at that size; page 2 image). No component can be read. The real schematic ("ssl.pdf", PCB Rev 9 / SCH Rev 7), the components list, the placement and PCB artwork are separate downloads that are **not in the file set**: "Components list" and "The Schematics" appear only as empty link text. So the attack resistor values, the release capacitor and discharge-resistor values (except the two Auto pairs quoted in the text), the ratio-switch offsets and the exact threshold network are **not available**.
- ssl-01 is the front-panel drawing: knob centre positions along the 19-inch panel (50, 120, 144, 193 ("49 mm" gap), 242, 291, 340, 375, 405, 425 mm; the last two gaps 35 and 30 mm), labels GAIN REDUCTION, THRESHOLD, RATIO, ATTACK, RELEASE, MAKEUP, BYPASS, POWER, "TYPE 4000", "STEREO COMPRESSOR". It carries no electrical data. ssl-08 is a prototype photo (moving-coil meter, THRESHOLD knob) plus the build-problem list. Neither adds a number.

Everything below is from the text. "(derived)" means I computed it from quoted numbers.

## 1. What it is

- Stereo mix-bus compressor cloned from the SSL 4044E desk (1985): the compressor inside the console's main quad fader/autofade system, cards 82E26 and 82E27. Changes from the original: electronically balanced input and output, autofade and main-fader parts removed, and **one sidechain common to both VCAs** (so the image does not "tilt" when compressing heavily; "tracking errors are cancelled").
- No tubes or transformers. Original VCA: DBX 202XT (ten paralleled 2150-type cells with a common low-impedance control buffer, per the author's reverse engineering). Substitute: one THAT 2180 or 2181 per VCA (three VCAs: two audio, one sidechain "dummy"); "the sound doesn't seem that different ... the 202 may tend to be a little more transparent".
- Supply: +/-15 V main, with +/-12 V sub-regulators on the board "for obtaining predictable gains"; 7815/7915 regulators; toroid 2 x 15 V, 10 VA or more; fuse for about 25 W.

## 2. Audio path

| Stage | Detail |
|---|---|
| Input | de-balanced by a pair of **NE5534**; input impedance **50 kohm electronically balanced** (Data) |
| Into the VCA | drives the VCA in **current mode through 27 kohm resistors** (THAT VCAs are current-in, current-out). The old component overlay marked these **15 kohm**; the 2006 revision "unity gain (two 15K resistors changed into 27K)". 27/15 = 1.8 (derived: 5.1 dB less signal current into the VCA, compensated by the VCA/output stage) |
| VCA | THAT 2180 (pre-trimmed: cut pin 4, the distortion-trim pin) or 2181 (external THD trim; "leave the trimmers about centered"). Distortion "almost exclusively second harmonic as far as I can measure ... and hear"; trimming a bit off centre was used by one maker "to get a little more 'sound'" |
| Output | current-to-voltage converter combined with a balanced output drive stage: **two NE5532**; output impedance **100 ohm** electronically balanced |
| Make-up | a DC voltage from the makeup pot is summed with the sidechain control voltage and drives the main VCAs. The original SSL makeup pot is active even in bypass (excess gain when bypassed). On this control PCB there is an option to disconnect makeup when bypassed so in/out compares fairly |

## 3. Sidechain and detector (the part that matters for modelling)

1. The two input op-amp outputs are summed through **47 kohm** resistors into the input of a third, **sidechain VCA**, a "tracking dummy" paralleling the gain reduction of the main VCAs. Purpose in the original: using one set of main VCAs for compression, fader, computer and autofade at the same time "without having to resort to less predictable feed-forward compression schemes". Result: "a combination of a feed-forward and a feed-back architecture, **acting mostly as a feed-back compressor**" (the detector reads the signal after the sidechain VCA, i.e. after the gain reduction it has itself commanded).
2. The sidechain VCA output is **full-wave rectified by two TL074 stages**. At this point **an offset is added and the signal gain is controlled by the ratio switch**; the ratio switch "actually changes the threshold a bit to compensate for level fluctuations caused by different ratio settings". The author says this is "the only part of the design that in any way differs from a direct off-the-application-note design" (i.e. the rest is the THAT application-note VCA compressor). **The size of the threshold shift per ratio is not given.**
3. The rectified DC goes through the bypass switch and one of a **bank of resistors selected by the ATTACK switch** to charge a **tantalum capacitor selected by the RELEASE switch**. The release tantalums are bypassed by different discharge resistors, "each delivering the desired release time".
4. **Threshold diode**: "The 'real' threshold to be overcome before charging the attack/release caps is the **0.6 V AK voltage across the diode** placed between the full-wave rectifier and the A/R timing."
5. **Threshold control**: the buffered DC is also summed with a DC voltage from the **THRESHOLD pot** (50 kohm linear) and drives the sidechain VCA. "In this way the added gain in the sidechain VCA looks to the rectifier like there is more signal coming in, changing the threshold this way." So the threshold knob works by gain ahead of the rectifier (and since the control signal is fed back, it also interacts with the amount of reduction). For modern levels the pot "can seem too sensitive"; fix: **a 47 kohm resistor between the control PCB and the + side of the threshold pot** (2006 revision "threshold de-sensitize").
6. The buffered, timed DC goes to three places: (a) summed with makeup DC to control the main VCAs; (b) summed with the threshold DC to control the sidechain VCA; (c) the **1 mA moving-coil GR meter**: linear scale at about **50 uA/dB**, so 1 mA = **20 dB full scale**; replace the 2 kohm series resistor with **1 kohm** for 0 to 10 dB full scale; for a 100 uA meter shunt 330 ohm across it.
7. Ratio correction for the THAT VCA: "the **100 kohm** resistor marked has to be replaced by a **127 kohm** resistor to compensate for higher input current sensitivity" (2006 revision "ratio adjust"). 127/100 = 1.27, i.e. 2.1 dB (derived). Without the change the ratios would be off by that factor.

## 4. Switch values and the derived times and ratios

Published "DATA" block (text, as is):

| Control | Positions |
|---|---|
| Ratio | 2:1, 4:1, 10:1 (three positions; ratio switch is a 4 x 3 Lorlin) |
| Attack | 0.1, 0.3, 1, 3, 10, 30 ms (six positions; 2 x 6 switch) |
| Release | 0.1 s, 0.3 s, 0.6 s, 1.2 s, "2.4 (Auto)" (five positions; a 2 x 6 switch programmed to 2 x 5) |
| Make-up gain | 0 to 20 dB adjustable (50 kohm lin pot) |
| Threshold | -20 to +20 dBm (50 kohm lin pot); 40 dB of range |
| Bypass | 4 x 3 switch programmed 4 x 2 |
| Input / output | 50 kohm electronically balanced in; 100 ohm electronically balanced out |
| Frequency response | less than 15 Hz to more than 35 kHz within 3 dB |
| Noise | less than -80 dB |
| Distortion | about -75 dB unadjusted, mostly 2nd harmonic (-75 dB = 0.018%, derived) |

**Auto release** (the fifth release position): "combining two release time constants (**91k + 6u8** and **750k + u47**)... short time constants for short programme peaks, but if compression is going on for a longer time, the slow time constant will set in." Taking these as R and C: 91 kohm x 6.8 uF = **0.62 s**, 750 kohm x 0.47 uF = **0.35 s** (both derived, naive RC products). **That does not reproduce "short, then slow" or the Data line's "2.4 (Auto)"**: the real network (series/parallel arrangement and the other resistors) is on the unreadable schematic, so the 2.4 s cannot be rebuilt. Keep the quoted 2.4 s as the nominal Auto time and treat the two RC products as only a hint that the fast part is hundreds of ms and the slow part is a few seconds.

The attack values (0.1 to 30 ms) and fixed releases (0.1 to 1.2 s, doubling from 0.3 to 0.6 to 1.2) are quoted times; whether they are time constants or full-release times is not stated. The component values behind them are not readable.

## 5. Build notes with numbers

- Bypass caps 100 nF polyester; signal electrolytics 22 uF and 100 uF low ESR; THAT pin 4 cut for the 2180; control PCB 30 x 160 mm, main board 100 x 160 mm; the forgotten wire link next to the meter connector causes "gross distortion in both channels" (what the link does is not stated).
- Revision history: PCB rev 4 (original) to rev 9 (2006); schematic rev 5 to rev 7 (2006: unity gain 15k to 27k, ratio 100k to 127k, threshold 47k).
- Aliases: SSL Logic FXG 384, Alan Smart Research C2 Stereo Compressor, Danfield 726.

## 6. Numbers worth keeping for modelling

| Quantity | Value | Where from |
|---|---|---|
| Ratios | 2, 4, 10 : 1 only | Data |
| Attack | 0.1, 0.3, 1, 3, 10, 30 ms | Data |
| Release | 0.1, 0.3, 0.6, 1.2 s, Auto (nominal 2.4 s; two constants in the text) | Data and text |
| Detector | full-wave rectifier, 0.6 V diode threshold, **no sidechain filter mentioned** (summed by 47k from the input buffers into a "dummy" VCA, flat as far as the text says) | text |
| Topology | mostly feed-back; common sidechain for both channels | text |
| Threshold | moves with ratio "a bit" (amount unknown); range -20 to +20 dBm; threshold knob is sidechain gain | text |
| Make-up | 0 to 20 dB, applied even when bypassed in the original | text |
| Distortion | -75 dB (about 0.018%), mostly 2nd | Data and text |
| GR meter | 50 uA/dB, 1 mA = 20 dB | text |
| Frequency response | below 15 Hz to above 35 kHz (3 dB) | Data |
