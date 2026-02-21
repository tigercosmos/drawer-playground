# Skia Geometry Drawer Plan (C++, Minimum Effort)

## Survey Notes
- Target library: [google/skia](https://github.com/google/skia).
- Skia usage model:
  - Draw through `SkCanvas` obtained from an `SkSurface`.
  - Typical frame flow: begin backend frame -> `SkCanvas` draw calls (`drawLine`, `drawRect`, `drawCircle`, `drawPath`, etc.) -> flush/present.
  - GPU path usually uses Ganesh backend (`GrDirectContext`) with OpenGL/Metal/Vulkan; for quick desktop setup, OpenGL + GLFW is practical.
- Local machine check:
  - `clang` available (Apple clang 17).
  - `cmake` available (4.0.2).
  - `glew` available (`2.2.0`).
  - `glfw` installed (`3.4`).
  - `ninja` currently not installed (needed for common Skia GN workflows).

## Implementation Plan
1. Vendor Skia source into workspace.
   - Create `third_party/skia/`.
   - Sync Skia dependencies (`python tools/git-sync-deps`) and generate build files with GN.
   - Build Skia static libs for desktop GPU rendering.
2. Create a tiny C++ app (`src/main.cpp`) with GLFW + OpenGL + GLEW.
   - Create window and OpenGL context.
   - Initialize GLEW.
   - Create Skia GPU context (`GrDirectContext`) and wrap framebuffer into `SkSurface`.
3. Implement shape model in C++.
   - `enum class ShapeType { Line, Rect, Circle, Polygon, Polyline, TriangleMesh };`
   - `struct Shape` with type + points + style fields.
   - Keep a small `std::vector<Shape>` as scene state.
4. Add minimal interactive UI/input flow.
   - Toolbar/shortcuts to select tool mode.
   - Mouse click-drag for line/rect/circle.
   - Multi-click for polygon/polyline; button or key to commit shape.
   - Clear action to reset shapes.
5. Add geometry testing modes (to evaluate Skia draw API usefulness quickly).
   - Basic primitives: line, rect, circle.
   - Polygon test: convex and concave polygons using `SkPath`.
   - Polyline test: stroke width, caps, joins, and miter limits.
   - Mesh-style test:
     - Triangle set rendering (path-based or `SkVertices`).
     - Grid-like triangulated patch stress sample (e.g., 20x20) to check draw overhead.
6. Render loop and diagnostics.
   - Draw all shapes each frame on `SkCanvas`.
   - Render lightweight HUD text for active tool + shape count.
   - Show simple frame stats (frame time / draw call count approximation).
7. Build configuration (`CMakeLists.txt`).
   - Target `skia_geometry_drawer` as C++17.
   - Link GLFW + GLEW + OpenGL plus built Skia libs/includes.
   - Build/run:
     - `cmake -S . -B build`
     - `cmake --build build`
     - `./build/skia_geometry_drawer`

## Done Criteria
- C++ app compiles and runs.
- User can draw line, rectangle, circle, polygon, and polyline interactively.
- Mesh-style sample is visible and toggleable.
- Clear/reset works without restart.
- Code stays small and disposable for quick evaluation.

## Risks / Notes
- Skia setup cost is significantly higher than Dear ImGui/NanoVG because GN/Ninja/dependency sync are typically required.
- Backend choice matters on macOS (OpenGL is straightforward for quick prototype; Metal may be preferred for long-term support).
- If GPU backend setup is blocked, fallback to Skia CPU raster surface is acceptable for API evaluation but not representative of final performance.
