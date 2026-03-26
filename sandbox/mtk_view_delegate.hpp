#pragma once

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>
#include <MetalKit/MetalKit.hpp>
#include <cstddef>
#include "renderer.hpp"

class my_mtk_view_delegate : public MTK::ViewDelegate {
public:
    my_mtk_view_delegate(MTL::Device* p_device)
    : MTK::ViewDelegate()
    , m_p_renderer(new renderer(p_device)) {}

    ~my_mtk_view_delegate() override {
        delete m_p_renderer;
    }

    void drawInMTKView(MTK::View* p_view) override {
        m_p_renderer->draw(p_view);
    }


private:
    renderer* m_p_renderer;
};