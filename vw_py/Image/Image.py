import bindings_vw_py as bindings
from weakref import finalize

class Image:
    def __init__(self, owner, image):
        self.owner = owner
        self.image = image
        finalize(self, bindings.vw_destroy_image, image)
