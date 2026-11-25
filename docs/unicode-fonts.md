# Extended text glyphs

The development firmware supplements each selected font with a same-size Lato
fallback. It includes all **2,068 non-ASCII BMP mappings** available in the
bundled [source font](../main/fonts/sources/Lato-Regular.ttf), at 8, 10, 12, 14,
16, 20, 24, 28, 36 and 40 pixels.

This covers Æ/æ, Œ/œ, ß, extended accented Latin, Greek and Cyrillic, plus the
punctuation, currency, arrows, mathematical and other symbols that Lato supplies.
It is **not every Unicode character**. Unsupported CJK, emoji and other scripts
need different font/image assets. Glyph availability does not supply complex
script shaping or guarantee combining-mark positioning.

Fallback glyphs use compressed **1-bpp monochrome** bitmaps to fit the existing
4 MiB OTA slots. Characters already present in a selected font retain its
original appearance and antialiasing. Unsupported characters can still show a
placeholder. The fonts are flash-resident; an entire expanded bitmap font is
not loaded into PSRAM.

## Reproduction and checks

Use Node and an installed `lv_font_conv` **1.5.3**:

```powershell
.\tools\make_unicode_fonts.ps1 -ConverterPath C:\font-tools\node_modules\lv_font_conv\lv_font_conv.js
node tests\test_unicode_fonts.js C:\font-tools\node_modules\lv_font_conv\lv_font_conv.js
```

The generator includes only glyphs actually supplied by the font within
U+0080-U+FFFF. The regression checks every source mapping and generated glyph
index at all ten sizes, rather than testing Æ alone. Firmware startup checks
representative Latin/ligature, Vietnamese, Greek, Cyrillic, euro and em-dash
glyph resolution through all 68 selectable font variants.

Run the production `reconfigure build`, verify app-slot fit, then follow the
[development/recovery guide](development.md) before deployment. The expanded
build has only about 31 KiB of slot headroom; do not erase or resize live
partitions to accommodate another font expansion.

The original font and generated subsets are distributed under the
[SIL Open Font License](../main/fonts/licenses/lato.txt). The subset names are
GooPad Unicode; preserve the source's copyright and license when redistributing.

## Delivery status

The font assets, source, generator and coverage regression can be committed
independently of the earlier accumulated application worktree. The font
selection/startup integration and a matching binary release still require that
earlier source backlog to be reconciled; an asset commit alone is not a released
firmware build.
