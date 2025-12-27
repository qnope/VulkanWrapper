// Atmosphere Parameters - Shared definitions for sky and lighting shaders
// This file provides the SkyParameters struct and parameter extraction functions.
//
// Usage: Include this file, then define your push constant block containing
// a SkyParameters member named 'sky':
//
//   #include "atmosphere_params.glsl"
//   layout(push_constant) uniform PushConstants {
//       SkyParameters sky;
//       mat4 inverseViewProj;  // Additional fields if needed
//   } pc;
//   #define sky pc.sky
//   #include "atmosphere_scattering.glsl"

#ifndef ATMOSPHERE_PARAMS_GLSL
#define ATMOSPHERE_PARAMS_GLSL

// Atmosphere parameters structure - matches C++ SkyParametersGPU layout
struct SkyParameters {
    vec4 star_direction_and_constant;   // xyz = direction FROM star, w = constant
    vec4 star_color_and_solid_angle;    // xyz = color, w = solid_angle
    vec4 rayleigh_and_height_r;         // xyz = rayleigh_coef, w = height_rayleigh
    vec4 mie_and_height_m;              // xyz = mie_coef, w = height_mie
    vec4 ozone_and_height_o;            // xyz = ozone_coef, w = height_ozone
    vec4 radii_and_efficiency;          // x = radius_planet, y = radius_atmosphere, z = luminous_efficiency
};

// Constants
const float ATMO_PI = 3.14159265359;
const int ATMO_STEPS = 8;

#endif // ATMOSPHERE_PARAMS_GLSL
