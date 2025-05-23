import bindings_vw_py as bindings
from weakref import finalize

class ImageView:
    def __init__(self, image, image_view):
        self.image = image
        self.image_view = image_view
        finalize(self, bindings.vw_destroy_image_view, image_view)

class ImageViewBuilder:
    def __init__(self, device, image):
        self.device = device
        self.image = image
        self.image_type = 0

    def with_type(self, image_type):
        self.image_type = image_type
        return self
    
    def build(self):
        arguments = bindings.VwImageViewCreateArguments()
        arguments.device = self.device.device
        arguments.image = self.image.image
        arguments.image_type = self.image_type
        image_view = bindings.vw_create_image_view(arguments)
        return ImageView(self.image, image_view)