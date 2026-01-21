#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#include <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>
#include <MetalKit/MetalKit.hpp>
#include <cstddef>

#include <simd/simd.h>

static constexpr size_t k_instance_rows = 10;
static constexpr size_t k_instance_columns = 10;
static constexpr size_t k_instance_depth = 10;
static constexpr size_t k_num_instances =
  (k_instance_rows * k_instance_columns * k_instance_depth);
static constexpr size_t k_max_frames_in_flight = 3;
static constexpr uint32_t k_texture_width = 128;
static constexpr uint32_t k_texture_height = 128;

#pragma region Declarations {

namespace math {
    constexpr simd::float3 add(const simd::float3& a, const simd::float3& b);
    constexpr simd_float4x4 make_identity();
    simd::float4x4 make_perspective();
    simd::float4x4 make_x_rotate(float angle_radians);
    simd::float4x4 make_y_rotate(float angle_radians);
    simd::float4x4 make_z_rotate(float angle_radians);
    simd::float4x4 make_translate(const simd::float3& v);
    simd::float4x4 make_scale(const simd::float3& v);
    simd::float3x3 discard_translation(const simd::float4x4& m);
}

class renderer {
public:
    renderer(MTL::Device* p_device);
    ~renderer();
    void build_shaders();
    void build_compute_pipeline();
    void build_depth_stencil_states();
    void build_textures();
    void build_buffers();
    void generate_mandelbrot_texture(MTL::CommandBuffer* p_command_buffer);
    void draw(MTK::View* p_view);

private:
    MTL::Device* m_p_device;
    MTL::CommandQueue* m_p_command_queue;
    MTL::Library* m_p_shader_library;
    MTL::RenderPipelineState* m_p_pso;
    MTL::ComputePipelineState* m_p_compute_pso;
    MTL::DepthStencilState* m_p_depth_stencil_state;
    MTL::Texture* m_p_texture;
    MTL::Buffer* m_p_vertex_data_buffer;
    MTL::Buffer* m_p_instance_data_buffer[k_max_frames_in_flight];
    MTL::Buffer* m_p_camera_data_buffer[k_max_frames_in_flight];
    MTL::Buffer* m_p_index_buffer;
    MTL::Buffer* m_p_texture_animation_buffer;
    float m_angle{};
    int m_frame{};
    dispatch_semaphore_t m_semaphore;
    static const int k_max_frames_in_flight;
    uint m_animation_index{};
};

class my_mtk_view_delegate : public MTK::ViewDelegate {
public:
    my_mtk_view_delegate(MTL::Device* p_device);
     ~my_mtk_view_delegate() override;
     void drawInMTKView(MTK::View* p_view) override;

private:
    renderer* m_p_renderer;
};

class my_app_delegate : public NS::ApplicationDelegate {
public:
    ~my_app_delegate() override;

    NS::Menu* create_menu_bar();

     void applicationWillFinishLaunching(
      NS::Notification* p_notification) override;
     void applicationDidFinishLaunching(
      NS::Notification* p_notification) override;
     bool applicationShouldTerminateAfterLastWindowClosed(
      NS::Application* p_sender) override;

private:
    NS::Window* m_p_window;
    MTK::View* m_p_mtk_view;
    MTL::Device* m_p_device;
    my_mtk_view_delegate* m_p_view_delegate = nullptr;
};

#pragma endregion Declarations }

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

#pragma mark - AppDelegate
#pragma region AppDelegate {

my_app_delegate::~my_app_delegate() {
    m_p_mtk_view->release();
    m_p_window->release();
    m_p_device->release();
    delete m_p_view_delegate;
}

NS::Menu*
my_app_delegate::create_menu_bar() {
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

void
my_app_delegate::applicationWillFinishLaunching(NS::Notification* p_notification) {
    NS::Menu* p_menu = create_menu_bar();
    NS::Application* p_app =
      reinterpret_cast<NS::Application*>(p_notification->object());
    p_app->setMainMenu(p_menu);
    p_app->setActivationPolicy(NS::ActivationPolicy::ActivationPolicyRegular);
}

void
my_app_delegate::applicationDidFinishLaunching(NS::Notification* p_notification) {
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

bool
my_app_delegate::applicationShouldTerminateAfterLastWindowClosed(
  NS::Application* p_sender) {
    return true;
}

#pragma endregion AppDelegate }

#pragma mark - ViewDelegate
#pragma region ViewDelegate {

my_mtk_view_delegate::my_mtk_view_delegate(MTL::Device* p_device)
  : MTK::ViewDelegate()
  , m_p_renderer(new renderer(p_device)) {}

my_mtk_view_delegate::~my_mtk_view_delegate() {
    delete m_p_renderer;
}

void
my_mtk_view_delegate::drawInMTKView(MTK::View* p_view) {
    m_p_renderer->draw(p_view);
}

#pragma endregion ViewDelegate }

#pragma mark - Math

namespace math {
    struct perspective_params {
        float fov_radians;
        float aspect;
        float znear;
        float zfar;
    };

    constexpr simd::float3 add(const simd::float3& a, const simd::float3& b) {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    constexpr simd_float4x4 make_identity() {
        using simd::float4;
        return (simd_float4x4){ (float4){ 1.f, 0.f, 0.f, 0.f },
                                (float4){ 0.f, 1.f, 0.f, 0.f },
                                (float4){ 0.f, 0.f, 1.f, 0.f },
                                (float4){ 0.f, 0.f, 0.f, 1.f } };
    }

    simd::float4x4 make_perspective(perspective_params params) {
        using simd::float4;
        float ys = 1.f / tanf(params.fov_radians * 0.5f);
        float xs = ys / params.aspect;
        float zs = params.zfar / (params.znear - params.zfar);
        return simd_matrix_from_rows((float4){ xs, 0.0f, 0.0f, 0.0f },
                                     (float4){ 0.0f, ys, 0.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, zs, params.znear * zs },
                                     (float4){ 0, 0, -1, 0 });
    }

    simd::float4x4 make_x_rotate(float angle_radians) {
        using simd::float4;
        const float a = angle_radians;
        return simd_matrix_from_rows((float4){ 1.0f, 0.0f, 0.0f, 0.0f },
                                     (float4){ 0.0f, cosf(a), sinf(a), 0.0f },
                                     (float4){ 0.0f, -sinf(a), cosf(a), 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 make_y_rotate(float angle_radians) {
        using simd::float4;
        const float a = angle_radians;
        return simd_matrix_from_rows((float4){ cosf(a), 0.0f, sinf(a), 0.0f },
                                     (float4){ 0.0f, 1.0f, 0.0f, 0.0f },
                                     (float4){ -sinf(a), 0.0f, cosf(a), 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 make_z_rotate(float angle_radians) {
        using simd::float4;
        const float a = angle_radians;
        return simd_matrix_from_rows((float4){ cosf(a), sinf(a), 0.0f, 0.0f },
                                     (float4){ -sinf(a), cosf(a), 0.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, 1.0f, 0.0f },
                                     (float4){ 0.0f, 0.0f, 0.0f, 1.0f });
    }

    simd::float4x4 make_translate(const simd::float3& v) {
        using simd::float4;
        const float4 col0 = { 1.0f, 0.0f, 0.0f, 0.0f };
        const float4 col1 = { 0.0f, 1.0f, 0.0f, 0.0f };
        const float4 col2 = { 0.0f, 0.0f, 1.0f, 0.0f };
        const float4 col3 = { v.x, v.y, v.z, 1.0f };
        return simd_matrix(col0, col1, col2, col3);
    }

    simd::float4x4 make_scale(const simd::float3& v) {
        using simd::float4;
        return simd_matrix((float4){ v.x, 0, 0, 0 },
                           (float4){ 0, v.y, 0, 0 },
                           (float4){ 0, 0, v.z, 0 },
                           (float4){ 0, 0, 0, 1.0 });
    }

    simd::float3x3 discard_translation(const simd::float4x4& m) {
        return simd_matrix(
          m.columns[0].xyz, m.columns[1].xyz, m.columns[2].xyz);
    }

}

#pragma mark - Renderer
#pragma region Renderer {

const int renderer::k_max_frames_in_flight = 3;

renderer::renderer(MTL::Device* p_device)
  : m_p_device(p_device->retain()) {
    m_p_command_queue = m_p_device->newCommandQueue();
    build_shaders();
    build_compute_pipeline();
    build_depth_stencil_states();
    build_textures();
    build_buffers();

    m_semaphore = dispatch_semaphore_create(renderer::k_max_frames_in_flight);
}

renderer::~renderer() {
    m_p_texture_animation_buffer->release();
    m_p_texture->release();
    m_p_shader_library->release();
    m_p_depth_stencil_state->release();
    m_p_vertex_data_buffer->release();
    for (int i = 0; i < k_max_frames_in_flight; ++i) {
        m_p_instance_data_buffer[i]->release();
    }
    for (int i = 0; i < k_max_frames_in_flight; ++i) {
        m_p_camera_data_buffer[i]->release();
    }
    m_p_index_buffer->release();
    m_p_compute_pso->release();
    m_p_pso->release();
    m_p_command_queue->release();
    m_p_device->release();
}

namespace shader_types {
    struct vertex_data {
        simd::float3 position;
        simd::float3 normal;
        simd::float2 texcoord;
    };

    struct instance_data {
        simd::float4x4 instanceTransform;
        simd::float3x3 instanceNormalTransform;
        simd::float4 instanceColor;
    };

    struct camera_data {
        simd::float4x4 perspectiveTransform;
        simd::float4x4 worldTransform;
        simd::float3x3 worldNormalTransform;
    };
}

void
renderer::build_shaders() {
    using NS::StringEncoding::UTF8StringEncoding;

    const char* shader_src = R"(
        #include <metal_stdlib>
        using namespace metal;

        struct v2f
        {
            float4 position [[position]];
            float3 normal;
            half3 color;
            float2 texcoord;
        };

        struct VertexData
        {
            float3 position;
            float3 normal;
            float2 texcoord;
        };

        struct InstanceData
        {
            float4x4 instanceTransform;
            float3x3 instanceNormalTransform;
            float4 instanceColor;
        };

        struct CameraData
        {
            float4x4 perspectiveTransform;
            float4x4 worldTransform;
            float3x3 worldNormalTransform;
        };

        v2f vertex vertexMain( device const VertexData* vertexData [[buffer(0)]],
                               device const InstanceData* instanceData [[buffer(1)]],
                               device const CameraData& cameraData [[buffer(2)]],
                               uint vertexId [[vertex_id]],
                               uint instanceId [[instance_id]] )
        {
            v2f o;

            const device VertexData& vd = vertexData[ vertexId ];
            float4 pos = float4( vd.position, 1.0 );
            pos = instanceData[ instanceId ].instanceTransform * pos;
            pos = cameraData.perspectiveTransform * cameraData.worldTransform * pos;
            o.position = pos;

            float3 normal = instanceData[ instanceId ].instanceNormalTransform * vd.normal;
            normal = cameraData.worldNormalTransform * normal;
            o.normal = normal;

            o.texcoord = vd.texcoord.xy;

            o.color = half3( instanceData[ instanceId ].instanceColor.rgb );
            return o;
        }

        half4 fragment fragmentMain( v2f in [[stage_in]], texture2d< half, access::sample > tex [[texture(0)]] )
        {
            constexpr sampler s( address::repeat, filter::linear );
            half3 texel = tex.sample( s, in.texcoord ).rgb;

            // assume light coming from (front-top-right)
            float3 l = normalize(float3( 1.0, 1.0, 0.8 ));
            float3 n = normalize( in.normal );

            half ndotl = half( saturate( dot( n, l ) ) );

            half3 illum = (in.color * texel * 0.1) + (in.color * texel * ndotl);
            return half4( illum, 1.0 );
        }
    )";

    NS::Error* p_error = nullptr;
    MTL::Library* p_library = m_p_device->newLibrary(
      NS::String::string(shader_src, UTF8StringEncoding), nullptr, &p_error);
    if (!p_library) {
        __builtin_printf("%s", p_error->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* p_vertex_fn = p_library->newFunction(
      NS::String::string("vertexMain", UTF8StringEncoding));
    MTL::Function* p_frag_fn = p_library->newFunction(
      NS::String::string("fragmentMain", UTF8StringEncoding));

    MTL::RenderPipelineDescriptor* p_desc =
      MTL::RenderPipelineDescriptor::alloc()->init();
    p_desc->setVertexFunction(p_vertex_fn);
    p_desc->setFragmentFunction(p_frag_fn);
    p_desc->colorAttachments()->object(0)->setPixelFormat(
      MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB);
    p_desc->setDepthAttachmentPixelFormat(
      MTL::PixelFormat::PixelFormatDepth16Unorm);

    m_p_pso = m_p_device->newRenderPipelineState(p_desc, &p_error);
    if (!m_p_pso) {
        __builtin_printf("%s", p_error->localizedDescription()->utf8String());
        assert(false);
    }

    p_vertex_fn->release();
    p_frag_fn->release();
    p_desc->release();
    m_p_shader_library = p_library;
}

void
renderer::build_compute_pipeline() {
    const char* kernel_src = R"(
        #include <metal_stdlib>
        using namespace metal;

        kernel void mandelbrot_set(texture2d< half, access::write > tex [[texture(0)]],
                                   uint2 index [[thread_position_in_grid]],
                                   uint2 gridSize [[threads_per_grid]],
                                   device const uint* frame [[buffer(0)]])
        {
            constexpr float kAnimationFrequency = 0.01;
            constexpr float kAnimationSpeed = 4;
            constexpr float kAnimationScaleLow = 0.62;
            constexpr float kAnimationScale = 0.38;

            constexpr float2 kMandelbrotPixelOffset = {-0.2, -0.35};
            constexpr float2 kMandelbrotOrigin = {-1.2, -0.32};
            constexpr float2 kMandelbrotScale = {2.2, 2.0};

            // Map time to zoom value in [kAnimationScaleLow, 1]
            float zoom = kAnimationScaleLow + kAnimationScale * cos(kAnimationFrequency * *frame);
            // Speed up zooming
            zoom = pow(zoom, kAnimationSpeed);

            //Scale
            float x0 = zoom * kMandelbrotScale.x * ((float)index.x / gridSize.x + kMandelbrotPixelOffset.x) + kMandelbrotOrigin.x;
            float y0 = zoom * kMandelbrotScale.y * ((float)index.y / gridSize.y + kMandelbrotPixelOffset.y) + kMandelbrotOrigin.y;

            // Implement Mandelbrot set
            float x = 0.0;
            float y = 0.0;
            uint iteration = 0;
            uint max_iteration = 1000;
            float xtmp = 0.0;
            while(x * x + y * y <= 4 && iteration < max_iteration)
            {
                xtmp = x * x - y * y + x0;
                y = 2 * x * y + y0;
                x = xtmp;
                iteration += 1;
            }

            // Convert iteration result to colors
            half color = (0.5 + 0.5 * cos(3.0 + iteration * 0.15));
            tex.write(half4(color, color, color, 1.0), index, 0);
        })";
    NS::Error* p_error = nullptr;

    MTL::Library* p_compute_library = m_p_device->newLibrary(
      NS::String::string(kernel_src, NS::UTF8StringEncoding), nullptr, &p_error);
    if (!p_compute_library) {
        __builtin_printf("%s", p_error->localizedDescription()->utf8String());
        assert(false);
    }

    MTL::Function* p_mandelbrot_fn = p_compute_library->newFunction(
      NS::String::string("mandelbrot_set", NS::UTF8StringEncoding));
    m_p_compute_pso = m_p_device->newComputePipelineState(p_mandelbrot_fn, &p_error);
    if (!m_p_compute_pso) {
        __builtin_printf("%s", p_error->localizedDescription()->utf8String());
        assert(false);
    }

    p_mandelbrot_fn->release();
    p_compute_library->release();
}

void
renderer::build_depth_stencil_states() {
    MTL::DepthStencilDescriptor* p_ds_desc =
      MTL::DepthStencilDescriptor::alloc()->init();
    p_ds_desc->setDepthCompareFunction(MTL::CompareFunction::CompareFunctionLess);
    p_ds_desc->setDepthWriteEnabled(true);

    m_p_depth_stencil_state = m_p_device->newDepthStencilState(p_ds_desc);

    p_ds_desc->release();
}

void
renderer::build_textures() {
    MTL::TextureDescriptor* p_texture_desc =
      MTL::TextureDescriptor::alloc()->init();
    p_texture_desc->setWidth(k_texture_width);
    p_texture_desc->setHeight(k_texture_height);
    p_texture_desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    p_texture_desc->setTextureType(MTL::TextureType2D);
    p_texture_desc->setStorageMode(MTL::StorageModeManaged);
    p_texture_desc->setUsage(MTL::ResourceUsageSample | MTL::ResourceUsageRead |
                           MTL::ResourceUsageWrite);

    MTL::Texture* p_texture = m_p_device->newTexture(p_texture_desc);
    m_p_texture = p_texture;

    p_texture_desc->release();
}

void
renderer::build_buffers() {
    using simd::float2;
    using simd::float3;

    const float s = 0.5f;

    shader_types::vertex_data verts[] = {
        //                                         Texture
        //   Positions           Normals         Coordinates
        { .position = { -s, -s, +s }, .normal = { 0.f, 0.f, 1.f }, .texcoord = { 0.f, 1.f } },
        { .position = { +s, -s, +s }, .normal = { 0.f, 0.f, 1.f }, .texcoord = { 1.f, 1.f } },
        { .position = { +s, +s, +s }, .normal = { 0.f, 0.f, 1.f }, .texcoord = { 1.f, 0.f } },
        { .position = { -s, +s, +s }, .normal = { 0.f, 0.f, 1.f }, .texcoord = { 0.f, 0.f } },

        { .position = { +s, -s, +s }, .normal = { 1.f, 0.f, 0.f }, .texcoord = { 0.f, 1.f } },
        { .position = { +s, -s, -s }, .normal = { 1.f, 0.f, 0.f }, .texcoord = { 1.f, 1.f } },
        { .position = { +s, +s, -s }, .normal = { 1.f, 0.f, 0.f }, .texcoord = { 1.f, 0.f } },
        { .position = { +s, +s, +s }, .normal = { 1.f, 0.f, 0.f }, .texcoord = { 0.f, 0.f } },

        { .position = { +s, -s, -s }, .normal = { 0.f, 0.f, -1.f }, .texcoord = { 0.f, 1.f } },
        { .position = { -s, -s, -s }, .normal = { 0.f, 0.f, -1.f }, .texcoord = { 1.f, 1.f } },
        { .position = { -s, +s, -s }, .normal = { 0.f, 0.f, -1.f }, .texcoord = { 1.f, 0.f } },
        { .position = { +s, +s, -s }, .normal = { 0.f, 0.f, -1.f }, .texcoord = { 0.f, 0.f } },

        { .position = { -s, -s, -s }, .normal = { -1.f, 0.f, 0.f }, .texcoord = { 0.f, 1.f } },
        { .position = { -s, -s, +s }, .normal = { -1.f, 0.f, 0.f }, .texcoord = { 1.f, 1.f } },
        { .position = { -s, +s, +s }, .normal = { -1.f, 0.f, 0.f }, .texcoord = { 1.f, 0.f } },
        { .position = { -s, +s, -s }, .normal = { -1.f, 0.f, 0.f }, .texcoord = { 0.f, 0.f } },

        { .position = { -s, +s, +s }, .normal = { 0.f, 1.f, 0.f }, .texcoord = { 0.f, 1.f } },
        { .position = { +s, +s, +s }, .normal = { 0.f, 1.f, 0.f }, .texcoord = { 1.f, 1.f } },
        { .position = { +s, +s, -s }, .normal = { 0.f, 1.f, 0.f }, .texcoord = { 1.f, 0.f } },
        { .position = { -s, +s, -s }, .normal = { 0.f, 1.f, 0.f }, .texcoord = { 0.f, 0.f } },

        { .position = { -s, -s, -s }, .normal = { 0.f, -1.f, 0.f }, .texcoord = { 0.f, 1.f } },
        { .position = { +s, -s, -s }, .normal = { 0.f, -1.f, 0.f }, .texcoord = { 1.f, 1.f } },
        { .position = { +s, -s, +s }, .normal = { 0.f, -1.f, 0.f }, .texcoord = { 1.f, 0.f } },
        { .position = { -s, -s, +s }, .normal = { 0.f, -1.f, 0.f }, .texcoord = { 0.f, 0.f } }
    };

    uint16_t indices[] = {
        0,  1,  2,  2,  3,  0,  /* front */
        4,  5,  6,  6,  7,  4,  /* right */
        8,  9,  10, 10, 11, 8,  /* back */
        12, 13, 14, 14, 15, 12, /* left */
        16, 17, 18, 18, 19, 16, /* top */
        20, 21, 22, 22, 23, 20, /* bottom */
    };

    const size_t vertex_data_size = sizeof(verts);
    const size_t index_data_size = sizeof(indices);

    MTL::Buffer* p_vertex_buffer =
      m_p_device->newBuffer(vertex_data_size, MTL::ResourceStorageModeManaged);
    MTL::Buffer* p_index_buffer =
      m_p_device->newBuffer(index_data_size, MTL::ResourceStorageModeManaged);

    m_p_vertex_data_buffer = p_vertex_buffer;
    m_p_index_buffer = p_index_buffer;

    memcpy(m_p_vertex_data_buffer->contents(), verts, vertex_data_size);
    memcpy(m_p_index_buffer->contents(), indices, index_data_size);

    m_p_vertex_data_buffer->didModifyRange(
      NS::Range::Make(0, m_p_vertex_data_buffer->length()));
    m_p_index_buffer->didModifyRange(NS::Range::Make(0, m_p_index_buffer->length()));

    const size_t instance_data_size =
      k_max_frames_in_flight * k_num_instances * sizeof(shader_types::instance_data);
    for (size_t i = 0; i < k_max_frames_in_flight; ++i) {
        m_p_instance_data_buffer[i] = m_p_device->newBuffer(
          instance_data_size, MTL::ResourceStorageModeManaged);
    }

    const size_t camera_data_size =
      k_max_frames_in_flight * sizeof(shader_types::camera_data);
    for (size_t i = 0; i < k_max_frames_in_flight; ++i) {
        m_p_camera_data_buffer[i] =
          m_p_device->newBuffer(camera_data_size, MTL::ResourceStorageModeManaged);
    }

    m_p_texture_animation_buffer =
      m_p_device->newBuffer(sizeof(uint), MTL::ResourceStorageModeManaged);
}

void
renderer::generate_mandelbrot_texture(MTL::CommandBuffer* p_command_buffer) {
    assert(p_command_buffer);

    uint* ptr = reinterpret_cast<uint*>(m_p_texture_animation_buffer->contents());
    *ptr = (m_animation_index++) % 5000;
    m_p_texture_animation_buffer->didModifyRange(NS::Range::Make(0, sizeof(uint)));

    MTL::ComputeCommandEncoder* p_compute_encoder =
      p_command_buffer->computeCommandEncoder();

    p_compute_encoder->setComputePipelineState(m_p_compute_pso);
    p_compute_encoder->setTexture(m_p_texture, 0);
    p_compute_encoder->setBuffer(m_p_texture_animation_buffer, 0, 0);

    MTL::Size grid_size = MTL::Size(k_texture_width, k_texture_height, 1);

    NS::UInteger thread_group_size =
      m_p_compute_pso->maxTotalThreadsPerThreadgroup();
    MTL::Size threadgroup_size(thread_group_size, 1, 1);

    p_compute_encoder->dispatchThreads(grid_size, threadgroup_size);

    p_compute_encoder->endEncoding();
}

void
renderer::draw(MTK::View* p_view) {
    using simd::float3;
    using simd::float4;
    using simd::float4x4;

    NS::AutoreleasePool* p_pool = NS::AutoreleasePool::alloc()->init();

    m_frame = (m_frame + 1) % renderer::k_max_frames_in_flight;
    MTL::Buffer* p_instance_data_buffer = m_p_instance_data_buffer[m_frame];

    MTL::CommandBuffer* p_cmd = m_p_command_queue->commandBuffer();
    dispatch_semaphore_wait(m_semaphore, DISPATCH_TIME_FOREVER);
    renderer* p_renderer = this;
    p_cmd->addCompletedHandler(^void(MTL::CommandBuffer* p_cmd) {
      dispatch_semaphore_signal(p_renderer->m_semaphore);
    });

    m_angle += 0.002f;

    const float scl = 0.2f;
    shader_types::instance_data* p_instance_data =
      reinterpret_cast<shader_types::instance_data*>(
        p_instance_data_buffer->contents());

    float3 object_position = { 0.f, 0.f, -10.f };

    float4x4 rt = math::make_translate(object_position);
    float4x4 rr1 = math::make_y_rotate(-m_angle);
    float4x4 rr0 = math::make_x_rotate(m_angle * 0.5f);
    float4x4 rt_inv = math::make_translate(
      { -object_position.x, -object_position.y, -object_position.z });
    float4x4 full_object_rot = rt * rr1 * rr0 * rt_inv;

    size_t ix = 0;
    size_t iy = 0;
    size_t iz = 0;
    for (size_t i = 0; i < k_num_instances; ++i) {
        if (ix == k_instance_rows) {
            ix = 0;
            iy += 1;
        }
        if (iy == k_instance_rows) {
            iy = 0;
            iz += 1;
        }

        float4x4 scale = math::make_scale((float3){ scl, scl, scl });
        float4x4 zrot = math::make_z_rotate(m_angle * sinf((float)ix));
        float4x4 yrot = math::make_y_rotate(m_angle * cosf((float)iy));

        float x = ((float)ix - (float)k_instance_rows / 2.f) * (2.f * scl) + scl;
        float y =
          ((float)iy - (float)k_instance_columns / 2.f) * (2.f * scl) + scl;
        float z = ((float)iz - (float)k_instance_depth / 2.f) * (2.f * scl);
        float4x4 translate =
          math::make_translate(math::add(object_position, { x, y, z }));

        p_instance_data[i].instanceTransform =
          full_object_rot * translate * yrot * zrot * scale;
        p_instance_data[i].instanceNormalTransform =
          math::discard_translation(p_instance_data[i].instanceTransform);

        float i_div_num_instances = static_cast<float>(i) / (float)k_num_instances;
        float r = i_div_num_instances;
        float g = 1.0f - r;
        float b = sinf(M_PI * 2.0f * i_div_num_instances);
        p_instance_data[i].instanceColor = (float4){ r, g, b, 1.0f };

        ix += 1;
    }
    p_instance_data_buffer->didModifyRange(
      NS::Range::Make(0, p_instance_data_buffer->length()));

    // Update camera state:

    MTL::Buffer* p_camera_data_buffer = m_p_camera_data_buffer[m_frame];
    shader_types::camera_data* p_camera_data =
      reinterpret_cast<shader_types::camera_data*>(
        p_camera_data_buffer->contents());
    p_camera_data->perspectiveTransform =
      math::make_perspective({ .fov_radians = 45.f * M_PI / 180.f, .aspect = 1.f, .znear = 0.03f, .zfar = 500.0f });
    p_camera_data->worldTransform = math::make_identity();
    p_camera_data->worldNormalTransform =
      math::discard_translation(p_camera_data->worldTransform);
    p_camera_data_buffer->didModifyRange(
      NS::Range::Make(0, sizeof(shader_types::camera_data)));

    // Update texture:

    generate_mandelbrot_texture(p_cmd);

    // Begin render pass:

    MTL::RenderPassDescriptor* p_rpd = p_view->currentRenderPassDescriptor();
    MTL::RenderCommandEncoder* p_enc = p_cmd->renderCommandEncoder(p_rpd);

    p_enc->setRenderPipelineState(m_p_pso);
    p_enc->setDepthStencilState(m_p_depth_stencil_state);

    p_enc->setVertexBuffer(m_p_vertex_data_buffer, /* offset */ 0, /* index */ 0);
    p_enc->setVertexBuffer(p_instance_data_buffer, /* offset */ 0, /* index */ 1);
    p_enc->setVertexBuffer(p_camera_data_buffer, /* offset */ 0, /* index */ 2);

    p_enc->setFragmentTexture(m_p_texture, /* index */ 0);

    p_enc->setCullMode(MTL::CullModeBack);
    p_enc->setFrontFacingWinding(MTL::Winding::WindingCounterClockwise);

    p_enc->drawIndexedPrimitives(MTL::PrimitiveType::PrimitiveTypeTriangle,
                                static_cast<NS::UInteger>(6 * 6),
                                MTL::IndexType::IndexTypeUInt16,
                                m_p_index_buffer,
                                0,
                                k_num_instances);

    p_enc->endEncoding();
    p_cmd->presentDrawable(p_view->currentDrawable());
    p_cmd->commit();

    p_pool->release();
}

#pragma endregion Renderer }
