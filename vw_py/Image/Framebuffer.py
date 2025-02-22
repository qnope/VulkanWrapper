import bindings_vw_py as bindings

class Framebuffer:
    def __init__(self, render_pass, image_views, framebuffer):
        self.render_pass = render_pass
        self.image_views = image_views
        self.framebuffer = framebuffer

    def __del__(self):
        print("Remove framebuffer")
        bindings.vw_destroy_framebuffer(self.framebuffer)

class FramebufferBuilder:
    def __init__(self, device, render_pass, width, height):
        self.device = device
        self.render_pass = render_pass
        self.width = width
        self.height = height
        self.image_views = []

    def add_attachment(self, image_view):
        self.image_views.append(image_view)
        return self
    
    def build(self):
        vector = bindings.ImageViewVector([x.image_view for x in self.image_views])
        arguments = bindings.VwFramebufferCreateArguments()
        arguments.device = self.device.device
        arguments.width = self.width
        arguments.height = self.height
        arguments.render_pass = self.render_pass.render_pass
        arguments.number_image_views = len(vector)
        arguments.image_views = bindings.image_views_vector_data(vector)
        framebuffer = bindings.vw_create_framebuffer(arguments)
        return Framebuffer(self.render_pass, self.image_views, framebuffer)
        