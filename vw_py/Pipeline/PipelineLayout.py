import bindings_vw_py as bindings
from weakref import finalize

class PipelineLayout:
    def __init__(self, device, pipeline_layout):
        self.device = device
        self.pipeline_layout = pipeline_layout
        finalize(self, bindings.vw_destroy_pipeline_layout, pipeline_layout)

class PipelineLayoutBuilder:
    def __init__(self, device):
        self.device = device

    def build(self):
        pipeline_layout = bindings.vw_create_pipeline_layout(self.device.device)
        return PipelineLayout(self.device, pipeline_layout)