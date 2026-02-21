#include "ImGuiLayer.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "imgui.h"
#include "backends/imgui_impl_metal.h"

#import <Metal/Metal.h>

namespace {
ImGuiKey qtKeyToImGuiKey(int key) {
    switch (key) {
    case Qt::Key_Tab: return ImGuiKey_Tab;
    case Qt::Key_Left: return ImGuiKey_LeftArrow;
    case Qt::Key_Right: return ImGuiKey_RightArrow;
    case Qt::Key_Up: return ImGuiKey_UpArrow;
    case Qt::Key_Down: return ImGuiKey_DownArrow;
    case Qt::Key_PageUp: return ImGuiKey_PageUp;
    case Qt::Key_PageDown: return ImGuiKey_PageDown;
    case Qt::Key_Home: return ImGuiKey_Home;
    case Qt::Key_End: return ImGuiKey_End;
    case Qt::Key_Insert: return ImGuiKey_Insert;
    case Qt::Key_Delete: return ImGuiKey_Delete;
    case Qt::Key_Backspace: return ImGuiKey_Backspace;
    case Qt::Key_Space: return ImGuiKey_Space;
    case Qt::Key_Enter: return ImGuiKey_Enter;
    case Qt::Key_Return: return ImGuiKey_Enter;
    case Qt::Key_Escape: return ImGuiKey_Escape;
    case Qt::Key_A: return ImGuiKey_A;
    case Qt::Key_C: return ImGuiKey_C;
    case Qt::Key_V: return ImGuiKey_V;
    case Qt::Key_X: return ImGuiKey_X;
    case Qt::Key_Y: return ImGuiKey_Y;
    case Qt::Key_Z: return ImGuiKey_Z;
    case Qt::Key_0: return ImGuiKey_0;
    case Qt::Key_1: return ImGuiKey_1;
    case Qt::Key_2: return ImGuiKey_2;
    case Qt::Key_3: return ImGuiKey_3;
    case Qt::Key_4: return ImGuiKey_4;
    case Qt::Key_5: return ImGuiKey_5;
    case Qt::Key_6: return ImGuiKey_6;
    case Qt::Key_7: return ImGuiKey_7;
    case Qt::Key_8: return ImGuiKey_8;
    case Qt::Key_9: return ImGuiKey_9;
    default: return ImGuiKey_None;
    }
}

int qtMouseButtonToImGui(Qt::MouseButton button) {
    switch (button) {
    case Qt::LeftButton: return 0;
    case Qt::RightButton: return 1;
    case Qt::MiddleButton: return 2;
    default: return -1;
    }
}
} // namespace

ImGuiLayer::ImGuiLayer() = default;

ImGuiLayer::~ImGuiLayer() {
    shutdown();
}

bool ImGuiLayer::initialize(void* metalDevice) {
    if (m_initialized || metalDevice == nullptr) {
        return m_initialized;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    if (!ImGui_ImplMetal_Init((__bridge id<MTLDevice>)metalDevice)) {
        ImGui::DestroyContext();
        return false;
    }

    m_initialized = true;
    return true;
}

void ImGuiLayer::shutdown() {
    if (!m_initialized) {
        return;
    }
    ImGui_ImplMetal_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
}

void ImGuiLayer::onResize(float width, float height, float framebufferScaleX, float framebufferScaleY) {
    m_displayWidth = width;
    m_displayHeight = height;
    m_fbScaleX = framebufferScaleX;
    m_fbScaleY = framebufferScaleY;
}

void ImGuiLayer::beginFrame(void* renderPassDescriptor, float deltaSeconds) {
    if (!m_initialized || renderPassDescriptor == nullptr) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(m_displayWidth, m_displayHeight);
    io.DisplayFramebufferScale = ImVec2(m_fbScaleX, m_fbScaleY);
    io.DeltaTime = (deltaSeconds > 0.0f) ? deltaSeconds : (1.0f / 60.0f);

    ImGui_ImplMetal_NewFrame((__bridge MTLRenderPassDescriptor*)renderPassDescriptor);
    ImGui::NewFrame();
}

void ImGuiLayer::buildUi() {
    if (!m_initialized) {
        return;
    }

    static bool showDemoWindow = true;
    ImGui::Begin("Inspector");
    ImGui::TextUnformatted("Qt + Metal + Dear ImGui");
    ImGui::Checkbox("Show ImGui Demo", &showDemoWindow);
    ImGui::TextUnformatted("Middle-drag to pan, wheel to zoom in canvas.");
    ImGui::End();

    if (showDemoWindow) {
        ImGui::ShowDemoWindow(&showDemoWindow);
    }

    ImGui::Begin("Geometry Canvas");
    const ImVec2 canvasOrigin = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    canvasSize.x = std::max(canvasSize.x, 300.0f);
    canvasSize.y = std::max(canvasSize.y, 220.0f);

    ImGui::InvisibleButton("canvas", canvasSize, ImGuiButtonFlags_MouseButtonMiddle);
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    static ImVec2 pan(0.0f, 0.0f);
    static float zoom = 1.0f;

    ImGuiIO& io = ImGui::GetIO();
    if (hovered && io.MouseWheel != 0.0f) {
        const float oldZoom = zoom;
        zoom = std::clamp(zoom * (1.0f + io.MouseWheel * 0.1f), 0.2f, 8.0f);
        const ImVec2 mouse = io.MousePos;
        const ImVec2 before = ImVec2((mouse.x - canvasOrigin.x - pan.x) / oldZoom,
                                     (mouse.y - canvasOrigin.y - pan.y) / oldZoom);
        pan.x = mouse.x - canvasOrigin.x - before.x * zoom;
        pan.y = mouse.y - canvasOrigin.y - before.y * zoom;
    }
    if (active && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f)) {
        pan.x += io.MouseDelta.x;
        pan.y += io.MouseDelta.y;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasMax(canvasOrigin.x + canvasSize.x, canvasOrigin.y + canvasSize.y);
    drawList->AddRectFilled(canvasOrigin, canvasMax, IM_COL32(25, 28, 34, 255));
    drawList->AddRect(canvasOrigin, canvasMax, IM_COL32(70, 75, 85, 255));

    const float gridStep = 50.0f * zoom;
    if (gridStep > 4.0f) {
        for (float x = std::fmodf(pan.x, gridStep); x < canvasSize.x; x += gridStep) {
            drawList->AddLine(ImVec2(canvasOrigin.x + x, canvasOrigin.y),
                              ImVec2(canvasOrigin.x + x, canvasOrigin.y + canvasSize.y),
                              IM_COL32(45, 50, 58, 255));
        }
        for (float y = std::fmodf(pan.y, gridStep); y < canvasSize.y; y += gridStep) {
            drawList->AddLine(ImVec2(canvasOrigin.x, canvasOrigin.y + y),
                              ImVec2(canvasOrigin.x + canvasSize.x, canvasOrigin.y + y),
                              IM_COL32(45, 50, 58, 255));
        }
    }

    auto worldToScreen = [&](float wx, float wy) -> ImVec2 {
        return ImVec2(canvasOrigin.x + pan.x + wx * zoom, canvasOrigin.y + pan.y + wy * zoom);
    };

    drawList->AddLine(worldToScreen(-150.0f, -70.0f), worldToScreen(180.0f, 90.0f),
                      IM_COL32(0, 200, 255, 255), 2.0f);
    drawList->AddLine(worldToScreen(-50.0f, 130.0f), worldToScreen(220.0f, -110.0f),
                      IM_COL32(255, 170, 0, 255), 2.0f);
    drawList->AddCircle(worldToScreen(0.0f, 0.0f), 70.0f * zoom, IM_COL32(120, 255, 120, 255), 64, 2.0f);
    drawList->AddCircleFilled(worldToScreen(140.0f, 40.0f), 24.0f * zoom, IM_COL32(255, 90, 140, 200), 40);

    ImGui::SetCursorScreenPos(ImVec2(canvasOrigin.x, canvasMax.y + 8.0f));
    ImGui::Text("Zoom: %.2f | Pan: (%.1f, %.1f)", zoom, pan.x, pan.y);
    ImGui::End();
}

void ImGuiLayer::endFrame(void* commandBuffer, void* renderEncoder) {
    if (!m_initialized || commandBuffer == nullptr || renderEncoder == nullptr) {
        return;
    }

    ImGui::Render();
    ImGui_ImplMetal_RenderDrawData(
        ImGui::GetDrawData(),
        (__bridge id<MTLCommandBuffer>)commandBuffer,
        (__bridge id<MTLRenderCommandEncoder>)renderEncoder);
}

void ImGuiLayer::handleMouseMove(const QPointF& pos) {
    if (!m_initialized) {
        return;
    }
    ImGui::GetIO().AddMousePosEvent(static_cast<float>(pos.x()), static_cast<float>(pos.y()));
}

void ImGuiLayer::handleMousePress(Qt::MouseButton button) {
    if (!m_initialized) {
        return;
    }
    const int idx = qtMouseButtonToImGui(button);
    if (idx >= 0) {
        ImGui::GetIO().AddMouseButtonEvent(idx, true);
    }
}

void ImGuiLayer::handleMouseRelease(Qt::MouseButton button) {
    if (!m_initialized) {
        return;
    }
    const int idx = qtMouseButtonToImGui(button);
    if (idx >= 0) {
        ImGui::GetIO().AddMouseButtonEvent(idx, false);
    }
}

void ImGuiLayer::handleWheel(const QPointF& angleDelta, const QPointF& pixelDelta) {
    if (!m_initialized) {
        return;
    }
    const float wheelX = static_cast<float>(angleDelta.x() / 120.0);
    const float wheelY = static_cast<float>(angleDelta.y() / 120.0);
    const float pixelX = static_cast<float>(pixelDelta.x() / 100.0);
    const float pixelY = static_cast<float>(pixelDelta.y() / 100.0);
    ImGui::GetIO().AddMouseWheelEvent(wheelX + pixelX, wheelY + pixelY);
}

void ImGuiLayer::handleKeyPress(int qtKey, Qt::KeyboardModifiers modifiers, const QString& text) {
    if (!m_initialized) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    updateModifierKeys(modifiers);

    const ImGuiKey key = qtKeyToImGuiKey(qtKey);
    if (key != ImGuiKey_None) {
        io.AddKeyEvent(key, true);
    }
    if (!text.isEmpty()) {
        const QByteArray utf8 = text.toUtf8();
        io.AddInputCharactersUTF8(utf8.constData());
    }
}

void ImGuiLayer::handleKeyRelease(int qtKey, Qt::KeyboardModifiers modifiers) {
    if (!m_initialized) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    updateModifierKeys(modifiers);

    const ImGuiKey key = qtKeyToImGuiKey(qtKey);
    if (key != ImGuiKey_None) {
        io.AddKeyEvent(key, false);
    }
}

void ImGuiLayer::handleFocusOut() {
    if (!m_initialized) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    io.AddMouseButtonEvent(0, false);
    io.AddMouseButtonEvent(1, false);
    io.AddMouseButtonEvent(2, false);
}

void ImGuiLayer::updateModifierKeys(Qt::KeyboardModifiers modifiers) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (modifiers & Qt::ControlModifier) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (modifiers & Qt::ShiftModifier) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (modifiers & Qt::AltModifier) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (modifiers & Qt::MetaModifier) != 0);
}
