// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  tutorialSidebar: [
    'intro',
    'getting-started',
    {
      type: 'category',
      label: 'Core Concepts',
      items: [
        'core-concepts/architecture',
        'core-concepts/builder-pattern',
        'core-concepts/raii-ownership',
        'core-concepts/type-safety',
        'core-concepts/error-handling',
      ],
    },
    {
      type: 'category',
      label: 'Modules',
      items: [
        {
          type: 'category',
          label: 'Vulkan',
          items: [
            'modules/vulkan/instance',
            'modules/vulkan/device',
            'modules/vulkan/swapchain',
          ],
        },
        {
          type: 'category',
          label: 'Memory',
          items: [
            'modules/memory/allocator',
            'modules/memory/buffers',
            'modules/memory/staging',
          ],
        },
        {
          type: 'category',
          label: 'Image',
          items: [
            'modules/image/images',
            'modules/image/image-views',
            'modules/image/samplers',
          ],
        },
        {
          type: 'category',
          label: 'Pipeline',
          items: [
            'modules/pipeline/graphics-pipeline',
            'modules/pipeline/pipeline-layout',
            'modules/pipeline/shaders',
          ],
        },
        {
          type: 'category',
          label: 'Descriptors',
          items: [
            'modules/descriptors/descriptor-sets',
            'modules/descriptors/descriptor-allocator',
            'modules/descriptors/vertices',
          ],
        },
        {
          type: 'category',
          label: 'Command',
          items: [
            'modules/command/command-pool',
            'modules/command/command-buffer',
          ],
        },
        {
          type: 'category',
          label: 'Synchronization',
          items: [
            'modules/synchronization/fences-semaphores',
            'modules/synchronization/resource-tracker',
          ],
        },
        {
          type: 'category',
          label: 'Render Pass',
          items: [
            'modules/renderpass/subpass',
            'modules/renderpass/screen-space-pass',
            'modules/renderpass/sky-system',
            'modules/renderpass/tone-mapping',
          ],
        },
        {
          type: 'category',
          label: 'Ray Tracing',
          items: [
            'modules/raytracing/acceleration-structures',
            'modules/raytracing/ray-tracing-pipeline',
            'modules/raytracing/shader-binding-table',
          ],
        },
        {
          type: 'category',
          label: 'Model',
          items: [
            'modules/model/mesh',
            'modules/model/scene',
            'modules/model/materials',
          ],
        },
      ],
    },
    {
      type: 'category',
      label: 'Tutorials',
      items: [
        'tutorials/hello-triangle',
        'tutorials/deferred-rendering',
        'tutorials/ray-traced-shadows',
      ],
    },
    {
      type: 'category',
      label: 'API Reference',
      items: [
        'api-reference/namespaces',
        'api-reference/classes',
        'api-reference/concepts',
      ],
    },
  ],
};

export default sidebars;
