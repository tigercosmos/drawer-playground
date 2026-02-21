#include "MainWindow.h"

#include "ImGuiLayer.h"
#include "MetalRenderer.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QWheelEvent>

MainWindow::MainWindow() {
    setSurfaceType(QSurface::MetalSurface);
    setTitle("Qt + Metal + Dear ImGui");
    resize(1280, 800);
    setMinimumSize(QSize(900, 600));

    connect(&m_frameTimer, &QTimer::timeout, this, &MainWindow::renderFrame);
    m_frameTimer.setTimerType(Qt::PreciseTimer);
    m_frameTimer.start(16);
    m_clock.start();

    m_metal = std::make_unique<MetalRenderer>();
    m_imgui = std::make_unique<ImGuiLayer>();
}

MainWindow::~MainWindow() = default;

void MainWindow::initializeIfNeeded() {
    if (m_initialized || !isExposed()) {
        return;
    }

    if (!m_metal->initialize(this)) {
        return;
    }

    const float dpr = static_cast<float>(devicePixelRatio());
    m_metal->resize(width(), height(), dpr);

    if (!m_imgui->initialize(m_metal->device())) {
        return;
    }
    m_imgui->onResize(static_cast<float>(width()), static_cast<float>(height()), dpr, dpr);
    m_initialized = true;
}

void MainWindow::renderFrame() {
    if (!isExposed()) {
        return;
    }

    initializeIfNeeded();
    if (!m_initialized) {
        return;
    }

    const qint64 nowNs = m_clock.nsecsElapsed();
    float deltaSeconds = 1.0f / 60.0f;
    if (m_lastFrameNs > 0) {
        deltaSeconds = static_cast<float>(nowNs - m_lastFrameNs) / 1'000'000'000.0f;
    }
    m_lastFrameNs = nowNs;

    const MetalFrame frame = m_metal->beginFrame();
    if (!frame.valid) {
        return;
    }

    m_imgui->beginFrame(frame.renderPassDescriptor, deltaSeconds);
    m_imgui->buildUi();
    m_imgui->endFrame(frame.commandBuffer, frame.renderEncoder);
    m_metal->endFrame(frame);
}

void MainWindow::exposeEvent(QExposeEvent* event) {
    QWindow::exposeEvent(event);
    if (isExposed()) {
        initializeIfNeeded();
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QWindow::resizeEvent(event);
    if (!m_initialized) {
        return;
    }
    const float dpr = static_cast<float>(devicePixelRatio());
    m_metal->resize(event->size().width(), event->size().height(), dpr);
    m_imgui->onResize(static_cast<float>(event->size().width()),
                      static_cast<float>(event->size().height()),
                      dpr,
                      dpr);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (m_initialized) {
        m_imgui->handleKeyPress(event->key(), event->modifiers(), event->text());
    }
    QWindow::keyPressEvent(event);
}

void MainWindow::keyReleaseEvent(QKeyEvent* event) {
    if (m_initialized) {
        m_imgui->handleKeyRelease(event->key(), event->modifiers());
    }
    QWindow::keyReleaseEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (m_initialized) {
        m_imgui->handleMouseMove(event->position());
        m_imgui->handleMousePress(event->button());
    }
    QWindow::mousePressEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event) {
    if (m_initialized) {
        m_imgui->handleMouseMove(event->position());
        m_imgui->handleMouseRelease(event->button());
    }
    QWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
    if (m_initialized) {
        m_imgui->handleMouseMove(event->position());
    }
    QWindow::mouseMoveEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent* event) {
    if (m_initialized) {
        m_imgui->handleMouseMove(event->position());
        m_imgui->handleWheel(event->angleDelta(), event->pixelDelta());
    }
    QWindow::wheelEvent(event);
}

void MainWindow::focusOutEvent(QFocusEvent* event) {
    if (m_initialized) {
        m_imgui->handleFocusOut();
    }
    QWindow::focusOutEvent(event);
}
