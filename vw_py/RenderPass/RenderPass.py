import bindings_vw_py as bindings

class RenderPass:
    def __init__(self, device, render_pass):
        self.device = device
        self.render_pass = render_pass
    
    def __del__(self):
        print("Destroy Render Pass")
        bindings.vw_destroy_render_pass(self.render_pass)

class RenderPassBuilder:
    subpasses = []
    c_subpasses = bindings.SubpassVector()

    def __init__(self, device):
        self.device = device

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