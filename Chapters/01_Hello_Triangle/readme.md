# 1 Hello Triangle

## What this chapter does

Open an 800×600 window with GLFW and draw an orange triangle with OpenGL.

## How the code works

Follow `main()` in [main.cpp](main.cpp):

1. Create the window and activate its OpenGL context.
2. Call `CreateProgram()` to compile and link two shaders. The vertex shader sets each corner’s position. The fragment shader colors the triangle orange.
3. Upload three pairs of coordinates to a vertex buffer and tell OpenGL to read each pair as one corner.
4. Each frame, clear the background, draw the triangle with `glDrawArrays`, display the frame, and process window events. On exit, release the graphics resources and close the window.

## Play with the code

Make these edits in [main.cpp](main.cpp). Before each run, predict what will change on screen.

### 1. Mix a new color

In the fragment shader, replace the line that assigns `color`. The first three numbers control red, green, and blue. What color do green and blue make together?

```glsl
color = vec4(0.0, 1.0, 1.0, 1.0);
```

### 2. Move one corner

In `vertices[]`, replace the last pair of coordinates with the pair below. Which corner moves? Try changing only its x coordinate, then only its y coordinate.

```cpp
0.75f, 0.5f,
```

### 3. Blend colors across the triangle

Give each corner its own color. In the vertex shader, add these declarations above `main()`:

```glsl
out vec3 vertexColor;
const vec3 colors[3] = vec3[3](
    vec3(1.0, 0.0, 0.0),  // first corner: red
    vec3(0.0, 1.0, 0.0),  // second corner: green
    vec3(0.0, 0.0, 1.0)   // third corner: blue
);
```

Inside that shader’s `main()`, add one line after the `gl_Position` assignment. For this draw call, `gl_VertexID` counts from 0 to 2.

```glsl
vertexColor = colors[gl_VertexID];
```

In the fragment shader, add  
`in vec3 vertexColor;`   
right after  
`#version 330 core`, and replace the color assignment in `main()`:

```glsl
color = vec4(vertexColor, 1.0);
```

Look halfway along the edge joining red and blue. It appears purple because OpenGL interpolates the corner colors and the fragment shader receives the blended value at each fragment. Move a corner again and watch how the gradient follows it.
