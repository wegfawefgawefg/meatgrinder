# First playable design

The initial campaign is a ten-front skeleton for experimenting with the genre.
It establishes the complete outer loop before deeper combat or content work:

```text
main menu -> level card -> battle -> defeat/retry
                              |
                              +-> score -> next level -> campaign complete
```

The battlefield is a graph drawn over an authored tile layer. The player can
select one node, box-select several, and issue a destination order. Each source
sends half its current garrison and computes a shortest graph route. Packets do
not collide on roads. Friendly packets merge at their destination; hostile
packets subtract from the destination garrison and capture it when their attack
survives the defense.

Every node produces independently. Base kind changes its local rate. A
Shift-click promotion spends local soldiers, raises the node's production, and
adds a modest defender advantage. There is intentionally no global bonus based
on castle count: territorial advantage already compounds through production,
and the old rule made a leading side unnecessarily stronger everywhere.

The first enemy controller is transparent and local. On a fixed interval it
looks at every enemy node, scores directly connected non-enemy targets, and
sends troops from the best available source. This is enough to pressure the
player while leaving future fronts open for personalities, objectives, supply,
formation behavior, diplomacy, and multiple independent sides.

Powerups, special scripted reinforcements, HRR dialogue, and HRR menu flow are
outside this foundation. The debug panel can tune the handful of prototype
rules or force outcomes so every campaign screen is easy to exercise.

