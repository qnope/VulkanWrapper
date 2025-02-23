import bindings_vw_py as bindings

class GraphicsQueue:
    def __init__(self, device, queue):
        self.device = device
        self.queue = queue

    def submit(self, cmd_buffers, wait_stages, wait_semaphores, signal_semaphores, fence):
        cmd = bindings.VkHandleVector(cmd_buffers)
        stages = bindings.EnumVector(wait_stages)
        wait_sem = bindings.VkHandleVector([x.handle() for x in wait_semaphores])
        sig_sem = bindings.VkHandleVector([x.handle() for x in signal_semaphores])

        arguments = bindings.VwQueueSubmitArguments()
        arguments.command_buffers = bindings.vk_handle_vector_data(cmd)
        arguments.command_buffer_count = len(cmd)

        arguments.wait_stages = bindings.vw_pipeline_stage_flag_bits_vector_data(stages)
        arguments.wait_stage_count = len(stages)

        arguments.wait_semaphores = bindings.vk_handle_vector_data(wait_sem)
        arguments.wait_semaphore_count = len(wait_sem)

        arguments.signal_semaphores = bindings.vk_handle_vector_data(sig_sem)
        arguments.signal_semaphores_count = len(sig_sem)

        arguments.fence = fence.fence
        bindings.vw_queue_submit(self.queue, arguments)