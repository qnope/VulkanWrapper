import bindings_vw_py as bindings

class Image:
    def __init__(self, owner, image):
        self.owner = owner
        self.image = image

    def __del__(self):
        print("Remove Image")
        bindings.vw_destroy_image(self.image)