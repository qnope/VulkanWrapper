import clsx from 'clsx';
import Link from '@docusaurus/Link';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';
import Layout from '@theme/Layout';
import Heading from '@theme/Heading';
import styles from './index.module.css';

function HomepageHeader() {
  const { siteConfig } = useDocusaurusContext();
  return (
    <header className={clsx('hero hero--primary', styles.heroBanner)}>
      <div className="container">
        <Heading as="h1" className="hero__title">
          {siteConfig.title}
        </Heading>
        <p className="hero__subtitle">{siteConfig.tagline}</p>
        <div className={styles.buttons}>
          <Link
            className="button button--secondary button--lg"
            to="/docs/getting-started">
            Get Started
          </Link>
          <Link
            className="button button--outline button--secondary button--lg"
            to="/docs/intro"
            style={{ marginLeft: '1rem' }}>
            Learn More
          </Link>
        </div>
      </div>
    </header>
  );
}

const FeatureList = [
  {
    title: 'Type-Safe C++23',
    description: (
      <>
        Leverage modern C++23 features including concepts, ranges, and
        <code>std::span</code> for compile-time type safety. Strong types like
        <code>Width</code>, <code>Height</code>, and <code>MipLevel</code> prevent
        common parameter mistakes.
      </>
    ),
  },
  {
    title: 'RAII Resource Management',
    description: (
      <>
        All Vulkan resources are automatically managed through RAII wrappers.
        No manual cleanup required - resources are freed when they go out of scope,
        preventing leaks and use-after-free bugs.
      </>
    ),
  },
  {
    title: 'Fluent Builder Pattern',
    description: (
      <>
        Construct complex Vulkan objects with intuitive, chainable builder APIs.
        <code>InstanceBuilder</code>, <code>GraphicsPipelineBuilder</code>, and more
        provide clear, readable object creation.
      </>
    ),
  },
  {
    title: 'Modern Vulkan 1.3',
    description: (
      <>
        Built for Vulkan 1.3 with dynamic rendering, synchronization2, and
        hardware ray tracing support. No legacy render pass compatibility layers.
      </>
    ),
  },
  {
    title: 'Automatic Synchronization',
    description: (
      <>
        The <code>ResourceTracker</code> system automatically tracks resource states
        and generates optimal memory barriers. Focus on rendering logic, not
        synchronization details.
      </>
    ),
  },
  {
    title: 'Physical-Based Rendering',
    description: (
      <>
        Complete PBR pipeline with atmospheric scattering, ray-traced lighting,
        and HDR tone mapping. Radiance-based calculations for physically accurate results.
      </>
    ),
  },
];

function Feature({ title, description }) {
  return (
    <div className={clsx('col col--4')}>
      <div className="padding-horiz--md padding-vert--md">
        <Heading as="h3">{title}</Heading>
        <p>{description}</p>
      </div>
    </div>
  );
}

function HomepageFeatures() {
  return (
    <section className={styles.features}>
      <div className="container">
        <div className="row">
          {FeatureList.map((props, idx) => (
            <Feature key={idx} {...props} />
          ))}
        </div>
      </div>
    </section>
  );
}

function CodeExample() {
  return (
    <section className={styles.codeExample}>
      <div className="container">
        <Heading as="h2" className="text--center margin-bottom--lg">
          Simple and Intuitive API
        </Heading>
        <div className="row">
          <div className="col col--6">
            <Heading as="h3">Create a Vulkan Instance</Heading>
            <pre className={styles.codeBlock}>
              <code>{`auto instance = InstanceBuilder()
    .setDebug()
    .setApiVersion(ApiVersion::e13)
    .addExtensions(window.vulkan_extensions())
    .build();`}</code>
            </pre>
          </div>
          <div className="col col--6">
            <Heading as="h3">Type-Safe Buffers</Heading>
            <pre className={styles.codeBlock}>
              <code>{`// Compile-time validated buffer types
using VertexBuffer = Buffer<Vertex,
    false,           // GPU-only
    VertexBufferUsage>;

auto buffer = allocator->allocate<VertexBuffer>(
    vertices.size());`}</code>
            </pre>
          </div>
        </div>
      </div>
    </section>
  );
}

export default function Home() {
  const { siteConfig } = useDocusaurusContext();
  return (
    <Layout
      title={`${siteConfig.title} - Modern C++23 Vulkan Library`}
      description="A modern C++23 library providing high-level abstractions over the Vulkan graphics API with type safety, RAII, and builder patterns.">
      <HomepageHeader />
      <main>
        <HomepageFeatures />
        <CodeExample />
      </main>
    </Layout>
  );
}
