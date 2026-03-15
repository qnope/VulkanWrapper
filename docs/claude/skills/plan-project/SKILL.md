# Plan Project Skill

## Skill: Plan Project from SPEC.md

### Purpose

From `specs/projects/$1/SPEC.md`, create detailed implementation plans for each major task or module. Each plan will be saved as `specs/projects/$1/tasks/<task_number>_<task_name>.md` and will include actionable steps, dependencies, and test plans.

Each plan should be specific and small enough to be actionable by a single developer. If a task is too large, break it down into smaller sub-tasks with their own plans.

You can check in `specs/architecture/` for helping you to write the plan.

If questions arise, ask the user for clarification using the AskUserQuestion tool. Always ask questions in interactive mode, never as plain text. You can ask several questions and propose for each of them a set of answer or a custom one.

### Outputs

- Each `<task_number>_<task_name>.md` contains:
  - Task/module summary
  - Implementation steps (with file paths, class/function names)
  - CMake registration notes
  - Dependencies (internal and external)
  - Test plan (test file locations, test cases)
  - Any special notes or design considerations
- One `Dependency.md` file summarizing all dependencies between tasks/modules.

When the tasks are ready, write to the user `You can launch /execute-project <project_name> to execute the project now or later.`.