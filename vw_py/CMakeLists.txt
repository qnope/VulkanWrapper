cmake_minimum_required(VERSION 3.25)

find_package(Python3 REQUIRED COMPONENTS Development)
find_package(SWIG 4.3 REQUIRED COMPONENTS python)

set(UseSWIG_MODULE_VERSION 2)

include(UseSWIG)

set_property(SOURCE vw.i PROPERTY CPLUSPLUS ON)
swig_add_library(bindings_vw_py
    TYPE SHARED
    LANGUAGE Python
    SOURCES vw.i
    OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    OUTFILE_DIR ${CMAKE_CURRENT_BINARY_DIR}
)

message(Test "${UseSWIG_MODULE_VERSION}")
if(${APPLE})
    set_property(TARGET bindings_vw_py PROPERTY SUFFIX ".so")
endif()

set_property(TARGET bindings_vw_py PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET bindings_vw_py PROPERTY SWIG_DEPENDS VulkanWrapper::Wrapper)

target_link_libraries(bindings_vw_py PRIVATE Python3::Python VulkanWrapper::Wrapper)
add_custom_command(TARGET bindings_vw_py POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:bindings_vw_py> ${CMAKE_SOURCE_DIR}/vw_py/)

