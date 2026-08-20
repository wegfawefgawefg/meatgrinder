# First playable design

The initial campaign is a ten-front skeleton for experimenting with the genre.
It establishes the complete outer loop before deeper combat or content work:

```text
main menu -> level card -> battle -> defeat/retry
                              |
                              +-> score -> next level -> campaign complete
```

The battlefield is a tile-aligned graph in world space. The camera fits each
front, eases into place at setup, emphasizes victory, and otherwise moves only
under player pan and zoom. Captures never steal camera control. Castles, roads,
troops, and background tiles all share the same coordinate system.

The player can select one node, box-select several, Shift-box to add more, or
Shift-click to toggle one member of the selection, then issue a destination
order. A normal order is an assault: the packet stops, fights, and captures at
every hostile node on its route before continuing. Control-click issues a
direct order which passes intermediate nodes and fights only at its final
target. Packets do not collide on roads.

Every occupied base generates one local soldier on a four-second GEN. The HUD
shows the countdown. Pressing R with one or more owned bases selected enters
rally creation; the next left-click sets an assault rally, or a direct rally
while Control is held. Entering rally creation clears the old orders, so Escape
intentionally leaves those bases unordered. A box selection can assign the same
rally to many sources. Each
GEN forwards the rally's stored one, half, or all-but-one commitment along its
persistent route. Source markers and colored paths remain visible until C clears
selected orders, or C enters click-to-clear mode with no selection. Headquarters
remains a relocatable strategic identity but is not the only production faucet.

There is intentionally no global combat bonus based on castle count. Territory
already increases headquarters recruitment, so a leading side does not also
make every individual soldier stronger.

Enemy controllers evaluate full routes, accumulated defense, distance, player
ownership, and headquarters value. Campaign levels rotate four personalities:
balanced waits for efficient attacks, aggressive accepts attrition to pressure
HQ, turtle accumulates a large favorable attack, and swarm repeatedly sends
small packets at nearby weak points. If none can justify an attack, a logistics
pass identifies the most useful frontline staging base and transfers surplus
from rear territory along an entirely friendly route. Troops already in transit
count toward the staging requirement, preventing blind over-supply. The styles
share readable rules rather than hidden bonuses.

Troop commitment is selected with number keys: one soldier, half the garrison,
or all but one. The same mode drives one-shot and rally orders, making probes,
steady forwarding, and decisive pushes explicit.

Powerups, special scripted reinforcements, HRR dialogue, and HRR menu flow are
outside this foundation. The debug panel can tune the handful of prototype
rules or force outcomes so every campaign screen is easy to exercise.
