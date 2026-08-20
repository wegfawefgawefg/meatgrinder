#!/usr/bin/env python3
"""Build Meatgrinder's original mechanic-led campaign."""

import json
from pathlib import Path


WORLDS = [
    ("front", "THE FRONT", "MOVEMENT AND PRODUCTION", 1, 1),
    ("battery", "THE BATTERY", "CANNON FIRE", 4, 1),
    ("walls", "THE WALLS", "FORTIFIED GROUND", 7, 2),
    ("remount", "THE REMOUNT", "FAST LOGISTICS", 7, 5),
    ("straits", "THE STRAITS", "DIVIDED FRONTS", 4, 6),
    ("goldroad", "THE GOLD ROAD", "SUPPLY LINES", 1, 5),
]


def node(node_id, x, y, kind="node", owner=-1, soldiers=0, **extra):
    return {"id": node_id, "x": x, "y": y, "kind": kind,
            "owner": owner, "soldiers": soldiers, **extra}


def headquarters(node_id, x, y, owner):
    return node(node_id, x, y, "producer", owner, 8, headquarters=True)


def level(world, number, name, briefing, nodes, links, sea_links=None):
    level_id = f"{world + 1}-{number}"
    if number == 1:
        requires = [] if world == 0 else [f"{world}-{5}"]
    elif number in (2, 3):
        requires = [f"{world + 1}-1"]
    elif number == 4:
        requires = [f"{world + 1}-2"]
    else:
        requires = [f"{world + 1}-4"]
    positions = [(1, 2), (3, 1), (3, 3), (5, 2), (7, 2)]
    return {
        "id": level_id,
        "name": name,
        "briefing": briefing,
        "world": world,
        "map_x": positions[number - 1][0],
        "map_y": positions[number - 1][1],
        "requires": requires,
        "tiles": [
            "00000000000000000000", "00000100000001000000",
            "00000000020000000000", "00100000000000000100",
            "00000010000010000000", "00000000000000000000",
            "00002000000000200000", "00000001001000000000",
            "00000000000000000000", "00100000000000000100",
            "00000000020000000000", "00000100000001000000",
        ],
        "nodes": nodes,
        "links": links,
        "sea_links": sea_links or [],
    }


def line_map(kinds, owners=None, soldiers=None, targets=None):
    count = len(kinds)
    owners = owners or [-1] * count
    soldiers = soldiers or [0] * count
    targets = targets or {}
    nodes = []
    for index, kind in enumerate(kinds):
        if index == 0:
            nodes.append(headquarters(index, 1, 4, 0))
        elif index == count - 1:
            nodes.append(headquarters(index, count, 4, 1))
        else:
            extra = {"target": targets[index]} if index in targets else {}
            nodes.append(node(index, index + 1, 4, kind, owners[index], soldiers[index], **extra))
    return nodes, [[index, index + 1] for index in range(count - 1)]


def world_one():
    levels = []
    nodes, links = line_map(["producer", "node", "producer", "node", "producer"])
    levels.append(level(0, 1, "THE LINE", "TAKE THE CENTER. BREAK THE RED HQ.", nodes, links))

    nodes = [headquarters(0, 1, 4, 0), node(1, 3, 2), node(2, 3, 6),
             node(3, 5, 2, "producer"), node(4, 5, 6), headquarters(5, 7, 4, 1)]
    links = [[0, 1], [0, 2], [1, 3], [2, 4], [3, 5], [4, 5], [3, 4]]
    levels.append(level(0, 2, "THE FORK", "CHOOSE A ROAD OR PRESS BOTH FRONTS.", nodes, links))

    nodes = [headquarters(0, 1, 4, 0), node(1, 2, 2, "producer", 0, 3),
             node(2, 2, 6, "producer", 0, 3), node(3, 4, 4), node(4, 6, 2, "producer", 1, 3),
             node(5, 6, 6, "producer", 1, 3), headquarters(6, 7, 4, 1)]
    links = [[0, 1], [0, 2], [1, 3], [2, 3], [3, 4], [3, 5], [4, 6], [5, 6]]
    levels.append(level(0, 3, "TWO HANDS", "SELECT TOGETHER. SEND TOGETHER.", nodes, links))

    nodes, links = line_map(["producer", "producer", "node", "node", "producer", "producer"],
                            [0, 0, -1, -1, 1, 1], [8, 4, 0, 0, 4, 8])
    levels.append(level(0, 4, "THE RELAY", "RALLY RECRUITS TOWARD THE FRONT.", nodes, links))

    nodes = [headquarters(0, 1, 4, 0), node(1, 2, 2, "producer", 0, 3), node(2, 2, 6),
             node(3, 4, 2), node(4, 4, 6, "producer"), node(5, 6, 2),
             node(6, 6, 6, "producer", 1, 3), headquarters(7, 7, 4, 1)]
    links = [[0, 1], [0, 2], [1, 3], [2, 4], [3, 5], [4, 6], [3, 4], [5, 7], [6, 7]]
    levels.append(level(0, 5, "FIRST FRONT", "USE EVERY ORDER YOU HAVE LEARNED.", nodes, links))
    return levels


def cannon_world():
    levels = []
    nodes, links = line_map(["producer", "cannon", "node", "producer"],
                            [0, 0, -1, 1], [8, 2, 0, 10], {1: 3})
    levels.append(level(1, 1, "RANGING SHOT", "YOUR CANNON FIRES ON THE RED HQ.", nodes, links))
    nodes, links = line_map(["producer", "node", "cannon", "node", "producer"],
                            targets={2: 4})
    levels.append(level(1, 2, "SEIZE THE GUN", "THE CENTER BATTERY BELONGS TO WHOEVER TAKES IT.", nodes, links))
    nodes, links = line_map(["producer", "node", "producer", "cannon", "producer"],
                            [-1, -1, -1, 1, -1], [0, 0, 0, 3, 0], {3: 0})
    levels.append(level(1, 3, "UNDER FIRE", "MOVE BEFORE THE BATTERY EMPTIES YOUR HQ.", nodes, links))
    nodes = [headquarters(0, 1, 4, 0), node(1, 3, 2, "cannon", 0, 2, target=4),
             node(2, 3, 6), node(3, 5, 2), node(4, 5, 6, "producer", 1, 5),
             headquarters(5, 7, 4, 1)]
    links = [[0, 1], [0, 2], [1, 3], [2, 4], [3, 5], [4, 5], [3, 4]]
    levels.append(level(1, 4, "CROSS BATTERY", "USE FIRE TO OPEN THE OTHER ROAD.", nodes, links))
    nodes = [headquarters(0, 1, 4, 0), node(1, 2, 2, "cannon", 0, 2, target=5),
             node(2, 2, 6), node(3, 4, 2, "cannon", -1, 0, target=7), node(4, 4, 6, "producer"),
             node(5, 6, 2, "producer", 1, 6), node(6, 6, 6), headquarters(7, 7, 4, 1)]
    links = [[0, 1], [0, 2], [1, 3], [2, 4], [3, 5], [4, 6], [5, 7], [6, 7], [3, 4]]
    levels.append(level(1, 5, "GUN LINE", "TURN THE CENTER GUN ON THE RED REAR.", nodes, links))
    return levels


def fort_world():
    layouts = [
        (["producer", "producer", "fort", "producer"], [-1, 0, 1, -1], [0, 4, 5, 0]),
        (["producer", "fort", "producer", "fort", "producer"], [-1, -1, -1, 1, -1], [0, 0, 0, 4, 0]),
        (["producer", "fort", "producer", "fort", "producer"], [-1, 0, -1, 1, -1], [0, 3, 0, 3, 0]),
        (["producer", "node", "fort", "cannon", "producer"], [-1, 0, -1, 1, -1], [0, 2, 0, 2, 0]),
        (["producer", "fort", "producer", "fort", "producer", "producer"], [-1, 0, -1, -1, 1, -1], [0, 3, 0, 0, 3, 0]),
    ]
    names = [("HOLDFAST", "A FORT MAKES EVERY DEFENDER COUNT TWICE."),
             ("THE BREACH", "AMASS ENOUGH FORCE TO BREAK THE RED FORT."),
             ("TWO WALLS", "DEFEND YOUR WALL WHILE TESTING THEIRS."),
             ("SHOT AND STONE", "CANNON FIRE SOFTENS A FORTIFIED ROAD."),
             ("CITADEL", "TAKE THE PRODUCER BEFORE THE FINAL WALL." )]
    result = []
    for index, ((kinds, owners, troops), (name, brief)) in enumerate(zip(layouts, names), 1):
        targets = {3: 2} if index == 4 else {}
        nodes, links = line_map(kinds, owners, troops, targets)
        result.append(level(2, index, name, brief, nodes, links))
    return result


def stable_world():
    result = []
    configs = [
        (["producer", "stable", "node", "node", "producer"], [0, 0, -1, -1, 1]),
        (["producer", "node", "stable", "node", "producer"], [0, -1, -1, -1, 1]),
        (["producer", "stable", "fort", "node", "producer"], [0, 0, -1, -1, 1]),
        (["producer", "producer", "stable", "cannon", "node", "producer"], [0, 0, 0, 1, -1, 1]),
        (["producer", "stable", "node", "producer", "stable", "producer"], [0, 0, -1, -1, 1, 1]),
    ]
    names = [("QUICK MARCH", "ARMIES LEAVING A STABLE MOVE FASTER."),
             ("THE REMOUNT", "CAPTURE THE STABLE, THEN LAUNCH FROM IT."),
             ("RACE TO THE WALL", "SPEED BUILDS THE FORCE THAT BREAKS A FORT."),
             ("RUN THE GUNS", "REINFORCE BEFORE THE CANNON WEARS YOU DOWN."),
             ("MOBILE FRONT", "TWO STABLES TURN THE LINE INTO A RACE.")]
    for index, ((kinds, owners), (name, brief)) in enumerate(zip(configs, names), 1):
        targets = {3: 0} if index == 4 else {}
        nodes, links = line_map(kinds, owners, [6 if owner >= 0 else 0 for owner in owners], targets)
        result.append(level(3, index, name, brief, nodes, links))
    return result


def port_level(world, number, name, brief, extra_kind="node"):
    extra = {"target": 0} if extra_kind == "cannon" else {}
    nodes = [headquarters(0, 1, 3, 0), node(1, 2, 3, "port", 0, 3),
             node(2, 5, 2, "port", 0, 2), node(3, 6, 2, extra_kind, **extra),
             headquarters(4, 8, 3, 1), node(5, 7, 3, "port", 1, 3),
             node(6, 4, 6, "port", 1, 2), node(7, 3, 6, "producer")]
    links = [[0, 1], [2, 3], [3, 4], [4, 5], [6, 7], [7, 0]]
    sea = [[1, 2], [5, 6]]
    return level(world, number, name, brief, nodes, links, sea)


def port_world():
    return [
        port_level(4, 1, "TWO SHORES", "FRIENDLY PORTS JOIN SEPARATE LAND FRONTS."),
        port_level(4, 2, "CUT THE FERRY", "TAKE A PORT TO BREAK RED LOGISTICS.", "fort"),
        port_level(4, 3, "THREE FRONTS", "WATCH EACH SHORE; THE SEA MOVES RESERVES."),
        port_level(4, 4, "FAST CROSSING", "STABLES AND PORTS MAKE A RAPID NETWORK.", "stable"),
        port_level(4, 5, "THE STRAITS", "CONTROL BOTH LAND WARS AND THEIR SEA BRIDGES.", "cannon"),
    ]


def mine_world():
    result = []
    configs = [
        (["producer", "node", "mine", "node", "producer"], [-1, 0, 0, -1, -1]),
        (["producer", "mine", "node", "mine", "producer"], [-1, 0, -1, 1, -1]),
        (["producer", "fort", "mine", "node", "producer"], [-1, 0, 0, -1, -1]),
        (["producer", "port", "mine", "stable", "producer"], [-1, 0, 0, 1, -1]),
        (["producer", "cannon", "mine", "fort", "mine", "producer"], [-1, 0, -1, 1, 1, -1]),
    ]
    names = [("PAYDIRT", "GOLD REACHING YOUR HQ BECOMES A SOLDIER."),
             ("CLAIM JUMP", "CONTEST BOTH MINES AND GUARD THEIR ROADS."),
             ("ARMORED CONVOY", "A FORT KEEPS THE GOLD ROAD OPEN."),
             ("LONG HAUL", "LOGISTICS CARRY WEALTH ACROSS THE FRONT."),
             ("MEATGRINDER", "EVERY MACHINE OF WAR IS ON THE MAP.")]
    for index, ((kinds, owners), (name, brief)) in enumerate(zip(configs, names), 1):
        if index == 4:
            result.append(port_level(5, 4, name, brief, "mine"))
            continue
        targets = {1: 4} if index == 5 else {}
        troops = [6 if owner >= 0 else 0 for owner in owners]
        nodes, links = line_map(kinds, owners, troops, targets)
        result.append(level(5, index, name, brief, nodes, links))
    return result


def main():
    worlds = [{"id": item[0], "name": item[1], "theme": item[2],
               "map_x": item[3], "map_y": item[4]} for item in WORLDS]
    levels = world_one() + cannon_world() + fort_world() + stable_world() + port_world() + mine_world()
    output = Path(__file__).resolve().parents[1] / "assets/levels/campaign.json"
    output.write_text(json.dumps({"format": 2, "worlds": worlds, "levels": levels}, indent=2) + "\n")


if __name__ == "__main__":
    main()
