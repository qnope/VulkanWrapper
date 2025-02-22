import bindings_vw_py as bindings

class Attachment:
    def __init__(self, id, format, final_layout):
        self.id = bindings.CString(id)
        self.attachment = bindings.VwAttachment()
        self.attachment.finalLayout = final_layout
        self.attachment.format = format
        self.attachment.id = bindings.VwString(self.id)

class AttachmentBuilder:
    def __init__(self, id):
        self.id = id
        self.format = 0
        self.final_layout = 0

    def with_final_layout(self, layout):
        self.final_layout = layout
        return self
    
    def with_format(self, format):
        self.format = format
        return self
    
    def build(self):
        return Attachment(self.id, self.format, self.final_layout)
