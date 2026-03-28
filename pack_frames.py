from PIL import Image
import glob

with open("bad_apple.bin", "wb") as out:
    for path in sorted(glob.glob("media/frames/frame_*.png")):
        img = Image.open(path).convert("1")
        out.write(img.tobytes())