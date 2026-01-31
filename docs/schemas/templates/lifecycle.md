# Resource Lifecycle Diagram Templates

## Template 1: Buffer Lifecycle

Use for documenting buffer creation and usage.

```mermaid
stateDiagram-v2
    [*] --> Created: allocator->create_buffer()

    Created --> Mapped: Host-visible
    Created --> Uploaded: Device-local

    Mapped --> InUse: Write data
    Uploaded --> InUse: Stage & copy

    InUse --> Reading: Shader read
    InUse --> Writing: Shader write
    InUse --> Transfer: Copy operation

    Reading --> InUse
    Writing --> InUse
    Transfer --> InUse

    InUse --> Destroyed: Buffer goes out of scope

    Destroyed --> [*]
```

## Template 2: Image State Transitions

Use for documenting image layout transitions.

```mermaid
stateDiagram-v2
    [*] --> Undefined: Created

    Undefined --> TransferDst: Prepare for upload
    TransferDst --> ShaderReadOnly: After upload

    Undefined --> ColorAttachment: Render target
    ColorAttachment --> ShaderReadOnly: After rendering
    ColorAttachment --> PresentSrc: Before present

    ShaderReadOnly --> ColorAttachment: Render to it
    ShaderReadOnly --> TransferSrc: Read back

    TransferSrc --> ShaderReadOnly: Continue sampling

    PresentSrc --> ColorAttachment: Next frame

    note right of TransferDst: Pipeline barrier required
    note right of ShaderReadOnly: Most common sampling layout
```

## Template 3: Frame Lifecycle

Use for documenting per-frame operations.

```mermaid
flowchart TB
    subgraph "Frame Start"
        Acquire[Acquire Swapchain Image]
        Wait[Wait for Fence]
        Reset[Reset Command Buffer]
    end

    subgraph "Recording"
        Begin[Begin Command Buffer]
        Barriers1[Pre-render Barriers]
        Render[Render Passes]
        Barriers2[Post-render Barriers]
        End[End Command Buffer]
    end

    subgraph "Submission"
        Submit[Submit to Queue]
        Signal[Signal Semaphore]
    end

    subgraph "Presentation"
        Present[Present to Swapchain]
    end

    Acquire --> Wait
    Wait --> Reset
    Reset --> Begin
    Begin --> Barriers1
    Barriers1 --> Render
    Render --> Barriers2
    Barriers2 --> End
    End --> Submit
    Submit --> Signal
    Signal --> Present
    Present -.-> |Next Frame| Acquire
```

## Template 4: Object Creation Pattern

Use for documenting builder patterns.

```mermaid
flowchart LR
    subgraph "Builder Pattern"
        Create[Create Builder]
        Config1[Configure Option 1]
        Config2[Configure Option 2]
        Config3[Configure Option N]
        Build[Build Object]
    end

    subgraph "Validation"
        Validate[Validate Config]
        Error{Valid?}
    end

    subgraph "Result"
        Object[Vulkan Object]
        Exception[Throw Exception]
    end

    Create --> Config1
    Config1 --> Config2
    Config2 --> Config3
    Config3 --> Build
    Build --> Validate
    Validate --> Error
    Error -->|Yes| Object
    Error -->|No| Exception
```

## Template 5: RAII Ownership

Use for documenting resource ownership.

```mermaid
flowchart TB
    subgraph "Unique Ownership"
        U1[Create Object]
        U2[Use Object]
        U3[Scope Exit]
        U4[Auto Destroy]
    end

    subgraph "Shared Ownership"
        S1[Create shared_ptr]
        S2[Share with Components]
        S3[Last Reference Gone]
        S4[Auto Destroy]
    end

    U1 --> U2
    U2 --> U3
    U3 --> U4

    S1 --> S2
    S2 --> S3
    S3 --> S4

    note1[unique_ptr: Single owner]
    note2[shared_ptr: Multiple owners]
```

## Template 6: Synchronization Lifecycle

Use for documenting sync primitive usage.

```mermaid
sequenceDiagram
    participant App
    participant Fence
    participant GPU

    App->>Fence: Create (unsignaled)
    App->>GPU: Submit work + fence
    GPU->>GPU: Execute commands
    GPU->>Fence: Signal

    App->>Fence: Wait
    Fence-->>App: Return when signaled

    App->>Fence: Reset
    Note over Fence: Ready for reuse
```

## Customization Guide

1. **Use state diagrams** for object states
2. **Use flowcharts** for creation sequences
3. **Use sequence diagrams** for temporal interactions
4. **Add notes** for important details
5. **Show error paths** with dashed lines
