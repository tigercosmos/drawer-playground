#pragma once

#include <memory>

class QWindow;

struct MetalFrame {
    void* renderPassDescriptor = nullptr;
    void* commandBuffer = nullptr;
    void* renderEncoder = nullptr;
    bool valid = false;
};

class MetalRenderer {
public:
    MetalRenderer();
    ~MetalRenderer();

    bool initialize(QWindow* window);
    void resize(int width, int height, float devicePixelRatio);
    MetalFrame beginFrame();
    void endFrame(const MetalFrame& frame);

    [[nodiscard]] void* device() const;
    [[nodiscard]] bool isInitialized() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
