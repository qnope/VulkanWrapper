import bindings_vw_py as bindings

class Device:
    def __init__(self, instance, surface, device):
        self.instance = instance
        self.surface = surface
        self.device = device

    def __del__(self):
        bindings.vw_destroy_device(self.device)
        print("Remove Device")