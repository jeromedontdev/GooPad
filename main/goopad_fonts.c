#include "goopad_fonts.h"
#include "goopad_styled.h"
#include "goopad_unicode.h"

#include <string.h>

LV_FONT_DECLARE(goopad_font_essential_8);
LV_FONT_DECLARE(goopad_font_essential_10);
LV_FONT_DECLARE(goopad_font_essential_12);
LV_FONT_DECLARE(goopad_font_essential_14);
LV_FONT_DECLARE(goopad_font_essential_16);
LV_FONT_DECLARE(goopad_font_essential_20);
LV_FONT_DECLARE(goopad_font_essential_24);
LV_FONT_DECLARE(goopad_font_essential_28);
LV_FONT_DECLARE(goopad_font_essential_36);
LV_FONT_DECLARE(goopad_font_essential_40);

static const char *const names[] = {
    "montserrat", "lato", "pacifico", "anton", "archivoblack",
    "abrilfatface", "lobster", "breeserif", "cousine", "sharetechmono", "righteous",
    "acme", "bangers", "patrickhand", "pressstart2p", "spacemono"
};
static const lv_font_t *const montserrat[] = {
    &lv_font_montserrat_40, &lv_font_montserrat_28, &lv_font_montserrat_20,
    &lv_font_montserrat_16, &lv_font_montserrat_14, &lv_font_montserrat_12,
    &lv_font_montserrat_10, &lv_font_montserrat_8
};

int goopad_font_index(const char *name)
{
    for (unsigned i = 0; i < sizeof(names) / sizeof(names[0]); i++)
        if (name && !strcmp(name, names[i])) return (int)i;
    return -1;
}

unsigned goopad_font_variants(unsigned family)
{
    return family == 0 ? sizeof(montserrat) / sizeof(montserrat[0]) : 4;
}

const lv_font_t *goopad_font_variant(unsigned family, unsigned variant, unsigned *size)
{
    static const unsigned default_sizes[] = {40, 28, 20, 16, 14, 12, 10, 8};
    if (family >= sizeof(names) / sizeof(names[0]) || variant >= goopad_font_variants(family)) return NULL;
    static lv_font_t extended[8];
    static const unsigned unicode_sizes[] = {8, 10, 12, 14, 16, 20, 24, 28, 36, 40};
    static const lv_font_t *const unicode[] = {
        &goopad_font_essential_8, &goopad_font_essential_10, &goopad_font_essential_12,
        &goopad_font_essential_14, &goopad_font_essential_16, &goopad_font_essential_20,
        &goopad_font_essential_24, &goopad_font_essential_28, &goopad_font_essential_36,
        &goopad_font_essential_40
    };
    *size = family == 0 ? default_sizes[variant] : (variant == 0 ? 36 : variant == 1 ? 24 : variant == 2 ? 16 : 12);
    if (family) {
        static const lv_font_t *const styled_fallbacks[] = {
            &goopad_font_essential_36, &goopad_font_essential_24,
            &goopad_font_essential_16, &goopad_font_essential_12
        };
        const lv_font_t *fallback = goopad_unicode_font(*size, styled_fallbacks[variant]);
        return goopad_styled_font(family - 1, variant, fallback);
    }
    lv_font_t *font = &extended[variant];
    if (!font->get_glyph_dsc) {
        *font = *montserrat[variant];
        for (unsigned i = 0; i < sizeof(unicode_sizes) / sizeof(unicode_sizes[0]); i++)
            if (unicode_sizes[i] == *size) font->fallback = goopad_unicode_font(*size, unicode[i]);
    }
    return font;
}
