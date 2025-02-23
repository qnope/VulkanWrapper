import bindings_vw_py as bindings

class PresentQueue:
    def __init__(self, device, queue):
        self.device = device
        self.queue = queue

    def present(self, swapchain, index, render_finished_semaphore):
        arguments = bindings.VwPresentQueueArguments()
        arguments.swapchain = swapchain.swapchain
        arguments.image_index = index
        arguments.wait_semaphore = render_finished_semaphore.semaphore
        bindings.vw_present_queue_present(self.queue, arguments)