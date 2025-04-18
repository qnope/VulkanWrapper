cmake_minimum_required(VERSION 3.25)

include(FetchContent)

find_package(glm REQUIRED COMPONENTS glm)
find_package(assimp REQUIRED COMPONENTS assimp)
find_package(SDL3 REQUIRED COMPONENTS SDL3)
find_package(Vulkan REQUIRED COMPONENTS glslangValidator)
find_package(VulkanMemoryAllocator REQUIRED)
find_package(SDL3_image REQUIRED)

add_library(VulkanWrapperCoreLibrary SHARED)

if(NOT Vulkan_GLSLANG_VALIDATOR_EXECUTABLE)
    message(FATAL_ERROR "glslc not found")
endif()

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(VulkanWrapperCoreLibrary PUBLIC include)
target_compile_features(VulkanWrapperCoreLibrary PUBLIC cxx_std_23)
target_compile_definitions(VulkanWrapperCoreLibrary PRIVATE VW_LIB)
target_link_libraries(VulkanWrapperCoreLibrary PUBLIC Vulkan::Vulkan assimp::assimp glm::glm GPUOpen::VulkanMemoryAllocator SDL3::SDL3)
target_link_libraries(VulkanWrapperCoreLibrary PRIVATE $<IF:$<TARGET_EXISTS:SDL3_image::SDL3_image-shared>,SDL3_image::SDL3_image-shared,SDL3_image::SDL3_image-static>)
target_precompile_headers(VulkanWrapperCoreLibrary PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/VulkanWrapper/3rd_party.h)

add_library(VulkanWrapper::VW ALIAS VulkanWrapperCoreLibrary)

function(VwCompileShader TARGET_NAME SHADER_PATH RELATIVE_OUTPUT_PATH)
    # Obtenir le chemin du fichier appelant
    get_filename_component(CALLER_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)
    set(ABS_SHADER_PATH "${CALLER_DIR}/${SHADER_PATH}")
    get_filename_component(SHADER_FILENAME "${SHADER_PATH}" NAME)

    # On essaie d’obtenir le chemin où la cible va générer ses binaires
    get_target_property(RUNTIME_DIR ${TARGET_NAME} RUNTIME_OUTPUT_DIRECTORY)

    if(NOT RUNTIME_DIR)
        # Si la propriété n’est pas définie, on suppose que c’est dans le binaire courant
        set(RUNTIME_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    endif()

    if(NOT RUNTIME_DIR)
        # Si toujours rien (valeur par défaut non définie), on fallback sur le répertoire de build local
        set(RUNTIME_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    # Destination finale du .spv
    set(OUTPUT_SHADER_DIR "${RUNTIME_DIR}")
    file(MAKE_DIRECTORY "${OUTPUT_SHADER_DIR}")

    set(COMPILED_SHADER "${OUTPUT_SHADER_DIR}/${RELATIVE_OUTPUT_PATH}")

    add_custom_command(
        OUTPUT "${COMPILED_SHADER}"
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V "${ABS_SHADER_PATH}" -o "${COMPILED_SHADER}"
        DEPENDS "${ABS_SHADER_PATH}"
        COMMENT "Compile Shader : ${COMPILED_SHADER}"
        VERBATIM
    )

    add_custom_target("${TARGET_NAME}_build_shader_${SHADER_FILENAME}" ALL
        DEPENDS "${COMPILED_SHADER}"
    )
    add_dependencies(${TARGET_NAME} "${TARGET_NAME}_build_shader_${SHADER_FILENAME}")

endfunction()

add_custom_command(TARGET VulkanWrapper::VW POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:VulkanWrapper::VW> ${CMAKE_SOURCE_DIR}/vw_rust/)
