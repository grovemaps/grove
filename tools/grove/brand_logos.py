#!/usr/bin/env python3
"""Builds Grove's brand logo pack: data/grove_brands.txt (index) and data/grove_brands.bin (badge PNGs).

Brands come from the Name Suggestion Index (names, countries), ranked by how many OpenStreetMap places carry
their brand:wikidata tag (taginfo). Logos come from Wikidata ("small logo or icon", "icon", then "logo image"),
rendered to PNG by Wikimedia Commons, which only hosts free or public-domain files. Each logo is set on a
rounded badge; the renderer (libs/drape/grove_brand_texture.cpp) scales it to the screen density.

    tools/grove/brand_logos.py [--min-count 25] [--limit 0]

Downloads are cached in build-grove/brands-cache, so reruns only fetch what is new.
"""

import argparse
import concurrent.futures
import io
import json
import os
import re
import sys
import time
import urllib.parse
import urllib.request

from PIL import Image, ImageFilter

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CACHE = os.path.join(ROOT, "build-grove", "brands-cache")
USER_AGENT = "GroveMaps/0.1 (https://github.com/grovemaps/grove; map brand icons)"
NSI_URL = "https://cdn.jsdelivr.net/npm/name-suggestion-index@latest/dist/nsi.min.json"
TAGINFO_URL = "https://taginfo.openstreetmap.org/api/4/key/values?key=brand:wikidata&sortname=count&sortorder=desc"
SPARQL_URL = "https://query.wikidata.org/sparql"
COMMONS_API = "https://commons.wikimedia.org/w/api.php"
# A standard Wikimedia thumbnail width (https://w.wiki/GHai): usually already rendered, so Commons doesn't throttle
# it. Twice the badge, so downscaling keeps edges clean.
THUMB_WIDTH = 500
# Thumbnails of each width get their own folder and URL list.
PNG_DIR = f"png{THUMB_WIDTH}"

# Symbol source size: 32 dp at 6x screens; the renderer scales it down to 32 dp at the screen density.
BADGE = 192
# The logo fills most of it; the rest is room for the halo.
LOGO_BOX = 172
WIDE_LOGO_WIDTH = 176
HALO_RADIUS = 6
# Wider logos are wordmarks, unreadable at icon size: those brands keep their category icons.
MAX_ASPECT = 1.8
# Wikidata properties in order of preference: small logo or icon, icon, logo image.
LOGO_PROPERTIES = ["P8972", "P2910", "P154"]


def fetch(url, retries=30):
    for attempt in range(retries):
        try:
            request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
            with urllib.request.urlopen(request, timeout=60) as response:
                return response.read()
        except Exception as e:  # noqa: BLE001 -- network errors of any kind are retried
            if attempt + 1 == retries:
                raise
            # Wikimedia throttles bursts (HTTP 429) and says how long to wait.
            retry_after = getattr(e, "headers", None) and e.headers.get("Retry-After")
            time.sleep(int(retry_after) + 1 if retry_after and retry_after.isdigit() else min(60, 3 * 2 ** attempt))
            print(f"retry {url[:100]}: {e}", file=sys.stderr)


def cached_json(name, url):
    path = os.path.join(CACHE, name)
    if not os.path.exists(path):
        with open(path, "wb") as f:
            f.write(fetch(url))
    with open(path, encoding="utf-8") as f:
        return json.load(f)


def brand_counts():
    counts = {}
    page = 1
    while True:
        data = cached_json(f"taginfo-{page}.json", f"{TAGINFO_URL}&rp=999&page={page}")["data"]
        for row in data:
            counts[row["value"]] = row["count"]
        if len(data) < 999:
            return counts
        page += 1


def nsi_brands():
    """Returns {qid: {"names": set, "countries": set, "categories": set}} from the Name Suggestion Index.
    Categories are Organic Maps type names like "shop-supermarket"."""
    brands = {}
    for key, value in cached_json("nsi.json", NSI_URL)["nsi"].items():
        if not key.startswith("brands/"):
            continue
        category = "-".join(key.split("/")[1:3])
        for item in value.get("items", []):
            tags = item.get("tags", {})
            qid = tags.get("brand:wikidata")
            if not qid:
                continue
            brand = brands.setdefault(qid, {"names": set(), "countries": set(), "categories": set()})
            brand["categories"].add(category)
            for tag, name in tags.items():
                if tag == "brand" or tag.startswith("brand:") and tag != "brand:wikidata" or tag == "name":
                    brand["names"].add(name)
            for code in item.get("locationSet", {}).get("include", []):
                # Country codes only; continents and custom areas mean "anywhere".
                brand["countries"].add(code.upper() if isinstance(code, str) and re.fullmatch(r"[a-z]{2}", code) else "*")
    return brands


def logo_files(qids):
    """Returns {qid: Commons file URL} for the preferred logo property of each brand."""
    path = os.path.join(CACHE, "logos.json")
    logos = json.load(open(path)) if os.path.exists(path) else {}
    todo = [q for q in qids if q not in logos]
    for i in range(0, len(todo), 250):
        batch = todo[i : i + 250]
        props = " ".join(f"wdt:{p}" for p in LOGO_PROPERTIES)
        query = f"SELECT ?item ?prop ?file WHERE {{ VALUES ?item {{ {' '.join('wd:' + q for q in batch)} }} " \
                f"VALUES ?prop {{ {props} }} ?item ?prop ?file }}"
        url = SPARQL_URL + "?" + urllib.parse.urlencode({"query": query, "format": "json"})
        found = {}
        for row in json.loads(fetch(url))["results"]["bindings"]:
            qid = row["item"]["value"].rsplit("/", 1)[1]
            prop = row["prop"]["value"].rsplit("/", 1)[1]
            found.setdefault(qid, {})[prop] = row["file"]["value"]
        for qid in batch:
            options = found.get(qid, {})
            logos[qid] = next((options[p] for p in LOGO_PROPERTIES if p in options), "")
        json.dump(logos, open(path, "w"))
        print(f"wikidata: {min(i + 250, len(todo))}/{len(todo)}", file=sys.stderr)
        time.sleep(1)
    return {q: logos[q] for q in qids if logos.get(q)}


def thumbnail_urls(logos):
    """Returns {qid: thumbnail URL}: Commons renders any format (SVG included) to PNG thumbnails."""
    path = os.path.join(CACHE, f"thumbs{THUMB_WIDTH}.json")
    thumbs = json.load(open(path)) if os.path.exists(path) else {}
    todo = [q for q in logos if q not in thumbs and not os.path.exists(os.path.join(CACHE, PNG_DIR, q + ".png"))]
    for i in range(0, len(todo), 50):
        batch = todo[i : i + 50]
        titles = {q: "File:" + urllib.parse.unquote(logos[q].rsplit("/", 1)[1]) for q in batch}
        query = {"action": "query", "format": "json", "prop": "imageinfo", "iiprop": "url",
                 "iiurlwidth": THUMB_WIDTH, "titles": "|".join(titles.values())}
        data = json.loads(fetch(COMMONS_API + "?" + urllib.parse.urlencode(query)))["query"]
        normalized = {n["from"]: n["to"] for n in data.get("normalized", [])}
        by_title = {p["title"]: p for p in data.get("pages", {}).values()}
        for qid, title in titles.items():
            info = by_title.get(normalized.get(title, title), {}).get("imageinfo", [{}])[0]
            thumbs[qid] = info.get("thumburl") or info.get("url") or ""
        json.dump(thumbs, open(path, "w"))
        print(f"commons: {min(i + 50, len(todo))}/{len(todo)}", file=sys.stderr)
        time.sleep(2)
    return thumbs


def logo_image(qid, thumb_url):
    path = os.path.join(CACHE, PNG_DIR, qid + ".png")
    if not os.path.exists(path):
        if not thumb_url:
            raise ValueError("no thumbnail")
        data = fetch(thumb_url)
        time.sleep(1)  # One request a second: Wikimedia blocks faster clients for minutes.
        with open(path, "wb") as f:
            f.write(data)
        downloaded = len(os.listdir(os.path.dirname(path)))
        if downloaded % 100 == 0:
            print(f"logos downloaded: {downloaded}", file=sys.stderr)
    return Image.open(path).convert("RGBA")


def brand_color(logo):
    """The logo's main colour as "RRGGBB": the most common clearly coloured pixel, or "" for grey logos."""
    counts = {}
    rgba = logo.convert("RGBA").resize((48, 48)).tobytes()
    for i in range(0, len(rgba), 4):
        r, g, b, a = rgba[i : i + 4]
        if a < 200 or max(r, g, b) - min(r, g, b) < 60:
            continue  # Transparent, white, black or grey.
        key = (r // 16, g // 16, b // 16)
        counts[key] = counts.get(key, 0) + 1
    if not counts:
        return ""
    r, g, b = max(counts, key=counts.get)
    return f"{r * 16 + 8:02X}{g * 16 + 8:02X}{b * 16 + 8:02X}"


def make_badge(logo, source_url):
    """Returns the badge PNG, or None for a logo too wide to read at icon size or a photo."""
    # Some Wikidata "logos" are photos of shop signs: JPEGs, opaque everywhere and full of colours.
    photo_format = source_url.lower().rsplit(".", 1)[-1] in ("jpg", "jpeg", "tif", "tiff")
    if photo_format and len(logo.getcolors(1 << 16) or range(1 << 16)) > 4000:
        return None
    bbox = logo.getchannel("A").point(lambda a: 255 if a > 8 else 0).getbbox()
    if bbox:
        logo = logo.crop(bbox)
    if max(logo.width / logo.height, logo.height / logo.width) > MAX_ASPECT:
        return None
    box_width = WIDE_LOGO_WIDTH if logo.width > 2 * logo.height else LOGO_BOX
    scale = min(box_width / logo.width, LOGO_BOX / logo.height)
    logo = logo.resize((max(1, round(logo.width * scale)), max(1, round(logo.height * scale))), Image.LANCZOS)

    # The logo itself, transparent where it is, with a soft halo like map labels have, so it reads over roads,
    # buildings and water. Logos drawn in white get a dark halo.
    rgba = logo.tobytes()
    pixels = [rgba[i : i + 4] for i in range(0, len(rgba), 4) if rgba[i + 3] > 128]
    light = pixels and sum(0.3 * p[0] + 0.59 * p[1] + 0.11 * p[2] for p in pixels) / len(pixels) > 225
    halo_color = (58, 58, 60) if light else (255, 255, 255)

    badge = Image.new("RGBA", (BADGE, BADGE), (0, 0, 0, 0))
    badge.alpha_composite(logo, ((BADGE - logo.width) // 2, (BADGE - logo.height) // 2))
    halo_alpha = badge.getchannel("A").filter(ImageFilter.MaxFilter(2 * HALO_RADIUS + 1))
    halo_alpha = halo_alpha.filter(ImageFilter.GaussianBlur(2.4)).point(lambda a: a * 230 // 255)
    halo = Image.new("RGBA", badge.size, halo_color + (0,))
    halo.putalpha(halo_alpha)
    halo.alpha_composite(badge)
    badge = halo
    out = io.BytesIO()
    badge.save(out, "PNG", optimize=True)
    return out.getvalue()


def om_countries():
    """Returns {ISO code: Organic Maps country names}; map files are named "<country>_<region>"."""
    meta = json.load(open(os.path.join(ROOT, "data", "countries_meta.txt"), encoding="utf-8"))
    names = {}
    for name, info in meta.items():
        iso = info.get("iso3166-1", {}).get("alpha-2")
        if iso and "_" not in name:
            names.setdefault(iso, []).append(name)
    return names


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-count", type=int, default=25, help="minimum OSM places with the brand")
    parser.add_argument("--limit", type=int, default=0, help="at most this many brands (0: no limit)")
    parser.add_argument("--cached-only", action="store_true", help="only use logos already downloaded")
    args = parser.parse_args()
    os.makedirs(os.path.join(CACHE, PNG_DIR), exist_ok=True)

    counts = brand_counts()
    brands = nsi_brands()
    qids = sorted((q for q in brands if counts.get(q, 0) >= args.min_count), key=lambda q: -counts[q])
    if args.limit:
        qids = qids[: args.limit]
    print(f"brands with >= {args.min_count} places: {len(qids)}", file=sys.stderr)

    logos = logo_files(qids)
    print(f"with a logo on Commons: {len(logos)}", file=sys.stderr)

    if args.cached_only:
        logos = {q: f for q, f in logos.items() if os.path.exists(os.path.join(CACHE, PNG_DIR, q + ".png"))}
    thumbs = {} if args.cached_only else thumbnail_urls(logos)

    def build(qid):
        try:
            logo = logo_image(qid, thumbs.get(qid, ""))
            badge = make_badge(logo, logos[qid])
            return qid, badge and (badge, brand_color(logo))
        except Exception as e:  # noqa: BLE001 -- a broken logo just leaves the brand out
            print(f"skip {qid}: {e}", file=sys.stderr)
            return qid, None

    with concurrent.futures.ThreadPoolExecutor(1) as pool:
        badges = dict(pool.map(build, [q for q in qids if q in logos]))

    countries_by_iso = om_countries()
    index_lines = ["# Grove brand logos, generated by tools/grove/brand_logos.py. Do not edit.",
                   "# qid<TAB>offset<TAB>size<TAB>Organic Maps countries (;, * for anywhere)<TAB>brand names (|)"
                   "<TAB>place types (;)<TAB>main colour (RRGGBB, empty for grey logos)"]
    with open(os.path.join(ROOT, "data", "grove_brands.bin"), "wb") as pack:
        for qid in qids:  # Most used first: the renderer prefers them for shared names.
            if not badges.get(qid):
                continue
            png, color = badges[qid]
            names = sorted(n for n in brands[qid]["names"] if "\t" not in n and "|" not in n)
            codes = brands[qid]["countries"]
            countries = "*" if "*" in codes or not codes else \
                ";".join(sorted(n for c in codes for n in countries_by_iso.get(c, [])))
            if not countries:
                continue
            categories = ";".join(sorted(brands[qid]["categories"]))
            index_lines.append(
                f"{qid}\t{pack.tell()}\t{len(png)}\t{countries}\t{'|'.join(names)}\t{categories}\t{color}")
            pack.write(png)
    with open(os.path.join(ROOT, "data", "grove_brands.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(index_lines) + "\n")
    print(f"packed {len(index_lines) - 2} brand badges", file=sys.stderr)


if __name__ == "__main__":
    main()
