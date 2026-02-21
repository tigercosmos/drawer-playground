# Dear ImGui Geometry Drawer Plan (C++, Minimum Effort)

## Survey Notes
- Target library: [ocornut/imgui](https://github.com/ocornut/imgui).
- Dear ImGui usage model:
  - Immediate-mode GUI with per-frame calls (`NewFrame()` -> UI/draw -> `Render()`).
  - Custom geometry is drawn through `ImDrawList` (e.g., `AddLine`, `AddRect`, `AddCircle`, `AddPolyline`, `AddConvexPolyFilled`, `AddTriangleFilled`).
  - Typical desktop setup uses platform + renderer backends (`imgui_impl_glfw` + `imgui_impl_opengl3`).
- Local machine check:
  - `clang` available (Apple clang 17).
  - `cmake` available (4.0.2).
  - `glew` available (`2.2.0`).
  - `glfw` installed (`3.4`).

## Implementation Plan
1. Vendor Dear ImGui source into workspace.
   - Create `third_party/imgui/`.
   - Add core files (`imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`) and headers.
   - Add backends `backends/imgui_impl_glfw.*` and `backends/imgui_impl_opengl3.*`.
2. Create a tiny C++ app (`src/main.cpp`) with GLFW + OpenGL + GLEW.
   - Create window and OpenGL context.
   - Initialize GLEW.
   - Initialize ImGui context and backends (GLFW/OpenGL3).
3. Implement shape model in C++.
   - `enum class ShapeType { Line, Rect, Circle, Polygon, Polyline, TriangleMesh };`
   - `struct Shape` with type + points + style fields.
   - Keep a small `std::vector<Shape>` as scene state.
4. Add minimal interactive UI and input flow.
   - Toolbar to select tool mode.
   - Mouse click-drag for line/rect/circle.
   - Multi-click for polygon/polyline; button or key to commit shape.
   - Clear button to reset shapes.
5. Add geometry testing modes (to evaluate ImGui draw API usefulness quickly).
   - Basic primitives: line, rect, circle.
   - Polygon test: convex and concave point sets (filled/stroked behavior validation).
   - Polyline test: different thickness and closed/open paths.
   - Mesh-style test:
     - Triangle set with repeated `AddTriangleFilled`/outline.
     - Grid-like triangulated patch stress sample (e.g., 20x20) to check CPU-side draw-list overhead.
6. Render loop and diagnostics.
   - Build an ImGui window as control panel (tool, colors, line thickness, toggles).
   - Render geometry to a dedicated canvas region using `ImDrawList`.
   - Show lightweight stats: shape count, vertex/index count from draw data.
7. Build configuration (`CMakeLists.txt`).
   - Target `imgui_geometry_drawer` as C++17.
   - Compile ImGui core + backend sources into the app.
   - Link GLFW + GLEW + OpenGL frameworks/libraries for macOS.
   - Build/run:
     - `cmake -S . -B build`
     - `cmake --build build`
     - `./build/imgui_geometry_drawer`

## Done Criteria
- C++ app compiles and runs.
- User can draw line, rectangle, circle, polygon, and polyline interactively on an ImGui canvas.
- Mesh-style sample is visible and toggleable.
- Clear/reset works without restart.
- Code stays small and disposable for quick evaluation.

## Risks / Notes
- Dear ImGui is primarily a GUI toolkit; geometry drawing is via low-level draw-list APIs rather than a dedicated vector engine.
- Concave polygon fill support needs care; fallback is triangulating concave shapes into triangles when needed.
- If OpenGL 3.2 core profile setup fails, fallback to a compatible OpenGL backend path is acceptable for this experiment.
