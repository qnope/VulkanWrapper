module;

#include <memory>
#include <shaderc/shaderc.hpp>
#include <string>
#include <vector>

export module shaderc3rd;

// Shaderc C types and enum values
export using ::shaderc_shader_kind;
export using ::shaderc_env_version;
export using ::shaderc_spirv_version;
export using ::shaderc_include_result;
export using ::shaderc_include_type;
export using ::shaderc_compilation_status;
export using ::shaderc_vertex_shader;
export using ::shaderc_tess_control_shader;
export using ::shaderc_tess_evaluation_shader;
export using ::shaderc_geometry_shader;
export using ::shaderc_fragment_shader;
export using ::shaderc_compute_shader;
export using ::shaderc_raygen_shader;
export using ::shaderc_anyhit_shader;
export using ::shaderc_closesthit_shader;
export using ::shaderc_miss_shader;
export using ::shaderc_intersection_shader;
export using ::shaderc_callable_shader;
export using ::shaderc_task_shader;
export using ::shaderc_mesh_shader;
export using ::shaderc_env_version_vulkan_1_3;
export using ::shaderc_env_version_vulkan_1_2;
export using ::shaderc_env_version_vulkan_1_1;
export using ::shaderc_env_version_vulkan_1_0;
export using ::shaderc_spirv_version_1_6;
export using ::shaderc_spirv_version_1_5;
export using ::shaderc_spirv_version_1_3;
export using ::shaderc_spirv_version_1_0;
export using ::shaderc_include_type_standard;
export using ::shaderc_target_env_vulkan;
export using ::shaderc_optimization_level_size;
export using ::shaderc_optimization_level_zero;
export using ::shaderc_compilation_status_success;

export namespace shaderc {
using shaderc::CompileOptions;
using shaderc::Compiler;
using shaderc::SpvCompilationResult;
} // namespace shaderc
