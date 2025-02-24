import bindings_vw_py as bindings
from weakref import finalize

class Fence:
    def __init__(self, device, fence):
        self.device = device
        self.fence = fence
        finalize(self, bindings.vw_destroy_fence, fence)

    def wait(self):
        bindings.vw_wait_fence(self.fence)

    def reset(self):
        bindings.vw_reset_fence(self.fence)

class FenceBuilder:
    def __init__(self, device):
        self.device = device

    def build(self):
        fence = bindings.vw_create_fence(self.device.device)
        return Fence(self.device, fence)