import bindings_vw_py as bindings

class Subpass:
    def __init__(self, subpass):
        self.subpass = subpass

    def __del__(self):
        print("Remove Subpass")
        bindings.vw_destroy_subpass(self.subpass)

class SubpassBuilder:
    def __init__(self):
        self.attachment_subpasses = bindings.AttachmentSubpassVector()

    def add_color_attachment(self, attachment, layout):
        attachment_subpass = bindings.VwAttachmentSubpass()
        attachment_subpass.attachment = attachment.attachment
        attachment_subpass.currentLayout = layout
        self.attachment_subpasses.push_back(attachment_subpass)
        return self
    
    def build(self):
        arguments = bindings.VwSubpassCreateArguments()
        arguments.attachments = bindings.attachment_subpass_vector_data(self.attachment_subpasses)
        arguments.attachment_count = len(self.attachment_subpasses)
        subpass = bindings.vw_create_subpass(arguments)
        return Subpass(subpass)