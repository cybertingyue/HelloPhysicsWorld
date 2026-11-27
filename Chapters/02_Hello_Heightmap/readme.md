# 2 Hello Heightmap

## What this chapter does

Build a hill from a grid of heights and draw its Jolt collision shape as an orange wireframe.

Complete the [one-time Jolt setup](../../README.md#jolt-physics-for-chapter-2-onward), reload CMake, and run `02_Hello_Heightmap` in CLion’s Release profile.

## How the code works

1. In `BuildTerrainWireframe()`, each column gives an x coordinate, each row gives a z coordinate, and the stored value gives the height (y). The formula fills a 32 × 32 grid with a hill that peaks near the center.
2. `HeightFieldShapeSettings` turns those samples into a collision shape. When Jolt draws the shape in wireframe mode, `WireframeCollector::DrawLine()` saves the two endpoints of each edge.
3. `main()` uploads the endpoints to OpenGL once. Each frame, the vertex shader projects them onto the screen using a fixed camera view, and `GL_LINES` draws each pair as a line.

## Play with the code

Try each edit separately in [main.cpp](main.cpp). Rebuild and run after each change.

### Turn the hill into a bowl

In the height assignment, change the peak height from positive to negative. Predict which part of the terrain will sink the most.

```cpp
heights[row * side + col] = -7.0f * std::exp(-(dx * dx + dz * dz));
```

### Stretch the hill into a ridge

Change only the divisor in the `dx` calculation. Heights now fall off more slowly along x. Can you tell which direction x points in the window?

```cpp
const float dx = (static_cast<float>(col) - center) / 12.0f;
```

### View the hill from the side

Replace the `view` assignment in `main()`. The camera now looks horizontally toward the hill, so you can trace its height profile.

```cpp
const auto view = JPH::Mat44::sLookAt(
    JPH::Vec3(0.0f, 2.0f, 30.0f),
    JPH::Vec3(0.0f, 2.0f, 0.0f),
    JPH::Vec3::sAxisY());
```
