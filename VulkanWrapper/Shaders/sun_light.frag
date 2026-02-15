#version 460
#extension GL_EXT_ray_query : require

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerColor;
layout (set = 0, binding = 1) uniform sampler2D samplerPosition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform accelerationStructureEXT topLevelAS;

#include "atmosphere_params.glsl"

// SkyParametersGPU - matches C++ struct layout exactly
layout (push_constant) uniform PushConstants {
    SkyParameters sky;
};

#include "atmosphere_scattering.glsl"
#include "sun_lighting_computation.glsl"

void main()
{
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 position = texture(samplerPosition, inUV).rgb;
    vec3 albedo = texture(samplerColor, inUV).rgb;

    // Physically-based lighting using radiance formulation:
    // L_out = BRDF * L_sun * solid_angle * cos(theta)
    // For Lambertian: BRDF = albedo / PI
    vec3 diffuse = (albedo / ATMO_PI)
                 * luminance_from_sun(sky, position, normal, topLevelAS);

    // Output final luminance (cd/m^2)
    // Note: Ambient/indirect lighting with AO is now handled by SkyLightPass
    outColor = vec4(diffuse, 1.0);
}
