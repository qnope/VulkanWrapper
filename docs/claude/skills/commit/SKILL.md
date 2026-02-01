# Commit Skill

This skill guides the creation of professional, well-structured git commits by analyzing differences against `origin/main`.

## Workflow

### 1. Analyze Changes Against origin/main

First, compare the current branch against the development branch:

```bash
# Fetch latest remote state
git fetch origin

# View all commits on current branch not in origin/main
git log origin/main..HEAD --oneline

# View detailed diff against origin/main
git diff origin/main...HEAD

# View summary of changed files
git diff origin/main...HEAD --stat

# If there is any commit: reset them
git reset --mixed origin/main
```

### 2. Categorize Changes

Group changes into logical units based on:

| Category | Prefix | Description |
|----------|--------|-------------|
| New feature | `feat` | New functionality or capability |
| Bug fix | `fix` | Corrects incorrect behavior |
| Refactor | `refactor` | Code restructuring without behavior change |
| Documentation | `docs` | Documentation only changes |
| Tests | `test` | Adding or updating tests |
| Build/config | `chore` | Build system, dependencies, configuration |
| Performance | `perf` | Performance improvements |

### 3. Create Atomic Commits

Each commit should be:

- **Atomic**: One logical change per commit
- **Self-contained**: The codebase should build and tests should pass after each commit
- **Reversible**: Easy to revert if needed

If changes span multiple logical units, create separate commits:

```bash
# Stage specific files for first logical change
git add path/to/related/files

# Commit with detailed message
git commit

# Repeat for next logical change
git add path/to/other/files
git commit
```

### 4. Commit Message Format

Use the following structure:

```
<type>(<scope>): <short summary>

<detailed explanation>

<optional footer>
```

**Short summary** (first line):
- Imperative mood ("add", "fix", "update", not "added", "fixed", "updated")
- Max 50 characters
- No period at the end
- Lowercase after the colon

**Detailed explanation**:
- Explain **why** the change was made, not just what
- Describe the problem being solved
- Mention any trade-offs or design decisions
- Reference related issues or discussions if applicable

**Example:**

```
feat(render): add screen-space reflections pass

Implement SSR using hierarchical ray marching for efficient
screen-space ray tracing. The pass integrates into the deferred
pipeline after the lighting passes.

Key implementation details:
- Uses hierarchical Z-buffer for acceleration
- Fallback to environment map when ray misses
- Configurable quality levels via push constants

The algorithm is based on the technique described in
"Efficient GPU Screen-Space Ray Tracing" (McGuire 2014).
```

### 5. Multi-Commit Workflow

When creating multiple commits from a single set of changes:

```bash
# 1. Review all changes
git diff origin/main...HEAD

# 2. Reset to unstaged state (keep changes)
git reset origin/main

# 3. Stage and commit first logical group
git add -p  # Interactive staging for fine control
git commit

# 4. Stage and commit subsequent groups
git add path/to/next/group
git commit

# 5. Verify commit history
git log origin/main..HEAD --oneline
```

### 6. Quality Checklist

Before finalizing commits:

- [ ] Each commit message clearly explains the **why**
- [ ] Commits are ordered logically (dependencies first)
- [ ] No "WIP", "fixup", or temporary commits
- [ ] No unrelated changes bundled together
- [ ] Sensitive files excluded (.env, credentials, etc.)
- [ ] Build succeeds after each commit
- [ ] Tests pass after each commit

## Common Patterns

### Feature Addition

```
feat(model): add support for PBR metallic-roughness workflow

Enable loading and rendering of glTF 2.0 materials using the
metallic-roughness PBR model. This provides more physically
accurate material representation compared to the previous
Phong-based approach.

- Adds MetallicRoughnessMaterial struct
- Extends BindlessMaterialManager with PBR handler
- Updates shaders to support both workflows
```

### Bug Fix

```
fix(memory): resolve buffer alignment issue on AMD GPUs

The uniform buffer offset was not respecting
minUniformBufferOffsetAlignment on AMD hardware, causing
validation errors and rendering artifacts.

Root cause: alignment calculation used 256 bytes unconditionally
instead of querying device limits.

Fixes #42
```

### Refactoring

```
refactor(pipeline): extract common pipeline configuration

Move shared pipeline state configuration into
GraphicsPipelineDefaults to reduce duplication across
render passes. No functional changes.

This consolidates:
- Viewport/scissor dynamic state
- Standard blend modes
- Common rasterization settings
```

## Anti-Patterns

| Avoid | Prefer |
|-------|--------|
| "Fixed stuff" | "fix(sync): correct fence wait timeout handling" |
| "Updates" | "refactor(image): simplify mipmap generation logic" |
| "WIP commit" | Squash or rebase before pushing |
| Mixing unrelated changes | Separate commits per logical unit |
| Only describing what changed | Explain why the change was needed |

## See Also

- [Conventional Commits](https://www.conventionalcommits.org/)
- Project CLAUDE.md for commit style expectations
