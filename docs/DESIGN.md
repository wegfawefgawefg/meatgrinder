# First playable design

The campaign is organized as six mechanic-led worlds with five battles each:

```text
main menu -> world map -> level map -> level card -> battle -> defeat/retry
                  ^             ^                         |
                  |             +---------- score <-------+
                  +------- world unlock transition
```

The battlefield is a tile-aligned graph in world space. The camera fits each
front, eases into place at setup, emphasizes victory, and otherwise moves only
under player pan and zoom. Middle-drag and Space-left-drag grab the map directly;
keyboard panning remains available. Captures never steal camera control.
Nodes, roads, troops, and background tiles all share the same coordinate system.
The top force meter compares every blue and red soldier in garrisons and in
transit, so issuing an order does not change either side's displayed total.

The player can select one node, box-select several, Shift-box to add more, or
Shift-click to toggle one member of the selection, then issue a destination
order. An ordinary click on a friendly node always replaces the selection;
Control-click deliberately transfers the current selection into a friendly
node. A normal hostile order is an assault: the packet stops, fights, and
captures at every hostile node on its route before continuing. Control-click issues a
direct order which passes intermediate nodes and fights only at its final
target. Packets do not collide on roads.

Ordinary nodes are non-producing territory. Producers and fixed HQs generate
one local soldier on a four-second GEN. The HUD
shows the countdown. Pressing R with one or more owned bases selected enters
rally creation; the next left-click sets an assault rally, or a direct rally
while Control is held. Entering rally creation clears the old orders, so Escape
intentionally leaves those bases unordered. A box selection can assign the same
rally to many sources. Each GEN forwards the rally's stored one, half, or
all-but-one commitment along its
persistent route. Source markers and colored paths remain visible until C clears
selected orders, or C enters click-to-clear mode with no selection. Headquarters
is fixed at match start. Its capture immediately defeats its owner, destroys the
HQ on the battlefield, then opens the score screen with a top-down impact wipe
and staggered counting rows.

Specialized nodes exchange production for positional power. A cannon fires at
one authored hostile target every two seconds; its projectile takes 1.1 seconds
to arrive and removes one defender without capturing. A fort doubles the force
required to remove its garrison. An army launched from a stable keeps a 1.5x
movement multiplier for that trip. Authored sea edges are traversable only while
both endpoint ports belong to the moving side. A mine produces one gold shipment
every six seconds only when an entirely friendly path to HQ exists. Gold moves
at half army speed, is lost when its route is cut, and adds one soldier on HQ
arrival.

There is intentionally no global combat bonus based on castle count. Territory
already increases total recruitment by adding local generators, so a leading
side does not also make every individual soldier stronger. Army packets travel
at a deliberate 20 world pixels per second.

Enemy controllers evaluate full routes, accumulated defense, distance, player
ownership, and headquarters value. Campaign levels rotate four personalities:
balanced waits for efficient attacks, aggressive accepts attrition to pressure
HQ, turtle accumulates a large favorable attack, and swarm repeatedly sends
single-soldier packets while keeping one defender. Logistics candidates
identify useful frontline staging bases and transfer surplus from rear
territory along entirely friendly routes. Troops already in transit count
toward staging requirements, preventing blind over-supply.

Each decision evaluates every node and builds attack, expansion, reinforcement,
and wait candidates. Personality weights choose the kind of action; difficulty
controls idle weight, evaluation error, reaction interval, and a global action
budget. Weak AI often waits, while hard AI can issue two non-conflicting orders.
The current strategic objective receives a small score bonus and changes only
when another target wins by a meaningful margin. The board is still rescored on
every decision, and hostile troops inbound to an owned base trigger immediate
reinforcement without waiting on the behavior roll. Troops already marching
keep their original route. The styles share readable rules rather than hidden
combat bonuses.

Troop commitment is selected with number keys: one soldier, half the garrison,
or all but one. The same mode drives one-shot and rally orders, making probes,
steady forwarding, and decisive pushes explicit.

Each world introduces its new mechanic safely, requires its use, lets the enemy
contest it, combines it with earlier mechanics, and ends with a mastery map.
The third battle is an optional branch; the fifth is the required world gate.
Scores and completion times persist. Powerups and scripted reinforcements remain
outside this foundation. The debug panel tunes rules or forces outcomes.
