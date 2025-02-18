/* example.i */
%module bindings_vw_py
%{
#include <iostream>
#include "bindings.h"
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
            c_array.emplace_back(array.back().c_str());
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

%include "Command/CommandPool.h"
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

%extend VwAttachment {
    ~VwAttachment() {
        delete[] self->id;
        delete self;
    }
}
