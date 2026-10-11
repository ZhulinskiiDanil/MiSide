"""Makes the menu's static font from Google Fonts' variable Roboto: Bold, condensed to 80% width.

    python tools/make_font.py <Roboto[wdth,wght].ttf> resources/fonts/RobotoMenu-CondensedBold.ttf

Needs fontTools (pip install fonttools). The source is
https://github.com/google/fonts/raw/main/ofl/roboto/Roboto%5Bwdth,wght%5D.ttf (SIL OFL 1.1, see LICENSES.md).
cocos2d-x 2.2 and Geode CLI's BMFont maker read static fonts: the variable axes are fixed here once.
The family is renamed, as the OFL asks of a modified font, so it's never taken for Roboto itself.
"""

import sys

from fontTools.ttLib import TTFont
from fontTools.varLib import instancer

WEIGHT = 700
WIDTH = 80
FAMILY = "Roboto Menu Condensed"
STYLE = "Bold"


def main(source, target):
    font = TTFont(source)
    static = instancer.instantiateVariableFont(font, {"wght": WEIGHT, "wdth": WIDTH})

    names = static["name"]
    full = f"{FAMILY} {STYLE}"
    postscript = f"{FAMILY.replace(' ', '')}-{STYLE}"
    for record in list(names.names):
        if record.nameID in (1, 16):
            names.setName(FAMILY, record.nameID, record.platformID, record.platEncID, record.langID)
        elif record.nameID in (2, 17):
            names.setName(STYLE, record.nameID, record.platformID, record.platEncID, record.langID)
        elif record.nameID in (3, 4):
            names.setName(full, record.nameID, record.platformID, record.platEncID, record.langID)
        elif record.nameID == 6:
            names.setName(postscript, record.nameID, record.platformID, record.platEncID, record.langID)
        elif record.nameID == 25:
            names.removeNames(nameID=25)
    static["OS/2"].usWeightClass = WEIGHT
    static.save(target)
    print(f"{target}: {full}, wght {WEIGHT}, wdth {WIDTH}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
