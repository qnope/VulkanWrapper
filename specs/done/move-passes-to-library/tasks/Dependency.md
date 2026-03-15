# Dependency Graph

## Task Overview

| # | Task | Status |
|---|------|--------|
| 1 | Slot System + RenderPass Base + ScreenSpacePass | Not started |
| 2 | Migrate Existing Passes (Sky, ToneMapping, IndirectLight) | Not started |
| 3 | Move ZPass to Library | Not started |
| 4 | Move DirectLightPass to Library | Not started |
| 5 | Move AmbientOcclusionPass to Library | Not started |
| 6 | RenderPipeline | Not started |
| 7 | Update Advanced Example + Cleanup | Not started |

## Dependency Diagram

```
            ┌───────────────────────┐
            │  Task 1: Foundation   │
            │  (Slot, RenderPass,   │
            │   ScreenSpacePass)    │
            └──────────┬────────────┘
                       │
         ┌─────────────┼─────────────────────┐
         │             │                     │
         ▼             ▼                     ▼
  ┌──────────┐  ┌──────────┐         ┌──────────────┐
  │ Task 2:  │  │ Task 3:  │         │   Task 6:    │
  │ Migrate  │  │  ZPass   │         │ RenderPipeline│
  │ existing │  └────┬─────┘         └──────┬───────┘
  │ passes   │       │                      │
  └────┬─────┘       ▼                      │
       │      ┌──────────────┐              │
       │      │   Task 4:    │              │
       │      │ DirectLight  │              │
       │      │    Pass      │              │
       │      └──────┬───────┘              │
       │             │                      │
       │             ▼                      │
       │      ┌──────────────┐              │
       │      │   Task 5:    │              │
       │      │   AO Pass    │              │
       │      └──────┬───────┘              │
       │             │                      │
       └──────┬──────┘──────────────────────┘
              │
              ▼
       ┌──────────────┐
       │   Task 7:    │
       │   Update     │
       │   Example    │
       └──────────────┘
```

## Dependency Details

### Task 1 → Tasks 2, 3, 4, 5, 6
All tasks depend on Task 1 (foundation). The `Slot` enum, `RenderPass` base class, and updated `ScreenSpacePass` must exist before any pass can be migrated or created.

### Task 3 → Task 4
DirectLightPass (Task 4) takes `Slot::Depth` as input, which ZPass (Task 3) produces. While DirectLightPass can be developed in isolation (the slot system is decoupled), end-to-end testing requires ZPass to provide the depth image.

### Task 4 → Task 5
AmbientOcclusionPass (Task 5) takes G-Buffer slots (`Position`, `Normal`, `Tangent`, `Bitangent`) as inputs. These come from DirectLightPass (Task 4). Again, isolated development is possible, but wiring tests need Task 4.

### Tasks 2, 3, 4, 5, 6 → Task 7
Task 7 (update example) requires ALL passes to be in the library and RenderPipeline to be available.

## Parallelization Opportunities

The following tasks can be developed **in parallel** after Task 1 is complete:

- **Task 2** (migrate existing) + **Task 3** (ZPass) + **Task 6** (RenderPipeline) — these are independent
- **Task 4** (DirectLightPass) can start in parallel with Task 2 and Task 6, though it depends on Task 3 for integration
- **Task 5** (AmbientOcclusionPass) can start after Task 4, or in parallel if input images are mocked in tests

## Critical Path

```
Task 1 → Task 3 → Task 4 → Task 5 → Task 7
```

Tasks 2 and 6 run in parallel with the critical path and must complete before Task 7.

## Cross-Task Amendments

Task 6 (RenderPipeline) requires a minor amendment to Task 1:
- Add `virtual void reset_accumulation() {}` (no-op default) to `RenderPass` base class
- Add `virtual std::string_view name() const = 0` to `RenderPass` for validation error messages
