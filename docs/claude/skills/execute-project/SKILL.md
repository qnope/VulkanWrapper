# Execute Project Skill

## Skill: Execute All Tasks in Directory with Subagent

In `specs/projects/$1/tasks/`, for each task file, spawn a subagent to execute the task. Collect and report the results from all subagents.
You can check the `specs/projects/$1/tasks/Dependency.md` for helping you to parallelize the work.

## Steps
1. For each task file in `specs/projects/$1/tasks/` (excluding SKILL.md), spawn a subagent with the corresponding task file as input. Can be parallelized if there is no dependency between tasks (see `specs/projects/$1/tasks/Dependency.md`).
    - Subagent may check the `specs/architecture/` for helping it to execute the task.
    - The subagent will execute the implementation steps, run the tests, and check that the implementation is correct. If not, it will continue to work on the task until it is done.
    - A task is considered done when all implementation steps are completed, all tests are passing, and the implementation is correct. 
    - When finished, the subagent will update the architecture documentation in `specs/architecture/` and subdirectories if necessary.
    - When finished and commited, the subagent will commit the changes with an appropriate professional commit message.
2. When all tasks are completed, check that all tests are passing and that the implementation is correct. If not, continue to work on them.
3. When everything is done, checked, commited, report the result in a concise way.
4. Move the project to `specs/done/`.
