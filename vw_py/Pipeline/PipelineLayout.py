import bindings_vw_py as bindings

class PipelineLayout:
    def __init__(self, device, pipeline_layout):
        self.device = device
        self.pipeline_layout = pipeline_layout

    def __del__(self):
        print("Remove Pipeline Layout")
        bindings.vw_destroy_pipeline_layout(self.pipeline_layout)

class PipelineLayoutBuilder:
    def __init__(self, device):
        self.device = device

    def build(self):
        pipeline_layout = bindings.vw_create_pipeline_layout(self.device.device)
        return PipelineLayout(self.device, pipeline_layout)