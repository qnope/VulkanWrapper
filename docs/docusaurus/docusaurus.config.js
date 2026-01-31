// @ts-check
import {themes as prismThemes} from 'prism-react-renderer';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'VulkanWrapper',
  tagline: 'Modern C++23 Vulkan Abstraction Library',
  favicon: 'img/favicon.ico',

  url: 'https://vulkanwrapper.dev',
  baseUrl: '/',

  organizationName: 'vulkanwrapper',
  projectName: 'vulkanwrapper',

  onBrokenLinks: 'throw',
  onBrokenMarkdownLinks: 'warn',

  i18n: {
    defaultLocale: 'en',
    locales: ['en'],
  },

  markdown: {
    mermaid: true,
  },

  themes: ['@docusaurus/theme-mermaid'],

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          sidebarPath: './sidebars.js',
          editUrl: 'https://github.com/vulkanwrapper/vulkanwrapper/tree/main/docs/docusaurus/',
        },
        blog: false,
        theme: {
          customCss: './src/css/custom.css',
        },
      }),
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      image: 'img/vulkanwrapper-social-card.png',
      navbar: {
        title: 'VulkanWrapper',
        logo: {
          alt: 'VulkanWrapper Logo',
          src: 'img/logo.svg',
        },
        items: [
          {
            type: 'docSidebar',
            sidebarId: 'tutorialSidebar',
            position: 'left',
            label: 'Documentation',
          },
          {
            href: 'https://github.com/vulkanwrapper/vulkanwrapper',
            label: 'GitHub',
            position: 'right',
          },
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Docs',
            items: [
              {
                label: 'Getting Started',
                to: '/docs/getting-started/introduction',
              },
              {
                label: 'Tutorials',
                to: '/docs/tutorials/hello-triangle',
              },
            ],
          },
          {
            title: 'More',
            items: [
              {
                label: 'GitHub',
                href: 'https://github.com/vulkanwrapper/vulkanwrapper',
              },
            ],
          },
        ],
        copyright: `Copyright ${new Date().getFullYear()} VulkanWrapper. Built with Docusaurus.`,
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['cpp', 'cmake', 'bash', 'glsl'],
      },
      mermaid: {
        theme: {light: 'neutral', dark: 'dark'},
      },
    }),
};

export default config;
