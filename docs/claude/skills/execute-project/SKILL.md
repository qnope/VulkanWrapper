# Execute Project Skill

## Skill: Execute All Tasks in Directory with Subagent

In `specs/projects/$1/tasks/`, for each task file (excluding SKILL.md), spawn a subagent to execute the task. Collect and report the results from all subagents.
You can check the `specs/projects/$1/tasks/Dependency.md` for helping you to parralelize the work.
You can check the `specs/architecture/` for helping you to execute the task.

## Steps
1. For each task file in `specs/projects/$1/tasks/` (excluding SKILL.md), spawn a subagent with the corresponding task file as input. Can be parralelized if there is no dependency between tasks (see `specs/projects/$1/tasks/Dependency.md`).
2. When all subagents have completed, check that all tests are passing and that the implementation is correct.