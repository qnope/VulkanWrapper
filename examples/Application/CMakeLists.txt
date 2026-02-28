add_library(App STATIC Application.cpp)
target_sources(App PUBLIC FILE_SET CXX_MODULES FILES Application.cppm)
target_link_libraries(App PUBLIC VulkanWrapper::VW)
