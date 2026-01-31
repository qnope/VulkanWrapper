# Pipeline Flow Diagram Templates

## Template 1: Graphics Pipeline Stages

Use for showing GPU pipeline stages.

```mermaid
flowchart LR
    subgraph "Input Assembly"
        VB[Vertex Buffer]
        IB[Index Buffer]
    end

    subgraph "Vertex Processing"
        VS[Vertex Shader]
        TCS[Tessellation Control]
        TES[Tessellation Eval]
        GS[Geometry Shader]
    end

    subgraph "Rasterization"
        Clip[Clipping]
        Cull[Culling]
        Rast[Rasterizer]
    end

    subgraph "Fragment Processing"
        FS[Fragment Shader]
        Depth[Depth Test]
        Blend[Blending]
    end

    subgraph "Output"
        FB[Framebuffer]
    end

    VB --> VS
    IB --> VS
    VS --> TCS
    TCS --> TES
    TES --> GS
    GS --> Clip
    Clip --> Cull
    Cull --> Rast
    Rast --> FS
    FS --> Depth
    Depth --> Blend
    Blend --> FB
```

## Template 2: Deferred Rendering Pipeline

Use for multi-pass rendering documentation.

```mermaid
flowchart TB
    subgraph "Geometry Pass"
        GInput[Scene Geometry]
        GBuffer[G-Buffer]
    end

    subgraph "G-Buffer Contents"
        Position[Position]
        Normal[Normal]
        Albedo[Albedo]
        Material[Material ID]
    end

    subgraph "Lighting Pass"
        LInput[G-Buffer Read]
        Lights[Process Lights]
        LOutput[HDR Buffer]
    end

    subgraph "Post-Processing"
        Bloom[Bloom]
        ToneMap[Tone Mapping]
        Final[Final Output]
    end

    GInput --> GBuffer
    GBuffer --> Position
    GBuffer --> Normal
    GBuffer --> Albedo
    GBuffer --> Material

    Position --> LInput
    Normal --> LInput
    Albedo --> LInput
    Material --> LInput

    LInput --> Lights
    Lights --> LOutput
    LOutput --> Bloom
    Bloom --> ToneMap
    ToneMap --> Final
```

## Template 3: Ray Tracing Pipeline

Use for RT shader documentation.

```mermaid
flowchart TB
    subgraph "Acceleration Structures"
        BLAS[BLAS<br/>Per-Mesh]
        TLAS[TLAS<br/>Scene]
    end

    subgraph "Ray Generation"
        Camera[Camera Rays]
        RayGen[Ray Generation Shader]
    end

    subgraph "Traversal"
        Traverse[BVH Traversal]
        Hit{Hit?}
    end

    subgraph "Shaders"
        AnyHit[Any Hit Shader]
        ClosestHit[Closest Hit Shader]
        Miss[Miss Shader]
    end

    subgraph "Output"
        Result[Ray Result]
        Accumulate[Accumulate]
    end

    BLAS --> TLAS
    Camera --> RayGen
    RayGen --> Traverse
    TLAS --> Traverse
    Traverse --> Hit
    Hit -->|Yes| AnyHit
    AnyHit --> ClosestHit
    ClosestHit --> Result
    Hit -->|No| Miss
    Miss --> Result
    Result --> Accumulate
```

## Template 4: Compute Pipeline

Use for compute shader documentation.

```mermaid
flowchart LR
    subgraph "Input"
        InBuffer[Input Buffer]
        InImage[Input Image]
        Uniforms[Uniforms]
    end

    subgraph "Dispatch"
        Workgroups[Workgroups<br/>X × Y × Z]
        Invocations[Invocations<br/>Local Size]
    end

    subgraph "Compute Shader"
        Load[Load Data]
        Process[Process]
        Barrier[Barrier]
        Store[Store Result]
    end

    subgraph "Output"
        OutBuffer[Output Buffer]
        OutImage[Output Image]
    end

    InBuffer --> Load
    InImage --> Load
    Uniforms --> Load
    Workgroups --> Invocations
    Invocations --> Load
    Load --> Process
    Process --> Barrier
    Barrier --> Store
    Store --> OutBuffer
    Store --> OutImage
```

## Template 5: Data Flow Through Passes

Use for multi-pass rendering.

```mermaid
flowchart LR
    subgraph "Pass 1: Z-Prepass"
        Z1[Scene] --> Z2[Depth Only]
    end

    subgraph "Pass 2: G-Buffer"
        G1[Scene] --> G2[Color + Normal]
    end

    subgraph "Pass 3: SSAO"
        S1[Depth + Normal] --> S2[AO Map]
    end

    subgraph "Pass 4: Lighting"
        L1[G-Buffer + AO] --> L2[Lit Result]
    end

    subgraph "Pass 5: Post"
        P1[Lit Result] --> P2[Final]
    end

    Z2 --> G1
    G2 --> S1
    Z2 --> S1
    S2 --> L1
    G2 --> L1
    L2 --> P1
```

## Customization Guide

1. **Use subgraphs** to group pipeline stages
2. **Show data flow** with arrows
3. **Add conditional branches** for hit/miss scenarios
4. **Include buffers/images** as inputs/outputs
5. **Use colors** via CSS classes for emphasis
