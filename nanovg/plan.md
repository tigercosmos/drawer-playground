# NanoVG Geometry Drawer Plan (C++, Minimum Effort)

## Survey Notes
- Target library: [memononen/nanovg](https://github.com/memononen/nanovg).
- NanoVG usage model:
  - Include `nanovg.h` + GL backend header (`nanovg_gl.h`).
  - Per-frame draw flow: `nvgBeginFrame(...)` -> draw paths -> `nvgEndFrame(...)`.
  - Requires OpenGL context and stencil buffer.
- Local machine check:
  - `clang` available (Apple clang 17).
  - `cmake` available (4.0.2).
  - `glew` available (`2.2.0`).
  - `glfw` installed (`3.4`).

## Implementation Plan
1. Vendor NanoVG source into workspace.
   - Create `third_party/nanovg/`.
   - Add `nanovg.h` and `nanovg_gl.h` from NanoVG `src/`.
2. Create a tiny C++ app (`src/main.cpp`) with GLFW + OpenGL + GLEW.
   - Create window and OpenGL context.
   - Initialize GLEW.
   - Create NanoVG context with `nvgCreateGL3(...)`.
3. Implement shape model in C++.
   - `enum class ShapeType { Line, Rect, Circle, Polygon, Polyline, TriangleMesh };`
   - `struct Shape` with type + points + style fields.
   - Keep a small `std::vector<Shape>` as scene state.
4. Add interactive draw tools (minimum UI).
   - Keys `1..6` select tool.
   - Click-drag for line/rect/circle.
   - Multi-click for polygon/polyline; `Enter` commits; `Backspace` removes last point.
   - Key `C` clears scene.
5. Add geometry testing modes (to evaluate NanoVG usefulness quickly).
   - Basic primitives: line, rect, circle.
   - Polygon test: convex and concave polygons, filled and stroked.
   - Polyline test: miter/bevel/round joins and butt/round/square caps.
   - Mesh-style test:
     - Triangle set rendered as many closed paths (wireframe + fill).
     - Grid/triangulated patch stress sample (e.g., 20x20 cell pattern) to check draw overhead.
6. Render loop and diagnostics.
   - Draw all stored shapes every frame.
   - Show light HUD text for active tool + point count + shape count.
7. Build configuration (`CMakeLists.txt`).
   - Target `geometry_drawer` as C++17.
   - Link GLFW + GLEW + OpenGL frameworks/libraries for macOS.
   - Build/run:
     - `cmake -S . -B build`
     - `cmake --build build`
     - `./build/geometry_drawer`

## Done Criteria
- C++ app compiles and runs.
- User can draw line, rectangle, circle, polygon, and polyline interactively.
- Mesh-style sample is visible and toggleable.
- Clear/reset works without restart.
- Code stays small and disposable for quick evaluation.

## Risks / Notes
- NanoVG is path-based, not a dedicated retained mesh renderer; "mesh" testing here is an approximation via many triangles/paths.
- If GL3 context setup fails on this machine, fallback to GL2 backend (`nvgCreateGL2`) is acceptable for this experiment.
