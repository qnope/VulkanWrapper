import bindings_vw_py as bindings

class Pipeline:
    def __init__(self, device, render_pass, pipeline_layout, pipeline):
        self.device = device
        self.render_pass = render_pass
        self.pipeline_layout = pipeline_layout
        self.pipeline = pipeline

    def __del__(self):
        print("Destroy Pipeline")
        bindings.vw_destroy_pipeline(self.pipeline)

class GraphicsPipelineBuilder:
    viewport = None
    scissor = None
    pipeline_layout = None
    number_color_attachment = 0
    shader_and_stage_vector = bindings.StageAndShaderVector()

    def __init__(self, device, render_pass):
        self.device = device
        self.render_pass = render_pass
    
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
