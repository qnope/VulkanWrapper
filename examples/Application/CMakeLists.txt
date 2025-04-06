add_library(App STATIC Application.cpp Application.h)
target_link_libraries(App PUBLIC VulkanWrapper::VW)
target_include_directories(App PUBLIC .)
