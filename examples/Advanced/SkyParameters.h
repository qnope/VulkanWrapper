#pragma once

#include <cmath>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>

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
    static glm::vec3 angle_to_direction(float angle_deg) {
        float angle_rad = glm::radians(angle_deg);
        return glm::vec3(std::cos(angle_rad), std::sin(angle_rad), 0.0f);
    }

    /**
     * @brief Convert star temperature in Kelvin to RGB color
     *
     * Uses Planck's law approximation for blackbody radiation.
     * Based on algorithm by Tanner Helland.
     *
     * @param temperature_kelvin Star temperature in Kelvin
     * @return Normalized RGB color
     */
    static glm::vec3 temperature_to_color(float temperature_kelvin) {
        // Clamp to reasonable range
        float temp = glm::clamp(temperature_kelvin, 1000.0f, 40000.0f);
        temp /= 100.0f;

        glm::vec3 color;

        // Red
        if (temp <= 66.0f) {
            color.r = 1.0f;
        } else {
            float red = temp - 60.0f;
            red = 329.698727446f * std::pow(red, -0.1332047592f);
            color.r = glm::clamp(red / 255.0f, 0.0f, 1.0f);
        }

        // Green
        if (temp <= 66.0f) {
            float green = temp;
            green = 99.4708025861f * std::log(green) - 161.1195681661f;
            color.g = glm::clamp(green / 255.0f, 0.0f, 1.0f);
        } else {
            float green = temp - 60.0f;
            green = 288.1221695283f * std::pow(green, -0.0755148492f);
            color.g = glm::clamp(green / 255.0f, 0.0f, 1.0f);
        }

        // Blue
        if (temp >= 66.0f) {
            color.b = 1.0f;
        } else if (temp <= 19.0f) {
            color.b = 0.0f;
        } else {
            float blue = temp - 10.0f;
            blue = 138.5177312231f * std::log(blue) - 305.0447927307f;
            color.b = glm::clamp(blue / 255.0f, 0.0f, 1.0f);
        }

        return color;
    }

    /**
     * @brief Compute solid angle from angular diameter in degrees
     *
     * @param angular_diameter_deg Angular diameter of the star in degrees
     * @return Solid angle in steradians
     */
    static float angular_diameter_to_solid_angle(float angular_diameter_deg) {
        float radius_rad = glm::radians(angular_diameter_deg / 2.0f);
        constexpr float PI = 3.14159265359f;
        return 2.0f * PI * (1.0f - std::cos(radius_rad));
    }

    /**
     * @brief Compute radiance from solar constant and solid angle
     *
     * Radiance L = E / Ω where E is the solar constant (irradiance)
     * and Ω is the solid angle of the star disk.
     *
     * @param solar_constant Solar constant (W/m² or similar irradiance unit)
     * @param solid_angle Solid angle of star disk (steradians)
     * @return Radiance (W/m²/sr or similar)
     */
    static float compute_radiance(float solar_constant, float solid_angle) {
        return solar_constant / solid_angle;
    }

    // =========================================================================
    // Factory methods
    // =========================================================================

    /**
     * @brief Create Earth-Sun parameters with default values
     *
     * @param sun_angle_deg Sun angle above horizon in degrees
     * @return SkyParameters configured for Earth-Sun system
     */
    static SkyParameters create_earth_sun(float sun_angle_deg) {
        SkyParameters params;

        // Solar constant at Earth's distance (W/m²)
        params.star_constant = 1361.0f;

        // Sun direction (from sun to planet, so negate the "to sun" direction)
        glm::vec3 to_sun = angle_to_direction(sun_angle_deg);
        params.star_direction = -to_sun;

        // Sun color (approximately 5778K blackbody)
        params.star_color = temperature_to_color(5778.0f);

        // Sun angular diameter ~0.53 degrees
        params.star_solid_angle = angular_diameter_to_solid_angle(0.53f);

        // Atmospheric scattering coefficients at sea level (per meter)
        params.rayleigh_coef = glm::vec3(5.8e-6f, 13.5e-6f, 33.1e-6f);
        params.mie_coef = glm::vec3(21e-6f, 21e-6f, 21e-6f);
        params.ozone_coef = glm::vec3(3.426f, 8.298f, 0.356f) * 0.06e-5f;

        // Scale heights (meters)
        params.height_rayleigh = 8000.0f;
        params.height_mie = 1200.0f;
        params.height_ozone = 8000.0f;

        // Earth parameters
        params.radius_planet = 6360000.0f;     // 6360 km
        params.radius_atmosphere = 6420000.0f; // 6420 km

        // Luminous efficiency for sunlight (lm/W)
        params.luminous_efficiency = 93.0f;

        return params;
    }

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
    struct SkyParametersGPU to_gpu() const;
};

// Verify the structure fits in push constants (< 128 bytes)
static_assert(sizeof(SkyParameters) <= 128,
              "SkyParameters must fit in push constants");

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
    // x = radius_planet, y = radius_atmosphere, z = luminous_efficiency, w = unused
    glm::vec4 radii_and_efficiency;
};

inline SkyParametersGPU SkyParameters::to_gpu() const {
    return SkyParametersGPU{
        .star_direction_and_constant =
            glm::vec4(star_direction, star_constant),
        .star_color_and_solid_angle =
            glm::vec4(star_color, star_solid_angle),
        .rayleigh_and_height_r =
            glm::vec4(rayleigh_coef, height_rayleigh),
        .mie_and_height_m = glm::vec4(mie_coef, height_mie),
        .ozone_and_height_o = glm::vec4(ozone_coef, height_ozone),
        .radii_and_efficiency = glm::vec4(radius_planet, radius_atmosphere,
                                          luminous_efficiency, 0.0f)};
}

// Verify GPU struct size (6 * 16 = 96 bytes)
static_assert(sizeof(SkyParametersGPU) == 96,
              "SkyParametersGPU must be 96 bytes");
