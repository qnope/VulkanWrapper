import bindings_vw_py as bindings
from Vulkan import DeviceFinder

class Instance:
    def __init__(self, instance):
        self.instance = instance

    def find_gpu(self):
        return DeviceFinder.DeviceFinder(self, bindings.vw_find_gpu_from_instance(self.instance))

    def __del__(self):
        print("remove instance")
        bindings.vw_destroy_instance(self.instance)

class InstanceBuilder:
    extensions = bindings.CStringArray()
    debug_mode = False

    def __init__(self):
        self.extensions = bindings.CStringArray()
        self.debug_mode = True

    def add_extensions(self, extensions):
        self.extensions.push(extensions)
        return self

    def set_debug_mode(self):
        debug_mode = True
        return self

    def build(self):
        arguments = bindings.VwInstanceCreateArguments()
        arguments.debug_mode = self.debug_mode
        arguments.extensions = bindings.VwStringArray(self.extensions)
        instance = bindings.vw_create_instance(arguments)
        return Instance(instance)
