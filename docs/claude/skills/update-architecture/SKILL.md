# SKILL.md

## Skill: Update Architecture from Recent Commits

### Overview

This skill updates the project architecture documentation in `specs/architecture/`, `specs/architecture/tests/`,  `specs/architecture/performance/`  and `specs/architecture/examples/` based on recent commits to the codebase. It analyzes the commit history to identify changes that impact the architecture and updates the relevant documentation accordingly.

First thing to do is to display "Architecture documentation update" in the console, so that the user knows what is going on.

If an element is removed from the codebase, it should be removed from the architecture documentation. If an element is added, it should be added to the architecture documentation. If an element is modified, the architecture documentation should be updated to reflect the modification.
You will not keep history of adding things or not, the specification should always reflect the current state of the codebase, not the history of how it evolved.

## What the README file contain

1. **Overview**: A brief description of the component or module, its purpose, and its role within the overall architecture.
2. **How to use**: A detailed explanation of how to use the component, including any necessary setup, configuration, or dependencies.
3. **Diagrams**: If applicable, include diagrams to illustrate the architecture, such as component diagrams, sequence diagrams, or flowcharts.
4. **Conciseness**: The README should be concise and focused on the most important information. Avoid unnecessary details or tangential topics that may distract from the main points. 50 lines is a good target for the length of the README file, but it can be longer if necessary to cover all relevant information. Never go above 100 lines. It's better to write:
```markdown
to configure: `python configure.py --option1 value1 --option2 value2`
```
instead of:
```markdown
To configure the project, you can run the following command in your terminal:
```bash
python configure.py --option1 value1 --option2 value2
```
```

