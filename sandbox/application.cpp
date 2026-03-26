#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>
#include <MetalKit/MetalKit.hpp>
#include <cstddef>
#include "mtk_view_delegate.hpp"

class my_app_delegate : public NS::ApplicationDelegate {
public:
    my_app_delegate() = default;
    ~my_app_delegate() override {
        m_p_mtk_view->release();
        m_p_window->release();
        m_p_device->release();
        delete m_p_view_delegate;
    }

    NS::Menu* create_menu_bar() {
    using NS::StringEncoding::UTF8StringEncoding;

    NS::Menu* p_main_menu = NS::Menu::alloc()->init();
    NS::MenuItem* p_app_menu_item = NS::MenuItem::alloc()->init();
    NS::Menu* p_app_menu = NS::Menu::alloc()->init(
        NS::String::string("Appname", UTF8StringEncoding));

    NS::String* app_name =
        NS::RunningApplication::currentApplication()->localizedName();
    NS::String* quit_item_name = NS::String::string("Quit ", UTF8StringEncoding)
                                    ->stringByAppendingString(app_name);
    SEL quit_cb = NS::MenuItem::registerActionCallback(
        "appQuit", [](void*, SEL, const NS::Object* p_sender) {
            auto p_app = NS::Application::sharedApplication();
            p_app->terminate(p_sender);
        });

    NS::MenuItem* p_app_quit_item = p_app_menu->addItem(
        quit_item_name, quit_cb, NS::String::string("q", UTF8StringEncoding));
    p_app_quit_item->setKeyEquivalentModifierMask(NS::EventModifierFlagCommand);
    p_app_menu_item->setSubmenu(p_app_menu);

    NS::MenuItem* p_window_menu_item = NS::MenuItem::alloc()->init();
    NS::Menu* p_window_menu =
        NS::Menu::alloc()->init(NS::String::string("Window", UTF8StringEncoding));

    SEL close_window_cb = NS::MenuItem::registerActionCallback(
        "windowClose", [](void*, SEL, const NS::Object*) {
            auto p_app = NS::Application::sharedApplication();
            p_app->windows()->object<NS::Window>(0)->close();
        });
    NS::MenuItem* p_close_window_item = p_window_menu->addItem(
        NS::String::string("Close Window", UTF8StringEncoding),
        close_window_cb,
        NS::String::string("w", UTF8StringEncoding));
    p_close_window_item->setKeyEquivalentModifierMask(
        NS::EventModifierFlagCommand);

    p_window_menu_item->setSubmenu(p_window_menu);

    p_main_menu->addItem(p_app_menu_item);
    p_main_menu->addItem(p_window_menu_item);

    p_app_menu_item->release();
    p_window_menu_item->release();
    p_app_menu->release();
    p_window_menu->release();

    return p_main_menu->autorelease();
    }

     void applicationWillFinishLaunching(NS::Notification* p_notification) override{
        NS::Menu* p_menu = create_menu_bar();
        NS::Application* p_app =
        reinterpret_cast<NS::Application*>(p_notification->object());
        p_app->setMainMenu(p_menu);
        p_app->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
    }
      
     void applicationDidFinishLaunching(NS::Notification* p_notification) override {
        CGRect frame = (CGRect){ { 100.0, 100.0 }, { 1024.0, 1024.0 } };

        m_p_window = NS::Window::alloc()->init(frame,
                                            NS::WindowStyleMaskClosable |
                                            NS::WindowStyleMaskTitled,
                                            NS::BackingStoreBuffered,
                                            false);

        m_p_device = MTL::CreateSystemDefaultDevice();

        m_p_mtk_view = MTK::View::alloc()->init(frame, m_p_device);
        m_p_mtk_view->setColorPixelFormat(
        MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
        m_p_mtk_view->setClearColor(MTL::ClearColor::Make(0.1, 0.1, 0.1, 1.0));
        m_p_mtk_view->setDepthStencilPixelFormat(
        MTL::PixelFormat::PixelFormatDepth16Unorm);
        m_p_mtk_view->setClearDepth(1.0f);

        m_p_view_delegate = new my_mtk_view_delegate(m_p_device);
        m_p_mtk_view->setDelegate(m_p_view_delegate);

        m_p_window->setContentView(m_p_mtk_view);
        m_p_window->setTitle(NS::String::string(
        "09 - Compute to Render", NS::StringEncoding::UTF8StringEncoding));

        m_p_window->makeKeyAndOrderFront(nullptr);

        NS::Application* p_app =
        reinterpret_cast<NS::Application*>(p_notification->object());
        p_app->activateIgnoringOtherApps(true);
     }

     bool applicationShouldTerminateAfterLastWindowClosed(NS::Application* p_sender) override {
        return true;
     }

private:
    NS::Window* m_p_window;
    MTK::View* m_p_mtk_view;
    MTL::Device* m_p_device;
    my_mtk_view_delegate* m_p_view_delegate = nullptr;
};

int
main(int argc, char* argv[]) {
    NS::AutoreleasePool* p_autorelease_pool =
      NS::AutoreleasePool::alloc()->init();

    my_app_delegate del;

    NS::Application* p_shared_application = NS::Application::sharedApplication();
    p_shared_application->setDelegate(&del);
    p_shared_application->run();

    p_autorelease_pool->release();

    return 0;
}