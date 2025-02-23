/* example.i */
%module bindings_vw_py
%{
#include <iostream>
#include <vector>
#include "bindings.h"

template<typename T>
auto *data_from_vector(std::vector<T> *x) {
    return x->data();
}

template<typename T>
const T *data_from_vector_enum(std::vector<int> *x) {
    return reinterpret_cast<T*>(x->data());
}

template<typename T>
std::vector<T> data_to_vector(const T *array, int number) {
    return std::vector(array, array + number);
}

%}

%include "std_string.i"
%include "std_vector.i"

namespace std {
    %template(vector_string) std::vector<std::string>;
}

%inline {
    struct CString {
        CString(const std::string &x) : string{x} {}
        std::string string;
    };

    struct CStringArray {
        CStringArray() {}

        CStringArray(const std::vector<std::string> &x) : array{x}{
            build_c_array();
        }

        CStringArray(const VwStringArray &x) {
            for(int i = 0; i < x.number; ++i)
                array.push_back(x.array[i].string);
            build_c_array();
        }

        void push(const std::string &x) {
            array.push_back(x);
            build_c_array();
        }

        void push(const std::vector<std::string> &array) {
            for(auto &x: array)
                push(x);
        }

        void push(const VwStringArray &array) {
            for(int i = 0; i < array.number; ++i)
                push(array.array[i].string);
        }

        void build_c_array() {
            c_array.clear();
            for(const auto &x: array)
                c_array.emplace_back(x.c_str());
        }

        std::vector<std::string> array;
        std::vector<VwString> c_array;
    };
}

%include "utils/utils.h"

%extend VwString {
    VwString(const CString &x) {
        return new VwString{x.string.c_str()};
    }
}

%extend VwStringArray {
    VwStringArray(const CStringArray &x) {
        return new VwStringArray {x.c_array.data(), static_cast<int>(x.array.size())}; 
    }
}

%apply int* OUTPUT {int *output_number}

%include "Vulkan/enums.h"
%include "Command/CommandPool.h"
%include "Command/CommandBuffer.h"
%include "Image/Framebuffer.h"
%include "Image/Image.h"
%include "Image/ImageView.h"
%include "Pipeline/Pipeline.h"
%include "Pipeline/PipelineLayout.h"
%include "Pipeline/ShaderModule.h"
%include "RenderPass/Attachment.h"
%include "RenderPass/RenderPass.h"
%include "RenderPass/Subpass.h"
%include "Synchronization/Fence.h"
%include "Synchronization/Semaphore.h"
%include "Vulkan/Device.h"
%include "Vulkan/Instance.h"
%include "Vulkan/PresentQueue.h"
%include "Vulkan/Queue.h"
%include "Vulkan/Swapchain.h"
%include "Window/SDL_Initializer.h"
%include "Window/Window.h"

void free(void*);

template<typename T>
T *data_from_vector(std::vector<T> *x);

typedef void* VkCommandBuffer;
typedef void* VkSemaphore;

template<typename T>
std::vector<T> data_to_vector(T *x, int number);

template<typename T>
const T *data_from_vector_enum(std::vector<int> *x);

%template(EnumVector) std::vector<int>;

%template(VkHandleVector) std::vector<void*>;
%template(vk_handle_vector_data) data_from_vector<void*>;

%template(AttachmentSubpassVector) std::vector<VwAttachmentSubpass>;
%template(SubpassVector) std::vector<vw::Subpass*>;
%template(StageAndShaderVector) std::vector<VwStageAndShader>;
%template(ImageVector) std::vector<vw::Image*>;
%template(ImageViewVector) std::vector<vw::ImageView*>;

%template(attachment_subpass_vector_data) data_from_vector<VwAttachmentSubpass>;
%template(subpass_vector_data) data_from_vector<vw::Subpass*>;
%template(stage_and_shader_vector_data) data_from_vector<VwStageAndShader>;
%template(image_views_vector_data) data_from_vector<vw::ImageView*>;

%template(command_buffer_array_to_vector) data_to_vector<VkCommandBuffer>;
%template(swapchain_image_array_to_vector) data_to_vector<vw::Image*>;

%template(vw_pipeline_stage_flag_bits_vector_data) data_from_vector_enum<VwPipelineStageFlagBits>;