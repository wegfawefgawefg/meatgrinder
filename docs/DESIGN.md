# First playable design

The initial campaign is a ten-front skeleton for experimenting with the genre.
It establishes the complete outer loop before deeper combat or content work:

```text
main menu -> level card -> battle -> defeat/retry
                              |
                              +-> score -> next level -> campaign complete
```

The battlefield is a tile-aligned graph in world space. The camera fits each
front, eases into place, follows captures slightly, and supports interpolated
pan and zoom. Castles, roads, troops, and background tiles all share the same
coordinate system.

The player can select one node, box-select several, and issue a destination
order. A normal order is an assault: the packet stops, fights, and captures at
every hostile node on its route before continuing. Control-click issues a
direct order which passes intermediate nodes and fights only at its final
target. Packets do not collide on roads.

Each faction has one headquarters. Every owned node contributes 50 production
points, and every GEN converts the total to `floor(points / 100) + 3` recruits
at headquarters. Active mines multiply that payout. The HUD shows the next GEN
countdown. The player may relocate HQ with H and can right-drag from HQ to
establish a rally route which forwards each new recruit batch as an assault.

There is intentionally no global combat bonus based on castle count. Territory
already increases headquarters recruitment, so a leading side does not also
make every individual soldier stronger.

The first enemy controller is transparent and local. On a fixed interval it
looks at every enemy node, scores directly connected non-enemy targets, and
sends troops from the best available source. This is enough to pressure the
player while leaving future fronts open for personalities, objectives, supply,
formation behavior, diplomacy, and multiple independent sides.

Powerups, special scripted reinforcements, HRR dialogue, and HRR menu flow are
outside this foundation. The debug panel can tune the handful of prototype
rules or force outcomes so every campaign screen is easy to exercise.
