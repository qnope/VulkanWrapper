import bindings_vw_py as bindings
from weakref import finalize

class Semaphore:
    def __init__(self, device, semaphore):
        self.device = device
        self.semaphore = semaphore
        finalize(self, bindings.vw_destroy_semaphore, semaphore)

    def handle(self):
        return bindings.vw_semaphore_handle(self.semaphore)

class SemaphoreBuilder:
    def __init__(self, device):
        self.device = device

    def build(self):
        semaphore = bindings.vw_create_semaphore(self.device.device)
        return Semaphore(self.device, semaphore)