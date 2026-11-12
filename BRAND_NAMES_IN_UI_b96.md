# Real product and brand names in the plug-in's own text: status (b96)

You said brand and artist names belong only in the docs. b96 did that for everything a user can see in the plug-in: menu entries, parameter names and hover text (57 lines plus the wah, octaver, amp and tone-stack lines the first scan missed; about 100 lines in all, 117 phrase rules). The names are now only in the docs: `MANUAL.md` Appendix A2 maps every neutral name to the gear it is modelled on, and `MODELED_GEAR.md` is unchanged. Menu choices keep their position, so saved sessions and presets load the same.

## Left as they were, on purpose
- **Era and style words:** blackface, silverface, tweed, Plexi (kept in code comments only), rectifier, Whammy-style wording, "Vitriol" and "Hex" (voicing names whose origin I could not confirm as a product). Say if you want any of them changed.
- **Invented amp names that sound like products** (for example Twin Sparkle, Brit Eighteen, Tweedle Dum) are your own names; unchanged.
- **Code comments (186 lines)** and **test messages** still name products. They never reach a user; they hold provenance (which circuit a number came from). Say if you want them reworded too.
- Anything my keyword scan did not know. If you spot a name in the UI, tell me and I will add it to the rules.
