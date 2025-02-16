/* example.i */
%module bindings_vw_py
%{
#include <iostream>
#include "bindings.h"

template<typename T>
PyObject *create_object(T *object) = delete;

template<>
PyObject *create_object<char>(char *object) {
    return PyUnicode_FromString(object);
}

template<typename T>
PyObject *create_pylist_from_ptr(char **list, int size) {
    auto *result = PyList_New(size);

    for(int i = 0; i < size; ++i) {
        auto *object = create_object(list[i]);
        PyList_SetItem(result, i, object);
    }

    return result;
}

/*
VwString *create_vw_string_from_pystring(PyObject *object) {
    std::cout << "Create vw_string" << std::endl;
    Py_ssize_t size = 0;
    const char *string = PyUnicode_AsUTF8AndSize(object, &size);
    char *result = new char[size + 1];
    std::memcpy(result, string, size + 1);
    return new VwString{result};
}

PyObject *pystring_from_vw_string(const VwString *string) {
    return PyUnicode_FromString(string->string);
}*/

%}

template<typename T>
PyObject *create_pylist_from_ptr(char **list, int size);

%template(create_pylist_string) create_pylist_from_ptr<char>;

%typemap(in) VwString {
    $1.string = PyUnicode_AsUTF8($input);
}

%typemap(out) VwString {
    $result = PyUnicode_FromString($1.string);
}

%typemap(in) char ** {
    const auto size = PyList_Size($input);
    std::cout << "Construction to give to C" << Py_REFCNT($input) << std::endl;
    $1 = new char*[size];
    for(int i = 0; i < size; ++i) {
        const auto current_string = PyList_GetItem($input, i);
        $1[i] = (char*)PyUnicode_AsUTF8(current_string);
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
