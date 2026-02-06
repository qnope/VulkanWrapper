# Commit Skill

Create atomic, well-structured commits for the VulkanWrapper project.

## Workflow

```bash
# 1. View changes against main
git fetch origin
git diff origin/main...HEAD --stat
git log origin/main..HEAD --oneline

# 2. Run clang-format on changed files
git diff --name-only -- '*.h' '*.cpp' | xargs -r clang-format -i

# 3. Build and run tests
cmake --build build-Clang20Debug
cd build-Clang20Debug && ctest --output-on-failure

# 4. Stage and commit logical groups
git add specific/files
git commit
```

## Message Format

```
<type>(<scope>): <summary>

<detailed explanation - why, not what>
```

**Types:** `feat`, `fix`, `refactor`, `docs`, `test`, `chore`, `perf`

**Scopes:** `vulkan`, `memory`, `pipeline`, `render`, `rt` (ray tracing), `shader`, `material`, `model`, `image`, `sync` (barriers/ResourceTracker), `descriptor`, `window`, `test`, `build`

**Summary:** Imperative mood, max 50 chars, no period

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

## Checklist

- [ ] Each commit is atomic (one logical change)
- [ ] Message explains **why**, not just what
- [ ] No WIP or fixup commits
- [ ] Sensitive files excluded
- [ ] Code formatted with `clang-format`
- [ ] Tests pass (`cd build-Clang20Debug && ctest --output-on-failure`)
