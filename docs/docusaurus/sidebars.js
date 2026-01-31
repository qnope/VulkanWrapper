// @ts-check

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  tutorialSidebar: [
    'intro',
    {
      type: 'category',
      label: 'Getting Started',
      items: [
        'getting-started/introduction',
        'getting-started/installation',
        'getting-started/building',
      ],
    },
    {
      type: 'category',
      label: 'Tutorials',
      items: [
        {
          type: 'category',
          label: 'Basics',
          items: [
            'tutorials/hello-triangle',
            'tutorials/hello-texture',
            'tutorials/hello-3d',
          ],
        },
        {
          type: 'category',
          label: 'Core Concepts',
          items: [
            'tutorials/buffers',
            'tutorials/images',
            'tutorials/descriptors',
            'tutorials/pipelines',
          ],
        },
        {
          type: 'category',
          label: 'Intermediate',
          items: [
            'tutorials/model-loading',
            'tutorials/materials',
            'tutorials/lighting',
          ],
        },
        {
          type: 'category',
          label: 'Advanced',
          items: [
            'tutorials/deferred-rendering',
            'tutorials/ray-tracing',
            'tutorials/post-processing',
          ],
        },
      ],
    },
    {
      type: 'category',
      label: 'Architecture',
      items: [
        'architecture/overview',
        'architecture/memory-management',
        'architecture/resource-tracking',
        'architecture/render-passes',
      ],
    },
    {
      type: 'category',
      label: 'API Reference',
      items: [
        'api/vulkan-module',
        'api/memory-module',
        'api/image-module',
        'api/pipeline-module',
        'api/raytracing-module',
      ],
    },
  ],
};

export default sidebars;
