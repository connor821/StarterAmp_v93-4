# Sennheiser MD409 family test (Xaudia blog, June 2022) - condensed notes

Source: https://xaudiaelektrik.blogspot.com/2022/06/testing-sennheiser-md409-family.html . Read through the web fetch tool; the page's plots are not filed, so there are no curves here, only the author's description. Distance is not stated and proximity is not discussed.

- Tested: five MD409-U3, one BF509, one e609, one 402/3 capsule in a replica body. Swept sine in a booth.
- MD409: four of five very close, the fifth with slightly less bass. Signature is a presence bump of about 5 dB around 100 to 200 Hz with a dip below 100 Hz. No transformer (capsule straight to XLR); apart from the capsule its parts are shared with the e609.
- BF509: same as the MD409 within unit-to-unit spread.
- e609: different from the 409. Broader low bump, a broad lift near 5 kHz, earlier high-frequency roll-off. The author says a touch of EQ makes it a good stand-in. (Matches the e609 datasheet filed here: about +8 dB at 4 kHz and a 16 kHz fall-off.)
- 402/3 capsule: very flat, no low bump; not a match.
- Nothing on guitar cabs.

Use: only a rough guide for a 409-type voicing; the 409 data point is the 100 to 200 Hz bump and the dip below 100 Hz.

## The plot (sent by you, filed as `MD409_blog_response_plot.png`)
1/12-octave response of the MD409 units (several traces, distance unknown). Read by eye, dB re the 1 kHz level (about -11.5 on the plot's axis), +/-1 dB:
50 Hz -3, 70 Hz -4.5, 100 Hz +1, 120 Hz +4 (peak, +3.5 to +4 from 120 to 150 Hz), 200 Hz +1.5, 300 Hz to 1 kHz about 0, 3.5 kHz +1.8, 5 to 6 kHz -0.5, 10 kHz +1, 12 kHz +5 (narrow peak), 14 kHz +4, 16 kHz -3.5, 20 kHz -12.5. One trace (the unit with less bass) is about 5 dB lower at 50 Hz and 3 dB lower at 100 Hz.
Added to `MIC_CURVES.csv` as "MD409 (blog plot)". Difference from the SM57 sheet curve per band (50-100, 100-300, 3-6k, 6-10k, 10-16k): +4.5, +3.6, -3.5, -3.9, +2.9. So against a 57 it is a low-mid-bump mic (+4 dB at 100 to 150 Hz, the "5 dB bump" the author describes), flat through the mids, and lacks the 57's 4 to 6 kHz presence. The 12 kHz peak is narrow and may be capsule or measurement behaviour.
