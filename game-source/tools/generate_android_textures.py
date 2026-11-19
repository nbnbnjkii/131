#!/usr/bin/env python3
"""Deterministic, seamless 512px albedo PNGs; Python standard library only.
No external images or services. Endpoints match exactly on both axes.
"""
import math
from pathlib import Path
import random
import struct
import zlib

SIZE = 512
DEST = Path(__file__).resolve().parents[1] / 'sys/android/app/src/main/assets/textures'


def noise_field(period, seed):
    rng = random.Random(seed)
    grid = [[rng.random() for _ in range(period)] for _ in range(period)]
    def sample(u, v):
        x, y = u * period, v * period
        ix, iy = math.floor(x), math.floor(y)
        fx, fy = x - ix, y - iy
        fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
        a, b = grid[iy % period][ix % period], grid[iy % period][(ix + 1) % period]
        c, d = grid[(iy + 1) % period][ix % period], grid[(iy + 1) % period][(ix + 1) % period]
        return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy
    return sample


def png(path, pixels):
    def chunk(tag, data):
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff)
    rows = b''.join(b'\0' + pixels[y * SIZE * 3:(y + 1) * SIZE * 3] for y in range(SIZE))
    path.write_bytes(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', SIZE, SIZE, 8, 2, 0, 0, 0)) + chunk(b'IDAT', zlib.compress(rows, 9)) + chunk(b'IEND', b''))


def generate():
    DEST.mkdir(parents=True, exist_ok=True)
    fields = [noise_field(p, 810 + p) for p in (4, 16, 64, 128)]
    result = {name: bytearray() for name in ('asphalt', 'concrete', 'brick', 'rust', 'wood', 'fabric', 'glass')}
    tau = math.tau
    for y in range(SIZE):
        # Identical first/last rows and columns are intentional for easy seam tests.
        v = (y % (SIZE - 1)) / (SIZE - 1)
        for x in range(SIZE):
            u = (x % (SIZE - 1)) / (SIZE - 1)
            coarse, medium, fine, grain = (f(u, v) for f in fields)
            detail = (medium - .5) * 16 + (fine - .5) * 22 + (grain - .5) * 28
            crack = min(abs(math.sin(tau * (3 * u + .12 * math.sin(tau * 2 * v) + .05 * math.sin(tau * 7 * v)))), abs(math.sin(tau * (2 * v + .16 * math.sin(tau * u)))))
            cut = 22 * max(0, 1 - crack / .045)
            asphalt = 48 + detail * .7 + coarse * 14 - cut
            concrete = 137 + detail + (coarse - .5) * 45 - cut * .9
            row = int(v * 8) % 8
            bx, by = (u * 4 + (row % 2) * .5) % 1, (v * 8) % 1
            mortar = bx < .035 or bx > .965 or by < .065 or by > .935
            brick = (97 + detail * .4, 94 + detail * .4, 84 + detail * .4) if mortar else (139 + detail*.85 + coarse*16, 69 + detail*.65, 46 + detail*.5)
            rust_weight = min(1, max(0, (coarse * .65 + medium * .35 - .36) * 3.4))
            rust = tuple(a * (1 - rust_weight) + b * rust_weight + detail * .5 for a, b in zip((65, 70, 68), (153, 72, 29)))
            # Periodic grain, plank seams and faded woven upholstery: all noise fields wrap.
            grain_line = math.sin(tau*(32*u + .15*math.sin(tau*2*v)+.06*math.sin(tau*7*v)))
            plank = 17 if (u*4)%1 < .012 else 0
            wear = detail*.25 + grain_line*7 - plank
            wood = (108+wear+coarse*20, 74+wear*.7+coarse*12, 42+wear*.4+coarse*8)
            weave = 5*math.sin(tau*64*u)*math.sin(tau*64*v) + 5*math.sin(tau*16*u)*math.sin(tau*16*v)
            fabric = (43+detail*.3+weave+(coarse-.5)*20, 56+detail*.35+weave+(coarse-.5)*25, 48+detail*.3+weave+(coarse-.5)*20)
            streak=7*math.sin(tau*(24*u+.03*math.sin(tau*v)))+detail*.18
            glass=(142+streak,177+streak,182+streak)
            for name, color in [('asphalt', (asphalt, asphalt + 2, asphalt + 3)), ('concrete', (concrete, concrete - 2, concrete - 8)), ('brick', brick), ('rust', rust), ('wood', wood), ('fabric', fabric), ('glass', glass)]:
                result[name].extend(max(0, min(255, round(c))) for c in color)
    for name, pixels in result.items():
        path = DEST / (name + '.png')
        png(path, pixels)
        print(path.relative_to(DEST.parents[6]), path.stat().st_size)


if __name__ == '__main__':
    generate()
