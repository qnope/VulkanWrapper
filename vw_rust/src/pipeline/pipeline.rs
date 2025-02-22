use std::ptr;

use super::pipeline_layout::PipelineLayout;
use super::shaders::ShaderModule;
use crate::render_pass::render_pass::RenderPass;
use crate::sys::bindings::{
    self, vw_Pipeline, VkPipeline, VwShaderStageFlagBits, VwGraphicsPipelineCreateArguments,
    VwStageAndShader,
};
use crate::vulkan::device::Device;

pub struct Pipeline<'a> {
    _device: &'a Device<'a>,
    _render_pass: &'a RenderPass<'a>,
    ptr: *mut vw_Pipeline,
}

pub struct GraphicsPipelineBuilder<'a> {
    device: &'a Device<'a>,
    render_pass: &'a RenderPass<'a>,
    stage_and_shaders: Vec<(VwShaderStageFlagBits, ShaderModule<'a>)>,

    viewport: Option<(i32, i32)>,
    scissor: Option<(i32, i32)>,

    pipeline_layout: Option<&'a PipelineLayout<'a>>,
    number_color_attachment: i32,
}

impl<'a> GraphicsPipelineBuilder<'a> {
    pub fn new(
        device: &'a Device<'a>,
        render_pass: &'a RenderPass<'a>,
    ) -> GraphicsPipelineBuilder<'a> {
        GraphicsPipelineBuilder {
            device: device,
            render_pass: render_pass,
            stage_and_shaders: vec![],
            viewport: None,
            scissor: None,
            pipeline_layout: None,
            number_color_attachment: 0,
        }
    }

    pub fn add_shader(
        mut self,
        stage: VwShaderStageFlagBits,
        shader: ShaderModule<'a>,
    ) -> GraphicsPipelineBuilder<'a> {
        self.stage_and_shaders.push((stage, shader));
        self
    }

    pub fn with_fixed_scissor(mut self, size: (i32, i32)) -> GraphicsPipelineBuilder<'a> {
        self.scissor = Some(size);
        self
    }

    pub fn with_fixed_viewport(mut self, size: (i32, i32)) -> GraphicsPipelineBuilder<'a> {
        self.viewport = Some(size);
        self
    }

    pub fn with_pipeline_layout(
        mut self,
        pipeline_layout: &'a PipelineLayout<'a>,
    ) -> GraphicsPipelineBuilder<'a> {
        self.pipeline_layout = Some(pipeline_layout);
        self
    }

    pub fn add_color_attachment(mut self) -> GraphicsPipelineBuilder<'a> {
        self.number_color_attachment += 1;
        self
    }

    pub fn build(self) -> Pipeline<'a> {
        let stage_and_shaders: Vec<_> = self
            .stage_and_shaders
            .iter()
            .map(|(stage, shader)| VwStageAndShader {
                stage: *stage,
                module: shader.as_mut_ptr(),
            })
            .collect();

        let arguments = VwGraphicsPipelineCreateArguments {
            device: self.device.as_ptr(),
            renderPass: self.render_pass.as_ptr(),
            stageAndShaders: stage_and_shaders.as_ptr(),
            size: stage_and_shaders.len() as i32,
            withViewport: self.viewport.is_some(),
            withScissor: self.scissor.is_some(),
            widthViewport: self.viewport.unwrap_or((0, 0)).0,
            heightViewport: self.viewport.unwrap_or((0, 0)).1,
            widthScissor: self.scissor.unwrap_or((0, 0)).0,
            heightScissor: self.scissor.unwrap_or((0, 0)).1,
            layout: match self.pipeline_layout {
                Some(x) => x.as_ptr(),
                _ => ptr::null(),
            },
            number_color_attachment: self.number_color_attachment,
        };

        unsafe {
            Pipeline {
                _device: self.device,
                _render_pass: self.render_pass,
                ptr: bindings::vw_create_graphics_pipeline(&arguments),
            }
        }
    }
}

impl<'a> Pipeline<'a> {
    pub fn as_ptr(&self) -> *const vw_Pipeline {
        self.ptr
    }

    pub fn handle(&self) -> VkPipeline {
        unsafe { bindings::vw_pipeline_handle(self.ptr) }
    }
}

impl<'a> Drop for Pipeline<'a> {
    fn drop(&mut self) {
        unsafe {
            bindings::vw_destroy_pipeline(self.ptr);
        }
    }
}
