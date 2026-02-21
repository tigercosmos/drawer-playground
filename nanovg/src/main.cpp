#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "nanovg.h"

#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

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
  NVGcolor stroke;
  NVGcolor fill;
  float stroke_width;
  int line_join;
  int line_cap;
  int rows;
  int cols;
  bool fill_enabled;
};

struct AppState {
  std::vector<Shape> shapes;
  ShapeType active_tool = ShapeType::Line;
  bool dragging = false;
  Vec2 drag_start{0.0f, 0.0f};
  Vec2 drag_current{0.0f, 0.0f};
  std::vector<Vec2> draft_points;
  int current_join = NVG_MITER;
  int current_cap = NVG_BUTT;
  bool show_stress_mesh = true;
  int font_id = -1;
};

static const char* tool_name(ShapeType type) {
  switch (type) {
    case ShapeType::Line:
      return "Line (1)";
    case ShapeType::Rect:
      return "Rect (2)";
    case ShapeType::Circle:
      return "Circle (3)";
    case ShapeType::Polygon:
      return "Polygon (4)";
    case ShapeType::Polyline:
      return "Polyline (5)";
    case ShapeType::TriangleMesh:
      return "Tri Mesh (6)";
  }
  return "Unknown";
}

static NVGcolor stroke_for(ShapeType type) {
  switch (type) {
    case ShapeType::Line:
      return nvgRGBA(245, 99, 84, 255);
    case ShapeType::Rect:
      return nvgRGBA(255, 196, 61, 255);
    case ShapeType::Circle:
      return nvgRGBA(66, 196, 136, 255);
    case ShapeType::Polygon:
      return nvgRGBA(61, 165, 255, 255);
    case ShapeType::Polyline:
      return nvgRGBA(204, 138, 255, 255);
    case ShapeType::TriangleMesh:
      return nvgRGBA(255, 140, 80, 255);
  }
  return nvgRGBA(255, 255, 255, 255);
}

static NVGcolor fill_for(ShapeType type) {
  switch (type) {
    case ShapeType::Line:
      return nvgRGBA(245, 99, 84, 32);
    case ShapeType::Rect:
      return nvgRGBA(255, 196, 61, 40);
    case ShapeType::Circle:
      return nvgRGBA(66, 196, 136, 40);
    case ShapeType::Polygon:
      return nvgRGBA(61, 165, 255, 48);
    case ShapeType::Polyline:
      return nvgRGBA(204, 138, 255, 28);
    case ShapeType::TriangleMesh:
      return nvgRGBA(255, 140, 80, 22);
  }
  return nvgRGBA(255, 255, 255, 32);
}

static float dist(Vec2 a, Vec2 b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

static void draw_triangle_grid(NVGcontext* vg, Vec2 a, Vec2 b, int rows, int cols, NVGcolor stroke,
                               NVGcolor fill, float stroke_width) {
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

  nvgLineJoin(vg, NVG_BEVEL);
  nvgLineCap(vg, NVG_BUTT);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const float lx = x0 + static_cast<float>(c) * cw;
      const float ty = y0 + static_cast<float>(r) * ch;
      const Vec2 p00{lx, ty};
      const Vec2 p10{lx + cw, ty};
      const Vec2 p01{lx, ty + ch};
      const Vec2 p11{lx + cw, ty + ch};

      nvgBeginPath(vg);
      nvgMoveTo(vg, p00.x, p00.y);
      nvgLineTo(vg, p10.x, p10.y);
      nvgLineTo(vg, p11.x, p11.y);
      nvgClosePath(vg);
      nvgFillColor(vg, fill);
      nvgFill(vg);
      nvgStrokeWidth(vg, stroke_width);
      nvgStrokeColor(vg, stroke);
      nvgStroke(vg);

      nvgBeginPath(vg);
      nvgMoveTo(vg, p00.x, p00.y);
      nvgLineTo(vg, p11.x, p11.y);
      nvgLineTo(vg, p01.x, p01.y);
      nvgClosePath(vg);
      nvgFillColor(vg, fill);
      nvgFill(vg);
      nvgStrokeWidth(vg, stroke_width);
      nvgStrokeColor(vg, stroke);
      nvgStroke(vg);
    }
  }
}

static void draw_shape(NVGcontext* vg, const Shape& shape) {
  if (shape.points.empty()) {
    return;
  }

  nvgStrokeWidth(vg, shape.stroke_width);
  nvgStrokeColor(vg, shape.stroke);

  switch (shape.type) {
    case ShapeType::Line: {
      if (shape.points.size() < 2) {
        return;
      }
      nvgBeginPath(vg);
      nvgMoveTo(vg, shape.points[0].x, shape.points[0].y);
      nvgLineTo(vg, shape.points[1].x, shape.points[1].y);
      nvgStroke(vg);
      break;
    }
    case ShapeType::Rect: {
      if (shape.points.size() < 2) {
        return;
      }
      const float x = std::min(shape.points[0].x, shape.points[1].x);
      const float y = std::min(shape.points[0].y, shape.points[1].y);
      const float w = std::fabs(shape.points[1].x - shape.points[0].x);
      const float h = std::fabs(shape.points[1].y - shape.points[0].y);
      nvgBeginPath(vg);
      nvgRect(vg, x, y, w, h);
      nvgFillColor(vg, shape.fill);
      nvgFill(vg);
      nvgStroke(vg);
      break;
    }
    case ShapeType::Circle: {
      if (shape.points.size() < 2) {
        return;
      }
      const Vec2 c{(shape.points[0].x + shape.points[1].x) * 0.5f,
                   (shape.points[0].y + shape.points[1].y) * 0.5f};
      const float rx = std::fabs(shape.points[1].x - shape.points[0].x) * 0.5f;
      const float ry = std::fabs(shape.points[1].y - shape.points[0].y) * 0.5f;
      const float r = std::min(rx, ry);
      nvgBeginPath(vg);
      nvgCircle(vg, c.x, c.y, r);
      nvgFillColor(vg, shape.fill);
      nvgFill(vg);
      nvgStroke(vg);
      break;
    }
    case ShapeType::Polygon: {
      if (shape.points.size() < 3) {
        return;
      }
      nvgBeginPath(vg);
      nvgMoveTo(vg, shape.points[0].x, shape.points[0].y);
      for (size_t i = 1; i < shape.points.size(); ++i) {
        nvgLineTo(vg, shape.points[i].x, shape.points[i].y);
      }
      nvgClosePath(vg);
      nvgFillColor(vg, shape.fill);
      nvgFill(vg);
      nvgStroke(vg);
      break;
    }
    case ShapeType::Polyline: {
      if (shape.points.size() < 2) {
        return;
      }
      nvgLineJoin(vg, shape.line_join);
      nvgLineCap(vg, shape.line_cap);
      nvgBeginPath(vg);
      nvgMoveTo(vg, shape.points[0].x, shape.points[0].y);
      for (size_t i = 1; i < shape.points.size(); ++i) {
        nvgLineTo(vg, shape.points[i].x, shape.points[i].y);
      }
      nvgStroke(vg);
      break;
    }
    case ShapeType::TriangleMesh: {
      if (shape.points.size() < 2) {
        return;
      }
      draw_triangle_grid(vg, shape.points[0], shape.points[1], shape.rows, shape.cols, shape.stroke,
                         shape.fill, shape.stroke_width);
      break;
    }
  }
}

static void commit_drag_shape(AppState* app) {
  Shape shape;
  shape.type = app->active_tool;
  shape.stroke = stroke_for(app->active_tool);
  shape.fill = fill_for(app->active_tool);
  shape.stroke_width = 2.0f;
  shape.line_join = app->current_join;
  shape.line_cap = app->current_cap;
  shape.rows = 10;
  shape.cols = 10;
  shape.fill_enabled = true;
  shape.points = {app->drag_start, app->drag_current};

  if (dist(app->drag_start, app->drag_current) < 2.0f) {
    return;
  }

  app->shapes.push_back(shape);
}

static void commit_draft_shape(AppState* app) {
  if ((app->active_tool == ShapeType::Polygon && app->draft_points.size() >= 3) ||
      (app->active_tool == ShapeType::Polyline && app->draft_points.size() >= 2)) {
    Shape shape;
    shape.type = app->active_tool;
    shape.stroke = stroke_for(app->active_tool);
    shape.fill = fill_for(app->active_tool);
    shape.stroke_width = 2.5f;
    shape.line_join = app->current_join;
    shape.line_cap = app->current_cap;
    shape.rows = 0;
    shape.cols = 0;
    shape.fill_enabled = app->active_tool == ShapeType::Polygon;
    shape.points = app->draft_points;
    app->shapes.push_back(shape);
  }
  app->draft_points.clear();
}

static void cursor_cb(GLFWwindow* window, double x, double y) {
  auto* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
  if (!app) {
    return;
  }
  app->drag_current = Vec2{static_cast<float>(x), static_cast<float>(y)};
}

static void mouse_button_cb(GLFWwindow* window, int button, int action, int mods) {
  (void)mods;
  auto* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
  if (!app || button != GLFW_MOUSE_BUTTON_LEFT) {
    return;
  }

  double x = 0.0;
  double y = 0.0;
  glfwGetCursorPos(window, &x, &y);
  const Vec2 p{static_cast<float>(x), static_cast<float>(y)};

  if (action == GLFW_PRESS) {
    if (app->active_tool == ShapeType::Polygon || app->active_tool == ShapeType::Polyline) {
      app->draft_points.push_back(p);
    } else {
      app->dragging = true;
      app->drag_start = p;
      app->drag_current = p;
    }
  }

  if (action == GLFW_RELEASE) {
    if (app->dragging) {
      app->drag_current = p;
      commit_drag_shape(app);
      app->dragging = false;
    }
  }
}

static void set_tool(AppState* app, ShapeType type) {
  app->active_tool = type;
  app->dragging = false;
  app->draft_points.clear();
  std::printf("tool: %s\n", tool_name(type));
}

static void key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
  (void)scancode;
  (void)mods;

  auto* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
  if (!app || action != GLFW_PRESS) {
    return;
  }

  switch (key) {
    case GLFW_KEY_ESCAPE:
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      break;
    case GLFW_KEY_1:
      set_tool(app, ShapeType::Line);
      break;
    case GLFW_KEY_2:
      set_tool(app, ShapeType::Rect);
      break;
    case GLFW_KEY_3:
      set_tool(app, ShapeType::Circle);
      break;
    case GLFW_KEY_4:
      set_tool(app, ShapeType::Polygon);
      break;
    case GLFW_KEY_5:
      set_tool(app, ShapeType::Polyline);
      break;
    case GLFW_KEY_6:
      set_tool(app, ShapeType::TriangleMesh);
      break;
    case GLFW_KEY_ENTER:
    case GLFW_KEY_KP_ENTER:
      commit_draft_shape(app);
      break;
    case GLFW_KEY_BACKSPACE:
      if (!app->draft_points.empty()) {
        app->draft_points.pop_back();
      }
      break;
    case GLFW_KEY_C:
      app->shapes.clear();
      app->draft_points.clear();
      break;
    case GLFW_KEY_J:
      if (app->current_join == NVG_MITER) {
        app->current_join = NVG_BEVEL;
      } else if (app->current_join == NVG_BEVEL) {
        app->current_join = NVG_ROUND;
      } else {
        app->current_join = NVG_MITER;
      }
      break;
    case GLFW_KEY_K:
      if (app->current_cap == NVG_BUTT) {
        app->current_cap = NVG_SQUARE;
      } else if (app->current_cap == NVG_SQUARE) {
        app->current_cap = NVG_ROUND;
      } else {
        app->current_cap = NVG_BUTT;
      }
      break;
    case GLFW_KEY_M:
      app->show_stress_mesh = !app->show_stress_mesh;
      break;
    default:
      break;
  }
}

static void error_cb(int code, const char* desc) {
  std::fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}

static int try_load_font(NVGcontext* vg) {
  const std::array<const char*, 2> candidates = {
      "third_party/nanovg/example/Roboto-Regular.ttf",
      "../third_party/nanovg/example/Roboto-Regular.ttf",
  };

  for (const char* path : candidates) {
    const int id = nvgCreateFont(vg, "ui", path);
    if (id >= 0) {
      return id;
    }
  }
  return -1;
}

static void draw_draft_overlay(NVGcontext* vg, const AppState& app) {
  if (app.dragging && app.active_tool != ShapeType::Polygon && app.active_tool != ShapeType::Polyline) {
    Shape preview;
    preview.type = app.active_tool;
    preview.points = {app.drag_start, app.drag_current};
    preview.stroke = nvgRGBA(255, 255, 255, 220);
    preview.fill = nvgRGBA(255, 255, 255, 20);
    preview.stroke_width = 1.5f;
    preview.line_join = app.current_join;
    preview.line_cap = app.current_cap;
    preview.rows = 10;
    preview.cols = 10;
    preview.fill_enabled = true;
    draw_shape(vg, preview);
  }

  if ((app.active_tool == ShapeType::Polygon || app.active_tool == ShapeType::Polyline) &&
      !app.draft_points.empty()) {
    nvgLineJoin(vg, app.current_join);
    nvgLineCap(vg, app.current_cap);

    nvgBeginPath(vg);
    nvgMoveTo(vg, app.draft_points[0].x, app.draft_points[0].y);
    for (size_t i = 1; i < app.draft_points.size(); ++i) {
      nvgLineTo(vg, app.draft_points[i].x, app.draft_points[i].y);
    }
    nvgLineTo(vg, app.drag_current.x, app.drag_current.y);
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 180));
    nvgStroke(vg);

    for (const Vec2& p : app.draft_points) {
      nvgBeginPath(vg);
      nvgCircle(vg, p.x, p.y, 3.0f);
      nvgFillColor(vg, nvgRGBA(255, 255, 255, 220));
      nvgFill(vg);
    }
  }
}

int main() {
  glfwSetErrorCallback(error_cb);
  if (!glfwInit()) {
    std::fprintf(stderr, "Failed to initialize GLFW\n");
    return 1;
  }

#ifndef _WIN32
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
  glfwWindowHint(GLFW_STENCIL_BITS, 8);
  glfwWindowHint(GLFW_SAMPLES, 4);

  GLFWwindow* window = glfwCreateWindow(1200, 800, "NanoVG Geometry Drawer", nullptr, nullptr);
  if (!window) {
    std::fprintf(stderr, "Failed to create window\n");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    std::fprintf(stderr, "Failed to initialize GLEW\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }
  glGetError();

  NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
  if (!vg) {
    std::fprintf(stderr, "Failed to create NanoVG GL3 context\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  AppState app;
  app.font_id = try_load_font(vg);

  glfwSetWindowUserPointer(window, &app);
  glfwSetCursorPosCallback(window, cursor_cb);
  glfwSetMouseButtonCallback(window, mouse_button_cb);
  glfwSetKeyCallback(window, key_cb);

  std::printf("Controls: 1-6 tools, Enter commit polygon/polyline, Backspace remove point, J/K join/cap, M stress mesh, C clear, Esc quit\n");

  while (!glfwWindowShouldClose(window)) {
    int win_w = 0;
    int win_h = 0;
    int fb_w = 0;
    int fb_h = 0;

    glfwGetWindowSize(window, &win_w, &win_h);
    glfwGetFramebufferSize(window, &fb_w, &fb_h);

    const float px_ratio = (win_w > 0) ? static_cast<float>(fb_w) / static_cast<float>(win_w) : 1.0f;

    glViewport(0, 0, fb_w, fb_h);
    glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg, static_cast<float>(win_w), static_cast<float>(win_h), px_ratio);

    if (app.show_stress_mesh) {
      draw_triangle_grid(vg, Vec2{40.0f, 70.0f}, Vec2{360.0f, 360.0f}, 20, 20,
                         nvgRGBA(255, 255, 255, 50), nvgRGBA(255, 255, 255, 8), 0.7f);
    }

    for (const Shape& shape : app.shapes) {
      draw_shape(vg, shape);
    }

    draw_draft_overlay(vg, app);

    if (app.font_id >= 0) {
      nvgFontFace(vg, "ui");
      nvgFontSize(vg, 16.0f);
      nvgFillColor(vg, nvgRGBA(230, 230, 230, 220));

      std::string hud = std::string("Tool: ") + tool_name(app.active_tool) +
                        " | Shapes: " + std::to_string(app.shapes.size()) +
                        " | Draft points: " + std::to_string(app.draft_points.size()) +
                        " | Join(J)/Cap(K) | Mesh(M): " +
                        (app.show_stress_mesh ? "on" : "off");
      nvgText(vg, 20.0f, 30.0f, hud.c_str(), nullptr);
    }

    nvgEndFrame(vg);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  nvgDeleteGL3(vg);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
