import bindings_vw_py as bindings
from weakref import finalize

class CommandPool:
    def __init__(self, device, command_pool):
        self.device = device
        self.command_pool = command_pool
        finalize(self, bindings.vw_destroy_command_pool, command_pool)

    def allocate(self, number):
        array = bindings.vw_command_pool_allocate(self.command_pool, number)
        result = bindings.command_buffer_array_to_vector(array.commandBuffers, array.size)
        bindings.free(array.commandBuffers)
        return result

class CommandPoolBuilder:
    def __init__(self, device):
        self.device = device

    def build(self):
        command_pool = bindings.vw_create_command_pool(self.device.device)
        return CommandPool(self.device, command_pool)