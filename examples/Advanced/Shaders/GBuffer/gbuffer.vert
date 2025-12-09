#version 450

layout(set = 0, binding = 0, std140) uniform UBO {
    mat4 proj;
    mat4 view;
};

layout(push_constant) uniform PushConstants {
    mat4 model;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangeant;
layout(location = 3) in vec3 inBitangeant;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec3 outTangeant;
layout(location = 2) out vec3 outBiTangeant;
layout(location = 3) out vec2 outTexCoord;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;

    // Transform TBN vectors by the model matrix (normal matrix for correct transformation)
    // For uniform scaling, mat3(model) is sufficient; for non-uniform scaling,
    // use transpose(inverse(mat3(model)))
    mat3 normalMatrix = mat3(model);
    outNormal = normalize(normalMatrix * inNormal);
    outTangeant = normalize(normalMatrix * inTangeant);
    outBiTangeant = normalize(normalMatrix * inBitangeant);
}