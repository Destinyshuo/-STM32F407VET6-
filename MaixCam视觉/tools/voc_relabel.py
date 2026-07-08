#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import random
import shutil
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path


DEFAULT_YELLOW_KEYS = [
    "yellow",
    "yellowing",
    "chlorosis",
    "chlorotic",
    "xanth",
    "pale",
]

DEFAULT_ROT_KEYS = [
    "rot",
    "rotten",
    "rotting",
    "decay",
    "decayed",
    "mold",
    "mould",
    "fungus",
    "fungal",
]


@dataclass
class SourcePair:
    image: Path
    xml: Path


def load_keyword_map(path: Path | None) -> dict[str, list[str]]:
    if not path:
        return {
            "yellow_leaf": DEFAULT_YELLOW_KEYS,
            "rot_leaf": DEFAULT_ROT_KEYS,
        }
    data = json.loads(path.read_text(encoding="utf-8"))
    return {
        "yellow_leaf": [s.lower() for s in data.get("yellow_leaf", DEFAULT_YELLOW_KEYS)],
        "rot_leaf": [s.lower() for s in data.get("rot_leaf", DEFAULT_ROT_KEYS)],
    }


def find_dir(root: Path, candidates: list[str]) -> Path | None:
    lower_map = {p.name.lower(): p for p in root.iterdir() if p.is_dir()}
    for name in candidates:
        if name.lower() in lower_map:
            return lower_map[name.lower()]
    for p in root.rglob("*"):
        if p.is_dir() and p.name.lower() in {c.lower() for c in candidates}:
            return p
    return None


def find_pairs(src: Path) -> list[SourcePair]:
    image_dir = find_dir(src, ["images", "JPEGImages", "jpg", "imgs", "img"])
    xml_dir = find_dir(src, ["xml", "Annotations", "annotation", "annotations"])
    if not image_dir or not xml_dir:
        raise SystemExit(
            f"找不到标准目录。请确认源数据里有图片目录和 xml 目录。当前检测到 image_dir={image_dir}, xml_dir={xml_dir}"
        )

    xmls = sorted(xml_dir.glob("*.xml"))
    pairs: list[SourcePair] = []
    for x in xmls:
        stem = x.stem
        image = None
        for ext in [".jpg", ".jpeg", ".png", ".bmp", ".JPG", ".JPEG", ".PNG", ".BMP"]:
            candidate = image_dir / f"{stem}{ext}"
            if candidate.exists():
                image = candidate
                break
        if image:
            pairs.append(SourcePair(image=image, xml=x))
    return pairs


def classify_name(name: str, keyword_map: dict[str, list[str]]) -> str | None:
    low = name.lower()
    for target, keys in keyword_map.items():
        if any(k in low for k in keys):
            return target
    return None


def relabel_xml(xml_path: Path, keyword_map: dict[str, list[str]]) -> tuple[ET.ElementTree, int]:
    tree = ET.parse(xml_path)
    root = tree.getroot()

    kept = 0
    for obj in list(root.findall("object")):
        name_node = obj.find("name")
        if name_node is None:
            root.remove(obj)
            continue

        mapped = classify_name(name_node.text or "", keyword_map)
        if not mapped:
            root.remove(obj)
            continue

        name_node.text = mapped
        kept += 1

    return tree, kept


def ensure_dirs(dst: Path) -> tuple[Path, Path]:
    images = dst / "JPEGImages"
    ann = dst / "Annotations"
    images.mkdir(parents=True, exist_ok=True)
    ann.mkdir(parents=True, exist_ok=True)
    return images, ann


def main() -> None:
    parser = argparse.ArgumentParser(description="Convert VOC-like plant disease dataset into 2 classes.")
    parser.add_argument("--src", type=Path, required=True, help="源数据集根目录")
    parser.add_argument("--dst", type=Path, required=True, help="输出数据集目录")
    parser.add_argument("--keywords", type=Path, default=None, help="可选 JSON 映射文件")
    parser.add_argument("--max-images", type=int, default=3000, help="最多保留图片数量")
    parser.add_argument("--seed", type=int, default=42, help="随机种子")
    parser.add_argument("--zip", dest="zip_path", type=Path, default=None, help="可选输出 zip 文件路径")
    args = parser.parse_args()

    keyword_map = load_keyword_map(args.keywords)
    pairs = find_pairs(args.src)
    if not pairs:
        raise SystemExit("没有找到可用的图片/xml 配对。")

    random.Random(args.seed).shuffle(pairs)
    pairs = pairs[: args.max_images]

    img_out, xml_out = ensure_dirs(args.dst)
    copied = 0
    kept_objects = 0
    skipped_images = 0

    for pair in pairs:
        tree, kept = relabel_xml(pair.xml, keyword_map)
        if kept == 0:
            skipped_images += 1
            continue

        shutil.copy2(pair.image, img_out / pair.image.name)
        tree.write(xml_out / pair.xml.name, encoding="utf-8", xml_declaration=True)
        copied += 1
        kept_objects += kept

    meta = {
        "source": str(args.src),
        "output": str(args.dst),
        "images_copied": copied,
        "objects_kept": kept_objects,
        "images_skipped_no_target": skipped_images,
        "classes": ["yellow_leaf", "rot_leaf"],
        "keywords": keyword_map,
    }
    (args.dst / "summary.json").write_text(json.dumps(meta, ensure_ascii=False, indent=2), encoding="utf-8")

    if args.zip_path:
        shutil.make_archive(str(args.zip_path.with_suffix("")), "zip", root_dir=args.dst)

    print(json.dumps(meta, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
