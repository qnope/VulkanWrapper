export module vw.test;
export import vw;

export namespace vw::tests {

struct GPU {
    std::shared_ptr<Instance> instance;
    std::shared_ptr<Device> device;
    std::shared_ptr<Allocator> allocator;

    vw::Queue &queue() { return device->graphicsQueue(); }
};

GPU &create_gpu();

} // namespace vw::tests
