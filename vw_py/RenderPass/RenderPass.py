import bindings_vw_py as bindings
from weakref import finalize

class RenderPass:
    def __init__(self, device, render_pass):
        self.device = device
        self.render_pass = render_pass
        finalize(self, bindings.vw_destroy_render_pass, render_pass)

class RenderPassBuilder:
    def __init__(self, device):
        self.device = device
        self.subpasses = []
        self.c_subpasses = bindings.SubpassVector()

    def add_subpass(self, subpass):
        self.subpasses.append(subpass)
        self.c_subpasses.push_back(subpass.subpass)
        return self
    
    def build(self):
        arguments = bindings.VwRenderPassCreateArguments()
        arguments.subpasses = bindings.subpass_vector_data(self.c_subpasses)
        arguments.size = len(self.c_subpasses)
        render_pass = bindings.vw_create_render_pass(self.device.device, arguments)
        return RenderPass(self.device, render_pass)