# ESP32-S3 Notes

## Font/Language TODO
- Current state: Arabic renders when Arabic font is active; English/Chinese/Korean render when their matching/default font is active.
- Open issue: mixed-language lines are not fully solved yet (example: UI language Arabic while learning language Korean in the same on-screen text).
- Target: implement font fallback per script in one label/text flow so Arabic + Korean can appear at the same time without blank glyphs.
