module;

#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>

export module glm3rd;

export using ::int32_t;
export using ::uint32_t;
export using ::uint64_t;
export using ::size_t;

export namespace glm {
using glm::clamp;
using glm::cross;
using glm::degrees;
using glm::determinant;
using glm::distance;
using glm::dot;
using glm::inverse;
using glm::length;
using glm::lookAt;
using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::mix;
using glm::normalize;
using glm::perspective;
using glm::radians;
using glm::rotate;
using glm::scale;
using glm::translate;
using glm::transpose;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::operator+;
using glm::operator-;
using glm::operator*;
using glm::operator/;
using glm::operator==;
using glm::operator!=;
using glm::packHalf1x16;
} // namespace glm
