#pragma once

// Material type tag macros
// These macros must be used outside of modules (in .cpp files or non-module
// headers) because macros cannot be exported from C++20 modules.

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define VW_DECLARE_MATERIAL_TYPE(Name)                                         \
    extern const ::vw::Model::Material::MaterialTypeTag Name

#define VW_DEFINE_MATERIAL_TYPE(Name)                                          \
    const ::vw::Model::Material::MaterialTypeTag Name {                        \
        ::vw::Model::Material::detail::g_next_material_type_id++               \
    }
// NOLINTEND(cppcoreguidelines-macro-usage)
