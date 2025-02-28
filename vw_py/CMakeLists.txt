cmake_minimum_required(VERSION 3.25)

find_package(Python3 REQUIRED COMPONENTS Development)
find_package(SWIG 4.3 REQUIRED COMPONENTS python)

include(UseSWIG)

set_property(SOURCE vw.i PROPERTY CPLUSPLUS ON)
swig_add_library(bindings_vw_py TYPE SHARED LANGUAGE Python SOURCES vw.i)

if(${APPLE})
    set_property(TARGET bindings_vw_py PROPERTY SUFFIX ".so")
endif()

set_property(TARGET bindings_vw_py PROPERTY SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
set_property(TARGET bindings_vw_py PROPERTY SWIG_DEPENDS VulkanWrapper::Wrapper)

target_link_libraries(bindings_vw_py PRIVATE Python3::Python VulkanWrapper::Wrapper)

