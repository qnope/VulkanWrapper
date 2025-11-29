#version 450

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D samplerColor;
layout (set = 0, binding = 1) uniform sampler2D samplerPosition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;

layout (push_constant) uniform PushConstants {
    vec4 sunDirection;
    vec4 sunColor;
} pushConstants;

void main() 
{
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec3 position = texture(samplerPosition, inUV).rgb;
    vec3 albedo = texture(samplerColor, inUV).rgb;

    vec3 L = normalize(-pushConstants.sunDirection.xyz);
    float NdotL = max(dot(normal, L), 0.0);

    vec3 diffuse = albedo * pushConstants.sunColor.rgb * NdotL;

    outColor = vec4(diffuse, 1.0);
}
