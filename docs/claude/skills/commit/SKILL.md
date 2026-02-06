# Commit Skill

Create atomic, well-structured commits for the VulkanWrapper project.

## Workflow

1. **Clean the branch**: `git reset --mixed origin/dev`
2. **Analyse the change**:
   1. Find all atomic logical changes.
   2. For all logical changes, create a professional and detailed commit
 
## What is an Atomic logical change?
An atomic logical change is a change that can be designed as a feature, a fix, or anything that is not linkable to other changes.

## Message Format

```
<type>(<scope>): <summary>

<detailed explanation - why, not what>
```

**Types:** `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `perf`

**Scopes:** `vulkan`, `memory`, `pipeline`, `render`, `rt`, `shader`, `material`, `model`, `image`, `sync`, `descriptor`, `window`, `test`, `build`

**Summary:** Imperative mood, max 50 chars, no period.

## Examples

```
feat(render): add screen-space reflections pass

Implement SSR using hierarchical ray marching.
Uses hierarchical Z-buffer for acceleration.
```

```
fix(memory): resolve buffer alignment on AMD GPUs

Alignment calculation now queries device limits
instead of using hardcoded 256 bytes.

Fixes #42
```

```
refactor(sync): migrate barriers to ResourceTracker

Replace manual vkCmdPipelineBarrier calls with
ResourceTracker track/request/flush pattern for
automatic Synchronization2 barrier generation.
```

```
test(render): add SunLightPass coherence tests

Verify lit areas are brighter than shadowed areas
using relationship-based assertions (no golden images).
```

## Definition of Done

- [ ] Each commit is atomic (one logical change)
- [ ] Message explains **why**, not just what
- [ ] No WIP or fixup commits
- [ ] Sensitive files excluded
- [ ] Build succeeds
- [ ] Tests pass (`cd build-Clang20Debug && ctest --output-on-failure`)
