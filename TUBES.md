# Power-tube datasheet figures (b18)
Read from the datasheets you supplied. "text" = read from the PDF text, "scan" = OCR of a scanned page (single digits may be off). Nothing here is wired into the model yet: the Power Tube menu (EL34, 6L6, 6V6, EL84, KT88) is still the hand-tuned table in PluginProcessor.cpp.

| Tube | Max plate dissipation | Other figures read | Source |
|---|---|---|---|
| EL34 (Svetlana) | 25 W | max plate 800 V, screen 500 V / 8 W, gm 11,000 uS at 250 V | text |
| 6L6GC (Svetlana) | 30 W | max plate and screen 500 V, screen 5 W, gm 5,200 uS, Rp 33k (tetrode, 350 V) | text |
| 6L6GC (GE) | 30 W | 500 V plate, screen 5 W | scan |
| 6550C (Svetlana) | 35 W | plate 680 V, screen 400 V / 6 W, 175 mA cathode, gm 9,500 uS | text |
| 6550A | 42 W | plate 660 V, cathode 190 mA, screen 6 W | scan |
| KT88 (two sheets) | 33 W | pair class AB data present; see PDFs | text / scan |
| KT77 | 32 W | designed for over 70 W from two tubes | scan |
| KT66 | 30 W | cathode 200 mA | scan |
| 6BQ5 / EL84 | 12 W | plate and screen 300 V max, 65 mA, gm 11,300 uS, Rp 38-40k | text / scan |
| 6V6GT / GTA | see PDFs | not extracted reliably | scan |
| EL37, 7189A, 7408 | see PDFs | not extracted reliably | scan |

What it says about the current table: the plate-dissipation ordering (EL84 12 W < EL34 25 W < 6L6 30 W < KT88 33 W < 6550 35-42 W) matches the headroom ordering already used (KT88 tightest, 6V6 and EL84 softest). It does not tell me the tonal differences, so those stay hand-tuned.
Not added: KT66, KT77, 6550, EL37 as menu choices. Needs your say.
