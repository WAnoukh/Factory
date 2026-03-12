# Factory - Behaviour Tree

[<img width="486" height="289" alt="youtube demo" src="https://github.com/user-attachments/assets/ec0db9e8-4694-41b8-a18e-2e022c3c5308" />](https://www.youtube.com/watch?v=oTogB8DYddQ)

This is a quick implementation of Behaviour Tree for driving worker AI for a Prison Architect like game.
This is an exploratory work and contains bugs.

## Details

The map is separated by zones. Only one worker can build in a zone, and use boxes of material to build. Other workers can provide the zone with boxes for the builder. A worker that have nothing to do just wanders.

Workers follow the following BT:
<img width="763,7" height="324,8"  alt="behaviour tree" src="https://github.com/user-attachments/assets/5493d94b-c7d6-4f0e-a189-99ec93a20cf9" />

The execution nodes and the blackboard are very specialized for this particular usage. For a real generalist implementation, it may have been better to have a Hashmap style blackboard and a lot of generalist execution nodes.

## Important files

The behaviour tree is implemented in [/game/src/behaviour_tree/behaviour_tree.c](https://github.com/WAnoukh/Factory/blob/behavior-tree/game/src/behaviour_tree/behaviour_tree.c) and [behaviour_tree.h](https://github.com/WAnoukh/Factory/blob/behavior-tree/game/src/behaviour_tree/behaviour_tree.h)

And the tree construction and execution functions are in [/game/src/gameplay/worker_behaviour.c](https://github.com/WAnoukh/Factory/blob/behavior-tree/game/src/gameplay/worker_behaviour.c)
