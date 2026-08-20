#!/usr/bin/env python3
"""Reduce the recovered HRR catalogue to Meatgrinder's topology-only campaign."""

import argparse
import json
from pathlib import Path


KINDS = {
    0: "route",
    1: "castle",
    2: "stable",
    3: "port",
    4: "cannon",
    5: "fort",
    6: "mine",
}


def make_tiles(seed: int) -> list[str]:
    rows = []
    for y in range(12):
        row = []
        for x in range(20):
            noise = (x * 17 + y * 29 + seed * 43 + x * y * 3) % 23
            row.append("2" if noise == 0 else "1" if noise < 5 else "0")
        rows.append("".join(row))
    return rows


def convert_record(record: dict) -> dict:
    active = {base["index"]: base for base in record["bases"] if base["type"] >= 0}
    links = set()
    for node_id, base in active.items():
        for neighbor in base["neighbors"].values():
            if neighbor in active:
                links.add(tuple(sorted((node_id, neighbor))))

    nodes = []
    for node_id, base in active.items():
        owner = base["owner"]
        nodes.append({
            "id": node_id,
            "x": node_id % 10,
            "y": node_id // 10,
            "kind": KINDS[base["type"]],
            "owner": -1 if owner < 0 else 0 if owner == 0 else 1,
            "soldiers": base["initial_soldiers"],
        })

    index = record["record_index"]
    return {
        "name": f"Front {index + 1}",
        "source": {
            "pack": record["pack"],
            "record": index,
            "sha256": record["sha256"],
        },
        "tiles": make_tiles(index),
        "nodes": nodes,
        "links": [list(link) for link in sorted(links)],
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("catalogue", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    catalogue = json.loads(args.catalogue.read_text())
    pack = next(pack for pack in catalogue["packs"] if pack["name"] == "map.dat")
    campaign = {
        "format": 1,
        "provenance": "HRR 2.0.0 map.dat records 0-9; topology-only reduction",
        "levels": [convert_record(record) for record in pack["records"][:10]],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(campaign, indent=2) + "\n")


if __name__ == "__main__":
    main()
