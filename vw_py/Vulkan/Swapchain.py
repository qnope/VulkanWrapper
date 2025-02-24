import bindings_vw_py as bindings
from weakref import finalize
from Image import Image

class Swapchain:
    def __init__(self, device, surface, swapchain): 
        self.device = device
        self.surface = surface
        self.swapchain = swapchain
        finalize(self, bindings.vw_destroy_swapchain, swapchain)

    def width(self):
        return bindings.vw_get_swapchain_width(self.swapchain)
    
    def height(self):
        return bindings.vw_get_swapchain_height(self.swapchain)

    def format(self):
        return bindings.vw_get_swapchain_format(self.swapchain)
    
    def acquire_next_image(self, semaphore):
        return bindings.vw_swapchain_acquire_next_image(self.swapchain, semaphore.semaphore)
    
    def images(self):
        images = bindings.vw_swapchain_get_images(self.swapchain)
        vector = bindings.swapchain_image_array_to_vector(images.images, images.size)
        bindings.free(images.images)
        return [Image.Image(self, img) for img in vector]
        