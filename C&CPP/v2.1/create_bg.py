import struct
import os

w, h = 1280, 720

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
except ImportError:
    print("Warning: Pillow (PIL) is not installed. Will not convert custom images.")

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

if has_custom:
    print("Custom image converted and BMP background generated!")
else:
    print("Default BMP background generated!")
