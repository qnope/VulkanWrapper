import bindings_vw_py as bindings
from weakref import finalize

class PipelineBoundCommandBufferRecorder:
    def __init__(self, recorder):
        self.recorder = recorder
    
    def draw(self, number_vertex, number_instance, first_vertex, first_instance):
        bindings.vw_draw(self.recorder, number_vertex, number_instance, first_vertex, first_instance)
    
    def __enter__(self):
        return self

    def __exit__(self, *args):
        bindings.vw_destroy_pipeline_bound_command_buffer_recorder(self.recorder)

class RenderPassCommandBufferRecorder:
    def __init__(self, recorder):
        self.recorder = recorder

    def bind_graphics_pipeline(self, pipeline):
        return PipelineBoundCommandBufferRecorder(bindings.vw_bind_graphics_pipeline(self.recorder, pipeline.pipeline))

    def __enter__(self):
        return self

    def __exit__(self, *args):
        bindings.vw_destroy_render_pass_command_buffer_recorder(self.recorder)

class CommandBufferRecorder:
    def __init__(self, cmd_buffer):
        self.recorder = bindings.vw_create_command_buffer_recorder(cmd_buffer)
    
    def begin_render_pass(self, render_pass, framebuffer):
        return RenderPassCommandBufferRecorder(bindings.vw_begin_render_pass(self.recorder, render_pass.render_pass, framebuffer.framebuffer))

    def __enter__(self):
        return self
    
    def __exit__(self, *args):
        bindings.vw_destroy_command_buffer_recorder(self.recorder)