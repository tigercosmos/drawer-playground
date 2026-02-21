#pragma once

#include <QElapsedTimer>
#include <QTimer>
#include <QWindow>

#include <memory>

class ImGuiLayer;
class MetalRenderer;

class MainWindow : public QWindow {
public:
    MainWindow();
    ~MainWindow() override;

protected:
    void exposeEvent(QExposeEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private:
    void initializeIfNeeded();
    void renderFrame();

    std::unique_ptr<MetalRenderer> m_metal;
    std::unique_ptr<ImGuiLayer> m_imgui;
    QTimer m_frameTimer;
    QElapsedTimer m_clock;
    qint64 m_lastFrameNs = 0;
    bool m_initialized = false;
};
