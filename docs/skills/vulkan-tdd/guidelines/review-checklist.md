# Code Review Checklist

## Pre-Review Checklist

Before requesting review, verify:

- [ ] All tests pass: `cd build-Clang20Debug && ctest`
- [ ] Code compiles without warnings
- [ ] Code is formatted: `clang-format -i <files>`
- [ ] No commented-out code
- [ ] No debug print statements left

## Test Quality Review

### Test Coverage

- [ ] **Happy path tested** - Normal usage scenarios covered
- [ ] **Edge cases tested** - Boundary conditions handled
- [ ] **Error paths tested** - Exceptions and error codes verified
- [ ] **No test interdependence** - Each test is self-contained

### Test Clarity

- [ ] **Descriptive names** - Test name explains what is being tested
- [ ] **Clear assertions** - EXPECT messages explain failures
- [ ] **Minimal setup** - Only necessary setup code
- [ ] **Single concern** - Each test verifies one behavior

### Test Reliability

- [ ] **Deterministic** - No flaky tests
- [ ] **Fast execution** - Individual tests complete quickly
- [ ] **Clean teardown** - Resources properly released
- [ ] **Isolated state** - No global state pollution

## Vulkan-Specific Review

### Resource Management

- [ ] **RAII used** - Resources use RAII wrappers
- [ ] **No leaks** - All allocations have corresponding frees
- [ ] **Proper ownership** - `shared_ptr` vs `unique_ptr` appropriate
- [ ] **Handle validity** - Handles checked before use

### Synchronization

- [ ] **Barriers correct** - Layout transitions valid
- [ ] **Stage masks correct** - Pipeline stages appropriate
- [ ] **Access masks correct** - Read/write access specified
- [ ] **Queue ownership** - Transfers handled correctly

### Pipeline State

- [ ] **Descriptor bindings match** - Layout matches shader
- [ ] **Push constants sized** - Range matches struct
- [ ] **Vertex input matches** - Attributes match shader inputs
- [ ] **Dynamic state set** - Viewport/scissor configured

### Validation Layers

- [ ] **No validation errors** - Clean validation output
- [ ] **No warnings** - Best practices followed
- [ ] **Debug names set** - Objects labeled for debugging

## Code Quality Review

### C++ Best Practices

- [ ] **const correctness** - `const` used appropriately
- [ ] **Reference vs copy** - Large objects passed by reference
- [ ] **Move semantics** - `std::move` for transfers
- [ ] **No raw `new`/`delete`** - Smart pointers used

### Naming Conventions

- [ ] **snake_case** - Functions and variables
- [ ] **PascalCase** - Types and classes
- [ ] **m_ prefix** - Member variables
- [ ] **Descriptive names** - No abbreviations without context

### Code Organization

- [ ] **Single responsibility** - Functions do one thing
- [ ] **DRY** - No duplicated logic
- [ ] **Appropriate abstraction** - Not over/under-engineered
- [ ] **Consistent style** - Matches existing codebase

## API Design Review

### Builder Pattern

- [ ] **Fluent interface** - Methods return `*this`
- [ ] **Sensible defaults** - Common case requires minimal config
- [ ] **Validation** - Invalid configurations caught
- [ ] **Immutable after build** - Built object is complete

### Error Handling

- [ ] **Appropriate exception type** - From vw::Exception hierarchy
- [ ] **Informative messages** - Context included
- [ ] **source_location used** - Error location tracked
- [ ] **No silent failures** - Errors always reported

### Type Safety

- [ ] **Strong types** - Width/Height/MipLevel not raw ints
- [ ] **Concepts used** - Template constraints clear
- [ ] **Compile-time checks** - `static_assert` where possible
- [ ] **No implicit conversions** - Explicit types required

## Performance Review

### Memory

- [ ] **Minimal allocations** - Reuse where possible
- [ ] **Appropriate buffer sizes** - Not over-allocated
- [ ] **Staging buffer pooling** - StagingBufferManager used
- [ ] **Alignment respected** - Uniform buffer alignment

### GPU

- [ ] **Minimal barriers** - ResourceTracker optimizes
- [ ] **Batched operations** - Commands grouped
- [ ] **Appropriate queue** - Graphics vs compute vs transfer
- [ ] **Minimal state changes** - Pipeline/descriptor binds

## Documentation Review

### Code Comments

- [ ] **Why, not what** - Explains intent
- [ ] **No obvious comments** - Code is self-documenting
- [ ] **Updated comments** - Match current code
- [ ] **TODO tracked** - Issues filed for TODOs

### Public API

- [ ] **Header comments** - Public functions documented
- [ ] **Parameter descriptions** - Purpose of each parameter
- [ ] **Return value documented** - What caller receives
- [ ] **Exceptions documented** - What can throw

## Review Response Template

```markdown
## Overall Assessment
[Summary of changes and quality]

## Strengths
- [What was done well]

## Required Changes
- [ ] [Must fix before merge]

## Suggestions
- [Optional improvements]

## Questions
- [Clarifications needed]
```

## Common Review Issues

### Test Issues

| Issue | Example | Fix |
|-------|---------|-----|
| Missing assertion | Test only creates object | Add EXPECT verifying behavior |
| Vague test name | `TestBuffer` | `CreateBufferWithValidSize` |
| Test does too much | Tests 5 behaviors | Split into 5 tests |
| Hardcoded magic numbers | `EXPECT_EQ(x, 42)` | Use named constants |

### Vulkan Issues

| Issue | Example | Fix |
|-------|---------|-----|
| Missing barrier | Image used without transition | Add ResourceTracker request |
| Wrong layout | Shader reads from ColorAttachment | Use ShaderReadOnlyOptimal |
| Stale descriptor | Descriptor bound after update | Bind after all writes |
| Missing wait | CPU reads before GPU done | Add fence/semaphore wait |

### C++ Issues

| Issue | Example | Fix |
|-------|---------|-----|
| Unnecessary copy | `void f(vector<T> v)` | `void f(const vector<T>& v)` |
| Raw pointer ownership | `T* create()` return | Return `unique_ptr<T>` |
| Missing const | `int getValue()` | `int getValue() const` |
| Unused variable | `auto x = compute();` | Remove or use `[[maybe_unused]]` |
