# Commit Skill

Create atomic, well-structured commits.

## Workflow

```bash
# 1. View changes against main
git fetch origin
git diff origin/main...HEAD --stat
git log origin/main..HEAD --oneline

# 2. Run clang-format on changed files
git diff --name-only origin/main...HEAD -- '*.h' '*.cpp' | xargs clang-format -i

# 3. Run tests
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

## Checklist

- [ ] Each commit is atomic (one logical change)
- [ ] Message explains **why**, not just what
- [ ] No WIP or fixup commits
- [ ] Sensitive files excluded
- [ ] Code formatted with `clang-format`
- [ ] Tests pass
