"""
Generates the texture images used by the Library of Floating Books (Deliverable 3).

All images are created from scratch with numpy noise and simple maths, so no
third-party texture assets are needed. Run once:
    python3 tools/make_textures.py models/textures

Outputs (PNG):
  floor_wood.png        tiling wooden planks (albedo)
  stone_albedo.png      tiling stone bricks (albedo)
  stone_normal.png      matching tangent-space normal map
  leather.png           light leather grain, tinted per book in the shader
  rug.png               round ornamental rug (decal, used once)
  sky_px/nx/py/ny/pz/nz.png   night-sky cubemap faces (stars, nebula, moon)
  font.png              16 x 6 ASCII bitmap-font atlas for the on-screen labels

Image row 0 is uploaded as texture row t = 0 by the program, so "down the
image" = +v. Normal maps are generated in that convention.
"""
import sys, os
import numpy as np
from PIL import Image, ImageDraw, ImageFont

OUT = sys.argv[1] if len(sys.argv) > 1 else "models/textures"
os.makedirs(OUT, exist_ok=True)
rng = np.random.default_rng(7)


def save(name, arr, mode="RGB"):
    arr = np.clip(arr, 0, 1)
    Image.fromarray((arr * 255 + 0.5).astype(np.uint8), mode).save(os.path.join(OUT, name))
    print("wrote", name)


def periodic_noise(h, w, scale):
    """Smooth noise that tiles seamlessly (white noise blurred in Fourier space)."""
    n = rng.standard_normal((h, w))
    fy = np.fft.fftfreq(h)[:, None]
    fx = np.fft.fftfreq(w)[None, :]
    g = np.exp(-(fx ** 2 + fy ** 2) * (scale ** 2) * 2.0)
    out = np.real(np.fft.ifft2(np.fft.fft2(n) * g))
    out -= out.min()
    return out / out.max()


def normal_from_height(hgt, strength):
    """Tangent-space normal map from a height map (wraps at the edges)."""
    du = (np.roll(hgt, -1, axis=1) - np.roll(hgt, 1, axis=1)) * 0.5
    dv = (np.roll(hgt, -1, axis=0) - np.roll(hgt, 1, axis=0)) * 0.5
    n = np.dstack([-du * strength, -dv * strength, np.ones_like(hgt)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return n * 0.5 + 0.5


# ---------------------------------------------------------------- wood floor
S = 1024
yy, xx = np.mgrid[0:S, 0:S].astype(float)
planks = 8
ph = S / planks
row = (yy // ph).astype(int)
offset = (row * 389) % S                          # each plank's boards start elsewhere
board_len = S / 2
board = (((xx + offset) % S) // board_len).astype(int)
board_id = row * 7 + board
tone = rng.uniform(0.75, 1.15, size=planks * 7 + 8)[board_id]
warp = periodic_noise(S, S, 40) * 30
grain = 0.5 + 0.5 * np.sin((yy + warp) * 0.9 + np.sin(xx * 0.01) * 3)
fine = periodic_noise(S, S, 2)
base = np.array([0.42, 0.25, 0.13])
wood = base[None, None, :] * (tone[..., None] * (0.82 + 0.18 * grain[..., None]) * (0.9 + 0.2 * fine[..., None]))
seam = ((yy % ph) < 3) | ((((xx + offset) % board_len)) < 3)
wood[seam] *= 0.35
save("floor_wood.png", wood)

# ------------------------------------------------------------- stone bricks
rows, cols = 8, 4
bh, bw = S / rows, S / cols
brow = (yy // bh).astype(int)
shift = (brow % 2) * (bw / 2)
bx = (xx + shift) % S
bcol = (bx // bw).astype(int)
lx = bx % bw
ly = yy % bh
mortar = 10.0
edge = np.minimum(np.minimum(lx, bw - lx), np.minimum(ly, bh - ly))
height = np.clip((edge - mortar * 0.5) / 18.0, 0, 1)          # bevelled edges
rough = periodic_noise(S, S, 6)
height = height * (0.85 + 0.15 * rough)
brick_tone = rng.uniform(0.75, 1.2, size=(rows, cols + 1))[brow, bcol]
stone = np.array([0.50, 0.45, 0.40])[None, None, :] * (brick_tone[..., None] * (0.8 + 0.35 * rough[..., None]))
stone[edge < mortar * 0.5] = np.array([0.22, 0.20, 0.18])
save("stone_albedo.png", stone)
save("stone_normal.png", normal_from_height(height, 6.0))

# ------------------------------------------------------------------ leather
L = 256
leather_n = periodic_noise(L, L, 1.5) * 0.6 + periodic_noise(L, L, 8) * 0.4
leather = np.dstack([0.78 + 0.22 * leather_n] * 3)
leather[..., 1] *= 0.93
leather[..., 2] *= 0.85
save("leather.png", leather)

# ---------------------------------------------------------------------- rug
yy, xx = np.mgrid[0:S, 0:S].astype(float)
u = (xx + 0.5) / S - 0.5
v = (yy + 0.5) / S - 0.5
r = np.sqrt(u * u + v * v) * 2.0          # 0 centre .. 1 edge
a = np.arctan2(v, u)
red = np.array([0.45, 0.06, 0.05])
gold = np.array([0.80, 0.58, 0.22])
navy = np.array([0.08, 0.08, 0.22])
rug = np.ones((S, S, 3)) * red
def band(r0, r1):
    return (r > r0) & (r < r1)
rug[r > 0.93] = navy
rug[band(0.89, 0.93) | band(0.60, 0.62) | band(0.30, 0.32)] = gold
diamonds = (np.abs(((a / np.pi * 24) % 2) - 1) * 0.04 + 0.66) > r
rug[band(0.66, 0.86) & (np.abs(np.cos(a * 24)) > 0.75) & (np.abs(r - 0.76) < 0.07 * np.abs(np.cos(a * 24)))] = gold * 0.9
petal = r < (0.22 + 0.08 * np.cos(a * 8))
rug[(r < 0.30) & petal] = navy
rug[(r < 0.12)] = gold
rug[band(0.35, 0.58) & (np.abs(np.sin(a * 16 + r * 20)) > 0.92)] = gold * 0.8
weave = periodic_noise(S, S, 1.2)
rug *= (0.85 + 0.2 * weave[..., None])
save("rug.png", rug)

# ------------------------------------------------------------ sky cubemap
F = 512
moon_dir = np.array([0.2, 1.0, 0.12]); moon_dir /= np.linalg.norm(moon_dir)

lat = rng.standard_normal((16, 16, 16))
def value_noise3(p):
    """Trilinear value noise on a 16^3 periodic lattice (seamless on the sphere)."""
    p = p * 4.0 + 8.0
    i = np.floor(p).astype(int); f = p - i
    f = f * f * (3 - 2 * f)
    def L(dx, dy, dz):
        return lat[(i[..., 0] + dx) % 16, (i[..., 1] + dy) % 16, (i[..., 2] + dz) % 16]
    x00 = L(0,0,0)*(1-f[...,0]) + L(1,0,0)*f[...,0]
    x10 = L(0,1,0)*(1-f[...,0]) + L(1,1,0)*f[...,0]
    x01 = L(0,0,1)*(1-f[...,0]) + L(1,0,1)*f[...,0]
    x11 = L(0,1,1)*(1-f[...,0]) + L(1,1,1)*f[...,0]
    y0 = x00*(1-f[...,1]) + x10*f[...,1]
    y1 = x01*(1-f[...,1]) + x11*f[...,1]
    return y0*(1-f[...,2]) + y1*f[...,2]

jj, ii = np.mgrid[0:F, 0:F].astype(float)
sc = 2 * (ii + 0.5) / F - 1
tc = 2 * (jj + 0.5) / F - 1
one = np.ones_like(sc)
faces = {   # OpenGL cubemap face directions
    "px": np.dstack([one, -tc, -sc]), "nx": np.dstack([-one, -tc, sc]),
    "py": np.dstack([sc, one, tc]),   "ny": np.dstack([sc, -one, -tc]),
    "pz": np.dstack([sc, -tc, one]),  "nz": np.dstack([-sc, -tc, -one]),
}
stars = rng.standard_normal((4000, 3)); stars /= np.linalg.norm(stars, axis=1, keepdims=True)
star_b = rng.uniform(0.3, 1.0, 4000) ** 3
for name, d in faces.items():
    d = d / np.linalg.norm(d, axis=2, keepdims=True)
    elev = d[..., 1]
    sky = np.dstack([0.01 + 0.03 * (elev + 1), 0.015 + 0.05 * (elev + 1), 0.06 + 0.10 * (elev + 1)])
    neb = np.clip(value_noise3(d) * 0.5 + value_noise3(d * 2.3) * 0.3, 0, None)
    sky += neb[..., None] * np.array([0.10, 0.04, 0.16])
    # stars: brightness from the nearest bright star directions on this face
    flat = d.reshape(-1, 3)
    img = sky.reshape(-1, 3)
    for k in range(0, 4000, 500):
        dots = flat @ stars[k:k+500].T                      # cos angle to each star
        glow = np.exp(-(1 - dots) * 2.5e5) * star_b[k:k+500]   # tiny gaussian dots
        img += glow.sum(axis=1)[:, None] * np.array([1.0, 0.95, 0.85])
    # moon
    m = flat @ moon_dir
    img += (np.exp(-(1 - m) * 900)[:, None] * np.array([1.0, 0.97, 0.88]) * 1.6
            + np.exp(-(1 - m) * 60)[:, None] * np.array([0.25, 0.3, 0.45]) * 0.6)
    save(f"sky_{name}.png", img.reshape(F, F, 3))

# --------------------------------------------------------------- font atlas
CW, CH, COLS = 32, 64, 16
atlas = Image.new("RGBA", (CW * COLS, CH * 6), (0, 0, 0, 0))
draw = ImageDraw.Draw(atlas)
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf", 46)
for c in range(32, 128):
    k = c - 32
    x, y = (k % COLS) * CW, (k // COLS) * CH
    draw.text((x + 2, y + 4), chr(c), font=font, fill=(255, 255, 255, 255))
atlas.save(os.path.join(OUT, "font.png"))
print("wrote font.png")
