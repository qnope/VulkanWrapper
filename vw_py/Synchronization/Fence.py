import bindings_vw_py as bindings

class Fence:
    def __init__(self, device, fence):
        self.device = device
        self.fence = fence

    def wait(self):
        bindings.vw_wait_fence(self.fence)

    def reset(self):
        bindings.vw_reset_fence(self.fence)

    def __del__(self):
        print("Remove Fence")
        bindings.vw_destroy_fence(self.fence)

class FenceBuilder:
    def __init__(self, device):
        self.device = device

    def build(self):
        fence = bindings.vw_create_fence(self.device.device)
        return Fence(self.device, fence)