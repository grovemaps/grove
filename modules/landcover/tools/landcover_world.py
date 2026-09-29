#!/usr/bin/env python3
"""Builds Grove's bundled world land cover, for the map zoomed out further than zoom 7.

    modules/landcover/tools/landcover_world.py [--workers 16]

Zoomed out, a map tile spans too many of ESA WorldCover's 3-degree files to read them online
(modules/landcover/map/landcover.cpp does that from zoom 7). This reads each file's coarsest overview (600 m
pixels, one TIFF tile) from WorldCover on AWS Open Data, mosaics them at 0.025 degrees, and cuts web
mercator tiles for Organic Maps zooms 1 to 6 (web mercator zooms 0 to 5). Tiles are 8-bit PNGs of
WorldCover classes, not colours, so the app colours them for the light and dark styles; tiles without
land are left out. Writes data/grove_landcover_world.bin (the PNGs) and data/grove_landcover_world.txt
(the index). Downloads (about 2,600 files' headers and overviews) are cached in
build-grove/landcover-cache.

ESA WorldCover 2021 v200, CC BY 4.0: (c) ESA WorldCover project 2021, contains modified Copernicus
Sentinel data (2021) processed by ESA WorldCover consortium.
"""

import argparse
import concurrent.futures
import io
import math
import os
import struct
import sys
import urllib.request
import xml.etree.ElementTree as ET
import zlib

import numpy as np
from PIL import Image

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
CACHE = os.path.join(ROOT, "build-grove", "landcover-cache")
BUCKET = "https://esa-worldcover.s3.eu-central-1.amazonaws.com/"
PREFIX = "v200/2021/map/"
HEAD_SIZE = 32 * 1024

# Mosaic: 40 pixels a degree, from 84 N (WorldCover's northern limit) to 60 S.
PIXELS_PER_DEGREE = 40
NORTH, SOUTH = 84, -60
MAX_WEB_MERCATOR_ZOOM = 5
# Water up to this many mosaic pixels (0.2 degrees) from land is filled with the land's class.
FILL_PIXELS = 8
TILE = 256


def fetch(url, byte_range=None):
    request = urllib.request.Request(url)
    if byte_range:
        request.add_header("Range", f"bytes={byte_range[0]}-{byte_range[1]}")
    for attempt in range(5):
        try:
            with urllib.request.urlopen(request, timeout=60) as response:
                return response.read()
        except Exception as e:  # noqa: BLE001 -- retried, then raised
            if attempt == 4:
                raise
            print(f"retry {url}: {e}", file=sys.stderr)


def list_files():
    names, token = [], None
    while True:
        url = BUCKET + f"?list-type=2&prefix={PREFIX}&max-keys=1000"
        if token:
            url += "&continuation-token=" + urllib.request.quote(token, safe="")
        root = ET.fromstring(fetch(url))
        ns = {"s3": root.tag.split("}")[0].strip("{")}
        names += [k.text for k in root.findall("s3:Contents/s3:Key", ns) if k.text.endswith("_Map.tif")]
        token_el = root.find("s3:NextContinuationToken", ns)
        if token_el is None:
            return names
        token = token_el.text


def last_level(head, read_more):
    """(width, height, tile width, tile height, offset, size) of the coarsest level of a classic TIFF."""
    ifd, level = struct.unpack_from("<I", head, 4)[0], None
    while ifd:
        count = struct.unpack_from("<H", head, ifd)[0]
        tags = {}
        for i in range(count):
            tag, typ, n, value = struct.unpack_from("<HHII", head, ifd + 2 + i * 12)
            if typ == 3 and n == 1:
                value &= 0xFFFF
            tags[tag] = (typ, n, value)
        def array(tag):
            typ, n, value = tags[tag]
            size = 2 if typ == 3 else 4
            if n * size <= 4:
                return [value]
            raw = head[value : value + n * size] if value + n * size <= len(head) else read_more(value, n * size)
            return list(struct.unpack_from("<" + ("H" if size == 2 else "I") * n, raw))
        level = (tags[256][2], tags[257][2], tags[322][2], tags[323][2], array(324), array(325))
        ifd = struct.unpack_from("<I", head, ifd + 2 + count * 12)[0]
    return level


def overview(key):
    """The file's coarsest overview as a 2D array of classes, and its square's south-west corner."""
    name = key.rsplit("_", 2)[-2]  # e.g. N51E003
    lat = int(name[1:3]) * (1 if name[0] == "N" else -1)
    lon = int(name[4:7]) * (1 if name[3] == "E" else -1)
    path = os.path.join(CACHE, name + ".npy")
    if os.path.exists(path):
        return lat, lon, np.load(path)

    url = BUCKET + key
    head = fetch(url, (0, HEAD_SIZE - 1))
    width, height, tile_w, tile_h, offsets, sizes = last_level(
        head, lambda offset, size: fetch(url, (offset, offset + size - 1)))
    across = (width + tile_w - 1) // tile_w
    pixels = np.zeros((height, width), np.uint8)
    for i, (offset, size) in enumerate(zip(offsets, sizes)):
        tile = np.frombuffer(zlib.decompress(fetch(url, (offset, offset + size - 1))), np.uint8)
        tile = tile.reshape(tile_h, tile_w)
        row, col = (i // across) * tile_h, (i % across) * tile_w
        h, w = min(tile_h, height - row), min(tile_w, width - col)
        pixels[row : row + h, col : col + w] = tile[:h, :w]
    np.save(path, pixels)
    return lat, lon, pixels


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--workers", type=int, default=16)
    args = parser.parse_args()
    os.makedirs(CACHE, exist_ok=True)

    keys = list_files()
    print(f"WorldCover files: {len(keys)}", file=sys.stderr)

    mosaic = np.zeros(((NORTH - SOUTH) * PIXELS_PER_DEGREE, 360 * PIXELS_PER_DEGREE), np.uint8)
    square = 3 * PIXELS_PER_DEGREE
    with concurrent.futures.ThreadPoolExecutor(args.workers) as pool:
        for done, (lat, lon, pixels) in enumerate(pool.map(overview, keys), 1):
            # Nearest-neighbour down to the mosaic's resolution; rows run north to south.
            rows = (np.arange(square) * pixels.shape[0] / square).astype(int)
            cols = (np.arange(square) * pixels.shape[1] / square).astype(int)
            top = (NORTH - (lat + 3)) * PIXELS_PER_DEGREE
            left = (lon + 180) * PIXELS_PER_DEGREE
            if 0 <= top and top + square <= mosaic.shape[0]:
                mosaic[top : top + square, left : left + square] = pixels[np.ix_(rows, cols)]
            if done % 200 == 0:
                print(f"files: {done}/{len(keys)}", file=sys.stderr)

    # Zoomed out, the map draws only large lakes, so small ones would show as background-coloured dots: take the
    # neighbouring land's class instead. The sea and the lakes the map draws cover what bleeds under them.
    for _ in range(FILL_PIXELS):
        empty = (mosaic == 0) | (mosaic == 80)
        if not empty.any():
            break
        for dy, dx in ((0, 1), (0, -1), (1, 0), (-1, 0)):
            shifted = np.roll(mosaic, (dy, dx), axis=(0, 1))
            take = empty & (shifted != 0) & (shifted != 80)
            mosaic[take] = shifted[take]
            empty &= ~take

    index = ["# Grove world land cover, generated by modules/landcover/tools/landcover_world.py. Do not edit.",
             "# web mercator zoom<TAB>x<TAB>y<TAB>offset<TAB>size; tiles are 8-bit PNGs of ESA WorldCover classes"]
    with open(os.path.join(ROOT, "data", "grove_landcover_world.bin"), "wb") as pack:
        for z in range(MAX_WEB_MERCATOR_ZOOM + 1):
            n = 1 << z
            size = n * TILE
            # Latitude and longitude of each pixel column and row of the whole zoom level.
            lons = (np.arange(size) + 0.5) / size * 360 - 180
            ys = math.pi * (1 - 2 * (np.arange(size) + 0.5) / size)
            lats = np.degrees(np.arctan(np.sinh(ys)))
            cols = np.clip(((lons + 180) * PIXELS_PER_DEGREE).astype(int), 0, mosaic.shape[1] - 1)
            rows = ((NORTH - lats) * PIXELS_PER_DEGREE).astype(int)
            inside = (rows >= 0) & (rows < mosaic.shape[0])
            rows = np.clip(rows, 0, mosaic.shape[0] - 1)
            for ty in range(n):
                r = rows[ty * TILE : (ty + 1) * TILE]
                ok = inside[ty * TILE : (ty + 1) * TILE]
                if not ok.any():
                    continue
                for tx in range(n):
                    tile = mosaic[np.ix_(r, cols[tx * TILE : (tx + 1) * TILE])]
                    tile[~ok, :] = 0
                    if not ((tile != 0) & (tile != 80)).any():  # No land: sea, lakes, no data.
                        continue
                    png = io.BytesIO()
                    Image.fromarray(tile, "L").save(png, "PNG", optimize=True)
                    index.append(f"{z}\t{tx}\t{ty}\t{pack.tell()}\t{png.tell()}")
                    pack.write(png.getvalue())
            print(f"zoom {z}: {len(index) - 2} tiles so far", file=sys.stderr)
    with open(os.path.join(ROOT, "data", "grove_landcover_world.txt"), "w") as f:
        f.write("\n".join(index) + "\n")


if __name__ == "__main__":
    main()
