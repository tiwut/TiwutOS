import struct
import os

w, h = 800, 600

has_custom = False
try:
    from PIL import Image
    if os.path.exists("custom_bg.jpg"):
        img = Image.open("custom_bg.jpg").convert("RGB").resize((w, h))
        pixels = img.load()
        has_custom = True
    elif os.path.exists("custom_bg.bmp"):
        img = Image.open("custom_bg.bmp").convert("RGB").resize((w, h))
        pixels = img.load()
        has_custom = True
    elif os.path.exists("custom_bg.png"):
        img = Image.open("custom_bg.png").convert("RGB").resize((w, h))
        pixels = img.load()
        has_custom = True
    elif os.path.exists("../v3.0/custom_bg.png"):
        img = Image.open("../v3.0/custom_bg.png").convert("RGB").resize((w, h))
        pixels = img.load()
        has_custom = True
except ImportError:
    pass

os.makedirs("isodir/boot", exist_ok=True)
with open("isodir/boot/bg.bmp", "wb") as f:
    f.write(b"BM")
    f.write(struct.pack("<I", 54 + w*h*3))
    f.write(b"\x00\x00\x00\x00")
    f.write(struct.pack("<I", 54))
    f.write(struct.pack("<I", 40))
    f.write(struct.pack("<ii", w, h))
    f.write(struct.pack("<HH", 1, 24))
    f.write(struct.pack("<I", 0))
    f.write(struct.pack("<I", w*h*3))
    f.write(struct.pack("<ii", 2835, 2835))
    f.write(struct.pack("<II", 0, 0))
    
    for y in range(h):
        for x in range(w):
            if has_custom:
                r, g, b = pixels[x, h - 1 - y]
            else:
                r = min(255, int(255 * (y / h)))
                g = min(255, int(150 * (x / w)))
                b = min(255, int(255 * ((w - x) / w)))
            f.write(bytes([b, g, r]))

logo_w, logo_h = 128, 128
has_logo = False
try:
    if os.path.exists("../v3.0/logo.png"):
        img = Image.open("../v3.0/logo.png").convert("RGBA").resize((logo_w, logo_h))
        logo_pixels = img.load()
        has_logo = True
    elif os.path.exists("logo.png"):
        img = Image.open("logo.png").convert("RGBA").resize((logo_w, logo_h))
        logo_pixels = img.load()
        has_logo = True
except: pass

with open("isodir/boot/logo.bmp", "wb") as f:
    f.write(b"BM")
    f.write(struct.pack("<I", 54 + logo_w*logo_h*4))
    f.write(b"\x00\x00\x00\x00")
    f.write(struct.pack("<I", 54))
    f.write(struct.pack("<I", 40))
    f.write(struct.pack("<ii", logo_w, logo_h))
    f.write(struct.pack("<HH", 1, 32))
    f.write(struct.pack("<I", 0))
    f.write(struct.pack("<I", logo_w*logo_h*4))
    f.write(struct.pack("<ii", 2835, 2835))
    f.write(struct.pack("<II", 0, 0))
    for y in range(logo_h):
        for x in range(logo_w):
            if has_logo:
                r, g, b, a = logo_pixels[x, logo_h - 1 - y]
                f.write(bytes([b, g, r, a]))
            else:
                dx, dy = x - logo_w/2, y - logo_h/2
                if dx*dx + dy*dy < 50*50: f.write(bytes([255, 100, 100, 255]))
                else: f.write(bytes([0, 0, 0, 0]))
