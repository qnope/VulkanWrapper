#include "VulkanWrapper/RenderPass/SkyParameters.h"

namespace vw {

glm::vec3 SkyParameters::angle_to_direction(float angle_deg) {
    float angle_rad = glm::radians(angle_deg);
    return glm::vec3(std::cos(angle_rad), std::sin(angle_rad), 0.0f);
}

glm::vec3 SkyParameters::temperature_to_color(float temperature_kelvin) {
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

float SkyParameters::angular_diameter_to_solid_angle(float angular_diameter_deg) {
    float radius_rad = glm::radians(angular_diameter_deg / 2.0f);
    constexpr float PI = 3.14159265359f;
    return 2.0f * PI * (1.0f - std::cos(radius_rad));
}

float SkyParameters::compute_radiance(float solar_constant, float solid_angle) {
    return solar_constant / solid_angle;
}

SkyParameters SkyParameters::create_earth_sun(float sun_angle_deg) {
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

} // namespace vw
