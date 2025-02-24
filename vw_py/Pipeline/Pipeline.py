import bindings_vw_py as bindings
from weakref import finalize

class Pipeline:
    def __init__(self, device, render_pass, pipeline_layout, pipeline):
        self.device = device
        self.render_pass = render_pass
        self.pipeline_layout = pipeline_layout
        self.pipeline = pipeline
        finalize(self, bindings.vw_destroy_pipeline, pipeline)

class GraphicsPipelineBuilder:
    def __init__(self, device, render_pass):
        self.device = device
        self.render_pass = render_pass
        self.viewport = None
        self.scissor = None
        self.pipeline_layout = None
        self.number_color_attachment = 0
        self.shader_and_stage_vector = bindings.StageAndShaderVector()
    
    def add_shader(self, stage, module):
        new = bindings.VwStageAndShader()
        new.module = module.shader_module
        new.stage = stage
        self.shader_and_stage_vector.push_back(new)
        return self
    
    def with_fixed_viewport(self, width, height):
        self.viewport = (width, height)
        return self
    
    def with_fixed_scissor(self, width, height):
        self.scissor = (width, height)
        return self
    
    def with_pipeline_layout(self, pipeline_layout):
        self.pipeline_layout = pipeline_layout
        return self
    
    def add_color_attachment(self):
        self.number_color_attachment += 1
        return self
    
    def build(self):
        arguments = bindings.VwGraphicsPipelineCreateArguments()
        arguments.device = self.device.device
        arguments.renderPass = self.render_pass.render_pass
        arguments.number_color_attachment = self.number_color_attachment
        
        if self.pipeline_layout is not None:
            arguments.layout = self.pipeline_layout.pipeline_layout
        
        if self.scissor is not None:
            arguments.withScissor = True
            arguments.widthScissor = self.scissor[0]
            arguments.heightScissor = self.scissor[1]
        
        if self.viewport is not None:
            arguments.withViewport = True
            arguments.widthViewport = self.viewport[0]
            arguments.heightViewport = self.viewport[1]

        arguments.stageAndShaders = bindings.stage_and_shader_vector_data(self.shader_and_stage_vector)
        arguments.size = len(self.shader_and_stage_vector)
        pipeline = bindings.vw_create_graphics_pipeline(arguments)
        return Pipeline(self.device, self.render_pass, self.pipeline_layout, pipeline)
