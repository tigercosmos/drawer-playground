#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

struct Vec2 {
  float x;
  float y;
};

enum class ShapeType {
  Line,
  Rect,
  Circle,
  Polygon,
  Polyline,
  TriangleMesh,
};

struct Shape {
  ShapeType type;
  std::vector<Vec2> points;
  ImU32 stroke_color;
  ImU32 fill_color;
  float stroke_thickness;
  bool closed;
  int rows;
  int cols;
};

struct AppState {
  std::vector<Shape> shapes;
  ShapeType active_tool = ShapeType::Line;
  std::vector<Vec2> draft_points;
  bool show_stress_mesh = true;
  float stroke_thickness = 2.0f;
  int mesh_rows = 10;
  int mesh_cols = 10;
  ImVec2 drag_start = ImVec2(0, 0);
  bool dragging = false;
  int last_total_vtx = 0;
  int last_total_idx = 0;
};

static const char* tool_name(ShapeType t) {
  switch (t) {
    case ShapeType::Line:
      return "Line";
    case ShapeType::Rect:
      return "Rect";
    case ShapeType::Circle:
      return "Circle";
    case ShapeType::Polygon:
      return "Polygon";
    case ShapeType::Polyline:
      return "Polyline";
    case ShapeType::TriangleMesh:
      return "Tri Mesh";
  }
  return "Unknown";
}

static ImU32 stroke_color_for(ShapeType t) {
  switch (t) {
    case ShapeType::Line:
      return IM_COL32(245, 99, 84, 255);
    case ShapeType::Rect:
      return IM_COL32(255, 196, 61, 255);
    case ShapeType::Circle:
      return IM_COL32(66, 196, 136, 255);
    case ShapeType::Polygon:
      return IM_COL32(61, 165, 255, 255);
    case ShapeType::Polyline:
      return IM_COL32(204, 138, 255, 255);
    case ShapeType::TriangleMesh:
      return IM_COL32(255, 140, 80, 255);
  }
  return IM_COL32_WHITE;
}

static ImU32 fill_color_for(ShapeType t) {
  switch (t) {
    case ShapeType::Line:
      return IM_COL32(245, 99, 84, 30);
    case ShapeType::Rect:
      return IM_COL32(255, 196, 61, 45);
    case ShapeType::Circle:
      return IM_COL32(66, 196, 136, 45);
    case ShapeType::Polygon:
      return IM_COL32(61, 165, 255, 55);
    case ShapeType::Polyline:
      return IM_COL32(204, 138, 255, 35);
    case ShapeType::TriangleMesh:
      return IM_COL32(255, 140, 80, 26);
  }
  return IM_COL32(255, 255, 255, 30);
}

static float distance(ImVec2 a, ImVec2 b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

static ImVec2 to_screen(Vec2 p, ImVec2 origin) { return ImVec2(origin.x + p.x, origin.y + p.y); }

static Vec2 from_screen(ImVec2 p, ImVec2 origin) { return Vec2{p.x - origin.x, p.y - origin.y}; }

static void draw_triangle_grid(ImDrawList* dl, Vec2 a, Vec2 b, int rows, int cols, ImU32 stroke,
                               ImU32 fill, float thickness, ImVec2 origin) {
  const float x0 = std::min(a.x, b.x);
  const float y0 = std::min(a.y, b.y);
  const float x1 = std::max(a.x, b.x);
  const float y1 = std::max(a.y, b.y);
  const float w = x1 - x0;
  const float h = y1 - y0;
  if (w < 1.0f || h < 1.0f || rows <= 0 || cols <= 0) {
    return;
  }

  const float cw = w / static_cast<float>(cols);
  const float ch = h / static_cast<float>(rows);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const float lx = x0 + static_cast<float>(c) * cw;
      const float ty = y0 + static_cast<float>(r) * ch;

      ImVec2 p00 = to_screen(Vec2{lx, ty}, origin);
      ImVec2 p10 = to_screen(Vec2{lx + cw, ty}, origin);
      ImVec2 p01 = to_screen(Vec2{lx, ty + ch}, origin);
      ImVec2 p11 = to_screen(Vec2{lx + cw, ty + ch}, origin);

      dl->AddTriangleFilled(p00, p10, p11, fill);
      dl->AddTriangle(p00, p10, p11, stroke, thickness);

      dl->AddTriangleFilled(p00, p11, p01, fill);
      dl->AddTriangle(p00, p11, p01, stroke, thickness);
    }
  }
}

static void draw_shape(ImDrawList* dl, const Shape& s, ImVec2 origin) {
  if (s.points.empty()) {
    return;
  }

  switch (s.type) {
    case ShapeType::Line: {
      if (s.points.size() < 2) {
        return;
      }
      dl->AddLine(to_screen(s.points[0], origin), to_screen(s.points[1], origin), s.stroke_color,
                  s.stroke_thickness);
      break;
    }
    case ShapeType::Rect: {
      if (s.points.size() < 2) {
        return;
      }
      ImVec2 a = to_screen(s.points[0], origin);
      ImVec2 b = to_screen(s.points[1], origin);
      dl->AddRectFilled(a, b, s.fill_color);
      dl->AddRect(a, b, s.stroke_color, 0.0f, 0, s.stroke_thickness);
      break;
    }
    case ShapeType::Circle: {
      if (s.points.size() < 2) {
        return;
      }
      const float x0 = std::min(s.points[0].x, s.points[1].x);
      const float y0 = std::min(s.points[0].y, s.points[1].y);
      const float x1 = std::max(s.points[0].x, s.points[1].x);
      const float y1 = std::max(s.points[0].y, s.points[1].y);
      const float radius = std::min(x1 - x0, y1 - y0) * 0.5f;
      ImVec2 center = to_screen(Vec2{(x0 + x1) * 0.5f, (y0 + y1) * 0.5f}, origin);
      dl->AddCircleFilled(center, radius, s.fill_color, 0);
      dl->AddCircle(center, radius, s.stroke_color, 0, s.stroke_thickness);
      break;
    }
    case ShapeType::Polygon: {
      if (s.points.size() < 3) {
        return;
      }
      std::vector<ImVec2> tmp;
      tmp.reserve(s.points.size());
      for (const Vec2& p : s.points) {
        tmp.push_back(to_screen(p, origin));
      }
      dl->AddConcavePolyFilled(tmp.data(), static_cast<int>(tmp.size()), s.fill_color);
      dl->AddPolyline(tmp.data(), static_cast<int>(tmp.size()), s.stroke_color, ImDrawFlags_Closed,
                      s.stroke_thickness);
      break;
    }
    case ShapeType::Polyline: {
      if (s.points.size() < 2) {
        return;
      }
      std::vector<ImVec2> tmp;
      tmp.reserve(s.points.size());
      for (const Vec2& p : s.points) {
        tmp.push_back(to_screen(p, origin));
      }
      dl->AddPolyline(tmp.data(), static_cast<int>(tmp.size()), s.stroke_color,
                      s.closed ? ImDrawFlags_Closed : ImDrawFlags_None, s.stroke_thickness);
      break;
    }
    case ShapeType::TriangleMesh: {
      if (s.points.size() < 2) {
        return;
      }
      draw_triangle_grid(dl, s.points[0], s.points[1], s.rows, s.cols, s.stroke_color, s.fill_color,
                         s.stroke_thickness, origin);
      break;
    }
  }
}

static void commit_drag_shape(AppState& app, Vec2 a, Vec2 b) {
  if (distance(ImVec2(a.x, a.y), ImVec2(b.x, b.y)) < 2.0f) {
    return;
  }
  Shape s;
  s.type = app.active_tool;
  s.points = {a, b};
  s.stroke_color = stroke_color_for(app.active_tool);
  s.fill_color = fill_color_for(app.active_tool);
  s.stroke_thickness = app.stroke_thickness;
  s.closed = false;
  s.rows = app.mesh_rows;
  s.cols = app.mesh_cols;
  app.shapes.push_back(s);
}

static void commit_draft_shape(AppState& app) {
  if ((app.active_tool == ShapeType::Polygon && app.draft_points.size() >= 3) ||
      (app.active_tool == ShapeType::Polyline && app.draft_points.size() >= 2)) {
    Shape s;
    s.type = app.active_tool;
    s.points = app.draft_points;
    s.stroke_color = stroke_color_for(app.active_tool);
    s.fill_color = fill_color_for(app.active_tool);
    s.stroke_thickness = app.stroke_thickness;
    s.closed = app.active_tool == ShapeType::Polygon;
    s.rows = 0;
    s.cols = 0;
    app.shapes.push_back(s);
  }
  app.draft_points.clear();
}

static void set_tool(AppState& app, ShapeType t) {
  app.active_tool = t;
  app.dragging = false;
  app.draft_points.clear();
}

int main() {
  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);

  GLFWwindow* window = glfwCreateWindow(1280, 820, "Dear ImGui Geometry Drawer", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }
  glGetError();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }
  if (!ImGui_ImplOpenGL3_Init("#version 150")) {
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  AppState app;
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::Begin("Geometry Drawer", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextUnformatted("Tools");
    if (ImGui::Button("Line (1)")) set_tool(app, ShapeType::Line);
    ImGui::SameLine();
    if (ImGui::Button("Rect (2)")) set_tool(app, ShapeType::Rect);
    ImGui::SameLine();
    if (ImGui::Button("Circle (3)")) set_tool(app, ShapeType::Circle);
    ImGui::SameLine();
    if (ImGui::Button("Polygon (4)")) set_tool(app, ShapeType::Polygon);
    ImGui::SameLine();
    if (ImGui::Button("Polyline (5)")) set_tool(app, ShapeType::Polyline);
    ImGui::SameLine();
    if (ImGui::Button("Tri Mesh (6)")) set_tool(app, ShapeType::TriangleMesh);

    ImGui::SliderFloat("Stroke", &app.stroke_thickness, 1.0f, 8.0f, "%.1f");
    ImGui::SliderInt("Mesh Rows", &app.mesh_rows, 2, 60);
    ImGui::SliderInt("Mesh Cols", &app.mesh_cols, 2, 60);
    ImGui::Checkbox("Show stress mesh", &app.show_stress_mesh);

    if (ImGui::Button("Commit Draft (Enter)")) {
      commit_draft_shape(app);
    }
    ImGui::SameLine();
    if (ImGui::Button("Undo Draft Point (Backspace)") && !app.draft_points.empty()) {
      app.draft_points.pop_back();
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear (C)")) {
      app.shapes.clear();
      app.draft_points.clear();
    }

    ImGui::Text("Active: %s | Shapes: %d | Draft points: %d", tool_name(app.active_tool),
                static_cast<int>(app.shapes.size()), static_cast<int>(app.draft_points.size()));
    ImGui::Text("DrawData: vtx=%d idx=%d", app.last_total_vtx, app.last_total_idx);

    const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    ImVec2 canvas_size = ImGui::GetContentRegionAvail();
    if (canvas_size.x < 100.0f) canvas_size.x = 100.0f;
    if (canvas_size.y < 100.0f) canvas_size.y = 100.0f;

    ImGui::InvisibleButton("canvas", canvas_size,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 canvas_end(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
    dl->AddRectFilled(canvas_pos, canvas_end, IM_COL32(26, 27, 30, 255));
    dl->AddRect(canvas_pos, canvas_end, IM_COL32(95, 95, 110, 255));

    auto mouse_canvas_pos = from_screen(io.MousePos, canvas_pos);

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
      if (app.active_tool == ShapeType::Polygon || app.active_tool == ShapeType::Polyline) {
        app.draft_points.push_back(mouse_canvas_pos);
      } else {
        app.dragging = true;
        app.drag_start = io.MousePos;
      }
    }

    if (app.dragging && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
      commit_drag_shape(app, from_screen(app.drag_start, canvas_pos), mouse_canvas_pos);
      app.dragging = false;
    }

    if (active && ImGui::IsKeyPressed(ImGuiKey_1)) set_tool(app, ShapeType::Line);
    if (active && ImGui::IsKeyPressed(ImGuiKey_2)) set_tool(app, ShapeType::Rect);
    if (active && ImGui::IsKeyPressed(ImGuiKey_3)) set_tool(app, ShapeType::Circle);
    if (active && ImGui::IsKeyPressed(ImGuiKey_4)) set_tool(app, ShapeType::Polygon);
    if (active && ImGui::IsKeyPressed(ImGuiKey_5)) set_tool(app, ShapeType::Polyline);
    if (active && ImGui::IsKeyPressed(ImGuiKey_6)) set_tool(app, ShapeType::TriangleMesh);
    if (active && ImGui::IsKeyPressed(ImGuiKey_Enter)) commit_draft_shape(app);
    if (active && ImGui::IsKeyPressed(ImGuiKey_Backspace) && !app.draft_points.empty()) {
      app.draft_points.pop_back();
    }
    if (active && ImGui::IsKeyPressed(ImGuiKey_C)) {
      app.shapes.clear();
      app.draft_points.clear();
    }

    if (app.show_stress_mesh) {
      draw_triangle_grid(dl, Vec2{30.0f, 30.0f}, Vec2{360.0f, 300.0f}, 20, 20,
                         IM_COL32(255, 255, 255, 45), IM_COL32(255, 255, 255, 12), 0.7f,
                         canvas_pos);
    }

    for (const Shape& s : app.shapes) {
      draw_shape(dl, s, canvas_pos);
    }

    if (app.dragging && app.active_tool != ShapeType::Polygon && app.active_tool != ShapeType::Polyline) {
      Shape preview;
      preview.type = app.active_tool;
      preview.points = {from_screen(app.drag_start, canvas_pos), mouse_canvas_pos};
      preview.stroke_color = IM_COL32(255, 255, 255, 210);
      preview.fill_color = IM_COL32(255, 255, 255, 20);
      preview.stroke_thickness = 1.5f;
      preview.closed = false;
      preview.rows = app.mesh_rows;
      preview.cols = app.mesh_cols;
      draw_shape(dl, preview, canvas_pos);
    }

    if ((app.active_tool == ShapeType::Polygon || app.active_tool == ShapeType::Polyline) &&
        !app.draft_points.empty()) {
      std::vector<ImVec2> pts;
      pts.reserve(app.draft_points.size() + 1);
      for (const Vec2& p : app.draft_points) {
        pts.push_back(to_screen(p, canvas_pos));
      }
      pts.push_back(io.MousePos);
      dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), IM_COL32(255, 255, 255, 180),
                      ImDrawFlags_None, 1.5f);
      for (const Vec2& p : app.draft_points) {
        dl->AddCircleFilled(to_screen(p, canvas_pos), 3.0f, IM_COL32(255, 255, 255, 220));
      }
    }

    ImGui::End();

    ImGui::Render();
    if (ImDrawData* dd = ImGui::GetDrawData()) {
      app.last_total_vtx = dd->TotalVtxCount;
      app.last_total_idx = dd->TotalIdxCount;
    }
    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
