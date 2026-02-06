# Commit Skill

Create atomic, well-structured commits for the VulkanWrapper project.

## Workflow

```bash
# 1. View changes against main
git fetch origin
git diff origin/main...HEAD --stat
git log origin/main..HEAD --oneline

# 2. Format staged files only
git diff --cached --name-only -- '*.h' '*.cpp' | xargs -r clang-format -i

# 3. Build
cmake --build build-Clang20Debug

# 4. Run tests — if tests fail, fix before committing
cd build-Clang20Debug && ctest --output-on-failure

# 5. Stage and commit logical groups
git add specific/files
git commit
```

**If build fails:** Fix compilation errors, re-run step 3.
**If tests fail:** Diagnose with `./TestExecutable --gtest_filter='*FailingTest*'`, fix, re-run steps 3-4.
**If clang-format changes files:** Re-stage the formatted files before committing.

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

## Checklist

- [ ] Each commit is atomic (one logical change)
- [ ] Message explains **why**, not just what
- [ ] No WIP or fixup commits
- [ ] Sensitive files excluded
- [ ] Code formatted with `clang-format` (staged files)
- [ ] Build succeeds
- [ ] Tests pass (`cd build-Clang20Debug && ctest --output-on-failure`)
