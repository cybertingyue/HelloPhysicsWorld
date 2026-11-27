# 3 Hello Collision

## What this chapter does

A lavender sphere drops once onto chapter 2’s orange hill. Yellow markers show where it touches the slope as it rolls downhill and off the edge.

Complete the [Jolt setup](../../README.md#jolt-physics-for-chapter-2-onward), reload CMake, and run `03_Hello_Collision` in the Release profile.

## How the code works

1. In [main.cpp](main.cpp), `main()` registers Jolt before calling `RunScene()`. The scene owns a `PhysicsSystem` until the window closes.
2. Add the heightfield as a static body and the sphere as an active dynamic body. `pairs.EnableCollision(terrainLayer, sphereLayer)` lets them collide. Starting the sphere off-center makes its first contact land on a slope.
3. Add elapsed time to an accumulator. Each time it holds at least 1/60 second, call `physics.Update()` for one step and subtract that time. A frame can take zero, one, or several steps.
4. Upload both wireframes once. Draw the hill with the identity matrix. For the sphere, read `GetWorldTransform(ballID)` after each step and pass its position and rotation to the vertex shader through the `model` uniform.
5. Before each physics step, clear the old points. Jolt calls `OnContactAdded` at first contact and `OnContactPersisted` during continued contact. Draw the reported points on the hill last so they stay visible over both wireframes.

## Play with the code

Try each edit separately in `RunScene()` in [main.cpp](main.cpp). Rebuild and run after each change.

### Drop on the opposite slope

Change the spawn point’s x coordinate from 2 to −2. Predict which way the sphere will roll.

```cpp
const JPH::RVec3 spawn(-2.0f, 16.0f, 0.0f);
```

### Switch off contact

Comment out the line that enables collisions between the terrain and sphere. Watch what happens when the sphere reaches the hill, then restore the line.

```cpp
// pairs.EnableCollision(terrainLayer, sphereLayer);
```

### Try weaker gravity

Add this line immediately after `physics.Init(...)`. How does the time it takes the sphere to reach the hill change?

```cpp
physics.SetGravity(JPH::Vec3(0.0f, -2.0f, 0.0f));
```
