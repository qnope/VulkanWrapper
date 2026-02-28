export module vw.renderpass:sky_parameters;
import std3rd;
import glm3rd;

export namespace vw {

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

    static glm::vec3 angle_to_direction(float angle_deg);
    static glm::vec3 temperature_to_color(float temperature_kelvin);
    static float angular_diameter_to_solid_angle(float angular_diameter_deg);
    static float compute_radiance(float solar_constant, float solid_angle);

    // =========================================================================
    // Factory methods
    // =========================================================================

    static SkyParameters create_earth_sun(float sun_angle_deg);

    glm::vec3 direction_to_star() const { return -star_direction; }

    float star_radiance() const {
        return compute_radiance(star_constant, star_solid_angle);
    }

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
