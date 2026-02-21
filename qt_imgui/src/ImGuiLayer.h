#pragma once

#include <QPointF>
#include <QString>
#include <Qt>

class ImGuiLayer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    bool initialize(void* metalDevice);
    void shutdown();

    void onResize(float width, float height, float framebufferScaleX, float framebufferScaleY);
    void beginFrame(void* renderPassDescriptor, float deltaSeconds);
    void buildUi();
    void endFrame(void* commandBuffer, void* renderEncoder);

    void handleMouseMove(const QPointF& pos);
    void handleMousePress(Qt::MouseButton button);
    void handleMouseRelease(Qt::MouseButton button);
    void handleWheel(const QPointF& angleDelta, const QPointF& pixelDelta);
    void handleKeyPress(int qtKey, Qt::KeyboardModifiers modifiers, const QString& text);
    void handleKeyRelease(int qtKey, Qt::KeyboardModifiers modifiers);
    void handleFocusOut();

private:
    void updateModifierKeys(Qt::KeyboardModifiers modifiers);
    bool m_initialized = false;
    float m_displayWidth = 1280.0f;
    float m_displayHeight = 720.0f;
    float m_fbScaleX = 1.0f;
    float m_fbScaleY = 1.0f;
};
