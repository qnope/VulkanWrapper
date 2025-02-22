import bindings_vw_py as bindings

class ShaderModule:
    def __init__(self, device, shader_module):
        self.device = device
        self.shader_module = shader_module

    def __del__(self):
        bindings.vw_destroy_shader_module(self.shader_module)
        print("Remove Shader Module")

def from_spirv_file(device, path):
    path_string = bindings.CString(path)
    shader_module = bindings.vw_create_shader_module_from_spirv_file(device.device, bindings.VwString(path_string))
    return ShaderModule(device, shader_module)

