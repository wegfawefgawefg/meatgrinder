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

The player can select one node, box-select several, and issue a destination
order. A normal order is an assault: the packet stops, fights, and captures at
every hostile node on its route before continuing. Control-click issues a
direct order which passes intermediate nodes and fights only at its final
target. Packets do not collide on roads.

Every occupied base generates one local soldier on a four-second GEN. The HUD
shows the countdown. Right-clicking any owned base enters rally creation; the
next left-click sets an assault rally, or a direct rally while Control is held.
Each GEN immediately forwards that base's new soldier along its persistent
route. Source markers and colored paths remain visible until the order is
cleared. Headquarters remains a relocatable strategic identity but is not the
only production faucet.

There is intentionally no global combat bonus based on castle count. Territory
already increases headquarters recruitment, so a leading side does not also
make every individual soldier stronger.

Enemy controllers evaluate full routes, accumulated defense, distance, player
ownership, and headquarters value. Campaign levels rotate four personalities:
balanced waits for efficient attacks, aggressive accepts attrition to pressure
HQ, turtle accumulates a large favorable attack, and swarm repeatedly sends
small packets at nearby weak points. They share readable rules rather than
hidden bonuses.

One-shot troop commitment is selected with number keys: 25%, 50%, 75%, or 90%.
This makes reinforcing, probing, and decisive pushes distinct decisions instead
of forcing half the garrison into every order.

Powerups, special scripted reinforcements, HRR dialogue, and HRR menu flow are
outside this foundation. The debug panel can tune the handful of prototype
rules or force outcomes so every campaign screen is easy to exercise.
