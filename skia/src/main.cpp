#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#include <skia/include/core/SkCanvas.h>
#include <skia/include/core/SkColor.h>
#include <skia/include/core/SkColorSpace.h>
#include <skia/include/core/SkFont.h>
#include <skia/include/core/SkPaint.h>
#include <skia/include/core/SkPath.h>
#include <skia/include/core/SkRefCnt.h>
#include <skia/include/core/SkSurface.h>
#include <skia/include/gpu/ganesh/GrBackendSurface.h>
#include <skia/include/gpu/ganesh/GrDirectContext.h>
#include <skia/include/gpu/ganesh/SkSurfaceGanesh.h>
#include <skia/include/gpu/ganesh/gl/GrGLBackendSurface.h>
#include <skia/include/gpu/ganesh/gl/GrGLDirectContext.h>
#include <skia/include/gpu/ganesh/gl/GrGLInterface.h>

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
  SkColor stroke;
  SkColor fill;
  float stroke_width;
  bool closed;
  int rows;
  int cols;
};

struct AppState {
  std::vector<Shape> shapes;
  ShapeType active_tool = ShapeType::Line;
  bool dragging = false;
  Vec2 drag_start{0.0f, 0.0f};
  Vec2 drag_current{0.0f, 0.0f};
  std::vector<Vec2> draft_points;
  bool show_stress_mesh = true;
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

static SkColor stroke_for(ShapeType type) {
  switch (type) {
    case ShapeType::Line:
      return SkColorSetARGB(255, 245, 99, 84);
    case ShapeType::Rect:
      return SkColorSetARGB(255, 255, 196, 61);
    case ShapeType::Circle:
      return SkColorSetARGB(255, 66, 196, 136);
    case ShapeType::Polygon:
      return SkColorSetARGB(255, 61, 165, 255);
    case ShapeType::Polyline:
      return SkColorSetARGB(255, 204, 138, 255);
    case ShapeType::TriangleMesh:
      return SkColorSetARGB(255, 255, 140, 80);
  }
  return SK_ColorWHITE;
}

static SkColor fill_for(ShapeType type) {
  switch (type) {
    case ShapeType::Line:
      return SkColorSetARGB(32, 245, 99, 84);
    case ShapeType::Rect:
      return SkColorSetARGB(44, 255, 196, 61);
    case ShapeType::Circle:
      return SkColorSetARGB(44, 66, 196, 136);
    case ShapeType::Polygon:
      return SkColorSetARGB(50, 61, 165, 255);
    case ShapeType::Polyline:
      return SkColorSetARGB(34, 204, 138, 255);
    case ShapeType::TriangleMesh:
      return SkColorSetARGB(26, 255, 140, 80);
  }
  return SkColorSetARGB(32, 255, 255, 255);
}

static float distance(Vec2 a, Vec2 b) {
  const float dx = a.x - b.x;
  const float dy = a.y - b.y;
  return std::sqrt(dx * dx + dy * dy);
}

static void draw_triangle_grid(SkCanvas* canvas, Vec2 a, Vec2 b, int rows, int cols, SkColor stroke,
                               SkColor fill, float stroke_width) {
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

  SkPaint fill_paint;
  fill_paint.setAntiAlias(true);
  fill_paint.setStyle(SkPaint::kFill_Style);
  fill_paint.setColor(fill);

  SkPaint stroke_paint;
  stroke_paint.setAntiAlias(true);
  stroke_paint.setStyle(SkPaint::kStroke_Style);
  stroke_paint.setColor(stroke);
  stroke_paint.setStrokeWidth(stroke_width);

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      const float lx = x0 + static_cast<float>(c) * cw;
      const float ty = y0 + static_cast<float>(r) * ch;
      const Vec2 p00{lx, ty};
      const Vec2 p10{lx + cw, ty};
      const Vec2 p01{lx, ty + ch};
      const Vec2 p11{lx + cw, ty + ch};

      SkPath t0;
      t0.moveTo(p00.x, p00.y);
      t0.lineTo(p10.x, p10.y);
      t0.lineTo(p11.x, p11.y);
      t0.close();
      canvas->drawPath(t0, fill_paint);
      canvas->drawPath(t0, stroke_paint);

      SkPath t1;
      t1.moveTo(p00.x, p00.y);
      t1.lineTo(p11.x, p11.y);
      t1.lineTo(p01.x, p01.y);
      t1.close();
      canvas->drawPath(t1, fill_paint);
      canvas->drawPath(t1, stroke_paint);
    }
  }
}

static void draw_shape(SkCanvas* canvas, const Shape& shape) {
  if (shape.points.empty()) {
    return;
  }

  SkPaint stroke;
  stroke.setAntiAlias(true);
  stroke.setStyle(SkPaint::kStroke_Style);
  stroke.setColor(shape.stroke);
  stroke.setStrokeWidth(shape.stroke_width);
  stroke.setStrokeCap(SkPaint::kRound_Cap);
  stroke.setStrokeJoin(SkPaint::kRound_Join);

  SkPaint fill;
  fill.setAntiAlias(true);
  fill.setStyle(SkPaint::kFill_Style);
  fill.setColor(shape.fill);

  switch (shape.type) {
    case ShapeType::Line: {
      if (shape.points.size() < 2) return;
      canvas->drawLine(shape.points[0].x, shape.points[0].y, shape.points[1].x, shape.points[1].y,
                       stroke);
      break;
    }
    case ShapeType::Rect: {
      if (shape.points.size() < 2) return;
      const SkRect r = SkRect::MakeLTRB(std::min(shape.points[0].x, shape.points[1].x),
                                        std::min(shape.points[0].y, shape.points[1].y),
                                        std::max(shape.points[0].x, shape.points[1].x),
                                        std::max(shape.points[0].y, shape.points[1].y));
      canvas->drawRect(r, fill);
      canvas->drawRect(r, stroke);
      break;
    }
    case ShapeType::Circle: {
      if (shape.points.size() < 2) return;
      const float x0 = std::min(shape.points[0].x, shape.points[1].x);
      const float y0 = std::min(shape.points[0].y, shape.points[1].y);
      const float x1 = std::max(shape.points[0].x, shape.points[1].x);
      const float y1 = std::max(shape.points[0].y, shape.points[1].y);
      const float cx = (x0 + x1) * 0.5f;
      const float cy = (y0 + y1) * 0.5f;
      const float radius = std::min(x1 - x0, y1 - y0) * 0.5f;
      canvas->drawCircle(cx, cy, radius, fill);
      canvas->drawCircle(cx, cy, radius, stroke);
      break;
    }
    case ShapeType::Polygon: {
      if (shape.points.size() < 3) return;
      SkPath path;
      path.moveTo(shape.points[0].x, shape.points[0].y);
      for (size_t i = 1; i < shape.points.size(); ++i) {
        path.lineTo(shape.points[i].x, shape.points[i].y);
      }
      path.close();
      canvas->drawPath(path, fill);
      canvas->drawPath(path, stroke);
      break;
    }
    case ShapeType::Polyline: {
      if (shape.points.size() < 2) return;
      SkPath path;
      path.moveTo(shape.points[0].x, shape.points[0].y);
      for (size_t i = 1; i < shape.points.size(); ++i) {
        path.lineTo(shape.points[i].x, shape.points[i].y);
      }
      if (shape.closed) {
        path.close();
      }
      canvas->drawPath(path, stroke);
      break;
    }
    case ShapeType::TriangleMesh: {
      if (shape.points.size() < 2) return;
      draw_triangle_grid(canvas, shape.points[0], shape.points[1], shape.rows, shape.cols,
                         shape.stroke, shape.fill, shape.stroke_width);
      break;
    }
  }
}

static void commit_drag_shape(AppState* app) {
  if (distance(app->drag_start, app->drag_current) < 2.0f) {
    return;
  }
  Shape s;
  s.type = app->active_tool;
  s.points = {app->drag_start, app->drag_current};
  s.stroke = stroke_for(app->active_tool);
  s.fill = fill_for(app->active_tool);
  s.stroke_width = 2.0f;
  s.closed = false;
  s.rows = 10;
  s.cols = 10;
  app->shapes.push_back(s);
}

static void commit_draft_shape(AppState* app) {
  if ((app->active_tool == ShapeType::Polygon && app->draft_points.size() >= 3) ||
      (app->active_tool == ShapeType::Polyline && app->draft_points.size() >= 2)) {
    Shape s;
    s.type = app->active_tool;
    s.points = app->draft_points;
    s.stroke = stroke_for(app->active_tool);
    s.fill = fill_for(app->active_tool);
    s.stroke_width = 2.5f;
    s.closed = app->active_tool == ShapeType::Polygon;
    s.rows = 0;
    s.cols = 0;
    app->shapes.push_back(s);
  }
  app->draft_points.clear();
}

static void cursor_cb(GLFWwindow* window, double x, double y) {
  auto* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
  if (!app) return;
  app->drag_current = Vec2{static_cast<float>(x), static_cast<float>(y)};
}

static void mouse_button_cb(GLFWwindow* window, int button, int action, int mods) {
  (void)mods;
  auto* app = static_cast<AppState*>(glfwGetWindowUserPointer(window));
  if (!app || button != GLFW_MOUSE_BUTTON_LEFT) return;

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

  if (action == GLFW_RELEASE && app->dragging) {
    app->drag_current = p;
    commit_drag_shape(app);
    app->dragging = false;
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
  if (!app || action != GLFW_PRESS) return;

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
      if (!app->draft_points.empty()) app->draft_points.pop_back();
      break;
    case GLFW_KEY_C:
      app->shapes.clear();
      app->draft_points.clear();
      break;
    case GLFW_KEY_M:
      app->show_stress_mesh = !app->show_stress_mesh;
      break;
    default:
      break;
  }
}

static void draw_draft_overlay(SkCanvas* canvas, const AppState& app) {
  if (app.dragging && app.active_tool != ShapeType::Polygon && app.active_tool != ShapeType::Polyline) {
    Shape preview;
    preview.type = app.active_tool;
    preview.points = {app.drag_start, app.drag_current};
    preview.stroke = SkColorSetARGB(220, 255, 255, 255);
    preview.fill = SkColorSetARGB(24, 255, 255, 255);
    preview.stroke_width = 1.5f;
    preview.closed = false;
    preview.rows = 10;
    preview.cols = 10;
    draw_shape(canvas, preview);
  }

  if ((app.active_tool == ShapeType::Polygon || app.active_tool == ShapeType::Polyline) &&
      !app.draft_points.empty()) {
    SkPaint line;
    line.setAntiAlias(true);
    line.setStyle(SkPaint::kStroke_Style);
    line.setColor(SkColorSetARGB(180, 255, 255, 255));
    line.setStrokeWidth(1.5f);

    SkPath p;
    p.moveTo(app.draft_points[0].x, app.draft_points[0].y);
    for (size_t i = 1; i < app.draft_points.size(); ++i) {
      p.lineTo(app.draft_points[i].x, app.draft_points[i].y);
    }
    p.lineTo(app.drag_current.x, app.drag_current.y);
    canvas->drawPath(p, line);

    SkPaint pt;
    pt.setAntiAlias(true);
    pt.setStyle(SkPaint::kFill_Style);
    pt.setColor(SkColorSetARGB(220, 255, 255, 255));
    for (const Vec2& v : app.draft_points) {
      canvas->drawCircle(v.x, v.y, 3.0f, pt);
    }
  }
}

int main() {
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

  GLFWwindow* window = glfwCreateWindow(1200, 800, "Skia Geometry Drawer", nullptr, nullptr);
  if (!window) {
    std::fprintf(stderr, "Failed to create window\n");
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);

  glGetError();

  sk_sp<const GrGLInterface> gl_interface = GrGLMakeNativeInterface();
  if (!gl_interface) {
    std::fprintf(stderr, "Failed to create native GL interface for Skia\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  sk_sp<GrDirectContext> gr = GrDirectContexts::MakeGL(gl_interface);
  if (!gr) {
    std::fprintf(stderr, "Failed to create Skia Ganesh context\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }

  AppState app;
  glfwSetWindowUserPointer(window, &app);
  glfwSetCursorPosCallback(window, cursor_cb);
  glfwSetMouseButtonCallback(window, mouse_button_cb);
  glfwSetKeyCallback(window, key_cb);

  std::printf("Controls: 1-6 tools, Enter commit polygon/polyline, Backspace remove point, M stress mesh, C clear, Esc quit\n");

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    int win_w = 0;
    int win_h = 0;
    int fb_w = 0;
    int fb_h = 0;
    glfwGetWindowSize(window, &win_w, &win_h);
    glfwGetFramebufferSize(window, &fb_w, &fb_h);

    GLint fbo = 0;
    GLint stencil_bits = static_cast<GLint>(glfwGetWindowAttrib(window, GLFW_STENCIL_BITS));
    GLint samples = static_cast<GLint>(glfwGetWindowAttrib(window, GLFW_SAMPLES));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);

    GrGLFramebufferInfo fb_info;
    fb_info.fFBOID = static_cast<GrGLuint>(fbo);
    fb_info.fFormat = GL_RGBA8;

    const GrBackendRenderTarget backend_rt =
        GrBackendRenderTargets::MakeGL(fb_w, fb_h, static_cast<int>(samples), static_cast<int>(stencil_bits),
                                       fb_info);

    sk_sp<SkSurface> surface = SkSurfaces::WrapBackendRenderTarget(
        gr.get(), backend_rt, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, nullptr);

    if (!surface) {
      std::fprintf(stderr, "Failed to wrap framebuffer into SkSurface\n");
      break;
    }

    SkCanvas* canvas = surface->getCanvas();
    canvas->clear(SkColorSetARGB(255, 26, 28, 33));

    if (app.show_stress_mesh) {
      draw_triangle_grid(canvas, Vec2{40.0f, 70.0f}, Vec2{360.0f, 360.0f}, 20, 20,
                         SkColorSetARGB(50, 255, 255, 255), SkColorSetARGB(10, 255, 255, 255), 0.7f);
    }

    for (const Shape& s : app.shapes) {
      draw_shape(canvas, s);
    }
    draw_draft_overlay(canvas, app);

    SkFont font;
    font.setSize(16.0f);
    SkPaint text;
    text.setAntiAlias(true);
    text.setColor(SkColorSetARGB(220, 230, 230, 230));
    std::string hud = std::string("Tool: ") + tool_name(app.active_tool) +
                      " | Shapes: " + std::to_string(app.shapes.size()) +
                      " | Draft points: " + std::to_string(app.draft_points.size()) +
                      " | Mesh(M): " + (app.show_stress_mesh ? "on" : "off");
    canvas->drawSimpleText(hud.c_str(), hud.size(), SkTextEncoding::kUTF8, 20.0f, 30.0f, font, text);

    skgpu::ganesh::FlushAndSubmit(surface);

    glfwSwapBuffers(window);
  }

  gr.reset();
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
