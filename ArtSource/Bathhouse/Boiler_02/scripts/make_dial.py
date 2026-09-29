"""Generates the 0-4 bar pressure gauge dial texture (source texture for the bake)."""
import math, random, sys
from PIL import Image, ImageDraw, ImageFont, ImageFilter

S = 1024
out = sys.argv[1] if len(sys.argv) > 1 else "T_Boiler02_Dial_Src.png"
img = Image.new("RGB", (S, S), (236, 226, 198))
d = ImageDraw.Draw(img)
c = S / 2

# subtle aged vignette
vig = Image.new("L", (S, S), 0)
vd = ImageDraw.Draw(vig)
for i in range(60):
    r = c - i * 2
    vd.ellipse([c - r, c - r, c + r, c + r], outline=int(90 * (1 - i / 60) ** 2), width=3)
vig = vig.filter(ImageFilter.GaussianBlur(18))
img.paste((120, 100, 70), mask=vig)

# stains
rnd = random.Random(7)
stain = Image.new("L", (S, S), 0)
sd = ImageDraw.Draw(stain)
for _ in range(40):
    x, y, r = rnd.uniform(0, S), rnd.uniform(0, S), rnd.uniform(10, 70)
    sd.ellipse([x - r, y - r, x + r, y + r], fill=rnd.randint(8, 30))
stain = stain.filter(ImageFilter.GaussianBlur(25))
img.paste((150, 125, 80), mask=stain)
d = ImageDraw.Draw(img)

# outer black ring
d.ellipse([6, 6, S - 6, S - 6], outline=(25, 22, 20), width=14)

A0, A1 = 225.0, -45.0  # value 0 at lower-left, 4 at lower-right (clockwise sweep)
def ang(v):
    return math.radians(A0 + (A1 - A0) * v / 4.0)
def pt(a, r):
    return (c + r * math.cos(a), c - r * math.sin(a))

# red zone 3..4
for k in range(120):
    v = 3 + k / 119
    a = ang(v)
    d.line([pt(a, 400), pt(a, 440)], fill=(170, 30, 25), width=6)

# ticks
for i in range(0, 41):
    v = i / 10
    a = ang(v)
    if i % 10 == 0:
        d.line([pt(a, 360), pt(a, 445)], fill=(20, 18, 16), width=12)
    elif i % 5 == 0:
        d.line([pt(a, 390), pt(a, 445)], fill=(20, 18, 16), width=7)
    else:
        d.line([pt(a, 415), pt(a, 445)], fill=(30, 28, 25), width=4)
# arc line
d.arc([c - 445, c - 445, c + 445, c + 445], start=-A0, end=-A1, fill=(20, 18, 16), width=5)

font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf", 108)
small = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSerif-Bold.ttf", 78)
for v in range(5):
    x, y = pt(ang(v), 275)
    d.text((x, y), str(v), font=font, fill=(18, 16, 14), anchor="mm")
d.text((c, c + 215), "bar", font=small, fill=(18, 16, 14), anchor="mm")

# needle (red, pointing ~2.35 bar) with dark tail
a = ang(2.35)
tip = pt(a, 420)
tail = pt(a + math.pi, 90)
perp = a + math.pi / 2
w = 16
poly = [tip, (c + w * math.cos(perp), c - w * math.sin(perp)), tail,
        (c - w * math.cos(perp), c + w * math.sin(perp))]
d.polygon(poly, fill=(175, 25, 20))
d.ellipse([c - 42, c - 42, c + 42, c + 42], fill=(30, 26, 22))

img = img.filter(ImageFilter.GaussianBlur(0.8))
img.save(out)
print("saved", out)
