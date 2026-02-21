#include "MetalRenderer.h"

#include <QWindow>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

struct MetalRenderer::Impl {
    id<MTLDevice> device = nil;
    id<MTLCommandQueue> commandQueue = nil;
    CAMetalLayer* layer = nil;
    id<CAMetalDrawable> drawable = nil;
    MTLRenderPassDescriptor* renderPassDescriptor = nil;
    id<MTLCommandBuffer> commandBuffer = nil;
    id<MTLRenderCommandEncoder> renderEncoder = nil;
    bool initialized = false;
};

MetalRenderer::MetalRenderer() : m_impl(std::make_unique<Impl>()) {}

MetalRenderer::~MetalRenderer() = default;

bool MetalRenderer::initialize(QWindow* window) {
    if (m_impl->initialized || window == nullptr) {
        return m_impl->initialized;
    }

    if (!window->handle()) {
        window->create();
    }

    m_impl->device = MTLCreateSystemDefaultDevice();
    if (m_impl->device == nil) {
        return false;
    }

    m_impl->commandQueue = [m_impl->device newCommandQueue];
    if (m_impl->commandQueue == nil) {
        return false;
    }

    void* nativeViewHandle = reinterpret_cast<void*>(window->winId());
    NSView* view = (__bridge NSView*)nativeViewHandle;
    if (view == nil) {
        return false;
    }

    view.wantsLayer = YES;
    if ([view.layer isKindOfClass:[CAMetalLayer class]]) {
        m_impl->layer = static_cast<CAMetalLayer*>(view.layer);
    } else {
        m_impl->layer = [CAMetalLayer layer];
        view.layer = m_impl->layer;
    }

    m_impl->layer.device = m_impl->device;
    m_impl->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    m_impl->layer.framebufferOnly = NO;
    m_impl->layer.contentsGravity = kCAGravityTopLeft;
    m_impl->layer.presentsWithTransaction = NO;

    m_impl->initialized = true;
    return true;
}

void MetalRenderer::resize(int width, int height, float devicePixelRatio) {
    if (!m_impl->initialized || m_impl->layer == nil || width <= 0 || height <= 0) {
        return;
    }

    const CGFloat scale = static_cast<CGFloat>(devicePixelRatio);
    m_impl->layer.contentsScale = scale;
    m_impl->layer.drawableSize = CGSizeMake(width * scale, height * scale);
    m_impl->layer.frame = CGRectMake(0.0, 0.0, width, height);
}

MetalFrame MetalRenderer::beginFrame() {
    MetalFrame frame{};
    if (!m_impl->initialized || m_impl->layer == nil) {
        return frame;
    }
    if (m_impl->layer.drawableSize.width <= 0.0 || m_impl->layer.drawableSize.height <= 0.0) {
        return frame;
    }

    m_impl->drawable = [m_impl->layer nextDrawable];
    if (m_impl->drawable == nil) {
        return frame;
    }

    m_impl->commandBuffer = [m_impl->commandQueue commandBuffer];
    if (m_impl->commandBuffer == nil) {
        m_impl->drawable = nil;
        return frame;
    }

    m_impl->renderPassDescriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    m_impl->renderPassDescriptor.colorAttachments[0].texture = m_impl->drawable.texture;
    m_impl->renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    m_impl->renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    m_impl->renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.10, 0.11, 0.13, 1.0);

    m_impl->renderEncoder = [m_impl->commandBuffer renderCommandEncoderWithDescriptor:m_impl->renderPassDescriptor];
    if (m_impl->renderEncoder == nil) {
        m_impl->renderPassDescriptor = nil;
        m_impl->commandBuffer = nil;
        m_impl->drawable = nil;
        return frame;
    }

    frame.renderPassDescriptor = (__bridge void*)m_impl->renderPassDescriptor;
    frame.commandBuffer = (__bridge void*)m_impl->commandBuffer;
    frame.renderEncoder = (__bridge void*)m_impl->renderEncoder;
    frame.valid = true;
    return frame;
}

void MetalRenderer::endFrame(const MetalFrame& frame) {
    if (!frame.valid || !m_impl->initialized || m_impl->drawable == nil) {
        return;
    }

    [m_impl->renderEncoder endEncoding];
    [m_impl->commandBuffer presentDrawable:m_impl->drawable];
    [m_impl->commandBuffer commit];
    m_impl->renderEncoder = nil;
    m_impl->renderPassDescriptor = nil;
    m_impl->commandBuffer = nil;
    m_impl->drawable = nil;
}

void* MetalRenderer::device() const {
    return (__bridge void*)m_impl->device;
}

bool MetalRenderer::isInitialized() const {
    return m_impl->initialized;
}
