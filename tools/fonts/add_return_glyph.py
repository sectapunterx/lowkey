#!/usr/bin/env python3
"""Adds U+21B5 (the return arrow, "↵") to the bundled fonts (R4-056).

Neither Golos Text nor JetBrains Mono has U+21B5, so every "↵" in a key hint
fell back to whatever font the machine had (offscreen: a plain "←", read as
"back"). This script builds the glyph inside each font from that font's own
`arrowleft` (so the stroke weight matches the hint text around it), moved down,
plus a stem of the shaft's thickness rising from its right end to cap height.
It maps it to U+21B5 as `uni21B5` and saves the file in place. A font that
already has U+21B5 is left alone, so a second run is harmless.

Re-run it whenever the fonts are regenerated or updated
(`tools/gen_bundled_fonts.py` writes them without the glyph), or the new files
silently drop it:

    pip install fonttools
    python tools/fonts/add_return_glyph.py                 # the five TTFs in resources/fonts/
    python tools/fonts/add_return_glyph.py a.ttf b.ttf     # just these files

The change makes these Modified Versions under the SIL OFL 1.1; the note at
the top of each resources/fonts/*-OFL.txt says so.
"""

import struct
import sys
from pathlib import Path

from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont, newTable

DROP = 120  # the arrow moves down (font units) so the stem has room to rise to cap height
CODEPOINT = 0x21B5
GLYPH_NAME = "uni21B5"
FONTS_DIR = Path(__file__).resolve().parent.parent.parent / "resources" / "fonts"


def area(pts):
    return sum(pts[i][0] * pts[(i + 1) % len(pts)][1] - pts[(i + 1) % len(pts)][0] * pts[i][1]
               for i in range(len(pts))) / 2


def repair_vmtx(t: TTFont, path: Path) -> bool:
    """Gives every glyph its vmtx entry; returns True when one was missing.

    fontTools copies a table it never loaded byte for byte, so the first
    version of this script grew the glyph order by one and left Golos's vmtx a
    glyph short. FreeType shrugs that off; DirectWrite (Qt on Windows) then
    draws U+21B5 as an empty advance, the bug R4-056 meant to fix.
    """
    if "vmtx" not in t:
        return False
    raw = t.reader["vmtx"]
    num_v = t["vhea"].numberOfVMetrics
    order = t.getGlyphOrder()
    if len(raw) >= num_v * 4 + (len(order) - num_v) * 2:
        return False
    metrics = {}
    for i, name in enumerate(order):
        if i < num_v:
            off = i * 4
            if off + 4 > len(raw):
                break
            adv, tsb = struct.unpack(">Hh", raw[off:off + 4])
        else:
            off = num_v * 4 + (i - num_v) * 2
            if off + 2 > len(raw):
                break
            adv = metrics[order[num_v - 1]][0]
            (tsb,) = struct.unpack(">h", raw[off:off + 2])
        metrics[name] = (adv, tsb)
    for name in order:
        if name not in metrics:
            metrics[name] = vertical_metric(t, name, metrics)
    table = newTable("vmtx")
    table.metrics = metrics
    t["vmtx"] = table
    print(path.name, "vmtx repaired")
    return True


def vertical_metric(t: TTFont, name: str, metrics=None):
    """The return arrow sits where arrowleft does, so it takes its advance and
    keeps the same gap from the top."""
    source = (metrics or t["vmtx"].metrics)["arrowleft"]
    glyf = t["glyf"]
    ref, new = glyf["arrowleft"], glyf[name]
    ref.recalcBounds(glyf)
    new.recalcBounds(glyf)
    return (source[0], source[1] + (ref.yMax - new.yMax))


def add_glyph(path: Path) -> None:
    t = TTFont(path)
    if CODEPOINT in t.getBestCmap():
        if repair_vmtx(t, path):
            t.recalcTimestamp = False
            t.save(path)
        else:
            print(path.name, "already has U+21B5")
        return
    if "vmtx" in t:
        t["vmtx"]  # load it, or the save below copies it a glyph short
    glyf = t["glyf"]
    coords, ends, _flags = glyf["arrowleft"].getCoordinates(glyf)
    pts = [(int(x), int(y) - DROP) for x, y in coords]
    xmax = max(p[0] for p in pts)
    # The shaft's right end: the two y values at x == xmax.
    ys = sorted({p[1] for p in pts if p[0] == xmax})
    lo, hi = ys[0], ys[-1]
    thick = hi - lo
    top = t["OS/2"].sCapHeight
    pen = TTGlyphPen(None)
    start = 0
    clockwise = area(pts) < 0
    for e in ends:
        c = pts[start:e + 1]
        pen.moveTo(c[0])
        for p in c[1:]:
            pen.lineTo(p)
        pen.closePath()
        start = e + 1
    rect = [(xmax - thick, lo), (xmax - thick, top), (xmax, top), (xmax, lo)]  # clockwise
    if not clockwise:
        rect.reverse()
    pen.moveTo(rect[0])
    for p in rect[1:]:
        pen.lineTo(p)
    pen.closePath()
    g = pen.glyph()
    order = t.getGlyphOrder() + [GLYPH_NAME]
    t.setGlyphOrder(order)
    glyf.glyphOrder = order
    glyf[GLYPH_NAME] = g
    g.recalcBounds(glyf)
    t["hmtx"][GLYPH_NAME] = (t["hmtx"]["arrowleft"][0], g.xMin)
    if "vmtx" in t:
        t["vmtx"][GLYPH_NAME] = vertical_metric(t, GLYPH_NAME)
    for sub in t["cmap"].tables:
        if sub.isUnicode():
            sub.cmap[CODEPOINT] = GLYPH_NAME
    t.recalcTimestamp = False  # keep head.modified: same bytes on every run (as gen_bundled_fonts.py)
    t.save(path)
    print(path.name, "added", (g.xMin, g.yMin, g.xMax, g.yMax), "stem", thick)


def main() -> None:
    paths = [Path(a) for a in sys.argv[1:]] or sorted(FONTS_DIR.glob("*.ttf"))
    if not paths:
        sys.exit(f"no fonts found in {FONTS_DIR}")
    for p in paths:
        add_glyph(p)


if __name__ == "__main__":
    main()
