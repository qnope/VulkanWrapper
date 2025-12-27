#pragma once

#include <glm/glm.hpp>

namespace vw {

/**
 * @brief GPU-compatible version of SkyParameters with proper alignment
 *
 * Uses vec4 to ensure consistent alignment between C++ (GLM) and GLSL.
 * This avoids vec3 alignment issues between different compilers.
 */
struct SkyParametersGPU {
    // xyz = direction FROM star TO planet, w = star_constant
    glm::vec4 star_direction_and_constant;
    // xyz = star color, w = star solid angle
    glm::vec4 star_color_and_solid_angle;
    // xyz = rayleigh coefficient, w = height_rayleigh
    glm::vec4 rayleigh_and_height_r;
    // xyz = mie coefficient, w = height_mie
    glm::vec4 mie_and_height_m;
    // xyz = ozone coefficient, w = height_ozone
    glm::vec4 ozone_and_height_o;
    // x = radius_planet, y = radius_atmosphere, z = luminous_efficiency, w =
    // unused
    glm::vec4 radii_and_efficiency;
};

// Verify GPU struct size (6 * 16 = 96 bytes)
static_assert(sizeof(SkyParametersGPU) == 96,
              "SkyParametersGPU must be 96 bytes");

/**
 * @brief Physical sky and star parameters for atmospheric rendering
 *
 * This structure contains all physical parameters needed for sky rendering
 * and lighting calculations. It can be used as push constants (<128 bytes)
 * or in a uniform buffer.
 *
 * The structure uses radiance-based calculations rather than illuminance.
 */
struct SkyParameters {
    // Star (sun) parameters
    float star_constant;      // Solar constant (W/m² at top of atmosphere)
    glm::vec3 star_direction; // Direction FROM star TO planet (normalized)
    glm::vec3 star_color;     // Star color (normalized, typically from temp)
    float star_solid_angle;   // Solid angle of star disk (steradians)

    // Atmospheric scattering coefficients at sea level (per meter)
    glm::vec3 rayleigh_coef; // Rayleigh scattering coefficient
    glm::vec3 mie_coef;      // Mie scattering coefficient
    glm::vec3 ozone_coef;    // Ozone absorption coefficient

    // Scale heights (meters)
    float height_rayleigh; // Rayleigh scale height
    float height_mie;      // Mie scale height
    float height_ozone;    // Ozone scale height

    // Planet parameters
    float radius_planet;     // Planet radius (meters)
    float radius_atmosphere; // Atmosphere outer radius (meters)

    // Luminous efficiency (lm/W) to convert W/m²/sr to cd/m²
    float luminous_efficiency; // Typically 93 lm/W for sunlight

    // =========================================================================
    // Static helper functions
    // =========================================================================

    /**
     * @brief Convert angle in degrees to direction vector
     *
     * @param angle_deg Angle above horizon in degrees (0 = horizon, 90 =
     * zenith)
     * @return Normalized direction vector pointing toward the star
     */
    static glm::vec3 angle_to_direction(float angle_deg);

    /**
     * @brief Convert star temperature in Kelvin to RGB color
     *
     * Uses Planck's law approximation for blackbody radiation.
     * Based on algorithm by Tanner Helland.
     *
     * @param temperature_kelvin Star temperature in Kelvin
     * @return Normalized RGB color
     */
    static glm::vec3 temperature_to_color(float temperature_kelvin);

    /**
     * @brief Compute solid angle from angular diameter in degrees
     *
     * @param angular_diameter_deg Angular diameter of the star in degrees
     * @return Solid angle in steradians
     */
    static float angular_diameter_to_solid_angle(float angular_diameter_deg);

    /**
     * @brief Compute radiance from solar constant and solid angle
     *
     * Radiance L = E / Omega where E is the solar constant (irradiance)
     * and Omega is the solid angle of the star disk.
     *
     * @param solar_constant Solar constant (W/m² or similar irradiance unit)
     * @param solid_angle Solid angle of star disk (steradians)
     * @return Radiance (W/m²/sr or similar)
     */
    static float compute_radiance(float solar_constant, float solid_angle);

    // =========================================================================
    // Factory methods
    // =========================================================================

    /**
     * @brief Create Earth-Sun parameters with default values
     *
     * @param sun_angle_deg Sun angle above horizon in degrees
     * @return SkyParameters configured for Earth-Sun system
     */
    static SkyParameters create_earth_sun(float sun_angle_deg);

    /**
     * @brief Get the direction toward the star (opposite of star_direction)
     */
    glm::vec3 direction_to_star() const { return -star_direction; }

    /**
     * @brief Compute star disk radiance from star constant and solid angle
     */
    float star_radiance() const {
        return compute_radiance(star_constant, star_solid_angle);
    }

    /**
     * @brief Convert to GPU-compatible structure
     */
    SkyParametersGPU to_gpu() const {
        return SkyParametersGPU{
            .star_direction_and_constant =
                glm::vec4(star_direction, star_constant),
            .star_color_and_solid_angle =
                glm::vec4(star_color, star_solid_angle),
            .rayleigh_and_height_r = glm::vec4(rayleigh_coef, height_rayleigh),
            .mie_and_height_m = glm::vec4(mie_coef, height_mie),
            .ozone_and_height_o = glm::vec4(ozone_coef, height_ozone),
            .radii_and_efficiency = glm::vec4(radius_planet, radius_atmosphere,
                                              luminous_efficiency, 0.0f)};
    }
};

// Verify the structure fits in push constants (< 128 bytes)
static_assert(sizeof(SkyParameters) <= 128,
              "SkyParameters must fit in push constants");

} // namespace vw
