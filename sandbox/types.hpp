#pragma once
#include <simd/simd.h>

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