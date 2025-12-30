#version 450

layout(set = 0, binding = 0) uniform sampler2D hdr_buffer;      // direct light
layout(set = 0, binding = 1) uniform sampler2D indirect_buffer; // indirect light

layout(location = 0) in vec2 in_uv;
layout(location = 0) out vec4 out_color;

// Tone mapping operator IDs (must match ToneMappingOperator enum in C++)
const int OPERATOR_ACES = 0;
const int OPERATOR_REINHARD = 1;
const int OPERATOR_REINHARD_EXTENDED = 2;
const int OPERATOR_UNCHARTED2 = 3;
const int OPERATOR_NEUTRAL = 4;

layout(push_constant) uniform PushConstants {
    float exposure;           // EV multiplier (default: 1.0)
    int operator_id;          // ToneMappingOperator enum value
    float white_point;        // For Reinhard Extended (default: 4.0)
    float luminance_scale;    // Normalization factor to de-normalize HDR values
    float indirect_intensity; // Multiplier for indirect light (0.0 = disabled)
} push;

// ACES (Academy Color Encoding System) approximation
// Based on the Narkowicz ACES fit
vec3 tone_map_aces(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Simple Reinhard: L / (1 + L)
vec3 tone_map_reinhard(vec3 x) {
    return x / (1.0 + x);
}

// Reinhard Extended with white point control
// Allows highlights above white_point to clip to white
vec3 tone_map_reinhard_extended(vec3 x, float white_point) {
    float w2 = white_point * white_point;
    vec3 numerator = x * (1.0 + x / w2);
    return numerator / (1.0 + x);
}

// Uncharted 2 / Hable filmic curve helper function
vec3 uncharted2_partial(vec3 x) {
    const float A = 0.15; // Shoulder strength
    const float B = 0.50; // Linear strength
    const float C = 0.10; // Linear angle
    const float D = 0.20; // Toe strength
    const float E = 0.02; // Toe numerator
    const float F = 0.30; // Toe denominator
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// Uncharted 2 / Hable filmic tone mapping
vec3 tone_map_uncharted2(vec3 x) {
    const float exposure_bias = 2.0;
    vec3 curr = uncharted2_partial(x * exposure_bias);

    // White scale to normalize the curve
    const float W = 11.2;
    vec3 white_scale = 1.0 / uncharted2_partial(vec3(W));

    return curr * white_scale;
}

// Neutral (linear + clamp) - no tone mapping, just clamp to [0, 1]
vec3 tone_map_neutral(vec3 x) {
    return clamp(x, 0.0, 1.0);
}

// Apply selected tone mapping operator
vec3 apply_tone_mapping(vec3 hdr_color, int op, float white_point) {
    switch (op) {
        case OPERATOR_ACES:
            return tone_map_aces(hdr_color);
        case OPERATOR_REINHARD:
            return tone_map_reinhard(hdr_color);
        case OPERATOR_REINHARD_EXTENDED:
            return tone_map_reinhard_extended(hdr_color, white_point);
        case OPERATOR_UNCHARTED2:
            return tone_map_uncharted2(hdr_color);
        case OPERATOR_NEUTRAL:
            return tone_map_neutral(hdr_color);
        default:
            return tone_map_aces(hdr_color); // Fallback to ACES
    }
}

void main() {
    // UVs are already in [0, 1] from fullscreen vertex shader
    vec2 uv = in_uv;

    // Sample HDR buffer (direct light)
    vec3 hdr_color = texture(hdr_buffer, uv).rgb;

    // Sample indirect light buffer and add with intensity multiplier
    vec3 indirect_color = texture(indirect_buffer, uv).rgb;
    vec3 combined = hdr_color + indirect_color * push.indirect_intensity;

    combined /= push.luminance_scale;

    // Apply exposure (exposure is in terms of physical luminance now)
    combined *= push.exposure;

    // Apply tone mapping (output is linear, gamma handled by sRGB format)
    vec3 ldr_color =
        apply_tone_mapping(combined, push.operator_id, push.white_point);

    out_color = vec4(ldr_color, 1.0);
}
